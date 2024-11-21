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
//
// Configuration
//
//------------------------------------------------------------------------------
// not implement
int gpio_pin_control (int gpio_pin, int rw)
{
    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int gpio_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eGPIO_PIN:
            value  = gpio_pin_control (id, DEVICE_ACTION(dev_id));
            status = (value == 1) ? 1 : -1;
            break;
        default :
            break;
    }
//    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', "not implement");
    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'P' : 'F', "not implement");
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int gpio_grp_init (void)
{
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
