//------------------------------------------------------------------------------
/**
 * @file header.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-25
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
#include "header.h"

//------------------------------------------------------------------------------
// GPIO num
int HEADER40[40 +1] = { -1, };
int HEADER14[14 +1] = { -1, };
int HEADER7 [ 7 +1] = { -1, };

// Check ADC Connecter name
char HEADER40_CON[16] = { 0, };
char HEADER14_CON[16] = { 0, };
char HEADER7_CON [16] = { 0, };;

//------------------------------------------------------------------------------
const int HEADER_PATTERN[4][2] = {
    { 0, 0 },
    { 0, 1 },
    { 1, 0 },
    { 1, 1 }
};

//------------------------------------------------------------------------------
static int pattern_write (int id, int pattern)
{
    int cnt = 0, err_cnt = 0, read = 0, nc_pin_cnt = 0;
    int gpio_cnt = 0, *gpio_list, *gpio_pattern, pattern_pos = 0;

    switch (id) {
        case eHEADER_7:
            gpio_cnt     = sizeof(HEADER7)/sizeof(HEADER7[0]);
            gpio_list    = (int *)HEADER7;
            break;
        case eHEADER_14:
            gpio_cnt     = sizeof(HEADER14)/sizeof(HEADER14[0]);
            gpio_list    = (int *)HEADER14;
            break;
        case eHEADER_40:
            gpio_cnt     = sizeof(HEADER40)/sizeof(HEADER40[0]);
            gpio_list    = (int *)HEADER40;
            break;
        default:
            printf ("%s : unknown header (id = %d)\n", __func__, id);
            return 0;
    }
    gpio_pattern = (int *)&HEADER_PATTERN[pattern % 4][0];

    for (cnt = 0; cnt < gpio_cnt; cnt++) {
        if (gpio_list[cnt] != NC) {
            gpio_set_value (gpio_list[cnt], gpio_pattern[pattern_pos]);
            gpio_get_value (gpio_list[cnt], &read);
            if (read != gpio_pattern[pattern_pos]) {
                printf ("%s : error, num = %d gpio = %d, set = %d, get = %d\n",
                    __func__, cnt, gpio_list[cnt], gpio_pattern [pattern_pos], read);
                err_cnt++;
            }
            pattern_pos = !pattern_pos;
        } else nc_pin_cnt++;
    }
    if (nc_pin_cnt == gpio_cnt) {
        printf ("%s : not used header id(%d)\n", __func__, id);
        return 0;
    }
    return err_cnt ? 0 : 1;
}

//------------------------------------------------------------------------------
static int pattern_compare (int id, int pattern, int *resp_pt)
{
    int cnt = 0, err_cnt = 0, nc_pin_cnt = 0;
    int gpio_cnt = 0, *gpio_list, *gpio_pattern, pattern_pos = 0;

    switch (id) {
        case eHEADER_7:
            gpio_cnt     = sizeof(HEADER7)/sizeof(HEADER7[0]);
            gpio_list    = (int *)HEADER7;
            break;
        case eHEADER_14:
            gpio_cnt     = sizeof(HEADER14)/sizeof(HEADER14[0]);
            gpio_list    = (int *)HEADER14;
            break;
        case eHEADER_40:
            gpio_cnt     = sizeof(HEADER40)/sizeof(HEADER40[0]);
            gpio_list    = (int *)HEADER40;
            break;
        default:
            printf ("%s : unknown header (id = %d)\n", __func__, id);
            return 0;
    }
    gpio_pattern = (int *)&HEADER_PATTERN[pattern % 4][0];

    for (cnt = 0; cnt < gpio_cnt; cnt++) {
        if (gpio_list[cnt] != NC) {
            if (gpio_pattern[pattern_pos] != resp_pt [cnt]) {
                printf ("%s : error, num = %d gpio = %d, set = %d, get = %d\n",
                    __func__, cnt, gpio_list [cnt], gpio_pattern [pattern_pos], resp_pt [cnt]);
                err_cnt++;
            }
            pattern_pos = !pattern_pos;
        } else nc_pin_cnt++;
    }

    if (nc_pin_cnt == gpio_cnt) {
        printf ("%s : not used header id(%d)\n", __func__, id);
        return 0;
    }
    return err_cnt ? 0 : 1;
}

//------------------------------------------------------------------------------
int header_data_check (int dev_id, char *resp_s)
{
    int id = DEVICE_ID(dev_id), i;
    int resp_pt[sizeof(HEADER40)/sizeof(HEADER40[0])];

    memset (resp_pt, 0, sizeof(resp_pt));

    for (i = 0; i < 20; i++) {
        switch (resp_s[i]) {
            default :
            case '0': resp_pt [1 + (i*2)] = 0; resp_pt [2 + (i*2)] = 0; break;
            case '1': resp_pt [1 + (i*2)] = 1; resp_pt [2 + (i*2)] = 0; break;
            case '2': resp_pt [1 + (i*2)] = 0; resp_pt [2 + (i*2)] = 1; break;
            case '3': resp_pt [1 + (i*2)] = 1; resp_pt [2 + (i*2)] = 1; break;
        }
    }

    return pattern_compare (id, DEVICE_ACTION(dev_id), resp_pt);
}

//------------------------------------------------------------------------------
int header_check (int dev_id, char *resp)
{
    int status = 0, id = DEVICE_ID(dev_id);

    status = pattern_write (id, DEVICE_ACTION(dev_id)) ? 1 : -1;
    switch (id) {
        case eHEADER_40:
            DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', HEADER40_CON);
            break;
        case eHEADER_14:
            DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', HEADER14_CON);
            break;
        case eHEADER_7:
            DEVICE_RESP_FORM_STR (resp, (status == 1) ? 'C' : 'F', HEADER7_CON);
            break;
        default :
            break;
    }
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(resp), resp);
    return status;
}

//------------------------------------------------------------------------------
void header_grp_init (char *cfg)
{
    char *tok;
    int did, h_s, h_c, *h_a, i;

    if ((tok = strtok (cfg, ",")) != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL) {
            did = atoi(tok);
            if (did != eHEADER_CFG) {
                if ((tok = strtok (NULL, ",")) != NULL) h_s = atoi (tok);
                if ((tok = strtok (NULL, ",")) != NULL) h_c = atoi (tok);
                switch (did) {
                    case eHEADER_40: h_a = &HEADER40[0]; break;
                    case eHEADER_14: h_a = &HEADER14[0]; break;
                    case eHEADER_7:  h_a = &HEADER7 [0]; break;
                    default :
                        printf ("%s : error! unknown did = %d\n", __func__, did);
                        return;
                }
                for (i = 0; i < h_c; i++) {
                    if ((tok = strtok (NULL, ",")) != NULL) h_a [h_s +i] = atoi (tok);
                    if (h_a[h_s +i] != NC) {
                        gpio_export    (h_a[h_s +i]);   gpio_direction (h_a[h_s +i], 1);
                    }
                }
            } else {
                // Header ADC Name(h_s == 0)
                if ((tok = strtok (NULL, ",")) != NULL) {
                    switch (atoi(tok)) {
                        case eHEADER_40:
                            HEADER40[0] = NC;
                            if ((tok = strtok (NULL, ",")) != NULL)
                                strncpy (HEADER40_CON, tok, strlen(tok));
                            break;
                        case eHEADER_14:
                            HEADER14[0] = NC;
                            if ((tok = strtok (NULL, ",")) != NULL)
                                strncpy (HEADER14_CON, tok, strlen(tok));
                            break;
                        case eHEADER_7:
                            HEADER7 [0] = NC;
                            if ((tok = strtok (NULL, ",")) != NULL)
                                strncpy (HEADER7_CON,  tok, strlen(tok));
                            break;
                        default :
                            printf ("%s : error! unknown did = %d\n", __func__, did);
                            return;
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
