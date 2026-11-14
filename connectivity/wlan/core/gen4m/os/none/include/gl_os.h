/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_os.h
 *    \brief  List the external reference to OS for GLUE Layer.
 *
 *    In this file we define the data structure - GLUE_INFO_T to store those
 *    objects
 *    we acquired from OS - e.g. TIMER, SPINLOCK, NET DEVICE ... . And all the
 *    external reference (header file, extern func() ..) to OS for GLUE Layer
 *    should also list down here.
 *    For porting to new OS, the API listed in this file needs to be
 *    implemented
 */


#ifndef _GL_OS_H
#define _GL_OS_H

/* Standard C, for implementation on differenct OS
 * the following include files needs to be refined
 */
/* types */
#include <stdint.h>
#include "sys/types.h"

/* bool type */
#include <stdbool.h>

/* memory operation */
#include <string.h>

/* networking */
#include <netinet/in.h>

/* offsetof series */
#include <stddef.h>

/* sprintf series */
#include <stdio.h>

/* error code */
#include <errno.h>
#include <sys/stat.h>

/* va_* series */
#include<stdarg.h>

/* for INT_MAX for some reason include fail not checked */
#if 0
/* when there is platform specific toolchain
 * there should be this file
 * eg. gcc-arm-none-eabi/arm-none-eabi/include/limits.h
 */
#include <limits.h>
#else
/* Minimum and maximum values a `signed int' can hold.  */
#  define INT_MIN	(-INT_MAX - 1)
#  define INT_MAX	2147483647
#endif
/* cannot find the necessary include header */
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

/* driver internal */
#include "version.h"
#include "config.h"
#include "gl_dependent.h"

#include "gl_typedef.h"
#include "typedef.h"
#include "queue.h"
#include "gl_kal.h"
#include "gl_rst.h"
#include "hif.h"

#if CFG_SUPPORT_TDLS
#include "tdls.h"
#endif

#include "debug.h"

#include "wlan_oid.h"

/*
 * comment: declared in wlan_lib.c
 * and almost all of the glue layer includes #include "precomp.h"
 * should we just but it in wlan_lib.h
 */
extern u_int8_t fgIsBusAccessFailed;
#if CFG_MTK_WIFI_PCIE_SUPPORT
extern u_int8_t fgIsPcieDataTransDisabled;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
extern u_int8_t fgTriggerDebugSop;
#endif
extern u_int32_t u4SdesDetectTime;

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#ifndef __weak
#define __weak __attribute__((weak))
#endif
/*------------------------------------------------------------------------------
 * Flags for OS dependent
 *------------------------------------------------------------------------------
 */
#define CFG_MAX_WLAN_DEVICES 1 /* number of wlan card will coexist */

#define CFG_MAX_TXQ_NUM 4 /* number of tx queue for support multi-queue h/w  */

/* 1: Enable use of SPIN LOCK Bottom Half for LINUX */
/* 0: Disable - use SPIN LOCK IRQ SAVE instead */
#define CFG_USE_SPIN_LOCK_BOTTOM_HALF       0

/* 1: Enable - Drop ethernet packet if it < 14 bytes.
 * And pad ethernet packet with dummy 0 if it < 60 bytes.
 * 0: Disable
 */
#define CFG_TX_PADDING_SMALL_ETH_PACKET     0

#define CFG_TX_STOP_NETIF_QUEUE_THRESHOLD   256	/* packets */

#define CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD   256	/* packets */
#define CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD  128	/* packets */

#define CHIP_NAME    "MT6632"

#define DRV_NAME "["CHIP_NAME"]: "

/* for CFG80211 IE buffering mechanism */
#define	CFG_CFG80211_IE_BUF_LEN		(512)
#define	GLUE_INFO_WSCIE_LENGTH		(500)
/* for non-wfa vendor specific IE buffer */
#define NON_WFA_VENDOR_IE_MAX_LEN	(128)

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define FW_LOG_CMD_ON_OFF		0
#define FW_LOG_CMD_SET_LEVEL		1

#define CONTROL_BUFFER_SIZE		(1025)
/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* driver events for threading */
#define GLUE_FLAG_HALT                  BIT(0)
#define GLUE_FLAG_INT                   BIT(1)
#define GLUE_FLAG_OID                   BIT(2)
#define GLUE_FLAG_TIMEOUT               BIT(3)
#define GLUE_FLAG_TXREQ                 BIT(4)
#define GLUE_FLAG_FRAME_FILTER          BIT(8)
#define GLUE_FLAG_FRAME_FILTER_AIS      BIT(9)

#define GLUE_FLAG_HALT_BIT              (0)
#define GLUE_FLAG_INT_BIT               (1)
#define GLUE_FLAG_OID_BIT               (2)
#define GLUE_FLAG_TIMEOUT_BIT           (3)
#define GLUE_FLAG_TXREQ_BIT             (4)
#define GLUE_FLAG_FRAME_FILTER_BIT      (8)
#define GLUE_FLAG_FRAME_FILTER_AIS_BIT  (9)

