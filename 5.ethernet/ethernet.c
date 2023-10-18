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
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/fb.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

//------------------------------------------------------------------------------
#include "../lib_dev_check.h"
#include "lib_efuse/lib_efuse.h"
#include "lib_efuse/lib_efuse.h"
#include "ethernet.h"

//------------------------------------------------------------------------------
// Client에서 iperf3 서버 모드로 1번만 실행함.
// Server 명령이 iperf3 -c {ip client} -t 1 -P 1 인 경우 strstr "receiver" 검색
// Server 명령이 iperf3 -c {ip client} -t 1 -P 1 -R 인 경우 strstr "sender" 검색
// 결과값 중 Mbits/sec or Gbits/sec를 찾아 속도를 구함.
//------------------------------------------------------------------------------
const char *odroid_mac_prefix = "001E06";
const char *iperf_run_cmd = "iperf3 -s -1";

// stdlib.h
// strtoul (string, endp, base(10, 16,...))

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_ethernet {
    // ip value ddd of aaa.bbb.ccc.ddd
    int speed;
    int ip_lsb;
    char mac_status;
    char mac_addr[MAC_STR_SIZE];
};

struct device_ethernet DeviceETHERNET;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int get_eth0_ip (void)
{
    int fd;
    struct ifreq ifr;
    char if_info[20], *p_str;

    /* this entire function is almost copied from ethtool source code */
    /* Open control socket. */
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf ("%s : Cannot get control socket\n", __func__);
        return 0;
    }
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    if (ioctl (fd, SIOCGIFADDR, &ifr) < 0) {
        printf ("iface name = eth0, SIOCGIFADDR ioctl Error!!\n");
        close (fd);
        return 0;
    }
    // board(iface) ip
    memset (if_info, 0, sizeof(if_info));
    inet_ntop (AF_INET, ifr.ifr_addr.sa_data+2, if_info, sizeof(struct sockaddr));

    if ((p_str = strtok (if_info, ".")) != NULL) {
        strtok (NULL, "."); strtok (NULL, ".");

        if ((p_str = strtok (NULL, ".")) != NULL)
            return atoi (p_str);
    }
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_ip_check (char action, char *resp)
{
    int value = 0;

    /* R = ip read, I = init value */
    switch (action) {
        case 'R':
            value = get_eth0_ip ();
            break;
        case 'I':
            value = DeviceETHERNET.ip_lsb;
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", value);
    return value ? 1 : 0;
}

//------------------------------------------------------------------------------
static int ethernet_mac_check (char action, char *resp)
{
    int value = 0;
    char efuse [EFUSE_SIZE_M1S];

    /* R = eth mac read, I = init value, W = eth mac write */
    switch (action) {
        case 'I':
            if (DeviceETHERNET.mac_status) {
                strncpy (resp, &DeviceETHERNET.mac_addr[6], 6);
                value = DeviceETHERNET.mac_status;
            }
            break;
        case 'R':
            efuse_control (efuse, EFUSE_READ);
            DeviceETHERNET.mac_status = efuse_valid_check (efuse);
            if (DeviceETHERNET.mac_status) {
                efuse_get_mac (efuse, DeviceETHERNET.mac_addr);
                strncpy (resp, &DeviceETHERNET.mac_addr[6], 6);
                value = DeviceETHERNET.mac_status;
            }
            break;
        case 'W':
            memset  ( DeviceETHERNET.mac_addr, 0, MAC_STR_SIZE);
            strncpy ( DeviceETHERNET.mac_addr, odroid_mac_prefix, 6);
//            strncpy (&DeviceETHERNET.mac_addr[6], extra, 6);
            // extra data write
//            if (efuse_control(e))
            break;
        default :
            break;
    }
    if (!value) sprintf (resp, "%06d", 0);
    return value;
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
    char efuse [EFUSE_SIZE_M1S];

    memset (efuse, 0, sizeof (efuse));
    memset (&DeviceETHERNET, 0, sizeof(DeviceETHERNET));

    // get Board lsb ip address value
    DeviceETHERNET.ip_lsb = get_eth0_ip ();

    // mac status & value
    efuse_control (efuse, EFUSE_READ);
    DeviceETHERNET.mac_status = efuse_valid_check (efuse);
    if (DeviceETHERNET.mac_status)
        efuse_get_mac (efuse, DeviceETHERNET.mac_addr);

    // link speed
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
