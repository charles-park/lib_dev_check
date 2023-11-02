//------------------------------------------------------------------------------
/**
 * @file audio.c
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
#define PLAY_TIME_SEC   2

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
    char cmd_line[STR_PATH_LENGTH];

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
pthread_mutex_t audio_mutex;
pthread_t audio_thread;

volatile int isAudioBusy = 0, AudioPlayTime = 0, AudioEnable = 0;
volatile char *AudioFileName = NULL;

//------------------------------------------------------------------------------
void *audio_thread_func (void *arg)
{
    FILE *fp;
    char cmd [STR_PATH_LENGTH];

    while (1) {
        if (!isAudioBusy && AudioEnable) {
            pthread_mutex_lock   (&audio_mutex);
            isAudioBusy = 1;
            pthread_mutex_unlock (&audio_mutex);

            memset (cmd, 0, sizeof(cmd));
            if ((AudioFileName != NULL) && AudioPlayTime) {
                sprintf (cmd, "aplay -Dhw:1,0 %s -d %d && sync",
                    AudioFileName, AudioPlayTime);
                if ((fp = popen (cmd, "r")) != NULL)
                    pclose(fp);
            }

            pthread_mutex_lock   (&audio_mutex);
            isAudioBusy = 0;    AudioEnable = 0;
            pthread_mutex_unlock (&audio_mutex);
        }
        usleep (1000);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int audio_check (int id, char action, char *resp)
{
    int value = 0, retry = PLAY_TIME_SEC + 1;;

    if ((id >= eAUDIO_END) || (DeviceAUDIO[id].is_file != 1)) {
        sprintf (resp, "%06d", 0);
        return 0;
    }

    switch (action) {
        case 'C':   /* wait audio stop */
            while (AudioEnable && retry--)  sleep (1);
            value = retry ? 1 : 0;
            break;
        case 'W':
            while (AudioEnable && retry--)  sleep (1);

            if (retry) {
                pthread_mutex_lock   (&audio_mutex);
                AudioFileName = DeviceAUDIO[id].path;
                AudioPlayTime = DeviceAUDIO[id].play_time;
                AudioEnable = 1;
                value = DeviceAUDIO[id].is_file;
                pthread_mutex_unlock (&audio_mutex);
            }
            else
                printf ("audio busy\n");

            break;
        default :
            break;
    }
    sprintf (resp, "%06d", value);
    return value;
}

//------------------------------------------------------------------------------
int audio_grp_init (void)
{
    int id;

    for (id = 0; id < eAUDIO_END; id++) {
        memset (DeviceAUDIO[id].path, 0, STR_PATH_LENGTH);
        DeviceAUDIO[id].is_file =
            find_file_path (DeviceAUDIO[id].fname, DeviceAUDIO[id].path);
    }
    pthread_create (&audio_thread, NULL, audio_thread_func, &DeviceAUDIO);
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