#if CFG_SUPPORT_MULTITHREAD
#define GLUE_FLAG_TX_CMD_DONE			BIT(11)
#define GLUE_FLAG_HIF_TX			BIT(12)
#define GLUE_FLAG_HIF_TX_CMD			BIT(13)
#define GLUE_FLAG_RX_TO_OS			BIT(14)
#define GLUE_FLAG_HIF_FW_OWN			BIT(15)
#define GLUE_FLAG_HIF_PRT_HIF_DBG_INFO		BIT(16)
#define GLUE_FLAG_UPDATE_WMM_QUOTA		BIT(17)
#define GLUE_FLAG_NOTIFY_MD_CRASH		BIT(18)

#define GLUE_FLAG_TX_CMD_DONE_BIT		(11)
#define GLUE_FLAG_HIF_TX_BIT			(12)
#define GLUE_FLAG_HIF_TX_CMD_BIT		(13)
#define GLUE_FLAG_RX_TO_OS_BIT			(14)
#define GLUE_FLAG_HIF_FW_OWN_BIT		(15)
#endif
#define GLUE_FLAG_RX				BIT(10)
#define GLUE_FLAG_HIF_PRT_HIF_DBG_INFO		BIT(16)
#define GLUE_FLAG_UPDATE_WMM_QUOTA		BIT(17)

#define GLUE_FLAG_RX_BIT			(10)
#define GLUE_FLAG_HIF_PRT_HIF_DBG_INFO_BIT	(16)
#define GLUE_FLAG_UPDATE_WMM_QUOTA_BIT		(17)
#define GLUE_FLAG_NOTIFY_MD_CRASH_BIT		(18)
#define GLUE_FLAG_DRV_INT_BIT			(19)

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
#define GLUE_FLAG_CNS_PWR_LEVEL_BIT		(21)
#define GLUE_FLAG_CNS_PWR_TEMP_BIT		(22)
#define GLUE_FLAG_CNS_PWR_LEVEL			BIT(21)
#define GLUE_FLAG_CNS_PWR_TEMP			BIT(22)
#endif

#define HIF_FLAG_AER_RESET		BIT(0)
#define HIF_FLAG_AER_RESET_BIT	(0)

#define HIF_FLAG_MSI_RECOVERY		BIT(1)
#define HIF_FLAG_MSI_RECOVERY_BIT	(1)

/* wake lock for suspend/resume */
#define WAKE_LOCK_RX_TIMEOUT                            300	/* ms */
#define WAKE_LOCK_THREAD_WAKEUP_TIMEOUT                 50	/* ms */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct GLUE_INFO;

struct GL_WPA_INFO {
	uint32_t u4WpaVersion;
	uint32_t u4KeyMgmt;
	uint32_t u4CipherGroup;
	uint32_t u4CipherPairwise;
	uint32_t u4CipherGroupMgmt;
	uint32_t u4AuthAlg;
	u_int8_t fgPrivacyInvoke;
#if CFG_SUPPORT_802_11W
	uint32_t u4Mfp;
	uint8_t ucRSNMfpCap;
#endif
	uint16_t u2RSNXCap;
	uint8_t aucKek[NL80211_KEK_LEN];
	uint8_t aucKck[NL80211_KCK_LEN];
	uint8_t aucReplayCtr[NL80211_REPLAY_CTR_LEN];
};

#if CFG_SUPPORT_REPLAY_DETECTION
struct GL_REPLEY_PN_INFO {
	uint8_t auPN[16];
	u_int8_t fgRekey;
	u_int8_t fgFirstPkt;
};
struct GL_DETECT_REPLAY_INFO {
	uint8_t ucCurKeyId;
	uint8_t ucKeyType;
	struct GL_REPLEY_PN_INFO arReplayPNInfo[4];
};
#endif

enum ENUM_NET_DEV_IDX {
	NET_DEV_WLAN_IDX = 0,
	NET_DEV_P2P_IDX,
	NET_DEV_BOW_IDX,
	NET_DEV_NUM
};

enum ENUM_RSSI_TRIGGER_TYPE {
	ENUM_RSSI_TRIGGER_NONE,
	ENUM_RSSI_TRIGGER_GREATER,
	ENUM_RSSI_TRIGGER_LESS,
	ENUM_RSSI_TRIGGER_TRIGGERED,
	ENUM_RSSI_TRIGGER_NUM
};

enum ENUM_NET_REG_STATE {
	ENUM_NET_REG_STATE_UNREGISTERED,
	ENUM_NET_REG_STATE_REGISTERING,
	ENUM_NET_REG_STATE_REGISTERED,
	ENUM_NET_REG_STATE_UNREGISTERING,
	ENUM_NET_REG_STATE_NUM
};

#if CFG_ENABLE_WIFI_DIRECT
enum ENUM_P2P_REG_STATE {
	ENUM_P2P_REG_STATE_UNREGISTERED,
	ENUM_P2P_REG_STATE_REGISTERING,
	ENUM_P2P_REG_STATE_REGISTERED,
	ENUM_P2P_REG_STATE_UNREGISTERING,
	ENUM_P2P_REG_STATE_NUM
};
#endif

