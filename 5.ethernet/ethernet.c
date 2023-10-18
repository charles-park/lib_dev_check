//------------------------------------------------------------------------------
/**
 * @file ethernet.c
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
#include "ethernet.h"

//------------------------------------------------------------------------------
struct device_ethernet {
    // Control path
    const char *path;
    // set
    const char *set;
    // clear
    const char *clr;
};

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
/* define pwm devices (ODROID-N2L) */
//------------------------------------------------------------------------------
struct device_ethernet DeviceETHERNET [eETHERNET_END] = {
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int ethernet_ip_check (char action, char *resp)
{
    /* R = ip read, I = init value */
    switch (action) {
        case 'R':
        case 'I':
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", 0);
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_mac_check (char action, char *resp)
{
    /* R = eth mac read, I = init value, W = eth mac write */
    switch (action) {
        case 'R':
        case 'I':
        case 'W':
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", 0);
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_iperf_check (char action, char *resp)
{
    /* R = run iperf server (iperf -s -1), iperf read */
    switch (action) {
        case 'R':
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", 0);
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_link_check (char action, char *resp)
{
    /* S = eth 1G setting, C = eth 100M setting, I = init valuue, R = read link speed */
    switch (action) {
        case 'I':
        case 'R':
        case 'S':
        case 'C':
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", 0);
    return 0;
}

//------------------------------------------------------------------------------
int ethernet_check (int id, char action, char *resp)
{
    switch (id) {
        case eETHERNET_IP:      return ethernet_ip_check    (action, resp);
        case eETHERNET_MAC:     return ethernet_mac_check   (action, resp);
        case eETHERNET_IPERF:   return ethernet_iperf_check (action, resp);
        case eETHERNET_LINK:    return ethernet_link_check  (action, resp);
        default:
            break;
    }
    sprintf (resp, "%06d", 0);
    return 0;
}

//------------------------------------------------------------------------------
int ethernet_grp_init (void)
{
    // ip
    // mac status & value
    // link speed
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
