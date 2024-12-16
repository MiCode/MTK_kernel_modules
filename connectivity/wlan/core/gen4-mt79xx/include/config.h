/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/include/config.h#3
 */

/*! \file   "config.h"
 *   \brief  This file includes the various configurable parameters for
 *           customers
 *
 *    This file ncludes the configurable parameters except the parameters
 *    indicate the turning-on/off of some features
 */

#ifndef _CONFIG_H
#define _CONFIG_H

#include <linux/version.h>

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* 2 Flags for OS capability */
#undef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
#if defined(_HIF_SDIO)
/* #ifdef CONFIG_X86 */
/*Kernel-3.10-ARM did not provide X86_FLAG & HIF shouldn't bind platform*/
#if (CFG_MTK_ANDROID_WMT)
#define MTK_WCN_HIF_SDIO		1
#else
#define MTK_WCN_HIF_SDIO		0
#endif
#else
#define MTK_WCN_HIF_SDIO		0
#endif

#if defined(_HIF_AXI)
#ifdef LINUX
#ifdef CONFIG_X86
#define MTK_WCN_HIF_AXI			0
#else
#define MTK_WCN_HIF_AXI			1
#endif
#else
#define MTK_WCN_HIF_AXI			0
#endif
#else
#define MTK_WCN_HIF_AXI			0
#endif


#ifndef CONFIG_WIFI_RAM_MQM_BA_DELAY_SUPPORT
#define CONFIG_WIFI_RAM_MQM_BA_DELAY_SUPPORT             0
#endif

/* Android build-in driver switch, Mike 2016/11/11*/
#ifndef CFG_BUILT_IN_DRIVER
#define CFG_BUILT_IN_DRIVER         0
#endif

/* Mike 2016/09/01 ALPS update K3.18 80211_disconnect to K4.4 version*/
/* work around for any alps K3.18 platform*/
#ifndef CFG_WPS_DISCONNECT
#define CFG_WPS_DISCONNECT         0
#endif


#if (CFG_SUPPORT_AEE == 1)
/* TODO: temp remove for 7663 on mobile */
/* #define CFG_ENABLE_AEE_MSG          1 */
#define CFG_ENABLE_AEE_MSG          0
#else
#define CFG_ENABLE_AEE_MSG          0
#endif

#define CFG_ENABLE_EARLY_SUSPEND        0
#define CFG_ENABLE_NET_DEV_NOTIFY		1

/* 2 Flags for Driver Features */
#define CFG_TX_FRAGMENT                 1 /*!< 1: Enable TX fragmentation */
/* 0: Disable */
#define CFG_SUPPORT_PERFORMANCE_TEST    0 /*Only for performance Test */

#define CFG_COUNTRY_CODE                NULL /* "US" */
#define CFG_SUPPORT_DEBUG_FS		0
#define CFG_SUPPORT_SET_CAM_BY_PROC	1 /*Enable/disable powersave mode*/

#ifndef LINUX
#define CFG_FW_FILENAME                 L"WIFI_RAM_CODE"
#define CFG_CR4_FW_FILENAME             L"WIFI_RAM_CODE2"
#else
#define CFG_FW_FILENAME                 "WIFI_RAM_CODE"
#define CFG_CR4_FW_FILENAME             "WIFI_RAM_CODE2"
#endif

#ifndef CFG_MET_PACKET_TRACE_SUPPORT
#define CFG_MET_PACKET_TRACE_SUPPORT    0 /*move to wlan/MAKEFILE */
#endif

#ifndef CFG_MET_TAG_SUPPORT
#define CFG_MET_TAG_SUPPORT             0
#endif

#ifndef CFG_SUPPORT_PERF_IND
#define CFG_SUPPORT_PERF_IND            1
#endif

/* Support AP Selection */
#define CFG_SUPPORT_RSN_SCORE		0
#define CFG_SELECT_BSS_BASE_ON_MULTI_PARAM	1
#define CFG_MAX_NUM_OF_CHNL_INFO		50
#define CFG_SUPPORT_CHNL_CONFLICT_REVISE	0


/*------------------------------------------------------------------------------
 * Driver config
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_CFG_FILE
#ifndef LINUX
#define CFG_SUPPORT_CFG_FILE	0
#else
#define CFG_SUPPORT_CFG_FILE	1
#endif
#endif

/* Radio Reasource Measurement (802.11k) */
#define CFG_SUPPORT_RRM		0

/* DFS (802.11h) */
#define CFG_SUPPORT_DFS		1
#ifndef CFG_SUPPORT_DFS_MASTER
#define CFG_SUPPORT_DFS_MASTER		1
#endif
/* SoftAp Cross Band Channel Switch */
#ifndef CFG_SUPPORT_IDC_CH_SWITCH
#define CFG_SUPPORT_IDC_CH_SWITCH	1
#endif

#if (CFG_SUPPORT_DFS == 1)	/* Add by Enlai */
/* If CSA new channel is DFS channel, link down directly*/
#define CFG_DFS_NEWCH_DFS_FORCE_DISCONNECT	1
/* Quiet (802.11h) */
#define CFG_SUPPORT_QUIET	0
/* Spectrum Management (802.11h): TPC and DFS */
#define CFG_SUPPORT_SPEC_MGMT	1
#else
/* Quiet (802.11h) */
#define CFG_SUPPORT_QUIET	0
/* Spectrum Management (802.11h): TPC and DFS */
#define CFG_SUPPORT_SPEC_MGMT	0
#endif

/* 11n feature. RX RDG capability */
#define CFG_SUPPORT_RX_RDG	0

/* 802.11n MCS Feedback responder */
#define CFG_SUPPORT_MFB		0

/* 802.11n RX STBC (1SS) */
#define CFG_SUPPORT_RX_STBC	1

/* 802.11n RX short GI for both 20M and 40M BW */
#define CFG_SUPPORT_RX_SGI	1

/* 802.11n RX HT green-field capability */
#define CFG_SUPPORT_RX_HT_GF	1

#ifndef CFG_SUPPORT_WAPI
#define CFG_SUPPORT_WAPI	1
#endif

/* Enable QA Tool Support */
#define CFG_SUPPORT_QA_TOOL	1

/* Enable TX BF Support */
#define CFG_SUPPORT_TX_BF	1

#define CFG_SUPPORT_TX_BF_FPGA	1

#if CFG_SUPPORT_TX_BF
#define CFG_SUPPORT_BFER	1
#define CFG_SUPPORT_BFEE	1
#else
#define CFG_SUPPORT_BFER	0
#define CFG_SUPPORT_BFEE	0
#endif

/* Enable MU MIMO Support */
#define CFG_SUPPORT_MU_MIMO	1

/* Enable WOW Support */
#define CFG_WOW_SUPPORT		1

#ifndef CFG_SUPPORT_DTIM_SKIP
#define CFG_SUPPORT_DTIM_SKIP	0
#endif

/* Disable magic packet vendor event */
#ifndef CFG_SUPPORT_MAGIC_PKT_VENDOR_EVENT
#define CFG_SUPPORT_MAGIC_PKT_VENDOR_EVENT  0
#endif

/* Disable WOW EINT mode */
#ifndef CFG_SUPPORT_WOW_EINT
#define CFG_SUPPORT_WOW_EINT	0
#endif

#if (CFG_SUPPORT_CONNAC3X == 1)
/* Enable Detection for 2021 Frag/AGG Attack from WFA */
#define CFG_SUPPORT_FRAG_AGG_ATTACK_DETECTION 1
#endif

/* when wow wakeup host, send keyevent to screen on */
#ifndef CFG_SUPPORT_WOW_EINT_KEYEVENT_WAKEUP
#define CFG_SUPPORT_WOW_EINT_KEYEVENT_WAKEUP	0
#endif

/* Enable Mdns offload */
#ifndef CFG_SUPPORT_MDNS_OFFLOAD
#define CFG_SUPPORT_MDNS_OFFLOAD	0
#endif

#if CFG_SUPPORT_MDNS_OFFLOAD
#ifndef CFG_SUPPORT_MDNS_OFFLOAD_GVA
#define CFG_SUPPORT_MDNS_OFFLOAD_GVA 0
#endif

#if CFG_SUPPORT_MDNS_OFFLOAD_GVA
#define CFG_SUPPORT_MDNS_OFFLOAD_TV 0
#else
#define CFG_SUPPORT_MDNS_OFFLOAD_TV 1
#endif

#define TEST_CODE_FOR_MDNS			0
#endif

/* Enable A-MSDU RX Reordering Support */
#define CFG_SUPPORT_RX_AMSDU	1

/* Enable Fragment Support */
#define CFG_SUPPORT_FRAG_SUPPORT 1

/* Enable frag and de-AMSDU security patch */
#define CFG_FRAG_DEAMSDU_ATTACK_DETECTION 1

#if (CFG_FRAG_DEAMSDU_ATTACK_DETECTION)

/* Enable A-MSDU Attack Detection */
#define CFG_SUPPORT_AMSDU_ATTACK_DETECTION 1

/* Enable Fragment Attack Detection */
#define CFG_SUPPORT_FRAG_ATTACK_DETECTION 1

/* Enable Fake EAPOL Detection */
#define CFG_SUPPORT_FAKE_EAPOL_DETECTION 1

/* Enable TKIP MIC ERROR Detection */
#define CFG_SUPPORT_TKIP_MICERROR_DETECTION 1

#else

/* Enable A-MSDU Attack Detection */
#define CFG_SUPPORT_AMSDU_ATTACK_DETECTION 0