enum ENUM_PKT_FLAG {
	ENUM_PKT_802_11,	/* 802.11 or non-802.11 */
	ENUM_PKT_802_3,		/* 802.3 or ethernetII */
	ENUM_PKT_1X,		/* 1x frame or not */
	ENUM_PKT_NON_PROTECTED_1X,	/* Non protected 1x frame */
	ENUM_PKT_VLAN_EXIST,	/* VLAN tag exist */
	ENUM_PKT_DHCP,		/* DHCP frame */
	ENUM_PKT_ARP,		/* ARP */
	ENUM_PKT_ICMP,		/* ICMP */
	ENUM_PKT_TDLS,		/* TDLS */
	ENUM_PKT_DNS,		/* DNS */
#if CFG_SUPPORT_TPENHANCE_MODE
	ENUM_PKT_TCP_ACK,	/* TCP ACK */
#endif /* CFG_SUPPORT_TPENHANCE_MODE */
	ENUM_PKT_ICMPV6,	/* ICMPV6 */
#if (CFG_IP_FRAG_DISABLE_HW_CHECKSUM == 1)
	ENUM_PKT_IP_FRAG,	/* fragmented IP packet */
#endif
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
	ENUM_PKT_802_11_MGMT,
#endif
	ENUM_PKT_FLAG_NUM
};

struct GL_IO_REQ {
	struct QUE_ENTRY rQueEntry;
	/* wait_queue_head_t       cmdwait_q; */
	u_int8_t fgRead;
	u_int8_t fgWaitResp;
	struct ADAPTER *prAdapter;
	PFN_OID_HANDLER_FUNC pfnOidHandler;
	void *pvInfoBuf;
	uint32_t u4InfoBufLen;
	uint32_t *pu4QryInfoLen;
	uint32_t rStatus;
	uint32_t u4Flag;
	struct CMD_INFO *prCmdInfo;
};

#if CFG_SUPPORT_SCAN_CACHE_RESULT
struct GL_SCAN_CACHE_INFO {
	struct GLUE_INFO *prGlueInfo;
/* TODO: os-related? */
	/* total number of channels to scan */
	uint32_t n_channels;

	/* Scan period time */
	OS_SYSTIME u4LastScanTime;
/* TODO: os-related */
#if 0
	/* for cfg80211 scan done indication */
	struct cfg80211_scan_request *prRequest;
#endif
};
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */

#if CFG_SUPPORT_PERF_IND
struct GL_PERF_IND_INFO {
	uint32_t u4CurTxBytes[MAX_BSSID_NUM]; /* Byte */
	uint32_t u4CurRxBytes[MAX_BSSID_NUM]; /* Byte */
	uint16_t u2CurRxRate[MAX_BSSID_NUM]; /* Unit 500 Kbps */
	uint8_t ucCurRxRCPI0[MAX_BSSID_NUM];
	uint8_t ucCurRxRCPI1[MAX_BSSID_NUM];
	uint8_t ucCurRxNss[MAX_BSSID_NUM]; /* 1NSS Data Counter */
	uint8_t ucCurRxNss2[MAX_BSSID_NUM]; /* 2NSS Data Counter */
};
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */

struct GL_CH_SWITCH_WORK {

};

struct GL_CH_SWITCH_START_WORK {

};

struct FT_IES {
	uint16_t u2MDID;
	struct IE_MOBILITY_DOMAIN *prMDIE;
	struct IE_FAST_TRANSITION *prFTIE;
	struct IE_TIMEOUT_INTERVAL *prTIE;
	struct RSN_INFO_ELEM *prRsnIE;
	uint8_t *pucIEBuf;
	uint32_t u4IeLength;
};

/*
 * type definition of pointer to p2p structure
 */
struct GL_P2P_INFO;	/* declare GL_P2P_INFO_T */
struct GL_P2P_DEV_INFO;	/* declare GL_P2P_DEV_INFO_T */

struct GLUE_INFO {
	/* Pointer to ADAPTER_T - main data structure of internal protocol
	 * stack
	 */
	struct ADAPTER *prAdapter;

	/* OID related */
	struct QUE rCmdQueue;
	/* Number of pending frames, also used for debuging if any frame is
	 * missing during the process of unloading Driver.
	 *
	 * NOTE(Kevin): In Linux, we also use this variable as the threshold
	 * for manipulating the netif_stop(wake)_queue() func.
	 */
	uint32_t u4TxStopTh[MAX_BSSID_NUM];
	uint32_t u4TxStartTh[MAX_BSSID_NUM];
	int32_t ai4TxPendingFrameNumPerQueue[MAX_BSSID_NUM][CFG_MAX_TXQ_NUM];
	int32_t i4TxPendingFrameNum;
	int32_t i4TxPendingCmdNum;

	uint32_t u4RoamFailCnt;
	uint64_t u8RoamFailTime;

