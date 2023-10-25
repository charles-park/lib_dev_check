//------------------------------------------------------------------------------
/**
 * @file lib_dev_test.h
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
#ifndef __LIB_DEV_TEST_H__
#define __LIB_DEV_TEST_H__

//------------------------------------------------------------------------------
#include "0.system/system.h"
#include "1.storage/storage.h"
#include "2.usb/usb.h"
#include "3.hdmi/hdmi.h"
#include "4.adc/adc.h"
#include "5.ethernet/ethernet.h"
#include "6.header/header.h"
#include "7.audio/audio.h"
#include "8.led/led.h"
#include "9.pwm/pwm.h"

//------------------------------------------------------------------------------
#define CONFIG_FILE_PATH    "/boot/"

//------------------------------------------------------------------------------
#define STR_PATH_LENGTH     128
#define STR_NAME_LENGTH     16

//------------------------------------------------------------------------------
#define SIZE_UI_ID      4
#define SIZE_GRP_ID     2
#define SIZE_DEV_ID     3
#define SIZE_EXTRA      6

struct msg_info {
    char    start;
    char    cmd;
    char    ui_id [SIZE_UI_ID];
    char    grp_id[SIZE_GRP_ID];
    char    dev_id[SIZE_DEV_ID];
    char    action;
    // extra data (response delay or mac write)
    char    extra [SIZE_EXTRA];
    char    end;
}   __attribute__((packed));

//------------------------------------------------------------------------------
enum {
    eGROUP_SYSTEM = 0,
    eGROUP_STORAGE,
    eGROUP_USB,
    eGROUP_HDMI,
    eGROUP_ADC,
    eGROUP_ETHERNET,
    eGROUP_HEADER,
    eGROUP_AUDIO,
    eGROUP_LED,
    eGROUP_PWM,
    eGROUP_END
};

//------------------------------------------------------------------------------
enum {
    eACTION_C = 0,
    eACTION_S,
    eACTION_L,
    eACTION_R,
    eACTION_W,
    eACTION_I,
    eACTION_NONE,
    eACTION_NUM,
    eACTION_END
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
extern int  device_check    (void *msg, char *resp);
extern int  device_setup    (void);

//------------------------------------------------------------------------------
#endif  // __LIB_DEV_TEST_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------