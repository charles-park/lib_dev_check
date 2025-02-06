//------------------------------------------------------------------------------
/**
 * @file ethernet.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-20
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
#include "../lib_mac/lib_mac.h"
#include "../lib_efuse/lib_efuse.h"
#include "ethernet.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define LINK_SPEED_1G       1000
#define LINK_SPEED_100M     100

/* iperf3_odroid 를 설치하여 사용함. socket통신을 통하여 iperf3 실행하도록 함. */
/* iperf3_odroid build 후 iperf3_odroid실행파일은 /usr/bin으로 복사하여 사용함. */
#if defined (__IPERF3_ODROID__)
    #define IPERF3_RUN_CMD      "iperf3_odroid -R -p 8000 -c"
#else
    // #define IPERF3_RUN_CMD      "iperf3 -t 1 -c"
    #define IPERF3_RUN_CMD      "iperf3 -t 1 -R -c"
#endif

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_ethernet {
    char board_ip_str [sizeof(struct sockaddr)+1];
    int  board_ip_int [4];   // aaa.bbb.ccc.ddd

    char server_ip_str [sizeof(struct sockaddr)+1];
    int  server_ip_int [4];   // aaa.bbb.ccc.ddd

    int  board_mac_validate;
    char board_mac_str[20];
    char board_mac[MAC_STR_SIZE +1];

    int iperf_speed;
    int iperf_check_speed;
};

#define DEFAULT_IPERF_SPEED     800
/*
    2024 New Server Port
    ODROID-C4  : 8888
    ODROID-M1  : 8889
    ODROID-M1S : 8890
    ODROID-M2  : 8891
*/
#define DEFAULT_SERVER_PORT     8888

#define EFUSE_BOARD_ID          eBOARD_ID_C5
#define EFUSE_MODEL_NAME        "c5"

//------------------------------------------------------------------------------
//
// Configuration
//
//------------------------------------------------------------------------------
struct device_ethernet DeviceETHERNET;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int net_status (char *ip_addr)
{
    char cmd_line[STR_PATH_LENGTH];
    FILE *fp;

    memset  (cmd_line, 0x00, sizeof(cmd_line));
    sprintf (cmd_line, "ping -c 1 -w 1 %s", ip_addr);

    if ((fp = popen(cmd_line, "r")) != NULL) {
        memset (cmd_line, 0x00, sizeof(cmd_line));
        while (fgets(cmd_line, STR_PATH_LENGTH, fp)) {
            if (NULL != strstr(cmd_line, "1 received")) {
                pclose(fp);
                printf ("%s : alive %s\n", __func__, ip_addr);
                return 1;
            }
        }
        pclose(fp);
    }
    printf ("%s : dead %s\n", __func__, ip_addr);
    return 0;
}

//------------------------------------------------------------------------------
static void ip_str_to_int (char *ip_str, int *ip_int)
{
    char *tok;

    tok = strtok(ip_str, ".");  if (tok != NULL)    ip_int [0] = atoi(tok);
    tok = strtok(NULL  , ".");  if (tok != NULL)    ip_int [1] = atoi(tok);
    tok = strtok(NULL  , ".");  if (tok != NULL)    ip_int [2] = atoi(tok);
    tok = strtok(NULL  , ".");  if (tok != NULL)    ip_int [3] = atoi(tok);
}

