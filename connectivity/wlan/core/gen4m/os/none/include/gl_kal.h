/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_kal.h
 *  \brief  Declaration of KAL functions - kal*() which is provided
 *          by GLUE Layer.
 *
 *    Any definitions in this file will be shared among GLUE Layer
 *    and internal Driver Stack.
 */

#ifndef _GL_KAL_H
#define _GL_KAL_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "config.h"
#include "gl_os.h"
#include "gl_typedef.h"
#include "gl_wext_priv.h"
#include "link_drv.h"
#include "nic/mac.h"
#include "nic/wlan_def.h"
#include "wlan_lib.h"
#include "wlan_oid.h"

#if CFG_ENABLE_BT_OVER_WIFI
#include "nic/bow.h"
#endif

#if DBG
extern int allocatedMemSize;
#endif

extern int g_u4HaltFlag;
extern int g_u4WlanInitFlag;

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* Define how many concurrent operation networks. */
#define KAL_AIS_NUM           1
#define KAL_P2P_NUM	      2

/* Threading */
#if CFG_SUPPORT_MULTITHREAD
#define GLUE_FLAG_MAIN_PROCESS \
	(GLUE_FLAG_HALT | GLUE_FLAG_SUB_MOD_MULTICAST | \
	GLUE_FLAG_TX_CMD_DONE | GLUE_FLAG_TXREQ | GLUE_FLAG_TIMEOUT | \
	GLUE_FLAG_FRAME_FILTER | GLUE_FLAG_OID | GLUE_FLAG_RX)

#define GLUE_FLAG_HIF_PROCESS \
	(GLUE_FLAG_HALT | GLUE_FLAG_INT | GLUE_FLAG_HIF_TX | \
	GLUE_FLAG_HIF_TX_CMD | GLUE_FLAG_HIF_FW_OWN | \
	GLUE_FLAG_HIF_PRT_HIF_DBG_INFO | \
	GLUE_FLAG_UPDATE_WMM_QUOTA | \
	GLUE_FLAG_HIF_MDDP | \
	GLUE_FLAG_BT_DUMP_VIA_WIFI)

#define HIF_FLAG \
	(HIF_FLAG_AER_RESET | HIF_FLAG_MSI_RECOVERY | \
	HIF_FLAG_ALL_TOKENS_UNUSED)

#define GLUE_FLAG_RX_PROCESS (GLUE_FLAG_HALT | GLUE_FLAG_RX_TO_OS)
#else
/* All flags for single thread driver */
#define GLUE_FLAG_MAIN_PROCESS  0xFFFFFFFF
#endif

/* performance monitor feature */
#define PERF_MON_INIT_BIT       (0)
#define PERF_MON_DISABLE_BIT    (1)
#define PERF_MON_STOP_BIT       (2)
#define PERF_MON_RUNNING_BIT    (3)

#define PERF_MON_UPDATE_INTERVAL (1000)
#define PERF_MON_TP_MAX_THRESHOLD (10)

#define KAL_TRACE __builtin_return_address(0)

#if (CFG_COALESCING_INTERRUPT == 1)
#define COALESCING_INT_MAX_TIME (1) /* ms */
#define COALESCING_INT_MAX_PKT (50)
#endif

#define OID_HDLR_REC_NUM	5

/* By wifi.cfg first. If it is not set 1s by default; 100ms on more. */
#define TX_LATENCY_STATS_UPDATE_INTERVAL (0)
#define TX_LATENCY_STATS_CONTINUOUS_FAIL_THREHOLD (10)

#define TX_LATENCY_STATS_MAX_DRIVER_DELAY_L1 (1)
#define TX_LATENCY_STATS_MAX_DRIVER_DELAY_L2 (5)
#define TX_LATENCY_STATS_MAX_DRIVER_DELAY_L3 (10)
#define TX_LATENCY_STATS_MAX_DRIVER_DELAY_L4 (20)

#define TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L1 (1)
#define TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L2 (5)
#define TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L3 (10)
#define TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L4 (20)

#define TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L1 (10)
#define TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L2 (20)
#define TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L3 (50)
#define TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L4 (80)

#define TX_LATENCY_STATS_MAX_MAC_DELAY_L1 (5)
#define TX_LATENCY_STATS_MAX_MAC_DELAY_L2 (10)
#define TX_LATENCY_STATS_MAX_MAC_DELAY_L3 (20)
#define TX_LATENCY_STATS_MAX_MAC_DELAY_L4 (50)

#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L1 (10)
#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L2 (20)
#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L3 (50)
#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L4 (80)

#ifndef UINT_MAX
#define UINT_MAX	(~0U)
#endif

#define NETIF_F_IP_CSUM 0
#define NETIF_F_IPV6_CSUM 0
#define KAL_SCN_BSS_DESC_STALE_SEC			20

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* Lock for process */
enum ENUM_SPIN_LOCK_CATEGORY_E {
	SPIN_LOCK_FSM = 0,

#if CFG_SUPPORT_MULTITHREAD
	SPIN_LOCK_TX_PORT_QUE,
	SPIN_LOCK_TX_CMD_QUE,
	SPIN_LOCK_TX_CMD_DONE_QUE,
	SPIN_LOCK_TC_RESOURCE,
	SPIN_LOCK_RX_TO_OS_QUE,
#endif

	/* FIX ME */
	SPIN_LOCK_RX_QUE,
	SPIN_LOCK_RX_FREE_QUE,
	SPIN_LOCK_TX_QUE,
	SPIN_LOCK_CMD_QUE,
	SPIN_LOCK_TX_RESOURCE,
	SPIN_LOCK_CMD_RESOURCE,
	SPIN_LOCK_QM_TX_QUEUE,
	SPIN_LOCK_CMD_PENDING,
	SPIN_LOCK_CMD_SEQ_NUM,
	SPIN_LOCK_TX_MSDU_INFO_LIST,
	SPIN_LOCK_TXING_MGMT_LIST,
	SPIN_LOCK_TX_SEQ_NUM,
	SPIN_LOCK_TX_COUNT,
	SPIN_LOCK_TXS_COUNT,
	/* end    */
	SPIN_LOCK_TX,
	/* TX/RX Direct : BEGIN */
	SPIN_LOCK_TX_DIRECT,
	SPIN_LOCK_RX_DIRECT,
	SPIN_LOCK_RX_DIRECT_REORDER,
	/* TX/RX Direct : END */

	/* RX: main vs. NAPI */
	SPIN_LOCK_RX_FLUSH_TIMEOUT,
	SPIN_LOCK_RX_FLUSH_BA,
	SPIN_LOCK_RX_TO_NAPI,

#if CFG_QUEUE_RX_IF_CONN_NOT_READY
	SPIN_LOCK_RX_PENDING,
#endif /* CFG_QUEUE_RX_IF_CONN_NOT_READY */

	SPIN_LOCK_IO_REQ,
	SPIN_LOCK_INT,
	SPIN_LOCK_UPDATE_WMM_QUOTA,

	SPIN_LOCK_MGT_BUF,
	SPIN_LOCK_MSG_BUF,
	SPIN_LOCK_STA_REC,

	SPIN_LOCK_MAILBOX,
	SPIN_LOCK_TIMER,

	/* SPIN_LOCK_BOW_TABLE,*/

	SPIN_LOCK_EHPI_BUS,	/* only for EHPI */
	SPIN_LOCK_NET_DEV,
	SPIN_LOCK_BSSLIST_FW,
	SPIN_LOCK_BSSLIST_CFG,
#if CFG_CHIP_RESET_SUPPORT
	SPIN_LOCK_WFSYS_RESET,
#endif
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	SPIN_LOCK_TX_MGMT_DIRECT_Q,
#endif
	SPIN_LOCK_HIF_REMAP,
	SPIN_LOCK_PMKID,
	SPIN_LOCK_MSDUIFO,
	SPIN_LOCK_NUM
};

enum ENUM_MUTEX_CATEGORY_E {
	MUTEX_TX_CMD_CLEAR,
	MUTEX_TX_DATA_DONE_QUE,
	MUTEX_DEL_INF,
	MUTEX_CHIP_RST,
	MUTEX_SET_OWN,
	MUTEX_BOOST_CPU,
	MUTEX_CSI_BUFFER,
	MUTEX_CSI_STA_LIST,
	MUTEX_FW_LOG,
	MUTEX_WF_VOTE,
	MUTEX_NUM
};

/* event for assoc information update */
struct EVENT_ASSOC_INFO {
	uint8_t ucAssocReq;	/* 1 for assoc req, 0 for assoc rsp */
	uint8_t ucReassoc;	/* 0 for assoc, 1 for reassoc */
	uint16_t u2Length;
	uint8_t *pucIe;
};

/* network type */
enum ENUM_KAL_NETWORK_TYPE_INDEX {
	KAL_NETWORK_TYPE_AIS_INDEX = 0,
#if CFG_ENABLE_WIFI_DIRECT
	KAL_NETWORK_TYPE_P2P_INDEX,
#endif
#if CFG_ENABLE_BT_OVER_WIFI
	KAL_NETWORK_TYPE_BOW_INDEX,
#endif
	KAL_NETWORK_TYPE_INDEX_NUM
};

/* malloc type */
enum ENUM_KAL_MEM_ALLOCATION_TYPE_E {
	PHY_MEM_TYPE,		/* physically continuous */
	VIR_MEM_TYPE,		/* virtually continuous */
	MEM_TYPE_NUM
};

/* suspend/resume related */
#define KAL_WAKE_LOCK_T uint32_t

/* TODO: unknown feature */
#if CFG_SUPPORT_AGPS_ASSIST
enum ENUM_MTK_AGPS_ATTR {
	MTK_ATTR_AGPS_INVALID,
	MTK_ATTR_AGPS_CMD,
	MTK_ATTR_AGPS_DATA,
	MTK_ATTR_AGPS_IFINDEX,
	MTK_ATTR_AGPS_IFNAME,
	MTK_ATTR_AGPS_MAX
};

enum ENUM_AGPS_EVENT {
	AGPS_EVENT_WLAN_ON,
	AGPS_EVENT_WLAN_OFF,
	AGPS_EVENT_WLAN_AP_LIST,
};
u_int8_t kalIndicateAgpsNotify(struct ADAPTER *prAdapter,
			       uint8_t cmd,
			       uint8_t *data, uint16_t dataLen);
