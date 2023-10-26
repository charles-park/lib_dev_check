//------------------------------------------------------------------------------
/**
 * @file lib_dev_test.c
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
int str_to_int (char *str, int str_size)
{
    char conv[10];

    memset (conv, 0, sizeof(conv));
    memcpy (conv, str, str_size);
    return atoi (conv);
}

//------------------------------------------------------------------------------
int device_check (void *msg, char *resp)
{
    struct msg_info *m_info = (struct msg_info *)msg;
    int grp_id  = str_to_int (m_info->grp_id, SIZE_GRP_ID);
    int dev_id  = str_to_int (m_info->dev_id, SIZE_DEV_ID);
    // extra data or delay ms
    int extra   = str_to_int (m_info->extra, SIZE_EXTRA);
    char action = toupper    (m_info->action);
    int status  = 0;

    switch(grp_id) {
        case eGROUP_SYSTEM:     status = system_check   (dev_id, action, resp); break;
        case eGROUP_STORAGE:    status = storage_check  (dev_id, action, resp); break;
        case eGROUP_USB:        status = usb_check      (dev_id, action, resp); break;
        case eGROUP_HDMI:       status = hdmi_check     (dev_id, action, resp); break;
        case eGROUP_ADC:        status = adc_check      (dev_id, action, resp); break;
        case eGROUP_ETHERNET:   status = ethernet_check (dev_id, action, resp); break;
        // ADC board check (server)
        case eGROUP_HEADER:     status = header_check   (dev_id, action, resp); break;
        case eGROUP_AUDIO:      status = audio_check    (dev_id, action, resp); break;
        case eGROUP_LED:        status = led_check      (dev_id, action, resp); break;
        case eGROUP_PWM:        status = pwm_check      (dev_id, action, resp); break;
        default :               sprintf (resp, "%06d", 0);  break;
    }
    // ms delay
    if (extra)
        usleep (extra * 1000);

    return status;
}

//------------------------------------------------------------------------------
int device_setup (void)
{
    system_grp_init ();
    storage_grp_init ();
    usb_grp_init ();
    hdmi_grp_init ();
    adc_grp_init ();
    ethernet_grp_init ();
    header_grp_init ();
    audio_grp_init ();
    led_grp_init ();
    pwm_grp_init ();
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
