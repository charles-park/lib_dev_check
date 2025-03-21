//------------------------------------------------------------------------------
/**
 * @file ehternet.h
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
#ifndef __ETHERNET_H__
#define __ETHERNET_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Define the Device ID for the ETHERNET group.
//------------------------------------------------------------------------------
/* Ehternet default config */
#define eETHERNET_CFG   -1

enum {
    /* R = ip read, I = init value */
    eETHERNET_IP = 0,
    /* R = eth mac read, I = init value, W = eth mac write */
    eETHERNET_MAC,
    /* R = run iperf server (iperf -s -1), iperf read */
    eETHERNET_IPERF,
    /* S = eth 1G setting, C = eth 100M setting, I = init valuue, R = read link speed */
    eETHERNET_LINK,
    /* NLP Server IP (Port 8888 ~ )*/
    eETHERNET_SERVER,

    eETHERNET_END
};

//------------------------------------------------------------------------------
// function prototype
//------------------------------------------------------------------------------
extern char *get_board_ip       (void);
extern char *get_mac_addr       (void);
extern int  get_ethernet_iperf  (void);

extern int  ethernet_check      (int dev_id, char *resp);
extern void ethernet_grp_init   (char *cfg);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#endif  // #define __ETHERNET_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
