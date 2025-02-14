//------------------------------------------------------------------------------
/**
 * @file system.c
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
#include <pthread.h>
#include <sys/sysinfo.h>

//------------------------------------------------------------------------------
#include "../lib_dev_check.h"
#include "system.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct device_system {
    // device check value
    int mem_size;
    int res_x;
    int res_y;
    char fb_path[STR_PATH_LENGTH+ 1];
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
static struct device_system DeviceSYSTEM = {
    0, 0, 0, {0, }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int get_memory_size (void)
{
    struct sysinfo sinfo;
    int mem_size = 0;

    if (!sysinfo (&sinfo)) {
        if (sinfo.totalram) {

            mem_size = sinfo.totalram / 1024 / 1024;

            switch (mem_size) {
                case    8193 ... 16384: mem_size = 16;  break;
                case    4097 ... 8192:  mem_size = 8;   break;
                case    2049 ... 4096:  mem_size = 4;   break;
                case    1025 ... 2048:  mem_size = 2;   break;
                default :
                    mem_size = 0;
                    break;
            }
        }
    }
    return mem_size;
}

//------------------------------------------------------------------------------
static int get_fb_size (const char *path, int id)
{
    FILE *fp;
    int x, y;

    if (access (path, R_OK) == 0) {
        if ((fp = fopen(path, "r")) != NULL) {
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
        switch (id) {
            case eSYSTEM_FB_X:  return x;
            case eSYSTEM_FB_Y:  return y;
            default :           return 0;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
int system_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eSYSTEM_MEM:
            value  = get_memory_size();
            if (DeviceSYSTEM.mem_size)
                status = (DeviceSYSTEM.mem_size == value) ? 1 : -1;
            else
                status = value ? 1 : -1;
            break;
        case eSYSTEM_FB_X:
            value  = get_fb_size (DeviceSYSTEM.fb_path, id);
            status = (value == DeviceSYSTEM.res_x) ? 1 : -1;
            break;
        case eSYSTEM_FB_Y:
            value = get_fb_size (DeviceSYSTEM.fb_path, id);
            status = (value == DeviceSYSTEM.res_y) ? 1 : -1;
            break;
        case eSYSTEM_FB_SIZE:
            if ((get_fb_size (DeviceSYSTEM.fb_path, eSYSTEM_FB_X) == DeviceSYSTEM.res_x) &&
                (get_fb_size (DeviceSYSTEM.fb_path, eSYSTEM_FB_Y) == DeviceSYSTEM.res_y))
                status = 1;
            else
                value = -1;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void system_grp_init (char *cfg)
{
    char *tok;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            switch (atoi(tok)) {
                case eSYSTEM_MEM:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceSYSTEM.mem_size = atoi(tok);
                    break;
                case eSYSTEM_FB_X:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceSYSTEM.res_x = atoi(tok);
                    break;
                case eSYSTEM_FB_Y:
                    if ((tok = strtok (NULL, ",")) != NULL)
                    DeviceSYSTEM.res_y = atoi(tok);
                    break;
                case eSYSTEM_FB_SIZE:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceSYSTEM.fb_path, tok, strlen(tok));
                    break;
                default :
                    printf ("%s : error! unknown did = %d\n", __func__, atoi(tok));
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
