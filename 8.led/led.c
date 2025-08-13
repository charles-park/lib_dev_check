//------------------------------------------------------------------------------
/**
 * @file led.c
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
#include <sys/sysinfo.h>

//------------------------------------------------------------------------------
#include "../lib_dev_check.h"
#include "led.h"

//------------------------------------------------------------------------------
struct device_led {
    // Control path
    char path [STR_PATH_LENGTH];
    // str set/clr
    int on_value, off_value;
    // ADC con_name
    char cname[STR_NAME_LENGTH];
    // adc value
    int max, min;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define led devices */
//------------------------------------------------------------------------------
struct device_led DeviceLED [eLED_END] = {
    // eLED_POWER
    { { 0, }, 0, 0, { 0, }, 0, 0},
    // eLED_ALIVE
    { { 0, }, 0, 0, { 0, }, 0, 0},
    // eLED_100M
    { { 0, }, 0, 0, { 0, }, 0, 0},
    // eLED_1G
    { { 0, }, 0, 0, { 0, }, 0, 0},
    // eLED_NVME
    { { 0, }, 0, 0, { 0, }, 0, 0},
};

//------------------------------------------------------------------------------
static int ethernet_link_speed (int id)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH];

    if (access (DeviceLED[id].path, F_OK) != 0)
        return 0;

    memset (cmd_line, 0x00, sizeof(cmd_line));
    if ((fp = fopen (DeviceLED[id].path, "r")) != NULL) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        if (NULL != fgets (cmd_line, sizeof(cmd_line), fp)) {
            fclose (fp);
            return atoi (cmd_line);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_link_setup (int dev_id, int speed)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH], retry = 10;

    if (ethernet_link_speed (DEVICE_ID(dev_id)) != speed) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        sprintf(cmd_line,"ethtool -s eth0 speed %d duplex full 2>&1 && sync ", speed);
        if ((fp = popen(cmd_line, "r")) != NULL)
            pclose(fp);

        // timeout 10 sec
        while (retry--) {
            sleep (1);
            if (ethernet_link_speed(DEVICE_ID(dev_id)) == speed)
                return 1;
        }
        return 0;
    }
    return 1;
}
//------------------------------------------------------------------------------
static int led_read (const char *path)
{
    char rdata[16];
    FILE *fp;

    memset (rdata, 0, sizeof (rdata));

    // led value get
    if ((fp = fopen(path, "r")) != NULL) {
        fread (rdata, 1, sizeof(rdata), fp);
        fclose(fp);
    }

    return atoi(rdata);
}

//------------------------------------------------------------------------------
static int led_write (const char *path, const char *wdata)
{
    FILE *fp;

    // led value set
    if ((fp = fopen(path, "w")) != NULL) {
        fwrite (wdata, 1, strlen(wdata), fp);
        fclose(fp);
    }

    return atoi(wdata);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int led_data_check (int dev_id, int resp_i)
{
    int status = 0, id = DEVICE_ID(dev_id);
    switch (id) {
        case eLED_ALIVE: case eLED_POWER:
        case eLED_100M:  case eLED_1G:  case eLED_NVME:
            if (DEVICE_ACTION(dev_id))
                status = (resp_i > DeviceLED[id].max) ? 1 : 0;    // led on
            else
                status = (resp_i < DeviceLED[id].min) ? 1 : 0;    // led off
            break;
        default :
            status = 0;
            break;
    }
    return status;
}

//------------------------------------------------------------------------------
pthread_t thread_led;
static volatile int ThreadRunning = 0;
const char *NVME_READ_CHECK = "dd of=/dev/null bs=16M count=%d iflag=nocache,dsync oflag=nocache,dsync if=%s 2>&1 && sync";

void *thread_func_led (void *arg)
{
    struct device_led *p_led = (struct device_led *)arg;
    char cmd [STR_PATH_LENGTH*2];
    FILE *fp;

    memset  (cmd, 0, sizeof(cmd));

    sprintf (cmd, NVME_READ_CHECK, p_led->on_value, p_led->path);

    ThreadRunning = 1;

    if ((fp = popen (cmd, "r")) != NULL)    pclose(fp);

    ThreadRunning = 0;

    return arg;
}

int led_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eLED_POWER: case eLED_ALIVE:
            if (!strncmp (DeviceLED[id].path, "none", strlen("none"))) {
                status = 1;
                value  = DEVICE_ACTION(dev_id);
            } else {
                char w_value[STR_NAME_LENGTH];

                memset (w_value, 0, sizeof(w_value));

                if (DEVICE_ACTION(dev_id) == 1) sprintf (w_value, "%d", DeviceLED[id].on_value);
                else                            sprintf (w_value, "%d", DeviceLED[id].off_value);

                value  = led_write (DeviceLED[id].path, w_value);
                status = (value == led_read (DeviceLED[id].path)) ? 1 : -1;
            }
            break;

        case eLED_100M: case eLED_1G:
            value  = DEVICE_ACTION(dev_id) ? DeviceLED[id].on_value : DeviceLED[id].off_value;
            status = ethernet_link_setup (dev_id, value) ? 1 : -1;
            break;

        case eLED_NVME:
            while (ThreadRunning)   sleep (1);
            if (access (DeviceLED[id].path, F_OK) == 0) {
                if (DEVICE_ACTION(dev_id) == 1) pthread_create (&thread_led, NULL, thread_func_led, &DeviceLED[id]);
                status =  1;
            }
            else
                status =  -1;
            break;

        default :
            break;
    }

    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', DeviceLED[id].cname);

    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void led_grp_init (char *cfg)
{
    char *tok;
    int did;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            switch (did) {
                case eLED_POWER: case eLED_ALIVE:
                case eLED_100M: case eLED_1G: case eLED_NVME:
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceLED[did].path, tok, strlen(tok));

                    // led on value
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceLED[did].on_value = atoi(tok);

                    // led off value
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceLED[did].off_value = atoi(tok);

                    // ADC port name
                    if ((tok = strtok (NULL, ",")) != NULL)
                        strncpy (DeviceLED[did].cname, tok, strlen(tok));

                    // led on ADC value
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceLED[did].max = atoi(tok);

                    // led off ADC value
                    if ((tok = strtok (NULL, ",")) != NULL)
                        DeviceLED[did].min = atoi(tok);

                    break;
                case eLED_CFG:
                    if ((tok = strtok (NULL, ",")) != NULL) {
                        char ctl_path [STR_PATH_LENGTH], ctl_str [STR_NAME_LENGTH];

                        memset (ctl_path, 0, sizeof(ctl_path));
                        memset (ctl_str,  0, sizeof(ctl_str));

                        switch (atoi(tok)) {
                            case eLED_POWER: case eLED_ALIVE:
                                if ((tok = strtok (NULL, ",")) != NULL)
                                    strncpy (ctl_path, tok, strlen(tok));
                                if ((tok = strtok (NULL, ",")) != NULL)
                                    strncpy (ctl_str, tok, strlen(tok));

                                led_write (ctl_path, ctl_str);
                                break;
                            default:
                                break;
                        }
                    }
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
