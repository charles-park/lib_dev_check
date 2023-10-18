//------------------------------------------------------------------------------
/**
 * @file ehternet.h
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
#ifndef __ETHERNET_H__
#define __ETHERNET_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Define the Device ID for the ETHERNET group.
//------------------------------------------------------------------------------
enum {
    /* R = ip read, I = init value */
    eETHERNET_IP = 0,
    /* R = eth mac read, I = init value, W = eth mac write */
    eETHERNET_MAC,
    /* R = run iperf server (iperf -s -1), iperf read */
    eETHERNET_IPERF,
    /* S = eth 1G setting, C = eth 100M setting, I = init valuue, R = read link speed */
    eETHERNET_LINK,
    eETHERNET_END
};

//------------------------------------------------------------------------------
// function prototype
//------------------------------------------------------------------------------
extern int ethernet_check     (int id, char action, char *resp);
extern int ethernet_grp_init  (void);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#endif  // #define __ETHERNET_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
