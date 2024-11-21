//------------------------------------------------------------------------------
/**
 * @file led.c
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
        sprintf(cmd_line,"ethtool -s eth0 speed %d duplex full && sync", speed);
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
            status = ethernet_link_setup ( (id == eLED_100M) ? LED_LINK_100M:LED_LINK_1G ) ? 1 : -1;
            value  = (id == eLED_100M) ? LED_LINK_100M : LED_LINK_1G;
            break;
        default :
            break;
    }

    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
int led_grp_init (void)
{
    // Alive LED Trigger setting
    if (access (ALIVE_TRIGGER, F_OK) == 0)
        led_write (ALIVE_TRIGGER, "none");

    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
