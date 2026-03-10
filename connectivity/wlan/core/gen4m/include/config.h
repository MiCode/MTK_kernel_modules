/*******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
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

/*------------------------------------------------------------------------------
 * Must define in makefile, based on TARGET_BUILD_VARIANT.
 * 1: TARGET_BUILD_VARIANT = userdebug or eng
 * 0: otherwise
 *------------------------------------------------------------------------------
 */
#ifndef BUILD_QA_DBG
#define BUILD_QA_DBG	0
#endif

#ifndef CFG_MTK_ANDROID_WMT
#define CFG_MTK_ANDROID_WMT 0
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
#define CFG_MAX_NUM_OF_CHNL_INFO		50
#define CFG_SUPPORT_CHNL_CONFLICT_REVISE	0


/*------------------------------------------------------------------------------
 * Driver config
 *------------------------------------------------------------------------------
 */

#ifndef CFG_SUPPORT_CFG_FILE
#define CFG_SUPPORT_CFG_FILE	1
#endif

/* Radio Reasource Measurement (802.11k) */
#define CFG_SUPPORT_RRM		0

/* DFS (802.11h) */
#define CFG_SUPPORT_DFS		1
#ifndef CFG_SUPPORT_DFS_MASTER
#define CFG_SUPPORT_DFS_MASTER		1
/* SoftAp Cross Band Channel Switch */
#ifndef CFG_SUPPORT_IDC_CH_SWITCH
#define CFG_SUPPORT_IDC_CH_SWITCH	1
#endif
#endif

#define CFG_SUPPORT_IDC_RIL_BRIDGE  (CFG_SUPPORT_IDC_CH_SWITCH)
#ifdef CFG_MTK_WIFI_SOC_S5E9925_SUPPORT
#define CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY  (CFG_SUPPORT_IDC_RIL_BRIDGE)
#endif
#ifndef CFG_SUPPORT_IDC_RIL_BRIDGE
#define CFG_SUPPORT_IDC_RIL_BRIDGE (0)
#endif
#ifndef CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
#define CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY (0)
#endif

#if (CFG_SUPPORT_DFS == 1)	/* Add by Enlai */
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

#define CFG_SUPPORT_WAPI	1

/* Enable QA Tool Support */
#define CFG_SUPPORT_QA_TOOL	1

#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_SUPPORT_ICAP_SOLICITED_EVENT	1
#else
#define CFG_SUPPORT_ICAP_SOLICITED_EVENT	0
#endif

#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_SUPPORT_CONNAC3X_SMALL_PKT        1
#else
#define CFG_SUPPORT_CONNAC3X_SMALL_PKT        0
#endif

/* Enable TX BF Support */
#define CFG_SUPPORT_TX_BF	1

#define CFG_SUPPORT_TX_BF_FPGA	1

#if CFG_SUPPORT_TX_BF
#define CFG_SUPPORT_BFER	1
#define CFG_SUPPORT_BFEE	1
/* Enable Bfee only when AP's Nss > STA's Nss */
#define CFG_SUPPORT_CONDITIONAL_BFEE	1
#else
#define CFG_SUPPORT_BFER	0
#define CFG_SUPPORT_BFEE	0
/* Enable Bfee only when AP's Nss > STA's Nss */
#define CFG_SUPPORT_CONDITIONAL_BFEE	0
#endif

/* Enable MU MIMO Support */
#define CFG_SUPPORT_MU_MIMO	1

/* Enable WOW Support */
#define CFG_WOW_SUPPORT		1

/* Disable WOW EINT mode */
#ifndef CFG_SUPPORT_WOW_EINT
#define CFG_SUPPORT_WOW_EINT	0
#endif

/* when wow wakeup host, send keyevent to screen on */
#ifndef CFG_SUPPORT_WOW_EINT_KEYEVENT_WAKEUP
#define CFG_SUPPORT_WOW_EINT_KEYEVENT_WAKEUP	0
#endif

/* Enable A-MSDU RX Reordering Support */
#define CFG_SUPPORT_RX_AMSDU	1

/* Enable Detection for 2021 Frag/AGG Attack from WFA */
#define CFG_SUPPORT_FRAG_AGG_ATTACK_DETECTION 1

/*------------------------------------------------------------------------------
 * Enable rx zero copy feature
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_RX_ZERO_COPY
#define CFG_SUPPORT_RX_ZERO_COPY 0
#endif

/*------------------------------------------------------------------------------
 * Use page pool for RX buffer
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_RX_PAGE_POOL
#define CFG_SUPPORT_RX_PAGE_POOL 0
#endif

#ifndef CFG_SUPPORT_DYNAMIC_PAGE_POOL
#define CFG_SUPPORT_DYNAMIC_PAGE_POOL 0
#endif

/*------------------------------------------------------------------------------
 * Support Return task.
 * Linux version only. Force remove for other platform
 *------------------------------------------------------------------------------
 */
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
#define CFG_SUPPORT_RETURN_TASK		0
#define CFG_SUPPORT_RETURN_WORK		1
#else
#define CFG_SUPPORT_RETURN_TASK		1
#define CFG_SUPPORT_RETURN_WORK		0
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */

#ifndef LINUX
#undef CFG_SUPPORT_RETURN_TASK
#define CFG_SUPPORT_RETURN_TASK		0
#define CFG_SUPPORT_RETURN_WORK		0
#endif /* LINUX */

/* Enable handling BA Request advance SSN before data in previous window */
#define CFG_SUPPORT_RX_OOR_BAR	1

/* Enable flushing reordering when running out of SWRFB */
#ifndef CFG_SUPPORT_RX_FLUSH_REORDERING
#define CFG_SUPPORT_RX_FLUSH_REORDERING 0
#endif

/* Mobile(must Android) need default 1 */
#if defined(CONFIG_ANDROID)
#ifndef CFG_ENABLE_WAKE_LOCK
#define CFG_ENABLE_WAKE_LOCK	1
#endif
#endif

/* CE default 0, if need, define 1 in makefile */
#ifndef CFG_ENABLE_WAKE_LOCK
#define CFG_ENABLE_WAKE_LOCK	0
#endif

#define CFG_SUPPORT_OSHARE	1

#define CFG_SUPPORT_LOWLATENCY_MODE	1

#define CFG_SUPPORT_ANT_SWAP		1

/* If skb_buff mark field marked with pre-defined value, change priority to VO*/
#define CFG_CHANGE_PRIORITY_BY_SKB_MARK_FIELD	1

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