/* Enable Fragment Attack Detection */
#define CFG_SUPPORT_FRAG_ATTACK_DETECTION 0

/* Enable Fake EAPOL Detection */
#define CFG_SUPPORT_FAKE_EAPOL_DETECTION 0

/* Enable TKIP MIC ERROR Detection */
#define CFG_SUPPORT_TKIP_MICERROR_DETECTION 0

#endif

/* Enable Android wake_lock operations */
#if defined(CONFIG_ANDROID)
#ifndef CFG_ENABLE_WAKE_LOCK
#define CFG_ENABLE_WAKE_LOCK	1
#endif
#else
#define CFG_ENABLE_WAKE_LOCK	0
#endif

#define CFG_SUPPORT_OSHARE	1

#ifndef CFG_SUPPORT_LOWLATENCY_MODE
#define CFG_SUPPORT_LOWLATENCY_MODE	0
#endif

#ifndef CFG_SUPPORT_LINK_LAYER_STAT
#define CFG_SUPPORT_LINK_LAYER_STAT	0
#endif

#define CFG_SUPPORT_ANT_SWAP		0

#if KERNEL_VERSION(4, 4, 0) <= LINUX_VERSION_CODE
#define CFG_SUPPORT_DATA_STALL			1
#else
#define CFG_SUPPORT_DATA_STALL			0
#endif

#ifndef CFG_SUPPORT_CSI
#define CFG_SUPPORT_CSI 1
#endif

#if CFG_SUPPORT_CSI
#define CFG_SUPPORT_CSI_NF 0
#define CFG_CSI_DEBUG 0
#endif

#if CFG_SUPPORT_CSI_NF
#define CFG_SUPPORT_CSI_BFADJ 0
#define CFG_SUPPORT_CSI_BWDF 0
#endif

/*------------------------------------------------------------------------------
 * Support Extend Range feature
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_HE_ER
#define CFG_SUPPORT_HE_ER			0
#endif

#define CFG_SUPPORT_ICS				1
#define CFG_SUPPORT_PHY_ICS			1

/*------------------------------------------------------------------------------
 * Flags of Buffer mode SUPPORT
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_BUFFER_MODE                 1

/*------------------------------------------------------------------------------
 * Support coex wifi/BT non co-Tx
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_COEX_NON_COTX
#define CFG_SUPPORT_COEX_NON_COTX	0
#endif

/*------------------------------------------------------------------------------
 * SLT Option
 *------------------------------------------------------------------------------
 */
#define CFG_SLT_SUPPORT				0

#ifdef NDIS60_MINIPORT
#define CFG_NATIVE_802_11                       1

#define CFG_TX_MAX_PKT_SIZE                     2304

/* !< 1: Enable TCP/IP header checksum offload */
#define CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60       0

/* 0: Disable */
#define CFG_TCP_IP_CHKSUM_OFFLOAD               0
#define CFG_WHQL_DOT11_STATISTICS               1
#define CFG_WHQL_ADD_REMOVE_KEY                 1
#define CFG_WHQL_CUSTOM_IE                      1
#define CFG_WHQL_SAFE_MODE_ENABLED              1

#else
#define CFG_TCP_IP_CHKSUM_OFFLOAD               1
#define CFG_TCP_IP_CHKSUM_OFFLOAD_NDIS_60       0
#define CFG_TX_MAX_PKT_SIZE                     1600
#define CFG_NATIVE_802_11                       0
#endif

/* By using GRO at NAPI level, the driver is doing the aggregation to a large
 * SKB very early, right at the receive completion handler. This means that all
 * the next functions in the receive stack do much less processing.
 * The GRO feature could enhance "Rx" tput.
 */
#define CFG_SUPPORT_RX_GRO                      1

/* 0 : direct-GRO mode (without NAPI poll-callback) (Default)
 * 1 : NAPI+GRO mode
 *
 * Note:
 * Due to lower CPU cost, main_thread may enter sleep status more frequently
 * and get lower RX T.P. in NAPI+GRO mode. This is an arch-dependency issue.
 */
#ifndef CFG_SUPPORT_RX_NAPI
#define CFG_SUPPORT_RX_NAPI                     0
#endif
#if (CFG_SUPPORT_RX_GRO == 0) && (CFG_SUPPORT_RX_NAPI == 1)
#error "NAPI should based on GRO in gen4m"
#endif

/* There is a "budget" concept in original NAPI design. However,
 * the default budget in Linux is 64 and it's hard to aggreate a 64K packet
 * within 64-packets in throughput test.
 * For example, if there are 8 traffic streams in test, the average
 * packets/stream would be 8 in the 64-packets-budget. We would get
 * worse performance than idea condition.
 * 0 : Default policy with budget control
 * 1 : Skip budget to aggregate as more as possible
 */
#define CFG_SUPPORT_RX_GRO_PEAK            1


/* Support Linux TCP segmentation offload(TSO)
 * With feature enabled, TCP stack may send a big TCP pkt to driver (more than
 * MTU, 64k is max) and driver MUST splite this pkt into multiple MTU-sized pkts
 */
#ifndef CFG_SUPPORT_TX_TSO_SW
#define CFG_SUPPORT_TX_TSO_SW                     0
#endif

/* Support Allocate TX packet buffer(streaming DMA) for TX ring buffer
 * With feature enabled, wifi driver will copy packet from protocal
 *  stack alloc mem(prskb)  to buffer that mentioned above.
 * With disabled, wifi driver not alloc TX packet buffer(streaming DMA)
 * for TX ring buffer, direct use protocal stack mem, it will reduce mem
 * copy ops in wifi driver. sometimes it will help for peak TP, but must
 * confirm that IOMMU for PCIE is supported on platform.
 */
#ifndef CFG_HIF_TX_PREALLOC_DATA_BUFFER
#define CFG_HIF_TX_PREALLOC_DATA_BUFFER			1
#endif

/* Support Linux netdevice Scatter/gather IO (NETIF_F_SG)
 * With feature enabled, L3 netdevice will push gathered skbs to wifi driver
 * and the driver should put fragmented memories to DMA properly.
 */
#ifndef CFG_SUPPORT_TX_SG
#define CFG_SUPPORT_TX_SG                         0
#endif

/* 2 Flags for Driver Parameters */
/*------------------------------------------------------------------------------
 * Flags for EHPI Interface in Colibri Platform
 *------------------------------------------------------------------------------
 */
/*!< 1: Do workaround for faster bus timing */
/* 0(default): Disable */
#define CFG_EHPI_FASTER_BUS_TIMING                  0

/*------------------------------------------------------------------------------
 * Flags for UMAC
 *------------------------------------------------------------------------------
 */
#define CFG_UMAC_GENERATION                         0x20

/*------------------------------------------------------------------------------
 * Flags for HIFSYS Interface
 *------------------------------------------------------------------------------
 */
#ifdef _lint
/* #define _HIF_SDIO   1 */
#endif

/* 1(default): Enable SDIO ISR & TX/RX status enhance mode
 * 0: Disable
 */
#define CFG_SDIO_INTR_ENHANCE                        1
/* 1(default): Enable SDIO ISR & TX/RX status enhance mode
 * 0: Disable
 */
#define CFG_SDIO_RX_ENHANCE                          1
/* 1: Enable SDIO TX enhance mode(Multiple frames in single BLOCK CMD)
 * 0(default): Disable
 */
#define CFG_SDIO_TX_AGG                              1

/* 1: Enable SDIO RX enhance mode(Multiple frames in single BLOCK CMD)
 * 0(default): Disable
 */
#define CFG_SDIO_RX_AGG                              1

/* 1: Enable SDIO RX Workqueue De-Aggregation
 * 0(default): Disable
 */
#ifndef CFG_SDIO_RX_AGG_WORKQUE
#define CFG_SDIO_RX_AGG_WORKQUE                      0
#endif

#if (CFG_SDIO_RX_AGG == 1) && (CFG_SDIO_INTR_ENHANCE == 0)
#error \
	"CFG_SDIO_INTR_ENHANCE should be 1 once CFG_SDIO_RX_AGG equals to 1"
#elif (CFG_SDIO_INTR_ENHANCE == 1 || CFG_SDIO_RX_ENHANCE == 1) && \
	(CFG_SDIO_RX_AGG == 0)
#error \
	"CFG_SDIO_RX_AGG should be 1 once CFG_SDIO_INTR_ENHANCE and/or CFG_SDIO_RX_ENHANCE equals to 1"
#endif

#ifdef WINDOWS_CE
/*!< 1: Support pass through (PATHRU) mode */
#define CFG_SDIO_PATHRU_MODE                    1
/* 0: Disable */
#else
/*!< 0: Always disable if WINDOWS_CE is not defined */
#define CFG_SDIO_PATHRU_MODE                    0
#endif

#define CFG_SDIO_ACCESS_N9_REGISTER_BY_MAILBOX      0
#define CFG_MAX_RX_ENHANCE_LOOP_COUNT               3

#ifndef CFG_SDIO_INTR_ENHANCE_FORMAT
#define CFG_SDIO_INTR_ENHANCE_FORMAT                1
#endif

#define CFG_USB_TX_AGG                              1
#define CFG_USB_CONSISTENT_DMA                      0
#define CFG_USB_TX_HANDLE_IN_HIF_THREAD             0
#define CFG_USB_RX_HANDLE_IN_HIF_THREAD             0

#ifndef CFG_TX_DIRECT
#define CFG_TX_DIRECT                               1
#endif
#ifndef CFG_RX_DIRECT
#define CFG_RX_DIRECT                               1
#endif