	/* 11R */
	struct FT_IES rFtIeR0;
	struct FT_IES rFtIeR1;
	uint32_t IsrAbnormalCnt;
	uint32_t IsrSoftWareCnt;

	/* Indicated media state */
	enum ENUM_PARAM_MEDIA_STATE eParamMediaStateIndicated;

	/* NVRAM availability */
	u_int8_t fgNvramAvailable;

	struct SET_TXPWR_CTRL rTxPwr;

	uint32_t IsrCnt;
	uint32_t IsrPassCnt;
	uint32_t TaskIsrCnt;

	uint32_t IsrTxCnt;
	uint32_t IsrRxCnt;

	/* Tx: for NetDev to BSS index mapping */
	struct NET_INTERFACE_INFO arNetInterfaceInfo[MAX_BSSID_NUM];

	u_int8_t fgTxDoneDelayIsARP;

	uint32_t u4ArriveDrvTick;
	uint32_t u4EnQueTick;
	uint32_t u4DeQueTick;
	uint32_t u4LeaveDrvTick;
	uint32_t u4CurrTick;
	uint64_t u8CurrTime;

	/*
	 * Buffer to hold non-wfa vendor specific IEs set
	 * from wpa_supplicant. This is used in sending
	 * Association Request in AIS mode.
	 */
	uint16_t non_wfa_vendor_ie_len;
	uint8_t non_wfa_vendor_ie_buf[NON_WFA_VENDOR_IE_MAX_LEN];

#if CFG_ENABLE_WIFI_DIRECT
	struct GL_P2P_DEV_INFO *prP2PDevInfo;
	struct GL_P2P_INFO *prP2PInfo[KAL_P2P_NUM];
#endif

	u_int8_t fgIsInSuspendMode;

	/* current IO request for kalIoctl */
	struct GL_IO_REQ OidEntry;

	/* registry info */
	struct REG_INFO rRegInfo;

	/*
	 * needed by
	 * common/cmm_asic_connac.c
	 */
	uint32_t u4InfType;
	/* not necessary for built */
	/* TODO: os-related */
	uint32_t u4ReadyFlag;	/* check if card is ready */

	uint8_t fgIsEnableMon;
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	uint8_t ucPriChannel;
	uint8_t ucChannelS1;
	uint8_t ucChannelS2;
	uint8_t ucBand;
	uint8_t ucChannelWidth;
	uint8_t ucSco;
	uint8_t ucBandIdx;
	uint8_t fgDropFcsErrorFrame;
	uint8_t aucBandIdxEn[CFG_MONITOR_BAND_NUM];
	uint16_t u2Aid;
	uint32_t u4AmpduRefNum[CFG_MONITOR_BAND_NUM];
#endif
#if (CFG_SUPPORT_PERF_IND == 1)
	struct GL_PERF_IND_INFO PerfIndCache;
#endif

#if CFG_SUPPORT_SCHED_SCAN
	struct PARAM_SCHED_SCAN_REQUEST *prSchedScanRequest;
#endif
	kal_completion rPendComp;	/* indicate main thread halt complete */

	unsigned long ulFlag;		/* GLUE_FLAG_XXX */
	unsigned long ulHifFlag;	/* HIF_FLAG_XXX */

	/* Host interface related information */
	/* defined in related hif header file */
	struct GL_HIF_INFO rHifInfo;

#if (CFG_SUPPORT_RETURN_TASK == 1)
	uint32_t rRxRfbRetTask;
#endif
#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
	struct CHANNEL_TIMING_T  rChanTimeRecord[CH_MAX_NUM];
	uint8_t u1NoiseLevel;
#endif

#if CFG_WIFI_TESTMODE_FW_REDOWNLOAD
	u_int8_t fgTestFwDl;
#endif
};

#if 0  /* irq & time in Linux */
typedef irqreturn_t(*PFN_WLANISR) (int irq, void *dev_id,
				   struct pt_regs *regs);

typedef void (*PFN_LINUX_TIMER_FUNC) (unsigned long);
#endif

/* generic sub module init/exit handler
 *   now, we only have one sub module, p2p
 */
#if CFG_ENABLE_WIFI_DIRECT
typedef u_int8_t(*SUB_MODULE_INIT) (struct GLUE_INFO
				    *prGlueInfo);
typedef u_int8_t(*SUB_MODULE_EXIT) (struct GLUE_INFO
				    *prGlueInfo);

struct SUB_MODULE_HANDLER {
	SUB_MODULE_INIT subModInit;
	SUB_MODULE_EXIT subModExit;
	u_int8_t fgIsInited;
};

#endif

#ifdef CONFIG_NL80211_TESTMODE

enum TestModeCmdType {
	TESTMODE_CMD_ID_SW_CMD = 1,
	TESTMODE_CMD_ID_WAPI = 2,
	TESTMODE_CMD_ID_HS20 = 3,

	/* Hotspot managerment testmode command */
	TESTMODE_CMD_ID_HS_CONFIG = 51,