#define CFG_SUPPORT_HE_ER               1

#ifdef CFG_COMBO_SLT_GOLDEN
#define CFG_SUPPORT_ICS                 0
#define CFG_SUPPORT_PHY_ICS             0
#else
#define CFG_SUPPORT_ICS                 1
#define CFG_SUPPORT_PHY_ICS             1
#endif

#ifndef CFG_SUPPORT_ICS_TIMESYNC
#define CFG_SUPPORT_ICS_TIMESYNC        1
#endif /* CFG_SUPPORT_ICS_TIMESYNC */

#define CFG_SUPPORT_BAR_DELAY_INDICATION	1

#define CFG_SUPPORT_DROP_INVALID_MSDUINFO	0

#define CFG_SUPPORT_SKB_CLONED_COPY		1

/*------------------------------------------------------------------------------
 * Flags of 6G SUPPORT
 *------------------------------------------------------------------------------
 */
#if 0 /* KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE */
#ifdef CFG_SUPPORT_WIFI_6G
#undef CFG_SUPPORT_WIFI_6G
#endif
#define CFG_SUPPORT_WIFI_6G			0
#endif

/*------------------------------------------------------------------------------
 * Flags of Buffer mode SUPPORT
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_BUFFER_MODE                 1

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

/*
 * TX Work is feature to reschedule Tx Direct to big CPU when BoostCpu
 */
#ifndef CFG_SUPPORT_TX_WORK
#define CFG_SUPPORT_TX_WORK                     0
#endif /* CFG_SUPPORT_TX_WORK */

#ifndef CFG_SUPPORT_RX_WORK
#define CFG_SUPPORT_RX_WORK                     0
#endif /* CFG_SUPPORT_RX_WORK */

/* By using GRO at NAPI level, the driver is doing the aggregation to a large
 * SKB very early, right at the receive completion handler. This means that all
 * the next functions in the receive stack do much less processing.
 * The GRO feature could enhance "Rx" tput.
 */
#define CFG_SUPPORT_RX_GRO                      1

/* 0 : direct-GRO mode (without NAPI poll-callback)
 * 1 : NAPI+GRO mode
 */
#define CFG_SUPPORT_RX_NAPI                     1
#if (CFG_SUPPORT_RX_GRO == 0) && (CFG_SUPPORT_RX_NAPI == 1)
#error "NAPI should based on GRO in gen4m"
#endif

#ifndef CFG_SUPPORT_RX_NAPI_THREADED
#define CFG_SUPPORT_RX_NAPI_THREADED            0
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
#if (CFG_SUPPORT_RX_NAPI == 0) && (CFG_SUPPORT_RX_NAPI_THREADED == 1)
#error "NAPI Threaded should based on NAPI"
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

#ifndef CFG_SDIO_INTR_ENHANCE_FORMAT
#define CFG_SDIO_INTR_ENHANCE_FORMAT                1
#endif

#define CFG_SDIO_ACCESS_N9_REGISTER_BY_MAILBOX      0
#define CFG_MAX_RX_ENHANCE_LOOP_COUNT               3

#define CFG_USB_TX_AGG                              1
#define CFG_USB_CONSISTENT_DMA                      0
#define CFG_USB_TX_HANDLE_IN_HIF_THREAD             0
#define CFG_USB_RX_HANDLE_IN_HIF_THREAD             0

#ifndef CFG_TX_DIRECT
#define CFG_TX_DIRECT                               0
#endif
#ifndef CFG_RX_DIRECT
#define CFG_RX_DIRECT                               0
#endif

#define CFG_HW_WMM_BY_BSS                           1

/*------------------------------------------------------------------------------
 * Flags and Parameters for Integration
 *------------------------------------------------------------------------------
 */

#define CFG_MULTI_ECOVER_SUPPORT	1

#define CFG_ENABLE_CAL_LOG		1
#define CFG_REPORT_RFBB_VERSION		1

#define MAX_BSSID_NUM			4	/* MAX SW BSSID number */
#define MAX_MLDDEV_NUM			4

#ifndef CFG_CHIP_RESET_SUPPORT
#define CFG_CHIP_RESET_SUPPORT		1
#endif

#if CFG_CHIP_RESET_SUPPORT
#define CFG_SER_L05_DEBUG		0
#define CFG_CHIP_RESET_HANG		0
#else
#define CFG_CHIP_RESET_HANG		0
#endif

#define HW_BSSID_NUM			4	/* HW BSSID number by chip */

#define INVALID_OMAC_IDX		0xFF

#define MLD_GROUP_NONE			0xff
#define OM_REMAP_IDX_NONE		0xff
#define MLD_LINK_ID_NONE		0xff
#define ML_PROBE_RETRY_COUNT		2
#define MLD_RETRY_COUNT			6
/* Reserve 0~31 for group mld index */
#define MAT_OWN_MLD_ID_BASE		32

#define MLD_TYPE_INVALID		0
#define MLD_TYPE_ICV_METHOD_V1		1
#define MLD_TYPE_ICV_METHOD_V2		2
#define MLD_TYPE_ICV_METHOD_V1_1	3
#define MLD_TYPE_EXTERNAL		0xff

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
#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_TX_MAX_PKT_NUM                      4096
#elif (CFG_SUPPORT_CONNAC2X == 1)
#define CFG_TX_MAX_PKT_NUM                      2048
#else
#define CFG_TX_MAX_PKT_NUM                      1024
#endif

/*! Maximum number of SW TX CMD packet buffer */
#define CFG_TX_MAX_CMD_PKT_NUM                  96

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

/* Enable this feature may degrade rx peak tput in normal mode */
#ifndef CFG_SUPPORT_SNIFFER_RADIOTAP_13K
#define CFG_SUPPORT_SNIFFER_RADIOTAP_13K 0
#endif

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
#define CFG_MONITOR_BAND_NUM	3
/* HW design: headroom size can only be 128 alignment */
#define CFG_RADIOTAP_HEADROOM	128
#endif
#if CFG_SUPPORT_SNIFFER_RADIOTAP_13K
#define CFG_RX_MAX_MPDU_SIZE	13312 /* support amsdu 7 */
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
#define CFG_RX_MAX_BA_TID_NUM                   8
#define CFG_RX_REORDERING_ENABLED               1

/* Cache RX reordering MSDU pointers by SN to locate search starting point */
#ifndef CFG_SUPPORT_RX_CACHE_INDEX
#define CFG_SUPPORT_RX_CACHE_INDEX		1
#endif

