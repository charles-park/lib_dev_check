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
    const char *path;
    // set str
    const char *set;
    // clear str
    const char *clr;
};

struct device_led_adc {
    // adc value
    int max, min;
    // adc pin
    char pin[STR_NAME_LENGTH];
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
    { "none", "0", "255" },
    // eLED_ALIVE
    { "/sys/class/leds/blue:heartbeat/brightness" , "255", "0" },
};

const char *ALIVE_TRIGGER = "/sys/class/leds/blue:heartbeat/trigger";

struct device_led_adc DeviceLED_ADC[eLED_END] = {
    { 600, 400, "P1_6.3" }, // eLED_POWER
    { 600, 500, "P1_6.4" }, // eLED_ALIVE
    { 300, 100, "P1_6.5" }, // eLED_100M
    { 300, 100, "P1_6.6" }, // eLED_1G
};

//------------------------------------------------------------------------------
#define LED_LINK_100M    100
#define LED_LINK_1G      1000

//------------------------------------------------------------------------------
static int ethernet_link_speed (void)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH];

    if (access ("/sys/class/net/eth0/speed", F_OK) != 0)
        return 0;

    memset (cmd_line, 0x00, sizeof(cmd_line));
    if ((fp = fopen ("/sys/class/net/eth0/speed", "r")) != NULL) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        if (NULL != fgets (cmd_line, sizeof(cmd_line), fp)) {
            fclose (fp);
            return atoi (cmd_line);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_link_setup (int speed)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH], retry = 10;

    if (ethernet_link_speed () != speed) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        sprintf(cmd_line,"ethtool -s eth0 speed %d duplex full 2>&1 && sync ", speed);
        if ((fp = popen(cmd_line, "w")) != NULL)
            pclose(fp);

        // timeout 10 sec
        while (retry--) {
            sleep (1);
            if (ethernet_link_speed() == speed)
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
        case eLED_100M:  case eLED_1G:
            if (DEVICE_ACTION(dev_id))
                status = (resp_i > DeviceLED_ADC[id].max) ? 1 : 0;    // led on
            else
                status = (resp_i < DeviceLED_ADC[id].min) ? 1 : 0;    // led off
            break;
        default :
            status = 0;
            break;
    }
    return status;
}

//------------------------------------------------------------------------------
int led_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eLED_POWER: case eLED_ALIVE:
            if (!strncmp (DeviceLED[id].path, "none", strlen("none"))) {
                status = 1;
                value  = DEVICE_ACTION(dev_id);
            } else {
                if (DEVICE_ACTION(dev_id) == 1)
                    value = led_write (DeviceLED[id].path, DeviceLED[id].set);
                else
                    value = led_write (DeviceLED[id].path, DeviceLED[id].clr);

                status = (value == led_read (DeviceLED[id].path)) ? 1 : -1;
            }
            break;

        case eLED_100M: case eLED_1G:
            // swap id (if led off)
            if (DEVICE_ACTION(dev_id) == 0)
                status = ethernet_link_setup ( (id == eLED_100M) ?
                                        LED_LINK_1G:LED_LINK_100M ) ? 1 : -1;
            else
                status = ethernet_link_setup ( (id == eLED_100M) ?
                                        LED_LINK_100M:LED_LINK_1G ) ? 1 : -1;
            break;
        default :
            break;
    }

    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', DeviceLED_ADC[id].pin);

    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
static void default_config_write (const char *fname)
{
    FILE *fp;
    char value [STR_PATH_LENGTH *2 +1];

    if ((fp = fopen(fname, "wt")) == NULL)
        return;

    // default value write
    fputs   ("# info : dev_id, adc_max, adc_min, adc_pin \n", fp);
    {
        int id;
        for (id = 0; id < eLED_END; id++) {
            memset  (value, 0, STR_PATH_LENGTH *2);
            sprintf (value, "%d,%d,%d,%s,\n",
                id, DeviceLED_ADC[id].max, DeviceLED_ADC[id].min, DeviceLED_ADC[id].pin);
            fputs   (value, fp);

        }
    }
    // file close
    fclose  (fp);
}

//------------------------------------------------------------------------------
static void default_config_read (void)
{
    FILE *fp;
    char fname [STR_PATH_LENGTH +1], value [STR_PATH_LENGTH +1], *ptr;
    int dev_id;

    memset  (fname, 0, STR_PATH_LENGTH);
    sprintf (fname, "%sjig-%s.cfg", CONFIG_FILE_PATH, "led");

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
                // default value write
                // fputs   ("# info : dev_id, dev_node, rd_speed, wr_speed \n", fp);
                if ((ptr = strtok (value, ",")) != NULL) {
                    dev_id = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL)
                       DeviceLED_ADC[dev_id].max = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL)
                       DeviceLED_ADC[dev_id].min = atoi (ptr);
                    if ((ptr = strtok ( NULL, ",")) != NULL) {
                        memset (DeviceLED_ADC[dev_id].pin, 0,
                                sizeof(DeviceLED_ADC[dev_id].pin));
                        strcpy (DeviceLED_ADC[dev_id].pin, ptr);
                    }
                }
                break;
        }
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
int led_grp_init (void)
{
    default_config_read ();
    // Alive LED Trigger setting
    if (access (ALIVE_TRIGGER, F_OK) == 0)
        led_write (ALIVE_TRIGGER, "none");

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