#endif /* CFG_SUPPORT_AGPS_ASSIST */

/* driver halt */
struct KAL_HALT_CTRL_T {
/* TODO: os-related */
#if 0
	struct semaphore lock;
	struct task_struct *owner;
#endif
	u_int8_t fgHalt;
	u_int8_t fgHeldByKalIoctl;
	OS_SYSTIME u4HoldStart;
};

struct KAL_THREAD_SCHEDSTATS {
	/* when marked: the profiling start time(ms),
	 * when unmarked: total duration(ms)
	 */
	unsigned long long time;
	/* time spent in exec (sum_exec_runtime) */
	unsigned long long exec;
	/* time spent in run-queue while not being scheduled (wait_sum) */
	unsigned long long runnable;
	/* time spent waiting for I/O (iowait_sum) */
	unsigned long long iowait;
};

enum ENUM_CMD_TX_RESULT {
	CMD_TX_RESULT_SUCCESS,
	CMD_TX_RESULT_FAILED,
	CMD_TX_RESULT_QUEUED,
	CMD_TX_RESULT_NUM
};

#if CFG_SUPPORT_DISABLE_DATA_DDONE_INTR
enum ENUM_PKT_PATH {
	PKT_PATH_TX = 0,
	PKT_PATH_RX,
	PKT_PATH_ALL
};
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR */

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
/* os-related: need implementation */
#define KAL_SET_BIT(bitOffset, value)      kal_set_bit(bitOffset, &value)
#define KAL_CLR_BIT(bitOffset, value)      kal_clear_bit(bitOffset, &value)
#define KAL_TEST_AND_CLEAR_BIT(bitOffset, value)  \
	kal_test_and_clear_bit(bitOffset, &value)
#define KAL_TEST_BIT(bitOffset, value)     kal_test_bit(bitOffset, &value)
#define SUSPEND_FLAG_FOR_WAKEUP_REASON	(0)
#define SUSPEND_FLAG_CLEAR_WHEN_RESUME	(1)
#define KAL_HEX_DUMP_TO_BUFFER(_buf, _len, _rowsize, _groupsize, _linebuf, \
	_linebuflen, _ascii) \
	kal_hex_dump_to_buffer(_buf, _len, _rowsize, _groupsize, _linebuf, \
	_linebuflen, _ascii)

#define KAL_WARN_ON(_condition) kal_warn_on(_condition)

#define KAL_IS_ERR(_ptr) kal_is_err(_ptr)
#define KAL_MIN(_a, _b) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/*----------------------------------------------------------------------------*/
/* Macros of getting current thread id                                        */
/*----------------------------------------------------------------------------*/
/* TODO: os-related: need implementation */
#define KAL_GET_CURRENT_THREAD_ID() (0)
#define KAL_GET_CURRENT_THREAD_NAME() ("n/a")

/*----------------------------------------------------------------------------*/
/* Macros of SPIN LOCK operations for using in Driver Layer                   */
/*----------------------------------------------------------------------------*/
#define KAL_SPIN_LOCK_DECLARATION()             unsigned long __ulFlags

#define KAL_ACQUIRE_SPIN_LOCK(_prAdapter, _rLockCategory)   \
	kalAcquireSpinLock(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
	_rLockCategory, &__ulFlags)

#define KAL_RELEASE_SPIN_LOCK(_prAdapter, _rLockCategory)   \
	kalReleaseSpinLock(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
	_rLockCategory, __ulFlags)

#if defined(_HIF_USB)
#define KAL_HIF_STATE_LOCK(prGlueInfo) \
	kalAcquiretHifStateLock(prGlueInfo, &__ulFlags)

#define KAL_HIF_STATE_UNLOCK(prGlueInfo) \
	kalReleaseHifStateLock(prGlueInfo, __ulFlags)
#endif

#define KAL_ACQUIRE_SPIN_LOCK_BH(_prAdapter, _rLockCategory)   \
	kalAcquireSpinLockBh(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
		_rLockCategory)

#define KAL_RELEASE_SPIN_LOCK_BH(_prAdapter, _rLockCategory)   \
	kalReleaseSpinLockBh(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
		_rLockCategory)

#define KAL_ACQUIRE_SPIN_LOCK_IRQ(_prAdapter, _rLockCategory)   \
	kalAcquireSpinLockIrq(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
		_rLockCategory, &__ulFlags)

#define KAL_RELEASE_SPIN_LOCK_IRQ(_prAdapter, _rLockCategory)   \
	kalReleaseSpinLockIrq(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
		_rLockCategory, __ulFlags)
/*----------------------------------------------------------------------------*/
/* Macros of MUTEX operations for using in Driver Layer                   */
/*----------------------------------------------------------------------------*/
#define KAL_ACQUIRE_MUTEX(_prAdapter, _rLockCategory)   \
	kalAcquireMutex(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
	_rLockCategory)

#define KAL_RELEASE_MUTEX(_prAdapter, _rLockCategory)   \
	kalReleaseMutex(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
	_rLockCategory)

/*----------------------------------------------------------------------------*/
/* Macros for accessing Reserved Fields of native packet                      */
/*----------------------------------------------------------------------------*/
#define KAL_GET_PKT_QUEUE_ENTRY(_p)             GLUE_GET_PKT_QUEUE_ENTRY(_p)
#define KAL_GET_PKT_DESCRIPTOR(_prQueueEntry)  \
	GLUE_GET_PKT_DESCRIPTOR(_prQueueEntry)
#define KAL_GET_PKT_TID(_p)                     GLUE_GET_PKT_TID(_p)
#define KAL_GET_PKT_IS1X(_p)                    GLUE_GET_PKT_IS1X(_p)
#define KAL_GET_PKT_HEADER_LEN(_p)              GLUE_GET_PKT_HEADER_LEN(_p)
#define KAL_GET_PKT_PAYLOAD_LEN(_p)             GLUE_GET_PKT_PAYLOAD_LEN(_p)
#define KAL_GET_PKT_ARRIVAL_TIME(_p)            GLUE_GET_PKT_ARRIVAL_TIME(_p)

/*----------------------------------------------------------------------------*/
/* Macros for kernel related defines                      */
/*----------------------------------------------------------------------------*/
/* TODO: os-related: need implementation */
#define IEEE80211_CHAN_PASSIVE_FLAG	(0)
#define IEEE80211_CHAN_PASSIVE_STR		"PASSIVE"

/**
 * enum nl80211_band - Frequency band
 * @NL80211_BAND_2GHZ: 2.4 GHz ISM band
 * @NL80211_BAND_5GHZ: around 5 GHz band (4.9 - 5.7 GHz)
 * @NL80211_BAND_60GHZ: around 60 GHz band (58.32 - 64.80 GHz)
 * @NUM_NL80211_BANDS: number of bands, avoid using this in userspace
 *	 since newer kernel versions may support more bands
 */
#define KAL_BAND_2GHZ (0)
#define KAL_BAND_5GHZ (1)
#define KAL_BAND_60GHZ (2)
#define KAL_BAND_6GHZ (3)
#define KAL_NUM_BANDS (4)

#if CFG_SUPPORT_DATA_STALL
#define REPORT_EVENT_INTERVAL		30
#define EVENT_PER_HIGH_THRESHOLD	80
#define EVENT_TX_LOW_RATE_THRESHOLD	20
#define EVENT_RX_LOW_RATE_THRESHOLD	20
#define TRAFFIC_RHRESHOLD	150

#define LOW_RATE_MONITOR_INTERVAL	5
#define LOW_RATE_MONITOR_THRESHOLD	20
#define LOW_RATE_MONITOR_TPUT_THRESHOLD	10
#define LOW_RATE_MONITOR_MPDU_THRESHOLD	16
#define LOW_RATE_MONITOR_EVENT_REPORT_INTERVAL	5

enum ENUM_VENDOR_DRIVER_EVENT {
	EVENT_TEST_MODE,
	EVENT_ARP_NO_RESPONSE,
	EVENT_PER_HIGH,
	EVENT_TX_LOW_RATE,
	EVENT_RX_LOW_RATE,
	EVENT_SG_DISABLE,
	EVENT_SG_1T1R,
	EVENT_SG_2T2R
};
#endif

/**
 * enum nl80211_reg_rule_flags - regulatory rule flags
 * @NL80211_RRF_NO_OFDM: OFDM modulation not allowed
 * @NL80211_RRF_AUTO_BW: maximum available bandwidth should be calculated
 *  base on contiguous rules and wider channels will be allowed to cross
 *  multiple contiguous/overlapping frequency ranges.
 * @NL80211_RRF_DFS: DFS support is required to be used
 */
#define KAL_RRF_NO_OFDM (0)
#define KAL_RRF_DFS     (1)
#define KAL_RRF_AUTO_BW 0

/**
 * kalCfg80211ScanDone - abstraction of cfg80211_scan_done
 *
 * @request: the corresponding scan request (sanity checked by callers!)
 * @aborted: set to true if the scan was aborted for any reason,
 *	userspace will be notified of that
 *
 * Since linux-4.8.y the 2nd parameter is changed from bool to
 * struct cfg80211_scan_info, but we don't use all fields yet.
 */
#define kalCfg80211ScanDone(_request, aborted)

/**
 * kalCfg80211VendorEventAlloc - abstraction of cfg80211_vendor_event_alloc
 * cfg80211_vendor_event_alloc - allocate vendor-specific event skb
 * @wiphy: the wiphy
 * @event_idx: index of the vendor event in the wiphy's vendor_events
 * @approxlen: an upper bound of the length of the data that will
 *	be put into the skb
 * @gfp: allocation flags
 *
 * This function allocates and pre-fills an skb for an event on the
 * vendor-specific multicast group.
 *
 * When done filling the skb, call cfg80211_vendor_event() with the
 * skb to send the event.
 *
 * Return: An allocated and pre-filled skb. %NULL if any errors happen.
 *
 * Since linux-4.1.0 the 2nd parameter is added struct wireless_dev
 */
#define kalCfg80211VendorEventAlloc(wiphy, wdev, approxlen, event_idx, gfp) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalCfg80211VendorEvent(pvPacket) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/* Consider on some Android platform, using request_firmware_direct()
 * may cause system failed to load firmware. So we still use
 * request_firmware().
 */
