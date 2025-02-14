//------------------------------------------------------------------------------
/**
 * @file lib_dev_check.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief Device Test library for ODROID-JIG.
 * @version 2.0
 * @date 2024-11-19
 *
 * @package apt install iperf3, nmap, ethtool, usbutils, alsa-utils
 *
 * @copyright Copyright (c) 2022
 *
 */
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

//------------------------------------------------------------------------------
#include "lib_dev_check.h"

//------------------------------------------------------------------------------
// return 1 : find success, 0 : not found
//------------------------------------------------------------------------------
int find_file_path (const char *fname, char *file_path)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH * 2];

    memset (cmd_line, 0, sizeof(cmd_line));
    sprintf(cmd_line, "%s\n", "pwd");

    if (NULL != (fp = popen(cmd_line, "r"))) {
        memset (cmd_line, 0, sizeof(cmd_line));
        fgets  (cmd_line, STR_PATH_LENGTH, fp);
        pclose (fp);

        strncpy (file_path, cmd_line, strlen(cmd_line)-1);

        memset (cmd_line, 0, sizeof(cmd_line));
        sprintf(cmd_line, "find -name %s\n", fname);
        if (NULL != (fp = popen(cmd_line, "r"))) {
            memset (cmd_line, 0, sizeof(cmd_line));
            fgets  (cmd_line, STR_PATH_LENGTH, fp);
            pclose (fp);
            if (strlen(cmd_line)) {
                strncpy (&file_path[strlen(file_path)], &cmd_line[1], strlen(cmd_line)-1);
                file_path[strlen(file_path)-1] = 0;
                return 1;
            }
            return 0;
        }
    }
    pclose(fp);
    return 0;
}

