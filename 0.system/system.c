//------------------------------------------------------------------------------
/**
 * @file system.c
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
#include <pthread.h>

#include <sys/sysinfo.h>

#include "../lib_dev_check.h"
#include "system.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static struct device_info   SystemGrp [eSYSTEM_END];

//------------------------------------------------------------------------------
static int get_memory_size (struct device_info *dev_info, int action, char *resp)
{
    struct sysinfo sinfo;
    int mem_size;

    if (!sysinfo (&sinfo)) {
        if (sinfo.totalram) {
            mem_size = sinfo.totalram / 1024 / 1024;

            switch (mem_size) {
                case    4097 ... 8192:
                    mem_size = 8192;
                    memcpy (resp, "008192", strlen("008192"));
                    break;
                case    2049 ... 4096:
                    mem_size = 8192;
                    memcpy (resp, "008192", strlen("004096"));
                    break;
                case    1025 ... 2048:
                    mem_size = 8192;
                    memcpy (resp, "008192", strlen("002048"));
                    break;
                default :
                    memcpy (resp, "000000", strlen("000000"));
                    return 0;
            }
            if (dev_info->value[action])
                return mem_size != dev_info->value[action] ? 0 : 1;

            return 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
static int get_fb_size (struct device_info *dev_info, int action, char *resp)
{
    FILE *fp;
    int x, y;

    if (access (dev_info->path, R_OK) == 0) {
        if ((fp = fopen(dev_info->path, "r")) != NULL) {
            char rdata[16], *ptr;
            memset (rdata, 0x00, sizeof(rdata));
            if (fgets (rdata, sizeof(rdata), fp) != NULL) {
                if ((ptr = strtok (rdata, ",")) != NULL)
                    x = atoi(ptr);

                if ((ptr = strtok (NULL, ",")) != NULL)
                    y = atoi(ptr);
            }
            fclose(fp);
        }

        if ((dev_info->id == eSYSTEM_FB_X) && (dev_info->value[action] == x)) {
            sprintf (resp, "%06d", x);
            return 1;
        }
        if ((dev_info->id == eSYSTEM_FB_Y) && (dev_info->value[action] == y)) {
            sprintf (resp, "%06d", y);
            return 1;
        }
    }
    sprintf (resp, "%06d", 0);
    return 0;
}

//------------------------------------------------------------------------------
int system_check (int id, int action, char *resp)
{
    if (!SystemGrp[id].init)
        return 0;

    switch (id) {
        case eSYSTEM_MEM:
            return get_memory_size  (&SystemGrp[id], action, resp);
        case eSYSTEM_FB_X:  case eSYSTEM_FB_Y:
            return get_fb_size      (&SystemGrp[id], action, resp);
        default :
            break;
    }
    return 0;
}

//------------------------------------------------------------------------------
// "," 구문자사이에 공백이 없고 데이터가 반드시 있어야 함. 문자열 처리시 에러 발생.
//------------------------------------------------------------------------------
//
// 작성 형식
//
// CFG_{GROUP_NAME},{device id},{dev name},{control path},{action type},{check value}
//
//------------------------------------------------------------------------------
#define CFG_GROUP_STR   "CFG_SYSTEM"

int system_grp_init (char *cfg_line)
{
    int dev_id, action;
    char *ptr;

    if (strncmp (CFG_GROUP_STR, cfg_line, strlen(CFG_GROUP_STR)))
        return 0;

#if defined(__LIB_DEV_TEST_APP__)
    printf ("%s : CFG_LINE = %s\n", __func__, cfg_line);
#endif

    // CFG_SYSTEM string check
    ptr = strtok (cfg_line, ",");

    // get Device ID
    if ((ptr = strtok (NULL, ",")) != NULL)
        dev_id = atoi (ptr);

    if (!SystemGrp[dev_id].init) {
        memset (&SystemGrp[dev_id], 0, sizeof(struct device_info));
        SystemGrp[dev_id].id    = dev_id;
        SystemGrp[dev_id].init  = 1;
    }

    // device name
    if ((ptr = strtok (NULL, ",")) != NULL)
        strncpy (SystemGrp[dev_id].name, ptr, strlen (ptr));
    // control file path
    if ((ptr = strtok (NULL, ",")) != NULL)
        strncpy (SystemGrp[dev_id].path, ptr, strlen (ptr));
    // action type
    if ((ptr = strtok (NULL, ",")) != NULL)
        action = action_to_enum(*ptr);
    // compare value
    if ((ptr = strtok (NULL, ",")) != NULL)
        SystemGrp[dev_id].value[action] = atoi (ptr);

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