	TESTMODE_CMD_ID_STR_CMD = 102,
	NUM_OF_TESTMODE_CMD_ID
};

#if CFG_SUPPORT_PASSPOINT
enum Hs20CmdType {
	HS20_CMD_ID_SET_BSSID_POOL = 0,
	NUM_OF_HS20_CMD_ID
};
#endif /* CFG_SUPPORT_PASSPOINT */

struct NL80211_DRIVER_TEST_MODE_PARAMS {
	uint32_t index;
	uint32_t buflen;
};

struct NL80211_DRIVER_STRING_CMD_PARAMS {
	struct NL80211_DRIVER_TEST_MODE_PARAMS hdr;
	uint32_t reply_buf_size;
	uint32_t reply_len;
	union _reply_buf {
		uint8_t *ptr;
		uint64_t data;
	} reply_buf;
};

/*SW CMD */
struct NL80211_DRIVER_SW_CMD_PARAMS {
	struct NL80211_DRIVER_TEST_MODE_PARAMS hdr;
	uint8_t set;
	uint32_t adr;
	uint32_t data;
};

struct iw_encode_exts {
	__u32 ext_flags;	/*!< IW_ENCODE_EXT_* */
	__u8 tx_seq[IW_ENCODE_SEQ_MAX_SIZE];	/*!< LSB first */
	__u8 rx_seq[IW_ENCODE_SEQ_MAX_SIZE];	/*!< LSB first */
	/*!< ff:ff:ff:ff:ff:ff for broadcast/multicast
	 *   (group) keys or unicast address for
	 *   individual keys
	 */
	__u8 addr[MAC_ADDR_LEN];
	__u16 alg;		/*!< IW_ENCODE_ALG_* */
	__u16 key_len;
	__u8 key[32];
};

/*SET KEY EXT */
struct NL80211_DRIVER_SET_KEY_EXTS {
	struct NL80211_DRIVER_TEST_MODE_PARAMS hdr;
	uint8_t key_index;
	uint8_t key_len;
	struct iw_encode_exts ext;
};

#if CFG_SUPPORT_PASSPOINT

struct param_hs20_set_bssid_pool {
	uint8_t fgBssidPoolIsEnable;
	uint8_t ucNumBssidPool;
	uint8_t arBssidPool[8][ETH_ALEN];
};

struct wpa_driver_hs20_data_s {
	struct NL80211_DRIVER_TEST_MODE_PARAMS hdr;
	enum Hs20CmdType CmdType;
	struct param_hs20_set_bssid_pool hs20_set_bssid_pool;
};

#endif /* CFG_SUPPORT_PASSPOINT */

#endif

struct NETDEV_PRIVATE_GLUE_INFO {
	struct GLUE_INFO *prGlueInfo;
	uint8_t ucBssIdx;
	u_int8_t ucIsP2p;
	u_int8_t ucMddpSupport;
	uint8_t ucMldBssIdx;
	uint32_t u4OsMgmtFrameFilter;
};

struct PACKET_PRIVATE_DATA {
	/* tx/rx both use cb */
	struct QUE_ENTRY rQueEntry;  /* 16byte total:16 */

	uint8_t ucBssIdx;	/* 1byte */
	/* only rx use cb */
	u_int8_t fgIsIndependentPkt; /* 1byte */
	/* only tx use cb */
	uint8_t ucTid;		/* 1byte */
	uint8_t ucHeaderLen;	/* 1byte */
	uint8_t ucFlag;		/* 1byte */
	uint8_t ucSeqNo;		/* 1byte */
	uint16_t u2Flag;		/* 2byte total:24 */

	uint16_t u2IpId;		/* 2byte */
	uint16_t u2FrameLen;	/* 2byte */
	uint16_t u2Sn;		/* 2byte */
	OS_SYSTIME rArrivalTime;/* 4byte total:32 */

	uint64_t u8ArriveTime;	/* 8byte total:40 */
};

struct PACKET_PRIVATE_RX_DATA {
	uint64_t u8IntTime;	/* 8byte */
	uint64_t u8RxTime;	/* 8byte */
};

struct CMD_CONNSYS_FW_LOG {
	int32_t fgCmd;
	int32_t fgValue;
	u_int8_t fgEarlySet;
};

enum ENUM_NVRAM_STATE {
	NVRAM_STATE_INIT = 0,
	NVRAM_STATE_READY, /*power on or update*/
	NVRAM_STATE_SEND_TO_FW,
	NVRAM_STATE_NUM
};

/* WMM QOS user priority from 802.1D/802.11e */
enum ENUM_WMM_UP {
	WMM_UP_BE_INDEX = 0,
	WMM_UP_BK_INDEX = 1,
	WMM_UP_RESV_INDEX = 2,
	WMM_UP_EE_INDEX = 3,
	WMM_UP_CL_INDEX = 4,
	WMM_UP_VI_INDEX = 5,
	WMM_UP_VO_INDEX = 6,
	WMM_UP_NC_INDEX = 7,
	WMM_UP_INDEX_NUM
};