//------------------------------------------------------------------------------
static int ethernet_board_ip (void)
{
    int fd, retry_cnt = 100;
    struct ifreq ifr;
    char ip_addr[sizeof(struct sockaddr)+1];

retry:
    usleep (100 * 1000);    // 500ms delay
    /* this entire function is almost copied from ethtool source code */
    /* Open control socket. */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        fprintf (stdout, "Cannot get control socket\n");
        if (retry_cnt--)    goto retry;
        return 0;
    }
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        fprintf (stdout, "SIOCGIFADDR ioctl Error!!\n");
        close(fd);
        if (retry_cnt--)    goto retry;
        return 0;
    }
    memset (ip_addr, 0x00, sizeof(ip_addr));
    inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, ip_addr, sizeof(struct sockaddr));
    printf ("%s : ip_address = %s\n", __func__, ip_addr);

    ip_str_to_int (ip_addr, DeviceETHERNET.board_ip_int);
    memset  (DeviceETHERNET.board_ip_str, 0, sizeof(DeviceETHERNET.board_ip_str));
    sprintf (DeviceETHERNET.board_ip_str, "%d.%d.%d.%d",
                DeviceETHERNET.board_ip_int[0],
                DeviceETHERNET.board_ip_int[1],
                DeviceETHERNET.board_ip_int[2],
                DeviceETHERNET.board_ip_int[3]);
    return 1;
}

//------------------------------------------------------------------------------
char *get_board_ip (void)
{
    if (ethernet_board_ip())
        return DeviceETHERNET.board_ip_str;

    return NULL;
}

char *get_mac_addr (void)
{
    if (DeviceETHERNET.board_mac_validate)
        return DeviceETHERNET.board_mac_str;

    return NULL;
}

int get_ethernet_iperf (void)
{
    return DeviceETHERNET.iperf_speed;
}

//------------------------------------------------------------------------------
#if 0
static int ethernet_board_ip_file (void)
{
    FILE *fp;
    char ip_addr[sizeof(struct sockaddr)+1];

    if ((fp = popen("hostname -I", "r")) != NULL) {
        memset (ip_addr, 0x00, sizeof(ip_addr));
        if (fgets(ip_addr, sizeof(struct sockaddr), fp) != NULL) {
            printf ("%s : IP Address = %s\n", __func__, ip_addr);
            ip_str_to_int (ip_addr, DeviceETHERNET.board_ip_int);
            pclose(fp);
            memset  (DeviceETHERNET.board_ip_str, 0, sizeof(DeviceETHERNET.board_ip_str));
            sprintf(DeviceETHERNET.board_ip_str, "%d.%d.%d.%d",
                DeviceETHERNET.board_ip_int[0],
                DeviceETHERNET.board_ip_int[1],
                DeviceETHERNET.board_ip_int[2],
                DeviceETHERNET.board_ip_int[3]);
            return 1;
        }
        pclose(fp);
    }
    return 0;
}
#endif
//------------------------------------------------------------------------------
static int ethernet_server_ip (void)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH], *ip_tok;

    if (DeviceETHERNET.server_ip_int[0] != 0)   return 1;

    memset(cmd_line, 0, sizeof(cmd_line));
    sprintf(cmd_line, "nmap %d.%d.%d.* -p T:%4d --open 2<&1",
        DeviceETHERNET.board_ip_int[0],
        DeviceETHERNET.board_ip_int[1],
        DeviceETHERNET.board_ip_int[2],
        DEFAULT_SERVER_PORT);

    if ((fp = popen(cmd_line, "r")) != NULL) {
        memset(cmd_line, 0, sizeof(cmd_line));
        while (fgets(cmd_line, sizeof(cmd_line), fp)) {
            char ip[10];
            memset  (ip, 0, sizeof(ip));
            sprintf (ip, "%d.%d.", DeviceETHERNET.board_ip_int[0], DeviceETHERNET.board_ip_int[1]);
            ip_tok = strstr(cmd_line, ip);
            if (ip_tok != NULL) {
                printf ("%s : %s\n", __func__, ip_tok);
                if (net_status (ip_tok)) {
                    ip_str_to_int (ip_tok, DeviceETHERNET.server_ip_int);

                    // ip_addr arrary size = 20 bytes
                    memset (DeviceETHERNET.server_ip_str, 0, sizeof (DeviceETHERNET.server_ip_str));
                    sprintf(DeviceETHERNET.server_ip_str, "%d.%d.%d.%d",
                                                DeviceETHERNET.server_ip_int[0],
                                                DeviceETHERNET.server_ip_int[1],
                                                DeviceETHERNET.server_ip_int[2],
                                                DeviceETHERNET.server_ip_int[3]);
                    pclose(fp);
                    return 1;
                }
            }
        }
    }
    pclose(fp);
    return 0;
}