#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_PF_ARP_NS_MAX_NUM                   5
#else
#define CFG_PF_ARP_NS_MAX_NUM                   3
#endif

#define CFG_COMPRESSION_DEBUG			0
#define CFG_DECOMPRESSION_TMP_ADDRESS		0
#define CFG_SUPPORT_COMPRESSION_FW_OPTION	0

/*------------------------------------------------------------------------------
 * Flags and Parameters for CMD/RESPONSE
 *------------------------------------------------------------------------------
 */
/* WMT expects wlan driver on/off to be completed within 4s.
 * To avoid on/off timeout, only polling ready bit in 1s
 * (CFG_RESPONSE_POLLING_TIMEOUT * CFG_RESPONSE_POLLING_DELAY).
 */
#if CFG_MTK_ANDROID_WMT
#define CFG_RESPONSE_POLLING_TIMEOUT            500
#else
#define CFG_RESPONSE_POLLING_TIMEOUT            1000
#endif
#define CFG_RESPONSE_POLLING_DELAY              5

#define CFG_CMD_ALLOC_FAIL_TIMEOUT_MS           (6000)

#define CFG_DEFAULT_SLEEP_WAITING_INTERVAL      50

#define CFG_PRE_CAL_SLEEP_WAITING_INTERVAL      50000

#define CFG_DEFAULT_RX_RESPONSE_TIMEOUT         3000

#define CFG_PRE_CAL_RX_RESPONSE_TIMEOUT         5000

/*------------------------------------------------------------------------------
 * Flags and Parameters for Protocol Stack
 *------------------------------------------------------------------------------
 */
/*! Maximum number of BSS in the SCAN list */
#define CFG_MAX_NUM_BSS_LIST                    192

#define CFG_MAX_COMMON_IE_BUF_LEN         ((1500 * CFG_MAX_NUM_BSS_LIST) / 3)

/*! Maximum size of Header buffer of each SCAN record */
#define CFG_RAW_BUFFER_SIZE                     1160

/*------------------------------------------------------------------------------
 * Flags and Parameters for Power management
 *------------------------------------------------------------------------------
 */
#define CFG_ENABLE_FULL_PM                      1
#define CFG_ENABLE_WAKEUP_ON_LAN                0

/* debug which packet wake up host */
#define CFG_SUPPORT_WAKEUP_REASON_DEBUG         1

#if CFG_MTK_ANDROID_WMT
#define CFG_MODIFY_TX_POWER_BY_BAT_VOLT         1
#else
#define CFG_MODIFY_TX_POWER_BY_BAT_VOLT         0
#endif

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

#define MAX_CHN_NUM			(MAX_2G_BAND_CHN_NUM + \
					MAX_5G_BAND_CHN_NUM + \
					MAX_6G_BAND_CHN_NUM)

#if (CFG_SUPPORT_WIFI_6G == 1)
#define MAX_2G_BAND_CHN_NUM		14
#define MAX_5G_BAND_CHN_NUM		25
#define MAX_6G_BAND_CHN_NUM		59 /* will be 59 for full channel set */
#define MAX_PER_BAND_CHN_NUM		59
#else
#define MAX_2G_BAND_CHN_NUM		14
#define MAX_5G_BAND_CHN_NUM		25
#define MAX_6G_BAND_CHN_NUM		0
#define MAX_PER_BAND_CHN_NUM		25
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

#if 0
/* to be compatible with old FW, we set ssid num to 0 here,
 * we should set correct num when query of scan capability from FW is done
 */
#define SCAN_CMD_EXT_SSID_NUM                   (0)
#define SCAN_CMD_EXT_CHNL_NUM                   (32)
#else
#define SCAN_CMD_EXT_SSID_NUM                   (6)
#define SCAN_CMD_EXT_CHNL_NUM                   (32)
#endif
#define CFG_SCAN_OOB_MAX_NUM			(4)
#define CFG_SCAN_SSID_MAX_NUM (SCAN_CMD_SSID_NUM+SCAN_CMD_EXT_SSID_NUM)
#define MAXIMUM_OPERATION_CHANNEL_LIST (SCAN_CMD_CHNL_NUM+SCAN_CMD_EXT_CHNL_NUM)


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

#define CFG_SUPPORT_TRACE_TC4			0

#ifndef CFG_CE_ASSERT_DUMP
#define CFG_CE_ASSERT_DUMP                         0
#endif

/*------------------------------------------------------------------------------
 * Flags of Firmware Download Option.
 *------------------------------------------------------------------------------
 */
#define CFG_ENABLE_FW_DOWNLOAD                  1

#define CFG_ENABLE_FW_DOWNLOAD_ACK              1

#ifndef CFG_WIFI_IP_SET
#define CFG_WIFI_IP_SET                         1
#endif

#ifndef CFG_WLAN_LK_FWDL_SUPPORT
#define CFG_WLAN_LK_FWDL_SUPPORT                0
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
// lc.zhangzichao1 Modify the WiFi add support connect no p2p IE AP jira id BUGP15-2831
#define CFG_P2P_SCAN_REPORT_ALL_BSS            1
#endif

/* Allow connection with no P2P IE device */
#ifndef CFG_P2P_CONNECT_ALL_BSS
// lc.zhangzichao1 Modify the WiFi add support connect no p2p IE AP jira id BUGP15-2831 
#define CFG_P2P_CONNECT_ALL_BSS            1
#endif

/* Allow setting max P2P GO client count */
#ifndef CFG_P2P_DEFAULT_CLIENT_COUNT
#define CFG_P2P_DEFAULT_CLIENT_COUNT 0
#endif

#ifndef CFG_P2P_FORCE_ROC_CSA
#define CFG_P2P_FORCE_ROC_CSA 1
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
#define CFG_ENABLE_WIFI_DIRECT           1
#define CFG_SUPPORT_802_11W              1	/* Not support at WinXP */
#endif /* LINUX */

#define CFG_SUPPORT_PERSISTENT_GROUP            0

#define CFG_TEST_WIFI_DIRECT_GO                 0

#define CFG_TEST_ANDROID_DIRECT_GO              0

#define CFG_UNITEST_P2P                         0

#ifndef CONFIG_WLAN_DRV_BUILD_IN
#define CONFIG_WLAN_DRV_BUILD_IN		0
#endif

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

#define CFG_HOTSPOT_SUPPORT_ADJUST_SCC          1