#define REQUEST_FIRMWARE(_fw, _name, _dev)

/*----------------------------------------------------------------------------*/
/* Macros of wake_lock operations for using in Driver Layer                   */
/*----------------------------------------------------------------------------*/
#define KAL_WAKE_LOCK_INIT(_prAdapter, _prWakeLock, _pcName)
#define KAL_WAKE_LOCK_DESTROY(_prAdapter, _prWakeLock)
#define KAL_WAKE_LOCK(_prAdapter, _prWakeLock)
#define KAL_WAKE_LOCK_TIMEOUT(_prAdapter, _prWakeLock, _u4Timeout)
#define KAL_WAKE_UNLOCK(_prAdapter, _prWakeLock)
#define KAL_WAKE_LOCK_ACTIVE(_prAdapter, _prWakeLock)

/*----------------------------------------------------------------------------*/
/*!
 * \brief Cache memory allocation
 *
 * \param[in] u4Size Required memory size.
 * \param[in] eMemType  Memory allocation type
 *
 * \return Pointer to allocated memory
 *         or NULL
 */
/*----------------------------------------------------------------------------*/
#define kmalloc(_size, _type) kal_kmalloc(_size, _type)
#define vmalloc(_size) kal_vmalloc(_size)
#define in_interrupt() (FALSE)
#define kfree(_addr) kal_kfree(_addr)
#define vfree(_addr) kal_vfree(_addr)

#define KAL_MB_W() \
({ \
	/* Avoid memory barrier problem  */ \
	/* TODO: OS-dependent */ \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__); \
})

#if DBG
#define kalMemAlloc(u4Size, eMemType) ({    \
	void *pvAddr; \
	if (eMemType == PHY_MEM_TYPE) { \
		if (in_interrupt()) \
			pvAddr = kmalloc(u4Size, GFP_ATOMIC);   \
		else \
			pvAddr = kmalloc(u4Size, GFP_KERNEL);   \
	} \
	else { \
		pvAddr = vmalloc(u4Size);   \
	} \
	if (pvAddr) {   \
		allocatedMemSize += u4Size;   \
		DBGLOG(INIT, INFO, "0x%p(%ld) allocated (%s:%s)\n", \
		    pvAddr, (uint32_t)u4Size, __FILE__, __func__);  \
	}   \
	pvAddr; \
})
#else
#define kalMemAlloc(u4Size, eMemType) ({    \
	void *pvAddr; \
	if (eMemType == PHY_MEM_TYPE) { \
		if (in_interrupt()) \
			pvAddr = kmalloc(u4Size, GFP_ATOMIC);   \
		else \
			pvAddr = kmalloc(u4Size, GFP_KERNEL);   \
	} \
	else { \
		pvAddr = vmalloc(u4Size);   \
	} \
	if (!pvAddr) \
		ASSERT_NOMEM(); \
	pvAddr; \
})
#endif

#define kalMemZAlloc(u4Size, eMemType) ({    \
	void *pvAddr; \
	pvAddr = kalMemAlloc(u4Size, eMemType); \
	if (pvAddr) \
		kalMemSet(pvAddr, 0, u4Size); \
	pvAddr; \
})

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free allocated cache memory
 *
 * \param[in] pvAddr Required memory size.
 * \param[in] eMemType  Memory allocation type
 * \param[in] u4Size Allocated memory size.
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
#if DBG
#define kalMemFree(pvAddr, eMemType, u4Size)  \
{   \
	if (pvAddr) {   \
		allocatedMemSize -= u4Size; \
		DBGLOG(INIT, INFO, "0x%p(%ld) freed (%s:%s)\n", \
			pvAddr, (uint32_t)u4Size, __FILE__, __func__);  \
	}   \
	if (eMemType == PHY_MEM_TYPE) { \
		kfree(pvAddr); \
	} \
	else { \
		vfree(pvAddr); \
	} \
}
#else
#define kalMemFree(pvAddr, eMemType, u4Size)  \
{   \
	if (eMemType == PHY_MEM_TYPE) { \
		kfree(pvAddr); \
	} \
	else { \
		vfree(pvAddr); \
	} \
}
#endif

#define kalMemZFree(pvAddr, eMemType, u4Size) \
{ \
	kalMemSet(pvAddr, 0, u4Size); \
	kalMemFree(pvAddr, eMemType, u4Size); \
}

#define kalUdelay(u4USec) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalMdelay(u4MSec) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalMsleep(u4MSec) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalUsleep(u4USec) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalUsleep_range(u4MinUSec, u4MaxUSec) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/* Copy memory from user space to kernel space */
#define kalMemCopyFromUser(_pvTo, _pvFrom, _u4N) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/* Copy memory from kernel space to user space */
#define kalMemCopyToUser(_pvTo, _pvFrom, _u4N) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/* Copy memory block with specific size */
#define kalMemCopy(pvDst, pvSrc, u4Size)  \
	memcpy(pvDst, pvSrc, u4Size)

/* Set memory block with specific pattern */
#define kalMemSet(pvAddr, ucPattern, u4Size)  \
	memset(pvAddr, ucPattern, u4Size)

/* Compare two memory block with specific length.
 * Return zero if they are the same.
 */
#define kalMemCmp(pvAddr1, pvAddr2, u4Size)  \
	memcmp(pvAddr1, pvAddr2, u4Size)

/* Zero specific memory block */
#define kalMemZero(pvAddr, u4Size)  \
	memset(pvAddr, 0, u4Size)

/* Move memory block with specific size */
#define kalMemMove(pvDst, pvSrc, u4Size)  \
	memmove(pvDst, pvSrc, u4Size)

#define strnicmp(s1, s2, n) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/* string operation */
#define kalStrCpy(dest, src)               strcpy(dest, src)
#define kalStrnCpy(dest, src, n)           strncpy(dest, src, n)
#define kalStrCmp(ct, cs)                  strcmp(ct, cs)
#define kalStrnCmp(ct, cs, n)              strncmp(ct, cs, n)
#define kalStrChr(s, c)                    strchr(s, c)
#define kalStrrChr(s, c)                   strrchr(s, c)
#define kalStrnChr(s, n, c)                strnchr(s, n, c)
#define kalStrLen(s)                       strlen(s)
/* #define kalStrtoul(cp, endp, base)      simple_strtoul(cp, endp, base) */
/* #define kalStrtol(cp, endp, base)       simple_strtol(cp, endp, base) */
#define kalkStrtou8(cp, base, resp)        kal_strtou8(cp, base, resp)
#define kalkStrtou16(cp, base, resp)       kal_strtou16(cp, base, resp)
#define kalkStrtou32(cp, base, resp)       kal_strtou32(cp, base, resp)
#define kalkStrtos32(cp, base, resp)       kal_strtos32(cp, base, resp)
#define kalSnprintf(buf, size, fmt, ...)   \
	snprintf(buf, size, fmt, ##__VA_ARGS__)
#define kalScnprintf(buf, size, fmt, ...)  \
	kal_scnprintf(buf, size, fmt, ##__VA_ARGS__)
#define kalVsnprintf(buf, size, fmt, args)          \
	vsnprintf(buf, size, fmt, args)
/* remove for AOSP */
/* #define kalSScanf(buf, fmt, ...)        sscanf(buf, fmt, __VA_ARGS__) */
#define kalStrStr(ct, cs)                  strstr(ct, cs)
#define kalStrCat(dest, src)               strcat(dest, src)
#define kalStrnCat(dst, src, n)            strncat(dst, src, n)
#define kalIsXdigit(c)                     isxdigit(c)
#define kalStrtoint(_data, _base, _res) kal_strtoint(_data, _base, _res)
#define kalStrtouint(_data, _base, _res) kal_strtouint(_data, _base, _res)
#define kalStrtoul(_data, _base, _res) kal_strtoul(_data, _base, _res)
/* implementation for no op API */

#define SHOW_DBGLOG(pcCommand, i4TotalLen, i4BytesWritten, fmt, args...) (\
	{\
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten,\
				i4TotalLen - i4BytesWritten,\
				fmt,\
				args);\
		DBGLOG(HAL, INFO, fmt, args);\
		i4BytesWritten;\
	} \
)

void kal_clear_bit(unsigned long bit, unsigned long *p);
int kal_test_and_clear_bit(unsigned long bit, unsigned long *p);
void kal_set_bit(unsigned long bit, unsigned long *p);
int kal_test_bit(unsigned long bit, unsigned long *p);

/* defined for wince sdio driver only */
#if defined(_HIF_SDIO)
#define kalDevSetPowerState(prGlueInfo, ePowerMode) \
	glSetPowerState(prGlueInfo, ePowerMode)
#else
#define kalDevSetPowerState(prGlueInfo, ePowerMode)
#endif

#define kal_init_completion(eComp) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kal_completion_done(eComp) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kal_reinit_completion(eComp) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kal_completion uint32_t
#define complete(eComp) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kal_kmemleak_ignore(pucAssocIEs) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_ieee80211_get_channel(_rWiphy, _freq) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_ieee80211_channel_to_frequency(_ch, _band) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_max_t(_type, _v1, _v2) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_min_t(_type, _v1, _v2) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_tasklet_schedule(_rTasklet) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_tasklet_hi_schedule(_rTasklet) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalIsZeroEtherAddr(_addr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(__fallthrough__)
#define kal_fallthrough __attribute__((__fallthrough__))
#else
#define kal_fallthrough do {} while (0)  /* fallthrough */
#endif

#define kal_min_t(_type, _v1, _v2) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_tasklet_schedule(_rTasklet) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/*----------------------------------------------------------------------------*/
/*!
 * \brief Notify OS with SendComplete event of the specific packet.
 *        Linux should free packets here.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] u4Status         Status Code for OS upper layer
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalSendComplete(_prGlueInfo, _pvPacket, u4Status) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, \
		_pvPacket, u4Status)
#else
void kalSendComplete(struct GLUE_INFO *prGlueInfo, void *pvPacket,
	uint32_t u4Status);
#endif


/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to locate the starting address of incoming
 *        ethernet frame for skb.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 *
 * \return starting address of ethernet frame buffer.
 */