//------------------------------------------------------------------------------
static int ethernet_mac_write (const char *model)
{
    char efuse [EFUSE_UUID_SIZE];

    memset (efuse, 0, sizeof (efuse));

    if (mac_server_request (MAC_SERVER_FACTORY, REQ_TYPE_UUID, model, efuse)) {
        if (efuse_control (efuse, EFUSE_WRITE)) {
            memset (efuse, 0, sizeof(efuse));
            if (efuse_control (efuse, EFUSE_READ)) {
                if (efuse_valid_check (efuse)) {
                    memset (DeviceETHERNET.board_mac, 0, sizeof(DeviceETHERNET.board_mac));
                    efuse_get_mac (efuse, DeviceETHERNET.board_mac);
                    return 1;
                }
            }
        }
    }
    efuse_control (efuse, EFUSE_ERASE);
    return 0;
}
//------------------------------------------------------------------------------
static void ethernet_efuse_check (void)
{
    char efuse [EFUSE_UUID_SIZE];

    memset (efuse, 0, sizeof (efuse));

    efuse_set_board (EFUSE_BOARD_ID);

    // mac status & value
    if (efuse_control (efuse, EFUSE_READ)) {
        DeviceETHERNET.board_mac_validate = efuse_valid_check (efuse);
        if (!DeviceETHERNET.board_mac_validate) {
            if (ethernet_mac_write (EFUSE_MODEL_NAME)) {
                memset (efuse, 0, sizeof (efuse));
                efuse_control (efuse, EFUSE_READ);
                DeviceETHERNET.board_mac_validate = efuse_valid_check (efuse);
            }
        }

        efuse_get_mac (efuse, DeviceETHERNET.board_mac);
        sprintf (DeviceETHERNET.board_mac_str, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
            DeviceETHERNET.board_mac[0], DeviceETHERNET.board_mac[1],
            DeviceETHERNET.board_mac[2], DeviceETHERNET.board_mac[3],
            DeviceETHERNET.board_mac[4], DeviceETHERNET.board_mac[5],
            DeviceETHERNET.board_mac[6], DeviceETHERNET.board_mac[7],
            DeviceETHERNET.board_mac[8], DeviceETHERNET.board_mac[9],
            DeviceETHERNET.board_mac[10], DeviceETHERNET.board_mac[11]);

        if (DeviceETHERNET.board_mac_validate)
            printf ("%s : mac address = %s\n", __func__, DeviceETHERNET.board_mac_str);
        else
            printf ("%s : ethernet mac write error! (%s)\n", __func__, EFUSE_MODEL_NAME);
    }
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
static int ethernet_link_check (char *resp)
{
    int status = 0, link_speed = 0;

    link_speed = ethernet_link_speed ();

    status = (link_speed == LINK_SPEED_1G)  ? 1 : -1;

    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', link_speed);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
static int ethernet_ip_check (char *resp)
{
    int status = 0;

    if (!DeviceETHERNET.board_ip_int[0])
        ethernet_board_ip ();

    status = DeviceETHERNET.board_ip_int[0] ? 1 : -1;

    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'P' : 'F', DeviceETHERNET.board_ip_str);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
static int ethernet_mac_check (char *resp)
{
    int status = 0;

    if (!DeviceETHERNET.board_mac_validate)
        ethernet_efuse_check ();

    status = DeviceETHERNET.board_mac_validate ? 1 : -1;

    DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'P' : 'F', DeviceETHERNET.board_mac_str);
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
pthread_t thread_iperf3;
volatile int ThreadRunning = 0;

static void *thread_iperf3_func (void *arg)
{
    FILE *fp;
    char cmd_line [STR_PATH_LENGTH], *pstr = NULL;

    printf ("%s : thread running!\n", __func__);
    ThreadRunning = 1;
    memset (cmd_line, 0, sizeof(cmd_line));
    if ((fp = popen("iperf3 -s -1", "r")) != NULL) {
        while (fgets(cmd_line, sizeof(cmd_line), fp)) {
            if (strstr (cmd_line, "receiver") != NULL) {
                if ((pstr = strstr (cmd_line, "MBytes")) != NULL) {
                    while (*pstr != ' ')    pstr++;
                    DeviceETHERNET.iperf_speed = atoi (pstr);
                    break;
                }
            }
            memset (cmd_line, 0, sizeof(cmd_line));
        }
        pclose(fp);
        printf ("%s : popen stop = %d\n", __func__, DeviceETHERNET.iperf_speed);
    }
    ThreadRunning = 0;
    printf ("%s : thread stop! iperf_speed = %d\n", __func__, DeviceETHERNET.iperf_speed);
    return arg;
}

//------------------------------------------------------------------------------
static int ethernet_iperf_check (char *resp)
{
    int status = 0;

    if (DeviceETHERNET.board_ip_int[0] != 0) {
        if (!ThreadRunning && !DeviceETHERNET.iperf_speed)
            pthread_create (&thread_iperf3, NULL, thread_iperf3_func, (void *)&DeviceETHERNET);

        if (DeviceETHERNET.iperf_speed)
            status = (DeviceETHERNET.iperf_speed > DeviceETHERNET.iperf_check_speed) ? 1 : -1;
    }

    if (status) DEVICE_RESP_FORM_INT(resp, 'P', DeviceETHERNET.iperf_speed);
    else        DEVICE_RESP_FORM_STR(resp, 'C', DeviceETHERNET.board_ip_str);

    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
static int ethernet_server_check (char *resp)
{
    int status = 0;

    if (DeviceETHERNET.board_ip_int[0]) {
        ethernet_server_ip   ();

        if (DeviceETHERNET.server_ip_int[0] != 0)   status = 1;

        DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'P' : 'F', DeviceETHERNET.server_ip_str);
    }

    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int ethernet_check (int dev_id, char *resp)
{
    int value = 0, status = 0, id = DEVICE_ID(dev_id);

    switch (id) {
        case eETHERNET_IP:      return ethernet_ip_check    (resp);
        case eETHERNET_MAC:     return ethernet_mac_check   (resp);
        case eETHERNET_IPERF:   return ethernet_iperf_check (resp);
        case eETHERNET_LINK:    return ethernet_link_check  (resp);
        case eETHERNET_SERVER:  return ethernet_server_check(resp);
        default:
            break;
    }
    DEVICE_RESP_FORM_INT (resp, (status == 1) ? 'P' : 'F', value);
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
    fputs   ("# info : iperf check speed \n", fp);
    memset  (value, 0, sizeof(value));
    sprintf (value, "%d,\n", DEFAULT_IPERF_SPEED);
    fputs   (value, fp);
    fclose  (fp);
}

//------------------------------------------------------------------------------
static void default_config_read (void)
{
    FILE *fp;
    char fname [STR_PATH_LENGTH +1], value [STR_PATH_LENGTH +1], *ptr;

    memset  (fname, 0, STR_PATH_LENGTH);
    sprintf (fname, "%sjig-%s.cfg", CONFIG_FILE_PATH, "ethernet");

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
                // fputs   ("# info : iperf check speed \n", fp);
                if ((ptr = strtok ( value, ",")) != NULL) {
                    DeviceETHERNET.iperf_check_speed = atoi (ptr);
                }
                break;
        }
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
int ethernet_grp_init (void)
{
    memset (&DeviceETHERNET, 0, sizeof(DeviceETHERNET));

    default_config_read ();
    if (ethernet_link_speed() != LINK_SPEED_1G) {
        ethernet_link_setup (LINK_SPEED_1G);
        sleep (1);
    }

    if (ethernet_board_ip ()) {
        ethernet_efuse_check ();
        // iperf server, nlp server
        // ethernet_server_ip   ();
    }
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
