//------------------------------------------------------------------------------
/**
 * @file usb.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-20
 *
 * @package apt install iperf3, nmap, ethtool, usbutils, alsa-utils
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
#include <getopt.h>
#include <pthread.h>
#include <sys/sysinfo.h>

//------------------------------------------------------------------------------
#include "../lib_dev_check.h"
#include "usb.h"

//------------------------------------------------------------------------------
struct device_usb {
    // Link speed
    int speed;
    // Control path
    char path[STR_PATH_LENGTH +1];
    // compare value (read min, write min : MB/s)
    int rw_check[2];
    // r/w value
    int rw_value[2];

    // r/w mode
    int rw;

    // r/w thread
    int thread_en;
    pthread_t thread;
};

pthread_mutex_t mutex_usb = PTHREAD_MUTEX_INITIALIZER;

//------------------------------------------------------------------------------
//
// Configuration (ODROID-C5)
//
//------------------------------------------------------------------------------
//           ------------  ------------
//           | USB_L_UP |  | USB_R_UP |
//  -------  |-----------  ------------               -----------
//  | ETH |  | USB_L_DN |  | USB_R_DN |               | USB-OTG |
//  -------  ------------  ------------               -----------
//------------------------------------------------------------------------------
struct device_usb DeviceUSB [eUSB_END] = {
    // init, speed, path, rw_check, rw_value, rw_mode, thread_en, pthread,
    // USB0-OTG
    { 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // USB1 - USB_L_DN
    { 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // USB2 - USB_L_UP
    { 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // USB3 - USB_R_DN
    { 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // USB4 - USB_R_UP
    { 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // USB5
    { 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
};

//------------------------------------------------------------------------------
// USB Read / Write (16 Mbytes, 1 block count)
//------------------------------------------------------------------------------
const char *USB_R_CHECK = "dd of=/dev/null bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync if=/dev/";
const char *USB_W_CHECK = "dd if=/dev/zero bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync of=/dev/";

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int usb_speed (const char *path)
{
    FILE *fp;
    char cmd[STR_PATH_LENGTH], rdata[STR_PATH_LENGTH];

    if (access (path, R_OK) == 0) {
        memset  (cmd, 0x00, sizeof(cmd));
        sprintf (cmd, "%s/speed", path);
        if ((fp = fopen (cmd, "r")) != NULL) {
            memset (rdata, 0, sizeof(rdata));
            fgets  (rdata, sizeof(rdata), fp);
            fclose (fp);
        }
        return atoi (rdata);
    }
    return 0;
}

//------------------------------------------------------------------------------
static int usb_rw (struct device_usb *p_usb)
{
    FILE *fp;
    char cmd[STR_PATH_LENGTH*2], rdata[STR_PATH_LENGTH], *ptr;

    if (!access (p_usb->path, F_OK)) {
        memset  (cmd, 0x00, sizeof(cmd));
        sprintf (cmd, "find %s/ -name sd* 2>&1", p_usb->path);

        if ((fp = popen (cmd, "r")) != NULL) {
            memset (rdata, 0x00, sizeof(cmd));
            // 1 line read
            fgets (rdata, sizeof(rdata), fp);
            pclose (fp);

            // find string "sd"
            if ((ptr = strstr (rdata, "sd")) != NULL) {
                memset  (cmd, 0, sizeof (cmd));
                // 0x00 -> 0x20 (space)
                ptr[strlen(ptr)-1] = ' ';
                sprintf (cmd, "%s%s 2>&1",
                    p_usb->rw ? USB_W_CHECK : USB_R_CHECK, ptr);

                if ((fp = popen(cmd, "r")) != NULL) {
                    while (1) {
                        memset (rdata, 0, sizeof (rdata));
                        if ((fgets (rdata, sizeof (rdata), fp)) == NULL)
                            break;
                        if ((ptr = strstr (rdata, " MB/s")) != NULL) {
                            while (*ptr != ',') ptr--;
                            pclose (fp);
                            return atoi (ptr+1);
                        }
                    }
                    pclose (fp);
                }
            }
        }
        return 0;
    }
    return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void *thread_func_usb (void *arg)
{
    struct device_usb *p_usb = (struct device_usb *)arg;
    int retry = 5;

    p_usb->thread_en = 1;
    while (retry--) {
        pthread_mutex_lock(&mutex_usb);

        if (p_usb->rw_value[p_usb->rw] <= p_usb->rw_check[p_usb->rw])
            p_usb->rw_value[p_usb->rw] = usb_rw (p_usb);

        pthread_mutex_unlock(&mutex_usb);

        if (p_usb->rw_value[p_usb->rw] > p_usb->rw_check[p_usb->rw])
            break;

        usleep (100 * 1000);
    }
    p_usb->thread_en = 0;
    return arg;
}

//------------------------------------------------------------------------------
int usb_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);
    struct device_usb *p_usb = &DeviceUSB[id];

    switch (id) {
        case eUSB_0: case eUSB_1: case eUSB_2:
        case eUSB_3: case eUSB_4: case eUSB_5:

            if ((DEVICE_ACTION(dev_id) == 0) || (DEVICE_ACTION(dev_id) == 1)) {
                do { usleep (100 * 1000); } while (p_usb->thread_en);

                p_usb->rw = DEVICE_ACTION(dev_id);

                pthread_create (&p_usb->thread, NULL,
                                    thread_func_usb, p_usb);

                do { usleep (100 * 1000); } while (p_usb->thread_en);

                value  = p_usb->rw_value[p_usb->rw];
                status = (value > p_usb->rw_check[p_usb->rw]) ? 1 : -1;
            } else {
                value  = usb_speed (p_usb->path);
                status = (value == p_usb->speed) ? 1 : -1;
            }
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void usb_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eUSB_0: case eUSB_1: case eUSB_2:
                case eUSB_3: case eUSB_4: case eUSB_5:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceUSB[did].path, tok, strlen(tok));

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceUSB[did].rw_check[0] = atoi(tok);

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceUSB[did].rw_check[1] = atoi(tok);

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceUSB[did].speed = atoi(tok);
                    break;

                default :
                    printf ("%s : error! unknown did = %d\n", __func__, atoi(tok));
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
