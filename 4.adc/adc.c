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
    int init;
    // Control path
    char path[STR_PATH_LENGTH +1];
    // compare value
    int max, min;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define adc devices */
//------------------------------------------------------------------------------
struct device_adc DeviceADC [eADC_END] = {
    // eADC_H37 (Header 37) - const 1.358V
    { 0, {0, }, 0, 0 },
    // eADC_H40 (Header 40) - const 0.441V
    { 0, {0, }, 0, 0 },
};

int ResolutionADC = 0, ReferenceADC = 0;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// default adc range (mV). ADC res 1.7578125mV (1800mV / ADC RESOLUTION)
// adc voltage = adc raw read * ADC res (1.75)
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

    if (ReferenceADC && ResolutionADC)
        return (atoi (rdata) * ReferenceADC) / ResolutionADC;

    return (atoi (rdata));
}

//------------------------------------------------------------------------------
int adc_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    if (DeviceADC[id].init) {
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
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void adc_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eADC_H37: case eADC_H40:
                DeviceADC[did].init = 1;
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceADC[did].path, tok, strlen(tok));

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceADC[did].max = atoi(tok);

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceADC[did].min = atoi(tok);
                    break;

                case 9: /* ADC config */
                    // Reference voltage(mV)
                    if ((tok = strtok (NULL, ",")) != NULL)
                        ReferenceADC = atoi(tok);

                    // Resolution ADC bits
                    if ((tok = strtok (NULL, ",")) != NULL)
                        ResolutionADC = atoi(tok);
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