#if CFG_TC1_FEATURE
#define CFG_HOTSPOT_SUPPORT_FORCE_ACS_SCC       0
#elif CFG_TC10_FEATURE
#define CFG_HOTSPOT_SUPPORT_FORCE_ACS_SCC       1
#else
#define CFG_HOTSPOT_SUPPORT_FORCE_ACS_SCC       0
#endif

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
#define CFG_LINK_QUALITY_VALID_PERIOD           1000

/*------------------------------------------------------------------------------
 * Migration Option
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_ADHOC                       0
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
#define CFG_SUPPORT_DETECT_SECURITY_MODE_CHANGE 0

/*------------------------------------------------------------------------------
 * Option for NVRAM and Version Checking
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_NVRAM                       1
#define CFG_NVRAM_EXISTENCE_CHECK               1
#define CFG_SW_NVRAM_VERSION_CHECK              1
#define CFG_SUPPORT_NIC_CAPABILITY              1

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

#define CFG_SUPPORT_TDLS		1

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

#define CFG_SUPPORT_MLR				1

#define CFG_SUPPORT_SWCR			1

#define CFG_SUPPORT_ANTI_PIRACY			1

#define CFG_SUPPORT_OSC_SETTING			1

#define CFG_SUPPORT_P2P_RSSI_QUERY		0

#define CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP	0

#define CFG_SHOW_MACADDR_SOURCE			1

#if BUILD_QA_DBG
#define CFG_SHOW_FULL_MACADDR			1
#define CFG_SHOW_FULL_IPADDR			1
#else
#define CFG_SHOW_FULL_MACADDR			0
#define CFG_SHOW_FULL_IPADDR			0
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
#define CFG_SUPPORT_OCE				1


/*!< 1(default): Enable 802.11d */
/* 0: Disable */
#ifndef CFG_SUPPORT_802_11D
#define CFG_SUPPORT_802_11D			1
#endif
#if (CFG_SUPPORT_802_11K == 1)
#undef CFG_SUPPORT_802_11D
#define CFG_SUPPORT_802_11D                     1
#endif

#define CFG_SUPPORT_SUPPLICANT_SME              0

#define CFG_SUPPORT_DPP				1

#if (CFG_SUPPORT_802_11K == 1) && (CFG_SUPPORT_SUPPLICANT_SME == 1)
/* Enable to do beacon reports by supplicant.
 * Beacon report is a sub-feature of 802_11K(RRM)
 * Supplicant only support RRM when SME supported.
 */
#define CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT 0
#else
#define CFG_SUPPORT_RM_BEACON_REPORT_BY_SUPPLICANT 0
#endif

/* Support 802.11v Wireless Network Management */
#ifndef CFG_SUPPORT_802_11V
#define CFG_SUPPORT_802_11V                     1
#endif

#define CFG_SUPPORT_802_11V_TIMING_MEASUREMENT	0

#if (CFG_SUPPORT_802_11V == 1)
#define CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT  1
#else
#define CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT  0
#endif

#if (CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT == 1) && (CFG_SUPPORT_BTM_OFFLOAD == 1)
#define CFG_SUPPORT_802_11V_BTM_OFFLOAD 1
#else
#define CFG_SUPPORT_802_11V_BTM_OFFLOAD 0
#endif

#if (CFG_SUPPORT_802_11V_TIMING_MEASUREMENT == 1) && \
	(CFG_SUPPORT_802_11V == 0)
#error \
"CFG_SUPPORT_802_11V should be 1 once CFG_SUPPORT_802_11V_TIMING_MEASUREMENT equals to 1"
#endif

#ifndef CFG_SUPPORT_802_11BE
#define CFG_SUPPORT_802_11BE                     0
#endif

#ifndef CFG_SUPPORT_802_PP_DSCB
#if CFG_SUPPORT_802_11BE
#define CFG_SUPPORT_802_PP_DSCB                  1
#else
#define CFG_SUPPORT_802_PP_DSCB                  0
#endif
#endif

#ifndef CFG_SUPPORT_802_11BE_MLO
#define CFG_SUPPORT_802_11BE_MLO                 0
#endif

#ifndef CFG_MLO_LINK_PLAN_MODE
#define CFG_MLO_LINK_PLAN_MODE			 0
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1) && \
	(CFG_SUPPORT_802_11BE == 0)
#error \
"CFG_SUPPORT_802_11BE should be 1 once CFG_SUPPORT_802_11BE_MLO equals to 1"
#endif

#ifndef CFG_SUPPORT_APS
#define CFG_SUPPORT_APS				0
#endif

#define WNM_UNIT_TEST CFG_SUPPORT_802_11V

#define CFG_SUPPORT_802_11V_MBSSID		0
#define CFG_SUPPORT_802_11V_MBSSID_OFFLOAD	0

#if (CFG_SUPPORT_802_11AX == 1)
/*11v MBSSID is mandatory for 11ax*/
#undef CFG_SUPPORT_802_11V_MBSSID
#define CFG_SUPPORT_802_11V_MBSSID		1
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

#ifndef CFG_TX_DIRECT_VIA_HIF_THREAD
#define CFG_TX_DIRECT_VIA_HIF_THREAD		0
#endif
#if (CFG_SUPPORT_MULTITHREAD == 0) && (CFG_TX_DIRECT_VIA_HIF_THREAD == 1)
#error "TX_DIRECT_VIA_HIF_THREAD is invalid without MULTITHREAD support"
#endif

#define CFG_SUPPORT_MTK_SYNERGY			1

#ifndef CFG_SUPPORT_RXSMM_WHITELIST
#define CFG_SUPPORT_RXSMM_WHITELIST		0
#endif

#define CFG_SUPPORT_VHT_IE_IN_2G		1

#define CFG_SUPPORT_PWR_LIMIT_COUNTRY		1

#if (CFG_SUPPORT_WIFI_6G == 1)
/* Add dynamic tx power support for 6G before turning on this option !!! */
#define CFG_SUPPORT_DYNAMIC_PWR_LIMIT		1
#else
#define CFG_SUPPORT_DYNAMIC_PWR_LIMIT		1
#endif

#ifdef MT7961
#undef CFG_SUPPORT_DYNAMIC_PWR_LIMIT
#define CFG_SUPPORT_DYNAMIC_PWR_LIMIT		0
#endif

#define CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG	1

#define CFG_FIX_2_TX_PORT			0