#define CFG_HW_WMM_BY_BSS                           1

#ifndef CFG_SUPPORT_PCIE_ASPM_IMPROVE
#define CFG_SUPPORT_PCIE_ASPM_IMPROVE               0
#endif

/*------------------------------------------------------------------------------
 * Flags and Parameters for Integration
 *------------------------------------------------------------------------------
 */

#define CFG_MULTI_ECOVER_SUPPORT	1

#define CFG_ENABLE_CAL_LOG		1
#define CFG_REPORT_RFBB_VERSION		1

#define MAX_BSSID_NUM			4	/* MAX BSSID number */

#define CFG_CHIP_RESET_SUPPORT          1

#ifndef CFG_CHIP_RESET_USB_DISCONNECT
#define CFG_CHIP_RESET_USB_DISCONNECT   0
#endif

#ifndef CFG_CHIP_RESET_USE_DTS_GPIO_NUM
#define CFG_CHIP_RESET_USE_DTS_GPIO_NUM 0
#endif

#ifndef CFG_CHIP_RESET_USE_LINUX_GPIO_API
#define CFG_CHIP_RESET_USE_LINUX_GPIO_API 0
#endif

#ifndef CFG_CHIP_RESET_USE_MSTAR_GPIO_API
#define CFG_CHIP_RESET_USE_MSTAR_GPIO_API 0
#endif

#if CFG_CHIP_RESET_SUPPORT
#define CFG_SER_L05_DEBUG		0
#define CFG_CHIP_RESET_HANG		0
#else
#define CFG_CHIP_RESET_HANG		0
#endif

#define HW_BSSID_NUM			4	/* HW BSSID number by chip */

/*------------------------------------------------------------------------------
 * Flags for workaround
 *------------------------------------------------------------------------------
 */

/*------------------------------------------------------------------------------
 * Flags for driver version
 *------------------------------------------------------------------------------
 */
#define CFG_DRV_OWN_VERSION	((uint16_t)((NIC_DRIVER_MAJOR_VERSION << 8) | \
				(NIC_DRIVER_MINOR_VERSION)))
#define CFG_DRV_PEER_VERSION	0x0000U

/*------------------------------------------------------------------------------
 * Flags and Parameters for TX path
 *------------------------------------------------------------------------------
 */

/*! Maximum number of SW TX packet queue */
#ifndef CFG_TX_MAX_PKT_NUM
#define CFG_TX_MAX_PKT_NUM                      1024
#endif

/*! Maximum number of SW TX CMD packet buffer */
#define CFG_TX_MAX_CMD_PKT_NUM                  32

#define CFG_TX_DYN_CMD_SUPPORT                  0

/* QM_CMD_RESERVED_THRESHOLD should less than the cmd tx resource */
#ifndef QM_CMD_RESERVED_THRESHOLD
#define QM_CMD_RESERVED_THRESHOLD               4
#endif

/*------------------------------------------------------------------------------
 * Flags and Parameters for RX path
 *------------------------------------------------------------------------------
 */

/*------------------------------------------------------------------------------
 * CONFIG_TITLE : Move BA from FW to Driver
 * OWNER            : Puff Wen
 * Description  : Move BA from FW to Driver
 *------------------------------------------------------------------------------
 */
#define CFG_M0VE_BA_TO_DRIVER                   0

/*! Max. descriptor number - sync. with firmware */
#if CFG_SLT_SUPPORT
#define CFG_NUM_OF_RX0_HIF_DESC                 42
#else
#define CFG_NUM_OF_RX0_HIF_DESC                 16
#endif
#define CFG_NUM_OF_RX1_HIF_DESC                 2

/*! Max. buffer hold by QM */
#define CFG_NUM_OF_QM_RX_PKT_NUM                HIF_NUM_OF_QM_RX_PKT_NUM

/*! Maximum number of SW RX packet buffer */
#define CFG_RX_MAX_PKT_NUM                      ((CFG_NUM_OF_RX0_HIF_DESC + \
						CFG_NUM_OF_RX1_HIF_DESC) * 3 \
						+ CFG_NUM_OF_QM_RX_PKT_NUM)

#define CFG_RX_REORDER_Q_THRESHOLD              8

#ifndef LINUX
#define CFG_RX_RETAINED_PKT_THRESHOLD           (CFG_NUM_OF_RX0_HIF_DESC + \
						CFG_NUM_OF_RX1_HIF_DESC \
						+ CFG_NUM_OF_QM_RX_PKT_NUM)
#else
#define CFG_RX_RETAINED_PKT_THRESHOLD           0
#endif

/*! Maximum RX packet size, if exceed this value, drop incoming packet */
/* 7.2.3 Maganement frames */
/* TODO: it should be 4096 under emulation mode */
#define CFG_RX_MAX_PKT_SIZE	(28 + 2312 + 12 /*HIF_RX_HEADER_T*/)

#if (CFG_SUPPORT_SNIFFER_RADIOTAP == 1)
#define CFG_MONITOR_BAND_NUM	3
#define CFG_RADIOTAP_HEADROOM	72
#define CFG_RX_MAX_MPDU_SIZE	13312
#else
#define CFG_RX_MAX_MPDU_SIZE	CFG_RX_MAX_PKT_SIZE
#endif

/*! Minimum RX packet size, if lower than this value, drop incoming packet */
#define CFG_RX_MIN_PKT_SIZE	10 /*!< 802.11 Control Frame is 10 bytes */

/*! RX BA capability */
#define CFG_NUM_OF_RX_BA_AGREEMENTS             80
#if CFG_M0VE_BA_TO_DRIVER
#define CFG_RX_BA_MAX_WINSIZE                   64
#endif
#define CFG_RX_BA_INC_SIZE                      64
#define CFG_RX_MAX_BA_TID_NUM                   8
#define CFG_RX_REORDERING_ENABLED               1

#define CFG_PF_ARP_NS_MAX_NUM                   3

/* After MT7922, the CFG_RX_REPORT_FORMAT should be 2 */
#ifndef CFG_RX_REPORT_FORMAT
#define CFG_RX_REPORT_FORMAT                    1
#endif

#define CFG_COMPRESSION_DEBUG			0
#define CFG_DECOMPRESSION_TMP_ADDRESS		0
#define CFG_SUPPORT_COMPRESSION_FW_OPTION	0

/*------------------------------------------------------------------------------
 * Flags and Parameters for CMD/RESPONSE
 *------------------------------------------------------------------------------
 */
#define CFG_RESPONSE_POLLING_TIMEOUT            1000
#define CFG_RESPONSE_POLLING_DELAY              5
#define CFG_CMD_ALLOC_FAIL_TIMEOUT_MS           (6000)

/*------------------------------------------------------------------------------
 * Flags and Parameters for Protocol Stack
 *------------------------------------------------------------------------------
 */
/*! Maximum number of BSS in the SCAN list */
#define CFG_MAX_NUM_BSS_LIST                    192

#define CFG_MAX_COMMON_IE_BUF_LEN         ((1500 * CFG_MAX_NUM_BSS_LIST) / 3)

/*! Maximum size of Header buffer of each SCAN record */
#define CFG_RAW_BUFFER_SIZE                      1024

/*! Maximum size of IE buffer of each SCAN record */
#define CFG_IE_BUFFER_SIZE                      512

/*------------------------------------------------------------------------------
 * Flags and Parameters for Power management
 *------------------------------------------------------------------------------
 */
#define CFG_ENABLE_FULL_PM                      1
#define CFG_ENABLE_WAKEUP_ON_LAN                0

/* debug which packet wake up host */
#define CFG_SUPPORT_WAKEUP_REASON_DEBUG         1

#define CFG_INIT_POWER_SAVE_PROF		ENUM_PSP_FAST_SWITCH

#define CFG_INIT_ENABLE_PATTERN_FILTER_ARP	0

/* (BIT(3) | BIT(2) | BIT(1) | BIT(0)) */
#define CFG_INIT_UAPSD_AC_BMP			0

/* #define CFG_SUPPORT_WAPI			0 */
#define CFG_SUPPORT_WPS				1
#define CFG_SUPPORT_WPS2			1

/*------------------------------------------------------------------------------
 * Flags 1: drop all multicast packets when device suspend
 * Flags 0: drop multicast packets except white list when device suspend
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_DROP_ALL_MC_PACKET		0

/*------------------------------------------------------------------------------
 * Auto Channel Selection maximun channel number
 *------------------------------------------------------------------------------
 */
/* ARRAY_SIZE(mtk_5ghz_channels) + ARRAY_SIZE(mtk_2ghz_channels) */
#if (CFG_SUPPORT_WIFI_6G == 1)
#define MAX_CHN_NUM			(MAX_2G_BAND_CHN_NUM + \
				MAX_5G_BAND_CHN_NUM + MAX_6G_BAND_CHN_NUM)
#define MAX_2G_BAND_CHN_NUM		14
#define MAX_5G_BAND_CHN_NUM		25
#define MAX_6G_BAND_CHN_NUM		59
#define MAX_PER_BAND_CHN_NUM		25
#else
#define MAX_CHN_NUM			39
#define MAX_2G_BAND_CHN_NUM		14
#define MAX_5G_BAND_CHN_NUM		(MAX_CHN_NUM - MAX_2G_BAND_CHN_NUM)
#define MAX_6G_BAND_CHN_NUM		0
#define MAX_PER_BAND_CHN_NUM		(MAX_CHN_NUM - MAX_2G_BAND_CHN_NUM)
#endif

#define ACS_PRINT_BUFFER_LEN		200

