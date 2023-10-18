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
#include "ethernet.h"

//------------------------------------------------------------------------------
// Client에서 iperf3 서버 모드로 1번만 실행함.
// Server 명령이 iperf3 -c {ip client} -t 1 -P 1 인 경우 strstr "receiver" 검색
// Server 명령이 iperf3 -c {ip client} -t 1 -P 1 -R 인 경우 strstr "sender" 검색
// 결과값 중 Mbits/sec or Gbits/sec를 찾아 속도를 구함.
//------------------------------------------------------------------------------
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
    char ip[20];
    char mac[20];
};

struct device_ethernet DeviceETHERNET;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int get_iface_info (struct device_ethernet *info, const char *if_name)
{
    int fd;
    struct ifreq ifr;
    char if_info[20];

    memset (info, 0, sizeof(struct device_ethernet));

    /* this entire function is almost copied from ethtool source code */
    /* Open control socket. */
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf ("%s : Cannot get control socket\n", __func__);
        return 0;
    }
    strncpy(ifr.ifr_name, (if_name != NULL) ? if_name : "eth0", IFNAMSIZ);
    if (ioctl (fd, SIOCGIFADDR, &ifr) < 0) {
printf ("%s : iface name = %s, SIOCGIFADDR ioctl Error!!\n", __func__, if_name);
        close (fd);
        return 0;
    }
    // board(iface) ip
    memset (if_info, 0, sizeof(if_info));
    inet_ntop (AF_INET, ifr.ifr_addr.sa_data+2, if_info, sizeof(struct sockaddr));
    strncpy (info->ip, if_info, strlen(if_info));

    // iface mac
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
        memset (if_info, 0, sizeof(if_info));
        memcpy (if_info, ifr.ifr_hwaddr.sa_data, 6);
        sprintf(info->mac, "%02x%02x%02x%02x%02x%02x",
            if_info[0], if_info[1], if_info[2], if_info[3], if_info[4], if_info[5]);
    }
printf ("Interface info : iface = %s, ip = %s, mac = %s\n",
        (if_name != NULL) ? if_name : "eth0",
        info->ip, info->mac);

    return 1;
}

//------------------------------------------------------------------------------
static int ethernet_ip_check (char action, char *resp)
{
    int value = 0;
    /* R = ip read, I = init value */
    switch (action) {
        case 'R':
            value = get_iface_info (&DeviceETHERNET, "eth0");
            break;
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