#define CFG_CHANGE_CRITICAL_PACKET_PRIORITY	1

#ifndef CFG_TX_HIF_PORT_QUEUE
#define CFG_TX_HIF_PORT_QUEUE		0
#endif
#if (CFG_TX_HIF_PORT_QUEUE == 1) && CFG_FIX_2_TX_PORT
#error "we did not expect fix 2 tx port queue supports TxHifPortQueue"
#endif

#ifndef CFG_TX_HIF_CREDIT_FEATURE
#define CFG_TX_HIF_CREDIT_FEATURE		0
#endif

#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_TX_MGMT_BY_DATA_Q		1
#else
#define CFG_TX_MGMT_BY_DATA_Q		0
#endif

#ifndef CFG_SUPPORT_TX_DATA_DELAY
#define CFG_SUPPORT_TX_DATA_DELAY		0
#endif

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

#ifndef CFG_SUPPORT_TX_LATENCY_STATS
#define CFG_SUPPORT_TX_LATENCY_STATS 0
#endif

#ifndef CFG_SUPPORT_LLS
#define CFG_SUPPORT_LLS 0
#endif

#define CFG_REPORT_TX_RATE_FROM_LLS 0
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
#define CFG_SUPPORT_SCAN_NO_AP_RECOVERY    (1)
#ifndef CFG_SUPPORT_SCHED_SCAN
#define CFG_SUPPORT_SCHED_SCAN             (1)
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
#define CFG_SUPPORT_FULL2PARTIAL_SCAN      (1)
#define CFG_SCAN_FULL2PARTIAL_PERIOD       (60)

/*------------------------------------------------------------------------------
 * Value of scan cache result
 *------------------------------------------------------------------------------
 */
#if CFG_MTK_ANDROID_WMT
#define CFG_SUPPORT_SCAN_CACHE_RESULT      (1)
#else
#define CFG_SUPPORT_SCAN_CACHE_RESULT      (0)
#endif
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
#define RUNNING_DUAL_P2P_MODE 4
#define RUNNING_P2P_DEV_MODE 5
#define RUNNING_P2P_NO_GROUP_MODE 6
#define RUNNING_P2P_MODE_NUM 7

#ifdef CFG_DRIVER_INITIAL_RUNNING_MODE
#define DEFAULT_RUNNING_P2P_MODE (CFG_DRIVER_INITIAL_RUNNING_MODE)
#else
#define DEFAULT_RUNNING_P2P_MODE (RUNNING_P2P_MODE)
#endif /* CFG_DRIVER_RUNNING_MODE */

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
 * Flags for Set IPv6 address to firmware
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_SET_IPV6_NETWORK
#define CFG_SUPPORT_SET_IPV6_NETWORK 0
#endif

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
#define CFG_FW_FLAVOR_MAX_LEN	(16)

/*------------------------------------------------------------------------------
 * Support WMT WIFI Path Config
 *------------------------------------------------------------------------------
 */
#define CFG_WMT_WIFI_PATH_SUPPORT	0

/*------------------------------------------------------------------------------
 * Support CFG_SISO_SW_DEVELOP
 *------------------------------------------------------------------------------
 */
#define CFG_SISO_SW_DEVELOP			1

/*------------------------------------------------------------------------------
 * Support spatial extension control
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_SPE_IDX_CONTROL		1

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

#ifndef CFG_SUPPORT_SINGLE_SKU_6G
#define CFG_SUPPORT_SINGLE_SKU_6G 1
#endif

#ifndef CFG_SUPPORT_SINGLE_SKU_LOCAL_DB
#define CFG_SUPPORT_SINGLE_SKU_LOCAL_DB 1
#endif

#ifndef CFG_SUPPORT_BW160
#define CFG_SUPPORT_BW160 0
#endif

#ifndef CFG_SUPPORT_BW320
#define CFG_SUPPORT_BW320 0
#endif

/*------------------------------------------------------------------------------
 * Direct Control for RF/PHY/BB/MAC for Manual Configuration via command/api
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_ADVANCE_CONTROL 1


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
#define CFG_FORCE_ENABLE_PERF_MONITOR	0

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
 * Driver supports p2p listen offload
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_P2P_LISTEN_OFFLOAD  1

/*------------------------------------------------------------------------------
 * Driver supports p2p ecsa
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_P2P_ECSA  1

/*------------------------------------------------------------------------------
 * Flag used for P2P GO to find the best channel list
 * Value 0: Disable
 * Value 1: Enable
 * Note: Must Enable CFG_SUPPORT_P2P_PREFERRED_FREQ_LIST in advance
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_P2PGO_ACS 1

/*------------------------------------------------------------------------------
 * Flag used for P2P U-APSD support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_P2P_UAPSD_SUPPORT
#define CFG_P2P_UAPSD_SUPPORT 1
#endif

/*------------------------------------------------------------------------------
 * Driver supports rx buffer size query
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_RX_QUOTA_INFO 1

/*-----------------------------------------------------------------------------
* Flags to support IOT AP blacklist
*------------------------------------------------------------------------------
*/
#ifndef CFG_SUPPORT_IOT_AP_BLACKLIST
#define CFG_SUPPORT_IOT_AP_BLACKLIST 1
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
#define CFG_REPORT_MAX_TX_RATE	1

/*------------------------------------------------------------------------------
 * Link Quality Monitor
 * Link quality monitor execution period base on performance monitor timer
 * CFG_LQ_MONITOR_FREQUENCY base on PERF_MON_UPDATE_INTERVAL
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_LINK_QUALITY_MONITOR
#define CFG_SUPPORT_LINK_QUALITY_MONITOR  1
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if (CFG_SUPPORT_LINK_QUALITY_MONITOR == 1)
#define CFG_LQ_MONITOR_FREQUENCY  1
#else
#define CFG_LQ_MONITOR_FREQUENCY  0
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

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
 * Flags of using wlan_assistant to read/write NVRAM
 *------------------------------------------------------------------------------
 */
#if CFG_MTK_ANDROID_WMT
#define CFG_WLAN_ASSISTANT_NVRAM		1
#else
#define CFG_WLAN_ASSISTANT_NVRAM		0
#endif

/*------------------------------------------------------------------------------
 * SW handles WTBL_SEARCH_FAIL
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_SW_WTBL_SEARCH_FAIL
#define CFG_WIFI_SW_WTBL_SEARCH_FAIL 1
#endif

/*------------------------------------------------------------------------------
 * SW enables CIPHER_MISMATCH
 *------------------------------------------------------------------------------
 */
