//------------------------------------------------------------------------------
/**
 * @file lib_dev_check.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-19
 *
 * @package apt install iperf3, nmap, ethtool, usbutils, alsa-utils
 *
 * @copyright Copyright (c) 2022
 *
 */
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

//------------------------------------------------------------------------------
#include "lib_dev_check.h"

//------------------------------------------------------------------------------
//
// status value : 0 -> Wait, 1 -> Success, -1 -> Error
//
//------------------------------------------------------------------------------
int device_check (int gid, int did, char *resp)
{
    int status  = 0;
    char dev_resp[DEVICE_RESP_SIZE];

    memset (resp, 0, SERIAL_RESP_SIZE);
    memset (dev_resp, 0, sizeof(dev_resp));

    switch(gid) {
        case eGID_SYSTEM:   status = system_check   (did, dev_resp);  break;
        case eGID_ADC:      status = adc_check      (did, dev_resp);  break;
        case eGID_HDMI:     status = hdmi_check     (did, dev_resp);  break;
        case eGID_STORAGE:  status = storage_check  (did, dev_resp);  break;
        case eGID_USB:      status = usb_check      (did, dev_resp);  break;
        case eGID_ETHERNET: status = ethernet_check (did, dev_resp);  break;
        case eGID_HEADER:   status = header_check   (did, dev_resp);  break;
        case eGID_AUDIO:    status = audio_check    (did, dev_resp);  break;
        case eGID_PWM:      status = pwm_check      (did, dev_resp);  break;
        case eGID_LED:      status = led_check      (did, dev_resp);  break;
        case eGID_IR:       status = ir_check       (did, dev_resp);  break;
        case eGID_GPIO:     status = gpio_check     (did, dev_resp);  break;
        case eGID_FW:       status = fw_check       (did, dev_resp);  break;
        default :
            sprintf (dev_resp, "0,%20s", "unkonwn");
            break;
    }

    SERIAL_RESP_FORM(resp, gid, did, dev_resp);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);

    return status;
}

//------------------------------------------------------------------------------
int device_setup (void)
{
    // usb f/w upgrade check
    fw_grp_init();
    // iperf, ip, mac, check
    ethernet_grp_init ();

    // sysfile read
    system_grp_init ();
    hdmi_grp_init ();
    adc_grp_init ();

    // thread func
    ir_grp_init ();
    storage_grp_init ();
    usb_grp_init ();

    // i2c ADC check
    header_grp_init ();
    // audio 1Khz L, R
    audio_grp_init ();
    // pwm1, pwm2
    pwm_grp_init ();
    // alive, power, eth_green, eth_orange
    led_grp_init ();
    // gpio number test
    gpio_grp_init ();

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