/*------------------------------------------------------------------------------
 * Flags and Parameters for Ad-Hoc
 *------------------------------------------------------------------------------
 */
#define CFG_INIT_ADHOC_FREQ                     (2462000)
#define CFG_INIT_ADHOC_MODE                     AD_HOC_MODE_MIXED_11BG
#define CFG_INIT_ADHOC_BEACON_INTERVAL          (100)
#define CFG_INIT_ADHOC_ATIM_WINDOW              (0)

/*------------------------------------------------------------------------------
 * Maximum scan SSID number and channel number
 * Should be aligned with FW scan command
 *------------------------------------------------------------------------------
 */
#define SCAN_CMD_SSID_NUM                       (4)
#define SCAN_CMD_CHNL_NUM                       (32)

#if 1
/* to be compatible with old FW, we set ssid num to 0 here,
 * we should set correct num when query of scan capability from FW is done
 */
#define SCAN_CMD_EXT_SSID_NUM                   (0)
#define SCAN_CMD_EXT_CHNL_NUM                   (32)
#else
#define SCAN_CMD_EXT_SSID_NUM                   (6)
#define SCAN_CMD_EXT_CHNL_NUM                   (32)
#endif
#define CFG_SCAN_SSID_MAX_NUM (SCAN_CMD_SSID_NUM+SCAN_CMD_EXT_SSID_NUM)

#if (CFG_SUPPORT_WIFI_6G == 1)
/* Align max channel number with FW */
#define MAXIMUM_OPERATION_CHANNEL_LIST (46+64)
#else
#define MAXIMUM_OPERATION_CHANNEL_LIST (SCAN_CMD_CHNL_NUM+SCAN_CMD_EXT_CHNL_NUM)
#endif

/*------------------------------------------------------------------------------
 * Flags and Parameters for Load Setup Default
 *------------------------------------------------------------------------------
 */

/*------------------------------------------------------------------------------
 * Flags for enable 802.11A Band setting
 *------------------------------------------------------------------------------
 */

/*------------------------------------------------------------------------------
 * Flags and Parameters for Interrupt Process
 *------------------------------------------------------------------------------
 */
#define CFG_IST_LOOP_COUNT                      HIF_IST_LOOP_COUNT

#define CFG_INT_WRITE_CLEAR                     0

/* 2 Flags for Driver Debug Options */
/*------------------------------------------------------------------------------
 * Flags of TX Debug Option. NOTE(Kevin): Confirm with SA before modifying
 * following flags.
 *------------------------------------------------------------------------------
 */
/*!< 1: Debug statistics usage of MGMT Buffer */
/* 0: Disable */
#define CFG_DBG_MGT_BUF                         1

#define CFG_HIF_STATISTICS                      0

#define CFG_HIF_RX_STARVATION_WARNING           0

#define CFG_RX_PKTS_DUMP                        0

#define CFG_SUPPORT_STATISTICS			1

#define CFG_ASSERT_DUMP                         1

#define CFG_SUPPORT_TRACE_TC4			0
/*------------------------------------------------------------------------------
 * Flags of Firmware Download Option.
 *------------------------------------------------------------------------------
 */
#define CFG_ENABLE_FW_DOWNLOAD                  1

#define CFG_ENABLE_FW_DOWNLOAD_ACK              1

#ifndef CFG_WIFI_IP_SET
#define CFG_WIFI_IP_SET                         1
#endif

/*------------------------------------------------------------------------------
 * Flags of Bluetooth-over-WiFi (BT 3.0 + HS) support
 *------------------------------------------------------------------------------
 */

#define CFG_ENABLE_BT_OVER_WIFI             0

#define CFG_BOW_SEPARATE_DATA_PATH              1

#define CFG_BOW_PHYSICAL_LINK_NUM               4

#define CFG_BOW_LIMIT_AIS_CHNL                  1

#define CFG_BOW_SUPPORT_11N                     1

#define CFG_BOW_RATE_LIMITATION                 1

/*------------------------------------------------------------------------------
 * Flags of Wi-Fi Direct support
 *------------------------------------------------------------------------------
 */
/*------------------------------------------------------------------------------
 * Support reporting all BSS networks to cfg80211 kernel when scan
 * request is from P2P interface
 * Originally only P2P networks will be reported when scan request is from p2p0
 *------------------------------------------------------------------------------
 */
#ifndef CFG_P2P_SCAN_REPORT_ALL_BSS
#define CFG_P2P_SCAN_REPORT_ALL_BSS            0
#endif

/* Allow connection with no P2P IE device */
#ifndef CFG_P2P_CONNECT_ALL_BSS
#define CFG_P2P_CONNECT_ALL_BSS            0
#endif

/* Allow setting max P2P GO client count */
#ifndef CFG_P2P_DEFAULT_CLIENT_COUNT
#define CFG_P2P_DEFAULT_CLIENT_COUNT 0
#endif

/*------------------------------------------------------------------------------
 * Flags for GTK rekey offload
 *------------------------------------------------------------------------------
 */

#ifdef LINUX
#ifdef CONFIG_X86
#define CFG_ENABLE_WIFI_DIRECT          1
#define CFG_SUPPORT_802_11W             1
#define CONFIG_SUPPORT_GTK_REKEY        1
#else
#define CFG_ENABLE_WIFI_DIRECT          1

/*!< 0(default): Disable 802.11W */
#define CFG_SUPPORT_802_11W             1

#define CONFIG_SUPPORT_GTK_REKEY        1
#endif
#else /* !LINUX */
#define CFG_ENABLE_WIFI_DIRECT           0
#define CFG_SUPPORT_802_11W              0	/* Not support at WinXP */
#endif /* LINUX */

#define CFG_SUPPORT_PERSISTENT_GROUP            0

#define CFG_TEST_WIFI_DIRECT_GO                 0

#define CFG_TEST_ANDROID_DIRECT_GO              0

#define CFG_UNITEST_P2P                         0

/*
 * Enable cfg80211 option after Android 2.2(Froyo) is suggested,
 * cfg80211 on linux 2.6.29 is not mature yet
 */
#define CFG_ENABLE_WIFI_DIRECT_CFG_80211        1

#define CFG_SUPPORT_HOTSPOT_OPTIMIZATION        0
#define CFG_HOTSPOT_OPTIMIZATION_BEACON_INTERVAL 300
#define CFG_HOTSPOT_OPTIMIZATION_DTIM           1
#define CFG_AUTO_CHANNEL_SEL_SUPPORT            1

#define CFG_SUPPORT_SOFTAP_WPA3	1

#ifndef CFG_ENABLE_UNIFY_WIPHY
#define CFG_ENABLE_UNIFY_WIPHY 1
#endif

#define CFG_ENABLE_OFFCHANNEL_TX 1

/*------------------------------------------------------------------------------
 * Configuration Flags (Linux Only)
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_EXT_CONFIG                  0

/*------------------------------------------------------------------------------
 * Statistics Buffering Mechanism
 *------------------------------------------------------------------------------
 */
#if CFG_SUPPORT_PERFORMANCE_TEST
#define CFG_ENABLE_STATISTICS_BUFFERING         1
#else
#define CFG_ENABLE_STATISTICS_BUFFERING         0
#endif
#define CFG_STATISTICS_VALID_CYCLE              2000
#define CFG_LINK_QUALITY_VALID_PERIOD           500

/*------------------------------------------------------------------------------
 * Migration Option
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_ADHOC                       1
#define CFG_SUPPORT_AAA                         1

#define CFG_SUPPORT_BCM                         0
#define CFG_SUPPORT_BCM_BWCS                    0
#define CFG_SUPPORT_BCM_BWCS_DEBUG              0

#define CFG_SUPPORT_RDD_TEST_MODE               0

#define CFG_SUPPORT_PWR_MGT                     1

#define CFG_ENABLE_HOTSPOT_PRIVACY_CHECK        1

#define CFG_MGMT_FRAME_HANDLING                 1

#define CFG_MGMT_HW_ACCESS_REPLACEMENT          0


#if CFG_SUPPORT_PERFORMANCE_TEST

#else

#endif

#define CFG_SUPPORT_AIS_5GHZ                    1
#define CFG_SUPPORT_BEACON_CHANGE_DETECTION     0

/*------------------------------------------------------------------------------
 * Option for NVRAM and Version Checking
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_NVRAM                       1
#define CFG_NVRAM_EXISTENCE_CHECK               1
#define CFG_SW_NVRAM_VERSION_CHECK              1
#define CFG_SUPPORT_NIC_CAPABILITY              1
#define CFG_SUPPORT_NVRAM_FRAGMENT              0

/*------------------------------------------------------------------------------
 * CONFIG_TITLE : Stress Test Option
 * OWNER        : Puff Wen
 * Description  : For stress test only. DO NOT enable it while normal operation
 *------------------------------------------------------------------------------
 */
#define CFG_STRESS_TEST_SUPPORT                 0

/*------------------------------------------------------------------------------
 * Flags for LINT
 *------------------------------------------------------------------------------
 */
#define LINT_SAVE_AND_DISABLE	/*lint -save -e* */

#define LINT_RESTORE		/*lint -restore */

#define LINT_EXT_HEADER_BEGIN		LINT_SAVE_AND_DISABLE

#define LINT_EXT_HEADER_END		LINT_RESTORE

/*------------------------------------------------------------------------------
 * Flags of Features
 *------------------------------------------------------------------------------
 */

#ifndef CFG_SUPPORT_TDLS
#define CFG_SUPPORT_TDLS		1
#endif

