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
    int gid     = str_to_int (m_info->gid, SIZE_GID);
    int dev_id  = str_to_int (m_info->dev_id, SIZE_DEV_ID);
    char action = toupper    (m_info->action);

    switch(gid) {
        case eGROUP_SYSTEM:     return system_check (dev_id, action, resp);
        case eGROUP_STORAGE:    return storage_check(dev_id, action, resp);
        case eGROUP_USB:        return usb_check    (dev_id, action, resp);
        case eGROUP_HDMI:       return hdmi_check   (dev_id, action, resp);
        case eGROUP_ADC:        return adc_check    (dev_id, action, resp);

        case eGROUP_AUDIO:      return audio_check  (dev_id, action, resp);
        case eGROUP_LED:        return led_check    (dev_id, action, resp);
        case eGROUP_PWM:        return pwm_check    (dev_id, action, resp);
        default :
            break;
    }
    sprintf (resp, "%06d", 0);
    return 0;
}

//------------------------------------------------------------------------------
int device_setup (void)
{
    system_grp_init ();
    storage_grp_init ();
    usb_grp_init ();
    hdmi_grp_init ();
    adc_grp_init ();
// ethernet
// header
    audio_grp_init ();
    led_grp_init ();
    pwm_grp_init ();
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