/* add for wlanSuspendRekeyOffload command because not include nl80211 */
enum nl80211_wpa_versions {
	NL80211_WPA_VERSION_1 = 1 << 0,
	NL80211_WPA_VERSION_2 = 1 << 1,
};

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

#define TYPEOF(__F)

/*----------------------------------------------------------------------------*/
/* Macros of SPIN LOCK operations for using in Glue Layer                     */
/*----------------------------------------------------------------------------*/
/* TODO: we have to lock source another is in kalAcquireSpinLock */
#define GLUE_SPIN_LOCK_DECLARATION()
#define GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, rLockCategory)
#define GLUE_RELEASE_SPIN_LOCK(prGlueInfo, rLockCategory)
/*----------------------------------------------------------------------------*/
/* Macros for accessing Reserved Fields of native packet                      */
/*----------------------------------------------------------------------------*/
/* TODO: os-related, implementation may refer to os/linux */
#define GLUE_GET_PKT_PRIVATE_DATA(_p) ((struct PACKET_PRIVATE_DATA *)0)

#define GLUE_GET_PKT_QUEUE_ENTRY(_p)    \
	    (&(GLUE_GET_PKT_PRIVATE_DATA(_p)->rQueEntry))

/* TODO: os-related implementation */
#define GLUE_GET_PKT_DESCRIPTOR(_prQueueEntry) ((struct PACKET_PRIVATE_DATA *)0)

#define GLUE_SET_PKT_TID(_p, _tid) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucTid = (uint8_t)(_tid))

#define GLUE_GET_PKT_TID(_p) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucTid)

#define GLUE_SET_PKT_FLAG(_p, _flag) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Flag |= BIT(_flag))

#define GLUE_TEST_PKT_FLAG(_p, _flag) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Flag & BIT(_flag))

#define GLUE_IS_PKT_FLAG_SET(_p) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Flag)

#define GLUE_SET_PKT_BSS_IDX(_p, _ucBssIndex) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucBssIdx = (uint8_t)(_ucBssIndex))

#define GLUE_GET_PKT_BSS_IDX(_p) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucBssIdx)

#define GLUE_SET_PKT_HEADER_LEN(_p, _ucMacHeaderLen) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucHeaderLen = \
	    (uint8_t)(_ucMacHeaderLen))

#define GLUE_GET_PKT_HEADER_LEN(_p) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucHeaderLen)

#define GLUE_SET_PKT_FRAME_LEN(_p, _u2PayloadLen) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u2FrameLen = (uint16_t)(_u2PayloadLen))

#define GLUE_GET_PKT_FRAME_LEN(_p) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2FrameLen)

#define GLUE_SET_PKT_ARRIVAL_TIME(_p, _rSysTime) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->rArrivalTime = (OS_SYSTIME)(_rSysTime))

#define GLUE_GET_PKT_ARRIVAL_TIME(_p)    \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->rArrivalTime)

#define GLUE_SET_PKT_IP_ID(_p, _u2IpId) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u2IpId = (uint16_t)(_u2IpId))

#define GLUE_GET_PKT_IP_ID(_p) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->u2IpId)

#define GLUE_SET_PKT_SEQ_NO(_p, _ucSeqNo) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->ucSeqNo = (uint8_t)(_ucSeqNo))

#define GLUE_GET_PKT_SEQ_NO(_p) \
	    (GLUE_GET_PKT_PRIVATE_DATA(_p)->ucSeqNo)

#define GLUE_SET_PKT_FLAG_PROF_MET(_p) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->ucFlag |= BIT(0))

#define GLUE_GET_PKT_IS_PROF_MET(_p) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->ucFlag & BIT(0))

#define GLUE_SET_PKT_CONTROL_PORT_TX(_p) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->ucFlag |= BIT(1))

#define GLUE_GET_PKT_IS_CONTROL_PORT_TX(_p) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->ucFlag & BIT(1))

#define GLUE_SET_PKT_SN_VALID(_p) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->ucFlag |= BIT(2))

#define GLUE_GET_PKT_IS_SN_VALID(_p) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->ucFlag & BIT(2))

#define GLUE_SET_PKT_XTIME(_p, _rSysTime) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u8ArriveTime = (uint64_t)(_rSysTime))

#define GLUE_GET_PKT_XTIME(_p)    \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u8ArriveTime)

/* TODO: os-related implementation */
#define GLUE_GET_PKT_PRIVATE_RX_DATA(_p) \
	((struct PACKET_PRIVATE_RX_DATA *)0)

#define GLUE_RX_SET_PKT_INT_TIME(_p, _rTime) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8IntTime = (uint64_t)(_rTime))

#define GLUE_RX_GET_PKT_INT_TIME(_p) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8IntTime)

#define GLUE_RX_SET_PKT_RX_TIME(_p, _rTime) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8RxTime = (uint64_t)(_rTime))

#define GLUE_RX_GET_PKT_RX_TIME(_p) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8RxTime)