/* Enable/disable QoS TX, AMPDU */
#define CFG_SUPPORT_QOS			1

#define CFG_SUPPORT_AMPDU_TX		1
#define CFG_SUPPORT_AMPDU_RX		1

/* Enable/disable TS-related Action frames handling */
#define CFG_SUPPORT_TSPEC		0
#define CFG_SUPPORT_UAPSD		1
#define CFG_SUPPORT_UL_PSMP		0

/* Roaming System */
#ifndef CFG_SUPPORT_ROAMING
#define CFG_SUPPORT_ROAMING		1
#endif

#if (CFG_SUPPORT_ROAMING == 1)

/* Roaming feature: skip roaming when only one ESSID AP
 * Need Android background scan
 * if no roaming event occurred
 * to trigger roaming scan
 * after skip roaming in one ESSID AP case
 */
#define CFG_SUPPORT_ROAMING_SKIP_ONE_AP		0
#define CFG_SUPPORT_DRIVER_ROAMING		1
#else
#define CFG_SUPPORT_ROAMING_SKIP_ONE_AP		0
#define CFG_SUPPORT_DRIVER_ROAMING		0

#endif /* CFG_SUPPORT_ROAMING */

#define CFG_SUPPORT_SWCR			1

#define CFG_SUPPORT_ANTI_PIRACY			1

#define CFG_SUPPORT_OSC_SETTING			1

#define CFG_SUPPORT_P2P_RSSI_QUERY		0

#define CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP	0

#define CFG_SHOW_MACADDR_SOURCE			1

#if BUILD_QA_DBG
#define CFG_SHOW_FULL_MACADDR     1
#define CFG_SHOW_FULL_IPADDR      1
#define CFG_SHOW_SSID             1
#else
#define CFG_SHOW_FULL_MACADDR     0
#define CFG_SHOW_FULL_IPADDR      0
#define CFG_SHOW_SSID             0
#endif

#ifndef CFG_SUPPORT_VO_ENTERPRISE
#define CFG_SUPPORT_VO_ENTERPRISE               1
#endif
#define CFG_SUPPORT_WMM_AC                      1
#if CFG_SUPPORT_VO_ENTERPRISE
#define CFG_SUPPORT_802_11R                     1
#define CFG_SUPPORT_802_11K                     1
#else
#define CFG_SUPPORT_802_11R                     0
#define CFG_SUPPORT_802_11K                     0
#endif
#define CFG_SUPPORT_MBO                         1

/*!< 1(default): Enable 802.11d */
/* 0: Disable */
#ifndef CFG_SUPPORT_802_11D
#define CFG_SUPPORT_802_11D		1
#endif
#if (CFG_SUPPORT_802_11K == 1)
#undef CFG_SUPPORT_802_11D
#define CFG_SUPPORT_802_11D                     1
#endif

#ifndef CFG_SUPPORT_DPP
#define CFG_SUPPORT_DPP                         0
#endif
/*------------------------------------------------------------------------------
 * Support supplicant SME/MLME.
 * Let upper layer to control auth/assoc,
 * driver process T/RX Auth/Assoc w/o SAA FSM
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_SUPPLICANT_SME
#define CFG_SUPPORT_SUPPLICANT_SME              0
#endif

#if (CFG_SUPPORT_802_11K == 1) && (CFG_SUPPORT_SUPPLICANT_SME == 1)
/* Enable to do beacon reports by supplicant.
 * Beacon report is a sub-feature of 802_11K(RRM)
 * Supplicant only support RRM when SME supported.
 */
#if (CFG_SUPPORT_WIFI_6G == 1)
#define CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT 1
#else
#define CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT 0
#endif
#else
#define CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT 0
#endif

/* Support 802.11v Wireless Network Management */
#ifndef CFG_SUPPORT_802_11V
#define CFG_SUPPORT_802_11V                     1
#endif
#if CFG_SUPPORT_802_11V
#define CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT  1
#else
#define CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT  0
#endif
#define CFG_SUPPORT_802_11V_TIMING_MEASUREMENT	0

#if (CFG_SUPPORT_802_11V_TIMING_MEASUREMENT == 1) && \
	(CFG_SUPPORT_802_11V == 0)
#error \
"CFG_SUPPORT_802_11V should be 1 once CFG_SUPPORT_802_11V_TIMING_MEASUREMENT equals to 1"
#endif

#define WNM_UNIT_TEST CFG_SUPPORT_802_11V

#define CFG_SUPPORT_802_11V_MBSSID		0
#if (CFG_SUPPORT_802_11AX == 1)
/*11v MBSSID is mandatory for 11ax*/
#undef CFG_SUPPORT_802_11V_MBSSID
#define CFG_SUPPORT_802_11V_MBSSID		1
#endif

#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
#define CFG_SUPPORT_SUPPLICANT_MBO		1
#else
#define CFG_SUPPORT_SUPPLICANT_MBO		0
#endif

#define CFG_SUPPORT_PPR2			1
#define CFG_DRIVER_COMPOSE_ASSOC_REQ		1
#define CFG_SUPPORT_802_11AC			1
#define CFG_STRICT_CHECK_CAPINFO_PRIVACY	0

#define CFG_SUPPORT_WFD				1
#define CFG_SUPPORT_WFD_COMPOSE_IE		1

/* Support Customer vendor IE */
#define CFG_SUPPORT_CUSTOM_VENDOR_IE		1

#define CFG_SUPPORT_HOTSPOT_WPS_MANAGER		1
#define CFG_SUPPORT_NFC_BEAM_PLUS		1

/* Refer to CONFIG_MTK_STAGE_SCAN */
#define CFG_MTK_STAGE_SCAN			1

/* Enable driver support multicore */
#ifndef CFG_SUPPORT_MULTITHREAD
#define CFG_SUPPORT_MULTITHREAD		1
#endif

/* Transfer Oid from main_thread to workqueue */
#ifndef CFG_REDIRECT_OID_SUPPORT
#if (CFG_SUPPORT_MULTITHREAD == 1) && (CFG_SUPPORT_SUPPLICANT_SME == 1)
#define CFG_REDIRECT_OID_SUPPORT	1
#else
#define CFG_REDIRECT_OID_SUPPORT	0
#endif
#endif

/* add cfg80211 tx/rx api to work queue */
#ifndef CFG_SUPPORT_CFG80211_QUEUE
#define CFG_SUPPORT_CFG80211_QUEUE 	0
#endif

#define CFG_SUPPORT_MTK_SYNERGY			1

#define CFG_SUPPORT_VHT_IE_IN_2G		1

#define CFG_SUPPORT_PWR_LIMIT_COUNTRY		1

#ifndef CFG_SUPPORT_DYNAMIC_PWR_LIMIT
#define CFG_SUPPORT_DYNAMIC_PWR_LIMIT		0
#endif
#ifndef CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
#define CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG	0
#endif

#define CFG_FIX_2_TX_PORT			0

#define CFG_CHANGE_CRITICAL_PACKET_PRIORITY	0

/*------------------------------------------------------------------------------
 * Flags of bus error tolerance
 *------------------------------------------------------------------------------
 */
#define CFG_FORCE_RESET_UNDER_BUS_ERROR     0

/*------------------------------------------------------------------------------
 * Build Date Code Integration
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_BUILD_DATE_CODE         0

/*------------------------------------------------------------------------------
 * Flags of SDIO test pattern support
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_SDIO_READ_WRITE_PATTERN 0

/*------------------------------------------------------------------------------
 * Flags of Workaround
 *------------------------------------------------------------------------------
 */
#define CFG_ENABLE_READ_EXTRA_4_BYTES       1

/* Handle IOT issue for 11ac certification */
#define CFG_OPMODE_CONFLICT_OPINFO	1

/*------------------------------------------------------------------------------
 * Flags of Packet Lifetime Profiling Mechanism
 *------------------------------------------------------------------------------
 */
#define CFG_ENABLE_PKT_LIFETIME_PROFILE     1
#define CFG_PRINT_PKT_LIFETIME_PROFILE      0

#define CFG_ENABLE_PER_STA_STATISTICS       1

#define CFG_ENABLE_PER_STA_STATISTICS_LOG 1

/*------------------------------------------------------------------------------
 * Flags for prepare the FW compile flag
 *------------------------------------------------------------------------------
 */
#define COMPILE_FLAG0_GET_STA_LINK_STATUS     (1<<0)
#define COMPILE_FLAG0_WFD_ENHANCEMENT_PROTECT (1<<1)

/*------------------------------------------------------------------------------
 * Flags of Batch Scan SUPPORT
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_BATCH_SCAN             (0)
#define CFG_BATCH_MAX_MSCAN                (2)

/*------------------------------------------------------------------------------
 * Flags of SCHEDULE SCAN SUPPORT
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_SCHED_SCAN
#define CFG_SUPPORT_SCHED_SCAN             (0)
#endif
#define SCHED_SCAN_CMD_VERSION             (1)

/* this value should be aligned to auSsid in struct CMD_SCHED_SCAN_REQ */
#define CFG_SCAN_HIDDEN_SSID_MAX_NUM       (10)
/* this value should be aligned to auMatchSsid in struct CMD_SCHED_SCAN_REQ */
#define CFG_SCAN_SSID_MATCH_MAX_NUM        (16)

/*------------------------------------------------------------------------------
 * Full2Partial Scan SUPPORT
 *------------------------------------------------------------------------------
 */
/* During a full2partial scan period, all online full scan requests would be
 * changed to partial scan. The unit of this value is second
 */
