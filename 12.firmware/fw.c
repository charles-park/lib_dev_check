//------------------------------------------------------------------------------
/**
 * @file fw.c
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
#include "fw.h"

//------------------------------------------------------------------------------
struct device_fw {
    // full path
    char bin_path [STR_PATH_LENGTH];
    char fw_path  [STR_PATH_LENGTH];

    char check_fw_ver [STR_NAME_LENGTH];
    char fw_ver [STR_NAME_LENGTH];
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_fw DeviceFW [eFW_END] = {
    // C4
    { { 0, }, { 0, }, { 0, }, { 0, } },
};

const char *scan_hub  = "lsusb | grep 2109 | awk '{print $6}'";
const char *reset_hub = "echo reset > /sys/devices/platform/gpio-reset/reset-usb_hub/control";
const char *read_fw_ver  = "usb-devices | grep Rev | grep 2109 | grep 817 | awk '{print $4}' | sed \"s/Rev=//g\"";

//------------------------------------------------------------------------------
static void usb_hub_reset (void)
{
    FILE *fp;
    char cmd [STR_PATH_LENGTH *2];

    memset  (cmd, 0, sizeof(cmd));
    sprintf (cmd, "%s && sync", reset_hub);

    if ((fp = popen (cmd, "w")) != NULL)
        pclose(fp);

    sleep (1);
}

//------------------------------------------------------------------------------
static int usb_hub_check (void)
{
    FILE *fp;
    char cmd [STR_PATH_LENGTH *2], rdata[10];

    memset  (cmd, 0, sizeof(cmd));
    sprintf (cmd, "%s && sync", scan_hub);

    if ((fp = popen (cmd, "r")) != NULL) {
        while (fgets (rdata, sizeof(rdata), fp) != NULL) {
            if (strstr(rdata, "2109") != NULL) {
                pclose(fp);
                return 1;
            }
        }
        pclose(fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
static int c4_fw_write (int id)
{
    FILE *fp;
    char cmd [STR_PATH_LENGTH *3], rdata[STR_PATH_LENGTH];

    if (!usb_hub_check())   return 0;

    memset  (cmd, 0, sizeof(cmd));
    sprintf (cmd, "%s --vid=2109 -pid=0817 -script=%s && sync",
                DeviceFW[id].bin_path, DeviceFW[id].fw_path);

    printf ("%s : %s\n", __func__, cmd);
    if ((fp = popen (cmd, "r")) != NULL) {
        memset (rdata, 0, sizeof(rdata));
        while (fgets (rdata, sizeof(rdata), fp) != NULL) {
            if (strstr(rdata, "success") != NULL) {
                pclose(fp);
                return 1;
            }
            memset (rdata, 0, sizeof(rdata));
        }
        pclose(fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
static int c4_ver_read (int id)
{
    FILE *fp;
    char cmd [STR_PATH_LENGTH *2], rdata[10];

    memset  (cmd, 0, sizeof(cmd));
    sprintf (cmd, "%s && sync", read_fw_ver);

    if (!usb_hub_check())   return 0;

    if ((fp = popen (cmd, "r")) != NULL) {
        while (fgets (rdata, sizeof(rdata), fp) != NULL) {
            if (strlen(rdata) > 2) {
                strncpy (DeviceFW[id].fw_ver, rdata, strlen(rdata)-1);
                printf ("%s : version = %s\n", __func__, rdata);
                pclose(fp);
                return 1;
            }
        }
        pclose(fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
static int c4_ver_check (int id)
{
    if (c4_ver_read (id)) {
        if (!strncmp (DeviceFW[id].fw_ver, DeviceFW[id].check_fw_ver,
                        strlen(DeviceFW[id].check_fw_ver)))
            return 1;

        printf ("%s : firmware version check error! (read %s : check %s)\n",
                            __func__, DeviceFW[id].fw_ver, DeviceFW[id].check_fw_ver);

        if (c4_fw_write (id)) {
            usb_hub_reset ();
            return (c4_ver_read (id));
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
int fw_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eFW_C4:
            value  = c4_ver_check (id);
            status = (value == 1) ? 1 : -1;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'P' : 'F', DeviceFW[id].fw_ver);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void fw_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eFW_C4:
                    // exec bin file path
                    if ((tok = strtok (NULL, ",")) != NULL)
                        find_file_path (tok, DeviceFW[did].bin_path);

                    // f/w file path
                    if ((tok = strtok (NULL, ",")) != NULL)
                        find_file_path (tok, DeviceFW[did].fw_path);

                    // f/w ver str
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceFW[did].check_fw_ver, tok, strlen(tok));
                    break;
                default :
                    printf ("%s : error! unknown did = %d\n", __func__, did);
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
