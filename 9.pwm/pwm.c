//------------------------------------------------------------------------------
/**
 * @file pwm.c
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
#include "pwm.h"

//------------------------------------------------------------------------------
struct device_pwm {
    char path [STR_PATH_LENGTH];
    int pwm_ch;
    int period;
    int duty;
    char cname [STR_NAME_LENGTH];
    int max, min;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_pwm DevicePWM [ePWM_END] = {
    // PWM0
    { { 0, }, 0, 0, 0, { 0, }, 0, 0 },
    // PWM1
    { { 0, }, 0, 0, 0, { 0, }, 0, 0 },
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int pwm_read (const char *path)
{
    char rdata[16];
    FILE *fp;

    memset (rdata, 0, sizeof (rdata));
    // adc raw value get
    if ((fp = fopen(path, "r")) != NULL) {
        fread (rdata, 1, sizeof(rdata), fp);
        fclose(fp);
    }

    return atoi(rdata);
}

//------------------------------------------------------------------------------
static int pwm_write (const char *path, const char *wdata)
{
    FILE *fp;

    // adc raw value get
    if ((fp = fopen(path, "w")) != NULL) {
        fwrite (wdata, 1, strlen(wdata), fp);
        fclose(fp);
    }

    return atoi(wdata);
}

//------------------------------------------------------------------------------
static int pwm_enable (struct device_pwm *pwm, int enable)
{
    char f_path[STR_PATH_LENGTH *2] = { 0, }, w_data[2] = { 0, };

    memset    (f_path, 0, sizeof(f_path)); memset  (w_data, 0, sizeof(w_data));
    sprintf   (f_path, "%s/pwm%d/enable", pwm->path, pwm->pwm_ch);
    sprintf   (w_data, "%d", enable);

    return pwm_write (f_path, w_data);
}

//------------------------------------------------------------------------------
static void pwm_config (struct device_pwm *pwm)
{
    char f_path[STR_PATH_LENGTH *2] = { 0, }, w_data[2] = { 0, };

    memset    (f_path, 0, sizeof(f_path)); memset  (w_data, 0, sizeof(w_data));
    sprintf   (f_path, "%s/export", pwm->path);
    sprintf   (w_data, "%d",pwm->pwm_ch);
    pwm_write (f_path, w_data);

    memset    (f_path, 0, sizeof(f_path)); memset  (w_data, 0, sizeof(w_data));
    sprintf   (f_path, "%s/pwm%d/peroid", pwm->path, pwm->pwm_ch);
    sprintf   (w_data, "%d", pwm->period);
    pwm_write (f_path, w_data);

    memset    (f_path, 0, sizeof(f_path)); memset  (w_data, 0, sizeof(w_data));
    sprintf   (f_path, "%s/pwm%d/duty", pwm->path, pwm->pwm_ch);
    sprintf   (w_data, "%d", pwm->duty);
    pwm_write (f_path, w_data);

    pwm_enable(pwm, 0);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int pwm_data_check (int dev_id, int resp_i)
{
    int status = 0, id = DEVICE_ID(dev_id);
    switch (id) {
        case ePWM_0: case ePWM_1:
            if (DEVICE_ACTION(dev_id))
                status = (resp_i > DevicePWM[id].max) ? 1 : 0;    // pwm on
            else
                status = (resp_i < DevicePWM[id].min) ? 1 : 0;    // pwm off
            break;
        default :
            status = 0;
            break;
    }
    return status;
}

//------------------------------------------------------------------------------
int pwm_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case ePWM_0: case ePWM_1:
            value  = pwm_enable (&DevicePWM[id], DEVICE_ACTION(dev_id) ? 1 : 0);
            status = (value == pwm_read (DevicePWM[id].path)) ? 1 : -1;
            break;
        default :
            break;
    }
    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', DevicePWM[id].cname);

    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void pwm_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case ePWM_0: case ePWM_1:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DevicePWM[did].path, tok, strlen(tok));

                    // PWM Channel
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DevicePWM[did].pwm_ch = atoi(tok);

                    // PWM Period
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DevicePWM[did].period = atoi(tok);
                    // PWM Duty
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DevicePWM[did].duty = atoi(tok);

                    // ADC con name
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DevicePWM[did].cname, tok, strlen(tok));
                    // ADC max
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DevicePWM[did].max = atoi(tok);
                    // ADC min
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DevicePWM[did].min = atoi(tok);

                    // pwm config (export pwm_ch, peroid, duty)
                    pwm_config (&DevicePWM[did]);
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