#define CFG_SUPPORT_FULL2PARTIAL_SCAN      (0)
#define CFG_SCAN_FULL2PARTIAL_PERIOD       (60)

/*------------------------------------------------------------------------------
 * Value of scan cache result
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_SCAN_CACHE_RESULT      (0)
#define CFG_SCAN_CACHE_RESULT_PERIOD       (7000)	/* Unit: ms */
#define CFG_SCAN_CACHE_MIN_CHANNEL_NUM     (10)

/*------------------------------------------------------------------------------
 * Default value: the duration in ms to check TRX
 *                      while the beacon timeout event comes.
 * This is the default value for
 *                      prAdapter->rWifiVar.u4BeaconTimoutFilterDurationMs
 * can customize
 *        1. by project's requirement in this default value
 *        2. or by define in wifi.cfg directly (BeaconTimoutFilterDurationMs)
 * if the value set to 0, it means disable the filter.
 * if the value set to 2000, it means the duration of fitler is 2000 ms
 *------------------------------------------------------------------------------
 */
#define CFG_BEACON_TIMEOUT_FILTER_DURATION_DEFAULT_VALUE	2000

/*------------------------------------------------------------------------------
 * Flags of Random MAC support
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_SCAN_RANDOM_MAC        (1)

/*------------------------------------------------------------------------------
 * Flags of Sniffer SUPPORT
 *------------------------------------------------------------------------------
 */
#define WLAN_INCLUDE_PROC                   1

#if CFG_TC10_FEATURE
#define WLAN_INCLUDE_SYS                   1
#else
#define WLAN_INCLUDE_SYS                   0
#endif

/*------------------------------------------------------------------------------
 * Flags of Sniffer SUPPORT
 *------------------------------------------------------------------------------
 */
#define CFG_DUAL_P2PLIKE_INTERFACE         1

#define RUNNING_P2P_MODE 0
#define RUNNING_AP_MODE 1
#define RUNNING_DUAL_AP_MODE 2
#define RUNNING_P2P_AP_MODE 3
#define RUNNING_P2P_MODE_NUM 4

/*------------------------------------------------------------------------------
 * Flags of MSP SUPPORT
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_MSP				1



/*------------------------------------------------------------------------------
 * Flags of driver fw customization
 *------------------------------------------------------------------------------
 */

#define CFG_SUPPORT_EASY_DEBUG               1


/*------------------------------------------------------------------------------
 * Flags of driver delay calibration atfer efuse buffer mode CMD
 *------------------------------------------------------------------------------
 */

#define CFG_EFUSE_BUFFER_MODE_DELAY_CAL         1

/*------------------------------------------------------------------------------
 * Flags of Drop Packet Replay SUPPORT
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_REPLAY_DETECTION		1

/*------------------------------------------------------------------------------
 * Flags of driver EEPROM pages for QA tool
 *------------------------------------------------------------------------------
 */

#define CFG_EEPROM_PAGE_ACCESS         1

/*------------------------------------------------------------------------------
 * Flags for HOST_OFFLOAD
 *------------------------------------------------------------------------------
 */

#define CFG_SUPPORT_WIFI_HOST_OFFLOAD	1

/*------------------------------------------------------------------------------
 * Flags for DBDC Feature
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_DBDC
#define CFG_SUPPORT_DBDC	1
#endif
#define CFG_SUPPORT_DBDC_NO_BLOCKING_OPMODE 1
#define CFG_SUPPORT_SAP_DFS_CHANNEL 1

#if (CFG_SUPPORT_DBDC == 1)
#ifndef CFG_DBDC_SW_FOR_P2P_LISTEN
#define CFG_DBDC_SW_FOR_P2P_LISTEN	0
#endif
#else
#undef CFG_DBDC_SW_FOR_P2P_LISTEN
#define CFG_DBDC_SW_FOR_P2P_LISTEN	0
#endif /* CFG_SUPPORT_DBDC */
/*------------------------------------------------------------------------------
 * Flags for Using TC4 Resource in ROM code stage
 *------------------------------------------------------------------------------
 */

#define CFG_USE_TC4_RESOURCE_FOR_INIT_CMD	0

/*------------------------------------------------------------------------------
 * Flags for Efuse Start and End address report from FW
 *------------------------------------------------------------------------------
 */

#define CFG_FW_Report_Efuse_Address	1

/*------------------------------------------------------------------------------
 * FW name max length
 *------------------------------------------------------------------------------
 */
#define CFG_FW_NAME_MAX_LEN	(64)

/*------------------------------------------------------------------------------
 * Support WMT WIFI Path Config
 *------------------------------------------------------------------------------
 */
#define CFG_WMT_WIFI_PATH_SUPPORT	0

/*------------------------------------------------------------------------------
 * Support CFG_SISO_SW_DEVELOP
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SISO_SW_DEVELOP
#define CFG_SISO_SW_DEVELOP			0
#endif

/*------------------------------------------------------------------------------
 * Support spatial extension control
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_SPE_IDX_CONTROL
#define CFG_SUPPORT_SPE_IDX_CONTROL		1
#endif

/*------------------------------------------------------------------------------
 * Flags for a Goal for MT6632 : Cal Result Backup in Host or NVRam when Android
 *                               Boot
 *------------------------------------------------------------------------------
 */
#if 0 /*(MTK_WCN_HIF_SDIO) : 20161003 Default Off, later will enable
       *                     by MTK_WCN_HIF_SDIO
       */
#define CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST				1
#define CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST_DBGLOG		0
#else
#define CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST				0
#define CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST_DBGLOG		0
#endif

/*------------------------------------------------------------------------------
 * Enable SDIO 1-bit Data Mode. (Usually debug only)
 *------------------------------------------------------------------------------
 */
#define CFG_SDIO_1BIT_DATA_MODE			0

/*------------------------------------------------------------------------------
 * Single Sku
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_SINGLE_SKU
#define CFG_SUPPORT_SINGLE_SKU	1
#endif

#ifndef CFG_SUPPORT_SINGLE_SKU_LOCAL_DB
#define CFG_SUPPORT_SINGLE_SKU_LOCAL_DB 1
#endif

#ifndef CFG_SUPPORT_BW160
#define CFG_SUPPORT_BW160 0
#endif

/*------------------------------------------------------------------------------
 * Direct Control for RF/PHY/BB/MAC for Manual Configuration via command/api
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_ADVANCE_CONTROL 1


/* Set 1 for 7915 version and 2 for 7668/7663 version */
#ifndef CFG_SUPPORT_TRAFFIC_REPORT
#define CFG_SUPPORT_TRAFFIC_REPORT 0
#endif

#ifndef CFG_ADM_CTRL
#define CFG_ADM_CTRL 0
#endif

/*------------------------------------------------------------------------------
 * One Time Calibration Command/API
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_ONE_TIME_CAL
#define CFG_SUPPORT_ONE_TIME_CAL 0
#endif

#if (CFG_SUPPORT_ONE_TIME_CAL == 1)
#define ONE_TIME_CAL_DATA_FILE_DIR "/tmp/"
#endif

/*------------------------------------------------------------------------------
 * Driver pre-allocate total size of memory in one time
 *------------------------------------------------------------------------------
 */
#ifndef CFG_PRE_ALLOCATION_IO_BUFFER
#define CFG_PRE_ALLOCATION_IO_BUFFER 0
#endif

/*------------------------------------------------------------------------------
 * Auto enable SDIO asynchronous interrupt mode
 *------------------------------------------------------------------------------
 */
#define CFG_SDIO_ASYNC_IRQ_AUTO_ENABLE	1

/*------------------------------------------------------------------------------
 * Flags to force enable performance monitor even when screen is OFF
 *------------------------------------------------------------------------------
 */
#ifndef CFG_FORCE_ENABLE_PERF_MONITOR
#define CFG_FORCE_ENABLE_PERF_MONITOR	0
#endif

/*------------------------------------------------------------------------------
 * Flags to ignore invalid auth tsn issue (ex. ALPS03089071)
 *------------------------------------------------------------------------------
 */
#define CFG_IGNORE_INVALID_AUTH_TSN	1

/*------------------------------------------------------------------------------
 * Flags of Network Controlled HandOver(NCHO) support
 * TC10 only: To improve the voice quality during handover,
 *		the NCHO is required to precisely control scanning parameters
 * CFG_SUPPORT_NCHO: 1: support, 0: not support
 * CFG_SUPPORT_NCHO_AUTO_ENABLE: sub-feature depends with CFG_SUPPORT_NCHO
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_NCHO		0
#define CFG_SUPPORT_NCHO_AUTO_ENABLE	0

/*------------------------------------------------------------------------------
 * Support OWE (Opportunistic Wireless Encryption)
 *------------------------------------------------------------------------------
  */
#ifndef CFG_SUPPORT_OWE
#define CFG_SUPPORT_OWE			0
#endif

#define CFG_SUPPORT_WPA3_H2E		1

/*------------------------------------------------------------------------------
 * Flags of Key Word Exception Mechanism
 *------------------------------------------------------------------------------
 */
#define CFG_ENABLE_KEYWORD_EXCEPTION_MECHANISM  0

