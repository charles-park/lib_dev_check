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
//
// Configuration
//
//------------------------------------------------------------------------------
#define IR_EVENT_PASS   5
/*
    https://wiki.odroid.com/odroid-c4/application_note/lirc/lirc_ubuntu18.04#tab__odroid-c2n2c4hc4
*/
const char *IR_EVENT_PATH = "/dev/input/event2";

int IR_EVENT_COUNT = 0, IR_KEY_CODE = 0;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
pthread_t thread_ir;

void *thread_func_ir (void *arg)
{
    struct input_event event;
    struct timeval  timeout;
    fd_set readFds;
    static int fd = -1;

    // IR Device Name
    // /sys/class/input/event0/device/name -> fdd70030.pwm
    if ((fd = open(IR_EVENT_PATH, O_RDONLY)) < 0) {
        printf ("%s : %s error!\n", __func__, IR_EVENT_PATH);
        return arg;
    }
    IR_EVENT_COUNT = 0, IR_KEY_CODE = 0;
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
                                IR_EVENT_COUNT++;
                                IR_KEY_CODE = event.code;
                                printf ("IR_EVENT_COUNT = %d, IR_KEY_CODE = %d\n",
                                            IR_EVENT_COUNT, IR_KEY_CODE);
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
//------------------------------------------------------------------------------
int ir_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eIR_EVENT:
            status = (IR_EVENT_COUNT > IR_EVENT_PASS) ? 1 : 0;
            value  =  IR_KEY_CODE;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'C', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int ir_grp_init (void)
{
    pthread_create (&thread_ir, NULL, thread_func_ir, NULL);
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
