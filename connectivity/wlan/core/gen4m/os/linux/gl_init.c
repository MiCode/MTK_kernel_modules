// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_init.c
 *    \brief  Main routines of Linux driver
 *
 *    This file contains the main routines of Linux driver for MediaTek Inc.
 *    802.11 Wireless LAN Adapters.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "gl_cfg80211.h"
#include "precomp.h"
#if ((CFG_SUPPORT_AGPS_ASSIST) || (CFG_VOLT_INFO == 1))
#include "gl_kal.h"
#endif
#if CFG_TC1_FEATURE
#include <tc1_partition.h>
#endif
#if CFG_CHIP_RESET_SUPPORT
#include "gl_rst.h"
#endif
#include "gl_vendor.h"
#include "gl_hook_api.h"
#include "mddp.h"
#if CFG_SUPPORT_NAN
#include "gl_vendor_ndp.h"
#endif
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
#include "gl_ics.h"
#endif
#if CFG_SUPPORT_SA_LOG
#include "gl_sa_log.h"
#endif
#if CFG_POWER_OFF_CTRL_SUPPORT
#include <linux/reboot.h>
#endif
#include "gl_coredump.h"
#include "gl_fw_log.h"
#if (CFG_SUPPORT_FW_IDX_LOG_SAVE == 1)
#include "gl_fw_dev.h"
#endif
#if CFG_SUPPORT_MET_LOG
#include "gl_met_log.h"
#endif
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#include "conninfra.h"
#endif

#include "wlan_pinctrl.h"

#if CFG_SUPPORT_CSI
#include "gl_csi.h"
#endif

#if ARP_MONITER_ENABLE
#include "arp_mon.h"
#endif /* ARP_MONITER_ENABLE */

#if (CFG_SUPPORT_IGMP_OFLD == 1)
#include <linux/igmp.h>
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
#include "rlm_domain.h"
#endif

#if CFG_SUPPORT_MBRAIN
#include "gl_mbrain.h"
#endif

#if CFG_MTK_ANDROID_WMT
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* #define MAX_IOREQ_NUM   10 */
struct semaphore g_halt_sem;
int g_u4HaltFlag;
int g_u4WlanInitFlag;
atomic_t g_wlanProbing;
atomic_t g_wlanRemoving;
enum ENUM_NVRAM_STATE g_NvramFsm = NVRAM_STATE_INIT;

uint8_t g_aucNvram[MAX_CFG_FILE_WIFI_REC_SIZE];
uint8_t g_aucNvram_OnlyPreCal[MAX_CFG_FILE_WIFI_RECAL_SIZE];
#if CFG_SUPPORT_XONVRAM
struct XO_CFG_PARAM_STRUCT g_rXonvCfg;
#endif
struct wireless_dev *gprWdev[KAL_AIS_NUM];
#if CFG_MTK_ANDROID_WMT
u_int8_t g_IsPlatCbsRegistered = FALSE;
#endif
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
bool fgIsPreOnProcessing = FALSE;
#endif
#if CFG_AP_80211KVR_INTERFACE
#define NETLINK_OSS_KERNEL 25
struct sock *nl_sk;
#endif/* CFG_AP_80211KVR_INTERFACE */
struct service_test *gprServiceTest;

// MIUI ADD
#define WMM_MIN_UP_VALUE 0
#define WMM_MAX_UP_VALUE 8
// END

/* Default QoS Map for BSS other than AIS */
static struct cfg80211_qos_map default_qos_map = {
#if CFG_WIFI_AT_THE_EDGE_QOS
	.num_des = 15,
#else
	.num_des = 17,
#endif
	.dscp_exception = { /* dscp, up */
		{8, 1},
		{18, 3}, {20, 3}, {22, 3},
		{24, 4}, {26, 4}, {28, 4}, {30, 4},
		{32, 4}, {34, 4}, {36, 4}, {38, 4},
		{40, 5},
		{44, 6}, {46, 6},
#if !CFG_WIFI_AT_THE_EDGE_QOS
		/* Extend for backward compatibility traffic generation.
		 * Allow to set 48, 56 to UP 6, 7, intended to now following
		 * RECOMMENDATION in RFC 8325 Sec 8.2.
		 */
		{48, 6}, {56, 7},
#endif
	},
	.up = {{0, 63}, },/* low, high */
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* Tasklet mechanism is like buttom-half in Linux. We just want to
 * send a signal to OS for interrupt defer processing. All resources
 * are NOT allowed reentry, so txPacket, ISR-DPC and ioctl must avoid preempty.
 */
struct WLANDEV_INFO {
	struct net_device *prDev;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

MODULE_AUTHOR(NIC_AUTHOR);
MODULE_DESCRIPTION(NIC_DESC);
#if KERNEL_VERSION(5, 12, 0) > CFG80211_VERSION_CODE
MODULE_SUPPORTED_DEVICE(NIC_NAME);
#endif

/* MODULE_LICENSE("MTK Propietary"); */
MODULE_LICENSE("Dual BSD/GPL");

#ifdef CFG_DRIVER_INF_NAME_CHANGE
char *gprifnamesta = "";
char *gprifnamep2p = "";
char *gprifnameap = "";
module_param_named(sta, gprifnamesta, charp, 0000);
module_param_named(p2p, gprifnamep2p, charp, 0000);
module_param_named(ap, gprifnameap, charp, 0000);
#endif /* CFG_DRIVER_INF_NAME_CHANGE */

char *gprifnamenvram = "";
module_param_named(nvram, gprifnamenvram, charp, 0000);

#if CFG_SUPPORT_XONVRAM
char *gprifnamexonv = "";
module_param_named(xonvram, gprifnamexonv, charp, 0000);
#endif

#if (CFG_SUPPORT_CONNFEM == 1 && CFG_CONNFEM_DEFAULT == 1)
uint32_t gu4ConnfemId;
module_param_named(connfemid, gu4ConnfemId, uint, 0000);
#endif

/* NIC interface name */
#ifdef CFG_COMBO_SLT_GOLDEN
#define NIC_INF_NAME    "ra%d"
#else
#define NIC_INF_NAME    "wlan%d"
#endif

#ifdef CFG_DRIVER_INF_NAME_CHANGE
/* Kernel IFNAMESIZ is 16, we use 5 in case some protocol might auto gen
 * interface name,
 */
/* in that case, the interface name might have risk of over kernel's IFNAMESIZ
 */
#define CUSTOM_IFNAMESIZ 5
#endif /* CFG_DRIVER_INF_NAME_CHANGE */

uint8_t aucDebugModule[DBG_MODULE_NUM];
uint32_t au4LogLevel[ENUM_WIFI_LOG_MODULE_NUM] = {ENUM_WIFI_LOG_LEVEL_DEFAULT};

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
u_int8_t fgIsCurrentInTestMode;
#endif

/* 4 2007/06/26, mikewu, now we don't use this, we just fix the number of wlan
 *               device to 1
 */
static struct WLANDEV_INFO
	arWlanDevInfo[CFG_MAX_WLAN_DEVICES] = { {0} };

static uint32_t
u4WlanDevNum;	/* How many NICs coexist now */

#if CFG_MTK_ANDROID_WMT
/* 0: off, 1: on-going, 2: done */
static enum ENUM_SHUTDOWN_STATE uShutdownState;
#endif

/* 20150205 added work queue for sched_scan to avoid cfg80211 stop schedule scan
 *          dead loack
 */
struct delayed_work sched_workq;

#define CFG_EEPRM_FILENAME    "EEPROM"
#define FILE_NAME_MAX     64

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
static uint8_t *apucEepromName[] = {
	(uint8_t *) CFG_EEPRM_FILENAME "_MT",
	NULL
};
#endif

#if CFG_SUPPORT_BUFFER_MODE
uint8_t	uacEEPROMImage[MAX_EEPROM_BUFFER_SIZE] = {
	/* 0x000 ~ 0x00F */
	0xAE, 0x86, 0x06, 0x00, 0x18, 0x0D, 0x00, 0x00,
	0xC0, 0x1F, 0xBD, 0x81, 0x3F, 0x01, 0x19, 0x00,
	/* 0x010 ~ 0x01F */
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
	/* 0x020 ~ 0x02F */
	0x80, 0x02, 0x00, 0x00, 0x32, 0x66, 0xC3, 0x14,
	0x32, 0x66, 0xC3, 0x14, 0x03, 0x22, 0xFF, 0xFF,
	/* 0x030 ~ 0x03F */
	0x23, 0x04, 0x0D, 0xF2, 0x8F, 0x02, 0x00, 0x80,
	0x0A, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x040 ~ 0x04F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x33, 0x40, 0x00, 0x00,
	/* 0x050 ~ 0x05F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08,
	/* 0x060 ~ 0x06F */
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08,
	/* 0x070 ~ 0x07F */
	0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0xE0, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x080 ~ 0x08F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x090 ~ 0x09F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x0A0 ~ 0x0AF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x0B0 ~ 0x0BF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x92, 0x10, 0x10, 0x28, 0x00, 0x00, 0x00, 0x00,
	/* 0x0C0 ~ 0x0CF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x0D0 ~ 0x0DF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x0E0 ~ 0x0EF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x0F0 ~ 0x0FF */
	0x0E, 0x05, 0x06, 0x06, 0x06, 0x0F, 0x00, 0x00,
	0x0E, 0x05, 0x06, 0x05, 0x05, 0x09, 0xFF, 0x00,
	/* 0x100 ~ 0x10F */
	0x12, 0x34, 0x56, 0x78, 0x2C, 0x2C, 0x28, 0x28,
	0x28, 0x26, 0x26, 0x28, 0x28, 0x28, 0x26, 0xFF,
	/* 0x110 ~ 0x11F */
	0x26, 0x25, 0x28, 0x21, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x27, 0x27, 0x27, 0x25,
	/* 0x120 ~ 0x12F */
	0x25, 0x25, 0x25, 0x25, 0x23, 0x23, 0x23, 0x21,
	0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00,
	/* 0x130 ~ 0x13F */
	0x40, 0x40, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0,
	0xD0, 0xD0, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25,
	/* 0x140 ~ 0x14F */
	0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x150 ~ 0x15F */
	0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0,
	/* 0x160 ~ 0x16F */
	0xD0, 0xD0, 0xD0, 0x25, 0x25, 0x25, 0x25, 0x25,
	0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x170 ~ 0x17F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xC2, 0xC4, 0xC5, 0xC8,
	/* 0x180 ~ 0x18F */
	0x00, 0x26, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x190 ~ 0x19F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x1A0 ~ 0x1AF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0xD0,
	0xD0, 0x0E, 0x05, 0x06, 0x05, 0x09, 0x0E, 0x00,
	/* 0x1B0 ~ 0x1BF */
	0x05, 0x06, 0x05, 0x05, 0x09, 0x00, 0x00, 0x00,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	/* 0x1C0 ~ 0x1CF */
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00,
	/* 0x1D0 ~ 0x1DF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x1E0 ~ 0x1EF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x1F0 ~ 0x1FF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x200 ~ 0x20F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x210 ~ 0x21F */
	0x48, 0xF5, 0x27, 0x49, 0x48, 0xF5, 0x57, 0x12,
	0x4B, 0x71, 0x80, 0x50, 0x91, 0xF6, 0x87, 0x50,
	/* 0x220 ~ 0x22F */
	0x7D, 0x29, 0x09, 0x42, 0x7D, 0x29, 0x41, 0x44,
	0x7D, 0x29, 0x41, 0x3C, 0x7D, 0x29, 0x31, 0x4D,
	/* 0x230 ~ 0x23F */
	0x49, 0x71, 0x24, 0x49, 0x49, 0x71, 0x54, 0x12,
	0x4B, 0x71, 0x80, 0x50, 0x91, 0xF6, 0x87, 0x50,
	/* 0x240 ~ 0x24F */
	0x7D, 0x29, 0x09, 0x42, 0x7D, 0x29, 0x41, 0x04,
	0x7D, 0x29, 0x41, 0x04, 0x7D, 0x29, 0x01, 0x40,
	/* 0x250 ~ 0x25F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x260 ~ 0x26F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x270 ~ 0x27F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x280 ~ 0x28F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x290 ~ 0x29F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x2A0 ~ 0x2AF */
	0x7D, 0x29, 0xC9, 0x16, 0x7D, 0x29, 0xC9, 0x16,
	0x44, 0x22, 0x32, 0x15, 0xEE, 0xEE, 0xEE, 0x08,
	/* 0x2B0 ~ 0x2BF */
	0x78, 0x90, 0x79, 0x1C, 0x78, 0x90, 0x79, 0x1C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x2C0 ~ 0x2CF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x2D0 ~ 0x2DF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x2E0 ~ 0x2EF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x2F0 ~ 0x2FF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x300 ~ 0x30F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x310 ~ 0x31F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x42, 0x10, 0x42, 0x08, 0x21,
	/* 0x320 ~ 0x32F */
	0x10, 0x42, 0x08, 0x21, 0x10, 0x42, 0x08, 0x21,
	0x10, 0x42, 0x08, 0x21, 0x10, 0x42, 0x08, 0x21,
	/* 0x330 ~ 0x33F */
	0x10, 0x42, 0x08, 0x21, 0x10, 0x42, 0x08, 0x21,
	0x10, 0x42, 0x08, 0x21, 0x10, 0x42, 0x08, 0x21,
	/* 0x340 ~ 0x34F */
	0x10, 0x42, 0x08, 0x21, 0x10, 0x42, 0x08, 0x21,
	0x10, 0x42, 0x08, 0x21, 0x10, 0x42, 0x08, 0x01,
	/* 0x350 ~ 0x35F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x360 ~ 0x36F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x370 ~ 0x37F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x380 ~ 0x38F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x390 ~ 0x39F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x3A0 ~ 0x3AF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x3B0 ~ 0x3BF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x3C0 ~ 0x3CF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x3D0 ~ 0x3DF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x3E0 ~ 0x3EF */
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	/* 0x3F0 ~ 0x3FF */
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
	/* 0x400 ~ 0x40F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x410 ~ 0x41F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x420 ~ 0x42F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x430 ~ 0x43F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x440 ~ 0x44F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x450 ~ 0x45F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x460 ~ 0x46F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x470 ~ 0x47F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x480 ~ 0x48F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x490 ~ 0x49F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x4A0 ~ 0x4AF */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
static struct dentry *dbgFsDir;

static int debugfs_u8_get(void *data, uint64_t *val)
{
	*val = *(uint8_t *)data;
	return 0;
}

static int debugfs_u8_set(void *data, uint64_t val)
{
	*(uint8_t *)data = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_u8, debugfs_u8_get, debugfs_u8_set, "%llu\n");

static int debugfs_u16_get(void *data, uint64_t *val)
{
	*val = *(uint16_t *)data;
	return 0;
}

static int debugfs_u16_set(void *data, uint64_t val)
{
	*(uint16_t *)data = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_u16, debugfs_u16_get, debugfs_u16_set, "%llu\n");

int32_t sysCreateMonDbgFs(struct GLUE_INFO *prGlueInfo)
{
	struct dentry *dbgFsFile;

	dbgFsDir = debugfs_create_dir("mtk_mon_dbgfs", NULL);
	if (!dbgFsDir) {
		DBGLOG(INIT, ERROR,
				"dbgFsDir is null for mtk_mon_dbgfs\n");
		return -1;
	}

	/* /sys/kernel/debug/mtk_mon_dbgfs/hemu_aid, mode: wr */
	dbgFsFile = debugfs_create_file("hemu_aid",
		0666, dbgFsDir, &prGlueInfo->u2Aid, &fops_u16);

	/* /sys/kernel/debug/mtk_mon_dbgfs/band_idx, mode: wr */
	dbgFsFile = debugfs_create_file("band_idx",
		0666, dbgFsDir, &prGlueInfo->ucBandIdx, &fops_u8);

	/* /sys/kernel/debug/mtk_mon_dbgfs/drop_fcs_err, mode: wr */
	dbgFsFile = debugfs_create_file("drop_fcs_err",
		0666, dbgFsDir, &prGlueInfo->fgDropFcsErrorFrame, &fops_u8);

	return 0;
}

void sysRemoveMonDbgFs(void)
{
	debugfs_remove_recursive(dbgFsDir);
}
#endif

/*  For DTV Ref project -> Default enable */
#if CFG_DC_USB_WOW_CALLBACK || CFG_POWER_OFF_CTRL_SUPPORT
/* Register  DC  wow callback */
int kalDcSetWow(void)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	uint32_t count = 0;
	int ret = 0;
#if !CFG_ENABLE_WAKE_LOCK
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen;

	GLUE_SPIN_LOCK_DECLARATION();
#endif

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo == NULL\n");
		return -ENODEV;
	}

	if (!prGlueInfo->prAdapter) {
		DBGLOG(INIT, ERROR, "prGlueInfo->prAdapter == NULL\n");
		return -ENODEV;
	}

	if (prGlueInfo && prGlueInfo->prAdapter) {
		prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
		prHifInfo = &prGlueInfo->rHifInfo;
#if CFG_DC_USB_WOW_CALLBACK
		prHifInfo->fgUsbShutdown = TRUE;
#endif
#if !CFG_ENABLE_WAKE_LOCK
		if (IS_FEATURE_ENABLED(prWifiVar->ucWow)) {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			if (prGlueInfo->prScanRequest) {
				kalCfg80211ScanDone(prGlueInfo->prScanRequest,
					TRUE);
				prGlueInfo->prScanRequest = NULL;
			}
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

			/* AIS flow: disassociation if wow_en=0 */
			DBGLOG(REQ, INFO, "Enter AIS pre-suspend\n");
			rStatus = kalIoctl(prGlueInfo,
						wlanoidAisPreSuspend,
						NULL,
						0,
						&u4BufLen);

			/* TODO: p2pProcessPreSuspendFlow
			 * In current design, only support AIS connection
			 * during suspend only.
			 * It need to add flow to deactive P2P (GC/GO)
			 * link during suspend flow.
			 * Otherwise, MT7668 would fail to enter deep sleep.
			 */
		}
#endif

		set_bit(SUSPEND_FLAG_FOR_WAKEUP_REASON,
			&prGlueInfo->prAdapter->ulSuspendFlag);
		set_bit(SUSPEND_FLAG_CLEAR_WHEN_RESUME,
			&prGlueInfo->prAdapter->ulSuspendFlag);
	}

	prGlueInfo->fgIsInSuspendMode = TRUE;

	/* Stop upper layers calling the device hard_start_xmit routine. */
	netif_tx_stop_all_queues(prGlueInfo->prDevHandler);

	/* wait wiphy device do cfg80211 suspend done, then start hif suspend */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanWaitCfg80211SuspendDone(prGlueInfo);

	/* change to pre-Suspend state & block cfg80211 ops */
	glUsbSetState(&prGlueInfo->rHifInfo, USB_STATE_PRE_SUSPEND);

#if CFG_CHIP_RESET_SUPPORT
	if (prGlueInfo->prAdapter->chip_info->fgIsSupportL0p5Reset)
		cancel_work_sync(&prGlueInfo->rWfsysResetWork);
#endif

	wlanSuspendPmHandle(prGlueInfo);

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	if (prBusInfo->asicUsbSuspend) {
		if (prBusInfo->asicUsbSuspend(prGlueInfo->prAdapter,
			prGlueInfo))
			return 0;
		else
			return -1;
	}

	halUSBPreSuspendCmd(prGlueInfo->prAdapter);

	while (prGlueInfo->rHifInfo.state != USB_STATE_SUSPEND) {
		if (count > 250) {
			DBGLOG(HAL, ERROR, "pre_suspend timeout\n");
			ret = -EFAULT;
			break;
		}
		mdelay(2);
		count++;
	}

	halDisableInterrupt(prGlueInfo->prAdapter);
	halTxCancelAllSending(prGlueInfo->prAdapter);

	/* pending cmd will be kept in queue
	 * and no one to handle it after HIF resume.
	 * In STR, it will result in cmd buf full and then cmd buf alloc fail .
	 */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanReleaseAllTxCmdQueue(prGlueInfo->prAdapter);
#if CFG_DC_USB_WOW_CALLBACK
	if (prHifInfo->udev->speed > USB_SPEED_HIGH)
		mtk_usb_shutdown_vnd_cmd(prGlueInfo);
#endif
	DBGLOG(HAL, STATE, "mtk_usb_suspend() done!\n");
	return ret;
}
#endif

int CFG80211_Suspend(struct wiphy *wiphy,
		     struct cfg80211_wowlan *wow)
{
	DBGLOG(INIT, INFO, "CFG80211 suspend CB\n");

	return 0;
}

int CFG80211_Resume(struct wiphy *wiphy)
{
	DBGLOG(INIT, INFO, "CFG80211 resume CB\n");

	return 0;
}

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

uint32_t mtk_akm_suites[] = {
	SWAP32(RSN_AKM_SUITE_802_1X),
	SWAP32(RSN_AKM_SUITE_PSK),
#if CFG_SUPPORT_802_11R
	SWAP32(RSN_AKM_SUITE_FT_802_1X),
	SWAP32(RSN_AKM_SUITE_FT_PSK),
#endif
#if CFG_SUPPORT_WPA3
	SWAP32(RSN_AKM_SUITE_SAE),
	SWAP32(RSN_AKM_SUITE_OWE),
	SWAP32(RSN_AKM_SUITE_SAE_EXT_KEY),
#if CFG_SUPPORT_802_11R
	SWAP32(RSN_AKM_SUITE_FT_OVER_SAE),
	SWAP32(RSN_AKM_SUITE_FT_SAE_EXT_KEY),
#endif
#endif
	SWAP32(RSN_AKM_SUITE_8021X_SUITE_B),
	SWAP32(RSN_AKM_SUITE_8021X_SUITE_B_192),
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	SWAP32(RSN_AKM_SUITE_FILS_SHA256),
	SWAP32(RSN_AKM_SUITE_FILS_SHA384),
#endif
	SWAP32(RSN_AKM_SUITE_OSEN),
#if CFG_SUPPORT_DPP
	SWAP32(RSN_AKM_SUITE_DPP),
#endif
};

#if KERNEL_VERSION(5, 8, 0) <= CFG80211_VERSION_CODE
	#define CHAN2G(_channel, _freq, _flags)		\
	{						\
		.band               = KAL_BAND_2GHZ,	\
		.center_freq        = (_freq),		\
		.freq_offset        = 0,		\
		.hw_value           = (_channel),	\
		.flags              = (_flags),		\
		.max_antenna_gain   = 0,		\
		.max_power          = 30,		\
	}
#else
	#define CHAN2G(_channel, _freq, _flags)		\
	{						\
		.band               = KAL_BAND_2GHZ,	\
		.center_freq        = (_freq),		\
		.hw_value           = (_channel),	\
		.flags              = (_flags),		\
		.max_antenna_gain   = 0,		\
		.max_power          = 30,		\
	}
#endif

static struct ieee80211_channel mtk_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

#if KERNEL_VERSION(5, 8, 0) <= CFG80211_VERSION_CODE
	#define CHAN5G(_channel, _flags)			\
	{							\
		.band              = KAL_BAND_5GHZ,		\
		.center_freq       =				\
			(((_channel >= 182) && (_channel <= 196)) ? \
			(4000 + (5 * (_channel))) : (5000 + (5 * (_channel)))),\
		.freq_offset       = 0,				\
		.hw_value          = (_channel),		\
		.flags             = (_flags),			\
		.max_antenna_gain  = 0,				\
		.max_power         = 30,			\
	}
#else
	#define CHAN5G(_channel, _flags)			\
	{							\
		.band              = KAL_BAND_5GHZ,		\
		.center_freq       =				\
			(((_channel >= 182) && (_channel <= 196)) ? \
			(4000 + (5 * (_channel))) : (5000 + (5 * (_channel)))),\
		.hw_value          = (_channel),		\
		.flags             = (_flags),			\
		.max_antenna_gain  = 0,				\
		.max_power         = 30,			\
	}
#endif

#if (CFG_SUPPORT_WIFI_6G == 1)
#if KERNEL_VERSION(5, 8, 0) <= CFG80211_VERSION_CODE
	#define CHAN6G(_channel, _flags)				\
	{								\
		.band               = KAL_BAND_6GHZ,			\
		.center_freq        =	\
			((_channel == 2) ? (5935) : (5950 + (5 * (_channel)))),\
		.freq_offset        = 0,				\
		.hw_value           = (_channel),			\
		.flags              = (_flags),				\
		.max_antenna_gain   = 0,				\
		.max_power          = 30,				\
	}
#else
	#define CHAN6G(_channel, _flags)				\
	{								\
		.band               = KAL_BAND_6GHZ,			\
		.center_freq        =	\
			((_channel == 2) ? (5935) : (5950 + (5 * (_channel)))),\
		.hw_value           = (_channel),			\
		.flags              = (_flags),				\
		.max_antenna_gain   = 0,				\
		.max_power          = 30,				\
	}
#endif
#endif

#if (CFG_SUPPORT_SINGLE_SKU_DFS_PROTECT == 1)
static struct ieee80211_channel mtk_5ghz_channels[] = {
	/* UNII-1 */
	CHAN5G(36, 0),
	CHAN5G(40, 0),
	CHAN5G(44, 0),
	CHAN5G(48, 0),
	/* UNII-2 */
	CHAN5G(52, IEEE80211_CHAN_RADAR),
	CHAN5G(56, IEEE80211_CHAN_RADAR),
	CHAN5G(60, IEEE80211_CHAN_RADAR),
	CHAN5G(64, IEEE80211_CHAN_RADAR),
	/* UNII-2e */
	CHAN5G(100, IEEE80211_CHAN_RADAR),
	CHAN5G(104, IEEE80211_CHAN_RADAR),
	CHAN5G(108, IEEE80211_CHAN_RADAR),
	CHAN5G(112, IEEE80211_CHAN_RADAR),
	CHAN5G(116, IEEE80211_CHAN_RADAR),
	CHAN5G(120, IEEE80211_CHAN_RADAR),
	CHAN5G(124, IEEE80211_CHAN_RADAR),
	CHAN5G(128, IEEE80211_CHAN_RADAR),
	CHAN5G(132, IEEE80211_CHAN_RADAR),
	CHAN5G(136, IEEE80211_CHAN_RADAR),
	CHAN5G(140, IEEE80211_CHAN_RADAR),
	CHAN5G(144, IEEE80211_CHAN_RADAR),
	/* UNII-3 */
	CHAN5G(149, 0),
	CHAN5G(153, 0),
	CHAN5G(157, 0),
	CHAN5G(161, 0),
	CHAN5G(165, 0)
};
#else
static struct ieee80211_channel mtk_5ghz_channels[] = {
	/* UNII-1 */
	CHAN5G(36, 0),
	CHAN5G(40, 0),
	CHAN5G(44, 0),
	CHAN5G(48, 0),
	/* UNII-2 */
	CHAN5G(52, 0),
	CHAN5G(56, 0),
	CHAN5G(60, 0),
	CHAN5G(64, 0),
	/* UNII-2e */
	CHAN5G(100, 0),
	CHAN5G(104, 0),
	CHAN5G(108, 0),
	CHAN5G(112, 0),
	CHAN5G(116, 0),
	CHAN5G(120, 0),
	CHAN5G(124, 0),
	CHAN5G(128, 0),
	CHAN5G(132, 0),
	CHAN5G(136, 0),
	CHAN5G(140, 0),
	CHAN5G(144, 0),
	/* UNII-3 */
	CHAN5G(149, 0),
	CHAN5G(153, 0),
	CHAN5G(157, 0),
	CHAN5G(161, 0),
	CHAN5G(165, 0)
};
#endif

#if (CFG_SUPPORT_WIFI_6G == 1)
static struct ieee80211_channel mtk_6ghz_channels[] = {
	/* UNII-5 */
	CHAN6G(2, 0),
	CHAN6G(1, 0),
	CHAN6G(5, 0),
	CHAN6G(9, 0),
	CHAN6G(13, 0),
	CHAN6G(17, 0),
	CHAN6G(21, 0),
	CHAN6G(25, 0),
	CHAN6G(29, 0),
	CHAN6G(33, 0),
	CHAN6G(37, 0),
	CHAN6G(41, 0),
	CHAN6G(45, 0),
	CHAN6G(49, 0),
	CHAN6G(53, 0),
	CHAN6G(57, 0),
	CHAN6G(61, 0),
	CHAN6G(65, 0),
	CHAN6G(69, 0),
	CHAN6G(73, 0),
	CHAN6G(77, 0),
	CHAN6G(81, 0),
	CHAN6G(85, 0),
	CHAN6G(89, 0),
	CHAN6G(93, 0),
	/* UNII-6 */
	CHAN6G(97, 0),
	CHAN6G(101, 0),
	CHAN6G(105, 0),
	CHAN6G(109, 0),
	CHAN6G(113, 0),
	CHAN6G(117, 0),
	/* UNII-7 */
	CHAN6G(121, 0),
	CHAN6G(125, 0),
	CHAN6G(129, 0),
	CHAN6G(133, 0),
	CHAN6G(137, 0),
	CHAN6G(141, 0),
	CHAN6G(145, 0),
	CHAN6G(149, 0),
	CHAN6G(153, 0),
	CHAN6G(157, 0),
	CHAN6G(161, 0),
	CHAN6G(165, 0),
	CHAN6G(169, 0),
	CHAN6G(173, 0),
	CHAN6G(177, 0),
	CHAN6G(181, 0),
	CHAN6G(185, 0),
	/* UNII-8 */
	CHAN6G(189, 0),
	CHAN6G(193, 0),
	CHAN6G(197, 0),
	CHAN6G(201, 0),
	CHAN6G(205, 0),
	CHAN6G(209, 0),
	CHAN6G(213, 0),
	CHAN6G(217, 0),
	CHAN6G(221, 0),
	CHAN6G(225, 0),
	CHAN6G(229, 0),
	CHAN6G(233, 0)
};
#endif

#define RATETAB_ENT(_rate, _rateid, _flags)	\
{						\
	.bitrate    = (_rate),			\
	.hw_value   = (_rateid),		\
	.flags      = (_flags),			\
}

/* for cfg80211 - rate table */
static struct ieee80211_rate mtk_rates[] = {
	RATETAB_ENT(10, 0x1000, 0),
	RATETAB_ENT(20, 0x1001, 0),
	RATETAB_ENT(55, 0x1002, 0),
	RATETAB_ENT(110, 0x1003, 0),	/* 802.11b */
	RATETAB_ENT(60, 0x2000, 0),
	RATETAB_ENT(90, 0x2001, 0),
	RATETAB_ENT(120, 0x2002, 0),
	RATETAB_ENT(180, 0x2003, 0),
	RATETAB_ENT(240, 0x2004, 0),
	RATETAB_ENT(360, 0x2005, 0),
	RATETAB_ENT(480, 0x2006, 0),
	RATETAB_ENT(540, 0x2007, 0),	/* 802.11a/g */
};

#define mtk_a_rates         (mtk_rates + 4)
#define mtk_a_rates_size    (ARRAY_SIZE(mtk_rates) - 4)
#define mtk_g_rates         (mtk_rates + 0)
#define mtk_g_rates_size    (ARRAY_SIZE(mtk_rates) - 0)

#define WLAN_MCS_INFO						\
{								\
	.rx_mask        = {0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0},	\
	.rx_highest     = 0,					\
	.tx_params      = IEEE80211_HT_MCS_TX_DEFINED,		\
}

#define WLAN_VHT_MCS_INFO					\
{								\
	.rx_mcs_map     = 0xFFFA,				\
	.rx_highest     = cpu_to_le16(867),			\
	.tx_mcs_map     = 0xFFFA,				\
	.tx_highest     = cpu_to_le16(867),			\
}

#define WLAN_HT_CAP						\
{								\
	.ht_supported   = true,					\
	.cap            = IEEE80211_HT_CAP_SUP_WIDTH_20_40	\
			| IEEE80211_HT_CAP_SM_PS		\
			| IEEE80211_HT_CAP_GRN_FLD		\
			| IEEE80211_HT_CAP_SGI_20		\
			| IEEE80211_HT_CAP_SGI_40,		\
	.ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,		\
	.ampdu_density  = IEEE80211_HT_MPDU_DENSITY_NONE,	\
	.mcs            = WLAN_MCS_INFO,			\
}

#define WLAN_VHT_CAP							\
{									\
	.vht_supported  = true,						\
	.cap            = IEEE80211_VHT_CAP_RXLDPC			\
			| IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ	\
			| IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_11454	\
			| IEEE80211_VHT_CAP_RXLDPC			\
			| IEEE80211_VHT_CAP_SHORT_GI_80			\
			| IEEE80211_VHT_CAP_TXSTBC			\
			| IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE	\
			| IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE,	\
	.vht_mcs        = WLAN_VHT_MCS_INFO,				\
}

#define WLAN_VHT_CAP_160	\
{									\
	.vht_supported  = true,						\
	.cap            = IEEE80211_VHT_CAP_RXLDPC			\
			| IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_11454	\
			| IEEE80211_VHT_CAP_RXLDPC			\
			| IEEE80211_VHT_CAP_SHORT_GI_80			\
			| IEEE80211_VHT_CAP_TXSTBC			\
			| IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160MHZ \
			| IEEE80211_VHT_CAP_EXT_NSS_BW_MASK \
			| IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE	\
			| IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE,	\
	.vht_mcs        = WLAN_VHT_MCS_INFO,				\
}

#if KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
#if (CFG_SUPPORT_802_11AX == 1)

#define WLAN_HE_CAP_ELEM_INFO					\
{								\
	.mac_cap_info[0] =					\
		IEEE80211_HE_MAC_CAP0_HTC_HE,			\
	.mac_cap_info[3] =					\
		IEEE80211_HE_MAC_CAP3_OMI_CONTROL,		\
	.phy_cap_info[0] =					\
		IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G    \
		| IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G,    \
}

#define WLAN_HE_CAP_160_ELEM_INFO					\
{								\
	.mac_cap_info[0] =					\
		IEEE80211_HE_MAC_CAP0_HTC_HE,			\
	.mac_cap_info[3] =					\
		IEEE80211_HE_MAC_CAP3_OMI_CONTROL,		\
	.phy_cap_info[0] =					\
		IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G    \
		| IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_160MHZ_IN_5G,    \
}

#define WLAN_HE_MCS_NSS_SUPP_INFO				\
{								\
	.rx_mcs_80 = cpu_to_le16(0xFFFA),			\
	.tx_mcs_80 = cpu_to_le16(0xFFFA),			\
}

#define WLAN_HE_CAP_INFO					\
{								\
	.has_he = true,						\
	.he_cap_elem = WLAN_HE_CAP_ELEM_INFO,			\
	.he_mcs_nss_supp = WLAN_HE_MCS_NSS_SUPP_INFO,		\
}

#define WLAN_HE_CAP_160_INFO					\
{								\
	.has_he = true,						\
	.he_cap_elem = WLAN_HE_CAP_160_ELEM_INFO,		\
	.he_mcs_nss_supp = WLAN_HE_MCS_NSS_SUPP_INFO,		\
}

#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE))

#define WLAN_EHT_CAP_ELEM_INFO					\
{								\
	.mac_cap_info[0] =					\
		IEEE80211_EHT_MAC_CAP0_OM_CONTROL,		\
	.phy_cap_info[0] =					\
		IEEE80211_EHT_PHY_CAP0_320MHZ_IN_6GHZ		\
		| IEEE80211_EHT_PHY_CAP0_242_TONE_RU_GT20MHZ	\
		| IEEE80211_EHT_PHY_CAP0_NDP_4_EHT_LFT_32_GI	\
		| IEEE80211_EHT_PHY_CAP0_SU_BEAMFORMER		\
		| IEEE80211_EHT_PHY_CAP0_SU_BEAMFORMEE,		\
}

#define WLAN_EHE_MCS_NSS_SUPP_BW_INFO				\
{								\
	.rx_tx_mcs9_max_nss = 0x22,				\
	.rx_tx_mcs11_max_nss = 0x22,				\
	.rx_tx_mcs13_max_nss = 0x22,				\
}

#define WLAN_EHE_MCS_NSS_SUPP_INFO				\
{								\
	.bw._80 = WLAN_EHE_MCS_NSS_SUPP_BW_INFO,		\
	.bw._160 = WLAN_EHE_MCS_NSS_SUPP_BW_INFO,		\
	.bw._320 = WLAN_EHE_MCS_NSS_SUPP_BW_INFO,		\
}

#define WLAN_EHT_CAP_INFO					\
{								\
	.has_eht = true,					\
	.eht_cap_elem = WLAN_EHT_CAP_ELEM_INFO,			\
	.eht_mcs_nss_supp = WLAN_EHE_MCS_NSS_SUPP_INFO,		\
}

#endif


static struct ieee80211_sband_iftype_data mtk_cap[] = {
	{
		.types_mask =
			BIT(NL80211_IFTYPE_AP),
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		.he_cap = WLAN_HE_CAP_160_INFO,
#else
		.he_cap = WLAN_HE_CAP_INFO,
#endif

#if (CFG_SUPPORT_802_11BE == 1)
#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE))
		.eht_cap = WLAN_EHT_CAP_INFO,
#endif
#endif
	},
	{
		.types_mask =
			BIT(NL80211_IFTYPE_STATION),
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		.he_cap = WLAN_HE_CAP_160_INFO,
#else
		.he_cap = WLAN_HE_CAP_INFO,
#endif

#if (CFG_SUPPORT_802_11BE == 1)
#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE))
		.eht_cap = WLAN_EHT_CAP_INFO,
#endif
#endif
	},
};

#endif
#endif

#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#if (CFG_SUPPORT_802_11AX == 1) && (CFG_SUPPORT_WIFI_6G == 1)

#define HE_6GHZ_MIN_MPDU_START_SPACE		0
#define HE_6GHZ_MAX_AMPUD_LEN_EXP		7
#define HE_6GHZ_MAX_MPDU			2

#define HE_6GHZ_BAND_CAP_MIN_MPDU_START              (BIT(0) | BIT(1) | BIT(2))
#define HE_6GHZ_BAND_CAP_MAX_AMPDU_LEN_EXP_MASK      (BIT(3) | BIT(4) | BIT(5))
#define HE_6GHZ_BAND_CAP_MAX_AMPDU_LEN_EXP_SHIFT     3
#define HE_6GHZ_BAND_CAP_MAX_MPDU_LEN_MASK           (BIT(6) | BIT(7))
#define HE_6GHZ_BAND_CAP_MAX_MPDU_LEN_SHIFT	     6
#define HE_6GHZ_BAND_CAP_SMPS_DISABLED               (BIT(9) | BIT(10))
#define HE_6GHZ_BAND_CAP_RX_ANTPAT_CONS              BIT(12)
#define HE_6GHZ_BAND_CAP_TX_ANTPAT_CONS              BIT(13)

#define WLAN_HE_6G_CAP_INFO						\
{									\
	.capa = (HE_6GHZ_MIN_MPDU_START_SPACE &				\
			HE_6GHZ_BAND_CAP_MIN_MPDU_START)		\
		| ((HE_6GHZ_MAX_AMPUD_LEN_EXP <<			\
			HE_6GHZ_BAND_CAP_MAX_AMPDU_LEN_EXP_SHIFT) &	\
			HE_6GHZ_BAND_CAP_MAX_AMPDU_LEN_EXP_MASK)	\
		| ((HE_6GHZ_MAX_MPDU <<					\
			HE_6GHZ_BAND_CAP_MAX_MPDU_LEN_SHIFT) &		\
			HE_6GHZ_BAND_CAP_MAX_MPDU_LEN_MASK)		\
		| HE_6GHZ_BAND_CAP_SMPS_DISABLED			\
		| HE_6GHZ_BAND_CAP_RX_ANTPAT_CONS			\
		| HE_6GHZ_BAND_CAP_TX_ANTPAT_CONS,			\
}

static struct ieee80211_sband_iftype_data mtk_cap_6g[] = {
	{
		.types_mask =
			BIT(NL80211_IFTYPE_AP),
		.he_cap = WLAN_HE_CAP_INFO,
#if KERNEL_VERSION(5, 8, 0) <= CFG80211_VERSION_CODE
		.he_6ghz_capa = WLAN_HE_6G_CAP_INFO,
#endif
#if (CFG_SUPPORT_802_11BE == 1)
#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE))
		.eht_cap = WLAN_EHT_CAP_INFO,
#endif
#endif
	},
	{
		.types_mask =
			BIT(NL80211_IFTYPE_STATION),
		.he_cap = WLAN_HE_CAP_INFO,
#if KERNEL_VERSION(5, 8, 0) <= CFG80211_VERSION_CODE
		.he_6ghz_capa = WLAN_HE_6G_CAP_INFO,
#endif
#if (CFG_SUPPORT_802_11BE == 1)
#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE))
		.eht_cap = WLAN_EHT_CAP_INFO,
#endif
#endif
	},
};

#endif /* (CFG_SUPPORT_802_11AX == 1) && (CFG_SUPPORT_WIFI_6G == 1) */
#endif

/* public for both Legacy Wi-Fi / P2P access */
struct ieee80211_supported_band mtk_band_2ghz = {
	.band = KAL_BAND_2GHZ,
	.channels = mtk_2ghz_channels,
	.n_channels = ARRAY_SIZE(mtk_2ghz_channels),
	.bitrates = mtk_g_rates,
	.n_bitrates = mtk_g_rates_size,
	.ht_cap = WLAN_HT_CAP,
#if KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
#if (CFG_SUPPORT_802_11AX == 1)
	.n_iftype_data = ARRAY_SIZE(mtk_cap),
	.iftype_data = mtk_cap,
#endif
#endif
};

/* public for both Legacy Wi-Fi / P2P access */
struct ieee80211_supported_band mtk_band_5ghz = {
	.band = KAL_BAND_5GHZ,
	.channels = mtk_5ghz_channels,
	.n_channels = ARRAY_SIZE(mtk_5ghz_channels),
	.bitrates = mtk_a_rates,
	.n_bitrates = mtk_a_rates_size,
	.ht_cap = WLAN_HT_CAP,
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	.vht_cap = WLAN_VHT_CAP_160,
#else
	.vht_cap = WLAN_VHT_CAP,
#endif
#if KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
#if (CFG_SUPPORT_802_11AX == 1)
	.n_iftype_data = ARRAY_SIZE(mtk_cap),
	.iftype_data = mtk_cap,
#endif
#endif
};

/* public for both Legacy Wi-Fi / P2P access */
#if (CFG_SUPPORT_WIFI_6G == 1)
struct ieee80211_supported_band mtk_band_6ghz = {
	.band = KAL_BAND_6GHZ,
	.channels = mtk_6ghz_channels,
	.n_channels = ARRAY_SIZE(mtk_6ghz_channels),
	.bitrates = mtk_a_rates,
	.n_bitrates = mtk_a_rates_size,
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#if (CFG_SUPPORT_802_11AX == 1)
	.n_iftype_data = ARRAY_SIZE(mtk_cap_6g),
	.iftype_data = mtk_cap_6g,
#endif
#endif
};
#endif

const uint32_t mtk_cipher_suites[] = {
	/* keep WEP first, it may be removed below */
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
#if CFG_SUPPORT_WAPI
	WLAN_CIPHER_SUITE_SMS4,
#endif
	/* keep last -- depends on hw flags! */
	WLAN_CIPHER_SUITE_AES_CMAC,
#if (CFG_SUPPORT_CONNAC2X == 1 || CFG_SUPPORT_CONNAC3X == 1)
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
	WLAN_CIPHER_SUITE_GCMP_256,
	WLAN_CIPHER_SUITE_BIP_GMAC_256,
#endif
#endif
	WLAN_CIPHER_SUITE_GCMP,
	WLAN_CIPHER_SUITE_NO_GROUP_ADDR
};

static struct cfg80211_ops mtk_cfg_ops = {
	.add_virtual_intf = mtk_cfg_add_iface,
	.del_virtual_intf = mtk_cfg_del_iface,
	.change_virtual_intf = mtk_cfg_change_iface,
#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
	.add_intf_link = mtk_cfg_add_intf_link,
	.del_intf_link = mtk_cfg_del_intf_link,
#endif
	.add_key = mtk_cfg_add_key,
	.get_key = mtk_cfg_get_key,
	.del_key = mtk_cfg_del_key,
	.set_default_mgmt_key = mtk_cfg_set_default_mgmt_key,
#if (CFG_SUPPORT_BCN_PROT == 1) && \
	(KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE)
	.set_default_beacon_key = mtk_cfg_set_default_beacon_key,
#endif
	.set_default_key = mtk_cfg_set_default_key,
	.get_station = mtk_cfg_get_station,
#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	.get_channel = mtk_cfg_get_channel,
#endif
#if CFG_SUPPORT_TDLS
	.change_station = mtk_cfg_change_station,
	.add_station = mtk_cfg_add_station,
	.tdls_oper = mtk_cfg_tdls_oper,
	.tdls_mgmt = mtk_cfg_tdls_mgmt,
#if CFG_SUPPORT_TDLS_OFFCHANNEL
	.tdls_channel_switch = mtk_tdls_channel_switch,
	.tdls_cancel_channel_switch = mtk_tdls_cancel_channel_switch,
#endif
#endif
	.del_station = mtk_cfg_del_station,	/* AP/P2P use this function */
	.scan = mtk_cfg_scan,
#if KERNEL_VERSION(4, 5, 0) <= CFG80211_VERSION_CODE
	.abort_scan = mtk_cfg_abort_scan,
#endif
#if CFG_SUPPORT_SCHED_SCAN
	.sched_scan_start = mtk_cfg_sched_scan_start,
	.sched_scan_stop = mtk_cfg_sched_scan_stop,
#endif /* CFG_SUPPORT_SCHED_SCAN */

	.connect = mtk_cfg_connect,
#if (CFG_SUPPORT_ROAMING == 1)
	.update_connect_params = mtk_cfg_update_connect_params,
#endif /* CFG_SUPPORT_ROAMING */
	.disconnect = mtk_cfg_disconnect,
	.join_ibss = mtk_cfg_join_ibss,
	.leave_ibss = mtk_cfg_leave_ibss,
	.set_power_mgmt = mtk_cfg_set_power_mgmt,
	.set_pmksa = mtk_cfg_set_pmksa,
	.del_pmksa = mtk_cfg_del_pmksa,
	.flush_pmksa = mtk_cfg_flush_pmksa,
#if CONFIG_SUPPORT_GTK_REKEY
	.set_rekey_data = mtk_cfg_set_rekey_data,
#endif
	.suspend = mtk_cfg_suspend,
	.resume = mtk_cfg_resume,

	.assoc = mtk_cfg_assoc,

	/* Action Frame TX/RX */
	.remain_on_channel = mtk_cfg_remain_on_channel,
	.cancel_remain_on_channel = mtk_cfg_cancel_remain_on_channel,
	.mgmt_tx = mtk_cfg_mgmt_tx,
	/* .mgmt_tx_cancel_wait        = mtk_cfg80211_mgmt_tx_cancel_wait, */
#if KERNEL_VERSION(5, 8, 0) > CFG80211_VERSION_CODE
	.mgmt_frame_register = mtk_cfg_mgmt_frame_register,
#endif
#if KERNEL_VERSION(5, 8, 0) <= CFG80211_VERSION_CODE
	.update_mgmt_frame_registrations = mtk_cfg_mgmt_frame_update,
#endif

#ifdef CONFIG_NL80211_TESTMODE
	.testmode_cmd = mtk_cfg_testmode_cmd,
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
	.start_radar_detection = mtk_cfg_start_radar_detection,
#if KERNEL_VERSION(3, 13, 0) <= CFG80211_VERSION_CODE
	.channel_switch = mtk_cfg_channel_switch,
#endif
#endif

#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211 != 0)
	.change_bss = mtk_cfg_change_bss,
	.mgmt_tx_cancel_wait = mtk_cfg_mgmt_tx_cancel_wait,
	.deauth = mtk_cfg_deauth,
	.disassoc = mtk_cfg_disassoc,
	.start_ap = mtk_cfg_start_ap,
	.change_beacon = mtk_cfg_change_beacon,
	.stop_ap = mtk_cfg_stop_ap,
	.set_wiphy_params = mtk_cfg_set_wiphy_params,
	.set_bitrate_mask = mtk_cfg_set_bitrate_mask,
	.set_tx_power = mtk_cfg_set_txpower,
	.get_tx_power = mtk_cfg_get_txpower,
#endif
	.update_ft_ies = mtk_cfg80211_update_ft_ies,
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	.set_monitor_channel = mtk_cfg80211_set_monitor_channel,
#endif
#if CFG_SUPPORT_WPA3
	.external_auth = mtk_cfg80211_external_auth,
#endif
#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_CONTROL_PORT_OVER_NL80211 == 1)
	.tx_control_port = mtk_cfg80211_tx_control_port,
#endif
};

#if KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE

static const struct wiphy_vendor_command
	mtk_wlan_vendor_ops[] = {
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_GET_CHANNEL_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_channel_list
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_wifi_attribute,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_COUNTRY_CODE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_country_code
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_wifi_attribute,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_PNO_RANDOM_MAC_OUI
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
			| WIPHY_VENDOR_CMD_NEED_NETDEV
			| WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_set_scan_mac_oui
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_wifi_attribute,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = QCA_NL80211_VENDOR_SUBCMD_SETBAND
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_band
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = qca_wlan_vendor_attr_policy,
		.maxattr = QCA_WLAN_VENDOR_ATTR_MAX
#endif
	},
#if CFG_SUPPORT_MBO
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = QCA_NL80211_VENDOR_SUBCMD_ROAM
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_roaming_param
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = qca_roaming_param_policy,
		.maxattr = QCA_ATTR_ROAMING_PARAM_MAX
#endif

	},
#endif
#if CFG_SUPPORT_ROAMING
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = QCA_NL80211_VENDOR_SUBCMD_ROAMING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_roaming_policy
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = qca_wlan_vendor_attr_policy,
		.maxattr = QCA_WLAN_VENDOR_ATTR_MAX
#endif
	},
#endif
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_GET_ROAMING_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_roaming_capabilities
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_CONFIG_ROAMING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_config_roaming
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_wifi_attribute,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_ENABLE_ROAMING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_enable_roaming
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_wifi_attribute,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
#if CFG_SUPPORT_RTT
	/* RTT */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = RTT_SUBCMD_GETCAPABILITY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_rtt_capabilities
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = RTT_SUBCMD_SET_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_rtt_config
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_set_rtt_config_policy,
		.maxattr = RTT_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = RTT_SUBCMD_CANCEL_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_cancel_rtt_config
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_set_rtt_config_policy,
		.maxattr = RTT_ATTRIBUTE_MAX
#endif
	},
#endif /* CFG_SUPPORT_RTT */
	/* Link Layer Statistics */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = LSTATS_SUBCMD_GET_INFO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
		WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_llstats_get_info
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
	/* RSSI Monitoring */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_RSSI_MONITOR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
		WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_rssi_monitoring
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_wifi_rssi_monitor,
		.maxattr = WIFI_ATTRIBUTE_RSSI_MONITOR_ATTRIBUTE_MAX
#endif
	},
	/* Packet Keep Alive */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_OFFLOAD_START_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
		WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_packet_keep_alive_start
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_offloading_policy,
		.maxattr = MKEEP_ALIVE_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_OFFLOAD_STOP_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
		WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_packet_keep_alive_stop
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_offloading_policy,
		.maxattr = MKEEP_ALIVE_ATTRIBUTE_MAX
#endif
	},
	/* Get Driver Version or Firmware Version */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = LOGGER_GET_VER
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_version
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_get_version_policy,
		.maxattr = LOGGER_ATTRIBUTE_MAX
#endif
	},
	/* Get Supported Feature Set */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_GET_FEATURE_SET
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_supported_feature_set
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif

	},
	/* Set dual STA use cases */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_MULTISTA_USE_CASE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_multista_use_case
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		,
		.policy = nla_parse_wifi_multista,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
	/* Select primary connection */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_MULTISTA_PRIMARY_CONNECTION
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_multista_primary_connection
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		,
		.policy = nla_parse_wifi_multista,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
	/* Set Tx Power Scenario */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SELECT_TX_POWER_SCENARIO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_tx_power_scenario
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_parse_wifi_attribute,
		.maxattr = WIFI_ATTRIBUTE_MAX
#endif
	},
#if CFG_SUPPORT_P2P_PREFERRED_FREQ_LIST
	/* P2P get preferred freq list */
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = NL80211_VENDOR_SUBCMD_GET_PREFER_FREQ_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
				| WIPHY_VENDOR_CMD_NEED_NETDEV
				| WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_get_preferred_freq_list
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_get_preferred_freq_list_policy,
		.maxattr = WIFI_VENDOR_ATTR_PREFERRED_FREQ_LIST_MAX
#endif
	},
#endif /* CFG_SUPPORT_P2P_PREFERRED_FREQ_LIST */
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = NL80211_VENDOR_SUBCMD_ACS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
				| WIPHY_VENDOR_CMD_NEED_NETDEV
				| WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_acs
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_get_acs_policy,
		.maxattr = WIFI_VENDOR_ATTR_ACS_MAX
#endif
	},
#endif
#if CFG_SUPPORT_DFS_MASTER
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = NL80211_VENDOR_SUBCMD_DFS_CAPABILITY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
				| WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_dfs_capability
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
#endif
	{
		{
			.vendor_id = OUI_QCA,
			.subcmd = NL80211_VENDOR_SUBCMD_GET_FEATURES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
				| WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_features
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif

	},
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd =  NL80211_VENDOR_SUBCMD_GET_APF_CAPABILITIES
		},
			.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				 WIPHY_VENDOR_CMD_NEED_NETDEV,
			.doit = mtk_cfg80211_vendor_get_apf_capabilities
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
			,
			.policy = nla_get_apf_policy,
			.maxattr = APF_ATTRIBUTE_MAX
#endif
	},
#if (CFG_SUPPORT_APF == 1)
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = NL80211_VENDOR_SUBCMD_SET_PACKET_FILTER
		},
			.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				 WIPHY_VENDOR_CMD_NEED_NETDEV,
			.doit = mtk_cfg80211_vendor_set_packet_filter
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
			,
			.policy = nla_get_apf_policy,
			.maxattr = APF_ATTRIBUTE_MAX
#endif
	},
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = NL80211_VENDOR_SUBCMD_READ_PACKET_FILTER
		},
			.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
			.doit = mtk_cfg80211_vendor_read_packet_filter
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_get_apf_policy,
		.maxattr = APF_ATTRIBUTE_MAX
#endif
	},
#endif /* CFG_SUPPORT_APF */
	/* Get Driver Memory Dump */
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = LOGGER_DRIVER_MEM_DUMP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_driver_memory_dump
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = WIFI_SUBCMD_SET_SCAN_PARAM
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
			| WIPHY_VENDOR_CMD_NEED_NETDEV
			| WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_set_scan_param
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = mtk_scan_param_policy,
		.maxattr = WIFI_ATTR_SCAN_MAX
#endif
	},
#if CFG_SUPPORT_NAN
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_NAN
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV |
				WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_nan
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_NDP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV |
				WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_ndp
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		,
		.policy = mtk_wlan_vendor_ndp_policy,
		.maxattr = MTK_WLAN_VENDOR_ATTR_NDP_PARAMS_MAX
#endif
	},
#endif
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_STRING_CMD
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV |
				WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_string_cmd
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		,
		.policy = nla_string_cmd_policy,
		.maxattr = STRING_ATTRIBUTE_MAX
#endif
	},
#if (CFG_SUPPORT_STATISTICS == 1)
	/* Get Trx Stats */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = NL80211_VENDOR_SUBCMD_GET_TRX_STATS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_trx_stats
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_trx_stats_policy,
		.maxattr = WIFI_ATTRIBUTE_STATS_MAX
#endif
	},
#endif
	/* Get Wifi Reset */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_TRIGGER_RESET
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_trigger_reset
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
	/* P2P GO Set DFS channel */
#if CFG_MTK_WIFI_SUPPORT_STA_DFS_CHANNEL
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_CHANNEL_POLICY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV |
				WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_enable_sta_channel_for_peer_network
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
		,
		.policy = mtk_enable_sta_channel_for_peer_network_policy,
		.maxattr = WIFI_ATTRIBUTE_ENABLE_STA_CHANNEL_FOR_P2P_MAX
#endif
	},
#endif /* CFG_MTK_WIFI_SUPPORT_STA_DFS_CHANNEL */
#if CFG_SUPPORT_CSI
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_CSI
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV |
				WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_csi_control
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_get_csi_policy,
		.maxattr = WIFI_ATTRIBUTE_CSI_MAX
#endif
	},
#endif
	/* Comb Matrix Support */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_GET_RADIO_COMBO_MATRIX
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_comb_matrix
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = VENDOR_CMD_RAW_DATA
#endif
	},
	/* Usable Channel Support */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_GET_USABLE_CHANNEL
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
					WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_usable_channel
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = mtk_usable_channel_policy,
		.maxattr = WIFI_ATTRIBUTE_USABLE_CHANNEL_MAX
#endif
	},
#if CFG_SUPPORT_P2P_LISTEN_OFFLOAD
	{
		{
		.vendor_id = OUI_QCA,
		.subcmd = QCA_NL80211_VENDOR_SUBCMD_P2P_LISTEN_OFFLOAD_START
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
				| WIPHY_VENDOR_CMD_NEED_NETDEV
				| WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_p2p_listen_offload_start
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_p2p_listen_offload_policy,
		.maxattr = QCA_WLAN_VENDOR_ATTR_P2P_LO_MAX
#endif
	},
	{
		{
		.vendor_id = OUI_QCA,
		.subcmd = QCA_NL80211_VENDOR_SUBCMD_P2P_LISTEN_OFFLOAD_STOP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV
				| WIPHY_VENDOR_CMD_NEED_NETDEV
				| WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mtk_cfg80211_vendor_p2p_listen_offload_stop
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = nla_p2p_listen_offload_policy,
		.maxattr = QCA_WLAN_VENDOR_ATTR_P2P_LO_MAX
#endif
	},
#endif /* CFG_SUPPORT_P2P_LISTEN_OFFLOAD */
	/* Set Tx Latency Monitor Parameter */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = NL80211_VENDOR_SUBCMD_SET_TX_LAT_MONTR_PARAM
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_tx_lat_montr_param,
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		.policy = mtk_tx_lat_montr_param_policy,
		.maxattr = WIFI_ATTR_TX_LAT_MONTR_MAX
#endif
	},
	/* Set WFD Tx Bitrate Monitor */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = NL80211_VENDOR_SUBCMD_SET_WFD_TX_BR_MONTR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_wfd_tx_br_montr,
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		.policy = mtk_wfd_tx_br_montr_policy,
		.maxattr = WIFI_ATTR_WFD_TX_BR_MONTR_MAX
#endif
	},
	/* Get WFD Predicted Tx Bitrate  */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = NL80211_VENDOR_SUBCMD_GET_WFD_PRED_TX_BR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_wfd_pred_tx_br,
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		.policy = VENDOR_CMD_RAW_DATA,
#endif
	},
	/* Get Chip Capabilities From Wifi Driver */
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_GET_CHIP_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_chip_capabilities,
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		.policy = VENDOR_CMD_RAW_DATA,
#endif
	},
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_GET_CHIP_CONCURRENCY_MATRIX
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_chip_concurrency_matrix,
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		.policy = VENDOR_CMD_RAW_DATA,
#endif
	},
#if CFG_SUPPORT_WIFI_ADJUST_DTIM
	{
		{
			.vendor_id = OUI_MTK,
			.subcmd = MTK_SUBCMD_SET_CHIP_DTIM_PERIOD
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
				WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_dtim_param
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		,
		.policy = mtk_set_dtim_param_policy,
		.maxattr = WIFI_ATTR_SET_DTIM_MAX
#endif
	},
#endif
};

static const struct nl80211_vendor_cmd_info
	mtk_wlan_vendor_events[] = {
	[GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS
	},
	[GSCAN_EVENT_HOTLIST_RESULTS_FOUND] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_HOTLIST_RESULTS_FOUND
	},
	[GSCAN_EVENT_SCAN_RESULTS_AVAILABLE] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_SCAN_RESULTS_AVAILABLE
	},
	[GSCAN_EVENT_FULL_SCAN_RESULTS] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_FULL_SCAN_RESULTS
	},
#if CFG_SUPPORT_RTT
	[RTT_EVENT_COMPLETE] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = RTT_EVENT_COMPLETE
	},
#endif
	[GSCAN_EVENT_COMPLETE_SCAN] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_COMPLETE_SCAN
	},
	[GSCAN_EVENT_HOTLIST_RESULTS_LOST] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = GSCAN_EVENT_HOTLIST_RESULTS_LOST
	},
	[WIFI_EVENT_RSSI_MONITOR] {
		.vendor_id = GOOGLE_OUI,
		.subcmd = WIFI_EVENT_RSSI_MONITOR
	},
#if CFG_SUPPORT_DATA_STALL
	[WIFI_EVENT_DRIVER_ERROR] {
		.vendor_id = OUI_MTK,
		.subcmd = WIFI_EVENT_DRIVER_ERROR
	},
#endif
#if CFG_AUTO_CHANNEL_SEL_SUPPORT
	[WIFI_EVENT_ACS] {
		.vendor_id = OUI_QCA,
		.subcmd = NL80211_VENDOR_SUBCMD_ACS
	},
#endif

	[WIFI_EVENT_GENERIC_RESPONSE] {
		.vendor_id = OUI_MTK,
		.subcmd = WIFI_EVENT_GENERIC_RESPONSE
	},

#if CFG_SUPPORT_BIGDATA_PIP
	[WIFI_EVENT_BIGDATA_PIP] {
		.vendor_id = OUI_MTK,
		.subcmd = WIFI_EVENT_BIGDATA_PIP
	},
#endif

#if CFG_SUPPORT_DBDC
	[WIFI_EVENT_OP_MODE_CHANGE] {
		.vendor_id = OUI_MTK,
		.subcmd = MTK_NL80211_OP_MODE_CHANGE
	},
#endif

	[WIFI_EVENT_DFS_OFFLOAD_CAC_STARTED] {
		.vendor_id = OUI_QCA,
		.subcmd = NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_STARTED
	},

	[WIFI_EVENT_DFS_OFFLOAD_CAC_FINISHED] {
		.vendor_id = OUI_QCA,
		.subcmd = NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_FINISHED
	},

	[WIFI_EVENT_DFS_OFFLOAD_CAC_ABORTED] {
		.vendor_id = OUI_QCA,
		.subcmd = NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_ABORTED
	},

	[WIFI_EVENT_DFS_OFFLOAD_CAC_NOP_FINISHED] {
		.vendor_id = OUI_QCA,
		.subcmd = NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_CAC_NOP_FINISHED
	},

	[WIFI_EVENT_DFS_OFFLOAD_RADAR_DETECTED] {
		.vendor_id = OUI_QCA,
		.subcmd = NL80211_VENDOR_SUBCMD_DFS_OFFLOAD_RADAR_DETECTED
	},
	[WIFI_EVENT_RESET_TRIGGERED] {
		.vendor_id = OUI_MTK,
		.subcmd = MTK_NL80211_TRIGGER_RESET
	},
	[WIFI_EVENT_SUBCMD_NAN] {
		.vendor_id = OUI_MTK,
		.subcmd = MTK_SUBCMD_NAN
	},
	[WIFI_EVENT_SUBCMD_NDP] {
		.vendor_id = OUI_MTK,
		.subcmd = MTK_SUBCMD_NDP
	},
	[WIFI_EVENT_SUBCMD_CSI] {
		.vendor_id = OUI_MTK,
		.subcmd = MTK_SUBCMD_CSI
	},
	[WIFI_EVENT_P2P_LISTEN_OFFLOAD] = {
		.vendor_id = OUI_QCA,
		.subcmd = QCA_NL80211_VENDOR_SUBCMD_P2P_LISTEN_OFFLOAD_STOP
	},
};
#endif

/* There isn't a lot of sense in it, but you can transmit anything you like */
static const struct ieee80211_txrx_stypes
	mtk_cfg80211_ais_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		      BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		      BIT(IEEE80211_STYPE_AUTH >> 4)
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
			| BIT(IEEE80211_STYPE_ACTION >> 4)
#if CFG_SUPPORT_SOFTAP_WPA3
			| BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			  BIT(IEEE80211_STYPE_AUTH >> 4) |
			  BIT(IEEE80211_STYPE_DEAUTH >> 4)
#endif
	},
	[NL80211_IFTYPE_AP_VLAN] = {
		/* copy AP */
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		      BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		      BIT(IEEE80211_STYPE_AUTH >> 4)
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		      BIT(IEEE80211_STYPE_ACTION >> 4)
#if CFG_SUPPORT_SOFTAP_WPA3
			| BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			  BIT(IEEE80211_STYPE_AUTH >> 4) |
			  BIT(IEEE80211_STYPE_DEAUTH >> 4)
#endif
	}
};

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support mtk_wlan_wowlan_support = {
	.flags = WIPHY_WOWLAN_MAGIC_PKT | WIPHY_WOWLAN_DISCONNECT |
		 WIPHY_WOWLAN_ANY,
};
#endif

#if CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT
/* NL80211_FEATURE_DS_PARAM_SET_IE_IN_PROBES & NL80211_FEATURE_QUIET
 * support in linux kernet version => 3.18
 */
#if KERNEL_VERSION(3, 18, 0) > CFG80211_VERSION_CODE
#define NL80211_FEATURE_DS_PARAM_SET_IE_IN_PROBES BIT(19)
#define NL80211_FEATURE_QUIET BIT(21)
#endif
#endif

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static void wlanRemove(void);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if 0
/*----------------------------------------------------------------------------*/
/*!
 * \brief Override the implementation of select queue
 *
 * \param[in] dev Pointer to struct net_device
 * \param[in] skb Pointer to struct skb_buff
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
unsigned int _cfg80211_classify8021d(struct sk_buff *skb)
{
	unsigned int dscp = 0;

	/* skb->priority values from 256->263 are magic values
	 * directly indicate a specific 802.1d priority.  This is
	 * to allow 802.1d priority to be passed directly in from
	 * tags
	 */

	if (skb->priority >= 256 && skb->priority <= 263)
		return skb->priority - 256;
	switch (skb->protocol) {
	case htons(ETH_P_IP):
		dscp = ip_hdr(skb)->tos & 0xfc;
		break;
	}
	return dscp >> 5;
}
#endif

u_int8_t __is_critical_packet(struct net_device *dev)
{
	bool is_critical = FALSE;
#if ARP_MONITER_ENABLE
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct GLUE_INFO *prGlueInfo;
	uint8_t ucBssIndex;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		kalGetNetDevPriv(dev);
	if (!prNetDevPrivate)
		goto end;

	prGlueInfo = prNetDevPrivate->prGlueInfo;
	if (!prGlueInfo || !prGlueInfo->prAdapter)
		goto end;

	ucBssIndex = prNetDevPrivate->ucBssIdx;
	if (arpMonIsCritical(prGlueInfo->prAdapter, ucBssIndex))
		is_critical = TRUE;

end:
#endif /* ARP_MONITER_ENABLE */
	return is_critical;
}

static bool is_critical_packet(struct net_device *dev,
	struct sk_buff *skb, u16 orig_queue_index)
{
#if CFG_CHANGE_CRITICAL_PACKET_PRIORITY
	uint8_t *pucPkt;
	uint16_t u2EtherType;
	bool is_critical = FALSE;

	if (!skb)
		return FALSE;

	pucPkt = skb->data;
	u2EtherType = pucPkt[ETH_TYPE_LEN_OFFSET] << 8 |
		      pucPkt[ETH_TYPE_LEN_OFFSET + 1];

	switch (u2EtherType) {
	case ETH_P_ARP:
		if (__netif_subqueue_stopped(dev, orig_queue_index))
			is_critical = TRUE;

		if (__is_critical_packet(dev))
			is_critical = TRUE;
		break;
	case ETH_P_1X:
	case ETH_P_PRE_1X:
#if CFG_SUPPORT_WAPI
	case ETH_WPI_1X:
#endif
		is_critical = TRUE;
		break;
	default:
		is_critical = FALSE;
		break;
	}
	return is_critical;
#else
	return FALSE;
#endif
}

#if CFG_SUPPORT_MANIPULATE_TID
static bool need_manipulate_priority_for_udp(
	struct sk_buff *skb,
	uint8_t *pucUserPriority)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint8_t ucUserPriority;
	uint16_t u2EtherType = 0;
	struct iphdr *iph = NULL;
	struct ipv6hdr *ipv6h = NULL;
	if (!skb)
		return FALSE;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(skb->dev));

	if (unlikely(!prGlueInfo || kalIsHalted()))
		return FALSE;

	prAdapter = prGlueInfo->prAdapter;
	if (unlikely(!prAdapter) ||
	    !prAdapter->rManipulateTidInfo.fgManipulateTidEnabled)
		return FALSE;

	ucUserPriority = prAdapter->rManipulateTidInfo.ucUserPriority;

	u2EtherType = NTOHS(skb_eth_hdr(skb)->h_proto);

	DBGLOG(TX, TEMP, "Ethertype = [%d]\n", u2EtherType);

	if (u2EtherType == ETH_P_IPV4) {
		iph = ip_hdr(skb);

		DBGLOG(TX, TEMP, "IPv4 protocol = [%d]\n", iph->protocol);

		if (iph->protocol == IPPROTO_UDP) {
			*pucUserPriority = ucUserPriority;
			return TRUE;
		}
	} else if (u2EtherType == ETH_P_IPV6) {
		ipv6h = ipv6_hdr(skb);

		DBGLOG(TX, TEMP, "IPv6 protocol = [%d]\n", ipv6h->nexthdr);

		if (ipv6h->nexthdr == IPPROTO_UDP) {
			*pucUserPriority = ucUserPriority;
			return TRUE;
		}
	}

	return FALSE;
}
#endif

static struct cfg80211_qos_map *get_qos_map(struct net_device *dev)
{
	struct cfg80211_qos_map *qos_map = &default_qos_map;

	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t ucBssIdx;

	_Static_assert(sizeof(struct cfg80211_qos_map) ==
			sizeof(struct QOS_MAP),
			"sizeof(cfg80211_qos_map) != sizeof(QOS_MAP)");
	_Static_assert(offsetof(struct cfg80211_qos_map, num_des) ==
			offsetof(struct QOS_MAP, ucDscpExNum),
			"offset(num_des) != offset(ucDscpExNuQOS_MAP)");
	_Static_assert(offsetof(struct cfg80211_qos_map, dscp_exception) ==
			offsetof(struct QOS_MAP, arDscpException),
			"offset(dscp_exception) != offset(arDscpException)");
	_Static_assert(offsetof(struct cfg80211_qos_map, up) ==
			offsetof(struct QOS_MAP, arDscpRange),
			"offset(up) != offset (arDscpRange)");

	do {
		prGlueInfo = *((struct GLUE_INFO **)netdev_priv(dev));
		if (unlikely(!prGlueInfo))
			break;

		if (unlikely(kalIsHalted())) {
			DBGLOG(TX, WARN, "Driver is not ready\n");
			break;
		}

		prAdapter = prGlueInfo->prAdapter;
		if (unlikely(!prAdapter))
			break;

		ucBssIdx = wlanGetBssIdx(dev);

		if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIdx)) {
			/**
			 * In WMMPS test case DUT as AP, the PC endpoint sends
			 * VO packets with TOS as 0xD0 and verify the DL frame.
			 * If apply QoS Map, 0xD0 will be mapped to BE, since in
			 * default QoS Map, VO could only be 0xC0 or 0xE0.
			 * Current solution limits QoS Map as STA only.
			 * Passing a NULL pointer to cfg80211_classify8021d()
			 * in Linux, it will map TOS to UP by the 3-bit MSBs.
			 */
			if (prAdapter->rWifiVar.fgApLegacyQosMap)
				qos_map = NULL;
			break;
		}

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
		if (unlikely(!prBssInfo))
			break;

		prStaRec = prBssInfo->prStaRecOfAP;
		if (unlikely(!prStaRec))
			break;

		qos_map = (struct cfg80211_qos_map *)&prStaRec->rQosMap;
	} while (0);

	DBGLOG(TX, TEMP, "return %s qos_map %p\n",
	       prStaRec ? "STA" : "default", qos_map);
	return qos_map;
}

static inline u16 kernel_ndev_select_queue(
	struct net_device *dev,
	struct sk_buff *skb,
	void *fallback)
{
	u16 queue_index = 0;

#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE
	queue_index = netdev_pick_tx(dev, skb, NULL);
#elif KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
	select_queue_fallback_t select_queue = fallback;

	queue_index = select_queue(dev, skb, NULL);
#elif KERNEL_VERSION(3, 14, 0) <= LINUX_VERSION_CODE
	select_queue_fallback_t select_queue = fallback;

	queue_index = select_queue(dev, skb);
#else
	queue_index = __netdev_pick_tx(dev, skb);
#endif
	return queue_index;
}

/* The netdev provides multiple TX queues.
 * This function returns the TX AC queue as the mapping sequence:
 * DSCP in IP header (0..63) => User Priority (0..7) (~ TID) => AC queue
 * NOTE: TID (4-bit) = 0(MSB) + User Priority (3-bit)
 *
 * The DSCP to UP mapping is determined by a QoS Map with default values
 * defined in standard or updated from QoS Map Action frame announced from AP.
 * skb->priority is updated in this function.
 */
static inline u16 mtk_wlan_ndev_select_queue(struct net_device *dev,
	struct sk_buff *skb, void *fallback)
{
#if !CFG_KERNEL_MAPPING_TXQ
	static const u16 ieee8021d_to_queue[8] = {
		ACI_BK, ACI_BE, ACI_BE, ACI_BK, ACI_VI, ACI_VI, ACI_VO, ACI_VO};
#endif
	u16 queue_index = 0;
	struct cfg80211_qos_map *qos_map = NULL;
#if CFG_SUPPORT_MANIPULATE_TID
	uint8_t ucUserPriority = 0;

	if (need_manipulate_priority_for_udp(skb, &ucUserPriority)) {
		skb->priority = ucUserPriority;
		return ieee8021d_to_queue[skb->priority];
	}
#endif
	qos_map = get_qos_map(dev);

	/* cfg80211_classify8021d returns 0~7 */
#if KERNEL_VERSION(3, 14, 0) > CFG80211_VERSION_CODE
	skb->priority = cfg80211_classify8021d(skb);
#else
	skb->priority = cfg80211_classify8021d(skb, qos_map);
#endif

#if CFG_KERNEL_MAPPING_TXQ
	queue_index = kernel_ndev_select_queue(dev, skb, fallback);
#else
	queue_index = ieee8021d_to_queue[skb->priority];
#endif

	if (is_critical_packet(dev, skb, queue_index)) {
		skb->priority = WMM_UP_VO_INDEX;
#if CFG_KERNEL_MAPPING_TXQ
		queue_index = kernel_ndev_select_queue(dev, skb, fallback);
#else
		queue_index = ieee8021d_to_queue[skb->priority];
#endif
	}

	return queue_index;
}

#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb,
		    struct net_device *sb_dev)
{
	return mtk_wlan_ndev_select_queue(dev, skb, NULL);
}
#elif KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb,
		    struct net_device *sb_dev, select_queue_fallback_t fallback)
{
	return mtk_wlan_ndev_select_queue(dev, skb, fallback);
}
#elif KERNEL_VERSION(3, 14, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb,
		    void *accel_priv, select_queue_fallback_t fallback)
{
	return mtk_wlan_ndev_select_queue(dev, skb, fallback);
}
#elif KERNEL_VERSION(3, 13, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb,
		    void *accel_priv)
{
	return mtk_wlan_ndev_select_queue(dev, skb, NULL);
}
#else
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb)
{
	return mtk_wlan_ndev_select_queue(dev, skb, NULL);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Load NVRAM data and translate it into REG_INFO_T
 *
 * \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
 * \param[out] prRegInfo  Pointer to struct REG_INFO_T
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void glLoadNvram(struct GLUE_INFO *prGlueInfo,
	struct REG_INFO *prRegInfo)
{
	struct WIFI_CFG_PARAM_STRUCT *prNvramSettings;
	const struct firmware *fw = NULL;
	int err = 0;

	ASSERT(prRegInfo);
	ASSERT(prGlueInfo);

	/*
	* In Linux scenario, bring nvram by insmod
	* If there is both exit nvram in wmtWifi & insmod,
	* it will apply insomd nvram
	*/
	if (gprifnamenvram != NULL) {
		err = request_firmware(&fw, gprifnamenvram, prGlueInfo->prDev);

		if (!err) {
			DBGLOG(INIT, INFO,
				"Find nvram file: %s by insmod data:0x%p,size:%lu\n",
				gprifnamenvram,
				fw->data,
				fw->size);

			/*
			*  No need to handle CRC
			*  WlanLoadManufactureData will parse nvram by TLV
			*/
			if (fw->size > 0 && fw->size <= sizeof(g_aucNvram)) {
				kalMemCopy(g_aucNvram, fw->data, fw->size);
				g_NvramFsm = NVRAM_STATE_READY;
				DBGLOG(INIT, INFO, "Set NVRAM state[%d]\n",
					g_NvramFsm);
#if CFG_MTK_ANDROID_WMT
				if (!g_IsPlatCbsRegistered) {
					register_plat_connsys_cbs();
					g_IsPlatCbsRegistered = TRUE;
				}
#endif
			}
			release_firmware(fw);
		}
	}

	DBGLOG(INIT, INFO, "g_NvramFsm = %d\n", g_NvramFsm);
	if (g_NvramFsm != NVRAM_STATE_READY) {
		DBGLOG(INIT, WARN, "Nvram not available\n");
		return;
	}

	if (sizeof(struct WIFI_CFG_PARAM_STRUCT) >
					sizeof(g_aucNvram)) {
		DBGLOG(INIT, ERROR,
		"Size WIFI_CFG_PARAM_STRUCT %zu > size aucNvram %zu\n"
		, sizeof(struct WIFI_CFG_PARAM_STRUCT),
		sizeof(g_aucNvram));
		return;
	}

	prGlueInfo->fgNvramAvailable = TRUE;
	prRegInfo->prNvramSettings =
		(struct WIFI_CFG_PARAM_STRUCT *)&g_aucNvram[0];
	prNvramSettings = prRegInfo->prNvramSettings;

#if CFG_SUPPORT_XONVRAM
	if (gprifnamexonv != NULL) {
		err = request_firmware(&fw, gprifnamexonv, prGlueInfo->prDev);
		if (!err) {
			DBGLOG(INIT, INFO,
				"Find xo nvram : %s by insmod data:0x%p,size:%lu\n",
				gprifnamexonv,
				fw->data,
				fw->size);

			if ((fw->size > 0)
				&& (fw->size <= sizeof(g_rXonvCfg.aucData))) {
				kalMemCopy(g_rXonvCfg.aucData,
					fw->data, fw->size);
			}
			release_firmware(fw);
		}
	}
	prRegInfo->prXonvCfg = &g_rXonvCfg;
#endif

#if CFG_TC1_FEATURE
		TC1_FAC_NAME(FacReadWifiMacAddr)(prRegInfo->aucMacAddr);
		DBGLOG(INIT, INFO,
			"MAC address: " MACSTR, MAC2STR(prRegInfo->aucMacAddr));
#else
	/* load MAC Address */
	kalMemCopy(prRegInfo->aucMacAddr,
			prNvramSettings->aucMacAddress,
			PARAM_MAC_ADDR_LEN*sizeof(uint8_t));
#endif
		/* load country code */
		/* cast to wide characters */
		prRegInfo->au2CountryCode[0] =
			(uint16_t) prNvramSettings->aucCountryCode[0];
		prRegInfo->au2CountryCode[1] =
			(uint16_t) prNvramSettings->aucCountryCode[1];

	prRegInfo->ucSupport5GBand =
			prNvramSettings->ucSupport5GBand;

	prRegInfo->ucEnable5GBand = prNvramSettings->ucEnable5GBand;

	/* load regulation subbands */
	prRegInfo->eRegChannelListMap = 0;
	prRegInfo->ucRegChannelListIndex = 0;

	if (prRegInfo->eRegChannelListMap == REG_CH_MAP_CUSTOMIZED) {
		kalMemCopy(prRegInfo->rDomainInfo.rSubBand,
			prNvramSettings->aucRegSubbandInfo,
			MAX_SUBBAND_NUM*sizeof(uint8_t));
	}

	log_dbg(INIT, INFO, "NVRAM Version = [%08x,%08x], 5G = [%d,%d]\n",
				 prNvramSettings->u2Part1OwnVersion,
				 prNvramSettings->u2Part1PeerVersion,
				 prRegInfo->ucSupport5GBand,
				 prRegInfo->ucEnable5GBand);
}

#if CFG_SUPPORT_TASKLET_FREE_MSDU
static void glTaskletResInit(struct GLUE_INFO *prGlueInfo)
{
	prGlueInfo->u4TxMsduRetFifoLen =
		kalRoundUpPowerOf2(CFG_TX_MAX_PKT_NUM) * sizeof(void *);
	prGlueInfo->prTxMsduRetFifoBuf = kalMemAlloc(
		prGlueInfo->u4TxMsduRetFifoLen, VIR_MEM_TYPE);

	if (prGlueInfo->prTxMsduRetFifoBuf) {
		KAL_FIFO_INIT(&prGlueInfo->rTxMsduRetFifo,
			prGlueInfo->prTxMsduRetFifoBuf,
			prGlueInfo->u4TxMsduRetFifoLen);
	} else {
		DBGLOG(INIT, ERROR,
				"Cannot alloc buf(%d) for TxMsduRetFifo\n",
				prGlueInfo->u4TxMsduRetFifoLen);
		prGlueInfo->u4TxMsduRetFifoLen = 0;
	}
}

static void glTaskletResUninit(struct GLUE_INFO *prGlueInfo)
{
	if (prGlueInfo->prTxMsduRetFifoBuf) {
		struct MSDU_INFO *prMsduInfo;

		/* Return pending MSDU */
		while (KAL_FIFO_OUT(&prGlueInfo->rTxMsduRetFifo, prMsduInfo)) {
			if (!prMsduInfo) {
				DBGLOG(RX, ERROR, "prMsduInfo null\n");
				break;
			}
			nicTxFreePacket(prGlueInfo->prAdapter, prMsduInfo,
				FALSE);
			nicTxReturnMsduInfo(prGlueInfo->prAdapter, prMsduInfo);
		}
		kalMemFree(prGlueInfo->prTxMsduRetFifoBuf, VIR_MEM_TYPE,
			prGlueInfo->u4TxMsduRetFifoLen);
		prGlueInfo->prTxMsduRetFifoBuf = NULL;
		prGlueInfo->u4TxMsduRetFifoLen = 0;
	}
}
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */

static void glTaskletInit(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_TASKLET_FREE_MSDU
	glTaskletResInit(prGlueInfo);
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */

	tasklet_init(&prGlueInfo->rRxTask, halRxTasklet,
			(unsigned long)prGlueInfo);

#if (CFG_SUPPORT_RETURN_TASK == 1)
	tasklet_init(&prGlueInfo->rRxRfbRetTask,
			wlanReturnPacketDelaySetupTasklet,
			(unsigned long)prGlueInfo);
#endif

#if CFG_SUPPORT_TASKLET_FREE_MSDU
	tasklet_init(&prGlueInfo->rTxMsduRetTask,
			halWpdmaFreeMsduTasklet,
			(unsigned long)prGlueInfo);
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */

	tasklet_init(&prGlueInfo->rTxCompleteTask,
			halTxCompleteTasklet,
			(unsigned long)prGlueInfo);

	GLUE_SET_REF_CNT(0, prGlueInfo->u4RxTaskScheduleCnt);

}

static void glTaskletUninit(struct GLUE_INFO *prGlueInfo)
{
	tasklet_kill(&prGlueInfo->rTxCompleteTask);

#if CFG_SUPPORT_TASKLET_FREE_MSDU
	tasklet_kill(&prGlueInfo->rTxMsduRetTask);
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */

#if (CFG_SUPPORT_RETURN_TASK == 1)
	tasklet_kill(&prGlueInfo->rRxRfbRetTask);
#endif
	tasklet_kill(&prGlueInfo->rRxTask);

#if CFG_SUPPORT_TASKLET_FREE_MSDU
	glTaskletResUninit(prGlueInfo);
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */
}

static void glTxRxInit(struct GLUE_INFO *prGlueInfo)
{
	kalTxDirectInit(prGlueInfo);
#if CFG_SUPPORT_CPU_STAT
	CPU_STAT_RESET_ALL_CNTS(prGlueInfo);
#endif /* CFG_SUPPORT_CPU_STAT */
#if CFG_SUPPORT_PER_CPU_TX
	kalPerCpuTxInit(prGlueInfo);
#endif /* CFG_SUPPORT_PER_CPU_TX */
#if CFG_SUPPORT_TX_WORK
	kalTxWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_TX_WORK */
#if CFG_SUPPORT_RX_NAPI_WORK
	kalRxNapiWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI_WORK */
#if CFG_SUPPORT_RX_WORK
	kalRxWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_SUPPORT_RX_GRO
	kalNapiInit(prGlueInfo);
#if CFG_SUPPORT_RX_NAPI
	kalNapiRxDirectInit(prGlueInfo);
	kalNapiEnable(prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI */
#endif /* CFG_SUPPORT_RX_GRO */
	glTaskletInit(prGlueInfo);
#if CFG_SUPPORT_TX_FREE_MSDU_WORK
	kalTxFreeMsduWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */
#if CFG_SUPPORT_TX_FREE_SKB_WORK
	kalTxFreeSkbWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
#if CFG_SUPPORT_RETURN_WORK
	kalRxRfbReturnWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_RETURN_WORK */
#if CFG_SUPPORT_SKB_ALLOC_WORK
	kalSkbAllocWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
#if CFG_SUPPORT_HIF_REG_WORK
	kalHifRegWorkInit(prGlueInfo);
#endif /* CFG_SUPPORT_HIF_REG_WORK */
}

static void glTxRxUninit(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_PER_CPU_TX
	kalPerCpuTxUninit(prGlueInfo);
#endif /* CFG_SUPPORT_PER_CPU_TX */
#if CFG_SUPPORT_TX_WORK
	kalTxWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_TX_WORK */
#if CFG_SUPPORT_RX_NAPI_WORK
	kalRxNapiWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI_WORK */
#if CFG_SUPPORT_RX_WORK
	kalRxWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_SUPPORT_TX_FREE_MSDU_WORK
	kalTxFreeMsduWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */
#if CFG_SUPPORT_TX_FREE_SKB_WORK
	kalTxFreeSkbWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
#if CFG_SUPPORT_RETURN_WORK
	kalRxRfbReturnWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_RETURN_WORK */
#if CFG_SUPPORT_SKB_ALLOC_WORK
	kalSkbAllocWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
	glTaskletUninit(prGlueInfo);
#if CFG_SUPPORT_RX_GRO
#if CFG_SUPPORT_RX_NAPI
	kalNapiDisable(prGlueInfo);
	kalNapiRxDirectUninit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_NAPI */
	kalNapiUninit(prGlueInfo);
#endif /* CFG_SUPPORT_RX_GRO */
	kalTxDirectUninit(prGlueInfo);
#if CFG_SUPPORT_HIF_REG_WORK
	kalHifRegWorkUninit(prGlueInfo);
#endif /* CFG_SUPPORT_HIF_REG_WORK */
}

static void wlanFreeNetDev(void)
{
	uint32_t u4Idx = 0;
	struct net_device *dev;

	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		if (!gprWdev[u4Idx] || !gprWdev[u4Idx]->netdev)
			continue;

		dev = gprWdev[u4Idx]->netdev;
		gprWdev[u4Idx]->netdev = NULL;
		if (dev->reg_state == NETREG_UNREGISTERING) {
			if (rtnl_is_locked())
				DBGLOG(INIT, INFO,
					"%s[%p] should free in net device destructor later\n",
					dev->name, dev);
			else
				DBGLOG(INIT, WARN,
					"free %s[%p], unregistering but rtnl not locked!\n",
					dev->name, dev);
		} else {
			DBGLOG(INIT, INFO, "free %s[%p] state[%d]\n",
				dev->name, dev, dev->reg_state);
			free_netdev(dev);
		}
	}
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief Release prDev from wlandev_array and free tasklet object related to
 *	  it.
 *
 * \param[in] prDev  Pointer to struct net_device
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void wlanClearDevIdx(struct net_device *prDev)
{
	int i;

	ASSERT(prDev);

	for (i = 0; i < CFG_MAX_WLAN_DEVICES; i++) {
		if (arWlanDevInfo[i].prDev == prDev) {
			arWlanDevInfo[i].prDev = NULL;
			u4WlanDevNum--;
		}
	}

}				/* end of wlanClearDevIdx() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Allocate an unique interface index, net_device::ifindex member for
 *	  this wlan device. Store the net_device in wlandev_array, and
 *	  initialize tasklet object related to it.
 *
 * \param[in] prDev  Pointer to struct net_device
 *
 * \retval >= 0      The device number.
 * \retval -1        Fail to get index.
 */
/*----------------------------------------------------------------------------*/
static int wlanGetDevIdx(struct net_device *prDev)
{
	int i;

	ASSERT(prDev);

	for (i = 0; i < CFG_MAX_WLAN_DEVICES; i++) {
		if (arWlanDevInfo[i].prDev == (struct net_device *)NULL) {
			/* Reserve 2 bytes space to store one digit of
			 * device number and NULL terminator.
			 */
			arWlanDevInfo[i].prDev = prDev;
			u4WlanDevNum++;
			return i;
		}
#if CFG_SUPPORT_PERSIST_NETDEV
		else if (arWlanDevInfo[i].prDev == prDev)
			return i;
#endif
	}

	return -1;
}				/* end of wlanGetDevIdx() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, a primary SOCKET interface to configure
 *        the interface lively. Handle an ioctl call on one of our devices.
 *        Everything Linux ioctl specific is done here. Then we pass the
 *	  contents of the ifr->data to the request message handler.
 *
 * \param[in] prDev      Linux kernel netdevice
 *
 * \param[in] prIFReq    Our private ioctl request structure, typed for the
 *			 generic
 *                       struct ifreq so we can use ptr to function
 *
 * \param[in] cmd        Command ID
 *
 * \retval WLAN_STATUS_SUCCESS The IOCTL command is executed successfully.
 * \retval OTHER The execution of IOCTL command is failed.
 */
/*----------------------------------------------------------------------------*/
int wlanDoIOCTL(struct net_device *prDev,
		struct ifreq *prIfReq, int i4Cmd)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int ret = 0;

	/* Verify input parameters for the following functions */
	ASSERT(prDev && prIfReq);
	if (!prDev || !prIfReq) {
		DBGLOG(INIT, ERROR, "Invalid input data\n");
		return -EINVAL;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(INIT, ERROR, "Adapter is not ready\n");
		return -EINVAL;
	}

	if ((i4Cmd >= SIOCIWFIRST) && (i4Cmd < SIOCIWFIRSTPRIV)) {
		/* 0x8B00 ~ 0x8BDF, wireless extension region */
		ret = wext_support_ioctl(prDev, prIfReq, i4Cmd);
	} else if ((i4Cmd >= SIOCIWFIRSTPRIV)
		   && (i4Cmd < SIOCIWLASTPRIV)) {
		/* 0x8BE0 ~ 0x8BFF, private ioctl region */
		ret = priv_support_ioctl(prDev, prIfReq, i4Cmd);
	} else if (i4Cmd == SIOCDEVPRIVATE + 1) {
#ifdef CFG_ANDROID_AOSP_PRIV_CMD
		ret = android_private_support_driver_cmd(prDev, prIfReq, i4Cmd);
#else
		ret = priv_support_driver_cmd(prDev, prIfReq, i4Cmd);
#endif /* CFG_ANDROID_AOSP_PRIV_CMD */
	}  else if (i4Cmd == SIOCDEVPRIVATE + 2) {
		ret = priv_support_ioctl(prDev, prIfReq, i4Cmd);
	} else if (i4Cmd == SIOCDEVPRIVATE + 3) {
		/* For mDNS offload template. */
#if CFG_WOW_SUPPORT
#if (CFG_SUPPORT_MDNS_OFFLOAD && CFG_SUPPORT_MDNS_OFFLOAD_TV)
		ret = priv_support_mdns_offload(prDev, prIfReq, i4Cmd);
#endif
#endif
	} else {
		DBGLOG(INIT, WARN, "Unexpected ioctl command: 0x%04x\n",
		       i4Cmd);
		ret = -EOPNOTSUPP;
	}

	return ret;
}				/* end of wlanDoIOCTL() */

#if KERNEL_VERSION(5, 15, 0) <= CFG80211_VERSION_CODE
int wlanDoPrivIOCTL(struct net_device *prDev, struct ifreq *prIfReq,
		void __user *prData, int i4Cmd)
{
	return wlanDoIOCTL(prDev, prIfReq, i4Cmd);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Export wlan GLUE_INFO_T pointer to p2p module
 *
 * \param[in]  prGlueInfo Pointer to struct GLUE_INFO_T
 *
 * \return TRUE: get GlueInfo pointer successfully
 *            FALSE: wlan is not started yet
 */
/*---------------------------------------------------------------------------*/
struct GLUE_INFO *wlanGetGlueInfo(void)
{
	struct net_device *prDev = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (u4WlanDevNum == 0)
		return NULL;

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (prDev == NULL)
		return NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));

	return prGlueInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is to set multicast list and set rx mode.
 *
 * \param[in] prDev  Pointer to struct net_device
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/

struct net_device *gPrDev;

static void wlanSetMulticastList(struct net_device *prDev)
{
	struct GLUE_INFO *prGlueInfo;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;

	if (!prDev)
		return;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));

	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}

	prNetDevPrivate
			= (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);

	if (!prNetDevPrivate) {
		DBGLOG(REQ, WARN, "prNetDevPrivate is NULL\n");
		return;
	}

	DBGLOG(INIT, TRACE,
		       "Bss[%d] set multicast list.\n",
		       prNetDevPrivate->ucBssIdx);

	/* Allow to receive all multicast for WOW */
	DBGLOG(INIT, TRACE, "flags: 0x%x\n", prDev->flags);
	prDev->flags |= (IFF_MULTICAST | IFF_ALLMULTI);
	gPrDev = prDev;
	schedule_work(&(prNetDevPrivate->workq));
}

/* FIXME: Since we cannot sleep in the wlanSetMulticastList, we arrange
 * another workqueue for sleeping. We don't want to block
 * main_thread, so we can't let tx_thread to do this
 */

static void wlanSetMulticastListWorkQueue(
	struct work_struct *work)
{
	struct NETDEV_PRIVATE_GLUE_INFO *ifp =
		CONTAINER_OF(work, struct NETDEV_PRIVATE_GLUE_INFO, workq);
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4PacketFilter = 0;
	uint32_t u4SetInfoLen;
	struct net_device *prDev;
	uint8_t ucBssIndex = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!ifp) {
		DBGLOG(INIT, INFO,
			"Can't find container of work.\n");
			return;
	}

	ucBssIndex = ifp->ucBssIdx;

	if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
		DBGLOG(INIT, INFO,
			"Invalid  Bss index:%d\n", ucBssIndex);
		return;
	}

	down(&g_halt_sem);
	if (g_u4HaltFlag) {
		up(&g_halt_sem);
		return;
	}

	prGlueInfo = ifp->prGlueInfo;

	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		up(&g_halt_sem);
		return;
	}

	prDev = wlanGetNetDev(prGlueInfo, ucBssIndex);

	if (!prDev) {
		DBGLOG(INIT, WARN,
			"prDev for Bss%d not exist.\n", ucBssIndex);
			up(&g_halt_sem);
		return;
	}

	DBGLOG(INIT, TRACE,
	       "Bss index:%d prDev->flags:0x%x\n",
	       ucBssIndex, prDev->flags);

	if (prDev->flags & IFF_PROMISC)
		u4PacketFilter |= PARAM_PACKET_FILTER_PROMISCUOUS;

	if (prDev->flags & IFF_BROADCAST)
		u4PacketFilter |= PARAM_PACKET_FILTER_BROADCAST;

	if (prDev->flags & IFF_MULTICAST) {
		if ((prDev->flags & IFF_ALLMULTI)
		    || (netdev_mc_count(prDev) > MAX_NUM_GROUP_ADDR))
			u4PacketFilter |= PARAM_PACKET_FILTER_ALL_MULTICAST;
		else
			u4PacketFilter |= PARAM_PACKET_FILTER_MULTICAST;
	}

	up(&g_halt_sem);

	if (kalIoctl(prGlueInfo, wlanoidSetCurrentPacketFilter,
		     &u4PacketFilter, sizeof(u4PacketFilter),
		     &u4SetInfoLen) != WLAN_STATUS_SUCCESS) {
		return;
	}

	if (u4PacketFilter & PARAM_PACKET_FILTER_MULTICAST) {
		/* Prepare multicast address list */
		struct netdev_hw_addr *ha;
		uint32_t i = 0;
		struct PARAM_MULTICAST_LIST rMcAddrList;

		kalMemZero(&rMcAddrList,
				sizeof(struct PARAM_MULTICAST_LIST));

		down(&g_halt_sem);
		if (g_u4HaltFlag) {
			up(&g_halt_sem);
			return;
		}

		/* Avoid race condition with kernel net subsystem */
		netif_addr_lock_bh(prDev);

		netdev_for_each_mc_addr(ha, prDev) {
			if (i < MAX_NUM_GROUP_ADDR) {
				COPY_MAC_ADDR(
					&rMcAddrList.aucMcAddrList[i],
					GET_ADDR(ha));
				DBGLOG(INIT, LOUD, "%u MAC: "MACSTR"\n",
					i, MAC2STR(GET_ADDR(ha)));
				i++;
			}
		}

		netif_addr_unlock_bh(prDev);

		up(&g_halt_sem);

		rMcAddrList.ucBssIdx = ucBssIndex;
		rMcAddrList.ucAddrNum = i;
		rMcAddrList.fgIsOid = TRUE;

		rStatus = kalIoctlByBssIdx(prGlueInfo,
			wlanoidSetMulticastList,
			&rMcAddrList,
			sizeof(struct PARAM_MULTICAST_LIST),
			&u4SetInfoLen,
			ucBssIndex);
	} else if (u4PacketFilter & PARAM_PACKET_FILTER_ALL_MULTICAST) {
		struct PARAM_MULTICAST_LIST rMcAddrList;

		kalMemZero(&rMcAddrList,
				sizeof(struct PARAM_MULTICAST_LIST));

		rMcAddrList.ucBssIdx = ucBssIndex;
		rMcAddrList.ucAddrNum = 0;
		rMcAddrList.fgIsOid = TRUE;

		DBGLOG(INIT, TRACE,
			"Clear previous MAR settings to rx all mc pkt\n");
		rStatus = kalIoctlByBssIdx(prGlueInfo,
				wlanoidSetMulticastList,
				&rMcAddrList,
				sizeof(struct PARAM_MULTICAST_LIST),
				&u4SetInfoLen, ucBssIndex);
	}
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, ERROR,
		"SetMulticastList fail 0x%x\n", rStatus);
}				/* end of wlanSetMulticastList() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief    To indicate scheduled scan has been stopped
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           None
 */
/*----------------------------------------------------------------------------*/
void wlanSchedScanStoppedWorkQueue(struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct net_device *prDev = gPrDev;

	DBGLOG(SCN, INFO, "wlanSchedScanStoppedWorkQueue\n");
	prGlueInfo = (prDev != NULL) ? *((struct GLUE_INFO **)
					 netdev_priv(prDev)) : NULL;
	if (!prGlueInfo) {
		DBGLOG(SCN, INFO, "prGlueInfo == NULL unexpected\n");
		return;
	}

	/* 2. indication to cfg80211 */
	/* 20150205 change cfg80211_sched_scan_stopped to work queue due to
	 * sched_scan_mtx dead lock issue
	 */
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
	cfg80211_sched_scan_stopped(wlanGetWiphy(), 0);
#else
	cfg80211_sched_scan_stopped(wlanGetWiphy());
#endif
	DBGLOG(SCN, INFO,
	       "cfg80211_sched_scan_stopped event send done WorkQueue thread return from wlanSchedScanStoppedWorkQueue\n");
	return;

}

/* FIXME: Since we cannot sleep in the wlanSetMulticastList, we arrange
 * another workqueue for sleeping. We don't want to block
 * main_thread, so we can't let tx_thread to do this
 */

void p2pSetMulticastListWorkQueueWrapper(struct GLUE_INFO
		*prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(INIT, WARN,
		       "abnormal dev or skb: prGlueInfo(0x%p)\n", prGlueInfo);
		return;
	}

#if CFG_ENABLE_WIFI_DIRECT
	if (prGlueInfo->prAdapter->fgIsP2PRegistered)
		mtk_p2p_wext_set_Multicastlist(prGlueInfo);
#endif
} /* end of p2pSetMulticastListWorkQueueWrapper() */

/*----------------------------------------------------------------------------*/
/*
 * \brief This function is TX entry point of NET DEVICE.
 *
 * \param[in] prSkb  Pointer of the sk_buff to be sent
 * \param[in] prDev  Pointer to struct net_device
 *
 * \retval NETDEV_TX_OK - on success.
 * \retval NETDEV_TX_BUSY - on failure, packet will be discarded by upper layer.
 */
/*----------------------------------------------------------------------------*/
netdev_tx_t wlanHardStartXmit(struct sk_buff *prSkb,
		      struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) NULL;
	struct GLUE_INFO *prGlueInfo = *((struct GLUE_INFO **)
					 netdev_priv(prDev));
	uint8_t ucBssIndex;

	ASSERT(prSkb);
	ASSERT(prDev);
	ASSERT(prGlueInfo);

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			  netdev_priv(prDev);
	ASSERT(prNetDevPrivate->prGlueInfo == prGlueInfo);
	ucBssIndex = prNetDevPrivate->ucBssIdx;

#if CFG_SUPPORT_PASSPOINT
	if (prGlueInfo->fgIsDad) {
		/* kalPrint("[Passpoint R2] Due to ipv4_dad...TX is forbidden\n"
		 *         );
		 */
		dev_kfree_skb(prSkb);
		return NETDEV_TX_OK;
	}
	if (prGlueInfo->fgIs6Dad) {
		/* kalPrint("[Passpoint R2] Due to ipv6_dad...TX is forbidden\n"
		 *          );
		 */
		dev_kfree_skb(prSkb);
		return NETDEV_TX_OK;
	}
#endif /* CFG_SUPPORT_PASSPOINT */

#if CFG_CHIP_RESET_SUPPORT
	if (!wlanIsDriverReady(prGlueInfo,
		(WLAN_DRV_READY_CHECK_RESET | WLAN_DRV_READY_CHECK_WLAN_ON))) {
		DBGLOG(INIT, WARN,
		"u4ReadyFlag:%u, kalIsResetting():%d, dropping the packet\n",
		prGlueInfo->u4ReadyFlag, kalIsResetting());
		dev_kfree_skb(prSkb);
		return NETDEV_TX_OK;
	}
#endif
	kalResetPacket(prGlueInfo, (void *) prSkb);
	if (kalHardStartXmit(prSkb, prDev, prGlueInfo,
			     ucBssIndex) == WLAN_STATUS_SUCCESS) {
		/* Successfully enqueue to Tx queue */
		if (netif_carrier_ok(prDev))
			kalPerMonStart(prGlueInfo);
	}

	/* For Linux, we'll always return OK FLAG, because we'll free this skb
	 * by ourself
	 */
	return NETDEV_TX_OK;
}				/* end of wlanHardStartXmit() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, to get the network interface
 *        statistical information.
 *
 * Whenever an application needs to get statistics for the interface, this
 * method is called. This happens, for example, when ifconfig or netstat -i
 * is run.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return net_device_stats buffer pointer.
 */
/*----------------------------------------------------------------------------*/
struct net_device_stats *wlanGetStats(struct net_device
				      *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);
	kalMemCopy(&prNetDevPrivate->stats, &prDev->stats,
			sizeof(struct net_device_stats));
#if CFG_MTK_MDDP_SUPPORT
	mddpGetMdStats(prDev);
#endif

	return (struct net_device_stats *) &prNetDevPrivate->stats;
}				/* end of wlanGetStats() */

void wlanDebugInit(void)
{
	/* Set the initial debug level of each module */
#if DBG
	/* enable all */
	wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX, DBG_CLASS_MASK);
#else
#ifdef CFG_DEFAULT_DBG_LEVEL
	wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX,
			      CFG_DEFAULT_DBG_LEVEL);
#else
	wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX,
			      DBG_LOG_LEVEL_DEFAULT);
#endif
#endif /* DBG */

	LOG_FUNC("Reset ALL DBG module log level to DEFAULT!");

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for prDev->init
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0         The execution of wlanInit succeeds.
 * \retval -ENXIO    No such device.
 */
/*----------------------------------------------------------------------------*/
static int wlanInit(struct net_device *prDev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;

	if (!prDev)
		return -ENXIO;

	prNetDevPrivate
			= (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	INIT_WORK(&(prNetDevPrivate->workq), wlanSetMulticastListWorkQueue);

	/* 20150205 work queue for sched_scan */
	INIT_DELAYED_WORK(&sched_workq,
			  wlanSchedScanStoppedWorkQueue);

	/* 20161024 init wow port setting */
#if CFG_WOW_SUPPORT
	kalWowInit(prGlueInfo);
#endif

#if CFG_SUPPORT_RX_GRO
	kalRxGroInit(prDev);
#endif /* CFG_SUPPORT_RX_GRO */
	return 0;		/* success */
}				/* end of wlanInit() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for prDev->uninit
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void wlanUninit(struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;

	prNetDevPrivate
			= (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);

	if (!prNetDevPrivate) {
		DBGLOG(REQ, WARN, "prNetDevPrivate is NULL\n");
		return;
	}

	cancel_work_sync(&(prNetDevPrivate->workq));
}				/* end of wlanUninit() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, to set the randomized mac address
 *
 * This method is called before Wifi Framework requests a new conenction with
 * enabled feature "Connected Random Mac".
 *
 * \param[in] ndev	Pointer to struct net_device.
 * \param[in] addr	Randomized Mac address passed from WIFI framework.
 *
 * \return int.
 */
/*----------------------------------------------------------------------------*/
static int wlanSetMacAddress(struct net_device *ndev, void *addr)
{
	struct ADAPTER *prAdapter = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct sockaddr *sa = NULL;
	struct AIS_FSM_INFO *prAisFsmInfo = NULL;
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
	u8 _addr[MAC_ADDR_LEN];
#endif

	/**********************************************************************
	 * Check if kernel passes valid data to us                            *
	 **********************************************************************
	 */
	if (!ndev || !addr) {
		DBGLOG(INIT, ERROR, "Set macaddr with ndev(%d) and addr(%d)\n",
		       (ndev == NULL) ? 0 : 1, (addr == NULL) ? 0 : 1);
		return -EINVAL;
	}

	/**********************************************************************
	 * 1. Change OwnMacAddr which will be updated to FW through           *
	 *    rlmActivateNetwork later.                                       *
	 * 2. Change dev_addr stored in kernel to notify framework that the   *
	 *    mac addr has been changed and what the new value is.            *
	 **********************************************************************
	 */
	sa = (struct sockaddr *)addr;
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(ndev));

	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	prAdapter = prGlueInfo->prAdapter;
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, wlanGetBssIdx(ndev));

	if (aisUpdateInterfaceAddr(prAdapter, prAisFsmInfo,
				   sa->sa_data) == FALSE)
		DBGLOG(INIT, ERROR, "[%s] Set mac failed.\n", ndev->name);

#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
	ether_addr_copy(_addr, sa->sa_data);
	eth_hw_addr_set(ndev, _addr);
#else
	COPY_MAC_ADDR(ndev->dev_addr, sa->sa_data);
#endif

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanSetMacAddr() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for prDev->open
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution of wlanOpen succeeds.
 * \retval < 0   The execution of wlanOpen failed.
 */
/*----------------------------------------------------------------------------*/
static int wlanOpen(struct net_device *prDev)
{
/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS || CFG_SUPPORT_WAKEUP_STATISTICS || \
CFG_SUPPORT_WED_PROXY
	struct GLUE_INFO *prGlueInfo = NULL;
#endif /* fos_change end */
#if CFG_SUPPORT_WED_PROXY
	uint32_t u4BufLen = 0;
#endif
	ASSERT(prDev);

/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS || CFG_SUPPORT_WAKEUP_STATISTICS || \
CFG_SUPPORT_WED_PROXY
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	ASSERT(prGlueInfo);
#endif /* fos_change begin */

#if CFG_SUPPORT_WED_PROXY
	kalIoctlByBssIdx(prGlueInfo, wlanoidWedAttachWarp, prDev,
		sizeof(struct net_device *), &u4BufLen, wlanGetBssIdx(prDev));
#endif
	netif_tx_start_all_queues(prDev);
/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
	if (prGlueInfo->prAdapter) {
		/* Initialize arWakeupStatistic */
		kalMemSet(prGlueInfo->prAdapter->arWakeupStatistic, 0,
			sizeof(prGlueInfo->prAdapter->arWakeupStatistic));
		/* Initialize wake_event_count */
		kalMemSet(prGlueInfo->prAdapter->wake_event_count, 0,
			sizeof(prGlueInfo->prAdapter->wake_event_count));
	}
#endif
#if CFG_SUPPORT_EXCEPTION_STATISTICS
	if (prGlueInfo->prAdapter) {
		kalMemSet(prGlueInfo->prAdapter->beacon_timeout_count, 0,
			sizeof(prGlueInfo->prAdapter->beacon_timeout_count));
		prGlueInfo->prAdapter->total_beacon_timeout_count = 0;

		kalMemSet(prGlueInfo->prAdapter->tx_done_fail_count, 0,
			sizeof(prGlueInfo->prAdapter->tx_done_fail_count));
		prGlueInfo->prAdapter->total_tx_done_fail_count = 0;

		kalMemSet(prGlueInfo->prAdapter->deauth_rx_count, 0,
			sizeof(prGlueInfo->prAdapter->deauth_rx_count));
		prGlueInfo->prAdapter->total_deauth_rx_count = 0;

		prGlueInfo->prAdapter->total_scandone_timeout_count = 0;
		prGlueInfo->prAdapter->total_mgmtTX_timeout_count = 0;
		prGlueInfo->prAdapter->total_mgmtRX_timeout_count = 0;
	}
#endif /* fos_change end */

	return 0;		/* success */
}				/* end of wlanOpen() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for prDev->stop
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution of wlanStop succeeds.
 * \retval < 0   The execution of wlanStop failed.
 */
/*----------------------------------------------------------------------------*/
static int wlanStop(struct net_device *prDev)
{
	uint32_t u4SetInfoLen = 0, rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t fgNeedAbortScan = FALSE;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));

	if (!prGlueInfo) {
		DBGLOG(INIT, WARN, "driver is not ready, prGlueInfo is NULL\n");
	} else if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(INIT, WARN, "driver is not ready, u4ReadyFlag = 0\n");
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prGlueInfo->prScanRequest) {
			kalCfg80211ScanDone(prGlueInfo->prScanRequest, TRUE);
			prGlueInfo->prScanRequest = NULL;
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	} else {
		/* CFG80211 down, report to kernel directly and run normal
		*  scan abort procedure
		*/
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prGlueInfo->prScanRequest) {
			DBGLOG(INIT, INFO, "wlanStop abort scan!\n");
			kalCfg80211ScanDone(prGlueInfo->prScanRequest, TRUE);
			prGlueInfo->prScanRequest = NULL;
			fgNeedAbortScan = TRUE;
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (fgNeedAbortScan) {
			rStatus = kalIoctlByBssIdx(prGlueInfo,
						wlanoidAbortScan,
						NULL, 1, &u4SetInfoLen,
						wlanGetBssIdx(prDev));
			if (rStatus != WLAN_STATUS_SUCCESS)
				DBGLOG(REQ, ERROR,
				"wlanoidAbortScan fail 0x%x\n", rStatus);
		}
	}

	netif_tx_stop_all_queues(prDev);
#if CFG_SUPPORT_WED_PROXY
	if (kalIsHalted() == FALSE)
		kalIoctlByBssIdx(prGlueInfo, wlanoidWedDetachWarp, prDev,
			sizeof(struct net_device *), &u4SetInfoLen,
			wlanGetBssIdx(prDev));
	else
		wlanoidWedDetachWarp(prGlueInfo->prAdapter, prDev,
			sizeof(struct net_device *), &u4SetInfoLen);
#endif

	return 0;		/* success */
}				/* end of wlanStop() */

void wlanUpdateChannelFlagByBand(struct GLUE_INFO *prGlueInfo,
				enum ENUM_BAND eBand)
{
	uint8_t i, j;
	uint8_t ucNumOfChannel;
	struct RF_CHANNEL_INFO aucChannelList[MAX_PER_BAND_CHN_NUM];

	kalMemZero(aucChannelList, sizeof(aucChannelList));

	/* 2. Get current domain channel list */
	rlmDomainGetChnlList(prGlueInfo->prAdapter,
			eBand, FALSE, MAX_PER_BAND_CHN_NUM,
			&ucNumOfChannel, aucChannelList);

	/* 3. Enable specific channel based on domain channel list */
	for (i = 0; i < ucNumOfChannel; i++) {
		switch (aucChannelList[i].eBand) {
		case BAND_2G4:
			for (j = 0; j < ARRAY_SIZE(mtk_2ghz_channels); j++) {
				if (mtk_2ghz_channels[j].hw_value ==
					aucChannelList[i].ucChannelNum) {
					mtk_2ghz_channels[j].flags &=
						~IEEE80211_CHAN_DISABLED;
					mtk_2ghz_channels[j].orig_flags &=
						~IEEE80211_CHAN_DISABLED;
					break;
				}
			}
			break;

		case BAND_5G:
			for (j = 0; j < ARRAY_SIZE(mtk_5ghz_channels); j++) {
				if (mtk_5ghz_channels[j].hw_value ==
					aucChannelList[i].ucChannelNum) {
					mtk_5ghz_channels[j].flags &=
						~IEEE80211_CHAN_DISABLED;
					mtk_5ghz_channels[j].orig_flags &=
						~IEEE80211_CHAN_DISABLED;
					mtk_5ghz_channels[j].dfs_state =
						((enum nl80211_dfs_state)
						aucChannelList[i].fgDFS
						!= NL80211_DFS_USABLE) ?
						 NL80211_DFS_USABLE :
						 NL80211_DFS_UNAVAILABLE;

					if (mtk_5ghz_channels[j].dfs_state ==
						NL80211_DFS_USABLE)
						mtk_5ghz_channels[j].flags |=
							IEEE80211_CHAN_RADAR;
					else
						mtk_5ghz_channels[j].flags &=
							~IEEE80211_CHAN_RADAR;
					break;
				}
			}
			break;

#if (CFG_SUPPORT_WIFI_6G == 1)
		case BAND_6G:
			for (j = 0; j < ARRAY_SIZE(mtk_6ghz_channels); j++) {
				if (mtk_6ghz_channels[j].hw_value ==
					aucChannelList[i].ucChannelNum) {
					mtk_6ghz_channels[j].flags &=
						~IEEE80211_CHAN_DISABLED;
					mtk_6ghz_channels[j].orig_flags
						&= ~IEEE80211_CHAN_DISABLED;
					break;
				}
			}
			break;
#endif

		default:
			DBGLOG(INIT, WARN, "Unknown band %d\n",
				aucChannelList[i].eBand);
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Update Channel table for cfg80211 for Wi-Fi Direct based on current
 *        country code
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   none
 */
/*----------------------------------------------------------------------------*/
void wlanUpdateChannelTable(struct GLUE_INFO *prGlueInfo)
{
	uint8_t i;
	uint8_t ucBandIdx;

	/* 1. Disable all channels */
	for (i = 0; i < ARRAY_SIZE(mtk_2ghz_channels); i++) {
		mtk_2ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
		mtk_2ghz_channels[i].orig_flags |= IEEE80211_CHAN_DISABLED;
	}

	for (i = 0; i < ARRAY_SIZE(mtk_5ghz_channels); i++) {
		mtk_5ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
		mtk_5ghz_channels[i].orig_flags |= IEEE80211_CHAN_DISABLED;
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	for (i = 0; i < ARRAY_SIZE(mtk_6ghz_channels); i++) {
		mtk_6ghz_channels[i].flags |= IEEE80211_CHAN_DISABLED;
		mtk_6ghz_channels[i].orig_flags |= IEEE80211_CHAN_DISABLED;
	}
#endif

	for (ucBandIdx = BAND_2G4; ucBandIdx < BAND_NUM; ucBandIdx++)
		wlanUpdateChannelFlagByBand(prGlueInfo, ucBandIdx);
}

static void wlanUpdateDfsChannelTable(struct ADAPTER *prAdapter,
	uint8_t aucWhiteList[], uint32_t u4WhiteListNum)
{
	uint8_t i, j, k, ucNumOfChannel;
	struct RF_CHANNEL_INFO aucChannelList[
			ARRAY_SIZE(mtk_5ghz_channels)] = {};

	/* 1. Get current domain DFS channel list */
	rlmDomainGetDfsChnls(prAdapter, ARRAY_SIZE(mtk_5ghz_channels),
			     &ucNumOfChannel, aucChannelList);

	/* 2. Enable specific channel based on domain channel list */
	for (i = 0; i < ucNumOfChannel; i++) {
		for (j = 0; j < ARRAY_SIZE(mtk_5ghz_channels); j++) {
			u_int8_t fgEnableDfsChnl = FALSE;

			if (aucChannelList[i].ucChannelNum !=
				mtk_5ghz_channels[j].hw_value)
				continue;

			for (k = 0; k < u4WhiteListNum; k++) {
				if (aucChannelList[i].ucChannelNum ==
				    aucWhiteList[k]) {
					fgEnableDfsChnl = TRUE;
					break;
				}
			}

			if (fgEnableDfsChnl) {
				mtk_5ghz_channels[j].dfs_state
					= NL80211_DFS_AVAILABLE;
				mtk_5ghz_channels[j].flags &=
					~IEEE80211_CHAN_RADAR;
				mtk_5ghz_channels[j].orig_flags &=
					~IEEE80211_CHAN_RADAR;
				DBGLOG(INIT, INFO,
					"ch (%d), force NL80211_DFS_AVAILABLE.\n",
					aucChannelList[i].ucChannelNum);
			} else {
				mtk_5ghz_channels[j].dfs_state
					= NL80211_DFS_USABLE;
				mtk_5ghz_channels[j].flags |=
					IEEE80211_CHAN_RADAR;
				mtk_5ghz_channels[j].orig_flags |=
					IEEE80211_CHAN_RADAR;
				DBGLOG(INIT, TRACE,
					"ch (%d), force NL80211_DFS_USABLE.\n",
					aucChannelList[i].ucChannelNum);
			}
		}
	}
}

static void __wlanDfsChannelsReqMerge(struct ADAPTER *prAdapter,
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntries, uint32_t u4EntriesNum,
	uint8_t aucChannels[], uint32_t u4MaxSize, uint32_t *pu4Size)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;
	uint8_t i = 0;

	for (i = 0; i < u4EntriesNum; i++) {
		uint32_t u4Freq, u4StartFreq = 0, u4EndFreq = 0,
			u4BandWidth = 0;

		prEntry = &prEntries[i];

		if (!prEntry->fgValid)
			continue;

		switch (prEntry->rRfChnlInfo.ucChnlBw) {
		case MAX_BW_20MHZ:
			u4BandWidth = 20;
			break;
		case MAX_BW_40MHZ:
			u4BandWidth = 40;
			break;
		case MAX_BW_80MHZ:
		case MAX_BW_80_80_MHZ:
			u4BandWidth = 80;
			break;
		case MAX_BW_160MHZ:
			u4BandWidth = 160;
			break;
		case MAX_BW_320_1MHZ:
		case MAX_BW_320_2MHZ:
			u4BandWidth = 320;
			break;
		default:
			DBGLOG(INIT, WARN, "unsupported bandwidth: %d",
				prEntry->rRfChnlInfo.ucChnlBw);
			u4BandWidth = 20;
			break;
		}

		if (prEntry->rRfChnlInfo.ucChnlBw <= MAX_BW_20MHZ) {
			u4StartFreq = prEntry->rRfChnlInfo.u4CenterFreq1;
			u4EndFreq = prEntry->rRfChnlInfo.u4CenterFreq1;
		} else {
			u4StartFreq = prEntry->rRfChnlInfo.u4CenterFreq1 -
				u4BandWidth / 2 + 10;
			u4EndFreq = prEntry->rRfChnlInfo.u4CenterFreq1 +
				u4BandWidth / 2 - 10;
		}

		for (u4Freq = u4StartFreq; u4Freq <= u4EndFreq; u4Freq += 20) {
			struct ieee80211_channel *channel;

			channel = ieee80211_get_channel(wlanGetWiphy(), u4Freq);
			if (!channel)
				continue;

			aucChannels[*pu4Size] = channel->hw_value;
			(*pu4Size)++;

			if (*pu4Size >= u4MaxSize) {
				DBGLOG(INIT, WARN, "exceed max size(%u %u)\n",
					*pu4Size, u4MaxSize);
				return;
			}
		}
	}
}

static void wlanDfsChannelsReqMerge(struct ADAPTER *prAdapter,
	uint8_t aucChannels[], uint32_t u4MaxSize, uint32_t *pu4Size)
{
	__wlanDfsChannelsReqMerge(prAdapter,
		prAdapter->aucDfsAisChnlReqEntries,
		ARRAY_SIZE(prAdapter->aucDfsAisChnlReqEntries),
		aucChannels,
		u4MaxSize,
		pu4Size);

	__wlanDfsChannelsReqMerge(prAdapter,
		prAdapter->aucDfsChnlReqEntries,
		ARRAY_SIZE(prAdapter->aucDfsChnlReqEntries),
		aucChannels,
		u4MaxSize,
		pu4Size);
}

uint32_t wlanDfsChannelsReqInit(struct ADAPTER *prAdapter)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(prAdapter->aucDfsAisChnlReqEntries);
		i++){
		prEntry = &prAdapter->aucDfsAisChnlReqEntries[i];

		prEntry->fgValid = FALSE;
		prEntry->eSource = DFS_CHANNEL_CTRL_SOURCE_STA;
		kalMemZero(&prEntry->rRfChnlInfo,
			   sizeof(prEntry->rRfChnlInfo));
	}

	for (i = 0; i < ARRAY_SIZE(prAdapter->aucDfsChnlReqEntries); i++) {
		prEntry = &prAdapter->aucDfsChnlReqEntries[i];

		prEntry->fgValid = FALSE;
		prEntry->eSource = DFS_CHANNEL_CTRL_SOURCE_NUM;
		kalMemZero(&prEntry->rRfChnlInfo,
			   sizeof(prEntry->rRfChnlInfo));
	}

	return WLAN_STATUS_SUCCESS;
}

void wlanDfsChannelsReqDeInit(struct ADAPTER *prAdapter)
{
	DBGLOG(INIT, INFO, "\n");

	kalMemZero(prAdapter->aucDfsAisChnlReqEntries,
		sizeof(prAdapter->aucDfsAisChnlReqEntries));
	kalMemZero(prAdapter->aucDfsChnlReqEntries,
		sizeof(prAdapter->aucDfsChnlReqEntries));
}

void wlanDfsChannelsReqDump(struct ADAPTER *prAdapter)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;
	uint8_t i = 0;

	DBGLOG(INIT, TRACE, "Dump for ais entries\n");
	for (i = 0; i < ARRAY_SIZE(prAdapter->aucDfsAisChnlReqEntries); i++) {
		prEntry = &prAdapter->aucDfsAisChnlReqEntries[i];

		DBGLOG(INIT, TRACE,
			"\t[%u] valid=%d, source=%d, channel=[%u %u %u %u %u %u]\n",
			i, prEntry->fgValid, prEntry->eSource,
			prEntry->rRfChnlInfo.eBand,
			prEntry->rRfChnlInfo.ucChannelNum,
			prEntry->rRfChnlInfo.u2PriChnlFreq,
			prEntry->rRfChnlInfo.u4CenterFreq1,
			prEntry->rRfChnlInfo.u4CenterFreq2,
			prEntry->rRfChnlInfo.ucChnlBw);
	}

	DBGLOG(INIT, TRACE, "Dump for other entries\n");
	for (i = 0; i < ARRAY_SIZE(prAdapter->aucDfsChnlReqEntries); i++) {
		prEntry = &prAdapter->aucDfsChnlReqEntries[i];

		DBGLOG(INIT, TRACE,
			"\t[%u] valid=%d, source=%d, channel=[%u %u %u %u %u %u]\n",
			i, prEntry->fgValid, prEntry->eSource,
			prEntry->rRfChnlInfo.eBand,
			prEntry->rRfChnlInfo.ucChannelNum,
			prEntry->rRfChnlInfo.u2PriChnlFreq,
			prEntry->rRfChnlInfo.u4CenterFreq1,
			prEntry->rRfChnlInfo.u4CenterFreq2,
			prEntry->rRfChnlInfo.ucChnlBw);
	}
}

static u_int8_t wlanIsChannelInDfsRange(struct ADAPTER *prAdapter,
	uint8_t ucChannel, uint8_t ucBandWidth,
	enum ENUM_CHNL_EXT eBssSCO, uint32_t u4CenterFreq,
	enum ENUM_BAND eBand)
{
	uint32_t u4StartFreq, u4EndFreq, u4Freq;
	uint32_t u4Bw;
	u_int8_t fgInDfsRange = FALSE;
	enum ENUM_MAX_BANDWIDTH_SETTING eBw;

	eBw = rlmVhtBw2Bw(ucBandWidth, eBssSCO);
	switch (eBw) {
	case MAX_BW_20MHZ:
		u4Bw = 20;
		break;
	case MAX_BW_40MHZ:
		u4Bw = 40;
		break;
	case MAX_BW_80MHZ:
	case MAX_BW_80_80_MHZ:
		u4Bw = 80;
		break;
	case MAX_BW_160MHZ:
		u4Bw = 160;
		break;
	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		u4Bw = 320;
		break;
	default:
		DBGLOG(INIT, WARN, "unsupported bandwidth: %d",
			ucBandWidth);
		u4Bw = 20;
		break;
	}

	if (eBw <= MAX_BW_20MHZ) {
		u4StartFreq = u4CenterFreq;
		u4EndFreq = u4CenterFreq;
	} else {
		u4StartFreq = u4CenterFreq - u4Bw / 2 + 10;
		u4EndFreq = u4CenterFreq + u4Bw / 2 - 10;
	}

	for (u4Freq = u4StartFreq; u4Freq <= u4EndFreq; u4Freq += 20) {
		uint32_t u4ChannelNum = nicFreq2ChannelNum(u4Freq * 1000);

		if (u4ChannelNum == 0)
			continue;

		if (rlmDomainIsDfsChnls(prAdapter, u4ChannelNum)) {
			fgInDfsRange = TRUE;
			break;
		}
	}

	return fgInDfsRange;
}

static void wlanDfsChannelsReqMergeNUpdate(struct ADAPTER *prAdapter)
{
	uint8_t aucChannels[MAX_5G_BAND_CHN_NUM] = { 0 };
	uint32_t u4Channels = 0;

	wlanDfsChannelsReqMerge(prAdapter, aucChannels,
				ARRAY_SIZE(aucChannels),
				&u4Channels);

	wlanDfsChannelsReqDump(prAdapter);

	wlanUpdateDfsChannelTable(prAdapter, aucChannels, u4Channels);
}

uint32_t wlanDfsChannelsReqAdd(struct ADAPTER *prAdapter,
	enum DFS_CHANNEL_CTRL_SOURCE eSource,
	uint8_t ucChannel, uint8_t ucBandWidth,
	enum ENUM_CHNL_EXT eBssSCO, uint32_t u4CenterFreq,
	enum ENUM_BAND eBand)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;

	if (eSource >= DFS_CHANNEL_CTRL_SOURCE_NUM)
		return WLAN_STATUS_INVALID_DATA;

	DBGLOG(INIT, INFO, "source=%d, channel=[%d %u %u %u %u]\n",
		eSource,
		eBand,
		ucChannel,
		u4CenterFreq,
		ucBandWidth,
		eBssSCO);

	prEntry = &prAdapter->aucDfsChnlReqEntries[eSource];

	if (prEntry->fgValid) {
		DBGLOG(INIT, WARN,
			"Remove previous req.\n");
		wlanDfsChannelsReqDel(prAdapter, eSource);
	}

	if (wlanIsChannelInDfsRange(prAdapter, ucChannel, ucBandWidth,
				    eBssSCO, u4CenterFreq, eBand) == FALSE)
		return WLAN_STATUS_SUCCESS;

	prEntry->eSource = eSource;
	prEntry->rRfChnlInfo.eBand = eBand;
	prEntry->rRfChnlInfo.u4CenterFreq1 = u4CenterFreq;
	prEntry->rRfChnlInfo.u4CenterFreq2 = 0;
	prEntry->rRfChnlInfo.u2PriChnlFreq =
		nicChannelNum2Freq(ucChannel, eBand) / 1000;
	prEntry->rRfChnlInfo.ucChnlBw = rlmVhtBw2Bw(ucBandWidth,
						    eBssSCO);
	prEntry->rRfChnlInfo.ucChannelNum = ucChannel;
	prEntry->fgValid = TRUE;

	wlanDfsChannelsReqMergeNUpdate(prAdapter);

	return WLAN_STATUS_SUCCESS;
}

void wlanDfsChannelsReqDel(struct ADAPTER *prAdapter,
	enum DFS_CHANNEL_CTRL_SOURCE eSource)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;

	if (!prAdapter)
		return;

	if (eSource >= DFS_CHANNEL_CTRL_SOURCE_NUM)
		return;

	DBGLOG(INIT, INFO, "source=%d\n", eSource);

	prEntry = &prAdapter->aucDfsChnlReqEntries[eSource];

	prEntry->fgValid = FALSE;
	prEntry->eSource = DFS_CHANNEL_CTRL_SOURCE_NUM;
	kalMemZero(&prEntry->rRfChnlInfo,
		   sizeof(prEntry->rRfChnlInfo));

	wlanDfsChannelsReqMergeNUpdate(prAdapter);
}

u_int8_t wlanDfsChannelsAllowdBySta(struct ADAPTER *prAdapter,
	struct RF_CHANNEL_INFO *prRfChnlInfo)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;
	uint32_t u4StartFreq, u4EndFreq, u4Bw;
	uint8_t i = 0;
	u_int8_t fgValid = FALSE;

	if (IS_STA_DFS_CHANNEL_ENABLED(prAdapter) == FALSE &&
	    IS_STA_INDOOR_CHANNEL_ENABLED(prAdapter) == FALSE)
		return FALSE;

	if (prRfChnlInfo->eBand != BAND_5G)
		return TRUE;

	switch (prRfChnlInfo->ucChnlBw) {
	case MAX_BW_20MHZ:
		u4Bw = 20;
		break;
	case MAX_BW_40MHZ:
		u4Bw = 40;
		break;
	case MAX_BW_80MHZ:
	case MAX_BW_80_80_MHZ:
		u4Bw = 80;
		break;
	case MAX_BW_160MHZ:
		u4Bw = 160;
		break;
	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		u4Bw = 320;
		break;
	default:
		DBGLOG(INIT, WARN, "unsupported bandwidth: %d",
			prRfChnlInfo->ucChnlBw);
		u4Bw = 20;
		break;
	}

	if (prRfChnlInfo->ucChnlBw <= MAX_BW_20MHZ) {
		u4StartFreq = prRfChnlInfo->u4CenterFreq1;
		u4EndFreq = prRfChnlInfo->u4CenterFreq1;
	} else {
		u4StartFreq = prRfChnlInfo->u4CenterFreq1 - u4Bw / 2 + 10;
		u4EndFreq = prRfChnlInfo->u4CenterFreq1 + u4Bw / 2 - 10;
	}

	for (i = 0;
	     i < ARRAY_SIZE(prAdapter->aucDfsAisChnlReqEntries);
	     i++) {
		uint32_t u4StaStartFreq, u4StaEndFreq, u4StaBw;

		prEntry = &prAdapter->aucDfsAisChnlReqEntries[i];

		if (!prEntry->fgValid)
			continue;

		switch (prEntry->rRfChnlInfo.ucChnlBw) {
		case MAX_BW_20MHZ:
			u4StaBw = 20;
			break;
		case MAX_BW_40MHZ:
			u4StaBw = 40;
			break;
		case MAX_BW_80MHZ:
		case MAX_BW_80_80_MHZ:
			u4StaBw = 80;
			break;
		case MAX_BW_160MHZ:
			u4StaBw = 160;
			break;
		case MAX_BW_320_1MHZ:
		case MAX_BW_320_2MHZ:
			u4StaBw = 320;
			break;
		default:
			DBGLOG(INIT, WARN, "unsupported bandwidth: %d",
				prEntry->rRfChnlInfo.ucChnlBw);
			u4StaBw = 20;
			break;
		}

		if (prEntry->rRfChnlInfo.ucChnlBw <= MAX_BW_20MHZ) {
			u4StaStartFreq = prEntry->rRfChnlInfo.u4CenterFreq1;
			u4StaEndFreq = prEntry->rRfChnlInfo.u4CenterFreq1;
		} else {
			u4StaStartFreq = prEntry->rRfChnlInfo.u4CenterFreq1 -
				u4StaBw / 2 + 10;
			u4StaEndFreq = prEntry->rRfChnlInfo.u4CenterFreq1 +
				u4StaBw / 2 - 10;
		}

		if (u4StaStartFreq <= u4StartFreq &&
		    u4StaEndFreq >= u4EndFreq) {
			fgValid = TRUE;
			break;
		}
	}

	return fgValid;
}

uint32_t wlanDfsChannelsNotifyStaConnected(struct ADAPTER *prAdapter,
	uint8_t ucAisIndex)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss;
#endif
	struct BSS_INFO *prBssInfo;
	uint32_t u4CenterFreq;
	uint8_t ucS1;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;

	if (ucAisIndex >= KAL_AIS_NUM)
		return WLAN_STATUS_INVALID_DATA;

	if (IS_STA_DFS_CHANNEL_ENABLED(prAdapter) == FALSE &&
	    IS_STA_INDOOR_CHANNEL_ENABLED(prAdapter) == FALSE)
		return FALSE;

	DBGLOG(INIT, INFO, "ucAisIndex=%d\n", ucAisIndex);

	prEntry = &prAdapter->aucDfsAisChnlReqEntries[ucAisIndex];
	prBssInfo = AIS_MAIN_BSS_INFO(prAdapter, ucAisIndex);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = mldBssGetByBss(prAdapter, prBssInfo);
	if (prMldBss) {
		LINK_FOR_EACH_ENTRY(prBssInfo, &prMldBss->rBssList,
				    rLinkEntryMld, struct BSS_INFO) {
			u4CenterFreq = nicChannelNum2Freq(
				prBssInfo->ucVhtChannelFrequencyS1,
				prBssInfo->eBand) / 1000;
			if (u4CenterFreq == 0) {
				ucS1 = nicGetS1(prAdapter,
					prBssInfo->eBand,
					prBssInfo->ucPrimaryChannel,
					prBssInfo->ucVhtChannelWidth);
				u4CenterFreq = nicChannelNum2Freq(ucS1,
					prBssInfo->eBand) / 1000;
				DBGLOG(INIT, WARN, "u4CenterFreq == 0, calculate to%d\n", u4CenterFreq);
			}
			if (wlanIsChannelInDfsRange(prAdapter,
						    prBssInfo->
							ucPrimaryChannel,
						    prBssInfo->
							ucVhtChannelWidth,
						    prBssInfo->eBssSCO,
						    u4CenterFreq,
						    prBssInfo->eBand) ==
			    FALSE)
				continue;

			prEntry->eSource = DFS_CHANNEL_CTRL_SOURCE_STA;
			prEntry->rRfChnlInfo.eBand = prBssInfo->eBand;
			prEntry->rRfChnlInfo.u4CenterFreq1 = u4CenterFreq;
			prEntry->rRfChnlInfo.u4CenterFreq2 = 0;
			prEntry->rRfChnlInfo.u2PriChnlFreq =
				nicChannelNum2Freq(prBssInfo->
							ucPrimaryChannel,
						   prBssInfo->eBand) / 1000;
			prEntry->rRfChnlInfo.ucChnlBw =
				rlmVhtBw2Bw(prBssInfo->ucVhtChannelWidth,
					    prBssInfo->eBssSCO);
			prEntry->rRfChnlInfo.ucChannelNum =
				prBssInfo->ucPrimaryChannel;
			prEntry->fgValid = TRUE;

			DBGLOG(INIT, TRACE,
				"[%u] channel=[%u %u %u %u %u %u]\n",
				prBssInfo->ucBssIndex,
				prEntry->rRfChnlInfo.eBand,
				prEntry->rRfChnlInfo.ucChannelNum,
				prEntry->rRfChnlInfo.u2PriChnlFreq,
				prEntry->rRfChnlInfo.u4CenterFreq1,
				prEntry->rRfChnlInfo.u4CenterFreq2,
				prEntry->rRfChnlInfo.ucChnlBw);

			break;
		}
	}
#else
	u4CenterFreq = nicChannelNum2Freq(
		prBssInfo->ucVhtChannelFrequencyS1,
		prBssInfo->eBand) / 1000;
	if (u4CenterFreq == 0) {
		ucS1 = nicGetS1(prAdapter,
			prBssInfo->eBand,
			prBssInfo->ucPrimaryChannel,
			prBssInfo->ucVhtChannelWidth);
		u4CenterFreq = nicChannelNum2Freq(ucS1,
			prBssInfo->eBand) / 1000;
		DBGLOG(INIT, WARN, "u4CenterFreq == 0, calculate to%d\n",
			u4CenterFreq);
	}
	if (wlanIsChannelInDfsRange(prAdapter,
				    prBssInfo->ucPrimaryChannel,
				    prBssInfo->ucVhtChannelWidth,
				    prBssInfo->eBssSCO,
				    u4CenterFreq,
				    prBssInfo->eBand) ==
	    TRUE) {
		prEntry->eSource = DFS_CHANNEL_CTRL_SOURCE_STA;
		prEntry->rRfChnlInfo.eBand = prBssInfo->eBand;
		prEntry->rRfChnlInfo.u4CenterFreq1 = u4CenterFreq;
		prEntry->rRfChnlInfo.u4CenterFreq2 = 0;
		prEntry->rRfChnlInfo.u2PriChnlFreq =
			nicChannelNum2Freq(prBssInfo->ucPrimaryChannel,
					   prBssInfo->eBand) / 1000;
		prEntry->rRfChnlInfo.ucChnlBw =
			rlmVhtBw2Bw(prBssInfo->ucVhtChannelWidth,
				    prBssInfo->eBssSCO);
		prEntry->rRfChnlInfo.ucChannelNum =
			prBssInfo->ucPrimaryChannel;
		prEntry->fgValid = TRUE;

		DBGLOG(INIT, TRACE,
			"[%u] channel=[%u %u %u %u %u %u]\n",
			prBssInfo->ucBssIndex,
			prEntry->rRfChnlInfo.eBand,
			prEntry->rRfChnlInfo.ucChannelNum,
			prEntry->rRfChnlInfo.u2PriChnlFreq,
			prEntry->rRfChnlInfo.u4CenterFreq1,
			prEntry->rRfChnlInfo.u4CenterFreq2,
			prEntry->rRfChnlInfo.ucChnlBw);
	}
#endif

	wlanDfsChannelsReqMergeNUpdate(prAdapter);

	return WLAN_STATUS_SUCCESS;
}

void wlanDfsChannelsNotifyStaDisconnected(struct ADAPTER *prAdapter,
	uint8_t ucAisIndex)
{
	struct WLAN_DFS_CHANNEL_REQ_ENTRY *prEntry;

	if (!prAdapter)
		return;

	if (ucAisIndex >= KAL_AIS_NUM)
		return;

	if (IS_STA_DFS_CHANNEL_ENABLED(prAdapter) == FALSE &&
	    IS_STA_INDOOR_CHANNEL_ENABLED(prAdapter) == FALSE)
		return;

	DBGLOG(INIT, INFO, "ucAisIndex=%d\n", ucAisIndex);

	prEntry = &prAdapter->aucDfsAisChnlReqEntries[ucAisIndex];
	prEntry->fgValid = FALSE;
	prEntry->eSource = DFS_CHANNEL_CTRL_SOURCE_STA;
	kalMemZero(prEntry, sizeof(*prEntry));

	wlanDfsChannelsReqMergeNUpdate(prAdapter);
}

static void mtk_vif_destructor(struct net_device *dev)
{
	struct wireless_dev *prWdev = NULL;

	if (dev) {
		DBGLOG(AIS, INFO, "netdev=%p, wdev=%p\n",
			dev, dev->ieee80211_ptr);
		prWdev = dev->ieee80211_ptr;
		if (prWdev)
			prWdev->netdev = NULL;

		if (u4WlanDevNum > 0 &&
		    u4WlanDevNum <= ARRAY_SIZE(arWlanDevInfo) &&
		    arWlanDevInfo[u4WlanDevNum - 1].prDev == dev)
			arWlanDevInfo[u4WlanDevNum - 1].prDev = NULL;

		free_netdev(dev);
		DBGLOG(AIS, INFO, "free_netdev done\n");
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Register the device to the kernel and return the index.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution of wlanNetRegister succeeds.
 * \retval < 0   The execution of wlanNetRegister failed.
 */
/*----------------------------------------------------------------------------*/
static int32_t wlanNetRegister(struct wireless_dev *prWdev)
{
	struct GLUE_INFO *prGlueInfo;
	int32_t i4Ret = WLAN_STATUS_SUCCESS;
	int32_t i4DevIdx = -1;
	struct ADAPTER *prAdapter = NULL;

	ASSERT(prWdev);

	do {
		if (!prWdev)
			break;

		WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
		prAdapter = prGlueInfo->prAdapter;
		i4DevIdx = wlanGetDevIdx(prWdev->netdev);
		if (i4DevIdx < 0) {
			DBGLOG(INIT, ERROR, "net_device number exceeds!\n");
			break;
		}

		if (prAdapter && prAdapter->rWifiVar.ucWow)
			kalInitDevWakeup(prGlueInfo->prAdapter,
				wiphy_dev(prWdev->wiphy));

		if (prWdev->netdev->reg_state == NETREG_UNINITIALIZED) {
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
			if (g_fgWlanOnOffHoldRtnlLock)
				i4Ret = register_netdevice(prWdev->netdev);
			else
#endif
				i4Ret = register_netdev(prWdev->netdev);
		}

		if (i4Ret < 0) {
			DBGLOG(INIT, ERROR,
				"Register net_device %d %p failed\n",
				i4DevIdx, prWdev->netdev);
			wlanClearDevIdx(prWdev->netdev);
			i4DevIdx = -1;
			break;
		}

		prGlueInfo->fgIsRegistered = TRUE;
	} while (FALSE);

	return i4DevIdx;	/* success */
}				/* end of wlanNetRegister() */


/*----------------------------------------------------------------------------*/
/*!
 * \brief Unregister the device from the kernel
 *
 * \param[in] prWdev      Pointer to struct wireless_dev.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void wlanNetUnregister(struct wireless_dev *prWdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	if (!prWdev) {
		DBGLOG(INIT, ERROR, "The device context is NULL\n");
		return;
	}

	WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return;
	}

	if (!prGlueInfo->prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL.\n");
		return;
	}

	if (netif_carrier_ok(prGlueInfo->prDevHandler))
		netif_carrier_off(prGlueInfo->prDevHandler);
	netif_tx_stop_all_queues(prGlueInfo->prDevHandler);
	netif_tx_disable(prGlueInfo->prDevHandler);

#if !CFG_SUPPORT_PERSIST_NETDEV
	{
		uint32_t u4Idx = 0;

		for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
			struct net_device *ndev =
				wlanGetAisNetDev(prGlueInfo, u4Idx);

			if (ndev && ndev->reg_state == NETREG_REGISTERED) {
				wlanClearDevIdx(ndev);
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
				if (g_fgWlanOnOffHoldRtnlLock)
					unregister_netdevice(ndev);
				else
#endif
					unregister_netdev(ndev);
			}
		}

		prGlueInfo->fgIsRegistered = FALSE;
	}
#endif
}				/* end of wlanNetUnregister() */

const struct net_device_ops wlan_netdev_ops = {
	.ndo_open = wlanOpen,
	.ndo_stop = wlanStop,
	.ndo_set_rx_mode = wlanSetMulticastList,
	.ndo_get_stats = wlanGetStats,
	.ndo_do_ioctl = wlanDoIOCTL,
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
	.ndo_siocdevprivate = wlanDoPrivIOCTL,
#endif
	.ndo_start_xmit = wlanHardStartXmit,
	.ndo_init = wlanInit,
	.ndo_uninit = wlanUninit,
	.ndo_select_queue = wlanSelectQueue,
	.ndo_set_mac_address = wlanSetMacAddress,
};

const struct net_device_ops *wlanGetNdevOps(void)
{
	return &wlan_netdev_ops;
}
void wlanNvramSetState(enum ENUM_NVRAM_STATE state)
{
	g_NvramFsm = state;
}
enum ENUM_NVRAM_STATE wlanNvramGetState(void)
{
	return g_NvramFsm;
}
#if CFG_WLAN_ASSISTANT_NVRAM
static void wlanNvramUpdateOnTestMode(void)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	enum ENUM_NVRAM_STATE nvrmState;
	struct REG_INFO *prRegInfo = NULL;
	struct ADAPTER *prAdapter = NULL;

	/* <1> Sanity Check */

	if (kalIsHalted()) {
		DBGLOG(INIT, WARN, "device not ready return");
		return;
	}

	if (u4WlanDevNum == 0) {
		DBGLOG(INIT, ERROR,
			   "wlanNvramUpdateOnTestMode invalid!!\n");
		return;
	}

	prGlueInfo = wlanGetGlueInfo();
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, WARN,
			   "prGlueInfo invalid!!\n");
		return;
	}
	if (!wlanIsDriverReady(prGlueInfo,
	WLAN_DRV_READY_CHECK_WLAN_ON | WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}
	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, WARN,
			   "prAdapter invalid!!\n");
		return;
	}
	prRegInfo = &prGlueInfo->rRegInfo;
	if (prRegInfo == NULL) {
		DBGLOG(INIT, WARN,
			   "prRegInfo invalid!!\n");
		return;
	}

	if (prAdapter->fgTestMode == FALSE) {
		DBGLOG(INIT, INFO,
			   "by-pass on Normal mode\n");
		return;
	}

	nvrmState = wlanNvramGetState();

	if (nvrmState == NVRAM_STATE_READY) {
		DBGLOG(RFTEST, INFO,
		"update nvram to fw on test mode!\n");

		if (kalIsConfigurationExist(prGlueInfo) == TRUE)
			wlanLoadManufactureData(prAdapter, prRegInfo);
		else
			DBGLOG(INIT, WARN, "%s: load manufacture data fail\n",
					   __func__);
	}
}
static uint8_t wlanNvramBufHandler(void *ctx,
			const char *buf,
			uint16_t length)
{
	DBGLOG(INIT, INFO, "buf = %p, length = %u\n", buf, length);
	if (buf == NULL || length <= 0)
		return -EFAULT;

	if (length > sizeof(g_aucNvram)) {
		DBGLOG(INIT, ERROR, "is over nvrm size %zu\n",
			sizeof(g_aucNvram));
		return -EINVAL;
	}

	kalMemZero(&g_aucNvram, sizeof(g_aucNvram));
	kalMemZero(&g_aucNvram_OnlyPreCal, sizeof(g_aucNvram_OnlyPreCal));
	if (copy_from_user(g_aucNvram, buf, length)) {
		DBGLOG(INIT, ERROR, "copy nvram fail\n");
		g_NvramFsm = NVRAM_STATE_INIT;
		return -EINVAL;
	}

	g_NvramFsm = NVRAM_STATE_READY;
	DBGLOG(INIT, INFO, "Set NVRAM state[%d]\n", g_NvramFsm);
#if CFG_MTK_ANDROID_WMT
	if (!g_IsPlatCbsRegistered) {
		register_plat_connsys_cbs();
		g_IsPlatCbsRegistered = TRUE;
	}
#endif


	/*do nvram update on test mode then driver sent new NVRAM to FW*/
	wlanNvramUpdateOnTestMode();

	return 0;
}

static uint8_t wlanXonvBufHandler(void *ctx,
			const char *buf,
			uint16_t length)
{
#if CFG_SUPPORT_XONVRAM
	DBGLOG(INIT, INFO, "buf = %p, length = %u\n", buf, length);
	if (buf == NULL || length <= 0)
		return -EFAULT;

	if (length > sizeof(g_rXonvCfg.aucData)) {
		DBGLOG(INIT, ERROR, "is over nvrm size %zu\n",
			sizeof(g_rXonvCfg.aucData));
		return -EINVAL;
	}

	kalMemZero(&g_rXonvCfg.aucData, sizeof(g_rXonvCfg.aucData));
	if (copy_from_user(g_rXonvCfg.aucData, buf, length)) {
		DBGLOG(INIT, ERROR, "copy xo nvram fail\n");
		return -EINVAL;
	}
	g_rXonvCfg.u2DataLen = length;

	DBGLOG(INIT, INFO, "Copy %d bytes from xo nvram\n", length);
#endif
	return 0;
}

#endif

#if (CFG_SUPPORT_CONNFEM == 1 && CFG_CONNFEM_DEFAULT == 1)
uint32_t wlanConnFemGetId(void)
{
	return gu4ConnfemId;
}
#endif

static void wlanCreateWirelessDevice(void)
{
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev[KAL_AIS_NUM] = {NULL};
	unsigned int u4SupportSchedScanFlag = 0;
	uint32_t u4Idx = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	/* 4 <1.1> Create wireless_dev for wlan0 only */
	prWdev[u4Idx] = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!prWdev[u4Idx]) {
		DBGLOG(INIT, ERROR,
			"Allocating memory to wireless_dev context failed\n");
		return;
	}
	prWdev[u4Idx]->iftype = NL80211_IFTYPE_STATION;

	/* 4 <1.2> Create wiphy */
	prWiphy = wiphy_new(&mtk_cfg_ops, sizeof(struct GLUE_INFO *));

	if (!prWiphy) {
		DBGLOG(INIT, ERROR,
		       "Allocating memory to wiphy device failed\n");
		goto free_wdev;
	}

	/* Allocate GLUE_INFO and set priv as pointer to glue structure */
	prGlueInfo = kalMemAlloc(sizeof(struct GLUE_INFO), VIR_MEM_TYPE);
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR,
		       "Allocating memory to GLUE_INFO failed\n");
		goto free_wiphy;
	}
	kalMemSet(prGlueInfo, 0, sizeof(struct GLUE_INFO));
	*((struct GLUE_INFO **) wiphy_priv(prWiphy)) = prGlueInfo;

	/* 4 <1.3> configure wireless_dev & wiphy */
	prWiphy->max_scan_ssids = CFG_SCAN_SSID_MAX_NUM +
				  1; /* include one wildcard ssid */
	prWiphy->max_scan_ie_len = 512;
#if CFG_SUPPORT_SCHED_SCAN
	prWiphy->max_sched_scan_ssids     =
		CFG_SCAN_HIDDEN_SSID_MAX_NUM;
	prWiphy->max_match_sets           =
		CFG_SCAN_SSID_MATCH_MAX_NUM;
	prWiphy->max_sched_scan_ie_len    = CFG_CFG80211_IE_BUF_LEN;
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
	/* In kernel 4.12 or newer,
	 * this is obsoletes - WIPHY_FLAG_SUPPORTS_SCHED_SCAN
	 */
	prWiphy->max_sched_scan_reqs = 1;
#else
	u4SupportSchedScanFlag            =
		WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
#endif
#endif /* CFG_SUPPORT_SCHED_SCAN */
#if CFG_SUPPORT_ADHOC
	prWiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
				   BIT(NL80211_IFTYPE_ADHOC);
#else
	prWiphy->interface_modes = BIT(NL80211_IFTYPE_STATION);
#endif
	prWiphy->bands[KAL_BAND_2GHZ] = &mtk_band_2ghz;
	/* always assign 5Ghz bands here, if the chip is not support 5Ghz,
	 *  bands[KAL_BAND_5GHZ] will be assign to NULL
	 */
	prWiphy->bands[KAL_BAND_5GHZ] = &mtk_band_5ghz;
#if (CFG_SUPPORT_WIFI_6G == 1)
	prWiphy->bands[KAL_BAND_6GHZ] = &mtk_band_6ghz;
	DBGLOG(INIT, INFO, "Support 6G\n");
#endif
	prWiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	prWiphy->cipher_suites = (const u32 *)mtk_cipher_suites;
	prWiphy->n_cipher_suites = ARRAY_SIZE(mtk_cipher_suites);
	prWiphy->flags = WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL
			| u4SupportSchedScanFlag;

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)
	prWiphy->max_num_akm_suites = RSN_MAX_NR_AKN_SUITES;
#endif

#if (CFG_SUPPORT_ROAMING == 1)
	prWiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM;
#endif /* CFG_SUPPORT_ROAMING */

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
	prWiphy->flags |= WIPHY_FLAG_SUPPORTS_MLO;
#endif

#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
#if (CFG_SUPPORT_DFS_MASTER == 1)
	prWiphy->flags |= WIPHY_FLAG_HAS_CHANNEL_SWITCH;
#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
	prWiphy->max_num_csa_counters = 2;
#endif
#endif /* CFG_SUPPORT_DFS_MASTER */
#endif

#if CFG_ENABLE_WIFI_DIRECT
#if KERNEL_VERSION(3, 14, 0) < CFG80211_VERSION_CODE
	prWiphy->max_ap_assoc_sta = P2P_MAXIMUM_CLIENT_COUNT;
#endif
#endif
	cfg80211_regd_set_wiphy(prWiphy);

#if (CFG_SUPPORT_TDLS == 1)
	TDLSEX_WIPHY_FLAGS_INIT(prWiphy->flags);
#if CFG_SUPPORT_TDLS_OFFCHANNEL
	prWiphy->features |= NL80211_FEATURE_TDLS_CHANNEL_SWITCH;
#endif
#endif /* CFG_SUPPORT_TDLS */
	prWiphy->max_remain_on_channel_duration = 5000;
	prWiphy->mgmt_stypes = mtk_cfg80211_ais_default_mgmt_stypes;

#if (CFG_SUPPORT_SCAN_RANDOM_MAC && \
	(KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE))
	prWiphy->features |= NL80211_FEATURE_SCAN_RANDOM_MAC_ADDR;
	prWiphy->features |= NL80211_FEATURE_SCHED_SCAN_RANDOM_MAC_ADDR;
#endif

#if KERNEL_VERSION(4, 10, 0) < CFG80211_VERSION_CODE
	wiphy_ext_feature_set(prWiphy, NL80211_EXT_FEATURE_LOW_SPAN_SCAN);
	wiphy_ext_feature_set(prWiphy,
		NL80211_EXT_FEATURE_FILS_MAX_CHANNEL_TIME);
	wiphy_ext_feature_set(prWiphy,
		NL80211_EXT_FEATURE_ACCEPT_BCAST_PROBE_RESP);
	wiphy_ext_feature_set(prWiphy,
		NL80211_EXT_FEATURE_OCE_PROBE_REQ_HIGH_TX_RATE);
	wiphy_ext_feature_set(prWiphy,
		NL80211_EXT_FEATURE_OCE_PROBE_REQ_DEFERRAL_SUPPRESSION);
	wiphy_ext_feature_set(prWiphy, NL80211_EXT_FEATURE_HIGH_ACCURACY_SCAN);
#endif

#if (CFG_SUPPORT_BCN_PROT == 1) && \
	(KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE)
	wiphy_ext_feature_set(prWiphy,
		NL80211_EXT_FEATURE_BEACON_PROTECTION_CLIENT);
#endif

	prWiphy->features |= NL80211_FEATURE_INACTIVITY_TIMER;

#if CFG_SUPPORT_WPA3
	prWiphy->features |= NL80211_FEATURE_SAE;
#endif

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	wiphy_ext_feature_set(prWiphy, NL80211_EXT_FEATURE_FILS_SK_OFFLOAD);
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#if KERNEL_VERSION(5, 10, 0) <= CFG80211_VERSION_CODE
	wiphy_ext_feature_set(prWiphy,
			      NL80211_EXT_FEATURE_FILS_DISCOVERY);
#if (CFG_SUPPORT_WIFI_6G == 1)
	wiphy_ext_feature_set(prWiphy,
			      NL80211_EXT_FEATURE_UNSOL_BCAST_PROBE_RESP);
#endif
#endif

#if KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
	prWiphy->vendor_commands = mtk_wlan_vendor_ops;
	prWiphy->n_vendor_commands = sizeof(mtk_wlan_vendor_ops) /
				     sizeof(struct wiphy_vendor_command);
	prWiphy->vendor_events = mtk_wlan_vendor_events;
	prWiphy->n_vendor_events = ARRAY_SIZE(
					   mtk_wlan_vendor_events);
#endif
	/* 4 <1.4> wowlan support */
#ifdef CONFIG_PM
#if KERNEL_VERSION(3, 11, 0) <= CFG80211_VERSION_CODE
	prWiphy->wowlan = &mtk_wlan_wowlan_support;
#else
	kalMemCopy(&prWiphy->wowlan, &mtk_wlan_wowlan_support,
		   sizeof(struct wiphy_wowlan_support));
#endif
#endif

#ifdef CONFIG_CFG80211_WEXT
	/* 4 <1.5> Use wireless extension to replace IOCTL */
	prWiphy->wext = NULL;
#endif
	/* initialize semaphore for halt control */
	sema_init(&g_halt_sem, 1);

#if CFG_ENABLE_WIFI_DIRECT
	prWiphy->iface_combinations = p_mtk_iface_combinations_p2p;
	prWiphy->n_iface_combinations =
		mtk_iface_combinations_p2p_num;

	prWiphy->interface_modes |= BIT(NL80211_IFTYPE_AP) |
				    BIT(NL80211_IFTYPE_P2P_CLIENT) |
				    BIT(NL80211_IFTYPE_P2P_GO) |
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
				    BIT(NL80211_IFTYPE_MONITOR) |
#endif
				    BIT(NL80211_IFTYPE_STATION);
	prWiphy->software_iftypes |= BIT(NL80211_IFTYPE_P2P_DEVICE);
	prWiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	prWiphy->flags |= WIPHY_FLAG_HAVE_AP_SME;
	prWiphy->ap_sme_capa = 1;
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_OFFCHANNEL_TX
	prWiphy->flags |= WIPHY_FLAG_OFFCHAN_TX;
#endif /* CFG_ENABLE_OFFCHANNEL_TX */

#if CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT
	/* Enable following to indicate supplicant
	 * to support Beacon report feature
	 */
	prWiphy->features |= NL80211_FEATURE_DS_PARAM_SET_IE_IN_PROBES;
	prWiphy->features |= NL80211_FEATURE_QUIET;
#endif

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_CONTROL_PORT_OVER_NL80211 == 1)
	wiphy_ext_feature_set(prWiphy,
		NL80211_EXT_FEATURE_CONTROL_PORT_OVER_NL80211);
	wiphy_ext_feature_set(prWiphy,
		NL80211_EXT_FEATURE_CONTROL_PORT_OVER_NL80211_TX_STATUS);
#endif

	prWiphy->n_akm_suites = ARRAY_SIZE(mtk_akm_suites);
	prWiphy->akm_suites = mtk_akm_suites;

	if (wiphy_register(prWiphy) < 0) {
		DBGLOG(INIT, ERROR, "wiphy_register error\n");
		goto free_glue_info;
	}
	prWdev[u4Idx]->wiphy = prWiphy;
	gprWdev[u4Idx] = prWdev[u4Idx];

#if CFG_WLAN_ASSISTANT_NVRAM
	register_file_buf_handler(wlanXonvBufHandler, (void *)NULL,
			ENUM_BUF_TYPE_XONV);
	register_file_buf_handler(wlanNvramBufHandler, (void *)NULL,
			ENUM_BUF_TYPE_NVRAM);
#endif

#if ((CFG_MTK_ANDROID_WMT) && (CFG_TESTMODE_WMT_WIFI_ON_SUPPORT))
	register_is_wifi_in_test_mode_handler(glIsWifiInTestMode);
#endif

	DBGLOG(INIT, INFO, "Create wireless device success\n");
	return;

free_glue_info:
	kalMemFree(prGlueInfo, VIR_MEM_TYPE, sizeof(struct GLUE_INFO));
free_wiphy:
	wiphy_free(prWiphy);
free_wdev:
	kfree(prWdev[u4Idx]);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Destroy all wdev (including the P2P device), and unregister wiphy
 *
 * \param (none)
 *
 * \return (none)
 *
 */
/*----------------------------------------------------------------------------*/
static void wlanDestroyAllWdev(void)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	/* There is only one wiphy, avoid that double free the wiphy */
	struct wiphy *wiphy = NULL;
	int i = 0;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	kalMemFree(prGlueInfo, VIR_MEM_TYPE, sizeof(struct GLUE_INFO));

#if CFG_ENABLE_WIFI_DIRECT
	/* free P2P wdev */
	for (i = 0; i < KAL_P2P_NUM; i++) {
		if (gprP2pRoleWdev[i] == NULL)
			continue;
		if (wlanIsAisDev(gprP2pRoleWdev[i]->netdev)) {
			/* This is AIS/AP Interface */
			gprP2pRoleWdev[i] = NULL;
			continue;
		}

		/* Do wiphy_unregister here. Take care the case that the
		 * gprP2pRoleWdev[i] is created by the cfg80211 add iface ops,
		 * And the base P2P dev is in the gprP2pWdev.
		 * Expect that new created gprP2pRoleWdev[i] is freed in
		 * unregister_netdev/mtk_vif_destructor. And gprP2pRoleWdev[i]
		 * is reset as gprP2pWdev in mtk_vif_destructor.
		 */
		if (gprP2pRoleWdev[i] == gprP2pWdev[i])
			gprP2pWdev[i] = NULL;

		wiphy = gprP2pRoleWdev[i]->wiphy;

		kfree(gprP2pRoleWdev[i]);
		gprP2pRoleWdev[i] = NULL;
	}

	/* This case is that gprP2pWdev isn't equal to gprP2pRoleWdev[0]
	 * . The gprP2pRoleWdev[0] is created in the p2p cfg80211 add
	 * iface ops. The two wdev use the same wiphy. Don't double
	 * free the same wiphy.
	 * This part isn't expect occur. Because p2pNetUnregister should
	 * unregister_netdev the new created wdev, and gprP2pRoleWdev[0]
	 * is reset as gprP2pWdev.
	 */
	for (i = 0; i < KAL_P2P_NUM; i++) {
		if (gprP2pWdev[i] != NULL) {
			wiphy = gprP2pWdev[i]->wiphy;
			kfree(gprP2pWdev[i]);
			gprP2pWdev[i] = NULL;
		}
	}
#endif	/* CFG_ENABLE_WIFI_DIRECT */

	/* free AIS wdev */
	if (gprWdev[0]) {
		wiphy = wlanGetWiphy();
		for (i = 0; i < KAL_AIS_NUM; i++) {
			kfree(gprWdev[i]);
			gprWdev[i] = NULL;
		}
	}

	/* unregister & free wiphy */
	if (wiphy) {
		/* set_wiphy_dev(wiphy, NULL): set the wiphy->dev->parent = NULL
		 * The trunk-ce1 does this, but the trunk seems not.
		 */
		/* set_wiphy_dev(wiphy, NULL); */
		wiphy_unregister(wiphy);
		wiphy_free(wiphy);
	}
}

void wlanWakeLockInit(struct GLUE_INFO *prGlueInfo)
{
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(NULL, prGlueInfo->rIntrWakeLock,
			   "WLAN interrupt");
	KAL_WAKE_LOCK_INIT(NULL, prGlueInfo->rTimeoutWakeLock,
			   "WLAN timeout");
#if CFG_SUPPORT_RX_WORK
	KAL_WAKE_LOCK_INIT(NULL, prGlueInfo->rRxWorkerLock,
			   "Rx Worker");
#endif
#if defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
	KAL_WAKE_LOCK_INIT(NULL, prGlueInfo->prDrvOwnWakeLock,
			   "WLAN Drv Own");
#endif
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	KAL_WAKE_LOCK_INIT(NULL, prGlueInfo->rTxPowerEmiWakeLock,
			   "Tx Power");
#endif
#endif
}

void wlanWakeLockUninit(struct GLUE_INFO *prGlueInfo)
{
#if CFG_ENABLE_WAKE_LOCK
	if (KAL_WAKE_LOCK_ACTIVE(NULL, prGlueInfo->rIntrWakeLock))
		KAL_WAKE_UNLOCK(NULL, prGlueInfo->rIntrWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, prGlueInfo->rIntrWakeLock);

	if (KAL_WAKE_LOCK_ACTIVE(NULL,
				 prGlueInfo->rTimeoutWakeLock))
		KAL_WAKE_UNLOCK(NULL, prGlueInfo->rTimeoutWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, prGlueInfo->rTimeoutWakeLock);

#if CFG_SUPPORT_RX_WORK
	if (KAL_WAKE_LOCK_ACTIVE(NULL,
				 prGlueInfo->rRxWorkerLock))
		KAL_WAKE_UNLOCK(NULL, prGlueInfo->rRxWorkerLock);
	KAL_WAKE_LOCK_DESTROY(NULL, prGlueInfo->rRxWorkerLock);
#endif
#if defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
	if (KAL_WAKE_LOCK_ACTIVE(NULL,
				 prGlueInfo->prDrvOwnWakeLock))
		KAL_WAKE_UNLOCK(NULL, prGlueInfo->prDrvOwnWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, prGlueInfo->prDrvOwnWakeLock);
#endif

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	if (KAL_WAKE_LOCK_ACTIVE(NULL,
				 prGlueInfo->rTxPowerEmiWakeLock))
		KAL_WAKE_UNLOCK(NULL, prGlueInfo->rTxPowerEmiWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL, prGlueInfo->rTxPowerEmiWakeLock);
#endif

#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method for creating Linux NET4 struct net_device object and the
 *        private data(prGlueInfo and prAdapter). Setup the IO address to the
 *        HIF.
 *        Assign the function pointer to the net_device object
 *
 * \param[in] pvData     Memory address for the device
 *
 * \retval Not null      The wireless_dev object.
 * \retval NULL          Fail to create wireless_dev object
 */
/*----------------------------------------------------------------------------*/
static struct lock_class_key rSpinKey[SPIN_LOCK_NUM];
static struct lock_class_key rMutexKey[MUTEX_NUM];
#if CFG_SUPPORT_RX_PAGE_POOL
static struct lock_class_key rMutexPagePoolKey[PAGE_POOL_NUM];
#endif
struct wireless_dev *wlanNetCreate(void *pvData,
		void *pvDriverData)
{
	struct wireless_dev *prWdev = gprWdev[0];
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t i;
	void *pvDev;
	struct device *prDev;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct wiphy *prWiphy = NULL;
	struct net_device *prDevHandler;

	uint8_t *prInfName = NULL;

	if (prWdev == NULL) {
		DBGLOG(INIT, ERROR,
		       "No wireless dev exist, abort power on\n");
		return NULL;
	}

	/* The gprWdev is created at initWlan() and isn't reset when the
	 * disconnection occur. That cause some issue.
	 */
	prWiphy = prWdev->wiphy;
	if (gprWdev[0]
#if CFG_SUPPORT_PERSIST_NETDEV
		&& !gprWdev[0]->netdev
#endif
		) {
		memset(gprWdev[0], 0, sizeof(struct wireless_dev));
		gprWdev[0]->wiphy = prWiphy;
		gprWdev[0]->iftype = NL80211_IFTYPE_STATION;
	}


#if (CFG_SUPPORT_SINGLE_SKU == 1)
	/* XXX: ref from cfg80211_regd_set_wiphy().
	 * The error case: Sometimes after unplug/plug usb, the wlan0 STA can't
	 * scan correctly (FW doesn't do scan). The usb_probe message:
	 * "mtk_reg_notify:(RLM ERROR) Invalid REG state happened. state = 0x6".
	 */
	if (rlmDomainGetCtrlState() == REGD_STATE_INVALID)
		rlmDomainResetCtrlInfo(TRUE);
#endif

	/* 4 <1.3> co-relate wiphy & prDev */
	glGetDev(pvData, &pvDev);
	if (!pvDev)
		DBGLOG(INIT, ERROR, "unable to get struct dev for wlan\n");
	prDev = (struct device *)pvDev;
	/* Some kernel API (ex: cfg80211_get_drvinfo) will use wiphy_dev().
	 * Without set_wiphy_dev(prWdev->wiphy, prDev), those API will crash.
	 */
	set_wiphy_dev(prWdev->wiphy, prDev);

	/* 4 <2> Create Glue structure */
	WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
	kalMemZero(prGlueInfo, sizeof(struct GLUE_INFO));

	/* 4 <2.1> Create Adapter structure */
	prAdapter = (struct ADAPTER *) wlanAdapterCreate(
			    prGlueInfo);

	if (!prAdapter) {
		DBGLOG(INIT, ERROR,
		       "Allocating memory to adapter failed\n");
		glClearHifInfo(prGlueInfo);
		return NULL;
	}

	DBGLOG(INIT, INFO, "Adapter allocated: %px\n",
			prAdapter);
	if (prGlueInfo->prAdapter)
		DBGLOG(INIT, WARN, "Adapter is not null: %px\n",
			prGlueInfo->prAdapter);

	prChipInfo = ((struct mt66xx_hif_driver_data *)
		      pvDriverData)->chip_info;
	prAdapter->chip_info = prChipInfo;
	prAdapter->fw_flavor = ((struct mt66xx_hif_driver_data *)
		pvDriverData)->fw_flavor;
	prGlueInfo->prAdapter = prAdapter;

	/* 4 <3> Initialize Glue structure */
	/* 4 <3.1> Create net device */

#ifdef CFG_DRIVER_INF_NAME_CHANGE

	if (kalStrLen(gprifnamesta) > 0) {
		prInfName = kalStrCat(gprifnamesta, "%d");
		DBGLOG(INIT, WARN, "Station ifname customized, use %s\n",
		       prInfName);
	} else
#endif /* CFG_DRIVER_INF_NAME_CHANGE */
		prInfName = NIC_INF_NAME;



#if CFG_SUPPORT_PERSIST_NETDEV
	if (!gprWdev[0]->netdev) {
		prDevHandler = alloc_netdev_mq(
			sizeof(struct NETDEV_PRIVATE_GLUE_INFO),
			prInfName,
#if KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
			NET_NAME_PREDICTABLE,
#endif
			ether_setup,
			CFG_MAX_TXQ_NUM);
	} else
		prDevHandler = gprWdev[0]->netdev;
#else
	prDevHandler = alloc_netdev_mq(
		sizeof(struct NETDEV_PRIVATE_GLUE_INFO),
		prInfName,
#if KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
		NET_NAME_PREDICTABLE,
#endif
		ether_setup,
		CFG_MAX_TXQ_NUM);
#endif /* end of CFG_SUPPORT_PERSIST_NETDEV */

	if (!prDevHandler) {
		DBGLOG(INIT, ERROR,
			"Allocating memory to net_device context failed\n");
		goto netcreate_err;
	}

#if KERNEL_VERSION(4, 14, 0) <= CFG80211_VERSION_CODE
	prDevHandler->priv_destructor = mtk_vif_destructor;
#else
	prDevHandler->destructor = mtk_vif_destructor;
#endif

	/* Device can help us to save at most 3000 packets,
	 * after we stopped queue
	 */
	prDevHandler->tx_queue_len = 3000;
	DBGLOG(INIT, TRACE, "net_device prDev(0x%p) allocated\n",
		prDevHandler);

	/* 4 <3.1.1> Initialize net device varaiables */
	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prDevHandler);
	prNetDevPrivate->prGlueInfo = prGlueInfo;

	prDevHandler->needed_headroom =
		NIC_TX_DESC_AND_PADDING_LENGTH +
		prChipInfo->txd_append_size;
	prDevHandler->netdev_ops = &wlan_netdev_ops;
#ifdef CONFIG_WIRELESS_EXT
	prDevHandler->wireless_handlers = &wext_handler_def;
#endif
	netif_carrier_off(prDevHandler);
	netif_tx_stop_all_queues(prDevHandler);
	kalResetStats(prDevHandler);

	/* 4 <3.1.2> co-relate with wiphy bi-directionally */
	prDevHandler->ieee80211_ptr = gprWdev[0];

	gprWdev[0]->netdev = prDevHandler;

	/* 4 <3.1.3> co-relate net device & prDev */
	SET_NETDEV_DEV(prDevHandler,
			wiphy_dev(prWdev->wiphy));
	prGlueInfo->fgIsInSuspend = 0;

	/* 4 <3.1.4> set device to glue */
	prGlueInfo->prDev = prDev;

	prGlueInfo->prDevHandler = gprWdev[0]->netdev;
	prGlueInfo->ePowerState = ParamDeviceStateD0;
#if !CFG_SUPPORT_PERSIST_NETDEV
	prGlueInfo->fgIsRegistered = FALSE;
#endif
	prGlueInfo->prScanRequest = NULL;
	prGlueInfo->prSchedScanRequest = NULL;


#if CFG_SUPPORT_PASSPOINT
	/* Init DAD */
	prGlueInfo->fgIsDad = FALSE;
	prGlueInfo->fgIs6Dad = FALSE;
	kalMemZero(prGlueInfo->aucDADipv4, 4);
	kalMemZero(prGlueInfo->aucDADipv6, 16);
#endif /* CFG_SUPPORT_PASSPOINT */

	init_completion(&prGlueInfo->rScanComp);
	init_completion(&prGlueInfo->rHaltComp);
	init_completion(&prGlueInfo->rPendComp);

#if CFG_SUPPORT_MULTITHREAD
	init_completion(&prGlueInfo->rHifHaltComp);
	init_completion(&prGlueInfo->rRxHaltComp);
#endif
#if CFG_SUPPORT_NAN
	init_completion(&prGlueInfo->rNanHaltComp);
#endif

#if CFG_SUPPORT_NCHO
	init_completion(&prGlueInfo->rAisChGrntComp);
#endif

	/* initialize timer for OID timeout checker */
	kalOsTimerInitialize(prGlueInfo, kalTimeoutHandler);

	for (i = 0; i < SPIN_LOCK_NUM; i++) {
		spin_lock_init(&prGlueInfo->rSpinLock[i]);
		lockdep_set_class(&prGlueInfo->rSpinLock[i], &rSpinKey[i]);
	}

	for (i = 0; i < MUTEX_NUM; i++) {
		mutex_init(&prGlueInfo->arMutex[i]);
		lockdep_set_class(&prGlueInfo->arMutex[i], &rMutexKey[i]);
	}

#if CFG_SUPPORT_RX_PAGE_POOL
	for (i = 0; i < PAGE_POOL_NUM; i++) {
		mutex_init(&prGlueInfo->arMutexPagePool[i]);
		lockdep_set_class(&prGlueInfo->arMutexPagePool[i],
			&rMutexPagePoolKey[i]);
	}
#endif

	/* initialize semaphore for ioctl */
	sema_init(&prGlueInfo->ioctl_sem, 1);

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
	/* initialize SDIO read-write pattern control */
	prGlueInfo->fgEnSdioTestPattern = FALSE;
	prGlueInfo->fgIsSdioTestInitialized = FALSE;
#endif

	/* 4 <8> Init Queues */
	init_waitqueue_head(&prGlueInfo->waitq);
	QUEUE_INITIALIZE(&prGlueInfo->rCmdQueue);
	prGlueInfo->i4TxPendingCmdNum = 0;
	QUEUE_INITIALIZE(&prGlueInfo->rTxQueue);
	glSetHifInfo(prGlueInfo, (unsigned long) pvData);

	/* Init wakelock */
	wlanWakeLockInit(prGlueInfo);

	/* main thread is created in this function */
#if CFG_SUPPORT_MULTITHREAD
	init_waitqueue_head(&prGlueInfo->waitq_rx);
	init_waitqueue_head(&prGlueInfo->waitq_hif);

	prGlueInfo->u4TxThreadPid = 0xffffffff;
	prGlueInfo->u4RxThreadPid = 0xffffffff;
	prGlueInfo->u4HifThreadPid = 0xffffffff;
#endif

#if (CFG_CE_ASSERT_DUMP == 1)
	init_waitqueue_head(&(prGlueInfo->waitq_coredump));
	skb_queue_head_init(&(prGlueInfo->rCoreDumpSkbQueue));
#endif

	return prWdev;

netcreate_err:
	if (prAdapter != NULL) {
		wlanAdapterDestroy(prAdapter);
		prAdapter = NULL;
	}
	prGlueInfo->prDevHandler = NULL;

	return prWdev;
}				/* end of wlanNetCreate() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Destroying the struct net_device object and the private data.
 *
 * \param[in] prWdev      Pointer to struct wireless_dev.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanNetDestroy(struct wireless_dev *prWdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;

	ASSERT(prWdev);

	if (!prWdev) {
		DBGLOG(INIT, ERROR, "The device context is NULL\n");
		return;
	}

	/* prGlueInfo is allocated with net_device */
	WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
	ASSERT(prGlueInfo);
	if (prGlueInfo->prAdapter)
		DBGLOG(INIT, INFO, "Prepare to Destroy Adapter: %px\n",
			prGlueInfo->prAdapter);
	else
		DBGLOG(INIT, WARN, "Adapter is null\n");

#if (CFG_CE_ASSERT_DUMP == 1)
	skb_queue_purge(&(prGlueInfo->rCoreDumpSkbQueue));
#endif

	/* prWdev: base AIS dev
	 * Because the interface dev (ex: usb_device) would be free
	 * after un-plug event. Should set the wiphy->dev->parent which
	 * pointer to the interface dev to NULL. Otherwise, the corresponding
	 * system operation (poweroff, suspend) might reference it.
	 * set_wiphy_dev(wiphy, NULL): set the wiphy->dev->parent = NULL
	 * The set_wiphy_dev(prWdev->wiphy, prDev) is done in wlanNetCreate.
	 * But that is after wiphy_register, and will cause exception in
	 * wiphy_unregister(), if do not set_wiphy_dev(wiphy, NULL).
	 */
	set_wiphy_dev(prWdev->wiphy, NULL);

	/* destroy kal OS timer */
	kalCancelTimer(prGlueInfo);

	glClearHifInfo(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	prGlueInfo->prAdapter = NULL;
	wlanAdapterDestroy(prAdapter);

	/* Free net_device and private data, which are allocated by
	 * alloc_netdev().
	 */
#if CFG_SUPPORT_PERSIST_NETDEV
#ifdef CONFIG_WIRELESS_EXT
	{
		uint32_t u4Idx = 0;

		for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
			struct net_device *ndev =
				wlanGetAisNetDev(prGlueInfo, u4Idx);

			if (ndev) {
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
				if (!g_fgWlanOnOffHoldRtnlLock)
#endif
					rtnl_lock();
				ndev->wireless_handlers = NULL;
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
				if (!g_fgWlanOnOffHoldRtnlLock)
#endif
					rtnl_unlock();
			}
		}
	}
#endif
#else
	wlanFreeNetDev();
#endif
	/* gPrDev is assigned by prGlueInfo->prDevHandler,
	 * set NULL to this global variable.
	 */
	gPrDev = NULL;

}				/* end of wlanNetDestroy() */

#if (CFG_SUPPORT_IGMP_OFLD == 1)

#define IGMP_RESP_TYPE				0x22
#define IGMP_RESP_PREFIX_SIZE			8
#define IGMP_RESP_TYPE_OFFSET			0
#define IGMP_RESP_GRP_ADDR_COUNT_OFFSET		6
#define IGMP_RESP_GRP_TYPE_OFFSET		0
#define IGMP_RESP_GRP_SRC_COUNT_OFFSET		2
#define IGMP_RESP_GRP_MC_ADDR_OFFSET		4

void wlanSetMcGroupList(struct GLUE_INFO *prGlueInfo,
	struct net_device *prDev,
	uint8_t fgEnable,
	uint8_t *prNum,
	uint8_t *prAddrList)
{
	uint16_t u2GroupAddrCount = 0, u2TotalLen = 0;
	uint8_t *prOfldBuf = NULL, *prPos = NULL;
	uint32_t u4SetInfoLen = 0;
	struct PARAM_OFLD_INFO rInfo;

	struct in_device *in_dev;
	struct ip_mc_list *pmc = NULL;

	uint8_t i;
	uint8_t aucMcIpMask[IPV4_ADDR_LEN] = {255, 128, 0, 0};
	uint8_t aucMcAddr[MAC_ADDR_LEN]	 = {
			0x01, 0x00, 0x5E, 0x00, 0x00, 0x00};
	uint8_t aucDstMcAddr[MAC_ADDR_LEN] = {
			0x01, 0x00, 0x5E, 0x00, 0x00, 0x01};

	kalMemZero(&rInfo, sizeof(struct PARAM_OFLD_INFO));
	rInfo.ucType = PKT_OFLD_TYPE_IGMP;

	if (fgEnable) {

		down(&g_halt_sem);
		if (g_u4HaltFlag) {
			up(&g_halt_sem);
			return;
		}

		in_dev = __in_dev_get_rtnl(prDev);

		rInfo.ucOp = PKT_OFLD_OP_ENABLE;

		for (pmc = rtnl_dereference(in_dev->mc_list);
			pmc != NULL; pmc = rtnl_dereference(pmc->next_rcu)) {
			uint8_t ucType = 0;
			struct ip_sf_list *psf, *psf_next, **psf_list;

			if (htonl(pmc->multiaddr) == 0xe0000001)
				continue;

			if (u2GroupAddrCount == 0) {
				prOfldBuf = &rInfo.aucBuf[0];
				kalMemZero(prOfldBuf, PKT_OFLD_BUF_SIZE);
				prOfldBuf[IGMP_RESP_TYPE_OFFSET] =
					IGMP_RESP_TYPE;
				prPos = prOfldBuf + IGMP_RESP_PREFIX_SIZE;
				u2TotalLen += IGMP_RESP_PREFIX_SIZE;
			}
			if (pmc->sfcount[MCAST_EXCLUDE])
				ucType = IGMPV3_MODE_IS_EXCLUDE;
			else
				ucType = IGMPV3_MODE_IS_INCLUDE;

			psf_list = &pmc->sources;

			prPos[IGMP_RESP_GRP_TYPE_OFFSET] = ucType;
			prPos += IGMP_RESP_GRP_MC_ADDR_OFFSET;
			u2TotalLen += IGMP_RESP_GRP_MC_ADDR_OFFSET;
			kalMemCopy(prPos, &pmc->multiaddr, IPV4_ADDR_LEN);
			DBGLOG(INIT, TRACE, "Addr %d.%d.%d.%d.\n",
				prPos[0], prPos[1], prPos[2], prPos[3]);

			if (prAddrList &&
					u2GroupAddrCount < MAX_NUM_GROUP_ADDR) {
				kalMemCopy(
				&prAddrList[MAC_ADDR_LEN * u2GroupAddrCount],
				aucMcAddr, MAC_ADDR_LEN);

				for (i = 0; i < IPV4_ADDR_LEN; i++) {
					prAddrList[
					MAC_ADDR_LEN * u2GroupAddrCount + i + 2]
						|= prPos[i] & (~aucMcIpMask[i]);
				}
			}
			prPos += IPV4_ADDR_LEN;
			u2TotalLen += IPV4_ADDR_LEN;
			if (*psf_list) {
				uint16_t u2SrcCnt = 0;

				for (psf = *psf_list; psf; psf = psf_next) {
					psf_next = psf->sf_next;
					kalMemCopy(prPos, &psf->sf_inaddr,
							IPV4_ADDR_LEN);

					DBGLOG(INIT, TRACE,
						"Src Addr %d.%d.%d.%d.\n",
						prPos[0], prPos[1],
						prPos[2], prPos[3]);

					prPos += IPV4_ADDR_LEN;
					u2TotalLen += IPV4_ADDR_LEN;
					u2SrcCnt++;
				}

				WLAN_SET_FIELD_BE16(
					&prPos[IGMP_RESP_GRP_SRC_COUNT_OFFSET],
					u2SrcCnt);
			}
			u2GroupAddrCount++;
		}

		up(&g_halt_sem);
		if (u2GroupAddrCount > 0) {
			if (prNum) {
				kalMemCopy(
				&prAddrList[MAC_ADDR_LEN * u2GroupAddrCount],
				aucDstMcAddr, MAC_ADDR_LEN);
				*prNum = u2GroupAddrCount + 1;
			}

			WLAN_SET_FIELD_BE16(
				&prOfldBuf[IGMP_RESP_GRP_ADDR_COUNT_OFFSET],
				u2GroupAddrCount);

			rInfo.u4TotalLen = (prPos - prOfldBuf);
			rInfo.u4BufLen = rInfo.u4TotalLen;

			kalIoctl(prGlueInfo,
				wlanoidSetOffloadInfo, &rInfo,
				sizeof(struct PARAM_OFLD_INFO),
				&u4SetInfoLen);
		}
	} else {
		rInfo.ucOp = PKT_OFLD_OP_DISABLE;
		kalIoctl(prGlueInfo,
				wlanoidSetOffloadInfo, &rInfo,
				sizeof(struct PARAM_OFLD_INFO),
				&u4SetInfoLen);
	}

}

#endif /* CFG_SUPPORT_IGMP_OFLD */
void wlanSetSuspendMode(struct GLUE_INFO *prGlueInfo,
			u_int8_t fgEnable)
{
	struct net_device *prDev = NULL;
	uint32_t u4PacketFilter = 0;
	uint32_t u4SetInfoLen = 0;
	uint32_t u4Idx = 0;

	if (!prGlueInfo)
		return;

	prGlueInfo->prAdapter->fgIsInSuspendMode = fgEnable;

	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		prDev = wlanGetAisNetDev(prGlueInfo, u4Idx);
		if (!prDev)
			continue;

		/* new filter should not include p2p mask */
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
		u4PacketFilter =
			prGlueInfo->prAdapter->u4OsPacketFilter &
			(~PARAM_PACKET_FILTER_P2P_MASK);
#endif
		if (kalIoctl(prGlueInfo, wlanoidSetCurrentPacketFilter,
			&u4PacketFilter, sizeof(u4PacketFilter),
			&u4SetInfoLen) != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR, "set packet filter failed.\n");

#if (!CFG_SUPPORT_DROP_ALL_MC_PACKET && \
	(!CFG_WOW_SUPPORT || CFG_SUPPORT_MDNS_WHITELIST == 1))
		if (fgEnable) {
			/* Prepare IPv6 RA packet when suspend */
			struct PARAM_MULTICAST_LIST rMcAddrList;
			uint8_t ucNum = 0;
			uint8_t aucDefaultAddr[MAC_ADDR_LEN] = {
					0x33, 0x33, 0, 0, 0, 1};

#if (CFG_SUPPORT_MDNS_WHITELIST == 1)
			uint8_t aucMdnsIpv4Addr[MAC_ADDR_LEN] = {
					0x01, 0x00, 0x5E, 0, 0, 0xFB};
			uint8_t aucMdnsIpv6Addr[MAC_ADDR_LEN] = {
					0x33, 0x33, 0, 0, 0, 0xFB};
#endif
			kalMemZero(&rMcAddrList,
					sizeof(struct PARAM_MULTICAST_LIST));

#if (CFG_SUPPORT_IGMP_OFLD == 1)
			DBGLOG(INIT, WARN,
					"Processing u4Idx %d\n", u4Idx);
			wlanSetMcGroupList(prGlueInfo, prDev, TRUE,
						&ucNum,
						&rMcAddrList.aucMcAddrList[0]);
#endif
			if (ucNum < MAX_NUM_GROUP_ADDR) {
				COPY_MAC_ADDR(
					&rMcAddrList.aucMcAddrList[ucNum],
					aucDefaultAddr);
				ucNum++;
			}
#if (CFG_SUPPORT_MDNS_WHITELIST == 1)
			if (ucNum < MAX_NUM_GROUP_ADDR) {
				COPY_MAC_ADDR(
					&rMcAddrList.aucMcAddrList[ucNum],
					aucMdnsIpv4Addr);
				ucNum++;
			}

			if (ucNum < MAX_NUM_GROUP_ADDR) {
				COPY_MAC_ADDR(
					&rMcAddrList.aucMcAddrList[ucNum],
					aucMdnsIpv6Addr);
				ucNum++;
			}
#endif
			rMcAddrList.ucBssIdx = u4Idx;
			rMcAddrList.ucAddrNum = ucNum;
			rMcAddrList.fgIsOid = TRUE;
			kalIoctl(prGlueInfo,
				wlanoidSetMulticastList, &rMcAddrList,
				sizeof(struct PARAM_MULTICAST_LIST),
				&u4SetInfoLen);
		} else if (u4PacketFilter & PARAM_PACKET_FILTER_MULTICAST) {
			/* Prepare multicast address list when resume */
			struct netdev_hw_addr *ha;
			struct PARAM_MULTICAST_LIST rMcAddrList;
			uint32_t i = 0;

			down(&g_halt_sem);
			if (g_u4HaltFlag) {
				up(&g_halt_sem);
				return;
			}

			kalMemZero(&rMcAddrList,
					sizeof(struct PARAM_MULTICAST_LIST));

			/* Avoid race condition with kernel net subsystem */
			netif_addr_lock_bh(prDev);

			netdev_for_each_mc_addr(ha, prDev) {
				if (i < MAX_NUM_GROUP_ADDR) {
					COPY_MAC_ADDR(
						&rMcAddrList.aucMcAddrList[i],
						ha->addr);
					i++;
				}
			}

			netif_addr_unlock_bh(prDev);

			up(&g_halt_sem);

			rMcAddrList.ucBssIdx = u4Idx;
			rMcAddrList.ucAddrNum = i;
			rMcAddrList.fgIsOid = TRUE;
			kalIoctl(prGlueInfo, wlanoidSetMulticastList,
				&rMcAddrList,
				sizeof(struct PARAM_MULTICAST_LIST),
				 &u4SetInfoLen);

#if (CFG_SUPPORT_IGMP_OFLD == 1)
			wlanSetMcGroupList(prGlueInfo, prDev, FALSE,
						NULL, NULL);
#endif /* CFG_SUPPORT_IGMP_OFLD */

		}
#endif

#if (CFG_SUPPORT_SCREENON_OFLD == 1)
		kalSetNetAddressFromInterface(prGlueInfo, prDev, TRUE);
#else
		kalSetNetAddressFromInterface(prGlueInfo, prDev, fgEnable);
#endif
		wlanNotifyFwSuspend(prGlueInfo, prDev, fgEnable);
	}

#if CFG_SUPPORT_NAN
	if (prGlueInfo->prAdapter->fgIsNANRegistered) {
		if (fgEnable) {
			DBGLOG(NAN, INFO,
				"Enter suspend mode, SetDWInterval 8\n");
			nanDevSetDWInterval(prGlueInfo->prAdapter, 8);
		} else {
			DBGLOG(NAN, INFO,
				"Leave suspend mode, SetDWInterval 1\n");
			nanDevSetDWInterval(prGlueInfo->prAdapter, 1);
		}
	}
#endif
}

#if CFG_ENABLE_EARLY_SUSPEND
static struct early_suspend wlan_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
};

static void wlan_early_suspend(struct early_suspend *h)
{
	struct net_device *prDev = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;

	/* 4 <1> Sanity Check */
	if ((u4WlanDevNum == 0)
	    && (u4WlanDevNum > CFG_MAX_WLAN_DEVICES)) {
		DBGLOG(INIT, ERROR,
		       "wlanLateResume u4WlanDevNum==0 invalid!!\n");
		return;
	}

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (!prDev)
		return;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	if (!prGlueInfo)
		return;

	DBGLOG(INIT, INFO, "********<%s>********\n", __func__);

	if (prGlueInfo->fgIsInSuspendMode == TRUE) {
		DBGLOG(INIT, INFO, "%s: Already in suspend mode, SKIP!\n",
		       __func__);
		return;
	}

	prGlueInfo->fgIsInSuspendMode = TRUE;

	wlanSetSuspendMode(prGlueInfo, TRUE);
	p2pSetSuspendMode(prGlueInfo, TRUE);
}

static void wlan_late_resume(struct early_suspend *h)
{
	struct net_device *prDev = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;

	/* 4 <1> Sanity Check */
	if ((u4WlanDevNum == 0)
	    && (u4WlanDevNum > CFG_MAX_WLAN_DEVICES)) {
		DBGLOG(INIT, ERROR,
		       "wlanLateResume u4WlanDevNum==0 invalid!!\n");
		return;
	}

	prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	if (!prDev)
		return;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	if (!prGlueInfo)
		return;

	DBGLOG(INIT, INFO, "********<%s>********\n", __func__);

	if (prGlueInfo->fgIsInSuspendMode == FALSE) {
		DBGLOG(INIT, INFO, "%s: Not in suspend mode, SKIP!\n",
		       __func__);
		return;
	}

	prGlueInfo->fgIsInSuspendMode = FALSE;

	/* 4 <2> Set suspend mode for each network */
	wlanSetSuspendMode(prGlueInfo, FALSE);
#if CFG_ENABLE_WIFI_DIRECT
	p2pSetSuspendMode(prGlueInfo, FALSE);
#endif
}
#endif

#if (CFG_MTK_ANDROID_WMT || WLAN_INCLUDE_PROC) && CFG_ENABLE_WIFI_DIRECT

void reset_p2p_mode(struct GLUE_INFO *prGlueInfo,
	uint8_t fgIsRtnlLockAcquired)
{
	struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rSetP2P;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;

	if (!prGlueInfo)
		return;

	rSetP2P.u4Enable = 0;
	rSetP2P.u4Mode = 0;
	rSetP2P.fgIsRtnlLockAcquired = fgIsRtnlLockAcquired;

	p2pNetUnregister(prGlueInfo, fgIsRtnlLockAcquired);

	rWlanStatus = kalIoctl(prGlueInfo, wlanoidSetP2pMode,
			(void *) &rSetP2P,
			sizeof(struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT),
			&u4BufLen);

	if (rWlanStatus != WLAN_STATUS_SUCCESS)
		p2pRemove(prGlueInfo, fgIsRtnlLockAcquired);

	DBGLOG(INIT, INFO,
			"ret = 0x%08x\n", (uint32_t) rWlanStatus);
}

int set_p2p_mode_handler_wrapper(struct net_device *netdev,
		struct PARAM_CUSTOM_P2P_SET_STRUCT p2pmode)
{
	struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rP2pmodeWithLock;
	int ret;

	DBGLOG(INIT, INFO, "set p2p enable[%d], mode[%d]\n",
		p2pmode.u4Enable, p2pmode.u4Mode);

	rP2pmodeWithLock.u4Enable = p2pmode.u4Enable;
	rP2pmodeWithLock.u4Mode = p2pmode.u4Mode;

	rP2pmodeWithLock.fgIsRtnlLockAcquired = TRUE;
	rtnl_lock();
	ret = set_p2p_mode_handler(netdev, rP2pmodeWithLock);
	rtnl_unlock();

	return ret;
}

int set_p2p_mode_handler(struct net_device *netdev,
			 struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT p2pmode)
{
	struct GLUE_INFO *prGlueInfo = *((struct GLUE_INFO **)
					 netdev_priv(netdev));
	struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rSetP2P;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;

	if (!prGlueInfo)
		return -1;

#if (CFG_MTK_ANDROID_WMT)
	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(INIT, ERROR, "adapter is not ready\n");
		return -1;
	}
#endif /*CFG_MTK_ANDROID_WMT*/

	/* Remember original ifindex for reset case */
	if (kalIsResetting()) {
		struct GL_P2P_INFO *prP2PInfo = NULL;
		int i = 0;

		for (i = 0 ; i < KAL_P2P_NUM; i++) {
			prP2PInfo = prGlueInfo->prP2PInfo[i];

			if (!prP2PInfo || !prP2PInfo->aprRoleHandler
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
				|| !prP2PInfo->prWdev
#endif
				)
				continue;
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
			/* Only restore sap part */
			if (prP2PInfo->prWdev->iftype != NL80211_IFTYPE_AP)
				continue;
#endif

			g_u4DevIdx[i] =
				prP2PInfo->aprRoleHandler->ifindex;
		}

		if (prGlueInfo->prAdapter->rWifiVar.u4RegP2pIfAtProbe) {
			DBGLOG(INIT, WARN, "Resetting p2p mode at probe\n");
			return 0;
		}
	}

	/* Resetting p2p mode if registered to avoid launch KE */
	if (p2pmode.u4Enable
		&& prGlueInfo->prAdapter->fgIsP2PRegistered
		&& !kalIsResetting()) {
		DBGLOG(INIT, WARN, "Resetting p2p mode\n");
		reset_p2p_mode(prGlueInfo, p2pmode.fgIsRtnlLockAcquired);
	}

	rSetP2P.u4Enable = p2pmode.u4Enable;
	rSetP2P.u4Mode = p2pmode.u4Mode;
	rSetP2P.fgIsRtnlLockAcquired = p2pmode.fgIsRtnlLockAcquired;

	if ((!rSetP2P.u4Enable) && (kalIsResetting() == FALSE))
		p2pNetUnregister(prGlueInfo, p2pmode.fgIsRtnlLockAcquired);

	rWlanStatus = kalIoctl(prGlueInfo, wlanoidSetP2pMode,
			(void *) &rSetP2P,
			sizeof(struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT),
			&u4BufLen);

	DBGLOG(INIT, INFO,
		"Mode%d: ret = 0x%08x, p2p reg = %d, resetting = %d\n",
		rSetP2P.u4Mode,
		(uint32_t) rWlanStatus,
		prGlueInfo->prAdapter->fgIsP2PRegistered,
		kalIsResetting());


	/* Need to check fgIsP2PRegistered, in case of whole chip reset.
	 * in this case, kalIOCTL return success always,
	 * and prGlueInfo->prP2PInfo[0] may be NULL
	 */
	if ((rSetP2P.u4Enable)
	    && (prGlueInfo->prAdapter->fgIsP2PRegistered)
	    && (kalIsResetting() == FALSE))
		p2pNetRegister(prGlueInfo, p2pmode.fgIsRtnlLockAcquired);

	return 0;
}

#endif

#if CFG_SUPPORT_EASY_DEBUG
/*----------------------------------------------------------------------------*/
/*!
 * \brief parse config from wifi.cfg
 *
 * \param[in] prAdapter
 *
 * \retval VOID
 */
/*----------------------------------------------------------------------------*/
void wlanGetParseConfig(struct ADAPTER *prAdapter)
{
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen;

	wlanCfgInit(prAdapter, NULL, 0, 0);
	u4ConfigReadLen = 0;

	if (kalRequestFirmware("wifi_sigma.cfg", &pucConfigBuf,
		   &u4ConfigReadLen, TRUE,
		   prAdapter->prGlueInfo->prDev) == 0) {
		/* ToDo:: Nothing */
	} else if (kalRequestFirmware("wifi.cfg", &pucConfigBuf,
		   &u4ConfigReadLen, TRUE,
		   prAdapter->prGlueInfo->prDev) == 0) {
		/* ToDo:: Nothing */
	}

	if (pucConfigBuf) {
		wlanCfgParse(prAdapter, pucConfigBuf, u4ConfigReadLen,
			TRUE);
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);
	}
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief get config from wifi.cfg
 *
 * \param[in] prAdapter
 *
 * \retval VOID
 */
/*----------------------------------------------------------------------------*/
void wlanGetConfig(struct ADAPTER *prAdapter)
{
#ifndef CFG_WIFI_CFG_FN
#define WIFI_CFG_FN	"wifi.cfg"
#else
#define WIFI_CFG_FN	CFG_WIFI_CFG_FN
#endif
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen;
#if WLAN_INCLUDE_SYS
	uint8_t *pucIniBuf = NULL;
	uint32_t u4ConfigReadLen2 = 0;
	uint8_t *pucMergedBuf = NULL;
	uint32_t u4ConfigMergedLen;
#endif

	wlanCfgInit(prAdapter, NULL, 0, 0);
	u4ConfigReadLen = 0;

	if (kalRequestFirmware("wifi_sigma.cfg", &pucConfigBuf,
		   &u4ConfigReadLen, TRUE,
		   prAdapter->prGlueInfo->prDev) == 0) {
		/* ToDo:: Nothing */
	} else if (kalRequestFirmware(WIFI_CFG_FN, &pucConfigBuf,
		   &u4ConfigReadLen, TRUE,
		   prAdapter->prGlueInfo->prDev) == 0) {
		/* ToDo:: Nothing */
	}

#if WLAN_INCLUDE_SYS
	iniFileErrorCheck(prAdapter, &pucIniBuf, &u4ConfigReadLen2);

	u4ConfigMergedLen = u4ConfigReadLen + u4ConfigReadLen2;
	if (u4ConfigMergedLen > 0) {
		pucMergedBuf = kalMemZAlloc(u4ConfigMergedLen, VIR_MEM_TYPE);
		if (pucMergedBuf) {
			if (pucConfigBuf) {
				pucConfigBuf[u4ConfigReadLen-1] = '\n';
				kalMemCopy(pucMergedBuf, pucConfigBuf,
					u4ConfigReadLen);
				kalMemFree(pucConfigBuf, VIR_MEM_TYPE,
					u4ConfigReadLen);
			}
			if (pucIniBuf) {
				kalMemCopy(pucMergedBuf + u4ConfigReadLen,
					pucIniBuf, u4ConfigReadLen2);
				kalMemFree(pucIniBuf, VIR_MEM_TYPE,
					u4ConfigReadLen2);
			}

			pucConfigBuf = pucMergedBuf;
			pucMergedBuf = NULL;
			u4ConfigReadLen = u4ConfigMergedLen;
		} else {
			if (pucIniBuf)
				kalMemFree(pucIniBuf, VIR_MEM_TYPE,
					u4ConfigReadLen2);
			DBGLOG(INIT, WARN, "pucMergedBuf allocate fail\n");
		}
	}
#endif

	if (pucConfigBuf) {
		wlanCfgInit(prAdapter, pucConfigBuf, u4ConfigReadLen,
			0);
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);
	}
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief this function send buffer bin EEPROB_MTxxxx.bin to FW.
 *
 * \param[in] prAdapter
 *
 * \retval WLAN_STATUS_SUCCESS Success
 * \retval WLAN_STATUS_FAILURE Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanDownloadBufferBin(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
#if (CFG_FW_Report_Efuse_Address)
	uint16_t u2InitAddr = prAdapter->u4EfuseStartAddress;
#else
	uint16_t u2InitAddr = EFUSE_CONTENT_BUFFER_START;
#endif
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_CUSTOM_EFUSE_BUFFER_MODE *prSetEfuseBufModeInfo
			= NULL;
	uint32_t u4ContentLen;
	uint32_t u4ReadLen;
	uint8_t *pucConfigBuf = NULL;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t chip_id;
	uint8_t aucEeprom[32];
	uint32_t retWlanStat = WLAN_STATUS_FAILURE;

#if CFG_EFUSE_AUTO_MODE_SUPPORT
	uint32_t u4Efuse_addr = 0;
	struct PARAM_CUSTOM_ACCESS_EFUSE *prAccessEfuseInfo
			= NULL;
#endif

	if (prAdapter->fgIsSupportPowerOnSendBufferModeCMD ==
	    TRUE) {
		DBGLOG(INIT, INFO, "Start Efuse Buffer Mode ..\n");
		DBGLOG(INIT, INFO, "ucEfuseBUfferModeCal is %x\n",
		       prAdapter->rWifiVar.ucEfuseBufferModeCal);

		prChipInfo = prAdapter->chip_info;
		chip_id = prChipInfo->chip_id;
		prGlueInfo = prAdapter->prGlueInfo;

		if (prGlueInfo == NULL || prGlueInfo->prDev == NULL)
			goto label_exit;

		/* allocate memory for buffer mode info */
		prSetEfuseBufModeInfo =
			(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE *)
			kalMemAlloc(sizeof(
				    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE),
				    VIR_MEM_TYPE);
		if (prSetEfuseBufModeInfo == NULL)
			goto label_exit;
		kalMemZero(prSetEfuseBufModeInfo,
			   sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE));

#if CFG_EFUSE_AUTO_MODE_SUPPORT
		/* allocate memory for Access Efuse Info */
		prAccessEfuseInfo =
			(struct PARAM_CUSTOM_ACCESS_EFUSE *)
			kalMemAlloc(sizeof(
				    struct PARAM_CUSTOM_ACCESS_EFUSE),
				    VIR_MEM_TYPE);
		if (prAccessEfuseInfo == NULL)
			goto label_exit;
		kalMemZero(prAccessEfuseInfo,
			   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));

		if (prAdapter->rWifiVar.ucEfuseBufferModeCal == LOAD_AUTO) {
			prAccessEfuseInfo->u4Address = (u4Efuse_addr /
				EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;
			rStatus = kalIoctl(prGlueInfo,
				wlanoidQueryProcessAccessEfuseRead,
				prAccessEfuseInfo,
				sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
				&u4BufLen);
			if (prGlueInfo->prAdapter->aucEepromVaule[1]
				== (chip_id>>8)) {
				prAdapter->rWifiVar.ucEfuseBufferModeCal
					= LOAD_EFUSE;
				DBGLOG(INIT, STATE,
					"[EFUSE AUTO] EFUSE Mode\n");
			} else {
				prAdapter->rWifiVar.ucEfuseBufferModeCal
					= LOAD_EEPROM_BIN;
				DBGLOG(INIT, STATE,
					"[EFUSE AUTO] Buffer Mode\n");
			}
		}
#endif
		if (prAdapter->rWifiVar.ucEfuseBufferModeCal
			== LOAD_EEPROM_BIN) {
			int ret = 0;

			/* Buffer mode */
			/* Only in buffer mode need to access bin file */
			/* 1 <1> Load bin file*/
			/* 1 <2> Construct EEPROM binary name */
			kalMemZero(aucEeprom, sizeof(aucEeprom));

			ret = kalSnprintf(aucEeprom, 32, "%s%x.bin",
				 apucEepromName[0], chip_id);
			if (ret < 0 || ret >= 32) {
				DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
						__LINE__, ret);
				goto label_exit;
			}

			/* 1 <3> Request buffer bin */
			if (kalRequestFirmware(aucEeprom, &pucConfigBuf,
			    &u4ReadLen, FALSE,
			    prGlueInfo->prDev) == 0) {
				DBGLOG(INIT, INFO, "request file done\n");
			} else {
				DBGLOG(INIT, INFO, "can't find file\n");
				goto label_exit;
			}

			/* 1 <4> Send CMD with bin file content */
			if (u4ReadLen > MAX_EEPROM_BUFFER_SIZE)
				goto label_exit;

			/* Update contents in local table */
			kalMemZero(uacEEPROMImage, MAX_EEPROM_BUFFER_SIZE);
			kalMemCopy(uacEEPROMImage, pucConfigBuf,
				   u4ReadLen);

			/* copy to the command buffer */
#if (CFG_FW_Report_Efuse_Address)
			u4ContentLen = (prAdapter->u4EfuseEndAddress) -
				       (prAdapter->u4EfuseStartAddress) + 1;
#else
			u4ContentLen = EFUSE_CONTENT_BUFFER_SIZE;
#endif
			if (u4ContentLen > MAX_EEPROM_BUFFER_SIZE)
				goto label_exit;
			kalMemCopy(prSetEfuseBufModeInfo->aBinContent,
				   &pucConfigBuf[u2InitAddr], u4ContentLen);

			prSetEfuseBufModeInfo->ucSourceMode = 1;
		} else {
			/* eFuse mode */
			/* Only need to tell FW the content from, contents are
			 * directly from efuse
			 */
			prSetEfuseBufModeInfo->ucSourceMode = 0;
		}
		prSetEfuseBufModeInfo->ucCmdType = 0x1 |
				   (prAdapter->rWifiVar.ucCalTimingCtrl << 4);
		prSetEfuseBufModeInfo->ucCount   =
			0xFF; /* ucCmdType 1 don't care the ucCount */

		rStatus = kalIoctl(prGlueInfo, wlanoidSetEfusBufferMode,
				(void *)prSetEfuseBufModeInfo,
				sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE),
				&u4BufLen);
	}

	retWlanStat = WLAN_STATUS_SUCCESS;

label_exit:

	/* free memory */
	if (prSetEfuseBufModeInfo != NULL)
		kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
			   sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE));

	if (pucConfigBuf != NULL)
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ReadLen);

#if CFG_EFUSE_AUTO_MODE_SUPPORT
	if (prAccessEfuseInfo != NULL)
		kalMemFree(prAccessEfuseInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
#endif

	return retWlanStat;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function send buffer bin EEPROB_MTxxxx.bin to FW.
 *
 * \param[in] prAdapter
 *
 * \retval WLAN_STATUS_SUCCESS Success
 * \retval WLAN_STATUS_FAILURE Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanConnacDownloadBufferBin(struct ADAPTER
				     *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T
		*prSetEfuseBufModeInfo = NULL;
	uint32_t u4ReadLen;
	uint8_t *pucConfigBuf = NULL;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t chip_id;
	uint8_t aucEeprom[32];
	uint32_t retWlanStat = WLAN_STATUS_FAILURE;

#if CFG_EFUSE_AUTO_MODE_SUPPORT
	uint32_t u4Efuse_addr = 0;
	struct PARAM_CUSTOM_ACCESS_EFUSE *prAccessEfuseInfo
			= NULL;
#endif

	if (prAdapter->fgIsSupportPowerOnSendBufferModeCMD == FALSE)
		return WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "Start Efuse Buffer Mode ..\n");
	DBGLOG(INIT, INFO, "ucEfuseBUfferModeCal is %x\n",
	       prAdapter->rWifiVar.ucEfuseBufferModeCal);

	prChipInfo = prAdapter->chip_info;
	chip_id = prChipInfo->chip_id;
	prGlueInfo = prAdapter->prGlueInfo;

	if (prGlueInfo == NULL || prGlueInfo->prDev == NULL)
		goto label_exit;

	/* allocate memory for buffer mode info */
	prSetEfuseBufModeInfo =
		(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T *)
		kalMemAlloc(sizeof(
			    struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T),
			    VIR_MEM_TYPE);
	if (prSetEfuseBufModeInfo == NULL)
		goto label_exit;
	kalMemZero(prSetEfuseBufModeInfo,
		   sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));

#if CFG_EFUSE_AUTO_MODE_SUPPORT
	/* allocate memory for Access Efuse Info */
	prAccessEfuseInfo =
		(struct PARAM_CUSTOM_ACCESS_EFUSE *)
			kalMemAlloc(sizeof(
				struct PARAM_CUSTOM_ACCESS_EFUSE),
					VIR_MEM_TYPE);
	if (prAccessEfuseInfo == NULL)
		goto label_exit;
	kalMemZero(prAccessEfuseInfo,
		sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));

	if (prAdapter->rWifiVar.ucEfuseBufferModeCal == LOAD_AUTO) {
		prAccessEfuseInfo->u4Address = (u4Efuse_addr /
			EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;
		rStatus = kalIoctl(prGlueInfo,
			wlanoidQueryProcessAccessEfuseRead, prAccessEfuseInfo,
			sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE), &u4BufLen);
		if (prGlueInfo->prAdapter->aucEepromVaule[1]
			== (chip_id>>8)) {
			prAdapter->rWifiVar.ucEfuseBufferModeCal
				= LOAD_EFUSE;
			DBGLOG(INIT, STATE,
				"[EFUSE AUTO] EFUSE Mode\n");
		} else {
			prAdapter->rWifiVar.ucEfuseBufferModeCal
				= LOAD_EEPROM_BIN;
			DBGLOG(INIT, STATE,
				"[EFUSE AUTO] Buffer Mode\n");
		}
	}
#endif

	if (prAdapter->rWifiVar.ucEfuseBufferModeCal
		== LOAD_EEPROM_BIN) {
		int ret = 0;

		/* Buffer mode */
		/* Only in buffer mode need to access bin file */
		/* 1 <1> Load bin file*/
		/* 1 <2> Construct EEPROM binary name */
		kalMemZero(aucEeprom, sizeof(aucEeprom));

		ret = kalSnprintf(aucEeprom, 32, "%s%x.bin",
			 apucEepromName[0], chip_id);
		if (ret < 0 || ret >= 32) {
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);
			goto label_exit;
		}

		/* 1 <3> Request buffer bin */
		if (kalRequestFirmware(aucEeprom, &pucConfigBuf,
		    &u4ReadLen, FALSE, prGlueInfo->prDev)
		    == 0) {
			DBGLOG(INIT, INFO, "request file done\n");
		} else {
			DBGLOG(INIT, INFO, "can't find file\n");
			goto label_exit;
		}
		DBGLOG(INIT, INFO,
			"u4ReadLen = %d,MAX_EEPROM_BUFFER_SIZE = %d\n",
			u4ReadLen, MAX_EEPROM_BUFFER_SIZE);

		/* 1 <4> Send CMD with bin file content */
		prGlueInfo = prAdapter->prGlueInfo;

		if (u4ReadLen > BUFFER_BIN_PAGE_SIZE)
			goto label_exit;

		/* Update contents in local table */
		kalMemZero(uacEEPROMImage, MAX_EEPROM_BUFFER_SIZE);
		kalMemCopy(uacEEPROMImage, pucConfigBuf, u4ReadLen);

		kalMemCopy(prSetEfuseBufModeInfo->aBinContent, pucConfigBuf,
			   u4ReadLen);

		prSetEfuseBufModeInfo->ucSourceMode = 1;
	} else {
		/* eFuse mode */
		/* Only need to tell FW the content from, contents are directly
		 * from efuse
		 */
		prSetEfuseBufModeInfo->ucSourceMode = 0;
		u4ReadLen = 0;
	}
	prSetEfuseBufModeInfo->ucContentFormat = 0x1 |
			(prAdapter->rWifiVar.ucCalTimingCtrl << 4);
	prSetEfuseBufModeInfo->u2Count = u4ReadLen;

	rStatus = kalIoctl(prGlueInfo, wlanoidConnacSetEfusBufferMode,
		(void *)prSetEfuseBufModeInfo,
		OFFSET_OF(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T,
		aBinContent) + u4ReadLen, &u4BufLen);

	retWlanStat = WLAN_STATUS_SUCCESS;

label_exit:

	/* free memory */
	if (prSetEfuseBufModeInfo != NULL)
		kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));

	if (pucConfigBuf != NULL)
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE,
			   u4ReadLen);

#if CFG_EFUSE_AUTO_MODE_SUPPORT
	if (prAccessEfuseInfo != NULL)
		kalMemFree(prAccessEfuseInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
#endif

	return retWlanStat;
}

uint32_t wlanConnac2XDownloadBufferBin(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t chip_id = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T
		*prSetEfuseBufModeInfo = NULL;
	uint8_t *pucConfigBuf = NULL;
	uint8_t aucEeprom[32];
	uint32_t u4ContentLen = 0;
	uint32_t u4ReadLen;
	uint8_t uTotalPage = 0;
	uint8_t uPageIdx = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint32_t retWlanStat = WLAN_STATUS_FAILURE;

#if CFG_EFUSE_AUTO_MODE_SUPPORT
	uint32_t u4Efuse_addr = 0;
	struct PARAM_CUSTOM_ACCESS_EFUSE *prAccessEfuseInfo
			= NULL;
#endif

	if (prAdapter->fgIsSupportPowerOnSendBufferModeCMD == FALSE
#if defined(UEFI_WORKAROUND)
		|| 1
#endif /* UEFI_WORKAROUND */
		)
		return WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "Start Efuse Buffer Mode ..\n");
	DBGLOG(INIT, INFO, "ucEfuseBUfferModeCal is %x\n",
		prAdapter->rWifiVar.ucEfuseBufferModeCal);

	prChipInfo = prAdapter->chip_info;
	chip_id = prChipInfo->chip_id;
	prGlueInfo = prAdapter->prGlueInfo;
	if (prGlueInfo == NULL || prGlueInfo->prDev == NULL)
		goto label_exit;

	/* allocate memory for buffer mode info */
	prSetEfuseBufModeInfo =
		(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T *)
		kalMemAlloc(sizeof(
			struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T),
			VIR_MEM_TYPE);
	if (prSetEfuseBufModeInfo == NULL)
		goto label_exit;
	kalMemZero(prSetEfuseBufModeInfo,
		sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));

#if CFG_EFUSE_AUTO_MODE_SUPPORT
		/* allocate memory for Access Efuse Info */
		prAccessEfuseInfo =
			(struct PARAM_CUSTOM_ACCESS_EFUSE *)
			kalMemAlloc(sizeof(
				    struct PARAM_CUSTOM_ACCESS_EFUSE),
				    VIR_MEM_TYPE);
		if (prAccessEfuseInfo == NULL)
			goto label_exit;
		kalMemZero(prAccessEfuseInfo,
			   sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));

		if (prAdapter->rWifiVar.ucEfuseBufferModeCal == LOAD_AUTO) {
			prAccessEfuseInfo->u4Address = (u4Efuse_addr /
				EFUSE_BLOCK_SIZE) * EFUSE_BLOCK_SIZE;
			rStatus = kalIoctl(prGlueInfo,
				wlanoidQueryProcessAccessEfuseRead,
				prAccessEfuseInfo,
				sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE),
				&u4BufLen);
			if (prGlueInfo->prAdapter->aucEepromVaule[1]
				== (chip_id>>8)) {
				prAdapter->rWifiVar.ucEfuseBufferModeCal
					= LOAD_EFUSE;
				DBGLOG(INIT, STATE,
					"[EFUSE AUTO] EFUSE Mode\n");
			} else {
				prAdapter->rWifiVar.ucEfuseBufferModeCal
					= LOAD_EEPROM_BIN;
				DBGLOG(INIT, STATE,
					"[EFUSE AUTO] Buffer Mode\n");
			}
		}
#endif

	if (prAdapter->rWifiVar.ucEfuseBufferModeCal
		== LOAD_EEPROM_BIN) {
		/* Buffer mode */
		/* Only in buffer mode need to access bin file */
		/* 1 <1> Load bin file*/
		/* 1 <2> Construct EEPROM binary name */
		kalMemZero(aucEeprom, sizeof(aucEeprom));
		if (prChipInfo->constructBufferBinFileName == NULL) {
			if (snprintf(aucEeprom, 32, "%s%x.bin",
				 apucEepromName[0], chip_id) < 0) {
				DBGLOG(INIT, ERROR, "gen BIN file name fail\n");
				goto label_exit;
			}
		} else {
			if (prChipInfo->constructBufferBinFileName(
			    prAdapter, aucEeprom) != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR, "gen BIN file name fail\n");
				goto label_exit;
			}
		}

		/* 1 <3> Request buffer bin */
		if (kalRequestFirmware(aucEeprom, &pucConfigBuf,
				&u4ReadLen, FALSE,
				prGlueInfo->prDev) == 0) {
			DBGLOG(INIT, INFO, "request file done\n");
		} else {
			DBGLOG(INIT, INFO, "can't find file\n");
			goto label_exit;
		}

		DBGLOG(INIT, INFO,
			"u4ReadLen = %d,MAX_EEPROM_BUFFER_SIZE = %d\n",
			u4ReadLen, MAX_EEPROM_BUFFER_SIZE);

		/* 1 <4> Send CMD with bin file content */
		if (u4ReadLen == 0 || u4ReadLen > MAX_EEPROM_BUFFER_SIZE)
			goto label_exit;

		/* Update contents in local table */
		kalMemZero(uacEEPROMImage, MAX_EEPROM_BUFFER_SIZE);
		kalMemCopy(uacEEPROMImage, pucConfigBuf, u4ReadLen);

		uTotalPage = u4ReadLen / BUFFER_BIN_PAGE_SIZE;
		if ((u4ReadLen % BUFFER_BIN_PAGE_SIZE) == 0)
			uTotalPage--;

		prSetEfuseBufModeInfo->ucSourceMode = 1;
	} else {
		/* eFuse mode */
		/* Only need to tell FW the content from, contents are directly
		 * from efuse
		 */
		prSetEfuseBufModeInfo->ucSourceMode = 0;
		u4ReadLen = 0;
		uTotalPage = 0;
	}

	u4ContentLen = u4ReadLen;
	for (uPageIdx = 0; uPageIdx <= uTotalPage; uPageIdx++) {
		/* set format */
		prSetEfuseBufModeInfo->ucContentFormat = (
			CONTENT_FORMAT_WHOLE_CONTENT |
			((uTotalPage << BUFFER_BIN_TOTAL_PAGE_SHIFT)
				& BUFFER_BIN_TOTAL_PAGE_MASK) |
			((uPageIdx << BUFFER_BIN_PAGE_INDEX_SHIFT)
				& BUFFER_BIN_PAGE_INDEX_MASK)
		);
		/* set buffer size */
		prSetEfuseBufModeInfo->u2Count =
			(u4ContentLen < BUFFER_BIN_PAGE_SIZE ?
				u4ContentLen : BUFFER_BIN_PAGE_SIZE);
		/* set buffer */
		kalMemZero(prSetEfuseBufModeInfo->aBinContent,
			BUFFER_BIN_PAGE_SIZE);
		if (prSetEfuseBufModeInfo->u2Count != 0)
			kalMemCopy(prSetEfuseBufModeInfo->aBinContent,
				pucConfigBuf + uPageIdx * BUFFER_BIN_PAGE_SIZE,
				prSetEfuseBufModeInfo->u2Count);
		/* send buffer */
		DBGLOG(INIT, INFO, "[%d/%d] load buffer size: 0x%x\n",
			uPageIdx, uTotalPage, prSetEfuseBufModeInfo->u2Count);
		rStatus = kalIoctl(prGlueInfo, wlanoidConnacSetEfusBufferMode,
			(void *) prSetEfuseBufModeInfo, OFFSET_OF(
				struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T,
				aBinContent) + prSetEfuseBufModeInfo->u2Count,
				&u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
			       "kalIoctl EXT_CMD_ID_EFUSE_BUFFER_MODE 0x%X\n",
			       rStatus);
			goto label_exit;
		}
		/* update remain size */
		u4ContentLen -= prSetEfuseBufModeInfo->u2Count;
	}
	retWlanStat = WLAN_STATUS_SUCCESS;

label_exit:
	/* free memory */
	if (prSetEfuseBufModeInfo != NULL)
		kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));
	if (pucConfigBuf != NULL)
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ReadLen);

#if CFG_EFUSE_AUTO_MODE_SUPPORT
	if (prAccessEfuseInfo != NULL)
		kalMemFree(prAccessEfuseInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE));
#endif

	return retWlanStat;
}

#if (CFG_SUPPORT_CONNAC3X == 1)
uint32_t wlanConnac3XDownloadBufferBin(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t chip_id = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T
		*prSetEfuseBufModeInfo = NULL;
	uint8_t *pucConfigBuf = NULL;
	uint8_t aucEeprom[32];
	uint32_t u4ContentLen = 0;
	uint32_t u4ReadLen;
	uint8_t uTotalPage = 0;
	uint8_t uPageIdx = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint32_t retWlanStat = WLAN_STATUS_FAILURE;

	if (prAdapter->fgIsSupportPowerOnSendBufferModeCMD == FALSE
#if defined(UEFI_WORKAROUND)
		|| 1
#endif /* UEFI_WORKAROUND */
		)
		return WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "Start Efuse Buffer Mode ..\n");
	DBGLOG(INIT, INFO, "ucEfuseBUfferModeCal is %x\n",
		prAdapter->rWifiVar.ucEfuseBufferModeCal);

	prChipInfo = prAdapter->chip_info;
	chip_id = prChipInfo->chip_id;

	prGlueInfo = prAdapter->prGlueInfo;
	if (prGlueInfo == NULL || prGlueInfo->prDev == NULL)
		goto label_exit;

	/* allocate memory for buffer mode info */
	prSetEfuseBufModeInfo =
		(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T *)
		kalMemAlloc(sizeof(
			struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T),
			VIR_MEM_TYPE);

	if (prSetEfuseBufModeInfo == NULL)
		goto label_exit;
	kalMemZero(prSetEfuseBufModeInfo,
		sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));

	if (prAdapter->rWifiVar.ucEfuseBufferModeCal
		== LOAD_EEPROM_BIN) {
		/* Buffer mode */
		/* Only in buffer mode need to access bin file */
		/* 1 <1> Load bin file*/
		/* 1 <2> Construct EEPROM binary name */
		kalMemZero(aucEeprom, sizeof(aucEeprom));

		if (prChipInfo->constructBufferBinFileName == NULL) {
			if (snprintf(aucEeprom, 32, "%s%x.bin",
				 apucEepromName[0], chip_id) < 0) {  //EEPROM_MT7903.bin
				DBGLOG(INIT, ERROR, "gen BIN file name '%s%x.bin' fail\n"
					, apucEepromName[0], chip_id);
				goto label_exit;
			}
			if(strcmp(aucEeprom, (uint8_t*) "EEPROM_MT7903.bin") == 0)
				strncpy(aucEeprom, (uint8_t*) "BELLWETHER_EEPROM.bin", 32);

			DBGLOG(INIT, INFO, "gen BIN file name '%s' success\n", aucEeprom);
		} else if (prChipInfo->constructBufferBinFileName(
		    prAdapter, aucEeprom) != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "gen BIN file name fail\n");
			goto label_exit;
		}

		/* 1 <3> Request buffer bin */
		if (kalRequestFirmware(aucEeprom, &pucConfigBuf,
				&u4ReadLen, FALSE,
				prGlueInfo->prDev) == 0) {
			DBGLOG(INIT, INFO, "request file done\n");
		} else {
			DBGLOG(INIT, INFO, "can't find file\n");
			goto label_exit;
		}

		DBGLOG(INIT, INFO,
			"u4ReadLen = %d,MAX_EEPROM_BUFFER_SIZE = %d\n",
			u4ReadLen, MAX_EEPROM_BUFFER_SIZE);

		/* 1 <4> Send CMD with bin file content */
		if (u4ReadLen == 0 || u4ReadLen > MAX_EEPROM_BUFFER_SIZE)
			goto label_exit;

		/* Update contents in local table */
		kalMemZero(uacEEPROMImage, MAX_EEPROM_BUFFER_SIZE);
		kalMemCopy(uacEEPROMImage, pucConfigBuf, u4ReadLen);

		uTotalPage = u4ReadLen / BUFFER_BIN_PAGE_SIZE;
		if ((u4ReadLen % BUFFER_BIN_PAGE_SIZE) == 0)
			uTotalPage--;

		prSetEfuseBufModeInfo->ucSourceMode = 1;
	} else {
		/* eFuse mode */
		/* Only need to tell FW the content from, contents are directly
		 * from efuse
		 */
		prSetEfuseBufModeInfo->ucSourceMode = 0;
		u4ReadLen = 0;
		uTotalPage = 0;
	}

	u4ContentLen = u4ReadLen;
	for (uPageIdx = 0; uPageIdx <= uTotalPage; uPageIdx++) {
		/* set format */
		prSetEfuseBufModeInfo->ucContentFormat = (
			CONTENT_FORMAT_WHOLE_CONTENT |
			((uTotalPage << BUFFER_BIN_TOTAL_PAGE_SHIFT)
				& BUFFER_BIN_TOTAL_PAGE_MASK) |
			((uPageIdx << BUFFER_BIN_PAGE_INDEX_SHIFT)
				& BUFFER_BIN_PAGE_INDEX_MASK)
		);
		/* set buffer size */
		prSetEfuseBufModeInfo->u2Count =
			(u4ContentLen < BUFFER_BIN_PAGE_SIZE ?
				u4ContentLen : BUFFER_BIN_PAGE_SIZE);
		/* set buffer */
		kalMemZero(prSetEfuseBufModeInfo->aBinContent,
			BUFFER_BIN_PAGE_SIZE);
		if (prSetEfuseBufModeInfo->u2Count != 0)
			kalMemCopy(prSetEfuseBufModeInfo->aBinContent,
				pucConfigBuf + uPageIdx * BUFFER_BIN_PAGE_SIZE,
				prSetEfuseBufModeInfo->u2Count);

		DBGLOG(INIT, INFO, "%s[%d] ucSourceMode = %d, ucContentFormat = 0x%x, u2Count = %d\n"
		, __func__, __LINE__,
		prSetEfuseBufModeInfo->ucSourceMode,
		prSetEfuseBufModeInfo->ucContentFormat,
		prSetEfuseBufModeInfo->u2Count);

		/* send buffer */
		DBGLOG(INIT, INFO, "[%d/%d] load buffer size: 0x%x\n",
			uPageIdx, uTotalPage, prSetEfuseBufModeInfo->u2Count);
		rStatus = kalIoctl(prGlueInfo, wlanoidConnacSetEfusBufferMode,
			(void *) prSetEfuseBufModeInfo, OFFSET_OF(
				struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T,
				aBinContent) + prSetEfuseBufModeInfo->u2Count,
				&u4BufLen);

		/* update remain size */
		u4ContentLen -= prSetEfuseBufModeInfo->u2Count;
	}
	retWlanStat = WLAN_STATUS_SUCCESS;

label_exit:
	/* free memory */
	if (prSetEfuseBufModeInfo != NULL)
		kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T));
	if (pucConfigBuf != NULL)
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ReadLen);
	return retWlanStat;
}
#endif

#if (CONFIG_WLAN_SERVICE == 1)
static uint32_t wlanServiceAllocInfo(struct GLUE_INFO *prGlueInfo)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!gprServiceTest) {
		gprServiceTest =
			kalMemAlloc(sizeof(struct service_test),
				VIR_MEM_TYPE);
		if (gprServiceTest == NULL) {
			DBGLOG(INIT, INFO, "gprServiceTest malloc fail\n");
			return WLAN_STATUS_FAILURE;
		}

		gprServiceTest->test_winfo
			= kalMemAlloc(sizeof(struct test_wlan_info),
				VIR_MEM_TYPE);
		if (gprServiceTest->test_winfo == NULL) {
			DBGLOG(INIT, INFO,
				"gprServiceTest->test_winfo malloc fail\n");
			goto label_exit;
		}

		gprServiceTest->test_op
			= kalMemAlloc(sizeof(struct test_operation),
				VIR_MEM_TYPE);
		if (gprServiceTest->test_op == NULL) {
			DBGLOG(INIT, INFO,
				"gprServiceTest->test_op malloc fail\n");
			goto label_exit;
		}
	} else {
		DBGLOG(INIT, INFO, "gprServiceTest has been malloc\n");
	}

	prGlueInfo->rService.serv_handle = gprServiceTest;
	return rStatus;

label_exit:
	/* free memory */
	if (gprServiceTest != NULL) {

		if (gprServiceTest->test_winfo != NULL)
			kalMemFree(gprServiceTest->test_winfo, VIR_MEM_TYPE,
				   sizeof(struct test_wlan_info));

		if (gprServiceTest->test_op != NULL)
			kalMemFree(gprServiceTest->test_op, VIR_MEM_TYPE,
				   sizeof(struct test_operation));

		kalMemFree(gprServiceTest, VIR_MEM_TYPE,
			sizeof(struct service_test));
	}

	return WLAN_STATUS_FAILURE;
}

static uint32_t wlanServiceFreeInfo(struct GLUE_INFO *prGlueInfo)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!gprServiceTest)
		return rStatus;

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	if (get_wifi_in_switch_mode() == true)
		return rStatus;
#endif

	KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_HQA_TEST);
	if (gprServiceTest->test_winfo)
		kalMemFree(gprServiceTest->test_winfo, VIR_MEM_TYPE,
			sizeof(struct test_wlan_info));

	if (gprServiceTest->test_op)
		kalMemFree(gprServiceTest->test_op, VIR_MEM_TYPE,
			sizeof(struct test_operation));

	kalMemFree(gprServiceTest, VIR_MEM_TYPE,
		sizeof(struct service_test));

	gprServiceTest = NULL;
	KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_HQA_TEST);

	return rStatus;
}

uint32_t wlanServiceInit(struct GLUE_INFO *prGlueInfo)
{

	struct service_test *prServiceTest;
	struct test_wlan_info *winfos;
	struct mt66xx_chip_info *prChipInfo;
#if CFG_SUPPORT_QA_TOOL
	struct ATE_OPS_T *prAteOps = NULL;
#endif
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, TRACE, "%s enter!\n", __func__);

	if (prGlueInfo == NULL)
		return WLAN_STATUS_FAILURE;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prGlueInfo->rService.serv_id = SERV_HANDLE_TEST;

	rStatus = wlanServiceAllocInfo(prGlueInfo);
	if (rStatus == WLAN_STATUS_FAILURE)
		return rStatus;

	prServiceTest = (struct service_test *)prGlueInfo->rService.serv_handle;

	winfos = prServiceTest->test_winfo;

	prServiceTest->test_winfo->net_dev = gPrDev;

	if (prChipInfo->asicGetChipID)
		prServiceTest->test_winfo->chip_id =
			prChipInfo->asicGetChipID(prGlueInfo->prAdapter);
	else
		prServiceTest->test_winfo->chip_id = prChipInfo->chip_id;

	DBGLOG(INIT, WARN,
			"%s chip_id = 0x%x\n", __func__,
			prServiceTest->test_winfo->chip_id);

	prServiceTest->test_winfo->chip_cap.support_6g = FALSE;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prGlueInfo->prAdapter->fgIsHwSupport6G)
		prServiceTest->test_winfo->chip_cap.support_6g = TRUE;
#endif

#if (CFG_MTK_ANDROID_EMI == 1)
	prServiceTest->test_winfo->emi_phy_base =
		emi_mem_get_phy_base(prGlueInfo->prAdapter->chip_info);
	prServiceTest->test_winfo->emi_phy_size =
		emi_mem_get_size(prGlueInfo->prAdapter->chip_info);
#else
	DBGLOG(RFTEST, WARN, "Platform doesn't support EMI address\n");
#endif

#if CFG_SUPPORT_QA_TOOL
	/* icap setting */
	prAteOps = prChipInfo->prAteOps;
	if (prAteOps != NULL) {
		prServiceTest->test_winfo->icap_arch
			= prAteOps->u4Architech;
		prServiceTest->test_winfo->icap_bitwidth
			= prAteOps->u4EnBitWidth;
		prServiceTest->test_winfo->icap_phy_idx
			= prAteOps->u4PhyIdx;
#if (CFG_MTK_ANDROID_EMI == 1)
		prServiceTest->test_winfo->icap_emi_start_addr
			= prAteOps->u4EmiStartAddress;
		prServiceTest->test_winfo->icap_emi_end_addr
			= prAteOps->u4EmiEndAddress;
		prServiceTest->test_winfo->icap_emi_msb_addr
			= prAteOps->u4EmiMsbAddress;
#else
		prServiceTest->test_winfo->icap_emi_start_addr = 0;
		prServiceTest->test_winfo->icap_emi_end_addr = 0;
		prServiceTest->test_winfo->icap_emi_msb_addr = 0;
#endif /* (CFG_MTK_ANDROID_EMI == 1) */
	} else {
		DBGLOG(INIT, WARN, "prAteOps is null!\n");
	}
#endif
	prServiceTest->engine_offload = true;
	winfos->oid_funcptr = (wlan_oid_handler_t) ServiceWlanOid;

	rStatus = mt_agent_init_service(&prGlueInfo->rService);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, WARN, "%s init fail err:%d\n", __func__, rStatus);

	return rStatus;
}

uint32_t wlanServiceExit(struct GLUE_INFO *prGlueInfo)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, TRACE, "%s enter\n", __func__);

	if (prGlueInfo == NULL)
		return WLAN_STATUS_FAILURE;

	rStatus = mt_agent_exit_service(&prGlueInfo->rService);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, WARN, "wlanServiceExit fail err:%d\n", rStatus);

	wlanServiceFreeInfo(prGlueInfo);

	prGlueInfo->rService.serv_id = 0;
	return rStatus;
}
#endif

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
static uint32_t u4LogOnOffCache;
static uint32_t u4LogLevelCache = -1;

uint32_t getFWLogOnOff(void)
{
	return u4LogOnOffCache;
}

uint32_t getFWLogLevel(void)
{
	return u4LogLevelCache;
}

uint32_t
connsysFwLogControl(struct ADAPTER *prAdapter, void *pvSetBuffer,
	uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct CMD_CONNSYS_FW_LOG *prCmd;
	struct CMD_HEADER rCmdV1Header;
	struct CMD_FORMAT_V1 rCmd_v1;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if ((prAdapter == NULL) || (pvSetBuffer == NULL)
		|| (pu4SetInfoLen == NULL))
		return WLAN_STATUS_FAILURE;

	/* init */
	*pu4SetInfoLen = sizeof(struct CMD_CONNSYS_FW_LOG);
	prCmd = (struct CMD_CONNSYS_FW_LOG *) pvSetBuffer;

	if (prCmd->fgCmd == FW_LOG_CMD_ON_OFF) {

		/*EvtDrvnLogEn 0/1*/
		uint8_t onoff[1] = {'0'};

		DBGLOG(INIT, TRACE, "FW_LOG_CMD_ON_OFF\n");

		rCmdV1Header.cmdType = CMD_TYPE_SET;
		rCmdV1Header.cmdVersion = CMD_VER_1;
		rCmdV1Header.cmdBufferLen = 0;
		rCmdV1Header.itemNum = 0;

		kalMemSet(rCmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);
		kalMemSet(&rCmd_v1, 0, sizeof(struct CMD_FORMAT_V1));

		rCmd_v1.itemType = ITEM_TYPE_STR;

		/*send string format to firmware */
		rCmd_v1.itemStringLength = kalStrLen("EnableDbgLog");
		kalMemZero(rCmd_v1.itemString, MAX_CMD_NAME_MAX_LENGTH);
		kalMemCopy(rCmd_v1.itemString, "EnableDbgLog",
			rCmd_v1.itemStringLength);

		if (prCmd->fgValue == 1) /* other cases, send 'OFF=0' */
			onoff[0] = '1';
		rCmd_v1.itemValueLength = 1;
		kalMemZero(rCmd_v1.itemValue, MAX_CMD_VALUE_MAX_LENGTH);
		kalMemCopy(rCmd_v1.itemValue, &onoff, 1);

		DBGLOG(INIT, INFO, "Send key word (%s) WITH (%s) to firmware\n",
				rCmd_v1.itemString, rCmd_v1.itemValue);

		kalMemCopy(((struct CMD_FORMAT_V1 *)rCmdV1Header.buffer),
				&rCmd_v1,  sizeof(struct CMD_FORMAT_V1));

		rCmdV1Header.cmdBufferLen += sizeof(struct CMD_FORMAT_V1);
		rCmdV1Header.itemNum = 1;
		prAdapter->fgSetLogOnOff = false;

		if (prCmd->fgEarlySet) {
			rStatus = wlanSendFwLogControlCmd(prAdapter,
					CMD_ID_GET_SET_CUSTOMER_CFG,
					NULL,
					NULL,
					sizeof(struct CMD_HEADER),
					(uint8_t *)&rCmdV1Header);
		} else {
			rStatus = wlanSendSetQueryCmd(
				prAdapter, /* prAdapter */
				CMD_ID_GET_SET_CUSTOMER_CFG, /* 0x70 */
				TRUE,  /* fgSetQuery */
				FALSE, /* fgNeedResp */
				FALSE, /* fgIsOid */
				NULL,  /* pfCmdDoneHandler*/
				NULL,  /* pfCmdTimeoutHandler */
				sizeof(struct CMD_HEADER),
				(uint8_t *)&rCmdV1Header, /* pucInfoBuffer */
				NULL,  /* pvSetQueryBuffer */
				0      /* u4SetQueryBufferLen */
			);
		}

		/* keep in cache */
		u4LogOnOffCache = prCmd->fgValue;

		if (rStatus != WLAN_STATUS_FAILURE)
			prAdapter->fgSetLogOnOff = true;
		else
			DBGLOG(INIT, INFO, "Log On/Off setting fail!\n");
	} else if (prCmd->fgCmd == FW_LOG_CMD_SET_LEVEL) {
		/*ENG_LOAD_OFFSET 1*/
		/*USERDEBUG_LOAD_OFFSET 2 */
		/*USER_LOAD_OFFSET 3 */
		int32_t u4LogLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;

		DBGLOG(INIT, INFO, "FW_LOG_CMD_SET_LEVEL %d\n", prCmd->fgValue);
		switch (prCmd->fgValue) {
		case 0:
			u4LogLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;
			break;
		case 1:
			u4LogLevel = ENUM_WIFI_LOG_LEVEL_MORE;
			break;
		case 2:
			u4LogLevel = ENUM_WIFI_LOG_LEVEL_EXTREME;
			break;
		default:
			u4LogLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;
			break;
		}

		if (prCmd->fgEarlySet) {
			wlanDbgSetLogLevel(prAdapter,
					   ENUM_WIFI_LOG_LEVEL_VERSION_V1,
					   ENUM_WIFI_LOG_MODULE_FW,
					   u4LogLevel, TRUE);
		} else {
			wlanDbgSetLogLevelImpl(prAdapter,
					   ENUM_WIFI_LOG_LEVEL_VERSION_V1,
					   ENUM_WIFI_LOG_MODULE_FW,
					   u4LogLevel);
		}

		/* keep in cache */
		u4LogLevelCache = prCmd->fgValue;
	} else {
		DBGLOG(INIT, INFO, "command can not parse\n");
	}
	return WLAN_STATUS_SUCCESS;
}

static void consys_log_event_notification(int cmd, int value)
{
	struct CMD_CONNSYS_FW_LOG rFwLogCmd;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4BufLen;
	u_int8_t fgRetrieveLog = FALSE;

	DBGLOG(INIT, INFO, "cmd=%d, value=%d\n", cmd, value);

	switch (cmd) {
	case FW_LOG_CMD_ON_OFF:
		if (value < 0 || value > 1)
			return;
		u4LogOnOffCache = value;
		if (u4LogOnOffCache == 0)
			fgRetrieveLog = TRUE;
		break;
	case FW_LOG_CMD_SET_LEVEL:
		if (value < 0 || value > 2)
			return;
		u4LogLevelCache = value;
		break;
	default:
		break;
	}

	if (kalIsHalted()) { /* power-off */
		DBGLOG(INIT, INFO,
			"Power off return, u4LogOnOffCache=%d\n",
				u4LogOnOffCache);
		return;
	}

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, INFO,
			"prGlueInfo == NULL return, u4LogOnOffCache=%d\n",
				u4LogOnOffCache);
		return;
	}
	prAdapter = prGlueInfo->prAdapter;
	DBGLOG(INIT, TRACE, "prAdapter=%p\n", prAdapter);
	if (!prAdapter) {
		DBGLOG(INIT, INFO,
			"prAdapter == NULL return, u4LogOnOffCache=%d\n",
				u4LogOnOffCache);
		return;
	}

	kalMemZero(&rFwLogCmd, sizeof(rFwLogCmd));
	rFwLogCmd.fgCmd = cmd;
	rFwLogCmd.fgValue = value;
	rFwLogCmd.fgEarlySet = FALSE;

	rStatus = kalIoctl(prGlueInfo, connsysFwLogControl, &rFwLogCmd,
			sizeof(struct CMD_CONNSYS_FW_LOG), &u4BufLen);

	if (cmd == FW_LOG_CMD_ON_OFF)
		fw_log_set_enabled(prAdapter,
				   value == 1 ? TRUE : FALSE);

	if (fgRetrieveLog)
		fw_log_handler();
}
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
int connsys_power_event_notification(
		enum conn_pwr_event_type type, void *data)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t *prLevel;
	struct conn_pwr_event_max_temp *prTempInfo;
	int ret = -1;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, INFO, "prGlueInfo is NULL return");
		return ret;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(INIT, INFO, "driver is not ready\n");
		return ret;
	}

	prAdapter = prGlueInfo->prAdapter;
	DBGLOG(INIT, TRACE, "prAdapter=%p\n", prAdapter);
	if (!prAdapter) {
		DBGLOG(INIT, INFO,
			"prAdapter == NULL return\n");
		return ret;
	}

	switch (type) {
	case CONN_PWR_EVENT_LEVEL:
		prLevel = (int *)data;
		prAdapter->u4PwrLevel = *prLevel;

		DBGLOG(INIT, INFO, "New power level: %d\n",
					prAdapter->u4PwrLevel);

		set_bit(GLUE_FLAG_CNS_PWR_LEVEL_BIT, &prGlueInfo->ulFlag);
		/* wake up main thread */
		wake_up_interruptible(&prGlueInfo->waitq);

		break;

	case CONN_PWR_EVENT_MAX_TEMP:
		prTempInfo = (struct conn_pwr_event_max_temp *)data;
		(prAdapter->rTempInfo).max_temp = prTempInfo->max_temp;
		(prAdapter->rTempInfo).recovery_temp =
						prTempInfo->recovery_temp;

		DBGLOG(INIT, INFO, "New max temp: %d/New recovery temp: %d",
					(prAdapter->rTempInfo).max_temp,
					(prAdapter->rTempInfo).recovery_temp);

		set_bit(GLUE_FLAG_CNS_PWR_TEMP_BIT, &prGlueInfo->ulFlag);
		/* wake up main thread */
		wake_up_interruptible(&prGlueInfo->waitq);

		break;

	default:
		DBGLOG(INIT, TRACE, "Unknown connsys power event type.\n");
		return ret;
	}

	return 0;
}

void power_throttling_start(void)
{
	conn_pwr_register_event_cb(CONN_PWR_DRV_WIFI,
		(CONN_PWR_EVENT_CB)connsys_power_event_notification);
}

void power_throttling_stop(void)
{
	conn_pwr_register_event_cb(CONN_PWR_DRV_WIFI, NULL);
}

void power_throttling_pre_start(void)
{
	struct mt66xx_hif_driver_data *prDriverData =
		get_platform_driver_data();

	/* Notify CONN_PWR wifi will be on, and it can calculate new level. */
	conn_pwr_drv_pre_on(CONN_PWR_DRV_WIFI, &prDriverData->u4PwrLevel);
	conn_pwr_send_msg(CONN_PWR_DRV_WIFI, CONN_PWR_MSG_GET_TEMP,
			&prDriverData->rTempInfo);
}

void power_throttling_post_stop(void)
{
	/* Notify CONN_PWR wifi is off, and it can calculate new level */
	conn_pwr_drv_post_off(CONN_PWR_DRV_WIFI);
}
#endif

static
void wlanOnPreAdapterStart(struct GLUE_INFO *prGlueInfo,
	struct ADAPTER *prAdapter,
	struct REG_INFO **pprRegInfo,
	struct mt66xx_chip_info *prChipInfo)
{
#if CFG_WMT_WIFI_PATH_SUPPORT
	int32_t i4RetVal = 0;
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	uint8_t ucBssIdx = 0;
#endif

	DBGLOG(INIT, TRACE, "start.\n");
	prGlueInfo->u4ReadyFlag = 0;
#if CFG_MTK_ANDROID_WMT
	update_driver_loaded_status(prGlueInfo->u4ReadyFlag);
#endif
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	prAdapter->fgIsSupportCsumOffload = FALSE;
	prAdapter->u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
#endif

#if CFG_SUPPORT_CFG_FILE
	wlanGetConfig(prAdapter);
#endif

	prAdapter->CurNoResSeqID = 0;

	/* Initialize Feature Options */
	wlanInitFeatureOption(prAdapter);

	/* Init Chip Capability */
	if (prChipInfo->asicCapInit)
		prChipInfo->asicCapInit(prAdapter);

	/* Default support 2.4/5G MIMO */
	prAdapter->rWifiFemCfg.u2WifiPath = (
			WLAN_FLAG_2G4_WF0 | WLAN_FLAG_5G_WF0 |
			WLAN_FLAG_2G4_WF1 | WLAN_FLAG_5G_WF1);

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* Default support 6G MIMO */
	prAdapter->rWifiFemCfg.u2WifiPath6G =
			(WLAN_FLAG_6G_WF0 | WLAN_FLAG_6G_WF1);
#endif

#if CFG_WMT_WIFI_PATH_SUPPORT
	i4RetVal = mtk_wcn_wmt_wifi_fem_cfg_report((
				void *)&prAdapter->rWifiFemCfg);
	if (i4RetVal)
		DBGLOG(INIT, ERROR, "Get WifiPath from WMT drv fail\n");
	else
		DBGLOG(INIT, INFO,
		       "Get WifiPath from WMT drv success, WifiPath=0x%x\n",
		       prAdapter->rWifiFemCfg.u2WifiPath);
#endif
	/* 4 <5> Start Device */
	*pprRegInfo = &prGlueInfo->rRegInfo;

	/* P_REG_INFO_T prRegInfo = (P_REG_INFO_T) kmalloc(
	 *				sizeof(REG_INFO_T), GFP_KERNEL);
	 */
	kalMemSet(*pprRegInfo, 0, sizeof(struct REG_INFO));

	/* Trigger the action of switching Pwr state to drv_own */
	prAdapter->fgIsFwOwn = TRUE;

	/* Load NVRAM content to REG_INFO_T */
	glLoadNvram(prGlueInfo, *pprRegInfo);

	/* kalMemCopy(&prGlueInfo->rRegInfo, prRegInfo,
	 *            sizeof(REG_INFO_T));
	 */

	(*pprRegInfo)->u4PowerMode = CFG_INIT_POWER_SAVE_PROF;
#if 0
		prRegInfo->fgEnArpFilter = TRUE;
#endif

#if CFG_SUPPORT_NAN
	prAdapter->fgIsNANfromHAL = TRUE;
	prAdapter->rPublishInfo.ucNanPubNum = 0;
	prAdapter->rSubscribeInfo.ucNanSubNum = 0;
	DBGLOG(INIT, WARN, "NAN fgIsNANfromHAL init %u\n",
	       prAdapter->fgIsNANfromHAL);
#endif

#if CFG_SUPPORT_TDLS
	prAdapter->u4TdlsLinkCount = 0;
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	for (ucBssIdx = 0; ucBssIdx < MAX_BSSID_NUM; ucBssIdx++)
		prAdapter->e6GPwrMode[ucBssIdx] = PWR_MODE_6G_LPI;
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE == 1 */

#if CFG_SUPPORT_WED_PROXY
	wedInitAdapterInfo(prAdapter);
#endif
}

static
void wlanOnPostAdapterStart(struct ADAPTER *prAdapter,
	struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(INIT, TRACE, "start.\n");
}

static int32_t wlanOnPreNetRegister(struct GLUE_INFO *prGlueInfo,
	struct ADAPTER *prAdapter,
	struct mt66xx_chip_info *prChipInfo,
	struct WIFI_VAR *prWifiVar,
	const u_int8_t bAtResetFlow)
{
	uint32_t i;

	DBGLOG(INIT, TRACE, "start.\n");

	if (!bAtResetFlow) {
		/* change net device mtu from feature option */
		if (prWifiVar->u4MTU > 0 && prWifiVar->u4MTU <= ETH_DATA_LEN) {
			for (i = 0; i < KAL_AIS_NUM; i++) {
				struct net_device *ndev =
					wlanGetAisNetDev(prGlueInfo, i);

				if (ndev)
					ndev->mtu = prWifiVar->u4MTU;
			}
		}
		INIT_DELAYED_WORK(&prGlueInfo->rRxPktDeAggWork,
				halDeAggRxPktWorker);
	}
	prGlueInfo->main_thread = kthread_run(main_thread,
		prGlueInfo->prDevHandler, "main_thread");
#if CFG_SUPPORT_MULTITHREAD
	INIT_WORK(&prGlueInfo->rTxMsduFreeWork, kalFreeTxMsduWorker);
	prGlueInfo->hif_thread = kthread_run(hif_thread,
			prGlueInfo->prDevHandler, "hif_thread");
	prGlueInfo->rx_thread = kthread_run(rx_thread,
			prGlueInfo->prDevHandler, "rx_thread");
#endif
	prGlueInfo->fgRxTaskReady = TRUE;

	/* do schedule for the first time */
	if (HAL_IS_RX_DIRECT(prGlueInfo->prAdapter))
		kalRxTaskSchedule(prGlueInfo);

	if (!bAtResetFlow)
		g_u4HaltFlag = 0;

#if CFG_SUPPORT_BUFFER_MODE && (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)

	if (prChipInfo->downloadBufferBin &&
	    (prChipInfo->downloadBufferBin(prAdapter) != WLAN_STATUS_SUCCESS))
		return -1;

#endif

#if CFG_SUPPORT_NAN
	if (!bAtResetFlow)
		kalCreateUserSock(prAdapter->prGlueInfo);
#endif

#if CFG_SUPPORT_DBDC
	/* Update DBDC default setting */
	cnmInitDbdcSetting(prAdapter);
#endif /*CFG_SUPPORT_DBDC*/

	/* send regulatory information to firmware */
	rlmDomainSendInfoToFirmware(prAdapter);

#if CFG_SUPPORT_CRYPTO
	g_prAdapter = prAdapter;
#endif

	/* set MAC address */
	if (!bAtResetFlow) {
		uint8_t i;
		uint32_t rStatus = WLAN_STATUS_FAILURE;
		struct sockaddr MacAddr = {0};
		uint32_t u4SetInfoLen = 0;
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		u8 addr[ETH_ALEN];
#endif

		for (i = 0; i < KAL_AIS_NUM; i++) {
			struct net_device *ndev =
				wlanGetAisNetDev(prGlueInfo, i);

			if (!ndev)
				continue;

			u4SetInfoLen = i;
			rStatus = kalIoctl(prGlueInfo, wlanoidQueryCurrentAddr,
					&MacAddr.sa_data, PARAM_MAC_ADDR_LEN,
					&u4SetInfoLen);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, WARN, "set MAC%d addr fail 0x%x\n",
								i, rStatus);
			} else {
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
				ether_addr_copy(addr, MacAddr.sa_data);
				eth_hw_addr_set(ndev, addr);
#else
				kalMemCopy(ndev->dev_addr,
					&MacAddr.sa_data, ETH_ALEN);
#endif
				kalMemCopy(ndev->perm_addr,
					ndev->dev_addr,	ETH_ALEN);
#if CFG_SHOW_MACADDR_SOURCE
				DBGLOG(INIT, INFO, "MAC%d address: " MACSTR, i,
				MAC2STR(&MacAddr.sa_data));
#endif
			}
		}
	}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	/* set HW checksum offload */
	if (!bAtResetFlow && prAdapter->fgIsSupportCsumOffload) {
		uint32_t rStatus;
		uint32_t u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
		uint32_t u4SetInfoLen = 0;

		rStatus = kalIoctl(prGlueInfo, wlanoidSetCSUMOffload,
				   (void *) &u4CSUMFlags,
				   sizeof(uint32_t), &u4SetInfoLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			for (i = 0; i < KAL_AIS_NUM; i++) {
				if (gprWdev[i] && gprWdev[i]->netdev)
					gprWdev[i]->netdev->features |=
						NETIF_F_IP_CSUM |
						NETIF_F_IPV6_CSUM |
						NETIF_F_RXCSUM;
			}
		} else {
			DBGLOG(INIT, WARN,
			       "set HW checksum offload fail 0x%x\n",
			       rStatus);
			prAdapter->fgIsSupportCsumOffload = FALSE;
		}
	}
#endif
	return 0;
}

static void wlanOnPostNetRegister(void)
{
	DBGLOG(INIT, TRACE, "start.\n");
	/* 4 <4> Register early suspend callback */
#if CFG_ENABLE_EARLY_SUSPEND
	glRegisterEarlySuspend(&wlan_early_suspend_desc,
			       wlan_early_suspend, wlan_late_resume);
#endif
	/* 4 <5> Register Notifier callback */
	wlanRegisterInetAddrNotifier();
}

static
void wlanOnP2pRegistration(struct GLUE_INFO *prGlueInfo,
	struct ADAPTER *prAdapter,
	struct wireless_dev *prWdev,
	uint8_t fgIsRtnlLockAcquired)
{
	DBGLOG(INIT, TRACE, "start.\n");

#if CFG_EXT_FEATURE
	kalMemCopy(prGlueInfo->rRegInfo.aucMacAddr,
		prAdapter->rWifiVar.aucMacAddress,
		PARAM_MAC_ADDR_LEN*sizeof(uint8_t));
	DBGLOG(INIT, INFO, "prGlueInfo->rRegInfo.aucMacAddr:" MACSTR,
		MAC2STR(prGlueInfo->rRegInfo.aucMacAddr));
#endif

#if (CFG_ENABLE_WIFI_DIRECT && CFG_MTK_ANDROID_WMT)
	register_set_p2p_mode_handler(set_p2p_mode_handler_wrapper);
#endif

#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->rWifiVar.u4RegP2pIfAtProbe) {
		struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rSetP2P;

		rSetP2P.u4Enable = 1;
		rSetP2P.u4Mode = prAdapter->rWifiVar.ucRegP2pMode;
		rSetP2P.fgIsRtnlLockAcquired = fgIsRtnlLockAcquired;

		if (set_p2p_mode_handler(prWdev->netdev, rSetP2P) == 0)
			DBGLOG(INIT, INFO,
				"%s: p2p device registered\n",
				__func__);
		else
			DBGLOG(INIT, ERROR,
				"%s: Failed to register p2p device\n",
				__func__);
	}
#endif
}

static
int32_t wlanOnWhenProbeSuccess(struct GLUE_INFO *prGlueInfo,
	struct ADAPTER *prAdapter,
	const u_int8_t bAtResetFlow)
{
#if CFG_SUPPORT_PERSIST_NETDEV
	uint8_t i;
#endif

	DBGLOG(INIT, TRACE, "start.\n");

#if CFG_SUPPORT_EASY_DEBUG
	/* move before reading file
	 * wlanLoadDefaultCustomerSetting(prAdapter);
	 */
	wlanFeatureToFw(prGlueInfo->prAdapter, WLAN_CFG_DEFAULT, NULL);

	/*if driver backup Engineer Mode CFG setting before*/
	wlanResoreEmCfgSetting(prGlueInfo->prAdapter);
	wlanFeatureToFw(prGlueInfo->prAdapter, WLAN_CFG_EM, NULL);
#endif

#if CFG_SUPPORT_IOT_AP_BLOCKLIST
	wlanCfgLoadIotApRule(prAdapter);
	wlanCfgDumpIotApRule(prAdapter);
#endif
	if (!bAtResetFlow) {
#if CFG_SUPPORT_AGPS_ASSIST
		kalIndicateAgpsNotify(prAdapter, AGPS_EVENT_WLAN_ON, NULL,
				0);
#endif

		wlanCfgSetSwCtrl(prGlueInfo->prAdapter);
		wlanCfgSetChip(prGlueInfo->prAdapter);
		wlanCfgSetCountryCode(prGlueInfo->prAdapter);
		kalPerMonInit(prGlueInfo);
#if CFG_MET_TAG_SUPPORT
		if (met_tag_init() != 0)
			DBGLOG(INIT, ERROR, "MET_TAG_INIT error!\n");
#endif

#if CFG_SUPPORT_TPENHANCE_MODE
		kalTpeInit(prGlueInfo);
#endif /* CFG_SUPPORT_TPENHANCE_MODE */
	}

#if CFG_SUPPORT_THERMAL_QUERY
	thermal_state_reset(prGlueInfo->prAdapter);
#endif

	/* card is ready */
	prGlueInfo->u4ReadyFlag = 1;
#if CFG_MTK_ANDROID_WMT
	update_driver_loaded_status(prGlueInfo->u4ReadyFlag);
#endif
	kalSetHalted(FALSE);


#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	ics_log_event_notification((int)ICS_LOG_CMD_ON_OFF,
		ics_get_onoff());
#endif
#endif

#if CFG_CHIP_RESET_HANG
	if (fgIsResetHangState == SER_L0_HANG_RST_TRGING) {
		DBGLOG(INIT, STATE, "[SER][L0] SET hang!\n");
			fgIsResetHangState = SER_L0_HANG_RST_HANG;
			glResetUpdateFlag(TRUE);
	}
	DBGLOG(INIT, STATE, "[SER][L0] PASS!!\n");
#endif

	coredump_register_bushang_chk_cb(prAdapter->chip_info->checkbushang);

#if CFG_SUPPORT_PERSIST_NETDEV
	for (i = 0; i < KAL_AIS_NUM; i++) {
		if (gprWdev[i] && gprWdev[i]->netdev)
			netif_device_attach(gprWdev[i]->netdev);
	}
#endif
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	wlanOnP2pRegistration(prGlueInfo, prAdapter, gprWdev[0],
		g_fgWlanOnOffHoldRtnlLock);
#else
	wlanOnP2pRegistration(prGlueInfo, prAdapter, gprWdev[0],
		FALSE);
#endif
	halSetSuspendFlagToFw(prAdapter, FALSE);
#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
	if (wlan_bat_volt == 3550) {
		kalEnableTxPwrBackoffByBattVolt(prAdapter, TRUE);
		kalSetTxPwrBackoffByBattVolt(prAdapter, TRUE);
		fgIsTxPowerDecreased = TRUE;
	} else if (wlan_bat_volt == 3650) {
		kalEnableTxPwrBackoffByBattVolt(prAdapter, TRUE);
		kalSetTxPwrBackoffByBattVolt(prAdapter, FALSE);
		fgIsTxPowerDecreased = FALSE;
	}
#endif

#if (CFG_VOLT_INFO == 1)
	kalVnfInit(prAdapter);
	kalVnfActive(prAdapter);
#endif

	return 0;
}

#if CFG_SUPPORT_NAN
int set_nan_handler(struct net_device *netdev, uint32_t ucEnable,
	uint8_t fgIsHoldRtnlLock)
{
	struct GLUE_INFO *prGlueInfo =
		*((struct GLUE_INFO **)netdev_priv(netdev));
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
#ifdef CFG_DRIVER_INITIAL_RUNNING_MODE
#define NAN_DISABLE_P2P_MODE \
	(CFG_DRIVER_INITIAL_RUNNING_MODE <= RUNNING_DUAL_P2P_MODE)
#else
#define NAN_DISABLE_P2P_MODE (0)
#endif

	if (kalIsResetting())
		return 0;

	if (prGlueInfo->prAdapter->fgIsNANRegistered && ucEnable)
		return 0;
	else if ((!prGlueInfo->prAdapter->fgIsNANRegistered) && (!ucEnable))
		return 0;

#if (NAN_DISABLE_P2P_MODE == 1)
	/* Disable p2p */
	if ((!ucEnable) && (kalIsResetting() == FALSE)) {
		nanNetUnregister(prGlueInfo, fgIsHoldRtnlLock);
	}
	if (ucEnable) {
		struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rSetP2P;

		rSetP2P.u4Mode = 0;
		rSetP2P.u4Enable = 0;
		rSetP2P.fgIsRtnlLockAcquired = fgIsHoldRtnlLock;
		set_p2p_mode_handler(netdev, rSetP2P);
	}
#else
	if (!ucEnable)
		nanNetUnregister(prGlueInfo, fgIsHoldRtnlLock);
#endif

#ifdef CFG_SUPPORT_TWT_EXT
	if (ucEnable)
		twt_teardown_all(wlanGetWiphy(), prGlueInfo->prAdapter);
#endif

	rWlanStatus = kalIoctl(prGlueInfo, wlanoidSetNANMode, (void *)&ucEnable,
			       sizeof(uint32_t), &u4BufLen);

	DBGLOG(INIT, INFO, "set_nan_handler ret = 0x%08x\n",
	       (uint32_t)rWlanStatus);

	/* Need to check fgIsNANRegistered, in case of whole chip reset.
	 * in this case, kalIOCTL return success always,
	 * and prGlueInfo->prP2PInfo[0] may be NULL
	 */
	/* Fixme: error handling */
	if ((ucEnable) && (prGlueInfo->prAdapter->fgIsNANRegistered) &&
		(kalIsResetting() == FALSE))
		nanNetRegister(prGlueInfo, fgIsHoldRtnlLock);

#if (NAN_DISABLE_P2P_MODE == 1)
	/* Disable p2p */
	if ((!ucEnable) && (kalIsResetting() == FALSE)) {
		wlanOnP2pRegistration(prGlueInfo,
			prGlueInfo->prAdapter, gprWdev[0], fgIsHoldRtnlLock);
	}
#endif

	return 0;
}
#endif

void wlanOffWaitWlanThreads(struct completion *prComp,
		struct task_struct *prThread)
{
	uint32_t waitRet = 0;
	struct timespec64 rEntryTs = {0};
	struct timespec64 rNowTs = {0};
	struct timespec64 rTimeout, rTime = {0};
	u_int8_t fgIsTimeout = FALSE;

	if (!prThread) {
		DBGLOG(INIT, INFO, "thread already stop");
		return;
	}

	rTimeout.tv_sec = 10;
	rTimeout.tv_nsec = 0;
	ktime_get_ts64(&rEntryTs);

	while (TRUE) {
		waitRet = wait_for_completion_interruptible_timeout(
			prComp, MSEC_TO_JIFFIES(1000));
		if (waitRet > 0)
			return;
		DBGLOG(INIT, WARN,
			"WlanThread not complete for 1 second:%s[%d]\n",
			prThread->comm, prThread->pid);
		kal_show_stack(NULL, prThread, NULL);

		if (fgIsTimeout)
			continue;

		ktime_get_ts64(&rNowTs);
		if (kalGetDeltaTime(&rNowTs, &rEntryTs, &rTime)) {
			if (kalTimeCompare(&rTime, &rTimeout) >= 0) {
				kalSendAeeWarning("WLAN",
					"off wait threads from %ld.%ld\n",
					rEntryTs.tv_sec, rEntryTs.tv_nsec);
				fgIsTimeout = TRUE;
			}
		}
	}
}

void wlanOffStopWlanThreads(struct GLUE_INFO *prGlueInfo)
{
#if (defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE) && CFG_ENABLE_WAKE_LOCK)
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
#endif

	DBGLOG(INIT, TRACE, "start.\n");

#if (defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE) && CFG_ENABLE_WAKE_LOCK)
	KAL_WAKE_LOCK(prAdapter, prAdapter->prGlueInfo->prDrvOwnWakeLock);
#endif

	prGlueInfo->fgRxTaskReady = FALSE;
#if CFG_SUPPORT_MULTITHREAD
	wake_up_interruptible(&prGlueInfo->waitq_hif);
	wlanOffWaitWlanThreads(&prGlueInfo->rHifHaltComp,
			prGlueInfo->hif_thread);
	wake_up_interruptible(&prGlueInfo->waitq_rx);
	wlanOffWaitWlanThreads(&prGlueInfo->rRxHaltComp,
			prGlueInfo->rx_thread);
#endif

	/* wake up main thread */
	wake_up_interruptible(&prGlueInfo->waitq);
	/* wait main thread stops */
	wlanOffWaitWlanThreads(&prGlueInfo->rHaltComp,
			prGlueInfo->main_thread);

#if (defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE) && CFG_ENABLE_WAKE_LOCK)
	KAL_WAKE_UNLOCK(prAdapter, prAdapter->prGlueInfo->prDrvOwnWakeLock);
#endif

	DBGLOG(INIT, INFO, "wlan thread stopped\n");
}


#if CFG_CHIP_RESET_SUPPORT
/*----------------------------------------------------------------------------*/
/*!
 * \brief slight off procedure for chip reset
 *
 * \return
 * WLAN_STATUS_FAILURE - reset off fail
 * WLAN_STATUS_SUCCESS - reset off success
 */
/*----------------------------------------------------------------------------*/
int32_t wlanOffAtReset(void)
{
	struct ADAPTER *prAdapter = NULL;
	struct net_device *prDev = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
#if CFG_SUPPORT_PERSIST_NETDEV
	uint8_t i;
#endif

	DBGLOG(INIT, STATE, "[SER] Driver Off during Reset\n");

	if (u4WlanDevNum > 0
		&& u4WlanDevNum <= CFG_MAX_WLAN_DEVICES) {
		prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	}

	if (prDev == NULL) {
		DBGLOG(INIT, ERROR, "prDev is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

#if CFG_SUPPORT_PERSIST_NETDEV
	for (i = 0; i < KAL_AIS_NUM; i++) {
		if (gprWdev[i] && gprWdev[i]->netdev)
			netif_device_detach(gprWdev[i]->netdev);
	}
#endif

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, INFO, "prGlueInfo is NULL\n");
		wlanFreeNetDev();
		return WLAN_STATUS_FAILURE;
	}

	/* to avoid that wpa_supplicant/hostapd triogger new cfg80211 command */
	prGlueInfo->u4ReadyFlag = 0;
#if CFG_MTK_ANDROID_WMT
	update_driver_loaded_status(prGlueInfo->u4ReadyFlag);
#endif

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, INFO, "prAdapter is NULL\n");
		wlanFreeNetDev();
		return WLAN_STATUS_FAILURE;
	}
#if (CFG_VOLT_INFO == 1)
	/* Uninit volt info mechanism */
	kalVnfUninit();
#endif
	kalPerMonDestroy(prGlueInfo);

	/* Auto abort test mode at wifi off*/
	if (prAdapter->fgTestMode == TRUE) {
		wlanSetRFTestModeCMD(prGlueInfo, 0);
		/*reset NVRAM State to ready for the next wifi-on*/
		if (g_NvramFsm == NVRAM_STATE_SEND_TO_FW)
			g_NvramFsm = NVRAM_STATE_READY;
	}

	/* complete possible pending oid, which may block wlanRemove some time
	 * and then whole chip reset may failed
	 */
	wlanReleasePendingOid(prGlueInfo->prAdapter, 1);

	prNetDevPrivate
			= (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);

	if (!prNetDevPrivate)
		DBGLOG(REQ, WARN, "prNetDevPrivate is NULL\n");
	else
		cancel_work_sync(&(prNetDevPrivate->workq));

	flush_delayed_work(&sched_workq);

	/* 4 <2> Mark HALT, notify main thread to stop, and clean up queued
	 *	 requests
	 */
	set_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag);
#if CFG_SUPPORT_MULTITHREAD
	/* Stop works */
	flush_work(&prGlueInfo->rTxMsduFreeWork);
#endif
	wlanOffStopWlanThreads(prGlueInfo);

	glTxRxUninit(prGlueInfo);

	wlanAdapterStop(prAdapter, TRUE);

	kalWlanUeventDeinit(prGlueInfo);

	/* 4 <x> Stopping handling interrupt and free IRQ */
	prBusInfo = prAdapter->chip_info->bus_info;
	nicDisableInterrupt(prAdapter);
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	if (prBusInfo->disableSwInterrupt)
		prBusInfo->disableSwInterrupt(prAdapter);
#endif
	glBusFreeIrq(prDev, prGlueInfo);

#if (CFG_SUPPORT_TRACE_TC4 == 1)
	wlanDebugTC4Uninit();
#endif

#if (CFG_SUPPORT_STATISTICS == 1)
	wlanWakeStaticsUninit();
#endif

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	if (prAdapter->chip_info->coexpccifoff)
		prAdapter->chip_info->coexpccifoff(prAdapter);
#else /* CFG_MTK_SUPPORT_LIGHT_MDDP */
	if (prAdapter->chip_info->fw_dl_ops->mcu_deinit)
		prAdapter->chip_info->fw_dl_ops->mcu_deinit(prAdapter);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

	fgSimplifyResetFlow = TRUE;

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A slight wlan on for chip reset
 *
 * \return
 * WLAN_STATUS_FAILURE - reset on fail
 * WLAN_STATUS_SUCCESS - reset on success
 */
/*----------------------------------------------------------------------------*/
int32_t wlanOnAtReset(void)
{
	struct net_device *prDev = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint32_t u4DisconnectReason = DISCONNECT_REASON_CODE_CHIPRESET;
	uint32_t u4Idx = 0;

	enum ENUM_PROBE_FAIL_REASON {
		BUS_INIT_FAIL,
		NET_CREATE_FAIL,
		BUS_SET_IRQ_FAIL,
		ADAPTER_START_FAIL,
		NET_REGISTER_FAIL,
		PROC_INIT_FAIL,
		FAIL_REASON_NUM
	} eFailReason = FAIL_REASON_NUM;

#if CFG_SUPPORT_PCIE_GEN_SWITCH
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	uint8_t ucBssIdx = 0;
#endif

	DBGLOG(INIT, STATE, "[SER] Driver On during Reset\n");

	if (u4WlanDevNum > 0
		&& u4WlanDevNum <= CFG_MAX_WLAN_DEVICES) {
		prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;
	}

	if (prDev == NULL) {
		DBGLOG(INIT, ERROR, "prDev is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, INFO, "prGlueInfo is NULL\n");
		wlanFreeNetDev();
		return WLAN_STATUS_FAILURE;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, INFO, "prAdapter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	if (prAdapter->chip_info->coexpccifon)
		prAdapter->chip_info->coexpccifon(prAdapter);
#else /* CFG_MTK_SUPPORT_LIGHT_MDDP */
	if (prAdapter->chip_info->fw_dl_ops->mcu_init)
		prAdapter->chip_info->fw_dl_ops->mcu_init(prAdapter);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

	prGlueInfo->ulFlag = 0;
	fgSimplifyResetFlow = FALSE;

#if CFG_SUPPORT_PCIE_GEN_SWITCH
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prBusInfo->pcie_current_speed = PCIE_GEN3;
#endif

	do {
#if (CFG_SUPPORT_TRACE_TC4 == 1)
		wlanDebugTC4Init();
#endif
#if (CFG_SUPPORT_STATISTICS == 1)
		wlanWakeStaticsInit();
#endif

		if (prGlueInfo->i4TxPendingCmdNum != 0) {
			DBGLOG(INIT, INFO, "wlanOnReset clear %d command\n",
				prGlueInfo->i4TxPendingCmdNum);

			kalClearCommandQueue(prAdapter->prGlueInfo, FALSE);
		}

		/* wlanNetCreate partial process */
		QUEUE_INITIALIZE(&prGlueInfo->rCmdQueue);
		prGlueInfo->i4TxPendingCmdNum = 0;
		QUEUE_INITIALIZE(&prGlueInfo->rTxQueue);

		glResetHifInfo(prGlueInfo);

		/*
		 * interrupt may come in after setup irq
		 * we need to make sure that rx is ready before it
		 */
		glTxRxInit(prGlueInfo);

		rStatus = glBusSetIrq(prDev, NULL, prGlueInfo);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "Set IRQ error\n");
			eFailReason = BUS_SET_IRQ_FAIL;
			break;
		}

		/* Trigger the action of switching Pwr state to drv_own */
		prAdapter->fgIsFwOwn = TRUE;

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		for (ucBssIdx = 0; ucBssIdx < MAX_BSSID_NUM; ucBssIdx++)
			prAdapter->e6GPwrMode[ucBssIdx] = PWR_MODE_6G_LPI;
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE == 1 */

		/* Need re-init rPendComp.done = 0, due to racing issue
		 * between main_thread & kernel thread(kalIoctlByBssIdx)
		 * when process pending OID CMD, duplicate call completion
		 * result rPendComp.done = 1 in the next round begin
		 * ==> next round 1st OID cmd fail.
		 */
		kal_reinit_completion(&prGlueInfo->rPendComp);

		/* wlanAdapterStart Section Start */
		rStatus = wlanAdapterStart(prAdapter,
					   &prGlueInfo->rRegInfo,
					   TRUE);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			eFailReason = ADAPTER_START_FAIL;
			break;
		}

		kalWlanUeventInit(prGlueInfo);

		if (wlanOnPreNetRegister(prGlueInfo, prAdapter,
					 prAdapter->chip_info,
					 &prAdapter->rWifiVar,
					 TRUE)) {
			rStatus = WLAN_STATUS_FAILURE;
			eFailReason = NET_REGISTER_FAIL;
			break;
		}

		/* Resend schedule scan */
		prAdapter->rWifiVar.rScanInfo.fgSchedScanning = FALSE;
#if CFG_SUPPORT_SCHED_SCAN
		if (prGlueInfo->prSchedScanRequest) {
			rStatus = kalIoctl(prGlueInfo, wlanoidSetStartSchedScan,
					prGlueInfo->prSchedScanRequest,
					sizeof(struct PARAM_SCHED_SCAN_REQUEST),
					&u4BufLen);
			if (rStatus != WLAN_STATUS_SUCCESS)
				DBGLOG(INIT, WARN,
				"SCN: Start sched scan failed after chip reset 0x%x\n",
					rStatus);
		}
#endif
	} while (FALSE);

	if (rStatus == WLAN_STATUS_SUCCESS) {
		wlanOnWhenProbeSuccess(prGlueInfo, prAdapter, TRUE);
		DBGLOG(INIT, INFO, "reset success\n");

		/* Clear pending request (SCAN). */
		scnFreeAllPendingScanRquests(prAdapter);

		/* If scan state is SCAN_STATE_SCANNING, means
		 * that have scan req not done before SER.
		 * Abort this request to prevent scan fail
		 * (scan state back to IDLE).
		 */
		aisFsmStateAbort_SCAN_All(prAdapter);
		/* Send disconnect */
		for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {

			if (!AIS_MAIN_BSS_INFO(prAdapter, u4Idx))
				continue;

			/* Clear pending request (AIS). */
			aisFsmFlushRequest(prAdapter, u4Idx);

			rStatus = kalIoctlByBssIdx(prGlueInfo,
				wlanoidSetDisassociate,
				&u4DisconnectReason,
				0, &u4BufLen,
				AIS_MAIN_BSS_INDEX(prAdapter, u4Idx));

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, WARN,
					"disassociate error:%x\n", rStatus);
				continue;
			}
			DBGLOG(INIT, INFO,
				"%d inform disconnected\n", u4Idx);
		}
	} else {
		prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
		halPrintHifDbgInfo(prAdapter);
		DBGLOG(INIT, WARN, "Fail reason: %d\n", eFailReason);

		/* Remove error handling here, leave it to coming wlanRemove
		 * for full clean.
		 *
		 * If WMT being removed in the future, you should invoke
		 * wlanRemove directly from here
		 */
		kalSendAeeWarning("WFSYS", "wlanOnAtReset fail\n");
		wlanRemove();

#if 0
		switch (eFailReason) {
		case ADAPTER_START_FAIL:
			wlanWakeLockUninit(prGlueInfo);
			wlanNetDestroy(prDev->ieee80211_ptr);
			/* prGlueInfo->prAdapter is released in
			 * wlanNetDestroy
			 */
			/* Set NULL value for local prAdapter as well */
			prAdapter = NULL;
			break;
		default:
			break;
		}
#endif
	}

	DBGLOG(INIT, INFO, "reinit thread's completion\n");
#if CFG_SUPPORT_MULTITHREAD
	reinit_completion(&prGlueInfo->rHifHaltComp);
	reinit_completion(&prGlueInfo->rRxHaltComp);
#endif
	reinit_completion(&prGlueInfo->rHaltComp);
	return rStatus;
}
#endif

u_int8_t wlanIsProbing(void)
{
	return GLUE_GET_REF_CNT(g_wlanProbing) ? TRUE : FALSE;
}

u_int8_t wlanIsRemoving(void)
{
	return GLUE_GET_REF_CNT(g_wlanRemoving) ? TRUE : FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Wlan probe function. This function probes and initializes the device.
 *
 * \param[in] pvData     data passed by bus driver init function
 *                           _HIF_EHPI: NULL
 *                           _HIF_SDIO: sdio bus driver handle
 *
 * \retval 0 Success
 * \retval negative value Failed
 */
/*----------------------------------------------------------------------------*/
static int32_t wlanProbe(void *pvData, void *pvDriverData)
{
	struct wireless_dev *prWdev = NULL;
	enum ENUM_PROBE_FAIL_REASON {
		BUS_INIT_FAIL,
		NET_CREATE_FAIL,
		ROM_DL_FAIL,
		BUS_SET_IRQ_FAIL,
		ADAPTER_START_FAIL,
		NET_REGISTER_FAIL,
		PROC_INIT_FAIL,
		FAIL_MET_INIT_PROCFS,
		FAIL_BY_RESET,
		FAIL_REASON_NUM
	} eFailReason;
	int32_t i4DevIdx = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t i4Status = 0;
	u_int8_t bRet = FALSE;
#if CFG_ENABLE_WIFI_DIRECT
	u_int8_t i = 0;
#endif
	struct REG_INFO *prRegInfo;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WIFI_VAR *prWifiVar;
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	struct mt66xx_hif_driver_data *prHifDriverData;
#endif
#if CFG_SUPPORT_PCIE_GEN_SWITCH
	struct BUS_INFO *prBusInfo;
#endif

	if (GLUE_GET_REF_CNT(g_wlanProbing)) {
		DBGLOG(INIT, ERROR, "%s in process\n", __func__);
		return 0;
	}
	GLUE_SET_REF_CNT(1, g_wlanProbing);

#if CFG_CHIP_RESET_KO_SUPPORT
	send_reset_event(RESET_MODULE_TYPE_WIFI, RFSM_EVENT_PROBE_START);
#endif

#if CFG_CHIP_RESET_SUPPORT
	if (fgSimplifyResetFlow) {
		i4Status = wlanOnAtReset();
#if CFG_MTK_MDDP_SUPPORT
		if (i4Status == WLAN_STATUS_SUCCESS)
			mddpNotifyWifiOnEnd(FALSE);
#endif
		goto WLAN_PROBE_RETURN;
	}
	glResetUpdateFlag(FALSE);
#endif

	eFailReason = FAIL_REASON_NUM;
	do {
		/* 4 <1> Initialize the IO port of the interface */
		/*  GeorgeKuo: pData has different meaning for _HIF_XXX:
		 * _HIF_EHPI: pointer to memory base variable, which will be
		 *      initialized by glBusInit().
		 * _HIF_SDIO: bus driver handle
		 */
		DBGLOG(INIT, INFO, "enter wlanProbe\n");

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
		fgIsCurrentInTestMode = FALSE;
#endif
		bRet = glBusInit(pvData);

#if (CFG_SUPPORT_TRACE_TC4 == 1)
		wlanDebugTC4Init();
#endif

#if (CFG_SUPPORT_STATISTICS == 1)
		wlanWakeStaticsInit();
#endif
		/* Cannot get IO address from interface */
		if (bRet == FALSE) {
			DBGLOG(INIT, ERROR, "wlanProbe: glBusInit() fail\n");
			i4Status = -EIO;
			eFailReason = BUS_INIT_FAIL;
			break;
		}
		/* 4 <2> Create network device, Adapter, KalInfo,
		 *       prDevHandler(netdev)
		 */
		prWdev = wlanNetCreate(pvData, pvDriverData);
		if (prWdev == NULL) {
			DBGLOG(INIT, ERROR,
			       "wlanProbe: No memory for dev and its private\n");
			i4Status = -ENOMEM;
			eFailReason = NET_CREATE_FAIL;
			break;
		}
		/* 4 <2.5> Set the ioaddr to HIF Info */
		WIPHY_PRIV(prWdev->wiphy, prGlueInfo);
		gPrDev = prGlueInfo->prDevHandler;

		/* 4 <4> Setup IRQ */
		prGlueInfo->i4DevIdx = i4DevIdx;
		prAdapter = prGlueInfo->prAdapter;
		prWifiVar = &prAdapter->rWifiVar;
		prChipInfo = prAdapter->chip_info;

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
		if (prAdapter->chip_info->coexpccifon)
			prAdapter->chip_info->coexpccifon(prAdapter);
#else /* CFG_MTK_SUPPORT_LIGHT_MDDP */
		if (prChipInfo->fw_dl_ops->mcu_init)
			i4Status = prChipInfo->fw_dl_ops->mcu_init(prAdapter);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "WF MCU init failed.\n");
			eFailReason = ROM_DL_FAIL;
			break;
		}

#if CFG_WMT_RESET_API_SUPPORT
		prAdapter->fgIsSkipFWL05 = TRUE;
#endif
		/*
		 * interrupt may come in after setup irq
		 * we need to make sure that rx is ready before it
		 */
		glTxRxInit(prGlueInfo);

		i4Status = glBusSetIrq(prWdev->netdev, NULL, prGlueInfo);

		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "wlanProbe: Set IRQ error\n");
			eFailReason = BUS_SET_IRQ_FAIL;
			break;
		}

		prGlueInfo->i4DevIdx = i4DevIdx;

		prAdapter = prGlueInfo->prAdapter;
		prWifiVar = &prAdapter->rWifiVar;

		wlanOnPreAdapterStart(prGlueInfo,
			prAdapter,
			&prRegInfo,
			prChipInfo);

		if (wlanAdapterStart(prAdapter,
				     prRegInfo, FALSE) != WLAN_STATUS_SUCCESS)
			i4Status = -EIO;

		wlanOnPostAdapterStart(prAdapter, prGlueInfo);

		/* kfree(prRegInfo); */

		if (i4Status < 0) {
			eFailReason = ADAPTER_START_FAIL;
			break;
		}

#if CFG_MTK_MDDP_SUPPORT
		if (IS_FEATURE_ENABLED(prWifiVar->fgMddpSupport))
			mddpEnableMddpSupport();
		else
			mddpDisableMddpSupport();

		mddpNotifyWifiOnStart(FALSE);
#endif
		/* FW might send Uevent on start running */
		kalWlanUeventInit(prGlueInfo);

		if (wlanOnPreNetRegister(prGlueInfo, prAdapter, prChipInfo,
					 prWifiVar, FALSE)) {
			i4Status = -EIO;
			eFailReason = NET_REGISTER_FAIL;
			break;
		}

		/* 4 <3> Register the card */
		i4DevIdx = wlanNetRegister(prWdev);
		if (i4DevIdx < 0) {
			i4Status = -ENXIO;
			DBGLOG(INIT, ERROR,
			       "wlanProbe: Cannot register the net_device context to the kernel\n");
			eFailReason = NET_REGISTER_FAIL;
			break;
		}

		wlanOnPostNetRegister();

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
		prHifDriverData = (struct mt66xx_hif_driver_data *)pvDriverData;
		prAdapter->u4PwrLevel = prHifDriverData->u4PwrLevel;
		kalMemCopy(&prAdapter->rTempInfo, &prHifDriverData->rTempInfo,
				sizeof(struct conn_pwr_event_max_temp));
		connsys_power_event_notification(CONN_PWR_EVENT_LEVEL,
				&(prAdapter->u4PwrLevel));

		power_throttling_start();
#endif

		/* 4 <6> Initialize /proc filesystem */
#if WLAN_INCLUDE_PROC
		i4Status = procCreateFsEntry(prGlueInfo);
		if (i4Status < 0) {
			DBGLOG(INIT, ERROR, "wlanProbe: init procfs failed\n");
			eFailReason = PROC_INIT_FAIL;
			break;
		}
#endif /* WLAN_INCLUDE_PROC */
#if WLAN_INCLUDE_SYS
		i4Status = sysCreateFsEntry(prGlueInfo);
		if (i4Status < 0) {
			DBGLOG(INIT, ERROR, "wlanProbe: init sysfs failed\n");
			eFailReason = PROC_INIT_FAIL;
			break;
		}
#endif /* WLAN_INCLUDE_SYS */

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
		prGlueInfo->fgIsEnableMon = FALSE;
		sysCreateMonDbgFs(prGlueInfo);
#endif

#if CFG_MET_PACKET_TRACE_SUPPORT
		kalMetInit(prGlueInfo);
#endif

#if CFG_SUPPORT_CSI
		glCsiSupportInit(prGlueInfo);
#endif

#if CFG_ENABLE_BT_OVER_WIFI
		prGlueInfo->rBowInfo.fgIsNetRegistered = FALSE;
		prGlueInfo->rBowInfo.fgIsRegistered = FALSE;
		glRegisterAmpc(prGlueInfo);
#endif

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
		/* dynamic tx power control load configuration */
		/* note: call this API after loading NVRAM */
		/* note: call this API after main thread is start */
		txPwrCtrlLoadConfig(prAdapter);
#endif

#if (CONFIG_WLAN_SERVICE == 1)
		wlanServiceInit(prGlueInfo);
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
		wlanOpenIdxLogBin(prAdapter);
#endif

#if CFG_SUPPORT_MBRAIN
		glRegCbsToMbraink(prAdapter);
#endif

		/* Configure 5G band for registered wiphy */
		if (prAdapter->fgEnable5GBand)
			prWdev->wiphy->bands[KAL_BAND_5GHZ] = &mtk_band_5ghz;
		else
			prWdev->wiphy->bands[KAL_BAND_5GHZ] = NULL;

#if CFG_ENABLE_WIFI_DIRECT
		for (i = 0 ; i < KAL_P2P_NUM; i++) {
			if (gprP2pRoleWdev[i] == NULL)
				continue;

			if (prAdapter->fgEnable5GBand)
				gprP2pRoleWdev[i]->wiphy->bands[KAL_BAND_5GHZ] =
				&mtk_band_5ghz;
			else
				gprP2pRoleWdev[i]->wiphy->bands[KAL_BAND_5GHZ] =
				NULL;
		}
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
		/* Configure 6G band for registered wiphy */
		if (prAdapter->fgIsHwSupport6G)
			prWdev->wiphy->bands[KAL_BAND_6GHZ] = &mtk_band_6ghz;
		else
			prWdev->wiphy->bands[KAL_BAND_6GHZ] = NULL;

#if CFG_ENABLE_WIFI_DIRECT
		for (i = 0 ; i < KAL_P2P_NUM; i++) {
			if (gprP2pRoleWdev[i] == NULL)
				continue;

			if (prAdapter->fgIsHwSupport6G)
				gprP2pRoleWdev[i]->wiphy->bands[KAL_BAND_6GHZ] =
				&mtk_band_6ghz;
			else
				gprP2pRoleWdev[i]->wiphy->bands[KAL_BAND_6GHZ] =
				NULL;
		}
#endif
#endif
	} while (FALSE);

	if (i4Status == 0 && kalIsResetting()) {
		DBGLOG(INIT, WARN, "Fake wlan on success due to reset.\n");
		eFailReason = FAIL_BY_RESET;
		i4Status = WLAN_STATUS_FAILURE;
	}

	if (i4Status == 0) {
		wlanOnWhenProbeSuccess(prGlueInfo, prAdapter, FALSE);

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
		/* After switch test mode FW, auto send enter test mode CMD */
		if (get_wifi_test_mode_fwdl() == 1)
			wlanSetRFTestModeCMD(prGlueInfo, 1);
#endif

		DBGLOG(INIT, INFO,
		       "wlanProbe: probe success, feature set: 0x%llx, persistNetdev: %d\n",
		       wlanGetSupportedFeatureSet(prGlueInfo),
		       CFG_SUPPORT_PERSIST_NETDEV);
#if CFG_MTK_MDDP_SUPPORT
		mddpNotifyWifiOnEnd(FALSE);
#endif
	} else {
		DBGLOG(INIT, ERROR, "wlanProbe: probe failed, reason:%d\n",
		       eFailReason);
		switch (eFailReason) {
		case FAIL_BY_RESET:
#if CFG_SUPPORT_MBRAIN
			glUnregCbsToMbraink();
#endif
			procRemoveProcfs();
			kal_fallthrough;
			/* fallthrough */
		case PROC_INIT_FAIL:
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
			power_throttling_stop();
#endif
			wlanNetUnregister(prWdev);
			/* Unregister notifier callback */
			wlanUnregisterInetAddrNotifier();
			kal_fallthrough;
		case NET_REGISTER_FAIL:
			set_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag);
#if CFG_SUPPORT_MULTITHREAD
			wake_up_interruptible(&prGlueInfo->waitq_hif);
			wait_for_completion_interruptible(
				&prGlueInfo->rHifHaltComp);
			wake_up_interruptible(&prGlueInfo->waitq_rx);
			wait_for_completion_interruptible(
				&prGlueInfo->rRxHaltComp);
#endif
			/* wake up main thread */
			wake_up_interruptible(&prGlueInfo->waitq);
			/* wait main thread stops */
			wait_for_completion_interruptible(
							&prGlueInfo->rHaltComp);
			wlanAdapterStop(prAdapter, FALSE);
			kalWlanUeventDeinit(prGlueInfo);
		kal_fallthrough;
		case ADAPTER_START_FAIL:
			/*reset NVRAM State to ready for the next wifi-on*/
			if (g_NvramFsm == NVRAM_STATE_SEND_TO_FW)
				g_NvramFsm = NVRAM_STATE_READY;
			glBusFreeIrq(prWdev->netdev,
				*((struct GLUE_INFO **)
						netdev_priv(prWdev->netdev)));
		kal_fallthrough;
		case BUS_SET_IRQ_FAIL:
			glTxRxUninit(prGlueInfo);
			if (prChipInfo && prChipInfo->fw_dl_ops->mcu_deinit)
				prChipInfo->fw_dl_ops->mcu_deinit(prAdapter);
		kal_fallthrough;
		case ROM_DL_FAIL:
			wlanWakeLockUninit(prGlueInfo);
			wlanNetDestroy(prWdev);
			/* prGlueInfo->prAdapter is released in
			 * wlanNetDestroy
			 */
			/* Set NULL value for local prAdapter as well */
			prAdapter = NULL;
		kal_fallthrough;
		case NET_CREATE_FAIL:
		kal_fallthrough;
		case BUS_INIT_FAIL:
#if (CFG_SUPPORT_STATISTICS == 1)
			wlanWakeStaticsUninit();
#endif
#if (CFG_SUPPORT_TRACE_TC4 == 1)
			wlanDebugTC4Uninit();
#endif
			break;
		default:
			break;
		}
	}

#if CFG_SUPPORT_PCIE_GEN_SWITCH
	if (prChipInfo && prChipInfo->bus_info) {
		prBusInfo = prChipInfo->bus_info;
		prBusInfo->pcie_current_speed = PCIE_GEN3;
	}
#endif

WLAN_PROBE_RETURN:
	glReseProbeRemoveDone(prGlueInfo, i4Status, TRUE);
	GLUE_SET_REF_CNT(0, g_wlanProbing);

	return i4Status;
}				/* end of wlanProbe() */

void
wlanOffNotifyCfg80211Disconnect(struct GLUE_INFO *prGlueInfo)
{
	uint32_t u4Idx = 0;
	u_int8_t bNotify = FALSE;

	DBGLOG(INIT, TRACE, "start.\n");

	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		struct net_device *prDevHandler =
			wlanGetAisNetDev(prGlueInfo, u4Idx);
		uint8_t ucBssIndex = 0;

		if (!prDevHandler)
			continue;

		ucBssIndex = AIS_MAIN_BSS_INDEX(prGlueInfo->prAdapter, u4Idx);
 		if (kalGetMediaStateIndicated(prGlueInfo, ucBssIndex) ==
		    MEDIA_STATE_CONNECTED) {
#if CFG_WPS_DISCONNECT || (KERNEL_VERSION(4, 2, 0) <= CFG80211_VERSION_CODE)
			cfg80211_disconnected(
				prDevHandler, 0, NULL, 0,
				TRUE, GFP_KERNEL);
#else
			cfg80211_disconnected(
				prDevHandler, 0, NULL, 0,
				GFP_KERNEL);
#endif
			bNotify = TRUE;
		}
	}
	if (bNotify)
		kalMsleep(500);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method to stop driver operation and release all resources. Following
 *        this call, no frame should go up or down through this interface.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanRemove(void)
{
	struct net_device *prDev = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	u_int8_t fgResult = FALSE;
#if CFG_SUPPORT_PERSIST_NETDEV
	uint8_t i;
#endif

	DBGLOG(INIT, INFO, "Remove wlan!\n");

	if (GLUE_GET_REF_CNT(g_wlanRemoving)) {
		DBGLOG(INIT, ERROR, "%s in process\n", __func__);
		return;
	}
	GLUE_SET_REF_CNT(1, g_wlanRemoving);

	kalSetHalted(TRUE);

	/*reset NVRAM State to ready for the next wifi-no*/
	if (g_NvramFsm == NVRAM_STATE_SEND_TO_FW)
		g_NvramFsm = NVRAM_STATE_READY;

#if CFG_MTK_MDDP_SUPPORT
	mddpNotifyWifiOffStart();
#endif

#if CFG_CHIP_RESET_SUPPORT
	/* During chip reset, use simplify remove flow first
	 * if anything goes wrong in wlanOffAtReset then goes to normal flow
	 */
	if (fgSimplifyResetFlow) {
		if (wlanOffAtReset() == WLAN_STATUS_SUCCESS) {
#if CFG_MTK_MDDP_SUPPORT
			mddpNotifyWifiOffEnd();
#endif
			goto WLAN_REMOVE_RETURN;
		}
	}
#endif

	/* 4 <0> Sanity check */
	ASSERT(u4WlanDevNum <= CFG_MAX_WLAN_DEVICES);
	if (u4WlanDevNum == 0) {
		DBGLOG(INIT, ERROR, "u4WlanDevNum = 0\n");
		goto WLAN_REMOVE_RETURN;
	}
#if (CFG_ENABLE_WIFI_DIRECT && CFG_MTK_ANDROID_WMT)
	register_set_p2p_mode_handler(NULL);
#endif
	if (u4WlanDevNum > 0 &&
	    u4WlanDevNum <= CFG_MAX_WLAN_DEVICES)
		prDev = arWlanDevInfo[u4WlanDevNum - 1].prDev;

	ASSERT(prDev);
	if (prDev == NULL) {
		DBGLOG(INIT, ERROR, "prDev is NULL\n");
		goto WLAN_REMOVE_RETURN;
	}

#if CFG_SUPPORT_PERSIST_NETDEV
	for (i = 0; i < KAL_AIS_NUM; i++) {
		if (gprWdev[i] && gprWdev[i]->netdev &&
		    gprWdev[i]->netdev->reg_state == NETREG_REGISTERED) {
			netif_device_detach(gprWdev[i]->netdev);
			if (i != AIS_DEFAULT_INDEX) {
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
				if (!g_fgWlanOnOffHoldRtnlLock)
#endif
					rtnl_lock();
				mtk_cfg80211_del_iface(gprWdev[i]->wiphy,
						       gprWdev[i]);
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
				if (!g_fgWlanOnOffHoldRtnlLock)
#endif
					rtnl_unlock();
			}
		}
	}
#endif

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	ASSERT(prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, INFO, "prGlueInfo is NULL\n");
		wlanFreeNetDev();
		goto WLAN_REMOVE_RETURN;
	}
	prAdapter = prGlueInfo->prAdapter;

	coredump_register_bushang_chk_cb(NULL);

	/* to avoid that wpa_supplicant/hostapd triogger new cfg80211 command */
	prGlueInfo->u4ReadyFlag = 0;
#if CFG_MTK_ANDROID_WMT
	update_driver_loaded_status(prGlueInfo->u4ReadyFlag);
#endif

	/* Auto abort test mode at wifi off*/
	if (prAdapter->fgTestMode == TRUE) {
		wlanSetRFTestModeCMD(prGlueInfo, 0);
		/*reset NVRAM State to ready for the next wifi-on*/
		if (g_NvramFsm == NVRAM_STATE_SEND_TO_FW)
			g_NvramFsm = NVRAM_STATE_READY;
	}

#if (CONFIG_WLAN_SERVICE == 1)
	wlanServiceExit(prGlueInfo);
#endif

	/* Have tried to do scan done here, but the exception occurs for */
	/* the P2P scan. Keep the original design that scan done in the	 */
	/* p2pStop/wlanStop.						 */

#if CFG_SUPPORT_MBRAIN
	glUnregCbsToMbraink();
#endif

#if WLAN_INCLUDE_PROC
	procRemoveProcfs();
#endif /* WLAN_INCLUDE_PROC */
#if WLAN_INCLUDE_SYS
	sysRemoveSysfs();
#endif /* WLAN_INCLUDE_SYS */
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	sysRemoveMonDbgFs();
#endif
	kalPerMonDestroy(prGlueInfo);

	/* Unregister notifier callback */
	wlanUnregisterInetAddrNotifier();

	/*backup EM mode cfg setting*/
	wlanBackupEmCfgSetting(prAdapter);

#if CFG_SUPPORT_TPENHANCE_MODE
	kalTpeUninit(prGlueInfo);
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

	/* Need to get A-DIE ver anytime when device plug in,
	 * or will fail on the case with insert different A-DIE card.
	 */
	prAdapter->chip_info->u4ADieVer = 0xFFFFFFFF;

	prAdapter->CurNoResSeqID = 0;

	/* complete possible pending oid, which may block wlanRemove some time
	 * and then whole chip reset may failed
	 */
	if (kalIsResetting())
		wlanReleasePendingOid(prGlueInfo->prAdapter, 1);

#if CFG_ENABLE_BT_OVER_WIFI
	if (prGlueInfo->rBowInfo.fgIsNetRegistered) {
		bowNotifyAllLinkDisconnected(prGlueInfo->prAdapter);
		/* wait 300ms for BoW module to send deauth */
		kalMsleep(300);
	}
#endif

	wlanOffNotifyCfg80211Disconnect(prGlueInfo);

	/* 20150205 work queue for sched_scan */

	flush_delayed_work(&sched_workq);

	prNetDevPrivate
			= (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);

	if (!prNetDevPrivate)
		DBGLOG(REQ, WARN, "prNetDevPrivate is NULL\n");
	else
		cancel_work_sync(&(prNetDevPrivate->workq));

#if CFG_AP_80211KVR_INTERFACE
	cancel_delayed_work_sync(&prAdapter->prGlueInfo->rChanNoiseControlWork);
	cancel_delayed_work_sync(&prAdapter->prGlueInfo->rChanNoiseGetInfoWork);
#endif

	down(&g_halt_sem);
	g_u4HaltFlag = 1;
	up(&g_halt_sem);

	/* 4 <2> Mark HALT, notify main thread to stop, and clean up queued
	 *       requests
	 */
	set_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag);

	/* Stop works */
#if CFG_SUPPORT_MULTITHREAD
	flush_work(&prGlueInfo->rTxMsduFreeWork);
#endif
	cancel_delayed_work_sync(&prGlueInfo->rRxPktDeAggWork);

	nicSerDeInit(prGlueInfo->prAdapter);

	wlanOffStopWlanThreads(prGlueInfo);

	glTxRxUninit(prGlueInfo);

#if (CFG_VOLT_INFO == 1)
	/* Uninit volt info mechanis */
	kalVnfUninit();
#endif

	/* Destroy wakelock */
	wlanWakeLockUninit(prGlueInfo);

	kalMemSet(&prAdapter->rWlanInfo, 0, sizeof(struct WLAN_INFO));

#if CFG_ENABLE_WIFI_DIRECT
	if (prGlueInfo->prAdapter->fgIsP2PRegistered) {
		DBGLOG(INIT, INFO, "p2pNetUnregister...\n");
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
		p2pNetUnregister(prGlueInfo, g_fgWlanOnOffHoldRtnlLock);
#else
		p2pNetUnregister(prGlueInfo, FALSE);
#endif
		DBGLOG(INIT, INFO, "p2pRemove...\n");
		/*p2pRemove must before wlanAdapterStop */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
		p2pRemove(prGlueInfo, g_fgWlanOnOffHoldRtnlLock);
#else
		p2pRemove(prGlueInfo, FALSE);
#endif
	}
#endif

#if CFG_SUPPORT_NAN
	if (prGlueInfo->prAdapter->fgIsNANRegistered) {
		DBGLOG(INIT, INFO, "NANNetUnregister...\n");
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
		nanNetUnregister(prGlueInfo, g_fgWlanOnOffHoldRtnlLock);
#else
		nanNetUnregister(prGlueInfo, FALSE);
#endif
		DBGLOG(INIT, INFO, "nanRemove...\n");
		/* nanRemove must before wlanAdapterStop */
		nanRemove(prGlueInfo);
	}
	kalReleaseUserSock(prGlueInfo);
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	kalPwrLevelHdlrUnregisterAll(prAdapter);
#endif

#if CFG_ENABLE_BT_OVER_WIFI
	if (prGlueInfo->rBowInfo.fgIsRegistered)
		glUnregisterAmpc(prGlueInfo);
#endif

#if CFG_SUPPORT_CSI
	glCsiSupportDeinit(prGlueInfo);
#endif

#if CFG_MET_TAG_SUPPORT
	if (GL_MET_TAG_UNINIT() != 0)
		DBGLOG(INIT, ERROR, "MET_TAG_UNINIT error!\n");
#endif

#if CFG_SUPPORT_MET_LOG && (CFG_SUPPORT_CONNAC3X == 1)
	met_log_stop(prGlueInfo);
#endif

	/* 4 <4> wlanAdapterStop */
#if CFG_SUPPORT_AGPS_ASSIST
	kalIndicateAgpsNotify(prAdapter, AGPS_EVENT_WLAN_OFF, NULL,
			      0);
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	wlanCloseIdxLogBin(prAdapter);
#endif

	wlanAdapterStop(prAdapter, FALSE);
	kalWlanUeventDeinit(prGlueInfo);

	/* 4 <x> Stopping handling interrupt and free IRQ */
	glBusFreeIrq(prDev, prGlueInfo);

	/* 4 <5> Release the Bus */
	glBusRelease(prDev);

	HAL_LP_OWN_SET(prAdapter, &fgResult);
	DBGLOG(INIT, INFO, "HAL_LP_OWN_SET(%d)\n",
	       (uint32_t) fgResult);

#if (CFG_SUPPORT_TRACE_TC4 == 1)
	wlanDebugTC4Uninit();
#endif

#if (CFG_SUPPORT_STATISTICS == 1)
	wlanWakeStaticsUninit();
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	power_throttling_stop();
#endif

	/* 4 <6> Unregister the card */
	wlanNetUnregister(gprWdev[0]);

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	if (prAdapter->chip_info->coexpccifoff)
		prAdapter->chip_info->coexpccifoff(prAdapter);
#else /* CFG_MTK_SUPPORT_LIGHT_MDDP */
	if (prAdapter->chip_info->fw_dl_ops->mcu_deinit)
		prAdapter->chip_info->fw_dl_ops->mcu_deinit(prAdapter);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

	/* 4 <7> Destroy the device */
	wlanNetDestroy(gprWdev[0]);
	prDev = NULL;

	/* 4 <8> Unregister early suspend callback */
#if CFG_ENABLE_EARLY_SUSPEND
	glUnregisterEarlySuspend(&wlan_early_suspend_desc);
#endif

#if !CFG_SUPPORT_PERSIST_NETDEV
	{
		uint32_t u4Idx = 0;

		for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
			if (gprWdev[u4Idx] && gprWdev[u4Idx]->netdev)
				gprWdev[u4Idx]->netdev = NULL;
		}
	}
#endif

WLAN_REMOVE_RETURN:
#if CFG_CHIP_RESET_SUPPORT
	glResetUpdateFlag(FALSE);
#endif
#if CFG_MTK_MDDP_SUPPORT
	mddpNotifyWifiOffEnd();
#endif
	glReseProbeRemoveDone(prGlueInfo, 0, FALSE);
	GLUE_SET_REF_CNT(0, g_wlanRemoving);
}				/* end of wlanRemove() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method to stop driver operation and release all resources. Following
 *        this call, no frame should go up or down through this interface.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
#if CFG_MTK_ANDROID_WMT
uint8_t kalGetShutdownState(void)
{
	return uShutdownState;
}
#endif
#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
void wlanShutdown(void)
{
	/* there are two shutdown entry,
	 * one is pre_fmd and another is platform
	 */
	if (kalGetShutdownState()) {
		DBGLOG(REQ, INFO, "shutdown is ongoing\n");
		return;
	}

	uShutdownState = SHUTDOWN_STATE_ONGOING;
	while (kalIsResetOnEnd()) {
		DBGLOG(REQ, WARN, "wifi driver is resetting\n");
		kalMsleep(100);
	}

	wfsys_lock();
	/* wifi is off */
	if ((!get_wifi_powered_status() && get_wifi_process_status() == 0)) {
		wfsys_unlock();
		return;
	}

	DBGLOG(INIT, INFO, "do wifi off\n");
	wlanFuncOff();
	wfsys_unlock();

	uShutdownState = SHUTDOWN_STATE_DONE;
}
#endif

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
int wlanFuncPreOnImpl(void)
{
	struct mt66xx_chip_info *chip = NULL;

	/* should be PRE_ON_PROCESS_DONE */
	fgIsPreOnProcessing = TRUE;

	glBusFuncOn();
	glGetChipInfo((void **)&chip);
	if (chip)
		wlan_pinctrl_action(chip, WLAN_PINCTRL_MSG_FUNC_OFF);

	fgIsPreOnProcessing = FALSE;
	return 0;
}
#endif

int wlanFuncOnImpl(void)
{
	struct mt66xx_chip_info *chip = NULL;
	int ret = 0;

#if CFG_MTK_ANDROID_WMT
	/*
	 * Initialize shutdown status to resolve reset-triggered failures
	 * if no shutdown occurs after pre_fmd callback is done.
	 */
	if (uShutdownState == SHUTDOWN_STATE_DONE)
		uShutdownState = SHUTDOWN_STATE_INIT;
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	power_throttling_pre_start();
#endif

	ret = glBusFuncOn();
	if (ret) {
		DBGLOG(HAL, ERROR,
			"glBusFuncOn failed, ret=%d\n",
			ret);
		goto power_throttling_post_stop;
	}

	goto exit;

power_throttling_post_stop:
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	power_throttling_post_stop();
#endif
	glGetChipInfo((void **)&chip);
	if (chip)
		wlan_pinctrl_action(chip, WLAN_PINCTRL_MSG_FUNC_OFF);
exit:
	return ret;
}

void wlanFuncOffImpl(void)
{
	struct mt66xx_chip_info *chip = NULL;

	glBusFuncOff();
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	power_throttling_post_stop();
#endif

	glGetChipInfo((void **)&chip);
	if (chip)
		wlan_pinctrl_action(chip, WLAN_PINCTRL_MSG_FUNC_OFF);
}

int wlanFuncOn(void)
{
	int ret = 0;

	ret = connsys_power_on();
	if (ret)
		goto exit;

	ret = wlanFuncOnImpl();
	if (ret) {
		DBGLOG(HAL, ERROR, "wlanFuncOnImpl failed, ret=%d\n",
			ret);
		goto connsys_pwr_off;
	}

	ret = connsys_power_done();
	if (ret)
		goto func_off;

	goto exit;

func_off:
	wlanFuncOffImpl();
connsys_pwr_off:
	connsys_power_off();
exit:
	return ret;
}

int wlanFuncOff(void)
{
	wlanFuncOffImpl();
	connsys_power_off();

	return 0;
}

#if CFG_MTK_ANDROID_WMT
static int wlanGetBootMode(void)
{
	struct device_node *dnode = NULL;
	struct tag_bootmode *tag = NULL;

	dnode = of_find_node_by_path("/chosen");
	if (dnode == NULL)
		dnode = of_find_node_by_path("/chosen@0");

	if (dnode == NULL) {
		DBGLOG(INIT, ERROR, "failed to get chosen node\n");
		return -1;
	}

	tag = (struct tag_bootmode *)of_get_property(dnode, "atag,boot", NULL);
	if (tag == NULL) {
		DBGLOG(INIT, ERROR, "failed to get atag,boot\n");
		return -1;
	}
	of_node_put(dnode);
	DBGLOG(INIT, INFO, "bootmode: 0x%x\n", tag->bootmode);
	return tag->bootmode;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver entry point when the driver is configured as a Linux Module,
 *        and is called once at module load time, by the user-level modutils
 *        application: insmod or modprobe.
 *
 * \retval 0     Success
 */
/*----------------------------------------------------------------------------*/
/* 1 Module Entry Point */
static int initWlan(void)
{
	int ret = 0;
#if CFG_MTK_ANDROID_WMT || CFG_MTK_MDDP_SUPPORT
	int bootmode = NORMAL_BOOT;
#endif
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *chip = NULL;

	DBGLOG(INIT, INFO, "initWlan\n");

#if CFG_MTK_ANDROID_WMT
	/* Skip module init if bootmode is KPOC */
	bootmode = wlanGetBootMode();
	if (bootmode == KERNEL_POWER_OFF_CHARGING_BOOT)
		return -1;
#endif

#if (CFG_CHIP_RESET_SUPPORT)
#if CFG_CHIP_RESET_KO_SUPPORT
	resetko_register_module(RESET_MODULE_TYPE_WIFI,
				"wifi",
				TRIGGER_RESET_TYPE_GPIO_API,
				resetkoReset,
				resetkoNotifyFunc);
#endif  /* CFG_CHIP_RESET_KO_SUPPORT */
#endif  /* CFG_CHIP_RESET_SUPPORT */

#if CFG_SUPPORT_WED_PROXY
	wedInitial();
#endif

#ifdef CFG_DRIVER_INF_NAME_CHANGE
	if (kalStrLen(gprifnamesta) > CUSTOM_IFNAMESIZ ||
	    kalStrLen(gprifnamep2p) > CUSTOM_IFNAMESIZ ||
	    kalStrLen(gprifnameap) > CUSTOM_IFNAMESIZ) {
		DBGLOG(INIT, ERROR, "custom infname len illegal > %d\n",
		       CUSTOM_IFNAMESIZ);
		ret = -EINVAL;
		goto INIT_WLAN_RETURN;
	}

#endif /*  CFG_DRIVER_INF_NAME_CHANGE */

	wlanDebugInit();

	/* memory pre-allocation */
#if CFG_PRE_ALLOCATION_IO_BUFFER
	kalInitIOBuffer(TRUE);
#else
	kalInitIOBuffer(FALSE);
#endif

	wlanRegisterNetdevNotifier();

#if WLAN_INCLUDE_PROC
	procInitFs();
#endif
#if WLAN_INCLUDE_SYS
	sysInitFs();
#endif

	wlanCreateWirelessDevice();
	if (gprWdev[0] == NULL) {
		ret = -ENOMEM;
		goto INIT_WLAN_RETURN;
	}

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
#if CFG_ENABLE_WIFI_DIRECT
	if (gprWdev[0])
		glP2pCreateWirelessDevice(prGlueInfo);
#endif
	gPrDev = NULL;

#if CFG_DC_USB_WOW_CALLBACK
	/* register system DC wow enable/disable callback function */
	do {
		typedef void (*func_ptr) (int (*f) (void));
		char *func_name = "WifiRegisterPdwncCallback";
		func_ptr pFunc =
			(func_ptr)GLUE_SYMBOL_GET(func_name);
		char *func_name2 = "RegisterPdwncCallbackWifi";
		func_ptr pFunc2 =
			(func_ptr)GLUE_SYMBOL_GET(func_name2);

		if (pFunc) {
			DBGLOG(INIT, ERROR,
			"Register Wow callback1 success\n");
				pFunc(&kalDcSetWow);
			GLUE_SYMBOL_PUT(func_name);
		} else if (pFunc2) {
			DBGLOG(INIT, ERROR,
			"Register Wow callback2 success\n");
			pFunc2(&kalDcSetWow);
			GLUE_SYMBOL_PUT(func_name2);
		} else
			DBGLOG(INIT, ERROR,
			"No Exported Func Found:%s\n",
			func_name);
	} while (0);
#endif

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
	fw_log_wifi_inf_init();
#if WLAN_INCLUDE_SYS
	sysFwLogInit();
#endif
#endif

#if CFG_MTK_MDDP_SUPPORT
	mddpInit(bootmode);
#endif

	kalPlatOpsInit();

#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
	ret = ((glRegisterShutdownCB(wlanShutdown)
		== WLAN_STATUS_SUCCESS) ? 0 : -EIO);
	if (ret == -EIO)
		goto INIT_WLAN_RETURN;
#endif

	ret = ((glRegisterBus(wlanProbe,
			      wlanRemove) == WLAN_STATUS_SUCCESS) ? 0 : -EIO);
	if (ret == -EIO) {
		kalUninitIOBuffer();
		goto INIT_WLAN_RETURN;
	}

#if (!CFG_MTK_ANDROID_WMT)
	ret = glBusFuncOn();
	if (ret)
		DBGLOG(INIT, ERROR, "glBusFuncOn failed.\n");
#endif /* CFG_MTK_ANDROID_WMT */

#if (CFG_CHIP_RESET_SUPPORT)
	glResetInit(prGlueInfo);
#endif
	kalFbNotifierReg(prGlueInfo);

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
	kalBatNotifierReg(prGlueInfo);
#endif

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
	wifi_fwlog_event_func_register(consys_log_event_notification);
#endif

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	ret = IcsInit();
	if (ret < 0) {
		DBGLOG(INIT, INFO, "ics log node init failed!");
		goto INIT_WLAN_RETURN;
	} else {
		wifi_ics_event_func_register(ics_log_event_notification);
	}
#endif /* CFG_SUPPORT_ICS */
#if (CFG_SUPPORT_SA_LOG == 1)
	ret = SalogInit();
	if (ret < 0) {
		DBGLOG(INIT, INFO, "sa log node init failed!");
		return ret;
	}
#endif /* CFG_SUPPORT_SA_LOG */
#if (CFG_SUPPORT_FW_IDX_LOG_SAVE == 1)
	FwLogDevInit();
#endif
#if WLAN_INCLUDE_SYS
	sysInitWifiVer();
#endif
	g_u4WlanInitFlag = 1;

#if CFG_POWER_OFF_CTRL_SUPPORT
	wlanRegisterRebootNotifier();
#endif
#if CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
	kalIdcRegisterRilNotifier();
#endif
#if CFG_AP_80211KVR_INTERFACE
	nl_sk = netlink_kernel_create(&init_net, NETLINK_OSS_KERNEL, NULL);
	if (!nl_sk) {
		DBGLOG(INIT, ERROR, "netlink create failed!\n");
		ret = -EBUSY;
		goto INIT_WLAN_RETURN;
	}
#endif /* CFG_AP_80211KVR_INTERFACE */

	glGetChipInfo((void **)&chip);

	if (!chip)
		DBGLOG(HAL, ERROR, "NULL chip info init Wlan.\n");
	else
		wlan_pinctrl_action(chip, WLAN_PINCTRL_MSG_FUNC_PTA_UART_INIT);

	DBGLOG(INIT, INFO, "initWlan::End\n");

INIT_WLAN_RETURN:
#if CFG_CHIP_RESET_SUPPORT
#if CFG_CHIP_RESET_KO_SUPPORT
	if (ret)
		resetko_unregister_module(RESET_MODULE_TYPE_WIFI);
#endif  /* CFG_CHIP_RESET_KO_SUPPORT */
#endif  /* CFG_CHIP_RESET_SUPPORT */

	return ret;
}				/* end of initWlan() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Driver exit point when the driver as a Linux Module is removed. Called
 *        at module unload time, by the user level modutils application: rmmod.
 *        This is our last chance to clean up after ourselves.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
/* 1 Module Leave Point */
static void exitWlan(void)
{
#if defined(_HIF_USB) || CFG_SUPPORT_PERSIST_NETDEV
	struct GLUE_INFO *prGlueInfo = NULL;
#endif
#if CFG_SUPPORT_PERSIST_NETDEV
	uint32_t u4Idx = 0;
	struct wiphy *wiphy = NULL;
#endif

	DBGLOG(INIT, INFO, "exitWlan::Start\n");

#if CFG_AP_80211KVR_INTERFACE
	if (nl_sk != NULL)
		netlink_kernel_release(nl_sk);
#endif /* CFG_AP_80211KVR_INTERFACE */

	kalFbNotifierUnReg();

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
	kalBatNotifierUnReg();
#endif

#if CFG_CHIP_RESET_SUPPORT
#if CFG_CHIP_RESET_KO_SUPPORT
	resetko_unregister_module(RESET_MODULE_TYPE_WIFI);
#endif
	glResetUninit();
#endif

#if defined(_HIF_USB)
	/* for USB remove ko case, Power off Wifi CMD need to be DONE
	 * before unregister bus, or connsys cannot enter deep sleep
	 * after rmmod
	 */
	prGlueInfo = wlanGetGlueInfo();
	if (prGlueInfo != NULL && prGlueInfo->prAdapter != NULL)
		wlanPowerOffWifi(prGlueInfo->prAdapter);
#endif

#if CFG_MTK_ANDROID_WMT
	unregister_plat_connsys_cbs();
	g_IsPlatCbsRegistered = FALSE;
#else
	glBusFuncOff();
#endif /* CFG_MTK_ANDROID_WMT */

	glUnregisterBus(wlanRemove);

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
	fw_log_wifi_inf_deinit();
#if WLAN_INCLUDE_SYS
	sysFwLogUninit();
#endif
#endif

#if CFG_SUPPORT_PERSIST_NETDEV

	wiphy = wlanGetWiphy();
	WIPHY_PRIV(wiphy, prGlueInfo);


	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		if (gprWdev[u4Idx] && gprWdev[u4Idx]->netdev &&
		    gprWdev[u4Idx]->netdev->reg_state == NETREG_REGISTERED) {
			wlanClearDevIdx(gprWdev[u4Idx]->netdev);
			DBGLOG(INIT, INFO, "Unregister wlan%d netdev start.\n",
					u4Idx);
			unregister_netdev(gprWdev[u4Idx]->netdev);
			DBGLOG(INIT, INFO, "Unregister wlan%d netdev end.\n",
					u4Idx);
		}
	}

	prGlueInfo->fgIsRegistered = FALSE;

	DBGLOG(INIT, INFO, "Free wlan device..\n");
	wlanFreeNetDev();
#endif

#if CFG_MTK_MDDP_SUPPORT
	mddpUninit();
#endif

#if CFG_DC_USB_WOW_CALLBACK
	/*Unregister system DC wow enable/disable callback function */
	do {
		typedef void (*func_ptr) (int (*f) (void));
		char *func_name = "WifiRegisterPdwncCallback";
		func_ptr pFunc =
			(func_ptr)GLUE_SYMBOL_GET(func_name);
		char *func_name2 = "RegisterPdwncCallbackWifi";
		func_ptr pFunc2 =
			(func_ptr)GLUE_SYMBOL_GET(func_name2);

		if (pFunc) {
			DBGLOG(INIT, STATE,
			"Unregister Wow callback1 success\n");
			pFunc(NULL);
			GLUE_SYMBOL_PUT(func_name);
		} else if (pFunc2) {
			DBGLOG(INIT, STATE,
			"Unregister Wow callback2 success\n");
			pFunc2(NULL);
			GLUE_SYMBOL_PUT(func_name2);
		} else
			DBGLOG(INIT, ERROR,
			"No Exported Func Found:%s\n",
			func_name);
		} while (0);
#endif

	wlanUnregisterNetdevNotifier();

	/* free pre-allocated memory */
	kalUninitIOBuffer();

	/* For single wiphy case, it's hardly to free wdev & wiphy in 2 func.
	 * So that, use wlanDestroyAllWdev to replace wlanDestroyWirelessDevice
	 * and glP2pDestroyWirelessDevice.
	 */
	wlanDestroyAllWdev();

#if WLAN_INCLUDE_SYS
	sysUninitSysFs();
#endif

#if WLAN_INCLUDE_PROC
	procUninitProcFs();
#endif

#if (CFG_SUPPORT_SA_LOG == 1)
	SalogDeInit();
#endif /* CFG_SUPPORT_SA_LOG */

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	IcsDeInit();
#endif /* CFG_SUPPORT_ICS */
#if (CFG_SUPPORT_FW_IDX_LOG_SAVE == 1)
	FwLogDevUninit();
#endif
	g_u4WlanInitFlag = 0;

#if CFG_POWER_OFF_CTRL_SUPPORT
	wlanUnregisterRebootNotifier();
#endif
#if CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
	kalIdcUnregisterRilNotifier();
#endif
	DBGLOG(INIT, INFO, "exitWlan::End\n");
}				/* end of exitWlan() */

#if CFG_POWER_OFF_CTRL_SUPPORT
static int wf_pdwnc_notify(struct notifier_block *nb,
		unsigned long event, void *unused)
{
#if defined(_HIF_USB) || CFG_SUPPORT_PERSIST_NETDEV
	struct GLUE_INFO *prGlueInfo = NULL;
#endif
#if CFG_SUPPORT_PERSIST_NETDEV
	uint32_t u4Idx = 0;
	struct wiphy *wiphy = NULL;
#endif

	if (event == SYS_RESTART) {
		DBGLOG(HAL, STATE, "wf_pdwnc_notify()\n");

		wlanUnregisterNetdevNotifier();
		kalFbNotifierUnReg();

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
		kalBatNotifierUnReg();
#endif

#if CFG_CHIP_RESET_SUPPORT
		glResetUninit();
#endif

#if defined(_HIF_USB)
		/* for USB remove ko case, Power off Wifi CMD need to be DONE
		* before unregister bus, or connsys cannot enter deep sleep
		* after rmmod
		*/
		prGlueInfo = wlanGetGlueInfo();
		if (prGlueInfo != NULL)
			wlanPowerOffWifi(prGlueInfo->prAdapter);
#endif

#if (CFG_MTK_ANDROID_WMT == 0)
		glBusFuncOff();
#endif
		glUnregisterBus(wlanRemove);
#if CFG_SUPPORT_PERSIST_NETDEV
		wiphy = wlanGetWiphy();
		WIPHY_PRIV(wiphy, prGlueInfo);

		for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
			if (gprWdev[u4Idx] && gprWdev[u4Idx]->netdev) {
				wlanClearDevIdx(gprWdev[u4Idx]->netdev);
				DBGLOG(INIT, INFO,
					"Unregister wlan%d netdev start.\n",
					u4Idx);
				unregister_netdev(gprWdev[u4Idx]->netdev);
				DBGLOG(INIT, INFO,
					"Unregister wlan%d netdev end.\n",
					u4Idx);
			}
		}

		prGlueInfo->fgIsRegistered = FALSE;

		DBGLOG(INIT, INFO, "Free wlan device..\n");
		wlanFreeNetDev();
#endif

#if CFG_MTK_MDDP_SUPPORT
		mddpUninit();
#endif
		/* free pre-allocated memory */
		kalUninitIOBuffer();

		/* For single wiphy case, it's hardly to
		* free wdev & wiphy in 2 func.
		* So that, use wlanDestroyAllWdev
		* to replace wlanDestroyWirelessDevice
		* and glP2pDestroyWirelessDevice.
		*/
		wlanDestroyAllWdev();

#if WLAN_INCLUDE_SYS
		sysUninitSysFs();
#endif

#if WLAN_INCLUDE_PROC
		procUninitProcFs();
#endif

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
		IcsDeInit();
#endif /* CFG_SUPPORT_ICS */

		g_u4WlanInitFlag = 0;

		DBGLOG(HAL, STATE, "wf_pdwnc_notify() done\n");
	}

	if (event == SYS_POWER_OFF || event == SYS_HALT) {
		DBGLOG(HAL, STATE, "DC Set WoW\n");
		kalDcSetWow();
	}
	return 0;
}

static struct notifier_block wf_pdwnc_notifier = {
	.notifier_call = wf_pdwnc_notify,
	.next = NULL,
	.priority = 0,
};

void wlanRegisterRebootNotifier(void)
{
	DBGLOG(HAL, STATE, "wlanRegisterRebootNotifier()\n");
	register_reboot_notifier(&wf_pdwnc_notifier);
	DBGLOG(HAL, STATE, "wlanRegisterRebootNotifier() done\n");
}

void wlanUnregisterRebootNotifier(void)
{
	DBGLOG(HAL, STATE, "wlanUnregisterRebootNotifier()\n");
	unregister_reboot_notifier(&wf_pdwnc_notifier);
	DBGLOG(HAL, STATE, "wlanUnregisterRebootNotifier() done\n");
}

#endif

struct wiphy *wlanGetWiphy(void)
{
	if (gprWdev[0])
		return gprWdev[0]->wiphy;

	return NULL;
}

struct net_device *wlanGetNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex)
{
	struct net_device *prNetDevice = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
#if CFG_ENABLE_WIFI_DIRECT
	struct GL_P2P_INFO *prP2pInfo = (struct GL_P2P_INFO *) NULL;

	GLUE_SPIN_LOCK_DECLARATION();
#endif

	if (!prGlueInfo)
		return NULL;

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return NULL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return NULL;

	/* AIS */
	if (IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex)) {
		struct AIS_FSM_INFO *ais =
			aisGetAisFsmInfo(prGlueInfo->prAdapter, ucBssIndex);

		if (ais && gprWdev[ais->ucAisIndex])
			return gprWdev[ais->ucAisIndex]->netdev;
	}
	else if (IS_BSS_P2P(prBssInfo)) { /* P2P */
#if CFG_ENABLE_WIFI_DIRECT
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prAdapter->rP2PNetRegState ==
			ENUM_NET_REG_STATE_REGISTERED) {
			prP2pInfo =
				prGlueInfo->prP2PInfo[prBssInfo->u4PrivateData];

			if (prP2pInfo) {
				if ((prP2pInfo->aprRoleHandler != NULL) &&
					(prP2pInfo->aprRoleHandler !=
						prP2pInfo->prDevHandler))
					prNetDevice = prP2pInfo->aprRoleHandler;
				else
					prNetDevice = prP2pInfo->prDevHandler;
			}
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
#endif
	} else if (IS_BSS_NAN(prBssInfo) && ucBssIndex < MAX_BSSID_NUM) {
#if CFG_SUPPORT_NAN
		prNetDevice = wlanGetNetInterfaceByBssIdx(prGlueInfo,
							  ucBssIndex);
#endif
	}

	if (prNetDevice == NULL)
		DBGLOG(REQ, LOUD, "bssidx=%d has NULL netdev caller=%pS\n",
			ucBssIndex, KAL_TRACE);

	return prNetDevice;
}

struct net_device *wlanGetAisNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucAisIndex)
{
	if (gprWdev[ucAisIndex] && gprWdev[ucAisIndex]->netdev)
		return gprWdev[ucAisIndex]->netdev;

	return NULL;
}


struct net_device *wlanGetP2pNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucP2pIndex)
{
#if CFG_ENABLE_WIFI_DIRECT
	if (gprP2pRoleWdev[ucP2pIndex] &&
		gprP2pRoleWdev[ucP2pIndex]->netdev)
		return gprP2pRoleWdev[ucP2pIndex]->netdev;
#endif
	return NULL;
}


uint8_t wlanGetBssIdx(struct net_device *ndev)
{
	if (ndev) {
		struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate
			= (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(ndev);

		DBGLOG(REQ, LOUD,
			"ucBssIndex = %d, ndev(%p)\n",
			prNetDevPrivate->ucBssIdx,
			ndev);

		return prNetDevPrivate->ucBssIdx;
	}

	DBGLOG(REQ, LOUD,
		"ucBssIndex = 0xff, ndev(%p)\n",
		ndev);

	return 0xff;
}

u_int8_t wlanIsAisDev(struct net_device *prDev)
{
	uint32_t u4Idx = 0;

	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++)
		if (gprWdev[u4Idx] && prDev == gprWdev[u4Idx]->netdev)
			return TRUE;

	return FALSE;
}

void
wlanNotifyFwSuspend(struct GLUE_INFO *prGlueInfo,
		    struct net_device *prDev, u_int8_t fgSuspend)
{
	uint32_t rStatus;
	uint32_t u4SetInfoLen;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) NULL;
	struct CMD_SUSPEND_MODE_SETTING rSuspendCmd;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			  netdev_priv(prDev);

	if (prNetDevPrivate->prGlueInfo != prGlueInfo)
		DBGLOG(REQ, WARN, "%s: unexpected prGlueInfo(0x%p)!\n",
			   __func__, prNetDevPrivate->prGlueInfo);

	rSuspendCmd.ucBssIndex = prNetDevPrivate->ucBssIdx;
	rSuspendCmd.ucEnableSuspendMode = fgSuspend;

#if CFG_WOW_SUPPORT
	if (prGlueInfo->prAdapter->rWifiVar.ucWow
		&& prGlueInfo->prAdapter->rWowCtrl.fgWowEnable) {
		/* cfg enable + wow enable => Wow On mdtim*/
		rSuspendCmd.ucMdtim =
			prGlueInfo->prAdapter->rWifiVar.ucWowOnMdtim;
		rSuspendCmd.ucWowSuspend = TRUE;
		DBGLOG(REQ, TRACE, "mdtim [1]\n");
	} else if (prGlueInfo->prAdapter->rWifiVar.ucWow
		   && !prGlueInfo->prAdapter->rWowCtrl.fgWowEnable) {
		if (prGlueInfo->prAdapter->rWifiVar.ucAdvPws) {
			/* cfg enable + wow disable + adv pws enable
			 * => Wow Off mdtim
			 */
			rSuspendCmd.ucMdtim =
				prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim;
			rSuspendCmd.ucWowSuspend = TRUE;
			DBGLOG(REQ, TRACE, "mdtim [2]\n");
		}
	} else if (!prGlueInfo->prAdapter->rWifiVar.ucWow) {
		if (prGlueInfo->prAdapter->rWifiVar.ucAdvPws) {
			/* cfg disable + adv pws enable => MT6632 case
			 * => Wow Off mdtim
			 */
			rSuspendCmd.ucMdtim =
				prGlueInfo->prAdapter->rWifiVar.ucWowOffMdtim;
			rSuspendCmd.ucWowSuspend = FALSE;
			DBGLOG(REQ, TRACE, "mdtim [3]\n");
		}
	} else
#endif
	{
		rSuspendCmd.ucMdtim = 1;
		rSuspendCmd.ucWowSuspend = FALSE;
	}

	/* When FW receive command, it check connection state to decide apply
	 * setting or not
	 */

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidNotifyFwSuspend,
			   (void *)&rSuspendCmd,
			   sizeof(rSuspendCmd),
			   &u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, INFO, "wlanNotifyFwSuspend fail\n");
}

#if ((MTK_WCN_HIF_SDIO == 1) && (CFG_BUILT_IN_DRIVER == 1)) || \
	((MTK_WCN_HIF_AXI == 1) && (CFG_BUILT_IN_DRIVER == 1))

int mtk_wcn_wlan_gen4_init(void)
{
	return initWlan();
}
EXPORT_SYMBOL(mtk_wcn_wlan_gen4_init);

void mtk_wcn_wlan_gen4_exit(void)
{
	return exitWlan();
}
EXPORT_SYMBOL(mtk_wcn_wlan_gen4_exit);

#elif ((MTK_WCN_HIF_SDIO == 0) && (CFG_BUILT_IN_DRIVER == 1))

device_initcall(initWlan);

#else

module_init(initWlan);
module_exit(exitWlan);

#endif
