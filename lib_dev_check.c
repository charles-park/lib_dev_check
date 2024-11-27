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
//#define SERIAL_RESP_FORM(buf, cmd, gid, did, resp)   sprintf (buf, "@,%1c,%02d,%04d,%22s,#\r\n", cmd, gid, did, resp)
//#define DEVICE_RESP_FORM_INT(buf, status, value) sprintf (buf, "%1c,%20d", status, value)
//#define DEVICE_RESP_FORM_STR(buf, status, value) sprintf (buf, "%1c,%20s", status, value)

#if 0
//void DEVICE_RESP_FORM_INT (void *buf, char cmd, int gid, int did, void *resp)
//void DEVICE_RESP_FORM_STR (void *buf, char cmd, int gid, int did, void *resp)

void SERIAL_RESP_FORM (void *buf, char cmd, int gid, int did, void *resp)
{
    int pos;

    memset  (buf, 0, SERIAL_RESP_SIZE);
    pos = sprintf (buf, "@,%c,%02d,%04d,%-22s,#\r\n", cmd, gid, did, resp);

//    pos = sprintf (buf, "@,%c,%02d,%04d,%21s,", cmd, gid, did, resp);
//    memcpy (&buf[pos], &resp[0], DEVICE_RESP_SIZE);
}
#endif

//------------------------------------------------------------------------------
//
// status value : 0 -> Wait, 1 -> Success, -1 -> Error
//
//------------------------------------------------------------------------------
int device_check (int gid, int did, char *dev_resp)
{
    int status  = 0;

    memset (dev_resp, 0, DEVICE_RESP_SIZE);

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
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(dev_resp), dev_resp);

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
