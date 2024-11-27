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
    int res_x;
    int res_y;
    char fb_path[STR_PATH_LENGTH+ 1];
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define system devices */
//------------------------------------------------------------------------------
// Client LCD res (vu5)
// ODROID-C4
#define DEFAULT_RES_X   (800)
#define DEFAULT_RES_Y   (480 * 2)

static struct device_system DeviceSYSTEM = {
    DEFAULT_RES_X, DEFAULT_RES_Y, "/sys/class/graphics/fb0/virtual_size"
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
static void default_config_write (const char *fname)
{
    FILE *fp;
    char value [STR_PATH_LENGTH *2 +1];

    if ((fp = fopen(fname, "wt")) == NULL)
        return;

    // default value write
    fputs   ("# info : res_x, res_y, fb_path \n", fp);
    memset  (value, 0, STR_PATH_LENGTH *2);
    sprintf (value, "%d,%d,%s,\n", DeviceSYSTEM.res_x, DeviceSYSTEM.res_y, DeviceSYSTEM.fb_path);
    fputs   (value, fp);
    fclose  (fp);
}

//------------------------------------------------------------------------------
static void default_config_read (void)
{
    FILE *fp;
    char fname [STR_PATH_LENGTH +1], value [STR_PATH_LENGTH +1], *ptr;

    memset  (fname, 0, STR_PATH_LENGTH);
    sprintf (fname, "%sjig-%s.cfg", CONFIG_FILE_PATH, "system");

    if (access (fname, R_OK) != 0) {
        default_config_write (fname);
        return;
    }

    if ((fp = fopen(fname, "r")) == NULL)
        return;

    while(1) {
        memset (value , 0, STR_PATH_LENGTH);
        if (fgets (value, sizeof (value), fp) == NULL)
            break;

        switch (value[0]) {
            case '#':   case '\n':
                break;
            default :
                // res_x, res_y, fb_path
                if ((ptr = strtok (value, ",")) != NULL)
                    DeviceSYSTEM.res_x = atoi (ptr);
                if ((ptr = strtok ( NULL, ",")) != NULL)
                    DeviceSYSTEM.res_y = atoi (ptr);
                if ((ptr = strtok ( NULL, ",")) != NULL) {
                    memset (DeviceSYSTEM.fb_path, 0, STR_PATH_LENGTH);
                    strcpy (DeviceSYSTEM.fb_path, ptr);
                }
                break;
        }
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
int system_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eSYSTEM_MEM:
            value  = get_memory_size();
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
            if ((DeviceSYSTEM.res_x == DEFAULT_RES_X) &&
                (DeviceSYSTEM.res_y == DEFAULT_RES_Y))
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
int system_grp_init (void)
{
    default_config_read();
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
