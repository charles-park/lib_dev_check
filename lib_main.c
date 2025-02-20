//------------------------------------------------------------------------------
/**
 * @file lib_main.c
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
const char *gid_str [] = {
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
    "IR",
    "GPIO",
    "F/W USB",
};

const char *action_str [] = {
    "Read, Clear, PT0",
    "Write, Set, PT1",
    "Link, PT2",
    "PT3",
    "END"
};

//------------------------------------------------------------------------------
const char *id_system_str[] = {
    "MEM ",
    "FB_X",
    "FB_Y",
    "FB_SIZE",
};

const char *id_storage_str[] = {
    "eMMC",
    "uSD",
    "SATA",
    "NVME",
};

const char *id_usb_str[] = {
    "USB 0",
    "USB 1",
    "USB 2",
    "USB 3",
    "USB 4",
    "USB 5",
};

const char *id_hdmi_str[] = {
    "EDID",
    "HPD",
};

const char *id_adc_str[] = {
    "ADC_37",
    "ADC_40",
};

const char *id_ethernet_str[] = {
    "ETHERNET_IP",
    "ETHERNET_MAC",
    "ETHERNET_IPERF",
    "ETHERNET_LINK",
};

const char *id_header_str[] = {
    "HEADER_H40",
    "HEADER_H7",
    "HEADER_H14",
};

const char *id_audio_str[] = {
    "AUDIO_LEFT",
    "AUDIO_RIGHT",
};

const char *id_led_str[] = {
    "LED_POWER",
    "LED_ALIVE",
    "LED_LINK_100M",
    "LED_LINK_1G",
};

const char *id_pwm_str[] = {
    "PWM_0",
    "PWM_1",
};

const char *id_ir_str[] = {
    "IR_EVENT",
};

const char *id_gpio_str[] = {
    "GPIO_PIN",
};

const char *id_fw_str[] = {
    "FW_C4",
    "FW_XU4",
};

//------------------------------------------------------------------------------
struct cmd_list {
    // group id
    const int   g_id;
    // group name string
    const char  **g_str;
    const char  **id_str;
    const int   id_cnt;
};

struct cmd_list list[eGID_END] = {
    { eGID_SYSTEM  , &gid_str[eGID_SYSTEM]  , &id_system_str[0]  , eSYSTEM_END   },
    { eGID_STORAGE , &gid_str[eGID_STORAGE] , &id_storage_str[0] , eSTORAGE_END  },
    { eGID_USB     , &gid_str[eGID_USB]     , &id_usb_str[0]     , eUSB_END      },
    { eGID_HDMI    , &gid_str[eGID_HDMI]    , &id_hdmi_str[0]    , eHDMI_END     },
    { eGID_ADC     , &gid_str[eGID_ADC]     , &id_adc_str[0]     , eADC_END      },
    { eGID_ETHERNET, &gid_str[eGID_ETHERNET], &id_ethernet_str[0], eETHERNET_END },
    { eGID_HEADER  , &gid_str[eGID_HEADER]  , &id_header_str[0]  , eHEADER_END   },
    { eGID_AUDIO   , &gid_str[eGID_AUDIO]   , &id_audio_str[0]   , eAUDIO_END    },
    { eGID_LED     , &gid_str[eGID_LED]     , &id_led_str[0]     , eLED_END      },
    { eGID_PWM     , &gid_str[eGID_PWM]     , &id_pwm_str[0]     , ePWM_END      },
    { eGID_IR      , &gid_str[eGID_IR]      , &id_ir_str[0]      , eIR_END       },
    { eGID_GPIO    , &gid_str[eGID_GPIO]    , &id_gpio_str[0]    , eGPIO_END     },
    { eGID_FW      , &gid_str[eGID_FW]      , &id_fw_str[0]      , eFW_END       },
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
         "https://docs.google.com/spreadsheets/d/1igBObU7CnP6FRaRt-x46l5R77-8uAKEskkhthnFwtpY/edit?gid=719914769#gid=719914769\n"
         "\n"
         "  -g --group id     Group ID(0~99)\n"
         "  -d --device id    Device ID(0~9999)\n"
         "  -a --action       Action(0 = Clear, Read, PT0)\n"
         "                    Action(1 = Set, Write, PT1)\n"
         "                    Action(2 = Link, PT2)\n"
         "                    Action(3 = PT3)\n"
         "\n"
         "  e.g) system memory read.\n"
         "       lib_dev_test -g 0 -d 0 -a 0\n"
    );
    exit(1);
}

//------------------------------------------------------------------------------
/* Control variable */
//------------------------------------------------------------------------------
static int  OPT_VIEW_INFO = 0;
static int  OPT_GROUP_ID  = 0;
static int  OPT_DEVICE_ID = 0;
static int  OPT_ACTION    = 0;

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
            OPT_ACTION = atoi(optarg);
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
// message discription (serial)
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// start |,|cmd|,|GID|,|DID |,| status |,| value(%20s) |,| end | extra  |
//------------------------------------------------------------------------------
//   1    1  1  1  2  1  4   1     1    1       20      1   1      2      = 36bytes(add extra 38)
//------------------------------------------------------------------------------
//   @   |,| S |,| 00|,|0000|,|P/F/I/W |,|  resp data  |,|  #  | '\r\n' |
//------------------------------------------------------------------------------
void make_msg (int grp_id, int dev_id, char *dev_resp)
{
    char msg[SERIAL_RESP_SIZE +1];

    memset (msg, 0, sizeof(msg));

    SERIAL_RESP_FORM (msg, 'S', grp_id, DEVICE_ID(dev_id), dev_resp);

    printf ("\nResponse : size = %ld, msg = %s\n", strlen(msg), msg);
}

//------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    parse_opts(argc, argv);

    // Display cmd list
    if (OPT_VIEW_INFO) {
        int i;
        puts("");
        printf ("GROUP_ID(%d) = %s\n", OPT_GROUP_ID, *list[OPT_GROUP_ID].g_str);
        for (i = 0; i < list[OPT_GROUP_ID].id_cnt; i++)
            printf ("\tDEVICE_ID(%d) = %s\n", i, *(list[OPT_GROUP_ID].id_str + i));
        puts("");
    }
    if (argc < 7)
        print_usage(argv[0]);

    device_init ();
    {
        int ret, did = OPT_DEVICE_ID + (OPT_ACTION * 10);
        char dev_resp [DEVICE_RESP_SIZE];

        // device thread wait
        sleep (2);
        ret = device_check (OPT_GROUP_ID, did, dev_resp);

        if (OPT_GROUP_ID < eGID_END) {
            printf ("GROUP_ID(%d) = %s, DEVICE_ID(%d) = %s, ACTION(%s), ret = %d\n",
                OPT_GROUP_ID, *list[OPT_GROUP_ID].g_str,
                DEVICE_ID(did), *(list[OPT_GROUP_ID].id_str + DEVICE_ID(did)),
                action_str [DEVICE_ACTION(did)],
                ret);
            // Serial msg
            make_msg (OPT_GROUP_ID, did, dev_resp);
        }

        while (1)   sleep (1);
    }
    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#endif  // #if defined(__LIB_DEV_CHECK_APP__)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
