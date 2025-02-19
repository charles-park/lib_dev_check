//------------------------------------------------------------------------------
/**
 * @file ir.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-21
 *
 * @package apt install iperf3, nmap, ethtool, usbutils, alsa-utils, evtest
 *
 * @copyright Copyright (c) 2022
 *
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/sysinfo.h>

//------------------------------------------------------------------------------
#include "../lib_dev_check.h"
#include "ir.h"

//------------------------------------------------------------------------------
struct device_ir {
    char path  [STR_PATH_LENGTH];
    // find string : udevadm info -a -n {path} | grep {f_str}
    char f_str [STR_NAME_LENGTH];

    int pass_count;
    int pass_key_code;

    int key_code;
    int key_count;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_ir DeviceIR = {
    { 0, }, { 0, }, 0, 0, 0, 0
};

//------------------------------------------------------------------------------
static int find_event (const char *f_str)
{
    FILE *fp;
    char cmd  [STR_PATH_LENGTH];
    int ev_num = 0;

    /*
        find /dev/input -name event*        udevadm info -a -n /dev/input/event3 | grep 0705
    */
    while (1) {
        memset  (cmd, 0, sizeof(cmd));
        sprintf (cmd, "/dev/input/event%d", ev_num);
        if (access  (cmd, F_OK))    return -1;

        memset  (cmd, 0, sizeof(cmd));
        sprintf (cmd, "udevadm info -a -n /dev/input/event%d | grep %s", ev_num, f_str);
        if ((fp = popen(cmd, "r")) != NULL) {
            memset (cmd, 0x00, sizeof(cmd));
            while (fgets (cmd, sizeof(cmd), fp) != NULL) {
                if (strstr (cmd, f_str) != NULL) {
                    return ev_num;
                    pclose (fp);
                }
            }
        }
        ev_num++;
    }
}

//------------------------------------------------------------------------------
pthread_t thread_ir;

void *thread_func_ir (void *arg)
{
    struct input_event event;
    struct timeval  timeout;
    fd_set readFds;
    static int fd = -1;
    struct device_ir *ir = (struct device_ir *)arg;

    // IR Device Name (meson-ir)
    sprintf (ir->path, "/dev/input/event%d", find_event (ir->f_str));

    if ((fd = open(ir->path, O_RDONLY)) < 0) {
        printf ("%s : %s error!\n", __func__, ir->path);
        return arg;
    }

    while (1) {
        // recive time out config
        // Set 1ms timeout counter
        timeout.tv_sec  = 0;
        // timeout.tv_usec = timeout_ms*1000;
        timeout.tv_usec = 100000;

        FD_ZERO(&readFds);
        FD_SET(fd, &readFds);
        select(fd+1, &readFds, NULL, NULL, &timeout);

        if(FD_ISSET(fd, &readFds))
        {
            if(fd && read(fd, &event, sizeof(struct input_event))) {

                switch (event.type) {
                    case EV_SYN:
                        break;
                    case EV_KEY:
                        switch (event.code) {
                            /* emergency stop */
                            case KEY_MUTE:  case KEY_HOME:  case KEY_VOLUMEDOWN: case KEY_VOLUMEUP:
                            case KEY_MENU:  case KEY_UP:    case KEY_DOWN:       case KEY_LEFT:
                            case KEY_RIGHT: case KEY_ENTER: case KEY_BACK:
                                if (ir->pass_key_code && (ir->pass_key_code == event.code))
                                    ir->key_count++;
                                else
                                    ir->key_count++;

                                ir->key_code = event.code;
                                printf ("IR_EVENT_COUNT = %d, IR_KEY_CODE = %d\n",
                                    ir->key_count, ir->key_code);
                                break;
                            default :
                                break;
                        }
                        break;
                    default :
                        printf("unknown event\n");
                        break;
                }
            }
        }
    }
    return arg;
}

//------------------------------------------------------------------------------
int ir_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eIR_ID0:
            status = (DeviceIR.key_count > DeviceIR.pass_count) ? 1 : 0;
            value  =  DeviceIR.key_code;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'C', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void ir_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eIR_ID0:
                    break;
                case eIR_CFG:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceIR.f_str, tok, strlen(tok));

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceIR.pass_count = atoi (tok);

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceIR.pass_key_code = atoi (tok);

                    pthread_create (&thread_ir, NULL, thread_func_ir, &DeviceIR);
                    break;
                default :
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
