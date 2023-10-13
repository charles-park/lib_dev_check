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

#include "lib_dev_check.h"

//------------------------------------------------------------------------------
#include "0.system/system.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Client HDMI connect to VU8s (1280x800)
#define DEFAULT_PATH        "/sys/class/graphics/fb0/virtual_size"
#define DEFAULT_X_RES       1280
#define DEFAULT_Y_RES       800

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int action_to_enum (char action)
{
    switch (toupper(action)) {
        case 'C':   return  eACTION_C;
        case 'S':   return  eACTION_S;
        case 'L':   return  eACTION_L;
        case 'R':   return  eACTION_R;
        case 'W':   return  eACTION_W;
        case '-':   return  eACTION_NONE;
        default :   return  eACTION_NUM;
    }
}

//------------------------------------------------------------------------------
char enum_to_action (int int_enum)
{
    switch (int_enum) {
        case eACTION_C:     return  'C';
        case eACTION_S:     return  'S';
        case eACTION_L:     return  'L';
        case eACTION_R:     return  'R';
        case eACTION_W:     return  'W';
        case eACTION_NONE:  return  '-';
        default :           return  (int_enum - '0');
    }
}

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
    int gid    = str_to_int (m_info->gid, SIZE_GID);
    int dev_id = str_to_int (m_info->dev_id, SIZE_DEV_ID);
    int action = action_to_enum (m_info->action);

    switch(gid) {
        case eGROUP_SYSTEM:
            //int system_check (int id, char action, char *resp)
            return system_check (dev_id, action, resp);
        case eGROUP_ADC:
//            return adc_check (dev_id, action, resp);
        default :
            break;
    }

    return 0;
}

//------------------------------------------------------------------------------
int device_setup (const char *cfg_file)
{
    FILE *fp;
    char read_line[128];

    if (access (cfg_file, R_OK) == 0) {
        if ((fp = fopen (cfg_file, "r")) != NULL) {
            while (1) {
                memset (read_line, 0x00, sizeof(read_line));
                if (fgets(read_line, sizeof(read_line), fp) == NULL)
                    break;
                if (read_line[0] != '#') {
                    if (system_grp_init (read_line))    continue;
//                    if (adc_grp_init (read_line))
//                        continue;
                }
            }
            fclose (fp);
            return 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
