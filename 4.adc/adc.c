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
    char path[STR_PATH_LENGTH +1];
    // compare value
    int max, min;

    // read value
    int value;
};

// default adc range (mV). ADC res 1.7578125mV (1800mV / 1024 bits)
// const 1.358V (ADC board : 772) - (m1s = 787)
#define DEFAULT_ADC_H37_H   800     // 800 * 1.75 = 1400.0mV
#define DEFAULT_ADC_H37_L   770     // 770 * 1.75 = 1347.5mV

// const 0.441V (ADC board : 250) - (m1s = 270)
#define DEFAULT_ADC_H40_H   280     // 280 * 1.75 = 490.0mV
#define DEFAULT_ADC_H40_L   250     // 250 * 1.75 = 437.5mV

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define adc devices */
//------------------------------------------------------------------------------
struct device_adc DeviceADC [eADC_END] = {
    // eADC_H37 (Header 37) - 1.365mV ~ 1.347mV (1.358mV) : 780 x 1.75mV
    { "/sys/bus/iio/devices/iio:device0/in_voltage3_raw", DEFAULT_ADC_H37_H, DEFAULT_ADC_H37_L, 0 },
    // eADC_H40 (Header 40) - 0.481mv ~ 0.463mV (0.441mV)
    { "/sys/bus/iio/devices/iio:device0/in_voltage2_raw", DEFAULT_ADC_H40_H, DEFAULT_ADC_H40_L, 0 },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int adc_read (const char *path)
{
    char rdata[16];
    FILE *fp;

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

    if ((id >= eADC_END) || (access (DeviceADC[id].path, R_OK) != 0)) {
        sprintf (resp, "%06d", 0);
        return 0;
    }

    value = (action == 'I') ? DeviceADC[id].value : adc_read (DeviceADC[id].path);

    sprintf (resp, "%06d", value);

    if ((value < DeviceADC[id].max) && (value > DeviceADC[id].min))
        return 1;

    return 0;
}

//------------------------------------------------------------------------------
static void default_config_write (const char *fname)
{
    FILE *fp;
    char value [STR_PATH_LENGTH *2 +1];

    if ((fp = fopen(fname, "wt")) == NULL)
        return;

    // default value write
    fputs   ("# info : dev_id, dev_node, max, min \n", fp);
    memset  (value, 0, sizeof(value));
    sprintf (value, "%d,%s,%d,%d,\n",
        eADC_H37, DeviceADC [eADC_H37].path, DEFAULT_ADC_H37_H, DEFAULT_ADC_H37_L);
    fputs   (value, fp);
    sprintf (value, "%d,%s,%d,%d,\n",
        eADC_H40, DeviceADC [eADC_H40].path, DEFAULT_ADC_H40_H, DEFAULT_ADC_H40_L);
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
    sprintf (fname, "%sjig-%s.cfg", CONFIG_FILE_PATH, "adc");

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
                // fputs   ("# info : dev_id, dev_node, max, min \n", fp);
                if ((ptr = strtok (value, ",")) != NULL) {
                    dev_id = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL) {
                        memset (DeviceADC[dev_id].path, 0, STR_PATH_LENGTH);
                        strcpy (DeviceADC[dev_id].path, ptr);
                    }
                    if ((ptr = strtok ( NULL, ",")) != NULL)
                       DeviceADC[dev_id].max  = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL)
                       DeviceADC[dev_id].min  = atoi (ptr);
                }
                break;
        }
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
int adc_grp_init (void)
{
    int i;

    default_config_read ();

    for (i = 0; i < eADC_END; i++) {
        if ((access (DeviceADC[i].path, R_OK)) == 0)
            DeviceADC[i].value = adc_read (DeviceADC[i].path);
    }

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
