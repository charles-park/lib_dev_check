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
#define CONFIG_FILE_NAME    "dev_check.cfg"

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
};

//------------------------------------------------------------------------------
const char *id_system_str[] = {
    "MEM",
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
#if 0
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
#endif
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
         "  -f --dev_cfg      Device config file\n"
         "  -h --help         show help\n"
         "\n"
         "  e.g) Default cfg = dev_check.cfg\n"
         "       lib_dev_test \n"
         "       lib_dev_test -f {dev cfg file}\n"
    );
    exit(1);
}

//------------------------------------------------------------------------------
/* Control variable */
//------------------------------------------------------------------------------
static int  OPT_GROUP_ID  = 0;
static int  OPT_DEVICE_ID = 0;
static int  OPT_ACTION    = 0;
static char *OPT_CFG_FNAME = CONFIG_FILE_NAME;

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
    while (1) {
        static const struct option lopts[] = {
            { "cfg file" ,  1, 0, 'f' },
            { "help    " ,  0, 0, 'h' },
            { NULL, 0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "hf:", lopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'f':
            OPT_CFG_FNAME = optarg;
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
int get_int (void)
{
    char buf [STR_NAME_LENGTH] = { 0, }, ch;
    int pos = 0;

    while ('\n' != (ch = getc(stdin)) ) {
        buf [ pos ] = ch;
        if (pos < STR_NAME_LENGTH -1) pos++;
    }
    return atoi(buf);
}

//------------------------------------------------------------------------------
int show_list (const char *lists[], int list_cnt)
{
    int i;
    for (i = 0; i < list_cnt; i++) {
        if (i && !(i % 3))  printf ("\n");

        printf ("%d. %-14s\t", i, lists[i]);
    }
    printf ("\n");
    return list_cnt ? (list_cnt - 1) : 0;
}

//------------------------------------------------------------------------------
void get_device_info (void)
{
    system("clear");
    printf ("*** SELECT TEST ITEM (%s) ***\n", OPT_CFG_FNAME);
    printf ("\n[ SELECT GROUP ]\n");
    printf ("* GROUP_ID [0 - %d] = ", show_list(gid_str, sizeof(gid_str)/sizeof(gid_str[0])));
    OPT_GROUP_ID  = get_int();

    printf ("\n[ SELECT DEVICE ]\n");
    printf ("* DEVICE_ID[0 - %d] = ", show_list(list[OPT_GROUP_ID].id_str, list[OPT_GROUP_ID].id_cnt));
    OPT_DEVICE_ID = get_int();

    printf ("\n[ SELECT ACTION ]\n");
    printf ("* ACTION[0 - %d]    = ", show_list(action_str, sizeof(action_str)/sizeof(action_str[0])));
    OPT_ACTION    = get_int();

}

//------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    int did;
    char dev_resp [DEVICE_RESP_SIZE];

    parse_opts(argc, argv);

    // device thread wait
    device_setup (OPT_CFG_FNAME);   sleep (2);

    while (1)
    {
        get_device_info();

        printf ("\n========================================================\n");
        printf ("\n[ ** TEST ITEM INFO ** ]\n");
        printf ("GID = %d (%s), DID = %d (%s), ACTION = %d (%s)\n\n",
            OPT_GROUP_ID , gid_str[OPT_GROUP_ID],
            OPT_DEVICE_ID, list[OPT_GROUP_ID].id_str[OPT_DEVICE_ID],
            OPT_ACTION   , action_str[OPT_ACTION]
        );

        did = OPT_DEVICE_ID + (OPT_ACTION * 10);
        printf ("\n===> TEST ITEM RESULT (ret = %s) <===\n",
            device_check (OPT_GROUP_ID, did, dev_resp) == 1 ? "PASS" : "FAIL");

        // Serial msg
        make_msg (OPT_GROUP_ID, did, dev_resp); printf ("\n");

        // pause
        sleep (1);
        printf ("\nPress [Enter] key to continue....");   get_int ();
        printf ("\n========================================================\n");
    }
    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#endif  // #if defined(__LIB_DEV_CHECK_APP__)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