/*----------------------------------------------------------------------------*/
#define kalQueryBufferPointer(prGlueInfo, pvPacket) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to query the length of valid buffer which is
 *        accessible during port read/write.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 *
 * \return starting address of ethernet frame buffer.
 */
/*----------------------------------------------------------------------------*/
#define kalQueryValidBufferLength(prGlueInfo, pvPacket) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is used to copy the entire frame from skb to the
 *        destination address in the input parameter.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] pucDestBuffer  Destination Address
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
#define kalCopyFrame(prGlueInfo, pvPacket, pucDestBuffer) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalGetTimeTick() KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalGetTimeTickNs() KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalGetJiffies() KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#ifdef CFG_COMBO_SLT_GOLDEN
#define WLAN_TAG                        "[wlan_golden]"
#else
#define WLAN_TAG                        "[wlan]"
#endif
#define kalPrint(_Fmt...) printf(WLAN_TAG _Fmt)
#define kalPrintLimited(_Fmt...) printf(WLAN_TAG _Fmt)
#define kalPrintWoTag(_Fmt...)   printf(_Fmt)

#define kalBreakPoint() \
do { \
	DBGLOG(INIT, ERROR, "WARN_ON()"); \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__); \
} while (0)

#if CFG_ENABLE_AEE_MSG
#define kalSendAeeException \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalSendAeeWarning \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalSendAeeReminding \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
#define kalSendAeeException(_module, _desc, ...)
#define kalSendAeeWarning(_module, _desc, ...)
#define kalSendAeeReminding(_module, _desc, ...)
#endif

#define PRINTF_ARG(...)      __VA_ARGS__
#define SPRINTF(buf, arg)    {buf += sprintf((char *)(buf), PRINTF_ARG arg); }

#define USEC_TO_SYSTIME(_usec)      ((_usec) / USEC_PER_MSEC)
#define MSEC_TO_SYSTIME(_msec)      (_msec)

#define MSEC_TO_JIFFIES(_msec) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_TIME_INTERVAL_DECLARATION()     uint32_t timeval __rTs, __rTe
#define KAL_REC_TIME_START()                ktime_get_ts64(&__rTs)
#define KAL_REC_TIME_END()                  ktime_get_ts64(&__rTe)
#define KAL_GET_TIME_INTERVAL() \
	((SEC_TO_USEC(__rTe.tv_sec) + __rTe.tv_usec) - \
	(SEC_TO_USEC(__rTs.tv_sec) + __rTs.tv_usec))
#define KAL_ADD_TIME_INTERVAL(_Interval) \
	{ \
		(_Interval) += KAL_GET_TIME_INTERVAL(); \
	}

/* TODO: os-related HIF should we move to os/xxx/hif/include? */
#if defined(_HIF_PCIE)
#define KAL_DMA_TO_DEVICE	DMA_TO_DEVICE
#define KAL_DMA_FROM_DEVICE	DMA_FROM_DEVICE

#define KAL_DMA_ALLOC_COHERENT(_dev, _size, _handle) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_FREE_COHERENT(_dev, _size, _addr, _handle) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_MAP_SINGLE(_dev, _ptr, _size, _dir) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_UNMAP_SINGLE(_dev, _addr, _size, _dir) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_MAPPING_ERROR(_dev, _addr) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
#define KAL_DMA_TO_DEVICE	DMA_TO_DEVICE
#define KAL_DMA_FROM_DEVICE	DMA_FROM_DEVICE

#define KAL_DMA_ALLOC_COHERENT(_dev, _size, _handle) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_FREE_COHERENT(_dev, _size, _addr, _handle) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_MAP_SINGLE(_dev, _ptr, _size, _dir) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_UNMAP_SINGLE(_dev, _addr, _size, _dir) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define KAL_DMA_MAPPING_ERROR(_dev, _addr) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#endif

#if defined(_HIF_AXI)
#define KAL_ARCH_SETUP_DMA_OPS(_dev, _base, _size, _iommu, _coherent) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
#define KAL_ARCH_SETUP_DMA_OPS(_dev, _base, _size, _iommu, _coherent)
#endif
/*----------------------------------------------------------------------------*/
/* Macros of show stack operations for using in Driver Layer                  */
/*----------------------------------------------------------------------------*/
#define kal_show_stack(_adapter, _task, _sp)

/*----------------------------------------------------------------------------*/
/* Macros of systrace operations for using in Driver Layer                    */
/*----------------------------------------------------------------------------*/
#define kalTraceBegin(_fmt, ...)
#define kalTraceEnd()
#define kalTraceInt(_value, _fmt, ...)
#define kalTraceCall()
#define kalTraceEvent(_fmt, ...)
#define TRACE(_expr, _fmt, ...) _expr

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/* Routines in gl_kal.c                                                       */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalAcquireSpinLock(_pr, _rLockCate, _plFlags) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr, _rLockCate, _plFlags)

#define kalReleaseSpinLock(_prGlueInfo, _rLockCategory, _ulFlags) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalAcquireSpinLockBh(_prGlueInfo, _rLockCategory) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define	kalReleaseSpinLockBh(_prGlueInfo, _rLockCategory) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalAcquireSpinLockIrq(_pr, _rLockCate, _plFlags) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr, _rLockCate, _plFlags)

#define kalReleaseSpinLockIrq(_prGlueInfo, _rLockCategory, _ulFlags) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#if defined(_HIF_USB)
#define kalAcquiretHifStateLock(_prGlueInfo, _plFlags) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, _plFlags)

#define kalReleaseHifStateLock(_prGlueInfo, _ulFlags) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, _ulFlags)
#endif

#define TX_DIRECT_LOCK(glue) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, glue)


#define TX_DIRECT_UNLOCK(glue) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, glue)


#define RX_DIRECT_REORDER_LOCK(glue, dbg) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, glue, \
	dbg)


#define RX_DIRECT_REORDER_UNLOCK(glue, dbg) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, glue, \
	dbg)

#define kalUpdateMACAddress(_prGlueInfo, _pucMacAddr) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, _pucMacAddr)

#define kalAcquireMutex(_prGlueInfo, _rMutexCategory) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, _rMutexCategory)

#define kalReleaseMutex(_prGlueInfo, _rMutexCategory) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, _rMutexCategory)

#define kalPacketFree(_prGlueInfo, _pvPacket) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, _pvPacket)

#define kalPacketAlloc(_prGlueInfo, _u4Size, _fgIsTx, _ppucData) \
((void *) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo))

#define kalPacketAllocWithHeadroom(_prGlueInfo, _u4Size, _ppucData) \
((void *) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo))

#define kalGetUIntRealTime() \
((uint64_t) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__))

#else
void kalAcquireSpinLock(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long *plFlags);

void kalReleaseSpinLock(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long ulFlags);

void kalUpdateMACAddress(struct GLUE_INFO *prGlueInfo,
			 uint8_t *pucMacAddr);

void kalAcquireMutex(struct GLUE_INFO *prGlueInfo,
		     enum ENUM_MUTEX_CATEGORY_E rMutexCategory);

void kalReleaseMutex(struct GLUE_INFO *prGlueInfo,
		     enum ENUM_MUTEX_CATEGORY_E rMutexCategory);

void kalPacketFree(struct GLUE_INFO *prGlueInfo,
		   void *pvPacket);

void *kalPacketAlloc(struct GLUE_INFO *prGlueInfo,
		     uint32_t u4Size,
		     u_int8_t fgIsTx,
		     uint8_t **ppucData);

void *kalPacketAllocWithHeadroom(struct GLUE_INFO
				 *prGlueInfo,
				 uint32_t u4Size, uint8_t **ppucData);
uint64_t kalGetUIntRealTime(void);
#endif

#define kalDuplicateSwRfbSanity(_prSwRfb) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

void kalOsTimerInitialize(struct GLUE_INFO *prGlueInfo,
			  void *prTimerHandler);

u_int8_t kalSetTimer(struct GLUE_INFO *prGlueInfo,
		     OS_SYSTIME rInterval);

#ifdef CFG_REMIND_IMPLEMENT
#define kalProcessRxPacket(_prGlueInfo, _pvPacket, _pucPacketStart, \
	_u4PacketLen, _aeCSUM) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
uint32_t
kalProcessRxPacket(struct GLUE_INFO *prGlueInfo,
		   void *pvPacket,
		   uint8_t *pucPacketStart, uint32_t u4PacketLen,
		   enum ENUM_CSUM_RESULT aeCSUM[]);
#endif

uint32_t kalRxIndicatePkts(struct GLUE_INFO *prGlueInfo,
			   void *apvPkts[],
			   uint8_t ucPktNum);

