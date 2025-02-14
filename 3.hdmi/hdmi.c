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
    {{0,},{0,}},
    {{0,},{0,}},
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
    char buf[HDMI_READ_BYTES+1];

    memset  (buf, 0, sizeof(buf));
    if (id)
        sprintf (buf, "%s", rdata);
    else
        sprintf (buf, "%02x%02x%02x%02x%02x%02x%02x%02x",
            rdata[0], rdata[1], rdata[2], rdata[3],
            rdata[4], rdata[5], rdata[6], rdata[7]);

    if (!strncmp (buf, DeviceHDMI[id].pass_str, strlen (DeviceHDMI[id].pass_str)))
        return 1;

    return 0;
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
        DEVICE_RESP_FORM_STR (resp, 'F', "FAIL");

    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void hdmi_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eHDMI_EDID: case eHDMI_HPD:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceHDMI[did].path, tok, strlen(tok));
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceHDMI[did].pass_str, tok, strlen(tok));
                    break;
                default :
                    printf ("%s : error! unknown gid = %d\n", __func__, atoi(tok));
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
