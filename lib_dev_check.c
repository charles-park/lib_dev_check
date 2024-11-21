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
int str_to_int (char *str, int str_size)
{
    char conv[10];

    memset (conv, 0, sizeof(conv));
    memcpy (conv, str, str_size);
    return atoi (conv);
}

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
        case eGID_AUDIO:    status = audio_check    (did, dev_resp);  break;
        case eGID_PWM:      status = pwm_check      (did, dev_resp);  break;
        case eGID_LED:      status = led_check      (did, dev_resp);  break;
        case eGID_IR:       status = ir_check       (did, dev_resp);  break;
        case eGID_GPIO:     status = gpio_check     (did, dev_resp);  break;
        case eGID_FW:       status = fw_check       (did, dev_resp);  break;
        default :
            sprintf (dev_resp, "0,%19s", "unkonwn");
            break;
    }

    SERIAL_RESP_FORM(resp, gid, did, dev_resp);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);

#if 0
    struct msg_info *m_info = (struct msg_info *)msg;
    int grp_id  = str_to_int (m_info->grp_id, SIZE_GRP_ID);
    int dev_id  = str_to_int (m_info->dev_id, SIZE_DEV_ID);
    // extra data or delay ms
    int extra   = str_to_int (m_info->extra, SIZE_EXTRA);
    char action = toupper    (m_info->action);
    switch(grp_id) {
        case eGROUP_ETHERNET:   status = ethernet_check (dev_id, action, resp); break;
        // ADC board check (server)
        case eGROUP_HEADER:     status = header_check   (dev_id, action, resp); break;
        default :               sprintf (resp, "%06d", 0);  break;
    }
    // ms delay
    if (extra)
        usleep (extra * 1000);
#endif
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
    audio_grp_init ();
    pwm_grp_init ();
    led_grp_init ();
    ir_grp_init ();
    gpio_grp_init ();
    fw_grp_init();
#if 0
    header_grp_init ();
#endif
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
