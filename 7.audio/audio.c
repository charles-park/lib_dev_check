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
    // audio file name
    char fname[STR_NAME_LENGTH];
    char path [STR_PATH_LENGTH];

    // ADC con_name
    char cname[STR_NAME_LENGTH];

    // ADC Check value (on, off)
    int max, min;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define audio devices */
//------------------------------------------------------------------------------
struct device_audio DeviceAUDIO [eAUDIO_END] = {
    // AUDIO LEFT
    { { 0, }, { 0, }, { 0, }, 0, 0 },
    // AUDIO RIGHT
    { { 0, }, { 0, }, { 0, }, 0, 0 },
    // AUDIO SLEFT
    { { 0, }, { 0, }, { 0, }, 0, 0 },
    // AUDIO SRIGHT
    { { 0, }, { 0, }, { 0, }, 0, 0 },
};

// Devuce H/W num. ch, play time
volatile int AudioHW = 0, AudioCH = 0, AudioTime = 0, AudioEnable = 0;

//------------------------------------------------------------------------------
// thread control variable
//------------------------------------------------------------------------------
// speaker-test -D hw:AudioHW,AudioCH -c 2 -t sine -f 1000 -p 2 -s 1 (left)
// speaker-test -D hw:AudioHW,AudioCH -c 2 -t sine -f 1000 -p 2 -s 2 (right)
//------------------------------------------------------------------------------
void *audio_thread_func (void *arg)
{
    struct device_audio *paudio = (struct device_audio *)arg;

    FILE *fp;
    char cmd [STR_PATH_LENGTH *2];

    AudioEnable = 1;

    memset  (cmd, 0, sizeof(cmd));
    sprintf (cmd, "aplay -Dhw:%d,%d %s -d %d && sync",
                AudioHW, AudioCH, paudio->path, AudioTime);

    if ((fp = popen (cmd, "w")) != NULL)
        pclose(fp);

    AudioEnable = 0;    usleep (100 *1000);

    return arg;
}

//------------------------------------------------------------------------------
void audio_thread_stop (void)
{
    FILE *fp;
    const char *cmd = "ps ax | grep aplay | awk '{print $1}' | xargs kill";

    if ((fp = popen (cmd, "w")) != NULL)
        pclose(fp);

    AudioEnable = 0;    usleep (100 *1000);
}

//------------------------------------------------------------------------------
int audio_data_check (int dev_id, int resp_i)
{
    int status = 0, id = DEVICE_ID(dev_id);
    switch (id) {
        case eAUDIO_LEFT: case eAUDIO_RIGHT: case eAUDIO_SLEFT: case eAUDIO_SRIGHT:
            if (DEVICE_ACTION(dev_id))
                status = (resp_i < DeviceAUDIO[id].min) ? 1 : 0;    // audio on (low)
            else
                status = (resp_i > DeviceAUDIO[id].max) ? 1 : 0;    // audio off (high)
            break;
        default :
            status = 0;
            break;
    }
    return status;
}

//------------------------------------------------------------------------------
int audio_check (int dev_id, char *resp)
{
    int status = 0, id = DEVICE_ID(dev_id);
    pthread_t audio_thread;

    if (AudioEnable)    audio_thread_stop();

    switch (id) {
        case eAUDIO_LEFT: case eAUDIO_RIGHT: case eAUDIO_SLEFT: case eAUDIO_SRIGHT:
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

    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', DeviceAUDIO[id].cname);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void audio_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eAUDIO_LEFT: case eAUDIO_RIGHT: case eAUDIO_SLEFT: case eAUDIO_SRIGHT:
                    if ((tok = strtok (NULL, ",")) != NULL) {
                        strncpy (DeviceAUDIO[did].fname, tok, strlen(tok));
                        find_file_path ((const char *)DeviceAUDIO[did].fname,
                                        (char *)DeviceAUDIO[did].path);
                    }
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceAUDIO[did].cname, tok, strlen(tok));

                    if ((tok = strtok (NULL, ",")) != NULL) DeviceAUDIO[did].max = atoi(tok);
                    if ((tok = strtok (NULL, ",")) != NULL) DeviceAUDIO[did].min = atoi(tok);
                    break;
                case eAUDIO_CFG:
                    if ((tok = strtok (NULL, ",")) != NULL) AudioHW   = atoi(tok);
                    if ((tok = strtok (NULL, ",")) != NULL) AudioCH   = atoi(tok);
                    if ((tok = strtok (NULL, ",")) != NULL) AudioTime = atoi(tok);

                    break;
                default :
                    printf ("%s : error! unknown did = %d\n", __func__, did);
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
