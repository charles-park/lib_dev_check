//------------------------------------------------------------------------------
/**
 * @file gpio_pin.c
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
#include "gpio_pin.h"

//------------------------------------------------------------------------------
struct device_gpio {
    int num;
    char cname[STR_NAME_LENGTH];
    int max, min;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_gpio DeviceGPIO[eGPIO_END] = {
    // eGPIO_ID0
    { 0, { 0, }, 0, 0 },
    // eGPIO_ID1
    { 0, { 0, }, 0, 0 },
    // ...
};

//------------------------------------------------------------------------------
static int gpio_pin_control (int id, int value)
{
    int g_value = 0;

    gpio_set_value (DeviceGPIO[id].num, value);
    gpio_get_value (DeviceGPIO[id].num, &g_value);

    return (value == g_value) ? 1 : 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int gpio_data_check (int dev_id, int resp_i)
{
    int status = 0, id = DEVICE_ID(dev_id);
    switch (id) {
        case 0 ... 9:
            if (DEVICE_ACTION(dev_id))
                status = (resp_i > DeviceGPIO[id].max) ? 1 : 0;    // gpio on
            else
                status = (resp_i < DeviceGPIO[id].min) ? 1 : 0;    // gpio off
            break;
        default :
            status = 0;
            break;
    }
    return status;
}

//------------------------------------------------------------------------------
int gpio_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case 0 ... 9:
            value  = gpio_pin_control (id, DEVICE_ACTION(dev_id));
            status = (value == 1) ? 1 : -1;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', DeviceGPIO[id].cname);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void gpio_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case 0 ... 9:
                    // gpio num
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceGPIO[did].num = atoi(tok);

                    // gpio adc con name
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceGPIO[did].cname, tok, strlen(tok));

                    // gpio on value
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceGPIO[did].max = atoi(tok);

                    // gpio off value
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceGPIO[did].min = atoi(tok);
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
