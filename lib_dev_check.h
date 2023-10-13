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
#define STR_PATH_LENGTH     128
#define STR_NAME_LENGTH     16

//------------------------------------------------------------------------------
#define SIZE_UI_ID      4
#define SIZE_GID        2
#define SIZE_DEV_ID     3
#define SIZE_R_DELAY    4

struct msg_info {
    char    start;
    char    cmd;
    char    ui_id[SIZE_UI_ID];
    char    gid[SIZE_GID];
    char    dev_id[SIZE_DEV_ID];
    char    action;
    char    resp_delay[SIZE_R_DELAY];
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
    eGROUP_FAN,
    eGROUP_GPIO,
    eGROUP_END
};

//------------------------------------------------------------------------------
enum {
    eACTION_C = 0,
    eACTION_S,
    eACTION_L,
    eACTION_R,
    eACTION_W,
    eACTION_NONE,
    eACTION_NUM,
    eACTION_END
};

//------------------------------------------------------------------------------
struct device_info {
    // 초기화 확인
    int     init;
    int     id;
    char    name [STR_NAME_LENGTH];
    char    path [STR_PATH_LENGTH];
    // 해당 Action에 따른 비교값 또는 최대값 저장, Action list(C, S, L, R, W, -, NUM)
    int     value[eACTION_END];
    int     min[eACTION_END];
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
extern int  action_to_enum  (char action);
extern char enum_to_action  (int int_enum);
extern int  device_check    (void *msg, char *resp);
extern int  device_setup    (const char *cfg_file);

//------------------------------------------------------------------------------
#endif  // __LIB_DEV_TEST_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------