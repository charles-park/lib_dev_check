//------------------------------------------------------------------------------
/**
 * @file header.c
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
#include "lib_gpio/lib_gpio.h"
#include "header.h"

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// ODROID-M1S Header GPIOs Define
//
//------------------------------------------------------------------------------
// Not Control
#define NC  0

const int HEADER14[] = {
    // Header J3 GPIOs
     NC,        // Not used (pin 0)
     NC,  NC,   // | 01 : DC_JACK  || 02 : 3.3V      |
     NC,  NC,   // | 03 : USB D_P  || 04 : AUDIO_R   |
     NC,  NC,   // | 05 : USB D_M  || 06 : AUDIO_GND |
     NC,  NC,   // | 07 : GND      || 08 : AUDIO_L   |
     NC,  NC,   // | 09 : PWRBTN   || 10 : RESET_H   |
    116, 117,   // | 11 : GPIO3_C4 || 12 : GPIO3_C5  |
    107, 108,   // | 13 : GPIO3_B3 || 14 : GPIO3_B4  |
};

const int HEADER40[] = {
    // Header J4 GPIOs
     NC,        // Not used (pin 0)
     NC,  NC,   // | 01 : 3.3V     || 02 : 5.0V     |
    110,  NC,   // | 03 : I2C_SDA1 || 04 : 5.0V     |
    109,  NC,   // | 05 : I2C_SCL1 || 06 : GND      |
     14,  68,   // | 07 : GPIO0_B6 || 08 : GPIO2_A4 |
     NC,  67,   // | 09 : GND      || 10 : GPIO2_A3 |
     16,  71,   // | 11 : GPIO0_C0 || 12 : GPIO2_A7 |
     17,  NC,   // | 13 : GPIO0_C1 || 14 : GND      |
     18,  77,   // | 15 : GPIO0_C2 || 16 : GPIO2_B5 |
     NC,  78,   // | 17 : 3.3V     || 18 : GPIO3_B6 |
    113,  NC,   // | 19 : GPIO3_C1 || 20 : GND      |
    114,  72,   // | 21 : GPIO3_C2 || 22 : GPIO2_B0 |
    115,  97,   // | 23 : GPIO3_C3 || 24 : GPIO3_A1 |
     NC,  73,   // | 25 : GND      || 26 : GPIO2_B1 |
     12,  11,   // | 27 : GPIO0_B4 || 28 : GPIO2_B3 |
     80,  NC,   // | 29 : GPIO2_C0 || 30 : GND      |
     79,  74,   // | 31 : GPIO2_B7 || 32 : GPIO2_B2 |
     13,  NC,   // | 33 : GPIO0_B5 || 34 : GND      |
     69,  70,   // | 35 : GPIO0_A5 || 36 : GPIO2_A6 |
     NC,  NC,   // | 37 : ADC.AIN1 || 38 : 1.8V     |
     NC,  NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
};

#define PATTERN_COUNT   4

const int GPIO_PATTERN[PATTERN_COUNT][sizeof(HEADER40)] = {
    // Pattern 0 : ALL High
    {
        // Header J4 GPIOs
        NC,        // Not used (pin 0)
        NC, NC,   // | 01 : 3.3V     || 02 : 5.0V     |
         1, NC,   // | 03 : I2C_SDA1 || 04 : 5.0V     |
         1, NC,   // | 05 : I2C_SCL1 || 06 : GND      |
         1,  1,   // | 07 : GPIO0_B6 || 08 : GPIO2_A4 |
        NC,  1,   // | 09 : GND      || 10 : GPIO2_A3 |
         1,  1,   // | 11 : GPIO0_C0 || 12 : GPIO2_A7 |
         1, NC,   // | 13 : GPIO0_C1 || 14 : GND      |
         1,  1,   // | 15 : GPIO0_C2 || 16 : GPIO2_B5 |
        NC,  1,   // | 17 : 3.3V     || 18 : GPIO3_B6 |
         1, NC,   // | 19 : GPIO3_C1 || 20 : GND      |
         1,  1,   // | 21 : GPIO3_C2 || 22 : GPIO2_B0 |
         1,  1,   // | 23 : GPIO3_C3 || 24 : GPIO3_A1 |
        NC,  1,   // | 25 : GND      || 26 : GPIO2_B1 |
         1,  1,   // | 27 : GPIO0_B4 || 28 : GPIO2_B3 |
         1, NC,   // | 29 : GPIO2_C0 || 30 : GND      |
         1,  1,   // | 31 : GPIO2_B7 || 32 : GPIO2_B2 |
         1, NC,   // | 33 : GPIO0_B5 || 34 : GND      |
         1,  1,   // | 35 : GPIO0_A5 || 36 : GPIO2_A6 |
        NC, NC,   // | 37 : ADC.AIN1 || 38 : 1.8V     |
        NC, NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
    },
    // Pattern 1 : ALL Low
    {
        // Header J4 GPIOs
        NC,        // Not used (pin 0)
        NC, NC,   // | 01 : 3.3V     || 02 : 5.0V     |
         0, NC,   // | 03 : I2C_SDA1 || 04 : 5.0V     |
         0, NC,   // | 05 : I2C_SCL1 || 06 : GND      |
         0,  0,   // | 07 : GPIO0_B6 || 08 : GPIO2_A4 |
        NC,  0,   // | 09 : GND      || 10 : GPIO2_A3 |
         0,  0,   // | 11 : GPIO0_C0 || 12 : GPIO2_A7 |
         0, NC,   // | 13 : GPIO0_C1 || 14 : GND      |
         0,  0,   // | 15 : GPIO0_C2 || 16 : GPIO2_B5 |
        NC,  0,   // | 17 : 3.3V     || 18 : GPIO3_B6 |
         0, NC,   // | 19 : GPIO3_C1 || 20 : GND      |
         0,  0,   // | 21 : GPIO3_C2 || 22 : GPIO2_B0 |
         0,  0,   // | 23 : GPIO3_C3 || 24 : GPIO3_A1 |
        NC,  0,   // | 25 : GND      || 26 : GPIO2_B1 |
         0,  0,   // | 27 : GPIO0_B4 || 28 : GPIO2_B3 |
         0, NC,   // | 29 : GPIO2_C0 || 30 : GND      |
         0,  0,   // | 31 : GPIO2_B7 || 32 : GPIO2_B2 |
         0, NC,   // | 33 : GPIO0_B5 || 34 : GND      |
         0,  0,   // | 35 : GPIO0_A5 || 36 : GPIO2_A6 |
        NC, NC,   // | 37 : ADC.AIN1 || 38 : 1.8V     |
        NC, NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
    },
    // Pattern 2 : Cross 0
    {
        // Header J4 GPIOs
        NC,        // Not used (pin 0)
        NC, NC,   // | 01 : 3.3V     || 02 : 5.0V     |
         0, NC,   // | 03 : I2C_SDA1 || 04 : 5.0V     |
         1, NC,   // | 05 : I2C_SCL1 || 06 : GND      |
         0,  1,   // | 07 : GPIO0_B6 || 08 : GPIO2_A4 |
        NC,  0,   // | 09 : GND      || 10 : GPIO2_A3 |
         0,  1,   // | 11 : GPIO0_C0 || 12 : GPIO2_A7 |
         1, NC,   // | 13 : GPIO0_C1 || 14 : GND      |
         0,  1,   // | 15 : GPIO0_C2 || 16 : GPIO2_B5 |
        NC,  0,   // | 17 : 3.3V     || 18 : GPIO3_B6 |
         1, NC,   // | 19 : GPIO3_C1 || 20 : GND      |
         0,  1,   // | 21 : GPIO3_C2 || 22 : GPIO2_B0 |
         1,  0,   // | 23 : GPIO3_C3 || 24 : GPIO3_A1 |
        NC,  1,   // | 25 : GND      || 26 : GPIO2_B1 |
         1,  0,   // | 27 : GPIO0_B4 || 28 : GPIO2_B3 |
         0, NC,   // | 29 : GPIO2_C0 || 30 : GND      |
         1,  0,   // | 31 : GPIO2_B7 || 32 : GPIO2_B2 |
         0, NC,   // | 33 : GPIO0_B5 || 34 : GND      |
         1,  0,   // | 35 : GPIO0_A5 || 36 : GPIO2_A6 |
        NC, NC,   // | 37 : ADC.AIN1 || 38 : 1.8V     |
        NC, NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
    },
    // Pattern 3 : Cross 1
    {
        // Header J4 GPIOs
        NC,        // Not used (pin 0)
        NC, NC,   // | 01 : 3.3V     || 02 : 5.0V     |
         1, NC,   // | 03 : I2C_SDA1 || 04 : 5.0V     |
         0, NC,   // | 05 : I2C_SCL1 || 06 : GND      |
         1,  0,   // | 07 : GPIO0_B6 || 08 : GPIO2_A4 |
        NC,  1,   // | 09 : GND      || 10 : GPIO2_A3 |
         1,  0,   // | 11 : GPIO0_C0 || 12 : GPIO2_A7 |
         0, NC,   // | 13 : GPIO0_C1 || 14 : GND      |
         1,  0,   // | 15 : GPIO0_C2 || 16 : GPIO2_B5 |
        NC,  1,   // | 17 : 3.3V     || 18 : GPIO3_B6 |
         0, NC,   // | 19 : GPIO3_C1 || 20 : GND      |
         1,  0,   // | 21 : GPIO3_C2 || 22 : GPIO2_B0 |
         0,  1,   // | 23 : GPIO3_C3 || 24 : GPIO3_A1 |
        NC,  0,   // | 25 : GND      || 26 : GPIO2_B1 |
         0,  1,   // | 27 : GPIO0_B4 || 28 : GPIO2_B3 |
         1, NC,   // | 29 : GPIO2_C0 || 30 : GND      |
         0,  1,   // | 31 : GPIO2_B7 || 32 : GPIO2_B2 |
         1, NC,   // | 33 : GPIO0_B5 || 34 : GND      |
         0,  1,   // | 35 : GPIO0_A5 || 36 : GPIO2_A6 |
        NC, NC,   // | 37 : ADC.AIN1 || 38 : 1.8V     |
        NC, NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
    },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int pattern_write (int pattern)
{
    if (pattern < PATTERN_COUNT) {
        int i;
        for (i = 0; i < (int)(sizeof(HEADER40)/sizeof(int)); i++) {
            if (HEADER40[i]) {
                gpio_direction (HEADER40[i], GPIO_DIR_OUT);
                gpio_set_value (HEADER40[i], GPIO_PATTERN[pattern][i]);
            }
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int header_check (int id, char action, char *resp)
{
    int ret_value = 0, value = 0;

    if (id) {
        switch (action) {
            case 'S':   case 'C':
                // gpio control
                if (gpio_direction (id, GPIO_DIR_OUT)) {
                    value = (action == 'S') ? 1 : 0;
                    ret_value = gpio_set_value (id, value);
                }
                break;
            case 'R':
                if (gpio_direction (id, GPIO_DIR_IN))
                    ret_value = gpio_get_value (id, &value);
                break;
            default :
                ret_value = gpio_get_value (id, &value);
                break;
        }
        printf ("gpio control %d ret = %d, value = %d\n", id, ret_value, value);

    } else {
        // header40 pattern test
        value = action - '0';
        ret_value = pattern_write (value);
        printf ("set pattern %d, ret = %d\n", value, ret_value);
    }

    sprintf (resp, "%06d", value);
    return ret_value;
}

//------------------------------------------------------------------------------
int header_grp_init (void)
{
    int i;

    for (i = 0; i < (int)(sizeof(HEADER14)/sizeof(int)); i++)
        gpio_export (HEADER14[i]);

    for (i = 0; i < (int)(sizeof(HEADER40)/sizeof(int)); i++)
        gpio_export (HEADER40[i]);

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
