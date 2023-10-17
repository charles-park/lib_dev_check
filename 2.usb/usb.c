//------------------------------------------------------------------------------
/**
 * @file usb.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 0.2
 * @date 2023-10-12
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
    // Control path
    const char *path;
    // compare value (read min, write min : MB/s)
    const int r_min, w_min;
    // Link speed
    const int speed;
    // read value (Init)
    int value;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define usb devices (USB3.0 eMMC reader with eMMC) */
//------------------------------------------------------------------------------
struct device_usb DeviceUSB [eUSB_END] = {
    // path, r_min(MB/s), w_min(MB/s), link, read
    // eUSB_30, USB 3.0
    { "/sys/bus/usb/devices/6-1", 100, 35, 5000, 0 },
    // eUSB_20, USB 2.0
    { "/sys/bus/usb/devices/2-1",  25, 20,  480, 0 },
    // eUSB_OTG, USB OTG (Micro USB)
    { "/sys/bus/usb/devices/7-1",  25, 20,  480, 0 },
    // eUSB_EXTRA, Extra 14 Pin Header (USB2.0)
    { "/sys/bus/usb/devices/1-1",  25, 20,  480, 0 },
};

// USB Read / Write (16 Mbytes, 1 block count)
#define USB_R_CHECK "dd of=/dev/null bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync if=/dev/"
#define USB_W_CHECK "dd if=/dev/zero bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync of=/dev/"

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
static int usb_rw (const char *path, const char *check_cmd)
{
    FILE *fp;
    char cmd[STR_PATH_LENGTH], rdata[STR_PATH_LENGTH], *ptr;

    memset  (cmd, 0x00, sizeof(cmd));
    sprintf (cmd, "find %s/ -name sd* 2>&1", path);

    if ((fp = popen (cmd, "r")) != NULL) {
        memset (rdata, 0x00, sizeof(cmd));
        // 1 line read
        fgets (rdata, sizeof(rdata), fp);
        fclose (fp);
        // find string "sd"
        if ((ptr = strstr (rdata, "sd")) != NULL) {
            memset  (cmd, 0, sizeof (cmd));
            // 0x00 -> 0x20 (space)
            ptr[strlen(ptr)-1] = ' ';
            sprintf (cmd, "%s%s 2>&1", check_cmd, ptr);

            if ((fp = popen(cmd, "r")) != NULL) {
                while (1) {
                    memset (rdata, 0, sizeof (rdata));
                    if ((fgets (rdata, sizeof (rdata), fp)) == NULL)
                        break;
                    if ((ptr = strstr (rdata, " MB/s")) != NULL) {
                        while (*ptr != ',') ptr--;
                        return atoi (ptr+1);
                    }
                }
                pclose (fp);
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
int usb_check (int id, char action, char *resp)
{
    int value = 0;

    if ((id >= eUSB_END) || ((access (DeviceUSB[id].path, R_OK)) != 0)) {
        sprintf (resp, "%06d", 0);
        return 0;
    }

    switch (action) {
        case 'I':   case 'R':
            if (action == 'I')
                value = DeviceUSB[id].value;
            else
                value = usb_rw (DeviceUSB[id].path, USB_R_CHECK);

            if (value < DeviceUSB[id].r_min)
                value = 0;
            break;
        case 'W':
            value = usb_rw (DeviceUSB[id].path, USB_W_CHECK);
            if (value < DeviceUSB[id].w_min)
                value = 0;
            break;
        case 'L':
            value = usb_speed (DeviceUSB[id].path);
            if (value != DeviceUSB[id].speed)
                value = 0;
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", value);
    return value ? 1 : 0;
}

//------------------------------------------------------------------------------
int usb_grp_init (void)
{
    int i;

    for (i = 0; i < eUSB_END; i++) {
        if ((access (DeviceUSB[i].path, R_OK)) == 0)
            DeviceUSB[i].value = usb_rw (DeviceUSB[i].path, USB_R_CHECK);
    }

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