/*------------------------------------------------------------------------------
 * Driver supports preferred frequency list for p2p operating channel
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_P2P_PREFERRED_FREQ_LIST  1

/*------------------------------------------------------------------------------
 * Flag used for P2P GO CSA
 * Value 0: Disable
 * Value 1: Enable
 * Note: Must Enable CFG_SUPPORT_DFS,
 * CFG_SUPPORT_DFS_MASTER,
 * CFG_SUPPORT_IDC_CH_SWITCH in advance
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_P2P_CSA
#if	CFG_SUPPORT_IDC_CH_SWITCH && \
	CFG_SUPPORT_DFS && \
	CFG_SUPPORT_DFS_MASTER
#define CFG_SUPPORT_P2P_CSA	0
#endif
#endif

/*------------------------------------------------------------------------------
 * Flags of AUTO SCC mode Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_AUTO_SCC
#if CFG_SUPPORT_IDC_CH_SWITCH && \
	CFG_SUPPORT_DFS_MASTER && \
	CFG_SUPPORT_DFS && \
	CFG_SUPPORT_P2P_CSA
#define CFG_SUPPORT_AUTO_SCC 1
#else
#define CFG_SUPPORT_AUTO_SCC 0
#endif
#endif

/*------------------------------------------------------------------------------
 * Flags of Channel switch to the channel selected by ACS
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_P2P_CSA_ACS
#if CFG_SUPPORT_IDC_CH_SWITCH && \
	CFG_SUPPORT_DFS_MASTER && \
	CFG_SUPPORT_DFS && \
	CFG_SUPPORT_P2P_CSA
#define CFG_SUPPORT_P2P_CSA_ACS 1
#else
#define CFG_SUPPORT_P2P_CSA_ACS 0
#endif
#endif


/*------------------------------------------------------------------------------
 * Flag used for P2P GO to find the best channel list
 * Value 0: Disable
 * Value 1: Enable
 * Note: Must Enable CFG_SUPPORT_P2P_PREFERRED_FREQ_LIST in advance
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_P2PGO_ACS
#define CFG_SUPPORT_P2PGO_ACS 0
#endif

/*-----------------------------------------------------------------------------
* Flags to support IOT AP blacklist
*------------------------------------------------------------------------------
*/
#ifndef CFG_SUPPORT_IOT_AP_BLACKLIST
#if CFG_SUPPORT_DBDC && CFG_SUPPORT_CFG_FILE
#define CFG_SUPPORT_IOT_AP_BLACKLIST 1
#else
#define CFG_SUPPORT_IOT_AP_BLACKLIST 0
#endif
#endif

#if CFG_SUPPORT_IOT_AP_BLACKLIST
#define CFG_IOT_AP_RULE_MAX_CNT 32
#define CFG_IOT_AP_DATA_MAX_LEN 16
#endif
/*------------------------------------------------------------------------------
 * Driver supports reporting max tx rate instead of current tx rate
 * in mtk_cfg80211_get_station
 *------------------------------------------------------------------------------
 */
#define CFG_REPORT_MAX_TX_RATE	0

/*------------------------------------------------------------------------------
 * Link Quality Monitor
 * Link quality monitor execution period base on performance monitor timer
 * CFG_LINK_QUALITY_MONITOR_UPDATE_FREQUENCY base on PERF_MON_UPDATE_INTERVAL
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_LINK_QUALITY_MONITOR
#define CFG_SUPPORT_LINK_QUALITY_MONITOR  1
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if (CFG_SUPPORT_LINK_QUALITY_MONITOR == 1)
#define CFG_LINK_QUALITY_MONITOR_UPDATE_FREQUENCY  1
#else
#define CFG_LINK_QUALITY_MONITOR_UPDATE_FREQUENCY  0
#endif

/*------------------------------------------------------------------------------
 * Driver supports SYS DVT automation
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_WIFI_SYSDVT
#define CFG_SUPPORT_WIFI_SYSDVT  0
#endif

#ifndef CFG_SUPPORT_DMASHDL_SYSDVT
#define CFG_SUPPORT_DMASHDL_SYSDVT  0
#endif

/*------------------------------------------------------------------------------
 * Driver supports WiFi get noise
 *------------------------------------------------------------------------------
 */
#ifndef CONFIG_WIFI_SUPPORT_GET_NOISE
#define CONFIG_WIFI_SUPPORT_GET_NOISE  0
#endif

/*------------------------------------------------------------------------------
 * Driver supports WiFi get antenna report
 *------------------------------------------------------------------------------
 */
#ifndef CONFIG_WIFI_ANTENNA_REPORT
#define CONFIG_WIFI_ANTENNA_REPORT  0
#endif

/*------------------------------------------------------------------------------
 * Flags of using wlan_assistant (only Android) to read/write NVRAM
 *------------------------------------------------------------------------------
 */
#define CFG_WLAN_ASSISTANT_NVRAM		0

/*------------------------------------------------------------------------------
 * Flags of WORKAROUND HWITS00012836 WTBL_SEARCH_FAIL
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_WORKAROUND_HWITS00012836_WTBL_SEARCH_FAIL
#define CFG_WIFI_WORKAROUND_HWITS00012836_WTBL_SEARCH_FAIL 0
#endif

/*------------------------------------------------------------------------------
 * Flags of enabling check if TX ethernet-II frame has empty payload. If yes,
 * then driver drops it.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD
#define CFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD    0
#endif

/*------------------------------------------------------------------------------
 * CONNINFRA SUPPORT (Without WMT)
 * CFG_SUPPORT_CONNINFRA: 1 : conninfra driver exist
 *                        0 : conninfra driver doesn't exist
 * We replace WMT driver with CONNINFRA driver in mobile project
 * after connac2.0.
 * CFG_MTK_ANDROID_WMT is used to separate between mobile project and others.
 * Although WMT driver is removed, CFG_MTK_ANDROID_WMT still need to be set 1
 * for some code flow in mobile gen4m.
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_CONNINFRA 0

/*------------------------------------------------------------------------------
 * Flags of Disconnect with disable channel based on REGD update
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_REGD_UPDATE_DISCONNECT_ALLOWED
#define CFG_SUPPORT_REGD_UPDATE_DISCONNECT_ALLOWED  0
#endif

/*------------------------------------------------------------------------------
 * Notify clients to reconnect when channel switch in hotspot mode to avoid IOT
 * issues, eg. cross band switch.
 *------------------------------------------------------------------------------
 */
#define CFG_SEND_DEAUTH_DURING_CHNL_SWITCH    1

#define CFG_SUPPORT_CUSTOM_NETLINK          0
#if CFG_SUPPORT_CUSTOM_NETLINK
#define CFG_SUPPORT_TX_BEACON_STA_MODE      0
#else
#define CFG_SUPPORT_TX_BEACON_STA_MODE      0
#endif

#define CFG_SUPPORT_WIFI_RNR  1

/*------------------------------------------------------------------------------
 * Flag used for AIS persistent netdev creating.
 * Value 0: Create & register AIS netdev for each wifi on,
 *          and unregister & free AIS netdev for each wifi off.
 * Value 1: Create & register AIS netdev for first wifi on,
 *          and unregister & free AIS netdev during module exit.
 *------------------------------------------------------------------------------
 */
#if defined(_HIF_AXI)
#define CFG_SUPPORT_PERSIST_NETDEV 1
#else
#define CFG_SUPPORT_PERSIST_NETDEV 0
#endif

/*
 * support Performance Monitor or not
 *
 */
#ifndef CFG_SUPPORT_PERMON
#define CFG_SUPPORT_PERMON 1
#endif

/*------------------------------------------------------------------------------
* Driver supports TX resource ctrl for Per-BSS mode
* Note1:
* WMMs map to per-BSS idx statically such as AIS-WMM0 / P2P-WMM1 / SAP-WMM2.
* Make sure your HW support more than 4-WMM queues before function enabled.
* Otherwise, may need to modify WMM-descision based your request.
*
* Note2:
* This feature is adapted with FW-TX-resource-config automatically.
* Make sure your FW working with proper configurations as well.
*------------------------------------------------------------------------------
*/
#ifndef CFG_TX_RSRC_WMM_ENHANCE
#define CFG_TX_RSRC_WMM_ENHANCE  0
#endif

/*------------------------------------------------------------------------------
 * Support platform power off control scenario
 * DC off for Mstar DTV
 *------------------------------------------------------------------------------
 */
#ifndef CFG_POWER_OFF_CTRL_SUPPORT
#define CFG_POWER_OFF_CTRL_SUPPORT	0
#endif

/*
*   Add callback for DC off low power settings for MTK DTV
*/
#ifndef CFG_DC_USB_WOW_CALLBACK
#define CFG_DC_USB_WOW_CALLBACK 0
#else
#if CFG_DC_USB_WOW_CALLBACK
#define CFG_POWER_OFF_CTRL_SUPPORT 1
#endif
#endif
/*------------------------------------------------------------------------------
 * Support wfdma reallocation by triggering L1 reset when device probe
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_WFDMA_REALLOC
#define CFG_SUPPORT_WFDMA_REALLOC	0
#endif

/*------------------------------------------------------------------------------
 * Support mailbox ack mechanism for driver own
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_MAILBOX_ACK
#define CFG_SUPPORT_MAILBOX_ACK	0
#endif

/*------------------------------------------------------------------------------
 * Support Coalescing Interrupt
 *------------------------------------------------------------------------------
 */

#ifndef CFG_COALESCING_INTERRUPT
#define CFG_COALESCING_INTERRUPT	0
#endif

/*------------------------------------------------------------------------------
 * Support EFUSE / EEPROM Auto Detect
 *------------------------------------------------------------------------------
 */
#ifndef CFG_EFUSE_AUTO_MODE_SUPPORT
#define CFG_EFUSE_AUTO_MODE_SUPPORT 0
#endif

