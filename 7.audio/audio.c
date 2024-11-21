//------------------------------------------------------------------------------
/**
 * @file audio.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-21
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
#include <signal.h>
#include <sys/sysinfo.h>

//------------------------------------------------------------------------------
#include "../lib_dev_check.h"
#include "audio.h"

//------------------------------------------------------------------------------
struct device_audio {
    // find flag
    char is_file;
    // Control file name
    const char *fname;
    // play time (sec)
    int play_time;
    char path[STR_PATH_LENGTH];
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define audio devices */
//------------------------------------------------------------------------------
#define PLAY_TIME_SEC   5
#define THREAD_KILL_CNT 5

struct device_audio DeviceAUDIO [eAUDIO_END] = {
    // AUDIO LEFT
    { 0, "1khz_left.wav" , PLAY_TIME_SEC, { 0, } },
    // AUDIO RIGHT
    { 0, "1khz_right.wav", PLAY_TIME_SEC, { 0, } },
};

//------------------------------------------------------------------------------
// return 1 : find success, 0 : not found
//------------------------------------------------------------------------------
static int find_file_path (const char *fname, char *file_path)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH * 2];

    memset (cmd_line, 0, sizeof(cmd_line));
    sprintf(cmd_line, "%s\n", "pwd");

    if (NULL != (fp = popen(cmd_line, "r"))) {
        memset (cmd_line, 0, sizeof(cmd_line));
        fgets  (cmd_line, STR_PATH_LENGTH, fp);
        pclose (fp);

        strncpy (file_path, cmd_line, strlen(cmd_line)-1);

        memset (cmd_line, 0, sizeof(cmd_line));
        sprintf(cmd_line, "find -name %s\n", fname);
        if (NULL != (fp = popen(cmd_line, "r"))) {
            memset (cmd_line, 0, sizeof(cmd_line));
            fgets  (cmd_line, STR_PATH_LENGTH, fp);
            pclose (fp);
            if (strlen(cmd_line)) {
                strncpy (&file_path[strlen(file_path)], &cmd_line[1], strlen(cmd_line)-1);
                file_path[strlen(file_path)-1] = ' ';
                return 1;
            }
            return 0;
        }
    }
    pclose(fp);
    return 0;
}

//------------------------------------------------------------------------------
// thread control variable
//------------------------------------------------------------------------------
pthread_t audio_thread;

volatile int AudioEnable = 0;

//------------------------------------------------------------------------------
void *audio_thread_func (void *arg)
{
    struct device_audio *paudio = (struct device_audio *)arg;

    if (paudio->is_file && paudio->play_time) {
        FILE *fp;
        char cmd [STR_PATH_LENGTH *2];

        AudioEnable = 1;
        memset  (cmd, 0, sizeof(cmd));
        sprintf (cmd, "aplay -Dhw:1,0 %s -d %d && sync",
                                paudio->path, paudio->play_time);

        if ((fp = popen (cmd, "r")) != NULL)
            pclose(fp);
    }
    AudioEnable = 0;
    return arg;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int audio_check (int dev_id, char *resp)
{
    int value = THREAD_KILL_CNT, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eAUDIO_LEFT: case eAUDIO_RIGHT:
            if (AudioEnable) {
                while ((value--) && (pthread_kill (audio_thread, 0) != ESRCH))
                    usleep (1000);

                printf ("audio_thread killed %s!\n", value ? "success" : "fail");

                // thread kill fail
                if (!value) {   status = -1;    break; }
            }

            status = 1;
            if (DEVICE_ACTION(dev_id)) {
                if (pthread_create (&audio_thread, NULL, audio_thread_func, &DeviceAUDIO[id])) {
                    printf ("%s : pthread_create error!\n", __func__);
                    status = -1;
                }
            }
            break;
        default :
            break;
    }

    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'P' : 'F', id ? "AUDIO RIGHT" : "AUDIO_LEFT");
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int audio_grp_init (void)
{
    int id;

    for (id = 0; id < eAUDIO_END; id++) {
        memset (DeviceAUDIO[id].path, 0, sizeof(DeviceAUDIO[id].path));
        DeviceAUDIO[id].is_file =
            find_file_path (DeviceAUDIO[id].fname, DeviceAUDIO[id].path);
        printf ("%s : (%d)%s\n", __func__, DeviceAUDIO[id].is_file, DeviceAUDIO[id].path);
    }
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
