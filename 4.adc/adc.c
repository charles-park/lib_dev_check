//------------------------------------------------------------------------------
/**
 * @file adc.c
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

#include "../lib_dev_check.h"
#include "adc.h"

//------------------------------------------------------------------------------
struct device_adc {
    // Control path
    const char *path;
    // compare value
    const int max, min;
    // read value
    int value;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define adc devices */
//------------------------------------------------------------------------------
struct device_adc DeviceADC [eADC_END] = {
    // eADC_0 (Header 37)
    { "/sys/bus/iio/devices/iio:device0/in_voltage3_raw", 1400, 1300, 0 },
    // eADC_1 (Header 40)
    { "/sys/bus/iio/devices/iio:device0/in_voltage2_raw", 500, 400, 0 },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int adc_read (const char *path)
{
    char rdata[16];
    FILE *fp;

    if (access (path, R_OK) != 0)
        return 0;

    memset (rdata, 0, sizeof (rdata));
    // adc raw value get
    if ((fp = fopen(path, "r")) != NULL) {
        fgets (rdata, sizeof(rdata), fp);
        fclose(fp);
    }

    return atoi(rdata);
}

//------------------------------------------------------------------------------
int adc_check (int id, char action, char *resp)
{
    int value = 0;

    if (id >= eADC_END)
        return 0;

    value = (action == 'I') ? DeviceADC[id].value : adc_read (DeviceADC[id].path);

    sprintf (resp, "%06d", value);

    if ((value < DeviceADC[id].max) && (value > DeviceADC[id].min))
        return 1;

    return 0;
}

//------------------------------------------------------------------------------
int adc_grp_init (void)
{
    int i;

    for (i = 0; i < eADC_END; i++)
        DeviceADC[i].value = adc_read (DeviceADC[i].path);

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