#define CFG_WIFI_SW_CIPHER_MISMATCH 1

/*------------------------------------------------------------------------------
 * Flags of enabling setting VTA in accordance with fixed rate.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_TX_FIXED_RATE_NO_VTA
#define CFG_WIFI_TX_FIXED_RATE_NO_VTA 0
#endif

/*------------------------------------------------------------------------------
 * Flags of enabling check if TX ethernet-II frame has empty payload. If yes,
 * then driver drops it.
 *------------------------------------------------------------------------------
 */
#define CFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD 1

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
#ifndef CFG_SUPPORT_CONNINFRA
#define CFG_SUPPORT_CONNINFRA 0
#endif

#ifndef CFG_SUPPORT_CONNFEM
#define CFG_SUPPORT_CONNFEM 0
#endif

#ifndef CFG_SUPPORT_PRE_ON_PHY_ACTION
#define CFG_SUPPORT_PRE_ON_PHY_ACTION 0
#endif

#ifndef CFG_DOWNLOAD_DYN_MEMORY_MAP
#define CFG_DOWNLOAD_DYN_MEMORY_MAP 0
#endif

#ifndef CFG_ROM_PATCH_NO_SEM_CTRL
#define CFG_ROM_PATCH_NO_SEM_CTRL 0
#endif

#ifndef CFG_SUPPORT_MDDP_AOR
#define CFG_SUPPORT_MDDP_AOR 0
#endif

#ifndef CFG_SUPPORT_MDDP_SHM
#define CFG_SUPPORT_MDDP_SHM 0
#endif

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

/*------------------------------------------------------------------------------
 *Smart Gear Feature Configure
 *------------------------------------------------------------------------------
*/
#ifndef CFG_SUPPORT_SMART_GEAR
#define CFG_SUPPORT_SMART_GEAR 1
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
#if defined(_HIF_USB)
#define CFG_SUPPORT_PERSIST_NETDEV 0
#else
#define CFG_SUPPORT_PERSIST_NETDEV 1
#endif


/*------------------------------------------------------------------------------
 * Dynamic tx power control:
 * Support additional tx power setting on CCK AND OFDM
 *
 * No define: CCK,HT20L,HT20H,HT40L,HT40H,HT80L,HT80H,HT160L,HT160H
 * Defined: CCK_L,CCK_H,OFDM_L,OFDM_H,HT20L,HT20H,HT40L,HT40H,HT80L,
 * HT80H,HT160L,HT160H
 *
 * note: need to confirm firmware support this feature
 *       COUNTRY_CHANNEL_TXPOWER_LIMIT_TYPE_COMP_11AC_V2
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING 0

/*------------------------------------------------------------------------------
 * Dynamic tx power control:
 * Support additional tx power setting on EHT
 *
 * support power limit for
 *                EHT26/EHT52/EHT104/EHT242/EHT484/EHT996/EHT996x2/
 *                EHT996x4/EHT26_52/EHT26_106/EHT484_242/EHT996_484/
 *                EHT996_484_242/EHT996x2_484/EHT996x3/EHT996x3_484
 *
 * note: 1. EHT support 2.4G/5G/6G
 *       2. need to confirm firmware support EHT(802.11BE)
 *------------------------------------------------------------------------------
 */
#if (CFG_SUPPORT_802_11BE == 1)
#define CFG_SUPPORT_PWR_LIMIT_EHT	1
#else
#define CFG_SUPPORT_PWR_LIMIT_EHT	0
#endif /* CFG_SUPPORT_802_11BE */

/*------------------------------------------------------------------------------
 * tx power control:
 * Support additional tx power setting for HE
 *
 * support power limit for RU26/RU52/RU104/RU242/RU484/RU996
 *
 * note: need to confirm firmware support HE (802.11AX)
 *------------------------------------------------------------------------------
 */
#if (CFG_SUPPORT_802_11AX == 1)
#if (CFG_SUPPORT_WIFI_6G == 1)
/* Add HE tx power support for 6G before turning on this option !!! */
#define CFG_SUPPORT_PWR_LIMIT_HE		1
#else
#define CFG_SUPPORT_PWR_LIMIT_HE		1
#endif /* CFG_SUPPORT_WIFI_6G */
#else
#define CFG_SUPPORT_PWR_LIMIT_HE		0
#endif /* CFG_SUPPORT_802_11AX */

#define CFG_SUPPORT_STAT_STATISTICS	0 /* fos_change oneline */
#define CFG_SUPPORT_WAKEUP_STATISTICS 0 /* fos_change oneline */
#define CFG_SUPPORT_EXCEPTION_STATISTICS 0 /* fos_change oneline */
/*------------------------------------------------------------------------------
 * cnm power control:
 * for power save, disable 2x2 and DBDC when power is low
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_CNM_POWER_CTRL		    1

/*------------------------------------------------------------------------------
 * Connsys Power Throttling feature
 * Value 1: support Connsys Power Throttling feature
 * Value 0: not support Connsys Power Throttling feature
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_POWER_THROTTLING
#define CFG_SUPPORT_POWER_THROTTLING 0
#endif

/*
*   Add callback for DC off low power settings for MTK DTV
*/
#ifndef CFG_DC_USB_WOW_CALLBACK
#define CFG_DC_USB_WOW_CALLBACK 0
#endif
/*------------------------------------------------------------------------------
 * Flag used for packet offload support.
 * Value 0: Do not enable packet offload.
 * Value 1: Enable packet offload.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_PKT_OFLD
#define CFG_SUPPORT_PKT_OFLD 0
#endif

/*------------------------------------------------------------------------------
 * Flag used for comb matrix support.
 * Value 0: Do not enable packet offload.
 * Value 1: Enable packet offload.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_COMB_MATRIX
#define CFG_SUPPORT_COMB_MATRIX 1
#endif

/*------------------------------------------------------------------------------
 * Flag used for APF support.
 * Value 0: Do not enable APF.
 * Value 1: Enable APF.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_APF
#define CFG_SUPPORT_APF 0
#endif

/*------------------------------------------------------------------------------
 * Support NAN or not.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_NAN
#define CFG_SUPPORT_NAN  0
#endif

#if (CFG_SUPPORT_NAN == 1)
#define CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL 2
#define CFG_SUPPORT_NAN_CARRIER_ON_INIT 1
#define CFG_NAN_BSS_SEPARATE_SEC_ROLE 0
#define CFG_NAN_PMF_PATCH 1 /* special handle for peer send PMF w/ NMI */
#define CFG_NAN_ACTION_FRAME_ADDR                                              \
	1 /* 0: use NDI if available, 1: always use NMI */

