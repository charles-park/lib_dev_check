//------------------------------------------------------------------------------
/**
 * @file hdmi.c
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
#include "hdmi.h"

//------------------------------------------------------------------------------
struct device_hdmi {
    // Control path
    char path[STR_PATH_LENGTH +1];
    // compare value
    char pass_str[STR_PATH_LENGTH +1];
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define hdmi devices */
//------------------------------------------------------------------------------
#define HDMI_READ_BYTES 16

struct device_hdmi DeviceHDMI [eHDMI_END] = {
    // EDID
    {
        "/sys/devices/virtual/amhdmitx/amhdmitx0/rawedid",
        /* 16 char string (8bytes raw data)*/
        "00ffffffffffff00",
    },
    // HPD
    {
        "/sys/devices/virtual/amhdmitx/amhdmitx0/rhpd_state",
        "1",
    },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int hdmi_read (const char *path, char *rdata)
{
    FILE *fp;

    // led value get
    if ((fp = fopen(path, "r")) != NULL) {
        fread (rdata, 1, HDMI_READ_BYTES, fp);
        fclose(fp);
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
static int data_check (int id, const char *rdata)
{
    char buf[HDMI_READ_BYTES];

    memset  (buf, 0, sizeof(buf));
    strncpy (buf, rdata, strlen (DeviceHDMI[id].pass_str));

    if (!strncmp (buf, DeviceHDMI[id].pass_str, strlen (DeviceHDMI[id].pass_str)))
        return 1;

    return 0;
}

//------------------------------------------------------------------------------
static void default_config_write (const char *fname)
{
    FILE *fp;
    char value [STR_PATH_LENGTH *3 +1];

    if ((fp = fopen(fname, "wt")) == NULL)
        return;

    // default value write
    fputs   ("# info : id, path, compare value \n", fp);
    {
        int id;
        for (id = 0; id < eHDMI_END; id++) {
            memset  (value, 0, STR_PATH_LENGTH *2);
            sprintf (value, "%d,%s,%s,\n", id, DeviceHDMI[id].path, DeviceHDMI[id].pass_str);
            fputs   (value, fp);
        }
    }
    fclose  (fp);
}

//------------------------------------------------------------------------------
static void default_config_read (void)
{
    FILE *fp;
    int id = 0;
    char fname [STR_PATH_LENGTH +1], value [STR_PATH_LENGTH +1], *ptr;

    memset  (fname, 0, STR_PATH_LENGTH);
    sprintf (fname, "%sjig-%s.cfg", CONFIG_FILE_PATH, "hdmi");

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
                // path, compare value
                if ((ptr = strtok (value, ",")) != NULL) {

                    id = atoi (ptr);

                    if ((ptr = strtok ( NULL, ",")) != NULL) {
                        memset (DeviceHDMI[id].path, 0, STR_PATH_LENGTH);
                        strcpy (DeviceHDMI[id].path, ptr);
                    }
                    if ((ptr = strtok ( NULL, ",")) != NULL) {
                        memset (DeviceHDMI[id].pass_str, 0, STR_PATH_LENGTH);
                        strcpy (DeviceHDMI[id].pass_str, ptr);
                    }
                }
                break;
        }
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
int hdmi_check (int dev_id, char *resp)
{
    int status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eHDMI_EDID: case eHDMI_HPD:
            if (hdmi_read (DeviceHDMI[id].path, resp))
                status = data_check (id, resp);
            else
                status = -1;
            break;
        default :
            break;
    }
    if (status == 1)
        DEVICE_RESP_FORM_STR (resp, 'P', "PASS");
    else
        DEVICE_RESP_FORM_STR (resp, 'P', "FAIL");

    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int hdmi_grp_init (void)
{
    default_config_read ();
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
