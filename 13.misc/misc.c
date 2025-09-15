//------------------------------------------------------------------------------
/**
 * @file misc.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2025-08-01
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
#include "misc.h"

//------------------------------------------------------------------------------
static char spi_bt_path [STR_PATH_LENGTH] = { 0, };
static char spi_pass_str[STR_NAME_LENGTH] = { 0, };

static char hpdet_str [STR_NAME_LENGTH] = { 0, };

//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = tolower(*p);
}

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
pthread_t thread_id0;   // SPI BT
volatile int BTPress = 0, BTRelease = 0;

void *thread_func_id0 (void *arg)
{
    FILE *fd;
    int prev_state = -1, state = 0;
    char rdata[STR_PATH_LENGTH];

    while (1) {
        if ((fd = fopen(spi_bt_path, "r")) == NULL) {
            printf ("%s : %s error!\n", __func__, spi_bt_path);
            return arg;
        }

        memset (rdata, 0, sizeof(rdata));
        if (NULL != fgets (rdata, sizeof(rdata), fd)) {
            tolowerstr (rdata);
            if (NULL != strstr (rdata, spi_pass_str))   state = 0;
            else                                        state = 1;

            if (prev_state == -1)
                prev_state = state;

            if (prev_state != state) {
                prev_state  = state;
                if (state)  BTPress   = 1;
                else        BTRelease = 1;
            }
        }
        fclose (fd);
        usleep (300 * 1000);

        if (BTRelease && BTPress)   break;
    }
    return arg;
}

//------------------------------------------------------------------------------
pthread_t thread_id1;   // HP DETECT
volatile int HPDetIn = 0, HPDetOut = 0;

int test_bit(int bit, const unsigned long *array) {
    return (array[bit / (8 * sizeof(unsigned long))] >> (bit % (8 * sizeof(unsigned long)))) & 1;
}

void *thread_func_id1 (void *arg)
{
    struct input_event event;
    struct timeval  timeout;
    fd_set readFds;
    int fd = -1, prev_state;
    char path[STR_PATH_LENGTH] = { 0, };

    // IR Device Name (meson-ir)
    sprintf (path, "/dev/input/event%d", find_event (hpdet_str));

    if ((fd = open(path, O_RDONLY)) < 0) {
        printf ("%s : %s error!\n", __func__, path);
        return arg;
    }

    {
        unsigned long sw_bits[(SW_MAX / (8 * sizeof(unsigned long))) + 1];

        memset(sw_bits, 0, sizeof(sw_bits));

        // ioctl로 EV_SW 상태 읽기
        if (ioctl(fd, EVIOCGSW(sizeof(sw_bits)), sw_bits) == -1) {
            perror("EVIOCGSW failed");
            close(fd);
            return arg;
        }

        if (test_bit(SW_HEADPHONE_INSERT, sw_bits)) prev_state = 1;
        else                                        prev_state = 0;;

        /*
        printf("현재 SW 상태:\n");
        for (int i = 0; i <= SW_MAX; ++i) {
            if (test_bit(i, sw_bits)) {
                printf("  SW code %d (0x%02x): ON\n", i, i);
            }
        }
        */
    }

    while (1) {
        // recive time out config
        // Set 1ms timeout counter
        timeout.tv_sec  = 0;
        // timeout.tv_usec = timeout_ms*1000;
        timeout.tv_usec = 300000;

        FD_ZERO(&readFds);
        FD_SET(fd, &readFds);
        select(fd+1, &readFds, NULL, NULL, &timeout);

        if(FD_ISSET(fd, &readFds))
        {
            if(fd && read(fd, &event, sizeof(struct input_event))) {

                switch (event.type) {
                    case EV_SYN:
                        break;
                    case EV_SW:
                        switch (event.code) {
                            case SW_HEADPHONE_INSERT:
                                if (prev_state != event.value) {
                                    prev_state  = event.value;
                                    if (event.value)    HPDetIn = 1;
                                    else                HPDetOut = 1;
                                }

                                printf ("%s : value = %d\n", __func__, event.value);
                                break;
                            case SW_MICROPHONE_INSERT:
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
        if (HPDetIn && HPDetOut)    break;
    }
    close (fd);
    return arg;
}

//------------------------------------------------------------------------------
int misc_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eMISC_ID0:
            status = DEVICE_ACTION(dev_id) ? BTPress : BTRelease;
            value  = status;
            break;

        case eMISC_ID1:
            status = DEVICE_ACTION(dev_id) ? HPDetIn : HPDetOut;
            value  = status;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'C', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void misc_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eMISC_ID0:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (spi_bt_path, tok, strlen(tok));

                    if ((tok = strtok (NULL, ",")) != NULL) {
                        strncpy    (spi_pass_str, tok, strlen(tok));
                        tolowerstr (spi_pass_str);
                    }

                    pthread_create (&thread_id0, NULL, thread_func_id0, NULL);
                    break;
                case eMISC_ID1:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (hpdet_str, tok, strlen(tok));

                    pthread_create (&thread_id1, NULL, thread_func_id1, NULL);
                    break;
                default :
                    break;
            }
        }
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