//------------------------------------------------------------------------------
int device_resp_parse (const char *resp_msg, parse_resp_data_t *pdata)
{
    int msg_size = (int)strlen(resp_msg);
    char *ptr, resp[SERIAL_RESP_SIZE+1];

    if ((msg_size != SERIAL_RESP_SIZE) && (msg_size != DEVICE_RESP_SIZE)) {
        printf ("%s : unknown resp size = %d, resp = %s\n", __func__, msg_size, resp_msg);
        return 0;
    }

    memset (resp,   0, sizeof(resp));
    memset (pdata,  0, sizeof(parse_resp_data_t));

    // copy org msg
    memcpy (resp, resp_msg, msg_size);

    if ((ptr = strtok (resp, ",")) != NULL) {
        if (msg_size == SERIAL_RESP_SIZE) {
            // cmd
            if ((ptr = strtok (NULL, ",")) != NULL) pdata->cmd = *ptr;
            // gid
            if ((ptr = strtok (NULL, ",")) != NULL) pdata->gid = atoi(ptr);
            // did
            if ((ptr = strtok (NULL, ",")) != NULL) pdata->did = atoi(ptr);

            ptr = strtok (NULL, ",");
        }

        // status
        if (ptr != NULL) {
            pdata->status_c =  *ptr;
            pdata->status_i = (*ptr == 'P') ? 1 : 0;
        }
        // resp str
        if ((ptr = strtok (NULL, ",")) != NULL) {
            {
                int i, pos;
                for (i = 0, pos = 0; i < DEVICE_RESP_SIZE -2; i++)
                {
                    if ((*(ptr + i) != 0x20) && (*(ptr + i) != 0x00))
                        pdata->resp_s[pos++] = *(ptr + i);
                }
            }
            pdata->resp_i = atoi(ptr);
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
int device_resp_check (parse_resp_data_t *pdata)
{
    char resp [DEVICE_RESP_SIZE+1];
    memset (resp, 0, sizeof(resp));

    switch (pdata->gid) {
        /* IR Thread running */
        case eGID_IR:
            pdata->status_i = ir_check (pdata->did, resp);
            return pdata->status_i;

         case eGID_ETHERNET:
            if (pdata->did == eETHERNET_IPERF)  {
                pdata->status_i = ethernet_check (pdata->did, resp);
                return pdata->status_i;
            }
            break;
        case eGID_LED:
            pdata->status_i = led_data_check (pdata->did, pdata->resp_i);
            break;
        case eGID_HEADER:
            pdata->status_i = header_data_check (pdata->did, pdata->resp_s);
            break;

        /* not implement */
        case eGID_PWM: case eGID_GPIO: case eGID_AUDIO:
        default:
            pdata->status_i = 0;
            break;
    }
    // device_data_check ok
    return 1;
}

//------------------------------------------------------------------------------
//
// status value : 0 -> Wait, 1 -> Success, -1 -> Error
//
//------------------------------------------------------------------------------
int device_check (int gid, int did, char *dev_resp)
{
    int status  = 0;

    memset (dev_resp, 0, DEVICE_RESP_SIZE);

    switch(gid) {
        case eGID_SYSTEM:   status = system_check   (did, dev_resp);  break;
        case eGID_ADC:      status = adc_check      (did, dev_resp);  break;
        case eGID_HDMI:     status = hdmi_check     (did, dev_resp);  break;
        case eGID_STORAGE:  status = storage_check  (did, dev_resp);  break;
        case eGID_USB:      status = usb_check      (did, dev_resp);  break;
        case eGID_ETHERNET: status = ethernet_check (did, dev_resp);  break;
        case eGID_HEADER:   status = header_check   (did, dev_resp);  break;
        case eGID_AUDIO:    status = audio_check    (did, dev_resp);  break;
        case eGID_PWM:      status = pwm_check      (did, dev_resp);  break;
        case eGID_LED:      status = led_check      (did, dev_resp);  break;
        case eGID_IR:       status = ir_check       (did, dev_resp);  break;
        case eGID_GPIO:     status = gpio_check     (did, dev_resp);  break;
        case eGID_FW:       status = fw_check       (did, dev_resp);  break;
        default :
            sprintf (dev_resp, "0,%20s", "unkonwn");
            break;
    }
    printf ("%s : [size = %d] -> %s\n", __func__, (int)strlen(dev_resp), dev_resp);

    return status;
}

//------------------------------------------------------------------------------
#if 0
ui_grp_t *ui_init (fb_info_t *fb, const char *cfg_filename)
{
    ui_grp_t *ui_grp;
    FILE *pfd;
    char buf[256], is_cfg_file = 0;

    if ((pfd = fopen(cfg_filename, "r")) == NULL)
        return   NULL;

    if ((ui_grp = (ui_grp_t *)malloc(sizeof(ui_grp_t))) == NULL)
        return   NULL;

    memset (ui_grp, 0x00, sizeof(ui_grp_t));
    memset (buf,    0x00, sizeof(buf));

    while(fgets(buf, sizeof(buf), pfd) != NULL) {
        if (!is_cfg_file) {
            is_cfg_file = strncmp ("ODROID-UI-CONFIG", buf, strlen(buf)-1) == 0 ? 1 : 0;
            memset (buf, 0x00, sizeof(buf));
            continue;
        }
        switch(buf[0]) {
            case  'C':  _ui_parser_cmd_C (buf, fb, ui_grp); break;
            case  'B':  _ui_parser_cmd_B (buf, fb, ui_grp); break;
            case  'I':  _ui_parser_cmd_I (buf, ui_grp);     break;
            case  'T':  _ui_parser_cmd_T (buf, ui_grp);     break;
            case  'R':  _ui_parser_cmd_R (buf, fb, ui_grp); break;
            case  'S':  _ui_parser_cmd_S (buf, ui_grp);     break;
            default :
                fprintf(stdout, "ERROR: Unknown parser command! cmd = %c\n", buf[0]);
            case  '#':  case  '\n':
                break;
        }
        memset (buf, 0x00, sizeof(buf));
    }

    if (!is_cfg_file) {
        fprintf(stdout, "ERROR: UI Config File not found! (filename = %s)\n", cfg_filename);
        free (ui_grp);
        return NULL;
    }

    /* all item update */
    if (ui_grp->b_item_cnt)
        ui_update (fb, ui_grp, -1);

    // cfg file close
    if (pfd)
        fclose (pfd);

    // file parser
    return   ui_grp;
}
#endif

//------------------------------------------------------------------------------
static int device_setup (void)
{
    FILE *pfd;
    char buf[STR_PATH_LENGTH] = {0,}, *ptr;

    memset (buf, 0, sizeof(buf));
    if (!find_file_path (CONFIG_FILE_NAME, buf)) {
        printf ("%s : %s file not found!\n", __func__, CONFIG_FILE_NAME);
        return 0;
    }

    if ((pfd = fopen(buf, "r")) == NULL) {
        printf ("%s : %s file open error!\n", __func__, buf);
        return 0;
    }

    while (fgets(buf, sizeof(buf), pfd) != NULL) {

        if (buf[0] == '#' || buf[0] == '\n')  continue;

//        printf ("%s : buf = %s\n", __func__, buf);
        if ((ptr = strstr (buf, "SYSTEM"))    != NULL)  system_grp_init (buf);
        if ((ptr = strstr (buf, "STORAGE"))   != NULL)  storage_grp_init (buf);
        if ((ptr = strstr (buf, "USB"))       != NULL)  usb_grp_init (buf);
        if ((ptr = strstr (buf, "HDMI"))      != NULL)  hdmi_grp_init (buf);
        if ((ptr = strstr (buf, "ADC"))       != NULL)  adc_grp_init (buf);

        if ((ptr = strstr (buf, "ETHERNET"))  != NULL)  ethernet_grp_init (buf);
        if ((ptr = strstr (buf, "HEADER"))    != NULL)  header_grp_init (buf);

        if ((ptr = strstr (buf, "AUDIO"))   != NULL)
        {
            printf ("%s : %s line -> %s\n", __func__, ptr, buf);
        }
    }
    fclose (pfd);
    return 1;
}

//------------------------------------------------------------------------------
int device_init (void)
{
    #if 0
    system_grp_init ();
    // usb f/w upgrade check
    fw_grp_init();
    // iperf, ip, mac, check
    ethernet_grp_init ();

    // sysfile read
    hdmi_grp_init ();
    adc_grp_init ();

    // thread func
    ir_grp_init ();
    storage_grp_init ();
    usb_grp_init ();

    // i2c ADC check
    header_grp_init ();
    // audio 1Khz L, R
    audio_grp_init ();
    // pwm1, pwm2
    pwm_grp_init ();
    // alive, power, eth_green, eth_orange
    led_grp_init ();
    // gpio number test
    gpio_grp_init ();
#endif

    return device_setup ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