#ifdef CFG_REMIND_IMPLEMENT
#define kalRxIndicateOnePkt(_prGlueInfo, _pvPkt) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalIndicateStatusAndComplete(_prGlInfo, _eStatus, _pvBuf, _u4BufLen, \
	_ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalUpdateReAssocReqInfo(_prGlueInfo, _pucFrameBody, _u4FrameBodyLen, \
	_fgReassocRequest, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalUpdateReAssocRspInfo(_prGlueInfo, _pucFrameBody, _u4FrameBodyLen, \
	_ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t kalRxIndicateOnePkt(struct GLUE_INFO
			     *prGlueInfo, void *pvPkt);

void
kalIndicateStatusAndComplete(struct GLUE_INFO
			     *prGlueInfo,
			     uint32_t eStatus, void *pvBuf,
			     uint32_t u4BufLen,
			     uint8_t ucBssIndex);

void
kalUpdateReAssocReqInfo(struct GLUE_INFO *prGlueInfo,
			uint8_t *pucFrameBody, uint32_t u4FrameBodyLen,
			u_int8_t fgReassocRequest,
			uint8_t ucBssIndex);

void kalUpdateReAssocRspInfo(struct GLUE_INFO
			     *prGlueInfo,
			     uint8_t *pucFrameBody,
			     uint32_t u4FrameBodyLen,
			     uint8_t ucBssIndex);
#endif

#if CFG_TX_FRAGMENT
u_int8_t
kalQueryTxPacketHeader(struct GLUE_INFO *prGlueInfo,
		       void *pvPacket, uint16_t *pu2EtherTypeLen,
		       uint8_t *pucEthDestAddr);
#endif /* CFG_TX_FRAGMENT */

#if CFG_TCP_IP_CHKSUM_OFFLOAD
#ifdef CFG_REMIND_IMPLEMENT
#define kalQueryTxChksumOffloadParam(_pvPacket, _pucFlag) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void kalQueryTxChksumOffloadParam(void *pvPacket,
				  uint8_t *pucFlag);
#endif

void kalUpdateRxCSUMOffloadParam(void *pvPacket,
				 enum ENUM_CSUM_RESULT eCSUM[]);
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

#ifdef CFG_REMIND_IMPLEMENT
#define kalRetrieveNetworkAddress(_prGlueInfo, _prMacAddr) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo, _prMacAddr)

#define kalReadyOnChannel(_prGlueInfo, _u8Cookie, _eBand, _eSco, \
	_ucChannelNum, _u4DurationMs, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalRemainOnChannelExpired(_prGlueInfo, _u8Cookie, _eBand, \
	_eSco, _ucChannelNum, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#if CFG_SUPPORT_DFS
#define	kalIndicateAllQueueTxAllowed(_prGlueInfo, _ucBssIndex, _fgIsTxAllowed) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define	kalIndicateChannelSwitch(_prGlueInfo, _eSco, _ucChannelNum, \
	_eBand, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#endif


#define kalIndicateMgmtTxStatus(_prGlueInfo, _u8Cookie, _fgIsAck, \
	_pucFrameBuf, _u4FrameLen, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalIndicateRxMgmtFrame(prAdapter, _prGlueInfo, _prSwRfb, _ucBssIndex, \
	_u4LinkId) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalRetrieveNetworkAddress(struct GLUE_INFO *prGlueInfo,
				uint8_t *prMacAddr);

void
kalReadyOnChannel(struct GLUE_INFO *prGlueInfo,
		  uint64_t u8Cookie,
		  enum ENUM_BAND eBand, enum ENUM_CHNL_EXT eSco,
		  uint8_t ucChannelNum, uint32_t u4DurationMs,
		  uint8_t ucBssIndex);

void
kalRemainOnChannelExpired(struct GLUE_INFO *prGlueInfo,
			  uint64_t u8Cookie, enum ENUM_BAND eBand,
			  enum ENUM_CHNL_EXT eSco, uint8_t ucChannelNum,
			  uint8_t ucBssIndex);

#if CFG_SUPPORT_DFS
void kalIndicateAllQueueTxAllowed(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex, uint8_t fgIsTxAllowed);

void
kalIndicateChannelSwitch(struct GLUE_INFO *prGlueInfo,
			enum ENUM_CHNL_EXT eSco,
			uint8_t ucChannelNum, enum ENUM_BAND eBand,
			uint8_t ucBssIndex);
#endif

void
kalIndicateMgmtTxStatus(struct GLUE_INFO *prGlueInfo,
			uint64_t u8Cookie, u_int8_t fgIsAck,
			uint8_t *pucFrameBuf, uint32_t u4FrameLen,
			uint8_t ucBssIndex);

void kalIndicateRxMgmtFrame(struct ADAPTER *prAdapter,
			    struct GLUE_INFO *prGlueInfo,
			    struct SW_RFB *prSwRfb,
			    uint8_t ucBssIndex,
			    uint32_t u4LinkId);
#endif
/*----------------------------------------------------------------------------*/
/* Routines in interface - ehpi/sdio.c                                        */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalDevRegRead(_prGlueInfo, _u4Register, _pu4Value) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalDevRegRead(struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register,
		       uint32_t *pu4Value);
#endif
u_int8_t kalDevRegRead_mac(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Register, uint32_t *pu4Value);

#ifdef CFG_REMIND_IMPLEMENT
#define kalDevRegWrite(_prGlueInfo, _u4Register, _u4Value) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalDevRegWrite(struct GLUE_INFO *prGlueInfo,
			uint32_t u4Register,
			uint32_t u4Value);
#endif
u_int8_t kalDevRegWrite_mac(struct GLUE_INFO *prGlueInfo,
			    uint32_t u4Register, uint32_t u4Value);

u_int8_t
kalDevPortRead(struct GLUE_INFO *prGlueInfo,
	       uint16_t u2Port, uint32_t u2Len, uint8_t *pucBuf,
	       uint32_t u2ValidOutBufSize, u_int8_t isPollMode);

u_int8_t
kalDevPortWrite(struct GLUE_INFO *prGlueInfo,
		uint16_t u2Port, uint32_t u2Len, uint8_t *pucBuf,
		uint32_t u2ValidInBufSize);

#ifdef CFG_REMIND_IMPLEMENT
#define kalDevWriteData(_prGlueInfo, _prMsduInfo) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalDevWriteCmd(_prGlueInfo, _prCmdInfo, _ucTC) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalDevKickData(_prGlueInfo) \
KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalDevWriteData(struct GLUE_INFO *prGlueInfo,
			 struct MSDU_INFO *prMsduInfo);
enum ENUM_CMD_TX_RESULT kalDevWriteCmd(struct GLUE_INFO *prGlueInfo,
			struct CMD_INFO *prCmdInfo, uint8_t ucTC);
u_int8_t kalDevKickData(struct GLUE_INFO *prGlueInfo);
#endif
void kalDevReadIntStatus(struct ADAPTER *prAdapter,
			 uint32_t *pu4IntStatus);

u_int8_t kalDevWriteWithSdioCmd52(struct GLUE_INFO
				  *prGlueInfo,
				  uint32_t u4Addr, uint8_t ucData);

#if CFG_SUPPORT_EXT_CONFIG
uint32_t kalReadExtCfg(struct GLUE_INFO *prGlueInfo);
#endif

#ifdef CFG_REMIND_IMPLEMENT
#define kalQoSFrameClassifierAndPacketInfo(_pr, _prPacket, _prTxPktInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalGetEthDestAddr(_prGlueInfo, _prPacket, _pucEthDestAddr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalOidComplete(_prGlueInfo, _prCmdInfo, _u4SetQueryInfoLen, \
	       _rOidStatus) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalIoctl(_pr, _pfnOidHandler, _pvInfoBuf, _u4InfoBufLen, \
	_pu4QryInfoLen) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr, \
	_pfnOidHandler, _pvInfoBuf, _u4InfoBufLen, _pu4QryInfoLen)

#define kalIoctlByBssIdx(_pr, _pfnOidHandler, _pvInfoBuf, _u4InfoBufLen, \
	_pu4QryInfoLen, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr, \
	_pfnOidHandler, _pvInfoBuf, _u4InfoBufLen, \
	_pu4QryInfoLen, _ucBssIndex)

#define SET_IOCTL_BSSIDX(_pr, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr, \
	_ucBssIndex)

#define GET_IOCTL_BSSIDX(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#else
u_int8_t
kalQoSFrameClassifierAndPacketInfo(struct GLUE_INFO
				   *prGlueInfo,
				   void *prPacket,
				   struct TX_PACKET_INFO *prTxPktInfo);

u_int8_t kalGetEthDestAddr(struct GLUE_INFO *prGlueInfo,
			   void *prPacket,
			   uint8_t *pucEthDestAddr);

void
kalOidComplete(struct GLUE_INFO *prGlueInfo,
	       struct CMD_INFO *prCmdInfo, uint32_t u4SetQueryInfoLen,
	       uint32_t rOidStatus);

uint32_t
kalIoctl(struct GLUE_INFO *prGlueInfo,
	 PFN_OID_HANDLER_FUNC pfnOidHandler,
	 void *pvInfoBuf, uint32_t u4InfoBufLen,
	 uint32_t *pu4QryInfoLen);

uint32_t
kalIoctlByBssIdx(struct GLUE_INFO *prGlueInfo,
	 PFN_OID_HANDLER_FUNC pfnOidHandler,
	 void *pvInfoBuf, uint32_t u4InfoBufLen,
	 uint32_t *pu4QryInfoLen,
	 uint8_t ucBssIndex);

void SET_IOCTL_BSSIDX(
	 struct ADAPTER *prAdapter,
	 uint8_t ucBssIndex);

uint8_t GET_IOCTL_BSSIDX(
	 struct ADAPTER *prAdapter);

#endif

void kalHandleAssocInfo(struct GLUE_INFO *prGlueInfo,
			struct EVENT_ASSOC_INFO *prAssocInfo);

#ifdef CFG_REMIND_IMPLEMENT
#define kalGetChipID(void) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t kalGetChipID(void);
#endif

#ifdef CFG_REMIND_IMPLEMENT
#define kalGetConnsysVersion(void) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t kalGetConnsysVersion(void);
#endif

#ifdef CFG_REMIND_IMPLEMENT
#define kalGetWfIpVersion(void) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t kalGetWfIpVersion(void);
#endif

#ifdef CFG_REMIND_IMPLEMENT
#define kalGetFwVerOffset(void) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t kalGetFwVerOffset(void);
#endif

#if CFG_ENABLE_FW_DOWNLOAD
#ifdef CFG_REMIND_IMPLEMENT
#define kalFirmwareImageMapping(_prGlueInfo, _ppvMapFileBuf, \
	_pu4FileLength, _eDlIdx) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalFirmwareImageUnmapping(_prGlueInfo, _prFwHandle, _pvMapFileBuf) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
void *kalFirmwareImageMapping(struct GLUE_INFO
			      *prGlueInfo,
			      void **ppvMapFileBuf,
			      uint32_t *pu4FileLength,
			      enum ENUM_IMG_DL_IDX_T eDlIdx);
void kalFirmwareImageUnmapping(struct GLUE_INFO
			       *prGlueInfo,
			       void *prFwHandle, void *pvMapFileBuf);
#endif
#endif

