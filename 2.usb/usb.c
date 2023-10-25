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
    char path[STR_PATH_LENGTH +1];
    // compare value (read min, write min : MB/s)
    int r_min, w_min;
    // Link speed
    int speed;

    // read value (Init)
    int value;
};

// default link speed, read/write (MB/s)
#define DEFAULT_USB30_L 5000
#define DEFAULT_USB30_R 100
#define DEFAULT_USB30_W 35

#define DEFAULT_USB20_L 480
#define DEFAULT_USB20_R 25
#define DEFAULT_USB20_W 20

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
    { "/sys/bus/usb/devices/8-1", DEFAULT_USB30_R, DEFAULT_USB30_W, DEFAULT_USB30_L, 0 },
    // eUSB_20, USB 2.0
    { "/sys/bus/usb/devices/2-1", DEFAULT_USB20_R, DEFAULT_USB20_W, DEFAULT_USB20_L, 0 },
    // eUSB_OTG, USB OTG (Micro USB)
    { "/sys/bus/usb/devices/5-1", DEFAULT_USB20_R, DEFAULT_USB20_W, DEFAULT_USB20_L, 0 },
    // eUSB_EXTRA, Extra 14 Pin Header (USB2.0)
    { "/sys/bus/usb/devices/1-1", DEFAULT_USB20_R, DEFAULT_USB20_W, DEFAULT_USB20_L, 0 },
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
    int value = 0, status = 0;

    if ((id >= eUSB_END) || ((access (DeviceUSB[id].path, R_OK)) != 0)) {
        sprintf (resp, "%06d", 0);
        return 0;
    }

    switch (action) {
        case 'I':
        case 'R':
            value  = (action == 'I') ?
                    DeviceUSB[id].value : usb_rw (DeviceUSB[id].path, USB_R_CHECK);
            status = (value < DeviceUSB[id].r_min) ? 0 : 1;
            break;
        case 'W':
            value  = usb_rw (DeviceUSB[id].path, USB_W_CHECK);
            status = (value < DeviceUSB[id].w_min) ? 0 : 1;
            break;
        case 'L':
            value  = usb_speed (DeviceUSB[id].path);
            status = (value != DeviceUSB[id].speed) ? 0 : 1;
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", value);
    return status;
}

//------------------------------------------------------------------------------
static void default_config_write (const char *fname)
{
    FILE *fp;
    char value [STR_PATH_LENGTH *2 +1];

    if ((fp = fopen(fname, "wt")) == NULL)
        return;

    // default value write
    fputs   ("# info : dev_id, dev_node, rd_speed, wr_speed, link_speed \n", fp);
    memset  (value, 0, sizeof(value));
    sprintf (value, "%d,%s,%d,%d,%d,\n",
        eUSB_30, DeviceUSB [eUSB_30].path, DEFAULT_USB30_R, DEFAULT_USB30_W, DEFAULT_USB30_L);
    fputs   (value, fp);
    memset  (value, 0, sizeof(value));
    sprintf (value, "%d,%s,%d,%d,%d,\n",
        eUSB_20, DeviceUSB [eUSB_20].path, DEFAULT_USB20_R, DEFAULT_USB20_W, DEFAULT_USB20_L);
    fputs   (value, fp);
    memset  (value, 0, sizeof(value));
    sprintf (value, "%d,%s,%d,%d,%d,\n",
        eUSB_OTG, DeviceUSB [eUSB_OTG].path, DEFAULT_USB20_R, DEFAULT_USB20_W, DEFAULT_USB20_L);
    fputs   (value, fp);
    memset  (value, 0, sizeof(value));
    sprintf (value, "%d,%s,%d,%d,%d,\n",
        eUSB_EXTRA, DeviceUSB [eUSB_EXTRA].path, DEFAULT_USB20_R, DEFAULT_USB20_W, DEFAULT_USB20_L);
    fputs   (value, fp);

    // file close
    fclose  (fp);
}

//------------------------------------------------------------------------------
static void default_config_read (void)
{
    FILE *fp;
    char fname [STR_PATH_LENGTH +1], value [STR_PATH_LENGTH +1], *ptr;
    int dev_id;

    memset  (fname, 0, STR_PATH_LENGTH);
    sprintf (fname, "%sjig-%s.cfg", CONFIG_FILE_PATH, "usb");

    if (access (fname, R_OK) != 0) {
        default_config_write (fname);
        return;
    }

    if ((fp = fopen(fname, "r")) == NULL)
        return;

    while(1) {
        memset (value , 0, STR_PATH_LENGTH);
        if (fgets (value, sizeof (value), fp) == NULL)
            break;

        switch (value[0]) {
            case '#':   case '\n':
                break;
            default :
                // default value write
                // fputs   ("# info : dev_id, dev_node, rd_speed, wr_speed, link_speed \n", fp);
                if ((ptr = strtok (value, ",")) != NULL) {
                    dev_id = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL) {
                        memset (DeviceUSB[dev_id].path, 0, STR_PATH_LENGTH);
                        strcpy (DeviceUSB[dev_id].path, ptr);
                    }
                    if ((ptr = strtok ( NULL, ",")) != NULL)
                       DeviceUSB[dev_id].r_min  = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL)
                       DeviceUSB[dev_id].w_min  = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL)
                       DeviceUSB[dev_id].speed = atoi (ptr);
                }
                break;
        }
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
int usb_grp_init (void)
{
    int i;

    default_config_read ();

    for (i = 0; i < eUSB_END; i++) {
        if ((access (DeviceUSB[i].path, R_OK)) == 0)
            DeviceUSB[i].value = usb_rw (DeviceUSB[i].path, USB_R_CHECK);
    }

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
