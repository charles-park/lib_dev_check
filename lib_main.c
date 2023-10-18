//------------------------------------------------------------------------------
/**
 * @file lib_main.c
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

//------------------------------------------------------------------------------
#include "lib_dev_check.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if defined(__LIB_DEV_CHECK_APP__)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char gid_str[eGROUP_END][STR_NAME_LENGTH] = {
    "SYSTEM",
    "STORAGE",
    "USB",
    "HDMI",
    "ADC",
    "ETHERNET",
    "HEADER_GPIO",
    "AUDIO",
    "LED",
    "PWM",
};

//------------------------------------------------------------------------------
const char id_system_str[eSYSTEM_END][STR_NAME_LENGTH] = {
    "MEM ",
    "FB_X",
    "FB_Y"
};

const char id_storage_str[eSTORAGE_END][STR_NAME_LENGTH] = {
    "eMMC",
    "uSD",
    "SATA",
    "NVME",
};

const char id_usb_str[eUSB_END][STR_NAME_LENGTH] = {
    "USB 3.0",
    "USB 2.0",
    "USB OTG",
    "USB Header",
};

const char id_hdmi_str[eHDMI_END][STR_NAME_LENGTH] = {
    "EDID",
    "HPD",
};

const char id_adc_str[eADC_END][STR_NAME_LENGTH] = {
    "ADC_37",
    "ADC_40",
};

const char id_ethernet_str[eETHERNET_END][STR_NAME_LENGTH] = {
    "ETHERNET_IP",
    "ETHERNET_MAC",
    "ETHERNET_IPERF",
    "ETHERNET_LINK",
};

const char id_header_str[eHEADER_END][STR_NAME_LENGTH] = {
    "HEADER_H40_H14",
    "HEADER_GPIO",
};

const char id_audio_str[eAUDIO_END][STR_NAME_LENGTH] = {
    "AUDIO_LEFT",
    "AUDIO_RIGHT",
};

const char id_led_str[eLED_END][STR_NAME_LENGTH] = {
    "LED_POWER",
    "LED_ALIVE",
};

const char id_pwm_str[ePWM_END][STR_NAME_LENGTH] = {
    "PWM_0",
    "PWM_1",
};

//------------------------------------------------------------------------------
struct cmd_list {
    // group id
    const int   g_id;
    // group name string
    const char  *g_str;
    const char  *id_str;
    const int   id_cnt;
};

struct cmd_list list[eGROUP_END] = {
    { eGROUP_SYSTEM  , gid_str[eGROUP_SYSTEM]  , id_system_str[0]  , eSYSTEM_END   },
    { eGROUP_STORAGE , gid_str[eGROUP_STORAGE] , id_storage_str[0] , eSTORAGE_END  },
    { eGROUP_USB     , gid_str[eGROUP_USB]     , id_usb_str[0]     , eUSB_END      },
    { eGROUP_HDMI    , gid_str[eGROUP_HDMI]    , id_hdmi_str[0]    , eHDMI_END     },
    { eGROUP_ADC     , gid_str[eGROUP_ADC]     , id_adc_str[0]     , eADC_END      },
    { eGROUP_ETHERNET, gid_str[eGROUP_ETHERNET], id_ethernet_str[0], eETHERNET_END },
    { eGROUP_HEADER  , gid_str[eGROUP_HEADER]  , id_header_str[0]  , eHEADER_END   },
    { eGROUP_AUDIO   , gid_str[eGROUP_AUDIO]   , id_audio_str[0]   , eAUDIO_END    },
    { eGROUP_LED     , gid_str[eGROUP_LED]     , id_led_str[0]     , eLED_END      },
    { eGROUP_PWM     , gid_str[eGROUP_PWM]     , id_pwm_str[0]     , ePWM_END      },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// 문자열 변경 함수. 입력 포인터는 반드시 메모리가 할당되어진 변수여야 함.
//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = tolower(*p);
}

//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = toupper(*p);
}

//------------------------------------------------------------------------------
//
// JIG Protocol(V2.0)
// https://docs.google.com/spreadsheets/d/1Of7im-2I5m_M-YKswsubrzQAXEGy-japYeH8h_754WA/edit#gid=0
//
//------------------------------------------------------------------------------
static void print_usage (const char *prog)
{
    puts("");
    printf("Usage: %s [-g:group] [-d:dev id] [-a:action]\n", prog);
    puts("\n"
         "Protocol)\n"
         "https://docs.google.com/spreadsheets/d/1Of7im-2I5m_M-YKswsubrzQAXEGy-japYeH8h_754WA/edit#gid=0\n"
         "\n"
         "  -g --group id     Group ID(0~99)\n"
         "  -d --device id    Device ID(0~999)\n"
         "  -a --action       Action(Clear/Set/Link/Read/Write/Init/0~9)\n"
         "\n"
         "  e.g) system memory read.\n"
         "       lib_dev_test -g 0 -d 0 -a r\n"
    );
    exit(1);
}

//------------------------------------------------------------------------------
/* Control variable */
//------------------------------------------------------------------------------
static char OPT_ACTION    = 0;
static char OPT_VIEW_INFO = 0;
static int  OPT_GROUP_ID  = 0;
static int  OPT_DEVICE_ID = 0;

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
    while (1) {
        static const struct option lopts[] = {
            { "group"    ,  1, 0, 'g' },
            { "device_id",  1, 0, 'd' },
            { "action"   ,  1, 0, 'a' },
            { "view"     ,  0, 0, 'v' },
            { NULL, 0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "g:d:a:vh", lopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'g':
            OPT_GROUP_ID = atoi(optarg);
            break;
        case 'd':
            OPT_DEVICE_ID = atoi(optarg);
            break;
        case 'a':
            toupperstr (optarg);
            OPT_ACTION = optarg[0];
            break;
        case 'v':
            OPT_VIEW_INFO = 1;
            break;
        case 'h':
        default:
            print_usage(argv[0]);
            break;
        }
    }
}

