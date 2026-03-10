/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */


#ifndef __UART_LAUNCHER_H__
#define __UART_LAUNCHER_H__

#define VERSION                 "1.0.2021052201"

#ifndef N_MTKSTP
#define N_MTKSTP    (15 + 1)  /* MediaTek WCN Serial Transport Protocol */
#endif

#define N_MTK (15+1)
#define HCIUARTSETPROTO _IOW('U', 200, int)
#define HCIUARTSETBAUD _IOW('U', 201, int)
#define HCIUARTGETBAUD _IOW('U', 202, int)
#define HCIUARTSETSTP _IOW('U', 203, int)
#define HCIUARTLOADPATCH _IOW('U', 204, int)
#define HCIUARTSETWAKEUP _IOW('U', 205, int)
#define HCIUARTINIT _IOW('U', 206, int)
#define HCIUARTDEINIT _IOW('U', 207, int)

#define CUST_COMBO_WMT_DEV "/dev/stpwmt"
#define CUST_COMBO_STP_DEV "/dev/ttyUSB0"
#define CUST_COMBO_PATCH_PATH "/etc/firmware" //-- for ALPS
#define LOG_TAG "uart_launcher"

#define UL_MSG_LVL_DBG       3
#define UL_MSG_LVL_INFO      2
#define UL_MSG_LVL_ERR       1
#define UL_MSG_LVL_NONE      0


#ifdef __ANDROID__
/* print log to main log */
/* LOG_TAG must be defined before log.h */
#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG               "btmtk_uart_launcher"
#include <log/log.h>
#include <android/log.h>


/* Debug log level */
#define UL_MSG_LVL_DEFAULT           UL_MSG_LVL_INFO

#else /* __ANDROID__ */
#define ALOGI	printf
#endif

#define BPRINT_D(fmt, ...) \
    do { if (UL_MSG_LVL_DEFAULT >= UL_MSG_LVL_DBG) \
        ALOGI("[%s:D] "fmt"\n", LOG_TAG, ##__VA_ARGS__);   } while (0);
#define BPRINT_I(fmt, ...) \
    do { if (UL_MSG_LVL_DEFAULT >= UL_MSG_LVL_INFO) \
        ALOGI("[%s:I] "fmt"\n", LOG_TAG, ##__VA_ARGS__);     } while (0);
#define BPRINT_E(fmt, ...) \
    do { if (UL_MSG_LVL_DEFAULT >= UL_MSG_LVL_ERR) \
        ALOGI("[%s:E] "fmt" !!!\n", LOG_TAG, ##__VA_ARGS__);} while (0);


#define CUST_BAUDRATE_DFT 115200
#define CUST_MULTI_PATCH 1

#ifdef CFG_MTK_SOC_CONSYS_SUPPORT
#define CUST_MTK_SOC_CONSYS (1)
#else
#define CUST_MTK_SOC_CONSYS (0)
#endif

#define INIT_CMD(c, e, s) {.cmd= c, .cmd_sz=sizeof(c), .evt=e, .evt_sz=sizeof(e), .str=s}

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif


typedef enum {
    STP_MIN = 0x0,
    STP_UART_FULL = 0x1,
    STP_UART_MAND = 0x2,
    STP_BTIF_FULL = 0x3,
    STP_SDIO = 0x4,
    STP_MAX = 0x5,
}STP_MODE;

#define MAX_CMD_LEN (NAME_MAX+1)

enum UART_FC {
    UART_DISABLE_FC = 0, /*NO flow control*/
    /*MTK SW Flow Control, differs from Linux Flow Control*/
    UART_MTK_SW_FC = 1,
    UART_LINUX_FC = 2,   /*Linux SW Flow Control*/
    UART_HW_FC = 3,      /*HW Flow Control*/
};

struct UART_CONFIG {
    enum UART_FC fc;
    int parity;
    int stop_bit;
    int iBaudrate;
};

struct speed_map {
    unsigned int baud;
    speed_t      speed;
};

static struct speed_map speeds[] = {
    {115200,    B115200},
    {921600,    B921600},
    {1000000,    B1000000},
    {1152000,    B1152000},
    {2000000,    B2000000},
    {2500000,    B2500000},
    {3000000,    B3000000},
    {3500000,    B3500000},
    {4000000,    B4000000},
};

#endif /*__UART_LAUNCHER_H__*/
