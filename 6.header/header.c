//------------------------------------------------------------------------------
/**
 * @file header.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-25
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
#include "../lib_gpio/lib_gpio.h"
#include "header.h"
#include "header_c4.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int pattern_write (int id, int pattern)
{
    int cnt = 0, err_cnt = 0, read = 0, nc_pin_cnt = 0;
    int gpio_cnt = 0, *gpio_pattern, *gpio_list;

    switch (id) {
        case eHEADER_7:
            gpio_cnt     = sizeof(HEADER7)/sizeof(HEADER7[0]);
            gpio_pattern = (int *)HEADER7_PATTERN[pattern];
            gpio_list    = (int *)HEADER7;
            break;
        case eHEADER_14:
            gpio_cnt     = sizeof(HEADER14)/sizeof(HEADER14[0]);
            gpio_pattern = (int *)HEADER14_PATTERN[pattern];
            gpio_list    = (int *)HEADER14;
            break;
        case eHEADER_40:
            gpio_cnt     = sizeof(HEADER40)/sizeof(HEADER40[0]);
            gpio_pattern = (int *)HEADER40_PATTERN[pattern];
            gpio_list    = (int *)HEADER40;
            break;
        default:
            printf ("%s : unknown header (id = %d)\n", __func__, id);
            return 0;
    }

    for (cnt = 0; cnt < gpio_cnt; cnt++) {
        if (gpio_list[cnt] != NC) {
            gpio_set_value (gpio_list[cnt], gpio_pattern[cnt]);
            gpio_get_value (gpio_list[cnt], &read);
            if (read != gpio_pattern[cnt]) {
                printf ("%s : error, num = %d gpio = %d, set = %d, get = %d\n",
                    __func__, cnt, gpio_list[cnt], gpio_pattern [cnt], read);
                err_cnt++;
            }
        } else nc_pin_cnt++;
    }
    if (nc_pin_cnt == gpio_cnt) {
        printf ("%s : not used header id(%d)\n", __func__, id);
        return 0;
    }
    return err_cnt ? 0 : 1;
}

//------------------------------------------------------------------------------
int header_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eHEADER_40: case eHEADER_14: case eHEADER_7:
            status = pattern_write (id, DEVICE_ACTION(dev_id)) ? 1 : -1;
            value  = DEVICE_ACTION(dev_id);
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'C' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int header_grp_init (void)
{
    int i;

    for (i = 0; i < (int)(sizeof(HEADER7)/sizeof(int)); i++) {
        if (HEADER7[i] != NC) {
            gpio_export    (HEADER7[i]);
            gpio_direction (HEADER7[i], 1);
        }
    }

    for (i = 0; i < (int)(sizeof(HEADER14)/sizeof(int)); i++) {
        if (HEADER14[i] != NC) {
            gpio_export    (HEADER14[i]);
            gpio_direction (HEADER14[i], 1);
        }
    }

    for (i = 0; i < (int)(sizeof(HEADER40)/sizeof(int)); i++) {
        if (HEADER40[i] != NC) {
            gpio_export    (HEADER40[i]);
            gpio_direction (HEADER40 [i], 1);
        }
    }

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