#define CFG_SUPPORT_NAN_SHOULD_REMOVE_FOR_NO_TYPEDEF 1

/* NAN scheduler version
* 0: AIS use last 8 slots
* 1: AIS+NAN SCC, or AIS use 0x00FF00FF for MCC
*/
#define CFG_NAN_SCHEDULER_VERSION  1

#define CFG_SUPPORT_NAN_NDP_DUAL_BAND 0

#else
#define CFG_SUPPORT_NAN_SHOULD_REMOVE_FOR_NO_TYPEDEF 0
#endif

#ifdef SOC7_0
#define CFG_SUPPORT_AVOID_DESENSE 1
#else
#define CFG_SUPPORT_AVOID_DESENSE 0
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
#define CFG_SUPPORT_TPENHANCE_MODE          0

#ifndef CFG_MTK_FPGA_PLATFORM
#define CFG_MTK_FPGA_PLATFORM			0
#endif

#ifdef CFG_MLD_LINK_MAX
#define MLD_LINK_MAX (CFG_MLD_LINK_MAX)
#else
#define MLD_LINK_MAX 1
#endif

#ifdef CFG_DBDC_MODE
#define DEFAULT_DBDC_MODE (CFG_DBDC_MODE)
#else
#define DEFAULT_DBDC_MODE ENUM_DBDC_MODE_DYNAMIC
#endif

#ifdef CFG_NSS
#define DEFAULT_NSS (CFG_NSS)
#else
#define DEFAULT_NSS (2)
#endif

#ifndef CFG_MTK_WIFI_SW_WFDMA
#define CFG_MTK_WIFI_SW_WFDMA			0
#endif

#ifndef CFG_MTK_WIFI_SW_EMI_RING
#define CFG_MTK_WIFI_SW_EMI_RING		0
#endif

#ifndef CFG_MTK_WIFI_EN_SW_EMI_READ
#define CFG_MTK_WIFI_EN_SW_EMI_READ		0
#endif

#if (CFG_SUPPORT_802_11AX == 1)
#define CFG_SUPPORT_BSS_MAX_IDLE_PERIOD         1
#else
#define CFG_SUPPORT_BSS_MAX_IDLE_PERIOD         0
#endif /* CFG_SUPPORT_802_11AX */

#ifndef CFG_MTK_WIFI_WFDMA_BK_RS
#define CFG_MTK_WIFI_WFDMA_BK_RS		0
#endif

#ifndef CFG_SUPPORT_TSF_SYNC
#define CFG_SUPPORT_TSF_SYNC    0
#endif

/* 1(default): Run on big core when tput over threshold
 * 0: Disable (Let system scheduler decide)
 */
#define CFG_SUPPORT_TPUT_ON_BIG_CORE 1

#define CFG_SUPPORT_LITTLE_CPU_BOOST 0

#ifndef CFG_DYNAMIC_RFB_ADJUSTMENT
#define CFG_DYNAMIC_RFB_ADJUSTMENT 0
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

#define CFG_SUPPORT_MCC_BOOST_CPU 1
#if CFG_SUPPORT_MCC_BOOST_CPU
#define MCC_BOOST_LEVEL 1
#define MCC_BOOST_MIN_TIME 70
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */

#define CFG_SUPPORT_ANDROID_DUAL_STA 0

/*------------------------------------------------------------------------------
 * Value of FWDL UMAC reserve size
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA
#define CFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA (0)
#endif

#define CFG_SUPPORT_LIMITED_PKT_PID  1

#ifndef CFG_RFB_TRACK
#define CFG_RFB_TRACK 1
#endif /* CFG_RFB_TRACK */

/*------------------------------------------------------------------------------
 * Support FreeMsdu tasklet.
 * Linux version only. Force remove for other platform
 *------------------------------------------------------------------------------
 */
#define CFG_SUPPORT_TASKLET_FREE_MSDU	1
#ifndef LINUX
#undef CFG_SUPPORT_TASKLET_FREE_MSDU
#define CFG_SUPPORT_TASKLET_FREE_MSDU	0
#endif /* LINUX */

#ifndef CFG_SUPPORT_TX_FREE_MSDU_WORK
#define CFG_SUPPORT_TX_FREE_MSDU_WORK 0
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */

#if (CFG_SUPPORT_TASKLET_FREE_MSDU == 0) && (CFG_SUPPORT_TX_FREE_MSDU_WORK == 1)
#error "TX_FREE_MSDU_WORK is based on TASKLET_FREE_MSDU."
#endif

/*------------------------------------------------------------------------------
 * Flags of Force TX via ALTX Q Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_FORCE_ALTX
#define CFG_SUPPORT_FORCE_ALTX	0
#endif

/*------------------------------------------------------------------------------
 * Flag of CMD over WFDMA support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_CMD_OVER_WFDMA
#define CFG_SUPPORT_CMD_OVER_WFDMA	0
#endif

#define CFG_SUPPORT_CUSTOM_NETLINK          0
#if CFG_SUPPORT_CUSTOM_NETLINK
#define CFG_SUPPORT_TX_BEACON_STA_MODE      0
#else
#define CFG_SUPPORT_TX_BEACON_STA_MODE      0
#endif

/*------------------------------------------------------------------------------
 * Support Debug SOP or not.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_DEBUG_SOP
#define CFG_SUPPORT_DEBUG_SOP  0
#endif

/*------------------------------------------------------------------------------
 * Support Coalescing Interrupt
 *------------------------------------------------------------------------------
 */

#ifndef CFG_COALESCING_INTERRUPT
#define CFG_COALESCING_INTERRUPT	0
#endif

/*------------------------------------------------------------------------------
 * Support platform power off control scenario
 *------------------------------------------------------------------------------
 */
#ifndef CFG_POWER_OFF_CTRL_SUPPORT
#define CFG_POWER_OFF_CTRL_SUPPORT	0
#endif

