//------------------------------------------------------------------------------
/**
 * @file storage.c
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
#include "storage.h"

//------------------------------------------------------------------------------
struct device_storage {
    // Control path
    const char *path;
    // compare value (read min, write min : MB/s)
    const int r_min, w_min;
    // read value
    int value;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define storage devices */
//------------------------------------------------------------------------------
// Boot device define (uSD)
#define BOOT_DEVICE     eSTORAGE_uSD
#define TEMP_FILE       "/tmp/wdat"

struct device_storage DeviceSTORAGE [eSTORAGE_END] = {
    // path, r_min(MB/s), w_min(MB/s), read
    // eSTORAGE_EMMC
    { "/dev/mmcblk0", 140,  70,   0 },
    // eSTORAGE_SD (boot device : /root)
    { "/dev/mmcblk1",  50,  20,   0 },
    // eSTORAGE_SATA
    {           NULL,   0,   0,   0 },
    // eSTORAGE_NVME
    { "/dev/nvme0n1", 250, 150,   0 },
};

// Storage Read / Write (16 Mbytes, 1 block count)
#define STORAGE_R_CHECK "dd of=/dev/null bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync if="
#define STORAGE_W_CHECK "dd if=/dev/zero bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync of="

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
static int storage_rw (const char *path, const char *check_cmd)
{
    FILE *fp;
    char cmd[STR_PATH_LENGTH], rdata[STR_PATH_LENGTH], *ptr;

    memset  (cmd, 0x00, sizeof(cmd));
    sprintf (cmd, "%s%s 2>&1", check_cmd, path);

    if ((fp = popen (cmd, "r")) != NULL) {
        while (fgets (rdata, sizeof(rdata), fp) != NULL) {
            if ((ptr = strstr (rdata, " MB/s")) != NULL) {
                while (*ptr != ',') ptr--;
                return atoi (ptr+1);
            }
        }
        pclose(fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
int storage_check (int id, char action, char *resp)
{
    int value = 0;

    if ((id >= eSTORAGE_END) || (access (DeviceSTORAGE[id].path, R_OK) != 0)) {
        sprintf (resp, "%06d", 0);
        return 0;
    }

    switch (action) {
        case 'I':   case 'R':
            if (action == 'I')
                value = DeviceSTORAGE[id].value;
            else
                value = storage_rw (DeviceSTORAGE[id].path, STORAGE_R_CHECK);

            if (value < DeviceSTORAGE[id].r_min)
                value = 0;
            break;
        case 'W':
            /* boot device의 경우 /dev/node를 바로 r/w할 수 없기 때문. */
            /* tmp파일을 생성하는데 걸리는 시간을 측정함. */
            if (id == BOOT_DEVICE) {
                value = storage_rw (TEMP_FILE, STORAGE_W_CHECK);
                remove_tmp (TEMP_FILE);
            }
            else
                value = storage_rw (DeviceSTORAGE[id].path, STORAGE_W_CHECK);

            if (value < DeviceSTORAGE[id].w_min)
                value = 0;
            break;
        case 'L':
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", value);
    return value ? 1 : 0;
}

//------------------------------------------------------------------------------
int storage_grp_init (void)
{
    int i;

    for (i = 0; i < eSTORAGE_END; i++) {
        if ((access (DeviceSTORAGE[i].path, R_OK)) == 0)
            DeviceSTORAGE[i].value =
                storage_rw (DeviceSTORAGE[i].path, STORAGE_R_CHECK);
    }

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