#define GLUE_SET_PKT_SN(_p, _rSn) \
	do { \
		GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Sn = (uint16_t)(_rSn); \
		GLUE_SET_PKT_SN_VALID(_p); \
	} while (0)

#define GLUE_GET_PKT_SN(_p)    \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->u2Sn)

/* TODO: os-related implementation */
#define GLUE_GET_TX_PKT_ETHER_DEST_ADDR(_p)
#define GLUE_GET_TX_PKT_ETHER_SRC_ADDR(_p)

/* Check validity of prDev, private data, and pointers */
/* TODO: os-related implementation */
#define GLUE_CHK_DEV(prDev)

#define GLUE_CHK_PR2(prDev, pr2) \
	((GLUE_CHK_DEV(prDev) && pr2) ? TRUE : FALSE)

#define GLUE_CHK_PR3(prDev, pr2, pr3) \
	((GLUE_CHK_PR2(prDev, pr2) && pr3) ? TRUE : FALSE)

#define GLUE_CHK_PR4(prDev, pr2, pr3, pr4) \
	((GLUE_CHK_PR3(prDev, pr2, pr3) && pr4) ? TRUE : FALSE)

#define GLUE_SET_EVENT(pr) \
	kalSetEvent(pr)
/* TODO: os-related implementation */
#define GLUE_INC_REF_CNT(_refCount)     (_refCount++)
#define GLUE_DEC_REF_CNT(_refCount)     (_refCount--)
#define GLUE_ADD_REF_CNT(_value, _refCount) \
	(_refCount += _value)
#define GLUE_SUB_REF_CNT(_value, _refCount) \
	(_refCount -= _value)
#define GLUE_GET_REF_CNT(_refCount)     (_refCount)

#define DbgPrint(...)

#define GLUE_LOOKUP_FUN(fun_name) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define GLUE_COPY_PRIV_DATA(_pDst, _pSrc) \
	(kalMemCopy(GLUE_GET_PKT_PRIVATE_DATA(_pDst), \
	GLUE_GET_PKT_PRIVATE_DATA(_pSrc), sizeof(struct PACKET_PRIVATE_DATA)))

#define GLUE_GET_INDEPENDENT_PKT(_p)    \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->fgIsIndependentPkt)

#define GLUE_SET_INDEPENDENT_PKT(_p, _fgIsIndePkt) \
	(GLUE_GET_PKT_PRIVATE_DATA(_p)->fgIsIndependentPkt = _fgIsIndePkt)

/*----------------------------------------------------------------------------*/
/* Macros of Data Type Check                                                  */
/*----------------------------------------------------------------------------*/
/* Kevin: we don't have to call following function to inspect the data
 * structure.
 * It will check automatically while at compile time.
 */
/* TODO: os-related, API implementation, may refer to Linux */
#define glPacketDataTypeCheck()

#define mtk_wlan_ndev_select_queue(_prNetdev, _prSkb, _prFallback)

#define netdev_for_each_mc_addr(mclist, dev)

#define GET_ADDR(ha)

#define LIST_FOR_EACH_IPV6_ADDR(_prIfa, _ip6_ptr)

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if WLAN_INCLUDE_PROC
int32_t procCreateFsEntry(struct GLUE_INFO *prGlueInfo);
int32_t procRemoveProcfs(struct GLUE_INFO *prGlueInfo);


int32_t procInitFs(struct GLUE_INFO *prGlueInfo);
int32_t procUninitProcFs(void);



/* TODO: os-related, API implementation, may refer to Linux */
#define procInitProcfs(_prDev, _pucDevName)
#endif /* WLAN_INCLUDE_PROC */

struct GLUE_INFO *wlanGetGlueInfo(void);
struct GLUE_INFO *wlanGetGlueInfoByWiphy(struct wiphy *wiphy);

#ifdef CFG_REMIND_IMPLEMENT
#define wlanSelectQueue(_dev, _skb) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDebugInit() \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb);

void wlanDebugInit(void);
#endif

uint32_t wlanSetDriverDbgLevel(uint32_t u4DbgIdx,
			       uint32_t u4DbgMask);

uint32_t wlanGetDriverDbgLevel(uint32_t u4DbgIdx,
			       uint32_t *pu4DbgMask);

void wlanSetSuspendMode(struct GLUE_INFO *prGlueInfo,
			u_int8_t fgEnable);

#ifdef CFG_REMIND_IMPLEMENT
#define wlanGetConfig(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void wlanGetConfig(struct ADAPTER *prAdapter);
#endif

#if CFG_SUPPORT_MULTI_CARD
uint32_t wlanGetDevConfig(struct ADAPTER *prAdapter);
#endif

uint32_t wlanDownloadBufferBin(struct ADAPTER *prAdapter);

uint32_t wlanConnacDownloadBufferBin(struct ADAPTER
				     *prAdapter);

uint32_t wlanConnac2XDownloadBufferBin(struct ADAPTER *prAdapter);

uint32_t wlanConnac3XDownloadBufferBin(struct ADAPTER *prAdapter);

void *wlanGetAisNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucAisIndex);