//------------------------------------------------------------------------------
//
// message discription
//
//------------------------------------------------------------------------------
// start | cmd | ui id | gid | dev_id | action | r_delay | end (total 17 bytes)
//   @   |  C  |  0000 |  00 |   000  |    0   |   0000  | #
//------------------------------------------------------------------------------
void make_msg (char *msg, int gid, int dev_id, char action)
{
    sprintf (msg, "@C%04d%02d%03d%c%04d#", 0, gid, dev_id, action, 0);
    printf ("make msg = %s, size = %ld\n", msg, sizeof(struct msg_info));
}

//------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    parse_opts(argc, argv);

    if (argc < 7)
        print_usage(argv[0]);

    // Display cmd list
    if (OPT_VIEW_INFO) {
        int i;
        puts("");
        printf ("GROUP_ID(%d) = %s\n", OPT_GROUP_ID, list[OPT_GROUP_ID].g_str);
        for (i = 0; i < list[OPT_GROUP_ID].id_cnt; i++)
            printf ("\tDEVICE_ID(%d) = %s\n", i, list[OPT_GROUP_ID].id_str + i * STR_NAME_LENGTH);
        puts("");
    }

    {
        char msg[sizeof(struct msg_info)+1], resp[10];
        int ret;

        memset (msg, 0, sizeof(msg));
        memset (resp, 0, sizeof(resp));

        make_msg (msg, OPT_GROUP_ID, OPT_DEVICE_ID, OPT_ACTION);

        device_setup();
        ret = device_check (msg, resp);
        if (OPT_GROUP_ID == eGROUP_AUDIO)
            sleep (3);
        printf ("resp = %s, return = %d\n", resp, ret);
    }
    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#endif  // #if defined(__LIB_DEV_CHECK_APP__)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