/*------------------------------------------------------------------------------
 * Support TX hidden SSID beacon
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_HIDDEN_SW_AP
#define CFG_SUPPORT_HIDDEN_SW_AP	0
#endif

#ifndef CFG_SUPPORT_DYNAMIC_EDCCA
#define CFG_SUPPORT_DYNAMIC_EDCCA 0
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

#ifndef CFG_EFUSE_AUTO_MODE_SUPPORT
#define CFG_EFUSE_AUTO_MODE_SUPPORT 0
#endif

/*------------------------------------------------------------------------------
 * Support bt/wifi isolation detect or not
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WIFI_ISO_DETECT
#define CFG_WIFI_ISO_DETECT  1
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
 * Support tx MGMT frame use ACQ or not.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_TX_MGMT_USE_DATAQ
#define CFG_SUPPORT_TX_MGMT_USE_DATAQ  0
#endif

/*------------------------------------------------------------------------------
 * Support separate TXS pid of Data from Management frames.
 *------------------------------------------------------------------------------
 */
#ifndef CFG_SUPPORT_SEPARATE_TXS_PID_POOL
#define CFG_SUPPORT_SEPARATE_TXS_PID_POOL 0
#endif

/*------------------------------------------------------------------------------
* Flags for supported not free pending Tx msduInfo in nicDeactivateNetworkEx()
* Prevent clear msdu which is still Tx(Host -> Device)
*------------------------------------------------------------------------------
*/
#ifndef CFG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK
#define CFG_NOT_CLR_FREE_MSDU_IN_DEACTIVE_NETWORK  0
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

/*------------------------------------------------------------------------------
 * Support get_cnm output compitable to customer olg projects
 *------------------------------------------------------------------------------
 */
#ifndef CFG_GET_CNM_INFO_BC
#define CFG_GET_CNM_INFO_BC	0
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

#define CFG_SUPPORT_DISABLE_CMD_DDONE_INTR    1
#define CFG_SUPPORT_DISABLE_DATA_DDONE_INTR   1

/*------------------------------------------------------------------------------
 * Flags of ATF (ARM Trusted firmware) Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WLAN_ATF_SUPPORT
#define CFG_WLAN_ATF_SUPPORT 0
#endif

#if CFG_SUPPORT_THERMAL_QUERY
#ifndef CONFIG_THERMAL_OF
#undef CFG_SUPPORT_THERMAL_QUERY
#define CFG_SUPPORT_THERMAL_QUERY 0
#endif
#endif

/*------------------------------------------------------------------------------
 * Flags of CSI (Channel State Information) Support
 *------------------------------------------------------------------------------
 */
#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_SUPPORT_CSI 1
#else
#define CFG_SUPPORT_CSI 0
#endif

#if (CFG_SUPPORT_CSI == 1)
#define CFG_CSI_DEBUG 1
#endif

/*------------------------------------------------------------------------------
 * Flags of WFD SCC Balance
 *------------------------------------------------------------------------------
 */
#ifndef CFG_WFD_SCC_BALANCE_SUPPORT
#define CFG_WFD_SCC_BALANCE_SUPPORT		0
#endif

#ifndef CFG_WFD_SCC_BALANCE_DEF_ENABLE
#define CFG_WFD_SCC_BALANCE_DEF_ENABLE	0
#endif

/*------------------------------------------------------------------------------
 * Flags of Fast Path Feature Support
 *------------------------------------------------------------------------------
 */
#ifndef CFG_FAST_PATH_SUPPORT
#define CFG_FAST_PATH_SUPPORT 0
#endif

#define CFG_SUPPORT_RTT			(1)
#define CFG_RTT_TEST_MODE		(0)
#define CFG_RTT_MAX_CANDIDATES	10

#if (CFG_SUPPORT_CONNAC3X == 1)
#define CFG_WIFI_IGTK_GTK_SEPARATE	0
#else
#define CFG_WIFI_IGTK_GTK_SEPARATE	1
#endif

#define CFG_SUPPORT_SW_BIP_GMAC	1

/*------------------------------------------------------------------------------
 * Flags of Advanced TDLS Support
 *------------------------------------------------------------------------------
 */
#if (CFG_TC10_FEATURE == 1) && (CFG_SUPPORT_TDLS == 1)
#define CFG_SUPPORT_TDLS_OFFCHANNEL	1
#define CFG_SUPPORT_TDLS_ADJUST_BW	1
#define CFG_SUPPORT_TDLS_P2P	1
#define CFG_SUPPORT_TDLS_P2P_AUTO	1
#if (CFG_SUPPORT_802_11AX == 1)
#define CFG_SUPPORT_TDLS_11AX		1
#endif
#endif

#ifndef CFG_SUPPORT_TDLS_OFFCHANNEL
#define CFG_SUPPORT_TDLS_OFFCHANNEL	0
#endif
#ifndef CFG_SUPPORT_TDLS_11AX
#define CFG_SUPPORT_TDLS_11AX	0
#endif
#ifndef CFG_SUPPORT_TDLS_11BE
#define CFG_SUPPORT_TDLS_11BE	0
#endif
#ifndef CFG_SUPPORT_TDLS_P2P
#define CFG_SUPPORT_TDLS_P2P	0
#endif
#ifndef CFG_SUPPORT_TDLS_P2P_OFFCHANNEL
#define CFG_SUPPORT_TDLS_P2P_OFFCHANNEL	0
#endif
#ifndef CFG_SUPPORT_TDLS_P2P_AUTO
#define CFG_SUPPORT_TDLS_P2P_AUTO	0
#endif
#ifndef CFG_SUPPORT_TDLS_ADJUST_BW
#define CFG_SUPPORT_TDLS_ADJUST_BW	0
#endif
#ifndef CFG_SUPPORT_TDLS_LOG
#define CFG_SUPPORT_TDLS_LOG	0
#endif

/*------------------------------------------------------------------------------
 * Flag of Wifi Standalone Log Support.
 * 1: Enable. Could be supported only if (CFG_MTK_ANDROID_WMT == 1).
 * 0: Disable.
 *------------------------------------------------------------------------------
 */
#if CFG_MTK_ANDROID_WMT
#define CFG_SUPPORT_SA_LOG 0
#else
#define CFG_SUPPORT_SA_LOG 0
#endif

#define CFG_SUPPORT_SKIP_REORDER_IN_INITIAL  0

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
#define WM_RAM_TYPE_MOBILE			0
#define WM_RAM_TYPE_CE				1

#define IS_MOBILE_SEGMENT \
	(CONFIG_WM_RAM_TYPE == WM_RAM_TYPE_MOBILE)

#define IS_CE_SEGMENT \
	(CONFIG_WM_RAM_TYPE == WM_RAM_TYPE_CE)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#endif /* _CONFIG_H */
