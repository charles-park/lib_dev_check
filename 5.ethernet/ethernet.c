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
#include "lib_mac/lib_mac.h"
#include "lib_efuse/lib_efuse.h"
#include "ethernet.h"

//------------------------------------------------------------------------------
//
// iperf3 client setup (고정된 server ip 필요), /boot/iperf_server_ip.txt
//
//------------------------------------------------------------------------------
#define SERVER_IP_ADDR      "192.168.20.45" // default server ip (charles pc)

char Iperf3ServerIP [20] = {0, };

//------------------------------------------------------------------------------
#define IPERF3_RUN_CMD      "iperf3 -t 1 -c"
#define IPERF_SPEED_MIN     800

//------------------------------------------------------------------------------
#define LINK_SPEED_1G       1000
#define LINK_SPEED_100M     100

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_ethernet {
    // ethernet link speed
    int speed;
    // ip value ddd of aaa.bbb.ccc.ddd
    int ip_lsb;
    // mac data validate
    char mac_status;
    // mac str (aabbccddeeff)
    char mac_str[MAC_STR_SIZE];
    // ip str (aaa.bbb.ccc.ddd)
    char ip_str [sizeof(struct sockaddr)];
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

    /* aaa.bbb.ccc.ddd 형태로 저장됨 (16 bytes) */
    memcpy (DeviceETHERNET.ip_str, if_info, strlen(if_info));

    if ((p_str = strtok (if_info, ".")) != NULL) {
        strtok (NULL, "."); strtok (NULL, ".");

        if ((p_str = strtok (NULL, ".")) != NULL)
            return atoi (p_str);
    }
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_iperf (const char *found_str)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH], *pstr;
    int value = 0;

    memset (cmd_line, 0x00, sizeof(cmd_line));
    sprintf(cmd_line, "%s %s", IPERF3_RUN_CMD, Iperf3ServerIP);

    if ((fp = popen(cmd_line, "r")) != NULL) {
        while (1) {
            memset (cmd_line, 0, sizeof (cmd_line));
            if (fgets (cmd_line, sizeof (cmd_line), fp) == NULL)
                break;

            if (strstr (cmd_line, found_str) != NULL) {
                if ((pstr = strstr (cmd_line, "MBytes")) != NULL) {
                    while (*pstr != ' ')    pstr++;
                    value = atoi (pstr);
                }
            }
        }
        pclose(fp);
    }
    return value;
}

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
        fclose (fp);
    }
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_link_setup (int speed)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH], retry = 10;

    memset (cmd_line, 0x00, sizeof(cmd_line));
    sprintf(cmd_line,"ethtool -s eth0 speed %d duplex full", speed);
    if ((fp = popen(cmd_line, "w")) != NULL)
        pclose(fp);

    // timeout 10 sec
    while (retry--) {
        if (ethernet_link_speed() == speed)
            return 1;
        sleep (1);
    }
    return 0;
}

//------------------------------------------------------------------------------
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
    char efuse [EFUSE_SIZE_M1S];

    memset (efuse, 0, sizeof (efuse));

    /* R = eth mac read, I = init value, W = eth mac write */
    switch (action) {
        case 'I':   case 'R':   case 'W':
            if ((action == 'W') && !DeviceETHERNET.mac_status) {
                if (mac_server_request (MAC_SERVER_FACTORY, REQ_TYPE_UUID, "m1s", efuse)) {
                    if (efuse_control (efuse, EFUSE_WRITE)) {
                        memset (efuse, 0, sizeof(efuse));
                        if (efuse_control (efuse, EFUSE_READ)) {
                            DeviceETHERNET.mac_status = efuse_valid_check (efuse);
                            if (DeviceETHERNET.mac_status) {
                                memset (DeviceETHERNET.mac_str, 0, MAC_STR_SIZE);
                                efuse_get_mac (efuse, DeviceETHERNET.mac_str);
                            }
                            else
                                efuse_control (efuse, EFUSE_ERASE);
                        }
                    }
                }
            }
            /* 001E06aabbcc 형태로 저장이며, 앞의 6바이트는 고정이므로 하위 6바이트만 전송함. */
            if (DeviceETHERNET.mac_status) {
                strncpy (resp, &DeviceETHERNET.mac_str[6], 6);
                return 1;
            }
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
    int value = 0;

    if (ethernet_link_speed() != LINK_SPEED_1G)
        ethernet_link_setup (LINK_SPEED_1G);

    /* R = sender speed, W = receiver speed, iperf read */
    switch (action) {
        case 'W':
            if ((value = ethernet_iperf ("sender"))) {
                sprintf (resp, "%06d", value);
                return value > IPERF_SPEED_MIN ? 1 : 0;
            }
            break;
        case 'R':
            if ((value = ethernet_iperf ("receiver"))) {
                sprintf (resp, "%06d", value);
                return value > IPERF_SPEED_MIN ? 1 : 0;
            }
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
    int value = 0;
    /* S = eth 1G setting, C = eth 100M setting, I = init valuue, R = read link speed */
    switch (action) {
        case 'I':   case 'R':
            if (action == 'R')
                DeviceETHERNET.speed = ethernet_link_speed ();

            value = DeviceETHERNET.speed ? 1 : 0;
            break;
        case 'S':
            DeviceETHERNET.speed = ethernet_link_speed();
            if (DeviceETHERNET.speed != LINK_SPEED_1G) {
                DeviceETHERNET.speed  =
                    ethernet_link_setup (LINK_SPEED_1G);
            }
            value = (DeviceETHERNET.speed == LINK_SPEED_1G) ? 1 : 0;
            break;
        case 'C':
            DeviceETHERNET.speed = ethernet_link_speed();
            if (DeviceETHERNET.speed != LINK_SPEED_100M) {
                DeviceETHERNET.speed  =
                    ethernet_link_setup (LINK_SPEED_100M);
            }
            value = (DeviceETHERNET.speed == LINK_SPEED_100M) ? 1 : 0;
            break;
        default :
            break;
    }
    sprintf (resp, "%06d", DeviceETHERNET.speed);
    return value;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void set_server_ip (void)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH];

    memset (Iperf3ServerIP, 0, sizeof(Iperf3ServerIP));
    if (access ("/boot/iperf_server_ip.txt", F_OK) == 0) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        if ((fp = fopen ("/boot/iperf_server_ip.txt", "r")) != NULL) {
            memset (cmd_line, 0x00, sizeof(cmd_line));
            if (NULL != fgets (cmd_line, sizeof(cmd_line), fp)) {
                fclose (fp);
                strncpy (Iperf3ServerIP, cmd_line, strlen(cmd_line));
                return;
            }
        }
    }
    sprintf (Iperf3ServerIP, "%s", SERVER_IP_ADDR);
}

//------------------------------------------------------------------------------
// ip_str은 16바이트 할당되어야 함.
//------------------------------------------------------------------------------
void ehternet_ip_str (char *ip_str)
{
    memcpy (ip_str, DeviceETHERNET.ip_str, strlen (DeviceETHERNET.ip_str));
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

    // get Board lsb ip address int value
    DeviceETHERNET.ip_lsb = get_eth0_ip ();

    // mac status & value
    if (efuse_control (efuse, EFUSE_READ)) {
        DeviceETHERNET.mac_status = efuse_valid_check (efuse);
        if (DeviceETHERNET.mac_status)
            efuse_get_mac (efuse, DeviceETHERNET.mac_str);
    }
    // link speed
    DeviceETHERNET.speed = ethernet_link_speed();

    // find /boot/iperf_server_ip.txt for Iperf3 server
    set_server_ip ();
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
