//------------------------------------------------------------------------------
/**
 * @file storage.c
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
#include "storage.h"

//------------------------------------------------------------------------------
struct device_storage {
    int init, boot_device;

    // Control path
    char path [STR_PATH_LENGTH +1];

    // compare value (read min, write min : MB/s)
    int rw_check[2];

    // read/write value
    int rw_value[2];

    // r/w mode(0 = read, 1 = write)
    int rw;

    // r/w thread
    int thread_en;
    pthread_t thread;
};

pthread_mutex_t mutex_storage = PTHREAD_MUTEX_INITIALIZER;

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_storage DeviceSTORAGE [eSTORAGE_END] = {
    // init, boot_device, path, rw_check, rw_value, rw_mode, thread_en, pthread,
    // eSTORAGE_EMMC
    { 0, 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // eSTORAGE_uSD (boot device : /root)
    { 0, 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // eSTORAGE_SATA
    { 0, 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
    // eSTORAGE_NVME
    { 0, 0, {0, }, {0, 0}, {0, 0}, 0, 0, 0} ,
};

//------------------------------------------------------------------------------
#define TEMP_FILE       "/tmp/wdat"

// Storage Read / Write (16 Mbytes, 1 block count)
const char *STORAGE_R_CHECK = "dd of=/dev/null bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync if=";
const char *STORAGE_W_CHECK = "dd if=/dev/zero bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync of=";

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int remove_tmp (const char *path)
{
    FILE *fp;
    char cmd[STR_PATH_LENGTH];

    if (access (path, R_OK) == 0) {
        memset  (cmd, 0x00, sizeof(cmd));
        sprintf (cmd, "rm -f %s 2>&1", path);

        if ((fp = popen (cmd, "r")) != NULL) {
            pclose(fp);
            return 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
static int storage_rw (struct device_storage *p_storage)
{
    FILE *fp;
    char cmd[STR_PATH_LENGTH *2], rdata[STR_PATH_LENGTH], *ptr;

    if (!access (p_storage->path, F_OK)) {
        memset  (cmd, 0x00, sizeof(cmd));
        if (p_storage->rw) {
            sprintf (cmd, "%s%s 2>&1 && sync",
                STORAGE_W_CHECK, p_storage->boot_device ? TEMP_FILE : p_storage->path);

        } else {
            sprintf (cmd, "%s%s 2>&1 && sync",
                STORAGE_R_CHECK, p_storage->path);
        }

        if ((fp = popen (cmd, "r")) != NULL) {
            while (fgets (rdata, sizeof(rdata), fp) != NULL) {
                if ((ptr = strstr (rdata, " MB/s")) != NULL) {
                    while (*ptr != ',') ptr--;
                    pclose(fp);
                    return atoi (ptr+1);
                }
            }
            pclose(fp);
        }
        return 0;
    }
    return -1;
}

//------------------------------------------------------------------------------
static void *thread_func_storage (void *arg)
{
    struct device_storage *p_storage = (struct device_storage *)arg;
    int retry = 5;

    p_storage->thread_en = 1;
    while (retry--) {
        pthread_mutex_lock(&mutex_storage);

        if (p_storage->rw_value[p_storage->rw] <= p_storage->rw_check[p_storage->rw])
            p_storage->rw_value[p_storage->rw] = storage_rw (p_storage);

        pthread_mutex_unlock(&mutex_storage);

        if (p_storage->rw_value[p_storage->rw] > p_storage->rw_check[p_storage->rw])
            break;

        usleep (100 * 1000);
    }
    p_storage->thread_en = 0;
    return arg;
}

//------------------------------------------------------------------------------
int storage_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);
    struct device_storage *p_storage = &DeviceSTORAGE[id];

    if (p_storage->init) {
        switch (id) {
            case eSTORAGE_eMMC: case eSTORAGE_uSD:
            case eSTORAGE_SATA: case eSTORAGE_NVME:

                do { usleep (100 * 1000); } while (p_storage->thread_en);

                p_storage->rw = DEVICE_ACTION(dev_id);

                pthread_create (&p_storage->thread, NULL,
                                    thread_func_storage, p_storage);

                do { usleep (100 * 1000); } while (p_storage->thread_en);

                if (p_storage->rw && p_storage->boot_device)
                    remove_tmp (TEMP_FILE);

                value  = p_storage->rw_value[p_storage->rw];
                status = (value > p_storage->rw_check[p_storage->rw]) ? 1 : -1;
                break;
            default :
                break;
        }
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void storage_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eSTORAGE_eMMC: case eSTORAGE_uSD:
                case eSTORAGE_SATA: case eSTORAGE_NVME:
                    DeviceSTORAGE[did].init = 1;

                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceSTORAGE[did].path, tok, strlen(tok));

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceSTORAGE[did].rw_check[0] = atoi(tok);

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceSTORAGE[did].rw_check[1] = atoi(tok);

                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceSTORAGE[did].boot_device = atoi(tok);

#if 0
                    // read = 0(default), write = 1
                    DeviceSTORAGE[did].rw = 0;
                    pthread_create (&DeviceSTORAGE[did].thread, NULL,
                                    thread_func_storage, &DeviceSTORAGE[did]);
#endif
                    break;

                default :
                    printf ("%s : error! unknown gid = %d\n", __func__, atoi(tok));
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
