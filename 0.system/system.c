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

//------------------------------------------------------------------------------
#include "../lib_dev_check.h"
#include "system.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct device_system {
    int mem, res_x, res_y;
    const char *fb_path;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define system devices */
//------------------------------------------------------------------------------
// Client LCD res (vu5)
#define LCD_RES_X   800
#define LCD_RES_Y   480

static struct device_system DeviceSYSTEM = {
    0, 0, 0, "/sys/class/graphics/fb0/virtual_size"
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
                case    4097 ... 8192:  mem_size = 8192;    break;
                case    2049 ... 4096:  mem_size = 8192;    break;
                case    1025 ... 2048:  mem_size = 8192;    break;
                default :
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
    }

    switch (id) {
        case eSYSTEM_FB_X:  return x;
        case eSYSTEM_FB_Y:  return y;
        default :           return 0;
    }
}

//------------------------------------------------------------------------------
int system_check (int id, char action, char *resp)
{
    int value = 0, ret = 0;

    if ((id >= eSYSTEM_END) || (access (DeviceSYSTEM.fb_path, R_OK) != 0)) {
        sprintf (resp, "%06d", 0);
        return 0;
    }

    switch (id) {
        case eSYSTEM_MEM:
            value = (action == 'I') ? DeviceSYSTEM.mem : get_memory_size();
            ret = value ? 1 : 0;
            break;
        case eSYSTEM_FB_X:
            value = (action == 'I') ? DeviceSYSTEM.res_x : get_fb_size (DeviceSYSTEM.fb_path, id);
            ret = (value == LCD_RES_X) ? 1 : 0;
            break;
        case eSYSTEM_FB_Y:
            value = (action == 'I') ? DeviceSYSTEM.res_y : get_fb_size (DeviceSYSTEM.fb_path, id);
            ret = (value == LCD_RES_Y) ? 1 : 0;
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", value);
    return ret;
}

//------------------------------------------------------------------------------
int system_grp_init (void)
{
    DeviceSYSTEM.mem = get_memory_size ();
    if (access (DeviceSYSTEM.fb_path, R_OK) == 0) {
        DeviceSYSTEM.res_x = get_fb_size (DeviceSYSTEM.fb_path, eSYSTEM_FB_X);
        DeviceSYSTEM.res_y = get_fb_size (DeviceSYSTEM.fb_path, eSYSTEM_FB_Y);
    }
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
