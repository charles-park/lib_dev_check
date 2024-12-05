//------------------------------------------------------------------------------
/**
 * @file header_c4.h
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
#ifndef __HEADER_C4_H__
#define __HEADER_C4_H__

#include "header.h"
//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// ODROID-C4 Header GPIOs Define
//
//------------------------------------------------------------------------------

// https://wiki.odroid.com/odroid-c4/hardware/expansion_connectors#j7_-_1x7_pins
const int HEADER7[] = {
    // Header J3 GPIOs
     NC,        // Not used (pin 0)
     NC,  NC,   // | 01 : GND       || 02 : SPDIF OUT |
     NC, 505,   // | 03 : 5.0V      || 04 : I2S_MCLK  |
    503, 504,   // | 05 : I2S LRCLK || 06 : I2S_SCLK  |
    500,        // | 07 : I2S_DATA  |
};

// none
const int HEADER14[] = {
    // Header J3 GPIOs
     NC,        // Not used (pin 0)
     NC,  NC,
     NC,  NC,
     NC,  NC,
     NC,  NC,
     NC,  NC,
     NC,  NC,
     NC,  NC,
};

// https://wiki.odroid.com/odroid-c4/hardware/expansion_connectors#j2_-_2x20_pins
const int HEADER40[] = {
    // Header J4 GPIOs
     NC,        // Not used (pin 0)
     NC,  NC,   // | 01 : 3.3V     || 02 : 5.0V     |
    493,  NC,   // | 03 : GPIOX_17 || 04 : 5.0V     |
    494,  NC,   // | 05 : GPIOX_18 || 06 : GND      |
    481, 488,   // | 07 : GPIOX_5  || 08 : GPIOX_12 |
     NC, 489,   // | 09 : GND      || 10 : GPIOX_13 |
    479, 492,   // | 11 : GPIOX_3  || 12 : GPIOX_16 |
    480,  NC,   // | 13 : GPIOX_4  || 14 : GND      |
    483, 476,   // | 15 : GPIOX_7  || 16 : GPIOX_0  |
     NC, 477,   // | 17 : 3.3V     || 18 : GPIOX_1  |
    484,  NC,   // | 19 : GPIOX_8  || 20 : GND      |
    485, 478,   // | 21 : GPIOX_9  || 22 : GPIOX_2  |
    487, 486,   // | 23 : GPIOX_11 || 24 : GPIOX_10 |
     NC, 433,   // | 25 : GND      || 26 : GPIOH_6  |
    474, 475,   // | 27 : GPIOA_14 || 28 : GPIOA_15 |
    490,  NC,   // | 29 : GPIOX_14 || 30 : GND      |
    491, 434,   // | 31 : GPIOX_15 || 32 : GPIOH_7  |
    482,  NC,   // | 33 : GPIOX_6  || 34 : GND      |
    495, 432,   // | 35 : GPIOX_19 || 36 : GPIOH_5  |
     NC,  NC,   // | 37 : ADC.AIN2 || 38 : 1.8V     |
     NC,  NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
};

//------------------------------------------------------------------------------
const int HEADER7_PATTERN[PATTERN_COUNT][sizeof(HEADER7)/sizeof(HEADER7[0])] = {
    // Pattern 0 : ALL Low
    {
        // Header J7 GPIOs
        NC,        // Not used (pin 0)
        NC,  NC,   // | 01 : GND       || 02 : SPDIF OUT |
        NC,   0,   // | 03 : 5.0V      || 04 : I2S_MCLK  |
         0,   0,   // | 05 : I2S LRCLK || 06 : I2S_SCLK  |
         0,        // | 07 : I2S_DATA  |
    },
    // Pattern 1 : ALL High
    {
        // Header J7 GPIOs
        NC,        // Not used (pin 0)
        NC,  NC,   // | 01 : GND       || 02 : SPDIF OUT |
        NC,   1,   // | 03 : 5.0V      || 04 : I2S_MCLK  |
         1,   1,   // | 05 : I2S LRCLK || 06 : I2S_SCLK  |
         1,        // | 07 : I2S_DATA  |
    },
    // Pattern 2 : Cross 0
    {
        // Header J7 GPIOs
        NC,        // Not used (pin 0)
        NC,  NC,   // | 01 : GND       || 02 : SPDIF OUT |
        NC,   1,   // | 03 : 5.0V      || 04 : I2S_MCLK  |
         1,   0,   // | 05 : I2S LRCLK || 06 : I2S_SCLK  |
         0,        // | 07 : I2S_DATA  |
    },
    // Pattern 3 : Cross 1
    {
        // Header J7 GPIOs
        NC,        // Not used (pin 0)
        NC,  NC,   // | 01 : GND       || 02 : SPDIF OUT |
        NC,   0,   // | 03 : 5.0V      || 04 : I2S_MCLK  |
         0,   1,   // | 05 : I2S LRCLK || 06 : I2S_SCLK  |
         1,        // | 07 : I2S_DATA  |
    },
};

const int HEADER14_PATTERN[PATTERN_COUNT][sizeof(HEADER14)/sizeof(HEADER14[0])] = {
    // Pattern 0 : ALL Low
    {
        NC,        // Not used (pin 0)
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
    },
    // Pattern 1 : ALL High
    {
        NC,        // Not used (pin 0)
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
    },
    // Pattern 2 : Cross 0
    {
        NC,        // Not used (pin 0)
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
    },
    // Pattern 3 : Cross 1
    {
        NC,        // Not used (pin 0)
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
        NC,  NC,
    },
};

const int HEADER40_PATTERN[PATTERN_COUNT][sizeof(HEADER40)/sizeof(HEADER40[0])] = {
    // Pattern 0 : ALL Low
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
    // Pattern 1 : Left pin 1 (1,3,5...39)
    {
        // Header J4 GPIOs
        NC,       // Not used (pin 0)
        NC, NC,   // | 01 : 3.3V     || 02 : 5.0V     |
         1, NC,   // | 03 : I2C_SDA1 || 04 : 5.0V     |
         1, NC,   // | 05 : I2C_SCL1 || 06 : GND      |
         1,  0,   // | 07 : GPIO0_B6 || 08 : GPIO2_A4 |
        NC,  0,   // | 09 : GND      || 10 : GPIO2_A3 |
         1,  0,   // | 11 : GPIO0_C0 || 12 : GPIO2_A7 |
         1, NC,   // | 13 : GPIO0_C1 || 14 : GND      |
         1,  0,   // | 15 : GPIO0_C2 || 16 : GPIO2_B5 |
        NC,  0,   // | 17 : 3.3V     || 18 : GPIO3_B6 |
         1, NC,   // | 19 : GPIO3_C1 || 20 : GND      |
         1,  0,   // | 21 : GPIO3_C2 || 22 : GPIO2_B0 |
         1,  0,   // | 23 : GPIO3_C3 || 24 : GPIO3_A1 |
        NC,  0,   // | 25 : GND      || 26 : GPIO2_B1 |
         1,  0,   // | 27 : GPIO0_B4 || 28 : GPIO2_B3 |
         1, NC,   // | 29 : GPIO2_C0 || 30 : GND      |
         1,  0,   // | 31 : GPIO2_B7 || 32 : GPIO2_B2 |
         1, NC,   // | 33 : GPIO0_B5 || 34 : GND      |
         1,  0,   // | 35 : GPIO0_A5 || 36 : GPIO2_A6 |
        NC, NC,   // | 37 : ADC.AIN1 || 38 : 1.8V     |
        NC, NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
    },
    // Pattern 2 : Right pin 1 (2,4,6...40)
    {
        // Header J4 GPIOs
        NC,        // Not used (pin 0)
        NC, NC,   // | 01 : 3.3V     || 02 : 5.0V     |
         0, NC,   // | 03 : I2C_SDA1 || 04 : 5.0V     |
         0, NC,   // | 05 : I2C_SCL1 || 06 : GND      |
         0,  1,   // | 07 : GPIO0_B6 || 08 : GPIO2_A4 |
        NC,  1,   // | 09 : GND      || 10 : GPIO2_A3 |
         0,  1,   // | 11 : GPIO0_C0 || 12 : GPIO2_A7 |
         0, NC,   // | 13 : GPIO0_C1 || 14 : GND      |
         0,  1,   // | 15 : GPIO0_C2 || 16 : GPIO2_B5 |
        NC,  1,   // | 17 : 3.3V     || 18 : GPIO3_B6 |
         0, NC,   // | 19 : GPIO3_C1 || 20 : GND      |
         0,  1,   // | 21 : GPIO3_C2 || 22 : GPIO2_B0 |
         0,  1,   // | 23 : GPIO3_C3 || 24 : GPIO3_A1 |
        NC,  1,   // | 25 : GND      || 26 : GPIO2_B1 |
         0,  1,   // | 27 : GPIO0_B4 || 28 : GPIO2_B3 |
         0, NC,   // | 29 : GPIO2_C0 || 30 : GND      |
         0,  1,   // | 31 : GPIO2_B7 || 32 : GPIO2_B2 |
         0, NC,   // | 33 : GPIO0_B5 || 34 : GND      |
         0,  1,   // | 35 : GPIO0_A5 || 36 : GPIO2_A6 |
        NC, NC,   // | 37 : ADC.AIN1 || 38 : 1.8V     |
        NC, NC,   // | 39 : PWRBTN   || 40 : ADC.AIN0 |
    },
    // Pattern 1 : ALL High
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
};

#endif  // __HEADER_C4_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
