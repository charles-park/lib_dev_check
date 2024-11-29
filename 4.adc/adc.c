//------------------------------------------------------------------------------
/**
 * @file adc.c
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

#include "../lib_dev_check.h"
#include "adc.h"

//------------------------------------------------------------------------------
struct device_adc {
    // Control path
    char path[STR_PATH_LENGTH +1];
    // compare value
    int max, min;
};


//------------------------------------------------------------------------------
// default adc range (mV). ADC res 1.7578125mV (1800mV / ADC RESOLUTION)
// adc voltage = adc raw read * ADC res (1.75)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define ADC_BITS            12      // ODROID-C4
#define ADC_REF_VOLTAGE     (1800)
#define ADC_RESOLUTION      (1<<ADC_BITS)

// const 1.358V
#define DEFAULT_ADC_H37_H   1400
#define DEFAULT_ADC_H37_L   1340

// const 0.441V
#define DEFAULT_ADC_H40_H   490
#define DEFAULT_ADC_H40_L   430

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define adc devices */
//------------------------------------------------------------------------------
struct device_adc DeviceADC [eADC_END] = {
    // eADC_H37 (Header 37) - const 1.358V
    { "/sys/bus/iio/devices/iio:device0/in_voltage2_raw", DEFAULT_ADC_H37_H, DEFAULT_ADC_H37_L },
    // eADC_H40 (Header 40) - const 0.441V
    { "/sys/bus/iio/devices/iio:device0/in_voltage0_raw", DEFAULT_ADC_H40_H, DEFAULT_ADC_H40_L },
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

    return (atoi (rdata) * ADC_REF_VOLTAGE) / ADC_RESOLUTION;
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
    {
        int id;
        for (id = 0; id < eADC_END; id++) {
            memset  (value, 0, sizeof(value));
            sprintf (value, "%d,%s,%d,%d,\n",
                id, DeviceADC [id].path, DeviceADC [id].max, DeviceADC [id].min);
            fputs   (value, fp);
        }
    }

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
int adc_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eADC_H37: case eADC_H40:
            value = adc_read (DeviceADC[id].path);
            if ((value < DeviceADC[id].max) && (value > DeviceADC[id].min))
                status = 1;
            else
                status = -1;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int adc_grp_init (void)
{
    default_config_read ();
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