void *wlanGetP2pNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucP2pIndex);

void *wlanGetNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex);

/*******************************************************************************
 *			 E X T E R N A L   F U N C T I O N S / V A R I A B L E
 *******************************************************************************
 */
#if (CFG_SUPPORT_CONNAC3X == 1 && CFG_SUPPORT_UPSTREAM_TOOL == 1)
extern struct wireless_dev *gprWdev[KAL_AIS_NUM];
#endif
extern enum ENUM_NVRAM_STATE g_NvramFsm;

extern uint8_t g_aucNvram[];
extern uint8_t g_aucNvram_OnlyPreCal[];

#ifdef CFG_DRIVER_INF_NAME_CHANGE
extern char *gprifnameap;
extern char *gprifnamep2p;
extern char *gprifnamesta;
#endif /* CFG_DRIVER_INF_NAME_CHANGE */

extern void wlanRegisterNotifier(void);
extern void wlanUnregisterNotifier(void);
#if CFG_MTK_ANDROID_WMT
typedef int (*set_p2p_mode) (struct net_device *netdev,
			     struct PARAM_CUSTOM_P2P_SET_STRUCT p2pmode);
extern void register_set_p2p_mode_handler(
	set_p2p_mode handler);
#endif

#if CFG_ENABLE_EARLY_SUSPEND
extern int glRegisterEarlySuspend(struct early_suspend
				  *prDesc,
				  early_suspend_callback wlanSuspend,
				  late_resume_callback wlanResume);

extern int glUnregisterEarlySuspend(struct early_suspend
				    *prDesc);
#endif

extern const uint8_t *kalFindIeMatchMask(uint8_t eid,
				const uint8_t *ies, int len,
				const uint8_t *match,
				int match_len, int match_offset,
				const uint8_t *match_mask);

extern const uint8_t *kalFindVendorIe(uint32_t oui, int type,
				const uint8_t *ies, int len);

extern const uint8_t *kalFindIeExtIE(uint8_t eid,
				uint8_t exteid,
				const uint8_t *ies, int len);

#ifdef CFG_REMIND_IMPLEMENT
#define wlanUpdateChannelTable(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void wlanUpdateChannelTable(struct GLUE_INFO *prGlueInfo);
#endif

const struct net_device_ops *wlanGetNdevOps(void);

#if CFG_MTK_ANDROID_WMT
extern void connectivity_flush_dcache_area(void *addr, size_t len);
extern void connectivity_arch_setup_dma_ops(
	struct device *dev, u64 dma_base,
	u64 size, struct iommu_ops *iommu,
	bool coherent);
#endif

#define wlanHardStartXmit(_prSkb, _prDev)

#ifndef ARRAY_SIZE
#define IS_ARRAY(arr) ((void *)&(arr) == &(arr)[0])
#define STATIC_EXP(e) (0 * sizeof(struct {int ARRAY_SIZE_FAILED:(2*(e) - 1); }))
#define ARRAY_SIZE(D) (sizeof(D) / sizeof((D)[0]) + STATIC_EXP(IS_ARRAY(D)))
#endif

#define wlanNvramSetState(_state) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#ifdef CFG_REMIND_IMPLEMENT
#define wlanDfsChannelsReqInit(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDfsChannelsReqDeInit(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDfsChannelsReqDump(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDfsChannelsReqAdd(_prAdapter, _eSource, _ucChannel, \
		_ucBandWidth, _eBssSCO, _u4CenterFreq, _eBand) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDfsChannelsReqDel(_prAdapter, _eSource) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDfsChannelsNotifyStaConnected(_prAdapter, _ucAisIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDfsChannelsNotifyStaDisconnected(_prAdapter, _ucAisIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define wlanDfsChannelsAllowdBySta(_prAdapter, _prRfChnlInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t wlanDfsChannelsReqInit(struct ADAPTER *prAdapter);
void wlanDfsChannelsReqDeInit(struct ADAPTER *prAdapter);
void wlanDfsChannelsReqDump(struct ADAPTER *prAdapter);
uint32_t wlanDfsChannelsReqAdd(struct ADAPTER *prAdapter,
	enum DFS_CHANNEL_CTRL_SOURCE eSource,
	uint8_t ucChannel, uint8_t ucBandWidth,
	enum ENUM_CHNL_EXT eBssSCO, uint32_t u4CenterFreq,
	enum ENUM_BAND eBand);
void wlanDfsChannelsReqDel(struct ADAPTER *prAdapter,
	enum DFS_CHANNEL_CTRL_SOURCE eSource);
uint32_t wlanDfsChannelsNotifyStaConnected(struct ADAPTER *prAdapter,
	uint8_t ucAisIndex);
void wlanDfsChannelsNotifyStaDisconnected(struct ADAPTER *prAdapter,
	uint8_t ucAisIndex);
u_int8_t wlanDfsChannelsAllowdBySta(struct ADAPTER *prAdapter,
	struct RF_CHANNEL_INFO *prRfChnlInfo);
#endif

#endif /* _GL_OS_H */