/*------------------------------------------------------------------------------
 * Support Debug SOP or not.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_DEBUG_SOP
#define CFG_SUPPORT_DEBUG_SOP  0
#endif

/*------------------------------------------------------------------------------
 * Support NAN or not.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_NAN
#define CFG_SUPPORT_NAN  0
#endif

#if (CFG_SUPPORT_NAN == 1)
#define CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL 1
#define CFG_SUPPORT_NAN_CARRIER_ON_INIT 1
#define CFG_NAN_BSS_SEPARATE_SEC_ROLE 0
#define CFG_NAN_PMF_PATCH 1 /* special handle for peer send PMF w/ NMI */
#define CFG_NAN_ACTION_FRAME_ADDR                                              \
	1 /* 0: use NDI if available, 1: always use NMI */

#define CFG_SUPPORT_NAN_SHOULD_REMOVE_FOR_NO_TYPEDEF 1
#else
#define CFG_SUPPORT_NAN_SHOULD_REMOVE_FOR_NO_TYPEDEF 0
#endif

#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_SUPPORT_WAKEUP_STATISTICS 0 /* fos_change oneline */
#endif

/*------------------------------------------------------------------------------
 * Support Noise histogram
 *------------------------------------------------------------------------------
 */
#ifndef CFG_IPI_2CHAIN_SUPPORT
#define CFG_IPI_2CHAIN_SUPPORT          0
#endif

#ifndef CFG_WIFI_SUPPORT_NOISE_HISTOGRAM
#define CFG_WIFI_SUPPORT_NOISE_HISTOGRAM          0
#endif

/*------------------------------------------------------------------------------
 * Support wifi get beacon timeout or not
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_PHY_SUPPORT_GET_BCNTH
#define CFG_WIFI_PHY_SUPPORT_GET_BCNTH  0
#endif

/*------------------------------------------------------------------------------
 * Support adjustable Fast-PS active interval
 *------------------------------------------------------------------------------
 */
#ifndef CFG_ENABLE_PS_INTV_CTRL
#define CFG_ENABLE_PS_INTV_CTRL  0
#endif

/*------------------------------------------------------------------------------
 * To enable Tx Power Table Dump
 * CFG_WIFI_TXPWR_TBL_DUMP : CCK + OFDM + HT + VHT
 * CFG_WIFI_TXPWR_TBL_DUMP_HE : CCK + OFDM + HT + VHT + HE
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_TXPWR_TBL_DUMP
#define CFG_WIFI_TXPWR_TBL_DUMP 0
#define CFG_WIFI_TXPWR_TBL_DUMP_HE 0
#else
#ifndef CFG_WIFI_TXPWR_TBL_DUMP_HE
#define CFG_WIFI_TXPWR_TBL_DUMP_HE 0
#endif
#endif

/*------------------------------------------------------------------------------
 * Support bt/wifi isolation detect or not
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_ISO_DETECT
#define CFG_WIFI_ISO_DETECT  1
#endif

/*------------------------------------------------------------------------------
 * Value of FWDL UMAC reserve size
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA
#define CFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA (0)
#endif

/*------------------------------------------------------------------------------
 * Support Return task.
 * Linux version only. Force remove for other platform
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_RETURN_TASK
#define CFG_SUPPORT_RETURN_TASK  0
#endif
#ifndef LINUX
#undef CFG_SUPPORT_RETURN_TASK
#define CFG_SUPPORT_RETURN_TASK  0
#endif /* LINUX */

/*------------------------------------------------------------------------------
 * Support tx MGMT frame use ACQ or not.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_TX_MGMT_USE_DATAQ
#define CFG_SUPPORT_TX_MGMT_USE_DATAQ  0
#endif

/*------------------------------------------------------------------------------
 * Support 5G TX MCS Limit
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_5G_TX_MCS_LIMIT
#define CFG_SUPPORT_5G_TX_MCS_LIMIT	0
#endif

/*------------------------------------------------------------------------------
 * Support get_cnm output compitable to customer olg projects
 *------------------------------------------------------------------------------
 */
#ifndef CFG_GET_CNM_INFO_BC
#define CFG_GET_CNM_INFO_BC	0
#endif

/*------------------------------------------------------------------------------
 * Suppoot to get Tx/Rx MCS Info
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_GET_MCS_INFO
#define CFG_WIFI_GET_MCS_INFO (0)
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
#define MCS_INFO_SAMPLE_CNT                 10
#define MCS_INFO_SAMPLE_PERIOD              100 /* Unit: ms */
#endif

#ifndef CFG_SUPPORT_HIDDEN_SW_AP
#define CFG_SUPPORT_HIDDEN_SW_AP	0
#endif


#ifndef CFG_SUPPORT_DYNAMIC_EDCCA
#define CFG_SUPPORT_DYNAMIC_EDCCA 0
#endif

/*------------------------------------------------------------------------------
 * Suppoot to get DPD Cache
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_GET_DPD_CACHE
#define CFG_WIFI_GET_DPD_CACHE (0)
#endif

#if (CFG_WIFI_GET_DPD_CACHE == 1)
#define PER_CH_CAL_CACHE_NUM (8)
#endif

/*------------------------------------------------------------------------------
 * Flags of Tp Enhance Mechanism
 *------------------------------------------------------------------------------
 */

/* 1: Enable Tp Enhance Mechanism
 * 0(default): Disable
 */
#ifndef CFG_SUPPORT_TPENHANCE_MODE
#define CFG_SUPPORT_TPENHANCE_MODE          0
#endif

/*------------------------------------------------------------------------------
 * Flag of GKI project requirement
 *------------------------------------------------------------------------------
 */
/* 1: filp_open/filp_close/kernel_read/kernel_write can't be used
 * 0(default): can be used
 */
#ifndef CFG_ENABLE_GKI_SUPPORT
#define CFG_ENABLE_GKI_SUPPORT          0
#endif

/*------------------------------------------------------------------------------
 * Flags of Customization AP 80211 KVR interface Mechanism
 *------------------------------------------------------------------------------
 */
#ifndef CFG_AP_80211KVR_INTERFACE
#define CFG_AP_80211KVR_INTERFACE 0
#endif

/*------------------------------------------------------------------------------
 * Flags of SAP 802.11K Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_AP_80211K_SUPPORT
#define CFG_AP_80211K_SUPPORT 0
#endif

/*------------------------------------------------------------------------------
 * Flags of SAP 802.11V Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_AP_80211V_SUPPORT
#define CFG_AP_80211V_SUPPORT 0
#endif

/*------------------------------------------------------------------------------
 * Flags of Support immediateley switch channel in CSA action frame
 *------------------------------------------------------------------------------
 */
#ifndef CFG_CSA_ACTION_FRAME_NO_DELAY
#define CFG_CSA_ACTION_FRAME_NO_DELAY 0
#endif

/*------------------------------------------------------------------------------
 * Flags of Force TX Specific Mgmt Frame via ALTX Q Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_ALTX_MGMT
#define CFG_SUPPORT_ALTX_MGMT	1
#endif

/*------------------------------------------------------------------------------
 * Flags of supported Vendor HAL version
 *------------------------------------------------------------------------------
 */

#ifndef CFG_SUPPORT_OLD_VENDOR_HAL
#define CFG_SUPPORT_OLD_VENDOR_HAL	0
#endif

/*------------------------------------------------------------------------------
 * Flags of supported tsf sync
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_TSF_SYNC
#define CFG_SUPPORT_TSF_SYNC    0
#endif

/*------------------------------------------------------------------------------
 * Flags of GET WAKEUP REASON from proc Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_PROC_GET_WAKEUP_REASON
#define CFG_SUPPORT_PROC_GET_WAKEUP_REASON 0
#endif

/*------------------------------------------------------------------------------
* Flags for supported not free pending Tx msduInfo in nicFreePendingTxMsduInfo()
* Prevent clear msdu which is still Tx(Host -> Device)
*------------------------------------------------------------------------------
*/
#ifndef CFG_NOT_CLR_FREE_MSDU_IN_ACTIVE_NETWORK
#define CFG_NOT_CLR_FREE_MSDU_IN_ACTIVE_NETWORK  0
#endif


/* Flags of WAC (Wireless Accessory Configuration) feature */
#ifndef CFG_SUPPORT_WAC
#define CFG_SUPPORT_WAC				0
#endif

/* Compact for some platforms, or wifi will probe fail */
#ifndef CFG_ALLOC_IRQ_VECTORS_IN_WIFI_DRV
#define CFG_ALLOC_IRQ_VECTORS_IN_WIFI_DRV		0
#endif

#ifndef CFG_SUPPORT_STAT_STATISTICS
#define CFG_SUPPORT_STAT_STATISTICS	0 /* fos_change oneline */
#endif

#ifndef CFG_SUPPORT_EXCEPTION_STATISTICS
#define CFG_SUPPORT_EXCEPTION_STATISTICS 0 /* fos_change oneline */
#endif

#ifndef CFG_SUPPORT_WAKEUP_STATISTICS
#define CFG_SUPPORT_WAKEUP_STATISTICS 0 /* fos_change oneline */
#endif

#ifndef CFG_WIFI_SUPPORT_WIFI_ON_STATISTICS
#define CFG_WIFI_SUPPORT_WIFI_ON_STATISTICS 0 /* fos_change oneline */
#endif

/* Flags of disable driver mapping tx queue function
 * 0: use driver mapping tx queue function
 * 1: use linux netstack mapping tx queue function
 */
#ifndef CFG_DISABLE_DRIVER_MAPPING_TXQ
#define CFG_DISABLE_DRIVER_MAPPING_TXQ 0
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#endif /* _CONFIG_H */
