//------------------------------------------------------------------------------
/**
 * @file pwm.c
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
#include "pwm.h"

//------------------------------------------------------------------------------
struct device_pwm {
    // Control path
    const char *path;
    // set
    const char *set;
    // clear
    const char *clr;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define pwm devices (ODROID-N2L) */
//------------------------------------------------------------------------------
struct device_pwm DevicePWM [ePWM_END] = {
    // PWM0
    { "/sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm0_enable", "1", "0" },
    // PWM1
    { "/sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1_enable", "1", "0" },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int pwm_read (const char *path)
{
    char rdata[16];
    FILE *fp;

    memset (rdata, 0, sizeof (rdata));
    // adc raw value get
    if ((fp = fopen(path, "r")) != NULL) {
        fread (rdata, 1, sizeof(rdata), fp);
        fclose(fp);
    }

    return atoi(rdata);
}

//------------------------------------------------------------------------------
static int pwm_write (const char *path, const char *wdata)
{
    FILE *fp;

    // adc raw value get
    if ((fp = fopen(path, "w")) != NULL) {
        fwrite (wdata, 1, strlen(wdata), fp);
        fclose(fp);
    }

    return atoi(wdata);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int pwm_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case ePWM_0: case ePWM_1:
            if (DEVICE_ACTION(dev_id) == 1) {
                value = pwm_write (DevicePWM[id].path, DevicePWM[id].set);
            }
            else
                value = pwm_write (DevicePWM[id].path, DevicePWM[id].clr);

            status = (value == pwm_read (DevicePWM[id].path)) ? 1 : -1;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int pwm_grp_init (void)
{
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