/*----------------------------------------------------------------------------*/
/* Card Removal Check                                                         */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalIsCardRemoved(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalIsCardRemoved(struct GLUE_INFO *prGlueInfo);
#endif
/*----------------------------------------------------------------------------*/
/* TX                                                                         */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalFlushPendingTxPackets(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
void kalFlushPendingTxPackets(struct GLUE_INFO *prGlueInfo);
#endif

/*----------------------------------------------------------------------------*/
/* RX                                                                         */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalScheduleNapiTask(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prAdapter)
#else
uint32_t kalScheduleNapiTask(struct ADAPTER *prAdapter);
#endif

/*----------------------------------------------------------------------------*/
/* Media State Indication                                                     */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalGetMediaStateIndicated(_prGlueInfo, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalSetMediaStateIndicated(_prGlueInfo, _eParamMediaStateIndicate, \
	_ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
enum ENUM_PARAM_MEDIA_STATE kalGetMediaStateIndicated(
	struct GLUE_INFO
	*prGlueInfo,
	uint8_t ucBssIndex);

void kalSetMediaStateIndicated(struct GLUE_INFO *prGlueInfo,
		enum ENUM_PARAM_MEDIA_STATE eParamMediaStateIndicate,
		uint8_t ucBssIndex);
#endif

/*----------------------------------------------------------------------------*/
/* OID handling                                                               */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalOidCmdClearance(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalOidClearance(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalEnqueueCommand(_prGlueInfo, _prQueueEntry) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
void kalOidCmdClearance(struct GLUE_INFO *prGlueInfo);

void kalOidClearance(struct GLUE_INFO *prGlueInfo);

void kalEnqueueCommand(struct GLUE_INFO *prGlueInfo,
		       struct QUE_ENTRY *prQueueEntry);
#endif

#if CFG_ENABLE_BT_OVER_WIFI
/*----------------------------------------------------------------------------*/
/* Bluetooth over Wi-Fi handling                                              */
/*----------------------------------------------------------------------------*/
void kalIndicateBOWEvent(struct GLUE_INFO *prGlueInfo,
			 struct BT_OVER_WIFI_EVENT *prEvent);

enum ENUM_BOW_DEVICE_STATE kalGetBowState(
	struct GLUE_INFO *prGlueInfo,
	uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN]);

u_int8_t kalSetBowState(struct GLUE_INFO *prGlueInfo,
			enum ENUM_BOW_DEVICE_STATE eBowState,
			uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN]);

enum ENUM_BOW_DEVICE_STATE kalGetBowGlobalState(
	struct GLUE_INFO
	*prGlueInfo);

uint32_t kalGetBowFreqInKHz(struct GLUE_INFO
			    *prGlueInfo);

uint8_t kalGetBowRole(struct GLUE_INFO *prGlueInfo,
		      uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN]);

void kalSetBowRole(struct GLUE_INFO *prGlueInfo,
		   uint8_t ucRole,
		   uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN]);

uint8_t kalGetBowAvailablePhysicalLinkCount(
	struct GLUE_INFO *prGlueInfo);

#if CFG_BOW_SEPARATE_DATA_PATH
/*----------------------------------------------------------------------------*/
/* Bluetooth over Wi-Fi Net Device Init/Uninit                                */
/*----------------------------------------------------------------------------*/
u_int8_t kalInitBowDevice(struct GLUE_INFO *prGlueInfo,
			  const char *prDevName);

u_int8_t kalUninitBowDevice(struct GLUE_INFO
			    *prGlueInfo);
#endif /* CFG_BOW_SEPARATE_DATA_PATH */
#endif /* CFG_ENABLE_BT_OVER_WIFI */

/*----------------------------------------------------------------------------*/
/* Management Frame Clearance                                                 */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalClearMgmtFrames(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalClearMgmtFramesByBssIdx(_prGlueInfo, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalGetTxPendingFrameCount(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalGetTxPendingCmdCount(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalClearCommandQueue(_prGlueInfo, fgIsNeedHandler) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalSetTimer(_prGlueInfo, _u4Interval) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
void kalClearMgmtFrames(struct GLUE_INFO *prGlueInfo);

void kalClearMgmtFramesByBssIdx(struct GLUE_INFO
				*prGlueInfo,
				uint8_t ucBssIndex);

uint32_t kalGetTxPendingFrameCount(struct GLUE_INFO
				   *prGlueInfo);

uint32_t kalGetTxPendingCmdCount(struct GLUE_INFO
				 *prGlueInfo);

void kalClearCommandQueue(struct GLUE_INFO *prGlueInfo,
	u_int8_t fgIsNeedHandler);

u_int8_t kalSetTimer(struct GLUE_INFO *prGlueInfo,
		     uint32_t u4Interval);
#endif

#define kalCancelTimer(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#ifdef CFG_REMIND_IMPLEMENT
#define kalScanDone(_prGlueInfo, _eNetTypeIdx, _status) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
void kalScanDone(struct GLUE_INFO *prGlueInfo,
		 enum ENUM_KAL_NETWORK_TYPE_INDEX eNetTypeIdx,
		 uint32_t status);
#endif

#if CFG_SUPPORT_SCAN_CACHE_RESULT
uint8_t kalUpdateBssTimestamp(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */

void kalTimeoutHandler(unsigned long arg);

#ifdef CFG_REMIND_IMPLEMENT
#define kalRandomNumber() KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalRandomGetBytes(a, b) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, a, b)

#define kalSetEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetIntEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetWmmUpdateEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetMddpEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetHifAerResetEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetHifMsiRecoveryEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#ifdef CFG_MTK_WIFI_CONNV3_SUPPORT
#define kalSetBtDumpViaWFEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)
#endif

#define kalSetHifDbgEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)
#else
uint32_t kalRandomNumber(void);
void kalRandomGetBytes(void *buf, uint32_t len);

void kalSetEvent(struct GLUE_INFO *pr);

void kalSetIntEvent(struct GLUE_INFO *pr);

void kalSetWmmUpdateEvent(struct GLUE_INFO *pr);

void kalSetMdCrashEvent(struct GLUE_INFO *pr);

void kalSetHifDbgEvent(struct GLUE_INFO *pr);
#endif

#if CFG_SUPPORT_MULTITHREAD
#ifdef CFG_REMIND_IMPLEMENT
#define kalSetSerIntEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetTxEvent2Hif(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetTxEvent2Rx(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetTxCmdEvent2Hif(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)
#define kalSetTxCmdDoneEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)

#define kalSetRxProcessEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)
#else
void kalSetSerIntEvent(struct GLUE_INFO *pr);

void kalSetTxEvent2Hif(struct GLUE_INFO *pr);

void kalSetTxEvent2Rx(struct GLUE_INFO *pr);

void kalSetTxCmdEvent2Hif(struct GLUE_INFO *pr);

void kalSetTxCmdDoneEvent(struct GLUE_INFO *pr);

void kalSetRxProcessEvent(struct GLUE_INFO *pr);
#endif
#endif
/*----------------------------------------------------------------------------*/
/* NVRAM/Registry Service                                                     */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalIsConfigurationExist(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalGetConfiguration(_prGlueInfo) \
((struct REG_INFO *)KAL_NEED_IMPLEMENT(__FILE__, __func__, \
	__LINE__, _prGlueInfo))

#define kalCfgDataRead16(_prGlueInfo, _u4Offset, _pu2Data) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalCfgDataWrite16(_prGlueInfo, _u4Offset, _u2Data) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalIsConfigurationExist(struct GLUE_INFO
				 *prGlueInfo);

struct REG_INFO *kalGetConfiguration(struct GLUE_INFO
				     *prGlueInfo);

u_int8_t kalCfgDataRead(struct GLUE_INFO *prGlueInfo,
			uint32_t u4Offset,
			ssize_t len, uint16_t *pu2Data);

u_int8_t kalCfgDataRead16(struct GLUE_INFO *prGlueInfo,
			  uint32_t u4Offset,
			  uint16_t *pu2Data);

u_int8_t kalCfgDataWrite16(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Offset, uint16_t u2Data);
#endif
/*----------------------------------------------------------------------------*/
/* WSC Connection                                                     */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalWSCGetActiveState(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalWSCGetActiveState(struct GLUE_INFO
			      *prGlueInfo);
#endif

/*----------------------------------------------------------------------------*/
/* RSSI Updating                                                              */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalUpdateRSSI(_prGlueInfo, _ucBssIndex, _cRssi, _cLinkQuality) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
void
kalUpdateRSSI(struct GLUE_INFO *prGlueInfo,
	      uint8_t ucBssIndex,
	      int8_t cRssi,
	      int8_t cLinkQuality);
#endif

/*----------------------------------------------------------------------------*/
/* I/O Buffer Pre-allocation                                                  */
/*----------------------------------------------------------------------------*/
u_int8_t kalInitIOBuffer(u_int8_t is_pre_alloc);

void kalUninitIOBuffer(void);

#ifdef CFG_REMIND_IMPLEMENT
#define kalAllocateIOBuffer(_u4AllocSize) \
((uint8_t *) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__))

#define kalReleaseIOBuffer(_pvAddr, _u4Size) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void *kalAllocateIOBuffer(uint32_t u4AllocSize);

void kalReleaseIOBuffer(void *pvAddr,
			uint32_t u4Size);
#endif

void
kalGetChannelList(struct GLUE_INFO *prGlueInfo,
		  enum ENUM_BAND eSpecificBand,
		  uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
		  struct RF_CHANNEL_INFO *paucChannelList);

u_int8_t kalIsAPmode(struct GLUE_INFO *prGlueInfo);

#if CFG_SUPPORT_802_11W
/*----------------------------------------------------------------------------*/
/* 802.11W                                                                    */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalGetMfpSetting(_prGlueInfo, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalGetRsnIeMfpCap(_prGlueInfo, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t kalGetMfpSetting(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex);
uint8_t kalGetRsnIeMfpCap(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex);
#endif
#endif

/*----------------------------------------------------------------------------*/
/* file opetation                                                             */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
/* used only under os folder */
#define kalRequestFirmware(_pucPath, _pucData, _pu4ReadSize, \
		_ucIsZeroPadding, _dev) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
/* used only under os folder */
int32_t kalRequestFirmware(const uint8_t *pucPath,
			   uint8_t **pucData,
			   uint32_t *pu4ReadSize,
			   uint8_t ucIsZeroPadding,
			   void *dev);
#endif
/*----------------------------------------------------------------------------*/
/* NL80211                                                                    */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalIndicateBssInfo(_prGlueInfo, _pucFrameBuf, _u4BufLen, \
	_ucChannelNum, _eBand, _i4SignalStrength) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
void
kalIndicateBssInfo(struct GLUE_INFO *prGlueInfo,
		   uint8_t *pucFrameBuf, uint32_t u4BufLen,
		   uint8_t ucChannelNum, enum ENUM_BAND eBand,
		   int32_t i4SignalStrength);
#endif
/*----------------------------------------------------------------------------*/
/* Net device                                                                 */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalHardStartXmit(_prSkb, _prDev, _prGlueInfo, _ucBssIndex) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalIsPairwiseEapolPacket(_prPacket) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalGetIPv4Address(_prDev, _u4MaxNumOfAddr, _pucIpv4Addrs, \
		_pu4NumOfIpv4Addr) \
		KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#if IS_ENABLED(CONFIG_IPV6)
#define kalGetIPv6Address(_prDev, _u4MaxNumOfAddr, _pucIpv6Addrs, \
		_pu4NumOfIpv6Addr) \
		KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#endif /* IS_ENABLED(CONFIG_IPV6) */

#define kalSetNetAddressFromInterface(_prGlueInfo, _prDev, _fgSet) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalResetStats(_prDev) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalGetStats(_prDev) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t
kalHardStartXmit(struct sk_buff *prSkb,
		 struct net_device *prDev,
		 struct GLUE_INFO *prGlueInfo, uint8_t ucBssIndex);

u_int8_t kalIsPairwiseEapolPacket(void *prPacket);

u_int8_t
kalGetIPv4Address(struct net_device *prDev,
		  uint32_t u4MaxNumOfAddr, uint8_t *pucIpv4Addrs,
		  uint32_t *pu4NumOfIpv4Addr);

#if IS_ENABLED(CONFIG_IPV6)
u_int8_t
kalGetIPv6Address(struct net_device *prDev,
		  uint32_t u4MaxNumOfAddr, uint8_t *pucIpv6Addrs,
		  uint32_t *pu4NumOfIpv6Addr);
#else
static inline u_int8_t
kalGetIPv6Address(struct net_device *prDev,
		  uint32_t u4MaxNumOfAddr, uint8_t *pucIpv6Addrs,
		  uint32_t *pu4NumOfIpv6Addr) {
	/* Not support IPv6 */
	*pu4NumOfIpv6Addr = 0;
	return 0;
}
#endif /* IS_ENABLED(CONFIG_IPV6) */

void kalSetNetAddressFromInterface(struct GLUE_INFO
				   *prGlueInfo,
				   struct net_device *prDev,
				   u_int8_t fgSet);

uint32_t kalResetStats(struct net_device *prDev);

void *kalGetStats(struct net_device *prDev);
#endif

void kalResetPacket(struct GLUE_INFO *prGlueInfo,
		    void *prPacket);

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
/*----------------------------------------------------------------------------*/
/* SDIO Read/Write Pattern Support                                            */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalSetSdioTestPattern(_prGlueInfo, _fgEn, _fgRead) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
u_int8_t kalSetSdioTestPattern(struct GLUE_INFO
			       *prGlueInfo,
			       u_int8_t fgEn, u_int8_t fgRead);
#endif
#endif

/*----------------------------------------------------------------------------*/
/* PNO Support                                                                */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalSchedScanResults(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalSchedScanStopped(_prGlueInfo, _fgDriverTriggerd) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)

#define kalSetFwOwnEvent2Hif(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)
#else
void kalSchedScanResults(struct GLUE_INFO *prGlueInfo);

void kalSchedScanStopped(struct GLUE_INFO *prGlueInfo,
			 u_int8_t fgDriverTriggerd);

void kalSetFwOwnEvent2Hif(struct GLUE_INFO *pr);
#endif

#if (CFG_CE_ASSERT_DUMP == 1)
#ifdef CFG_REMIND_IMPLEMENT
#define kalEnqCoreDumpLog(_prAdapter, _pucBuffer, _u2Size) \
		KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint32_t kalEnqCoreDumpLog(struct ADAPTER *prAdapter, uint8_t *pucBuffer,
			     uint16_t u2Size)
#endif
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if CFG_WOW_SUPPORT
#ifdef CFG_REMIND_IMPLEMENT
#define kalWowInit(_glueinfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalWowProcess(_glueinfo, _enable) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void kalWowInit(struct GLUE_INFO *prGlueInfo);
void kalWowProcess(struct GLUE_INFO *prGlueInfo,
		   uint8_t enable);
#endif
#endif

int main_thread(void *data);

#if CFG_SUPPORT_MULTITHREAD
int hif_thread(void *data);
int rx_thread(void *data);
#endif

#ifdef CFG_REMIND_IMPLEMENT
#define kalGetBootTime() KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
uint64_t kalGetBootTime(void);
#endif

int kalMetInitProcfs(struct GLUE_INFO *prGlueInfo);
int kalMetRemoveProcfs(void);

uint8_t kalGetEapolKeyType(void *prPacket);

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
#ifdef CFG_REMIND_IMPLEMENT
#define kalIsWakeupByWlan(_prAdapter) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
u_int8_t kalIsWakeupByWlan(struct ADAPTER *prAdapter);
#endif
#endif

int32_t kalHaltLock(uint32_t waitMs);
int32_t kalHaltTryLock(void);
void kalHaltUnlock(void);
void kalSetHalted(u_int8_t fgHalt);
u_int8_t kalIsHalted(void);

#if CFG_SUPPORT_MULTITHREAD
#ifdef CFG_REMIND_IMPLEMENT
#define kalFreeTxMsduWorker(_work) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void kalFreeTxMsduWorker(struct work_struct *work);
#endif
void kalFreeTxMsdu(struct ADAPTER *prAdapter,
		   struct MSDU_INFO *prMsduInfo);
#endif

int32_t kalPerMonInit(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonDisable(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonEnable(struct GLUE_INFO *prGlueInfo);
#ifdef CFG_REMIND_IMPLEMENT
#define kalPerMonStart(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _prGlueInfo)
#else
int32_t kalPerMonStart(struct GLUE_INFO *prGlueInfo);
#endif
int32_t kalPerMonStop(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonDestroy(struct GLUE_INFO *prGlueInfo);
void kalPerMonHandler(struct ADAPTER *prAdapter,
		      unsigned long ulParam);
uint32_t kalPerMonGetInfo(struct ADAPTER *prAdapter,
			  uint8_t *pucBuf,
			  uint32_t u4Max);
int32_t kalBoostCpu(struct ADAPTER *prAdapter,
		    uint32_t u4TarPerfLevel,
		    uint32_t u4BoostCpuTh);
int32_t kalCheckTputLoad(struct ADAPTER *prAdapter,
			 uint32_t u4CurrPerfLevel,
			 uint32_t u4TarPerfLevel,
			 int32_t i4Pending,
			 uint32_t u4Used);
void kalSetRpsMap(struct GLUE_INFO *glue, unsigned long value);
#if CFG_MTK_ANDROID_EMI
void kalSetEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
			    uint32_t size, bool enable);
void kalSetDrvEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
			       uint32_t size);
#endif
int32_t kalSetCpuNumFreq(uint32_t u4CoreNum,
			 uint32_t u4Freq);
int32_t kalPerMonSetForceEnableFlag(uint8_t uFlag);
int32_t kalFbNotifierReg(struct GLUE_INFO *prGlueInfo);
void kalFbNotifierUnReg(void);

#ifdef CFG_REMIND_IMPLEMENT
#define kalInitDevWakeup(_prAdapter, _prDev) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
void kalInitDevWakeup(struct ADAPTER *prAdapter, struct device *prDev);
#endif

#ifdef CFG_REMIND_IMPLEMENT
#define kalIsValidMacAddr(_addr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalScanParseRandomMac(_prdev, prRrequest, _pucRandomMac) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalSchedScanParseRandomMac(_prdev, prRrequest, _pucRandomMac \
	, _pucRandomMacMask) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalScanReqLog(prRrequest) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalScanResultLog(_prAdapter, _prMgmt) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
u_int8_t kalIsValidMacAddr(const uint8_t *addr);

u_int8_t kalScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_scan_request *request, uint8_t *pucRandomMac);

u_int8_t kalSchedScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_sched_scan_request *request,
	uint8_t *pucRandomMac, uint8_t *pucRandomMacMask);

void kalScanReqLog(struct cfg80211_scan_request *request);
void kalScanResultLog(struct ADAPTER *prAdapter, struct ieee80211_mgmt *mgmt);
#endif
void kalScanLogCacheFlushBSS(struct ADAPTER *prAdapter,
	const uint16_t logBufLen);

#ifdef CFG_REMIND_IMPLEMENT
#define kalMaskMemCmp(_cs, _ct, _mask, _count) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalChannelScoSwitch(_channel_type, _prChnlSco) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalChannelFormatSwitch(_prChannel_def, _prChannel \
		, _prRfChnlInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalRemoveBss(_prGlueInfo, \
	_aucBSSID, _ucChannelNum, \
	_eBand) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalIsResetting(_void) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalRxNapiValidSkb(_prGlueInfo, _prSkb) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
int kalMaskMemCmp(const void *cs, const void *ct,
	const void *mask, size_t count);

u_int8_t
kalChannelScoSwitch(enum nl80211_channel_type channel_type,
		enum ENUM_CHNL_EXT *prChnlSco);

u_int8_t
kalChannelFormatSwitch(struct cfg80211_chan_def *channel_def,
		struct ieee80211_channel *channel,
		struct RF_CHANNEL_INFO *prRfChnlInfo);

void kalRemoveBss(struct GLUE_INFO *prGlueInfo,
	uint8_t aucBSSID[],
	uint8_t ucChannelNum,
	enum ENUM_BAND eBand);

u_int8_t kalIsResetting(void);
u_int8_t kalIsRstPreventFwOwn(void);

uint8_t kalRxNapiValidSkb(struct GLUE_INFO *prGlueInfo,
	struct sk_buff *prSkb);
#endif

#if CFG_CHIP_RESET_SUPPORT
void kalRemoveProbe(struct GLUE_INFO *prGlueInfo);

#ifdef CFG_REMIND_IMPLEMENT
#define kalCheckWfsysResetPostpone(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#else
u_int8_t kalCheckWfsysResetPostpone(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_REMIND_IMPLEMENT */
#endif
#if (CFG_SUPPORT_SINGLE_SKU == 1)
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
void kalApplyCustomRegulatory(const void *pRegdom,
	uint8_t fgNeedHoldRtnlLock);
const void *kalGetDefaultRegWW(void);
#endif
uint8_t kalGetRdmVal(uint8_t dfs_region);
u_int8_t kalIsETSIDfsRegin(void);
#endif
u_int8_t kalIsChFlagMatch(uint32_t uFlags, enum CHAN_FLAGS matchFlag);

#define kalGetCpuBoostThreshold() \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalCheckVcoreBoost(_prAdapter, _bssInd) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define likely(_cond) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define unlikely(_cond) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_mod64(_a, _b) do_div(_a, _b)

#define do_div(_ret, _base) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kal_div64_u64(_a, _b) div64_u64(_a, _b)

#define kal_div_u64(_a, _b) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define div64_u64(_dividend, _divisor) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalSetDrvIntEvent(_prGlueInfo) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalSendUevent(_adapter, _src) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalCsaNotifyWorkDeinit(_prAdapter, _ucBssIdx) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalAisCsaNotifyWorkInit(_prAdapter, _ucBssIdx) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalIsHalted() \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#if CFG_SUPPORT_DBDC
#define kalIndicateOpModeChange(_prAdapter, _ucBssIdx, _ucChannelBw, _ucTxNss, \
				_ucRxNss) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#endif

#define kalIcsWrite(buf, size) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#define kalIndexWrite(buf, size) \

#if (CFG_SUPPORT_CONNAC3X == 1 && CFG_SUPPORT_UPSTREAM_TOOL == 1)
#define kalWiphy_info(wiphy, format, ...) \

#endif

#if CFG_SUPPORT_DATA_STALL
#define KAL_REPORT_ERROR_EVENT(_prAdapter, \
			       _event, \
			       _dataLen, \
			       _ucBssIdx, \
			       _fgForceReport) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#endif

#define kalGetTpMbpsByBssId(prAdapter, ePath, ucBssIdx) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

#define kalVendorEvtRssiBeyondRange(_prAdapter, _ucBssIdx, _i4Rssi) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)

void kalTxTimeoutDump(struct ADAPTER *prAdapter);
void kalSetTxTimeoutDump(struct GLUE_INFO *pr);
int kalTimeCompare(uint32_t *pu4Ts1, uint32_t *pu4Ts2);
u_int8_t kalGetDeltaTime(uint32_t *pu4Ts1, uint32_t *pu4Ts2,
			 uint32_t *pu4TsRst);

uint32_t kalSyncTimeToFW(struct ADAPTER *prAdapter,
	u_int8_t fgInitCmd);
void kalSetLogTooMuch(uint32_t u4DriverLevel,
	uint32_t u4FwLevel);
void kalGetRealTime(struct REAL_TIME *prRealTime);

void kalVendorEventRssiBeyondRange(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIdx, int rssi);

void kalTxDirectStartCheckQTimer(
	struct GLUE_INFO *prGlueInfo,
	uint8_t offset);
uint32_t kalGetTxDirectQueueLength(
	struct GLUE_INFO *prGlueInfo);

#if CFG_SUPPORT_WPA3
int kalExternalAuthRequest(
	struct GLUE_INFO *prGlueInfo,
	struct STA_RECORD *prStaRec);
#endif

void kalKfreeSkb(void *pvPacket, u_int8_t fgIsFreeData);
void *kalBuildSkb(void *pvPacket, uint32_t u4MgmtLength,
	uint32_t u4TotLen, u_int8_t fgIsSetLen);
uint32_t kalGetSKBSharedInfoSize(void);
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
void kalSetMgmtDirectTxEvent2Hif(
		struct GLUE_INFO *pr);
#endif

uint32_t kalGetChannelFrequency(
		uint8_t ucChannel,
		uint8_t ucBand);

#if (CFG_SUPPORT_SINGLE_SKU == 1)
u_int8_t kalFillChannels(
	struct GLUE_INFO *prGlueInfo,
	struct CMD_DOMAIN_CHANNEL *pChBase,
	uint8_t ucChSize,
	uint8_t ucOpChannelNum,
	u_int8_t fgDisconnectUponInvalidOpChannel
);
#endif
uint8_t kalGetChannelCount(struct GLUE_INFO *prGlueInfo);

u_int8_t kalIsValidChnl(struct GLUE_INFO *prGlueInfo,
			uint8_t ucNumOfChannel,
			enum ENUM_BAND eBand);

int kalStrniCmp(const char *s1, const char *s2, size_t n);
char *kalStrSep(char **stringp, const char *delim);
size_t kalStrnLen(const char *s, size_t b);
char *kalStrtokR(char *s, const char *delim, char **last);
int kalFfs(int s);

void *kalGetNetDevPriv(void *prNet);

uint32_t kalGetNetDevRxPacket(void *prNet);

#if CFG_SUPPORT_TDLS
void kalTdlsOpReq(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex,
	uint8_t *aucMacAddr,
	uint16_t eOpMode,
	uint16_t u2ReasonCode
	);
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD

void kalConfigChksumOffload(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable);
#endif

uint32_t kalQueryPacketLength(void *pvPacket);

void kalSetPacketLength(void *pvPacket, uint32_t u4len);

uint16_t kalQueryPacketEtherType(void *pvPacket);

uint8_t kalQueryPacketIPVersion(void *pvPacket);

uint8_t kalQueryPacketIPV4Precedence(void *pvPacket);

uint8_t kalQueryPacketIPv4Protocol(void *pvPacket);

uint16_t kalQueryPacketIPv4Identification(void *pvPacket);

uint16_t kalQueryPacketIPv4TCPUDPSrcPort(void *pvPacket);

uint16_t kalQueryPacketIPv4TCPUDPDstPort(void *pvPacket);

int kalComparePacketIPv4UDPPayload(void *pvPacket,
				int8_t *pattern,
				size_t length);

void kalUpdatePacketIPv4UDPPayload(void *pvPacket,
				uint16_t offset,
				void *pattern,
				size_t length);

void kalGetPacketBuf(void *pvPacket, uint8_t **ppucData);
void kalGetPacketBufHeadManipulate(void *pvPacket,
				uint8_t **ppucData,
				int16_t length);
void kalGetPacketBufTailManipulate(void *pvPacket,
				uint8_t **ppucData,
				int16_t length);

uint32_t kalGetPacketMark(void *pvPacket);
u_int8_t kalProcessRadiotap(void *pvPacket,
	uint8_t **ppucData,
	uint16_t radiotap_len,
	uint16_t u2RxByteCount);

void kalSetPacketDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex,
	void *pvPacket);

void *kalGetPacketDev(void *pvPacket);

int kal_skb_checksum_help(void *pvPacket);
void kalSkbCopyCbData(void *pvDstPacket, void *pvSrcPacket);
void *kal_skb_copy(void *pvPacket);
void kal_skb_reserve(void *pvPacket, uint8_t ucLength);
void kal_skb_split(void *pvPacket, void *pvPacket1, const uint32_t u4Length);
uint8_t *kal_skb_push(void *pvPacket, uint32_t u4Length);
uint8_t *kal_skb_pull(void *pvPacket, uint32_t u4Length);

void kalWlanHardStartXmit(void *pvPacket, void *pvDev);

uint8_t kalNlaPut(void *pvPacket, uint32_t attrType,
		uint32_t attrLen, const void *data);

void *
kalProcessRttReportDone(struct GLUE_INFO *prGlueInfo,
		uint32_t u4DataLen, uint32_t u4Count);

void *kalGetGlueNetDevHdl(struct GLUE_INFO *prGlueInfo);
void *kalGetGlueDevHdl(struct GLUE_INFO *prGlueInfo);
void kalGetDev(void **dev);
void kalClearGlueScanReq(struct GLUE_INFO *prGlueInfo);
void *kalGetGlueScanReq(struct GLUE_INFO *prGlueInfo);
void *kalGetGlueSchedScanReq(struct GLUE_INFO *prGlueInfo);
void kalClearGlueSchedScanReq(struct GLUE_INFO *prGlueInfo);
void kalGetFtIeParam(void *pvftie,
	uint16_t *pu2MDID, uint32_t *pu4IeLength,
	const uint8_t **pucIe);

int kal_strtou8(const char *s, unsigned int base, uint8_t *res);
int kal_strtou16(const char *s, unsigned int base, uint16_t *res);
int kal_strtou32(const char *s, unsigned int base, uint32_t *res);
int kal_strtos32(const char *s, unsigned int base, int32_t *res);
int kal_strtoint(const char *s, unsigned int base, int *res);
int kal_strtouint(const char *s, unsigned int base, unsigned int *res);
int kal_strtoul(const char *s, unsigned int base, unsigned long *res);
int kal_scnprintf(char *buf, size_t size, const char *fmt, ...);
void *kal_kmalloc(size_t size, enum gfp_t type);
void *kal_vmalloc(size_t size);
void kal_kfree(void *addr);
void kal_vfree(void *addr);
int kal_hex_dump_to_buffer(const void *buf, size_t len, int rowsize,
	int groupsize, char *linebuf, size_t linebuflen,
	bool ascii);
bool kal_warn_on(uint8_t condition);

int kalRegulatoryHint(char *country);

bool kal_is_err(void *ptr);

#if CFG_MTK_WIFI_PCIE_SR
u_int8_t kalIsSupportPcieL2(void);
#endif
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
u_int8_t kalIsSupportMawd(void);
u_int8_t kalIsSupportSdo(void);
u_int8_t kalIsSupportRro(void);
uint32_t kalGetMawdVer(void);
uint32_t kalGetConnInfraId(void);
#endif

uint32_t kalFirmwareOpen(struct GLUE_INFO *prGlueInfo,
			 uint8_t **apucNameTable);
uint32_t kalFirmwareClose(struct GLUE_INFO *prGlueInfo);
uint32_t kalFirmwareSize(struct GLUE_INFO *prGlueInfo,
			 uint32_t *pu4Size);
uint32_t kalFirmwareLoad(struct GLUE_INFO *prGlueInfo,
			 void *prBuf, uint32_t u4Offset,
			 uint32_t *pu4Size);

int32_t kalGetFwFlavor(uint8_t *flavor);

void kalIndicateControlPortTxStatus(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus);

#endif /* _GL_KAL_H */
