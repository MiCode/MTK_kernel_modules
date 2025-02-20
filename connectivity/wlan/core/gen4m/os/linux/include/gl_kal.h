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
#include "gl_typedef.h"
#include "gl_wext_priv.h"
#include "link.h"
#include "nic/mac.h"
#include "nic/wlan_def.h"
#include "wlan_lib.h"
#include "wlan_oid.h"

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
#include "conn_power_throttling.h"
#endif

#if ((CFG_MODIFY_TX_POWER_BY_BAT_VOLT) || (CFG_VOLT_INFO == 1))
#include "pmic_lbat_service.h"
#endif

#if CFG_ENABLE_BT_OVER_WIFI
#include "nic/bow.h"
#endif

#include "linux/bug.h"
#include "linux/kmemleak.h"
#include "linux/kallsyms.h"
#include "linux/sched.h"
#if KERNEL_VERSION(4, 11, 0) <= CFG80211_VERSION_CODE
#include "uapi/linux/sched/types.h"
#endif

#if CFG_SUPPORT_SCAN_CACHE_RESULT
#include "wireless/core.h"
#endif

#if (CFG_VOLT_INFO == 1)
#include <linux/workqueue.h>
#endif

#if CFG_SUPPORT_RX_PAGE_POOL
#if KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE
#include <net/page_pool.h>
#else
#include <net/page_pool/helpers.h>
#endif
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

/* for sched_clock() */
#include <linux/sched/clock.h>

#include <linux/platform_device.h>

#if DBG
extern int allocatedMemSize;
#endif

extern struct semaphore g_halt_sem;
extern int g_u4HaltFlag;
extern int g_u4WlanInitFlag;

extern struct delayed_work sched_workq;
#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
extern unsigned int wlan_bat_volt;
extern bool fgIsTxPowerDecreased;
#endif

extern u_int8_t wlan_perf_monitor_force_enable;

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#ifdef CFG_STA_NUM
#define KAL_AIS_NUM CFG_STA_NUM
#else
#define KAL_AIS_NUM 2
#endif

#ifdef CFG_P2P_NUM
#define KAL_P2P_NUM CFG_P2P_NUM
#else
#define KAL_P2P_NUM 2
#endif

#define OID_HDLR_REC_NUM	5

#define SKB_RESERVED_SIZE	32


#if CFG_SUPPORT_MULTITHREAD
#define GLUE_FLAG_MAIN_PROCESS \
	(GLUE_FLAG_HALT | GLUE_FLAG_SUB_MOD_MULTICAST | \
	GLUE_FLAG_TX_CMD_DONE | GLUE_FLAG_TXREQ | GLUE_FLAG_TIMEOUT | \
	GLUE_FLAG_FRAME_FILTER | GLUE_FLAG_OID | GLUE_FLAG_RX | \
	GLUE_FLAG_SER_TIMEOUT | GLUE_FLAG_DISABLE_PERF)

#define GLUE_FLAG_HIF_PROCESS \
	(GLUE_FLAG_HALT | GLUE_FLAG_INT | GLUE_FLAG_HIF_TX | \
	GLUE_FLAG_HIF_TX_CMD | GLUE_FLAG_HIF_FW_OWN | \
	GLUE_FLAG_HIF_PRT_HIF_DBG_INFO | \
	GLUE_FLAG_UPDATE_WMM_QUOTA | \
	GLUE_FLAG_HIF_MDDP | \
	GLUE_FLAG_DRV_INT | \
	GLUE_FLAG_MGMT_DIRECT_HIF_TX | \
	GLUE_FLAG_SER_INT)

#define HIF_FLAG \
	(HIF_FLAG_AER_RESET | HIF_FLAG_MSI_RECOVERY | \
	HIF_FLAG_ALL_TOKENS_UNUSED)

#define GLUE_FLAG_RX_PROCESS (GLUE_FLAG_HALT | GLUE_FLAG_RX_TO_OS)
#else
/* All flags for single thread driver */
#define GLUE_FLAG_MAIN_PROCESS  0xFFFFFFFF
#endif

#define HIF_DETECT_TX_HANG_INTERVAL (10000)

#define PERF_MON_INIT_BIT       (0)
#define PERF_MON_DISABLE_BIT    (1)
#define PERF_MON_STOP_BIT       (2)
#define PERF_MON_RUNNING_BIT    (3)

#define PERF_MON_UPDATE_MIN_INTERVAL (500)
#define PERF_MON_UPDATE_INTERVAL (1000)
#define PERF_MON_TP_MAX_THRESHOLD (12)

#define PERF_MON_TP_CONDITION (125000)
#define PERF_MON_COEX_TP_THRESHOLD (100)
#define PERF_MON_MCC_TP_THRESHOLD	(50)

#define PERF_MON_RFB_MAX_THRESHOLD (3)

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

#define TX_LATENCY_STATS_MAX_AIR_DELAY_L1 (5)
#define TX_LATENCY_STATS_MAX_AIR_DELAY_L2 (10)
#define TX_LATENCY_STATS_MAX_AIR_DELAY_L3 (20)
#define TX_LATENCY_STATS_MAX_AIR_DELAY_L4 (50)

#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L1 (10)
#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L2 (20)
#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L3 (50)
#define TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L4 (80)

#if (CFG_COALESCING_INTERRUPT == 1)
#define COALESCING_INT_MAX_TIME (1) /* ms */
#define COALESCING_INT_MAX_PKT (50)
#endif

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
#endif

#if CFG_SUPPORT_SA_LOG
#define WIFI_LOG_MSG_MAX	(512)
#define WIFI_LOG_MSG_BUFFER	(WIFI_LOG_MSG_MAX * 2)
#endif

#define KAL_TRACE __builtin_return_address(0)

#define AXI_REMAP_SIZE		(1 * 1024 * 1024)
#define PCIE_REMAP_SZ		(64 * 1024)

#if defined(_HIF_PCIE)
#define BUS_REMAP_SIZE		PCIE_REMAP_SZ
#elif defined(_HIF_AXI)
#define BUS_REMAP_SIZE		AXI_REMAP_SIZE
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
#define PWR_LEVEL_STAT_UPDATE_INTERVAL	60	/* sec */
#endif

/* Scan Request Timeout */
#if KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
#define KAL_SCN_BSS_DESC_STALE_SEC			30
#else
#define KAL_SCN_BSS_DESC_STALE_SEC			20
#endif

#if (CFG_VOLT_INFO == 1)
#define VOLT_INFO_DEBOUNCE_TIMES_MAX 100
#define VOLT_INFO_DEBOUNCE_TIMES 3
#define VOLT_INFO_DEBOUNCE_INTERVAL 2000 /* ms */
#define VOLT_INFO_DELTA 50 /* mV */
/* align pmic_lbat_service.c THD_VOLT_MAX */
#define VOLT_INFO_MAX_VOLT_THRESH 5400 /* mV */
/* align pmic_lbat_service.c THD_VOLT_MIN */
#define VOLT_INFO_MIN_VOLT_THRESH 2650 /* mV */
#define VOLT_INFO_LOW_BOUND_UNLMT 0
#define VOLT_INFO_LOW_BOUND 3850 /* mV */
#endif

#if CFG_SUPPORT_PCIE_GEN_SWITCH
#define PCIE_GEN_SWITCH_MONITOR_TIMES_MAX 1000000
#endif

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
#define MAWD_VER_NONE	0
#define MAWD_VER_1_0	1
#define MAWD_VER_1_1	2
#endif

#if CFG_SUPPORT_RX_PAGE_POOL
#define PAGE_POOL_MAX_MEM_SIZE		(0x8000000)
#define PAGE_POOL_NUM_SHIFT		(2)
#define PAGE_POOL_NUM			((1 << PAGE_POOL_NUM_SHIFT) + 1)
#define PAGE_POOL_MAX_SIZE \
	(PAGE_POOL_MAX_MEM_SIZE >> (PAGE_SHIFT + PAGE_POOL_NUM_SHIFT))
#define PAGE_POOL_LAST_IDX		(PAGE_POOL_NUM - 1)
#endif

/* OID waiting time (12s) */
#define KAL_OID_WAIT_TIME		(WLAN_OID_TIMEOUT_THRESHOLD + 10000)

#if CFG_SUPPORT_HIF_REG_WORK
#define CFG_HIF_REG_MAX_REQ_NUM		100
#define CFG_HIF_REG_WORK_TIMEOUT_TIME	1000   /* 1ms */
#define CFG_HIF_REG_WORK_TIMEOUT_CNT	5000   /* 5s */
#define CFG_HIF_REG_REQ_TIMEOUT_TIME	1000   /* 1ms */
#define CFG_HIF_REG_REQ_TIMEOUT_CNT	5000   /* 5s */
#endif /* CFG_SUPPORT_HIF_REG_WORK */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct CPU_INFO {
	int32_t i4LittleCpuFreq;
	int32_t i4BigCpuFreq;
};

struct THREAD_INFO {
	uint32_t u4CpuMask;
	uint32_t u4Priority;
};

enum CPU_CORE_TYPE {
	CPU_CORE_NONE = 0,
	CPU_CORE_LITTLE,
	CPU_CORE_BIG
};

struct BOOST_INFO {
	struct CPU_INFO rCpuInfo;
	struct THREAD_INFO rHifThreadInfo;
	struct THREAD_INFO rMainThreadInfo;
	struct THREAD_INFO rRxThreadInfo;
	struct THREAD_INFO rRxNapiThreadInfo;
	struct THREAD_INFO rHifNapiThreadInfo;
	uint32_t u4RpsMap;
	uint32_t u4ISRMask;
	int32_t i4RxRfbRetWorkCpu;
	int32_t i4TxWorkCpu;
	int32_t i4RxWorkCpu;
	int32_t i4RxNapiWorkCpu;
	int32_t i4TxFreeMsduWorkCpu;
	int32_t i4DramBoostLv;
	u_int8_t fgKeepPcieWakeup;
	uint32_t u4WfdmaTh;
	u_int8_t fgWifiNappingForceDis;
	enum CPU_CORE_TYPE eSkbAllocWorkCoreType;
	enum CPU_CORE_TYPE eTxFreeSkbWorkCoreType;
};

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
#if CFG_SUPPORT_TPENHANCE_MODE
	SPIN_LOCK_TXACK_QUE,
#endif /* CFG_SUPPORT_TPENHANCE_MODE */
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
	SPIN_LOCK_TX_DESC,
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

	SPIN_LOCK_BOW_TABLE,

	SPIN_LOCK_EHPI_BUS,	/* only for EHPI */
	SPIN_LOCK_NET_DEV,

	SPIN_LOCK_BSSLIST_FW,
	SPIN_LOCK_BSSLIST_CFG,
	SPIN_LOCK_SET_OWN,
#if CFG_SUPPORT_NAN
	SPIN_LOCK_NAN_NEGO_CRB,
	SPIN_LOCK_NAN_NDL_FLOW_CTRL,
#endif
#if (CFG_CE_ASSERT_DUMP == 1)
	SPIN_LOCK_CORE_DUMP,
#endif
#if CFG_CHIP_RESET_SUPPORT
	SPIN_LOCK_WFSYS_RESET,
#endif
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	SPIN_LOCK_TX_MGMT_DIRECT_Q,
#endif
#if CFG_MTK_WIFI_PCIE_SUPPORT
	SPIN_LOCK_PCIE_VOTE,
#endif
	SPIN_LOCK_HIF_REMAP,
	SPIN_LOCK_PMKID,
	SPIN_LOCK_MSDUIFO,
	SPIN_LOCK_DYNAMIC_RFB,
#if CFG_SUPPORT_MBRAIN
	SPIN_LOCK_MBR_TXTIMEOUT,
#endif
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
	MUTEX_DYNAMIC_RFB,
	MUTEX_HQA_TEST,
	MUTEX_NUM
};

/* event for assoc information update */
struct EVENT_ASSOC_INFO {
	uint8_t ucAssocReq;	/* 1 for assoc req, 0 for assoc rsp */
	uint8_t ucReassoc;	/* 0 for assoc, 1 for reassoc */
	uint16_t u2Length;
	uint8_t *pucIe;
};

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

enum ENUM_KAL_MEM_ALLOCATION_TYPE_E {
	PHY_MEM_TYPE,		/* physically continuous */
	VIR_MEM_TYPE,		/* virtually continuous */
	MEM_TYPE_NUM
};

#if CFG_MTK_ANDROID_WMT
#if (KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE)
#define KAL_WAKE_LOCK_T struct wakeup_source
#elif (KERNEL_VERSION(4, 9, 0) <= CFG80211_VERSION_CODE)
#define KAL_WAKE_LOCK_T struct wakeup_source
#else
#define KAL_WAKE_LOCK_T struct wake_lock
#endif
#else
#define KAL_WAKE_LOCK_T uint32_t
#endif

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

struct KAL_HALT_CTRL_T {
	struct semaphore lock;
	struct task_struct *owner;
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

enum ENUM_WLAN_FB_EVENT {
	WLAN_FB_EVENT_IGNORE,
	WLAN_FB_EVENT_POWERDOWN, /* screen on */
	WLAN_FB_EVENT_UNBLANK,   /* screen off */
	WLAN_FB_EVENT_NUM
};

#if CFG_SUPPORT_DATA_STALL
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

#if (CFG_WLAN_ATF_SUPPORT == 1)
enum ENUM_WLAN_SMC_OPID {
	/* init, on, off */
	SMC_WLAN_PCCIF_ON_OPID = 3,
	SMC_WLAN_PCCIF_OFF_OPID = 4,
	SMC_WLAN_ENABLE_FWDL_MODE_OPID = 5,

	/* add new module here */
	/* set OPID +1000 based on previous module */

	/* general register access */
	SMC_WLAN_DEV_REG_WR_CR_OPID = 9998,
	SMC_WLAN_IOREMAP_WR_CR_OPID = 9999,
};

enum ENUM_SMC_WLAN_STATUS_CODE {
	SMC_WLAN_SUCCESS = 0,
	SMC_WLAN_UNKNOWN_OPID = 1,
	SMC_WLAN_INVALID_REGISTER = 2,
};

#endif

typedef int(*PFN_PWR_LEVEL_HANDLER)(struct ADAPTER *, uint8_t);

struct PWR_LEVEL_HANDLER_ELEMENT {
	struct LINK_ENTRY rLinkEntry;
	PFN_PWR_LEVEL_HANDLER prPwrLevelHandler;
};

#if (CFG_VOLT_INFO == 1)
enum ENUM_VOLT_INFO_STATE {
	VOLT_INFO_STATE_INIT = 0,
	VOLT_INFO_STATE_IN_PROGRESS,
	VOLT_INFO_STATE_COMPLETE,
	VOLT_INFO_STATE_NUM
};

struct VOLT_INFO_DEBOUNCE_PARAMETER_T {
	uint32_t u4Total;
	uint32_t u4Cnt;
};

struct VOLT_INFO_BATTERY_NOTIFY_T {
	struct lbat_user *lbat_pt;
	bool fgReg;
};

struct VOLT_INFO_T {
	uint32_t u4CurrVolt;
	struct VOLT_INFO_DEBOUNCE_PARAMETER_T rDebParam;
	struct VOLT_INFO_BATTERY_NOTIFY_T rBatNotify;
	struct ADAPTER *prAdapter;
	struct delayed_work dwork;
	struct delayed_work dBatWork;
	struct mutex rMutex;
	enum ENUM_VOLT_INFO_STATE eState;
};
#endif /* CFG_VOLT_INFO */

#if CFG_NEW_HIF_DEV_REG_IF
enum HIF_DEV_REG_REASON {
	/* HIF_DEV_REG_{module}_{reason} */
	HIF_DEV_REG_HIF_READ,
	HIF_DEV_REG_HIF_RING,
	HIF_DEV_REG_HIF_DBG,
	HIF_DEV_REG_HIF_EXTDBG,
	HIF_DEV_REG_HIF_CONNAC1_2,
	HIF_DEV_REG_HIF_USB,
	HIF_DEV_REG_HIF_BT_DBG,
	HIF_DEV_REG_ONOFF_READ,
	HIF_DEV_REG_ONOFF_DBG,
	HIF_DEV_REG_RESET_READ,
	HIF_DEV_REG_COREDUMP_DBG,
	HIF_DEV_REG_LPOWN_READ,
	HIF_DEV_REG_SER_READ,
	HIF_DEV_REG_OFFLOAD_READ,
	HIF_DEV_REG_OFFLOAD_HOST,
	HIF_DEV_REG_OFFLOAD_DBG,
	HIF_DEV_REG_CCIF_READ,
	HIF_DEV_REG_PLAT_DBG,
	HIF_DEV_REG_WTBL_DBG,
	HIF_DEV_REG_OID_DBG,
	HIF_DEV_REG_PCIEASPM_READ,
	HIF_DEV_REG_NOMMIO_DBG,
	HIF_DEV_REG_MAX
};
#endif /* CFG_NEW_HIF_DEV_REG_IF */

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
#define KAL_SET_BIT(bitOffset, value)      set_bit(bitOffset, &value)
#define KAL_CLR_BIT(bitOffset, value)      clear_bit(bitOffset, &value)
#define KAL_TEST_AND_CLEAR_BIT(bitOffset, value)  \
	test_and_clear_bit(bitOffset, &value)
#define KAL_TEST_BIT(bitOffset, value)     test_bit(bitOffset, &value)
#define SUSPEND_FLAG_FOR_WAKEUP_REASON	(0)
#define SUSPEND_FLAG_CLEAR_WHEN_RESUME	(1)
#define KAL_HEX_DUMP_TO_BUFFER(_buf, _len, _rowsize, _groupsize, _linebuf, \
	_linebuflen, _ascii) \
	hex_dump_to_buffer(_buf, _len, _rowsize, _groupsize, _linebuf, \
	_linebuflen, _ascii)

#define KAL_WARN_ON WARN_ON
#define KAL_IS_ERR IS_ERR
#define KAL_MIN min
/*----------------------------------------------------------------------------*/
/* Macros of getting current thread id                                        */
/*----------------------------------------------------------------------------*/
#define KAL_GET_CURRENT_THREAD_ID() (current->pid)
#define KAL_GET_CURRENT_THREAD_NAME() (current->comm)

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

#define KAL_ACQUIRE_SPIN_LOCK_BH(_prAdapter, _rLockCategory)   \
	kalAcquireSpinLockBh(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
	_rLockCategory)

#define KAL_RELEASE_SPIN_LOCK_BH(_prAdapter, _rLockCategory)   \
	kalReleaseSpinLockBh(((struct ADAPTER *)_prAdapter)->prGlueInfo,  \
	_rLockCategory)

#if defined(_HIF_USB)
#define KAL_HIF_STATE_LOCK(prGlueInfo) \
	kalAcquiretHifStateLock(prGlueInfo, &__ulFlags)

#define KAL_HIF_STATE_UNLOCK(prGlueInfo) \
	kalReleaseHifStateLock(prGlueInfo, __ulFlags)
#endif

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
#if KERNEL_VERSION(3, 14, 0) > CFG80211_VERSION_CODE
#define IEEE80211_CHAN_PASSIVE_FLAG	IEEE80211_CHAN_PASSIVE_SCAN
#define IEEE80211_CHAN_PASSIVE_STR		"PASSIVE"
#else
#define IEEE80211_CHAN_PASSIVE_FLAG	IEEE80211_CHAN_NO_IR
#define IEEE80211_CHAN_PASSIVE_STR		"NO_IR"
#endif

#if KERNEL_VERSION(4, 7, 0) <= CFG80211_VERSION_CODE
/**
 * enum nl80211_band - Frequency band
 * @NL80211_BAND_2GHZ: 2.4 GHz ISM band
 * @NL80211_BAND_5GHZ: around 5 GHz band (4.9 - 5.7 GHz)
 * @NL80211_BAND_60GHZ: around 60 GHz band (58.32 - 64.80 GHz)
 * @NUM_NL80211_BANDS: number of bands, avoid using this in userspace
 *	 since newer kernel versions may support more bands
 */
#define KAL_BAND_2GHZ NL80211_BAND_2GHZ
#define KAL_BAND_5GHZ NL80211_BAND_5GHZ
#if (CFG_SUPPORT_WIFI_6G == 1)
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#define KAL_BAND_6GHZ NL80211_BAND_6GHZ
#else
/*#pragma message("WARNING!! Kernel version " \
 *	STR(CFG80211_VERSION_CODE) " is too old to support 6GHZ.")
 */
#define KAL_BAND_6GHZ NL80211_BAND_60GHZ /* for build pass only */
#endif
#endif
#define KAL_NUM_BANDS NUM_NL80211_BANDS
#else
#define KAL_BAND_2GHZ IEEE80211_BAND_2GHZ
#define KAL_BAND_5GHZ IEEE80211_BAND_5GHZ
#define KAL_BAND_6GHZ NL80211_BAND_60GHZ /* for build pass only */
#define KAL_NUM_BANDS IEEE80211_NUM_BANDS
#endif

/**
 * enum nl80211_reg_rule_flags - regulatory rule flags
 * @NL80211_RRF_NO_OFDM: OFDM modulation not allowed
 * @NL80211_RRF_AUTO_BW: maximum available bandwidth should be calculated
 *  base on contiguous rules and wider channels will be allowed to cross
 *  multiple contiguous/overlapping frequency ranges.
 * @NL80211_RRF_DFS: DFS support is required to be used
 */
#define KAL_RRF_NO_OFDM NL80211_RRF_NO_OFDM
#define KAL_RRF_DFS     NL80211_RRF_DFS
#if KERNEL_VERSION(3, 15, 0) > CFG80211_VERSION_CODE
#define KAL_RRF_AUTO_BW 0
#else
#define KAL_RRF_AUTO_BW NL80211_RRF_AUTO_BW
#endif

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
#if KERNEL_VERSION(4, 8, 0) <= CFG80211_VERSION_CODE
static inline void kalCfg80211ScanDone(struct cfg80211_scan_request *request,
				       bool aborted)
{
	struct cfg80211_scan_info info = {.aborted = aborted };

	cfg80211_scan_done(request, &info);
}
#else
static inline void kalCfg80211ScanDone(struct cfg80211_scan_request *request,
				       bool aborted)
{
	cfg80211_scan_done(request, aborted);
}
#endif

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

#if KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE
static inline struct sk_buff *
kalCfg80211VendorEventAlloc(struct wiphy *wiphy, struct wireless_dev *wdev,
				 int approxlen, int event_idx, gfp_t gfp)
{
	return cfg80211_vendor_event_alloc(wiphy, wdev,
					approxlen, event_idx, gfp);
}
#else
static inline struct sk_buff *
kalCfg80211VendorEventAlloc(struct wiphy *wiphy, struct wireless_dev *wdev,
				 int approxlen, int event_idx, gfp_t gfp)
{
	return cfg80211_vendor_event_alloc(wiphy,
					approxlen, event_idx, gfp);
}
#endif

static inline void kalCfg80211VendorEvent(void *pvPacket)
{
	struct sk_buff *pkt = (struct sk_buff *)pvPacket;

	return cfg80211_vendor_event(pkt, GFP_KERNEL);
}

/* Consider on some Android platform, using request_firmware_direct()
 * may cause system failed to load firmware. So we still use
 * request_firmware().
 */
#define REQUEST_FIRMWARE(_fw, _name, _dev) \
	request_firmware(_fw, _name, _dev)

/*----------------------------------------------------------------------------*/
/* Macros of wake_lock operations for using in Driver Layer                   */
/*----------------------------------------------------------------------------*/
#if CFG_ENABLE_WAKE_LOCK
/* CFG_ENABLE_WAKE_LOCK is defined in makefile */
#if (KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE)
#if (KERNEL_VERSION(4, 14, 149) <= LINUX_VERSION_CODE)
#define KAL_WAKE_LOCK_INIT(_prAdapter, _prWakeLock, _pcName) \
	_prWakeLock = wakeup_source_register(NULL, _pcName);

#define KAL_WAKE_LOCK_DESTROY(_prAdapter, _prWakeLock) \
{ \
	if (_prWakeLock) { \
		wakeup_source_unregister(_prWakeLock); \
		_prWakeLock = NULL; \
	} \
}
#else
#define KAL_WAKE_LOCK_INIT(_prAdapter, _prWakeLock, _pcName) \
{ \
	_prWakeLock = kalMemAlloc(sizeof(KAL_WAKE_LOCK_T), \
		VIR_MEM_TYPE); \
	if (!_prWakeLock) { \
		DBGLOG(HAL, ERROR, \
			"KAL_WAKE_LOCK_INIT init fail!\n"); \
	} \
	else { \
		wakeup_source_init(_prWakeLock, _pcName); \
	} \
}

#define KAL_WAKE_LOCK_DESTROY(_prAdapter, _prWakeLock) \
{ \
	if (_prWakeLock) { \
		wakeup_source_trash(_prWakeLock); \
		_prWakeLock = NULL; \
	} \
}
#endif
#define KAL_WAKE_LOCK(_prAdapter, _prWakeLock) \
{ \
	if (_prWakeLock) { \
		__pm_stay_awake(_prWakeLock); \
	} \
}

#define KAL_WAKE_LOCK_TIMEOUT(_prAdapter, _prWakeLock, _u4Timeout) \
{ \
	if (_prWakeLock) { \
		__pm_wakeup_event(_prWakeLock, JIFFIES_TO_MSEC(_u4Timeout)); \
	} \
}

#define KAL_WAKE_UNLOCK(_prAdapter, _prWakeLock) \
{ \
	if (_prWakeLock) { \
		__pm_relax(_prWakeLock); \
	} \
}

#define KAL_WAKE_LOCK_ACTIVE(_prAdapter, _prWakeLock) \
	((_prWakeLock) && ((_prWakeLock)->active))

#else
#define KAL_WAKE_LOCK_INIT(_prAdapter, _prWakeLock, _pcName) \
{ \
	_prWakeLock = kalMemAlloc(sizeof(KAL_WAKE_LOCK_T), \
		VIR_MEM_TYPE); \
	if (!_prWakeLock) { \
		DBGLOG(HAL, ERROR, \
			"KAL_WAKE_LOCK_INIT init fail!\n"); \
	} \
	else { \
		wake_lock_init(_prWakeLock, WAKE_LOCK_SUSPEND, _pcName); \
	} \
}

#define KAL_WAKE_LOCK_DESTROY(_prAdapter, _prWakeLock) \
{ \
	if (_prWakeLock) { \
		wake_lock_destroy(_prWakeLock); \
	} \
}

#define KAL_WAKE_LOCK(_prAdapter, _prWakeLock) \
{ \
	if (_prWakeLock) { \
		wake_lock(_prWakeLock); \
	} \
}

#define KAL_WAKE_LOCK_TIMEOUT(_prAdapter, _prWakeLock, _u4Timeout) \
{ \
	if (_prWakeLock) { \
		wake_lock_timeout(_prWakeLock, _u4Timeout); \
	} \
}

#define KAL_WAKE_UNLOCK(_prAdapter, _prWakeLock) \
{ \
	if (_prWakeLock) { \
		wake_unlock(_prWakeLock); \
	} \
}

#define KAL_WAKE_LOCK_ACTIVE(_prAdapter, _prWakeLock) \
		((_prWakeLock) && wake_lock_active(_prWakeLock))
#endif

#else
#define KAL_WAKE_LOCK_INIT(_prAdapter, _prWakeLock, _pcName)
#define KAL_WAKE_LOCK_DESTROY(_prAdapter, _prWakeLock)
#define KAL_WAKE_LOCK(_prAdapter, _prWakeLock)
#define KAL_WAKE_LOCK_TIMEOUT(_prAdapter, _prWakeLock, _u4Timeout)
#define KAL_WAKE_UNLOCK(_prAdapter, _prWakeLock)
#define KAL_WAKE_LOCK_ACTIVE(_prAdapter, _prWakeLock)
#endif

#define KAL_FIFO_INIT(_prFiFoQ, _prBuf, _rBufLen) \
	kfifo_init((_prFiFoQ), (_prBuf), _rBufLen)
#define KAL_FIFO_IN(_prFiFoQ, _rObj) \
	kfifo_in((_prFiFoQ), &(_rObj), sizeof(_rObj))
#define KAL_FIFO_OUT(_prFiFoQ, _rObj) \
	kfifo_out((_prFiFoQ), &(_rObj), sizeof(_rObj))
#define KAL_FIFO_IN_LOCKED(_prFiFoQ, _rObj, _lock) \
	kfifo_in_locked((_prFiFoQ), &(_rObj), sizeof(_rObj), (_lock))
#define KAL_FIFO_OUT_LOCKED(_prFiFoQ, _rObj, _lock) \
	kfifo_out_locked((_prFiFoQ), &(_rObj), sizeof(_rObj), (_lock))
#define KAL_FIFO_LEN(_prFiFoQ) \
	kfifo_len((_prFiFoQ))
#define KAL_FIFO_AVAIL(_prFiFoQ) \
	kfifo_avail((_prFiFoQ))
#define KAL_FIFO_IS_EMPTY(_prFiFoQ) \
	kfifo_is_empty((_prFiFoQ))
#define KAL_FIFO_IS_FULL(_prFiFoQ) \
	kfifo_is_full((_prFiFoQ))
#if CFG_SUPPORT_RX_NAPI
#define KAL_GET_FIFO_CNT(_prGlueInfo) \
	((unsigned int) (KAL_FIFO_LEN((&_prGlueInfo->rRxKfifoQ)) \
		/ sizeof(void *)))
#else
#define KAL_GET_FIFO_CNT(_prGlueInfo) (0)
#endif


#define KAL_MB_RW() \
({ \
	/* Avoid memory barrier problem  */ \
	smp_mb(); \
})

#define KAL_MB_W() \
({ \
	/* Avoid memory barrier problem  */ \
	smp_wmb(); \
})

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
		if (u4Size > PAGE_SIZE) \
			pvAddr = vmalloc(u4Size);   \
		else \
			pvAddr = kmalloc(u4Size, GFP_KERNEL);   \
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
		if (u4Size > PAGE_SIZE) \
			pvAddr = vmalloc(u4Size);   \
		else \
			pvAddr = kmalloc(u4Size, GFP_KERNEL);   \
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
	kvfree(pvAddr); \
}
#else
#define kalMemFree(pvAddr, eMemType, u4Size)  \
{   \
	kvfree(pvAddr); \
}
#endif

#define kalMemZFree(pvAddr, eMemType, u4Size) \
{ \
	kalMemSet(pvAddr, 0, u4Size); \
	kalMemFree(pvAddr, eMemType, u4Size); \
}

#define kalUdelay(u4USec)                           udelay(u4USec)
#define kalMdelay(u4MSec)                           mdelay(u4MSec)
#define kalMsleep(u4MSec)                           msleep(u4MSec)
#define kalUsleep(u4USec) \
{ \
	if (u4USec > 10000) \
		msleep(u4USec / 1000); \
	else \
		usleep_range(u4USec, u4USec + 50); \
}
#define kalUsleep_range(u4MinUSec, u4MaxUSec)  \
	usleep_range(u4MinUSec, u4MaxUSec)

/* Copy memory from user space to kernel space */
#define kalMemCopyFromUser(_pvTo, _pvFrom, _u4N)  \
	copy_from_user(_pvTo, _pvFrom, _u4N)

/* Copy memory from kernel space to user space */
#define kalMemCopyToUser(_pvTo, _pvFrom, _u4N)  \
	copy_to_user(_pvTo, _pvFrom, _u4N)

/* Copy memory block with specific size */
#define kalMemCopy(pvDst, pvSrc, u4Size)  \
	memcpy(pvDst, pvSrc, u4Size)

/* Set memory block with specific pattern */
#define kalMemSet(pvAddr, ucPattern, u4Size)  \
	memset(pvAddr, ucPattern, u4Size)

/* Copy memory block from IO memory */
#define kalMemCopyFromIo(pvDst, pvSrc, u4Size)  \
	memcpy_fromio(pvDst, pvSrc, u4Size)

/* Copy memory block to IO memory */
#define kalMemCopyToIo(pvDst, pvSrc, u4Size)  \
	memcpy_toio(pvDst, pvSrc, u4Size)

/* Set memory block to IO memory */
#define kalMemSetIo(pvAddr, ucPattern, u4Size)  \
	memset_io(pvAddr, ucPattern, u4Size)

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

#if KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE
#define strnicmp(s1, s2, n)                         strncasecmp(s1, s2, n)
#endif

/* string operation */
#define kalStrCpy(dest, src)               strcpy(dest, src)
#define kalStrnCpy(dest, src, n)           strncpy(dest, src, n)
#define kalStrCmp(ct, cs)                  strcmp(ct, cs)
#define kalStrnCmp(ct, cs, n)              strncmp(ct, cs, n)
#define kalStrChr(s, c)                    strchr(s, c)
#define kalStrrChr(s, c)                   strrchr(s, c)
#define kalStrnChr(s, n, c)                strnchr(s, n, c)
#define kalStrLen(s)                       strlen(s)
#define kalStrnLen(s, b)                   strnlen(s, b)
#define kalStrniCmp(ct, cs, n)             strncasecmp(ct, cs, n)
/* #define kalStrtoul(cp, endp, base)      simple_strtoul(cp, endp, base) */
/* #define kalStrtol(cp, endp, base)       simple_strtol(cp, endp, base) */
#define kalkStrtou8(cp, base, resp)        kstrtou8(cp, base, resp)
#define kalkStrtou16(cp, base, resp)       kstrtou16(cp, base, resp)
#define kalkStrtou32(cp, base, resp)       kstrtou32(cp, base, resp)
#define kalkStrtos32(cp, base, resp)       kstrtos32(cp, base, resp)
#define kalSnprintf(buf, size, fmt, ...)   \
	_kalSnprintf((char *)(buf), (size_t)(size), \
		(const char *)(fmt), ##__VA_ARGS__)
#define kalScnprintf(buf, size, fmt, ...)  \
	scnprintf(buf, size, fmt, ##__VA_ARGS__)
#define kalVsnprintf(buf, size, fmt, args)          \
	vsnprintf(buf, size, fmt, args)
/* remove for AOSP */
/* #define kalSScanf(buf, fmt, ...)        sscanf(buf, fmt, __VA_ARGS__) */
#define kalStrStr(ct, cs)                  strstr(ct, cs)
#define kalStrSep(s, ct)                   strsep(s, ct)
#define kalStrCat(dest, src)               strcat(dest, src)
#define kalStrnCat(dst, src, n)            strncat(dst, src, n)
#define kalIsXdigit(c)                     isxdigit(c)
#define kalStrtoint(_data, _base, _res) kstrtoint(_data, _base, _res)
#define kalStrtouint(_data, _base, _res) kstrtouint(_data, _base, _res)
#define kalStrtoul(_data, _base, _res) kstrtoul(_data, _base, _res)
char *strtok_r(char *s, const char *delim, char **last);
#define kalStrtokR(_buf, _tok, _saved) \
	strtok_r(_buf, _tok, _saved)

#define kalFfs ffs

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
/* defined for wince sdio driver only */
#if defined(_HIF_SDIO)
#define kalDevSetPowerState(prGlueInfo, ePowerMode) \
	glSetPowerState(prGlueInfo, ePowerMode)
#else
#define kalDevSetPowerState(prGlueInfo, ePowerMode)
#endif

#if CFG_MTK_ANDROID_WMT
#define _kalRequestFirmware request_firmware
#else
#define _kalRequestFirmware request_firmware
#endif
#define kal_init_completion(rComp)  init_completion(rComp)

#define kal_completion_done(rComp)  completion_done(rComp)

#define kal_reinit_completion(rComp)  reinit_completion(rComp)

#define kal_completion struct completion

#define kal_timer_list struct timer_list

#define kal_kmemleak_ignore(pucAssocIEs) \
	kmemleak_ignore(pucAssocIEs)

#define kal_ieee80211_get_channel(_rWiphy, _freq) \
	ieee80211_get_channel(_rWiphy, _freq)

#define kal_ieee80211_channel_to_frequency(_ch, _band) \
	ieee80211_channel_to_frequency(_ch, _band)

#define kal_max_t(_type, _v1, _v2) \
	max_t(_type, _v1, _v2)

#define kal_min_t(_type, _v1, _v2) \
	min_t(_type, _v1, _v2)

#define kal_tasklet_schedule(_rTasklet) \
	tasklet_schedule(_rTasklet)

#define kal_tasklet_hi_schedule(_rTasklet) \
	tasklet_hi_schedule(_rTasklet)

#define kalIsZeroEtherAddr(_addr) \
	is_zero_ether_addr(_addr)

#define kal_max_t(_type, _v1, _v2) max_t(_type, _v1, _v2)
#define kal_min_t(_type, _v1, _v2) min_t(_type, _v1, _v2)

/*----------------------------------------------------------------------------*/
/*!
 * \brief Notify OS with SendComplete event of the specific packet.
 *        Linux should free packets here.
 *
 * \param[in] prGlueInfo     Pointer of GLUE Data Structure
 * \param[in] pvPacket       Pointer of Packet Handle
 * \param[in] u4Status       Status Code for OS upper layer
 *
 * \return -
 */
/*----------------------------------------------------------------------------*/
void kalSendComplete(struct GLUE_INFO *prGlueInfo, void *pvPacket,
	uint32_t u4Status);

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
#define kalQueryBufferPointer(prGlueInfo, pvPacket)     \
	    ((uint8_t *)((struct sk_buff *)pvPacket)->data)

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
#define kalQueryValidBufferLength(prGlueInfo, pvPacket)     \
	    ((uint32_t)((struct sk_buff *)pvPacket)->end -  \
	     (uint32_t)((struct sk_buff *)pvPacket)->data)

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
#define kalCopyFrame(prGlueInfo, pvPacket, pucDestBuffer)   \
	do {struct sk_buff *skb = (struct sk_buff *)pvPacket; \
	    memcpy(pucDestBuffer, skb->data, skb->len); } while (0)

#define kalGetJiffies()                (jiffies)
#define kalGetTimeTick()                jiffies_to_msecs(jiffies)

#define kalGetTimeTickNs()              sched_clock()

#if CFG_SUPPORT_SA_LOG
#define kalPrintSALogLimited(fmt, ...)					\
({									\
	static DEFINE_RATELIMIT_STATE(_rs,				\
		DEFAULT_RATELIMIT_INTERVAL, DEFAULT_RATELIMIT_BURST);	\
	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);		\
									\
	if (__ratelimit(&_rs))						\
		kalPrintSALog(fmt, ##__VA_ARGS__);			\
})
#endif

#ifdef CFG_COMBO_SLT_GOLDEN
#define WLAN_TAG                        "[wlan_golden]"
#else
#define WLAN_TAG                        "[wlan]"
#endif

#if CFG_SUPPORT_SA_LOG
#define kalPrint(_Fmt...) \
	((get_wifi_standalone_log_mode() == 1) \
	? kalPrintSALog(WLAN_TAG _Fmt) \
	: pr_info(WLAN_TAG _Fmt))
#define kalPrintLimited(_Fmt...) \
	((get_wifi_standalone_log_mode() == 1) \
	? kalPrintSALogLimited(WLAN_TAG _Fmt) \
	: pr_info_ratelimited(WLAN_TAG _Fmt))
#else
#define kalPrint(_Fmt...)               pr_info(WLAN_TAG _Fmt)
#define kalPrintLimited(_Fmt...)        pr_info_ratelimited(WLAN_TAG _Fmt)
#endif

#define kalBreakPoint() \
do { \
	WARN_ON(1); \
	panic("Oops"); \
} while (0)

#if CFG_ENABLE_AEE_MSG
#define kalSendAeeException                         aee_kernel_exception
#define kalSendAeeWarning                           aee_kernel_warning
#define kalSendAeeReminding                         aee_kernel_reminding
#else
#define kalSendAeeException(_module, _desc, ...)
#define kalSendAeeWarning(_module, _desc, ...)
#define kalSendAeeReminding(_module, _desc, ...)
#endif

#define PRINTF_ARG(...)      __VA_ARGS__
#define SPRINTF(buf, arg)    {buf += sprintf((char *)(buf), PRINTF_ARG arg); }

#define USEC_TO_SYSTIME(_usec)      ((_usec) / USEC_PER_MSEC)
#define MSEC_TO_SYSTIME(_msec)      (_msec)

#define MSEC_TO_JIFFIES(_msec)      msecs_to_jiffies(_msec)
#define JIFFIES_TO_MSEC(_jiffie)    jiffies_to_msecs(_jiffie)

#if KERNEL_VERSION(5, 1, 0) <= CFG80211_VERSION_CODE
#define get_ds() KERNEL_DS
#endif

#if KERNEL_VERSION(5, 0, 0) <= CFG80211_VERSION_CODE
#define kal_access_ok(type, addr, size) access_ok(addr, size)
#else
#define kal_access_ok(type, addr, size) access_ok(type, addr, size)
#endif

#if KERNEL_VERSION(3, 17, 0) <= CFG80211_VERSION_CODE
#define ktime_get_ts64 ktime_get_real_ts64
#else
#define timespec64 timespec
#define ktime_get_ts64 ktime_get_real_ts
#define ktime_get_real_ts64 ktime_get_real_ts
#define rtc_time64_to_tm rtc_time_to_tm
#endif

#if KERNEL_VERSION(4, 18, 0) <= CFG80211_VERSION_CODE
/* ktime_get_boottime_ts64 defined in kernel */
#elif KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
#define ktime_get_boottime_ts64 get_monotonic_boottime64
#else
#define ktime_get_boottime_ts64 get_monotonic_boottime
#endif

#define KAL_GET_USEC(_time) ((uint32_t)NSEC_TO_USEC(_time.tv_nsec))
#define KAL_SET_MSEC_TO_TIME(_Time, _ms)\
	do { \
		_Time.tv_sec = MSEC_TO_SEC(_ms); \
		_Time.tv_nsec = MSEC_TO_USEC((_ms) % MSEC_PER_SEC) \
			* NSEC_PER_USEC; \
	} while (0)

#define KAL_TIME_TO_MSEC(_Time) (\
	(uint64_t)(_Time.tv_sec * MSEC_PER_SEC + \
	USEC_TO_MSEC(NSEC_TO_USEC(_Time.tv_nsec))))

#define KAL_TIME_INTERVAL_DECLARATION()     struct timespec64 __rTs, __rTe
#define KAL_REC_TIME_START()                ktime_get_ts64(&__rTs)
#define KAL_REC_TIME_END()                  ktime_get_ts64(&__rTe)
#define KAL_GET_TIME_INTERVAL() \
	((SEC_TO_USEC(__rTe.tv_sec) + KAL_GET_USEC(__rTe)) - \
	(SEC_TO_USEC(__rTs.tv_sec) + KAL_GET_USEC(__rTs)))
#define KAL_ADD_TIME_INTERVAL(_Interval) \
	do { \
		(_Interval) += KAL_GET_TIME_INTERVAL(); \
	} while (0)

#if defined(_HIF_PCIE)
#define KAL_DMA_TO_DEVICE	DMA_TO_DEVICE
#define KAL_DMA_FROM_DEVICE	DMA_FROM_DEVICE

#define KAL_DMA_ALLOC_COHERENT(_dev, _size, _handle) \
	dma_alloc_coherent(&((struct pci_dev *)(_dev))->dev, \
			_size, _handle, GFP_ATOMIC)
#define KAL_DMA_FREE_COHERENT(_dev, _size, _addr, _handle) \
	dma_free_coherent(&((struct pci_dev *)(_dev))->dev, \
			_size, _addr, _handle)
#define KAL_DMA_MAP_SINGLE(_dev, _ptr, _size, _dir) \
	dma_map_single(&((struct pci_dev *)(_dev))->dev, _ptr, _size, _dir)
#define KAL_DMA_UNMAP_SINGLE(_dev, _addr, _size, _dir) \
	dma_unmap_single(&((struct pci_dev *)(_dev))->dev, _addr, _size, _dir)
#define KAL_DMA_MAPPING_ERROR(_dev, _addr) \
	dma_mapping_error(&((struct pci_dev *)(_dev))->dev, _addr)

#if CFG_SUPPORT_WED_PROXY
#define KAL_DMA_MAP_SINGLE_ATTRS(_dev, _ptr, _size, _dir) \
	dma_map_single_attrs(&((struct pci_dev *)(_dev))->dev, \
	_ptr, _size, _dir, DMA_ATTR_SKIP_CPU_SYNC)
#define KAL_DMA_UNMAP_SINGLE_ATTRS(_dev, _addr, _size, _dir) \
	dma_unmap_single_attrs(&((struct pci_dev *)(_dev))->dev, \
	_addr, _size, _dir, DMA_ATTR_SKIP_CPU_SYNC)
#endif

#else
#define KAL_DMA_TO_DEVICE	DMA_TO_DEVICE
#define KAL_DMA_FROM_DEVICE	DMA_FROM_DEVICE

#define KAL_DMA_ALLOC_COHERENT(_dev, _size, _handle) \
	dma_alloc_coherent(_dev, _size, _handle, GFP_DMA)
#define KAL_DMA_FREE_COHERENT(_dev, _size, _addr, _handle) \
	dma_free_coherent(_dev, _size, _addr, _handle)
#define KAL_DMA_MAP_SINGLE(_dev, _ptr, _size, _dir) \
	dma_map_single(_dev, _ptr, _size, _dir)
#define KAL_DMA_UNMAP_SINGLE(_dev, _addr, _size, _dir) \
	dma_unmap_single(_dev, _addr, _size, _dir)
#define KAL_DMA_MAPPING_ERROR(_dev, _addr) \
	dma_mapping_error(_dev, _addr)
#endif

#if CFG_SUPPORT_DATA_STALL
#define KAL_REPORT_ERROR_EVENT			kalIndicateDriverEvent
#endif

#if CFG_SUPPORT_BIGDATA_PIP
#define KAL_REPORT_BIGDATA_PIP			kalBigDataPip
#endif

/*----------------------------------------------------------------------------*/
/* Macros of show stack operations for using in Driver Layer                  */
/*----------------------------------------------------------------------------*/
#if CFG_MTK_ANDROID_WMT && CFG_MTK_WIFI_PLAT_ALPS
extern void connectivity_export_show_stack(struct task_struct *tsk,
	unsigned long *sp);

#define kal_show_stack(_adapter, _task, _sp) \
	connectivity_export_show_stack(_task, _sp)
#else
#define kal_show_stack(_adapter, _task, _sp)
#endif

/*----------------------------------------------------------------------------*/
/* Macros of systrace operations for using in Driver Layer                    */
/*----------------------------------------------------------------------------*/
/* Not build-in project and not userload */
#if (CONFIG_WLAN_DRV_BUILD_IN == 0) && (BUILD_QA_DBG == 1)
#define kalTraceBegin(_fmt, ...) \
	tracing_mark_write("B|%d|" _fmt "\n", current->tgid, ##__VA_ARGS__)

#define kalTraceEnd() \
	tracing_mark_write("E|%d\n", current->tgid)

#define kalTraceInt(_value, _fmt, ...) \
	tracing_mark_write("C|%d|" _fmt "|%d\n", \
		current->tgid, ##__VA_ARGS__, _value)

#define kalTraceCall() \
	{ kalTraceBegin("%s", __func__); kalTraceEnd(); }

#define kalTraceEvent(_fmt, ...) \
	{ kalTraceBegin(_fmt, ##__VA_ARGS__); kalTraceEnd(); }

#define __type_is_void(expr) __builtin_types_compatible_p(typeof(expr), void)
#define __expr_zero(expr) __builtin_choose_expr(__type_is_void(expr), 0, (expr))

#define TRACE(_expr, _fmt, ...) \
	__builtin_choose_expr(__type_is_void(_expr), \
	__TRACE_VOID(_expr, _fmt, ##__VA_ARGS__), \
	__TRACE(__expr_zero(_expr), _fmt, ##__VA_ARGS__))

#define __TRACE(_expr, _fmt, ...) \
	({ \
		typeof(_expr) __ret; \
		kalTraceBegin(_fmt, ##__VA_ARGS__); \
		__ret = (_expr); \
		kalTraceEnd(); \
		__ret; \
	})

#define __TRACE_VOID(_expr, _fmt, ...) \
	({ \
		kalTraceBegin(_fmt, ##__VA_ARGS__); \
		(void) (_expr); \
		kalTraceEnd(); \
	})
#else

#define kalTraceBegin(_fmt, ...)
#define kalTraceEnd()
#define kalTraceInt(_value, _fmt, ...)
#define kalTraceCall()
#define kalTraceEvent(_fmt, ...)
#define TRACE(_expr, _fmt, ...) _expr

#endif

#define TX_DIRECT_LOCK(glue) \
do { \
	if (irqs_disabled()) \
		spin_lock(&glue->rSpinLock[SPIN_LOCK_TX_DIRECT]); \
	else \
		spin_lock_bh(&glue->rSpinLock[SPIN_LOCK_TX_DIRECT]); \
} while (0)

#define TX_DIRECT_UNLOCK(glue) \
do { \
	if (irqs_disabled()) \
		spin_unlock(&glue->rSpinLock[SPIN_LOCK_TX_DIRECT]); \
	else \
		spin_unlock_bh(&glue->rSpinLock[SPIN_LOCK_TX_DIRECT]);\
} while (0)

#define RX_DIRECT_REORDER_LOCK(glue, dbg) \
do { \
	if (!glue) \
		break; \
	if (dbg) \
		DBGLOG(QM, EVENT, "RX_DIRECT_REORDER_LOCK %d\n", __LINE__); \
	if (irqs_disabled()) \
		spin_lock(&glue->rSpinLock[SPIN_LOCK_RX_DIRECT_REORDER]); \
	else \
		spin_lock_bh(&glue->rSpinLock[SPIN_LOCK_RX_DIRECT_REORDER]); \
} while (0)

#define RX_DIRECT_REORDER_UNLOCK(glue, dbg) \
do { \
	if (!glue) \
		break; \
	if (dbg) \
		DBGLOG(QM, EVENT, "RX_DIRECT_REORDER_UNLOCK %u\n", __LINE__); \
	if (irqs_disabled()) \
		spin_unlock(&glue->rSpinLock[SPIN_LOCK_RX_DIRECT_REORDER]); \
	else \
		spin_unlock_bh(&glue->rSpinLock[SPIN_LOCK_RX_DIRECT_REORDER]);\
} while (0)

#if KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE
#define KAL_CLASS_CREATE(__name)	class_create(__name)
#else
#define KAL_CLASS_CREATE(__name)	class_create(THIS_MODULE, __name)
#endif

/*----------------------------------------------------------------------------*/
/* Macros of wiphy operations for using in Driver Layer                       */
/*----------------------------------------------------------------------------*/
#define WIPHY_PRIV(_wiphy, _priv) \
	(_priv = *((struct GLUE_INFO **) wiphy_priv(_wiphy)))

/*******************************************************************************
 *	64 bit operand
********************************************************************************
*/
#define kal_mod64(_a, _b) do_div(_a, _b)
#define kal_div64_u64(_a, _b) div64_u64(_a, _b)
#define kal_div_u64(_a, _b) div_u64(_a, _b)
/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/* Routines in gl_kal.c                                                       */
/*----------------------------------------------------------------------------*/
void kalAcquireSpinLock(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long *plFlags);

void kalReleaseSpinLock(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long ulFlags);

void kalAcquireSpinLockBh(struct GLUE_INFO *prGlueInfo,
			 enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory);

void kalReleaseSpinLockBh(struct GLUE_INFO *prGlueInfo,
			 enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory);

void kalAcquireSpinLockIrq(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long *plFlags);

void kalReleaseSpinLockIrq(struct GLUE_INFO *prGlueInfo,
			enum ENUM_SPIN_LOCK_CATEGORY_E rLockCategory,
			unsigned long ulFlags);

void kalAcquiretHifStateLock(struct GLUE_INFO *prGlueInfo,
			unsigned long *plFlags);

void kalReleaseHifStateLock(struct GLUE_INFO *prGlueInfo,
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
uint32_t kalDuplicateSwRfbSanity(struct SW_RFB *prSwRfb);
#if CFG_SUPPORT_RX_PAGE_POOL
void kalSkbReuseCheck(struct SW_RFB *prSwRfb);
void kalSkbMarkForRecycle(struct sk_buff *pkt);
struct sk_buff *kalAllocRxSkbFromPp(
	struct GLUE_INFO *prGlueInfo, uint8_t **ppucData, int i4Idx);
int kalPtrRingCnt(struct ptr_ring *ring);
void kalCreatePagePool(struct GLUE_INFO *prGlueInfo);
void kalReleasePagePool(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_SUPPORT_RX_PAGE_POOL */

void kalOsTimerInitialize(struct GLUE_INFO *prGlueInfo,
			  void *prTimerHandler);

u_int8_t kalSetTimer(struct GLUE_INFO *prGlueInfo,
		     OS_SYSTIME rInterval);

uint32_t
kalProcessRxPacket(struct GLUE_INFO *prGlueInfo,
		   void *pvPacket,
		   uint8_t *pucPacketStart, uint32_t u4PacketLen,
		   enum ENUM_CSUM_RESULT aeCSUM[]);

uint32_t kalRxIndicatePkts(struct GLUE_INFO *prGlueInfo,
			   void *apvPkts[],
			   uint8_t ucPktNum);

uint32_t kalRxIndicateOnePkt(struct GLUE_INFO
			     *prGlueInfo, void *pvPkt);
#if CFG_RFB_RECOVERY
void kalRxRFBFailRecoveryCheck(struct GLUE_INFO *prGlueInfo);
#endif

#if CFG_SUPPORT_NAN
int kalIndicateNetlink2User(struct GLUE_INFO *prGlueInfo, void *pvBuf,
			    uint32_t u4BufLen);
void kalCreateUserSock(struct GLUE_INFO *prGlueInfo);
void kalReleaseUserSock(struct GLUE_INFO *prGlueInfo);
void kalNanIndicateStatusAndComplete(struct GLUE_INFO *prGlueInfo,
				     uint32_t eStatus, uint8_t ucRoleIdx);
#endif

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

#if CFG_TX_FRAGMENT
u_int8_t
kalQueryTxPacketHeader(struct GLUE_INFO *prGlueInfo,
		       void *pvPacket, uint16_t *pu2EtherTypeLen,
		       uint8_t *pucEthDestAddr);
#endif /* CFG_TX_FRAGMENT */

#if CFG_TCP_IP_CHKSUM_OFFLOAD
void kalQueryTxChksumOffloadParam(void *pvPacket,
				  uint8_t *pucFlag);

void kalUpdateRxCSUMOffloadParam(void *pvPacket,
				 enum ENUM_CSUM_RESULT eCSUM[]);
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

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

#if CFG_SUPPORT_DATA_STALL
u_int8_t kalIndicateDriverEvent(struct ADAPTER *prAdapter,
				uint32_t event,
				uint16_t dataLen,
				uint8_t ucBssIdx,
				u_int8_t fgForceReport);
#endif

#if CFG_SUPPORT_BIGDATA_PIP
int8_t kalBigDataPip(struct ADAPTER *prAdapter,
					uint8_t *payload,
					uint16_t dataLen);
#endif

#if CFG_SUPPORT_DBDC
int8_t kalIndicateOpModeChange(struct ADAPTER *prAdapter,
					uint8_t ucBssIdx,
					uint8_t ucChannelBw,
					uint8_t ucTxNss,
					uint8_t ucRxNss);
#endif

void kalTxTimeoutDump(struct ADAPTER *prAdapter);
void kalSetTxTimeoutDump(struct GLUE_INFO *pr);

/*----------------------------------------------------------------------------*/
/* Routines in interface - ehpi/sdio.c                                        */
/*----------------------------------------------------------------------------*/
#if CFG_NEW_HIF_DEV_REG_IF
u_int8_t kalDevRegRead(enum HIF_DEV_REG_REASON reason,
		       struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register,
		       uint32_t *pu4Value);
u_int8_t kalDevRegReadRange(
	enum HIF_DEV_REG_REASON reason,
	struct GLUE_INFO *glue,
	uint32_t reg, void *buf, uint32_t total_size);
#else
u_int8_t kalDevRegRead(struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register,
		       uint32_t *pu4Value);
u_int8_t kalDevRegReadRange(struct GLUE_INFO *glue,
	uint32_t reg, void *buf, uint32_t total_size);
#endif /* CFG_NEW_HIF_DEV_REG_IF */

#if CFG_MTK_WIFI_SW_EMI_RING
u_int8_t kalDevRegReadByEmi(struct GLUE_INFO *prGlueInfo,
			    uint32_t u4Reg, uint32_t *pu4Val);
#endif

u_int8_t kalDevRegRead_mac(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Register, uint32_t *pu4Value);

u_int8_t kalDevRegWrite(struct GLUE_INFO *prGlueInfo,
			uint32_t u4Register,
			uint32_t u4Value);

#if CFG_SUPPORT_WED_PROXY
u_int8_t kalDevRegWriteDirectly(struct GLUE_INFO *prGlueInfo,
			uint32_t u4Register,
			uint32_t u4Value);
u_int8_t kalDevRegReadDirectly(struct GLUE_INFO *prGlueInfo,
		       uint32_t u4Register,
		       uint32_t *pu4Value);
#endif

u_int8_t kalDevRegWrite_mac(struct GLUE_INFO *prGlueInfo,
			    uint32_t u4Register, uint32_t u4Value);
u_int8_t kalDevRegWriteRange(struct GLUE_INFO *glue,
	uint32_t reg, void *buf, uint32_t total_size);

u_int8_t kalDevUhwRegRead(struct GLUE_INFO *prGlueInfo,
			  uint32_t u4Register, uint32_t *pu4Value);

u_int8_t kalDevUhwRegWrite(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Register, uint32_t u4Value);

u_int8_t
kalDevPortRead(struct GLUE_INFO *prGlueInfo,
	       uint16_t u2Port, uint32_t u2Len, uint8_t *pucBuf,
	       uint32_t u2ValidOutBufSize, u_int8_t isPollMode);

u_int8_t
kalDevPortWrite(struct GLUE_INFO *prGlueInfo,
		uint16_t u2Port, uint32_t u2Len, uint8_t *pucBuf,
		uint32_t u2ValidInBufSize);

u_int8_t kalDevWriteData(struct GLUE_INFO *prGlueInfo,
			 struct MSDU_INFO *prMsduInfo);
enum ENUM_CMD_TX_RESULT kalDevWriteCmd(struct GLUE_INFO *prGlueInfo,
			struct CMD_INFO *prCmdInfo, uint8_t ucTC);
u_int8_t kalDevKickData(struct GLUE_INFO *prGlueInfo);
void kalDevReadIntStatus(struct ADAPTER *prAdapter,
			 uint32_t *pu4IntStatus);

u_int8_t kalDevWriteWithSdioCmd52(struct GLUE_INFO
				  *prGlueInfo,
				  uint32_t u4Addr, uint8_t ucData);

#if CFG_SUPPORT_EXT_CONFIG
uint32_t kalReadExtCfg(struct GLUE_INFO *prGlueInfo);
#endif

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

void kalHandleAssocInfo(struct GLUE_INFO *prGlueInfo,
			struct EVENT_ASSOC_INFO *prAssocInfo);

#if CFG_ENABLE_FW_DOWNLOAD
void *kalFirmwareImageMapping(struct GLUE_INFO
			      *prGlueInfo,
			      void **ppvMapFileBuf,
			      uint32_t *pu4FileLength,
			      enum ENUM_IMG_DL_IDX_T eDlIdx);
void kalFirmwareImageUnmapping(struct GLUE_INFO
			       *prGlueInfo,
			       void *prFwHandle, void *pvMapFileBuf);

uint32_t kalFirmwareOpen(struct GLUE_INFO *prGlueInfo,
			 uint8_t **apucNameTable);
uint32_t kalFirmwareClose(struct GLUE_INFO *prGlueInfo);
uint32_t kalFirmwareSize(struct GLUE_INFO *prGlueInfo,
			 uint32_t *pu4Size);
uint32_t kalFirmwareLoad(struct GLUE_INFO *prGlueInfo,
			 void *prBuf, uint32_t u4Offset,
			 uint32_t *pu4Size);
#endif

#if CFG_CHIP_RESET_SUPPORT
void kalRemoveProbe(struct GLUE_INFO *prGlueInfo);

u_int8_t kalCheckWfsysResetPostpone(struct GLUE_INFO *prGlueInfo);
#endif
/*----------------------------------------------------------------------------*/
/* Card Removal Check                                                         */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsCardRemoved(struct GLUE_INFO *prGlueInfo);

/*----------------------------------------------------------------------------*/
/* TX                                                                         */
/*----------------------------------------------------------------------------*/
void kalFlushPendingTxPackets(struct GLUE_INFO
			      *prGlueInfo);

/*----------------------------------------------------------------------------*/
/* RX                                                                         */
/*----------------------------------------------------------------------------*/
uint32_t kalScheduleNapiTask(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/* Media State Indication                                                     */
/*----------------------------------------------------------------------------*/
enum ENUM_PARAM_MEDIA_STATE kalGetMediaStateIndicated(
	struct GLUE_INFO
	*prGlueInfo, uint8_t ucBssIndex);

void kalSetMediaStateIndicated(struct GLUE_INFO *prGlueInfo,
		enum ENUM_PARAM_MEDIA_STATE eParamMediaStateIndicate,
		uint8_t ucBssIndex);

/*----------------------------------------------------------------------------*/
/* OID handling                                                               */
/*----------------------------------------------------------------------------*/
void kalOidCmdClearance(struct GLUE_INFO *prGlueInfo);

void kalOidClearance(struct GLUE_INFO *prGlueInfo);

void kalEnqueueCommand(struct GLUE_INFO *prGlueInfo,
		       struct QUE_ENTRY *prQueueEntry);

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

u_int8_t kalCancelTimer(struct GLUE_INFO *prGlueInfo);

void kalScanDone(struct GLUE_INFO *prGlueInfo,
		 uint8_t ucBssIndex,
		 uint32_t status);

#if CFG_SUPPORT_SCAN_CACHE_RESULT
uint8_t kalUpdateBssTimestamp(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */

uint32_t kalRandomNumber(void);
void kalRandomGetBytes(void *buf, uint32_t len);

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void kalTimeoutHandler(struct timer_list *timer);
#else
void kalTimeoutHandler(unsigned long arg);
#endif

void kalSetEvent(struct GLUE_INFO *pr);

void kalSetSerTimeoutEvent(struct GLUE_INFO *pr);

void kalRxTaskSchedule(struct GLUE_INFO *pr);

uint32_t kalRxTaskWorkDone(struct GLUE_INFO *pr, u_int8_t fgIsInt);

void kalSetIntEvent(struct GLUE_INFO *pr);

void kalSetSerIntEvent(struct GLUE_INFO *pr);

void kalSetDrvIntEvent(struct GLUE_INFO *pr);

void kalSetWmmUpdateEvent(struct GLUE_INFO *pr);

void kalSetMddpEvent(struct GLUE_INFO *pr);

void kalSetHifAerResetEvent(struct GLUE_INFO *pr);

void kalSetHifMsiRecoveryEvent(struct GLUE_INFO *pr);

void kalSetHifHandleAllTokensUnusedEvent(struct GLUE_INFO *pr);

void kalSetHifDbgEvent(struct GLUE_INFO *pr);

#if CFG_SUPPORT_MULTITHREAD

#if (CFG_TX_MGMT_BY_DATA_Q == 1)
void kalSetMgmtDirectTxEvent2Hif(struct GLUE_INFO *pr);
#endif /* CFG_TX_MGMT_BY_DATA_Q == 1 */

void kalSetTxEvent2Hif(struct GLUE_INFO *pr);

void kalSetTxEvent2Rx(struct GLUE_INFO *pr);

void kalSetTxCmdEvent2Hif(struct GLUE_INFO *pr);

void kalSetTxCmdDoneEvent(struct GLUE_INFO *pr);

void kalSetRxProcessEvent(struct GLUE_INFO *pr);
#endif
/*----------------------------------------------------------------------------*/
/* NVRAM/Registry Service                                                     */
/*----------------------------------------------------------------------------*/
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

u_int8_t kalCfgDataWrite8(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Offset, uint8_t u2Data);


/*----------------------------------------------------------------------------*/
/* RSSI Updating                                                              */
/*----------------------------------------------------------------------------*/
void
kalUpdateRSSI(struct GLUE_INFO *prGlueInfo,
	      uint8_t ucBssIndex,
	      int8_t cRssi,
	      int8_t cLinkQuality);

/*----------------------------------------------------------------------------*/
/* I/O Buffer Pre-allocation                                                  */
/*----------------------------------------------------------------------------*/
u_int8_t kalInitIOBuffer(u_int8_t is_pre_alloc);

void kalUninitIOBuffer(void);

void *kalAllocateIOBuffer(uint32_t u4AllocSize);

void kalReleaseIOBuffer(void *pvAddr,
			uint32_t u4Size);

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
uint32_t kalGetMfpSetting(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex);
uint8_t kalGetRsnIeMfpCap(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex);
#endif

/*----------------------------------------------------------------------------*/
/* file opetation                                                             */
/*----------------------------------------------------------------------------*/
int32_t kalRequestFirmware(const uint8_t *pucPath,
			   uint8_t **pucData,
			   uint32_t *pu4ReadSize,
			   uint8_t ucIsZeroPadding,
			   struct device *dev);


/*----------------------------------------------------------------------------*/
/* NL80211                                                                    */
/*----------------------------------------------------------------------------*/
void
kalIndicateBssInfo(struct GLUE_INFO *prGlueInfo,
		   uint8_t *pucFrameBuf, uint32_t u4BufLen,
		   uint8_t ucChannelNum, enum ENUM_BAND eBand,
		   int32_t i4SignalStrength);

/*----------------------------------------------------------------------------*/
/* Net device                                                                 */
/*----------------------------------------------------------------------------*/
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

void kalResetPacket(struct GLUE_INFO *prGlueInfo,
		    void *prPacket);

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
/*----------------------------------------------------------------------------*/
/* SDIO Read/Write Pattern Support                                            */
/*----------------------------------------------------------------------------*/
u_int8_t kalSetSdioTestPattern(struct GLUE_INFO
			       *prGlueInfo,
			       u_int8_t fgEn, u_int8_t fgRead);
#endif

/*----------------------------------------------------------------------------*/
/* PNO Support                                                                */
/*----------------------------------------------------------------------------*/
void kalSchedScanResults(struct GLUE_INFO *prGlueInfo);

void kalSchedScanStopped(struct GLUE_INFO *prGlueInfo,
			 u_int8_t fgDriverTriggerd);

void kalSetFwOwnEvent2Hif(struct GLUE_INFO *pr);
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
uint32_t kalOpenIcsDumpFile(void);
uint32_t kalWriteIcsDumpFile(uint8_t *pucBuffer, uint16_t u2Size);
#endif /* CFG_SUPPORT_ICS */

#if (CFG_CE_ASSERT_DUMP == 1)
uint32_t kalEnqCoreDumpLog(struct ADAPTER *prAdapter, uint8_t *pucBuffer,
			     uint16_t u2Size);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#if CFG_WOW_SUPPORT
void kalWowInit(struct GLUE_INFO *prGlueInfo);
void kalWowProcess(struct GLUE_INFO *prGlueInfo,
		   uint8_t enable);
#if CFG_SUPPORT_MDNS_OFFLOAD
uint32_t kalMdnsProcess(struct GLUE_INFO *prGlueInfo,
		 struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo);
void kalMdnsOffloadInit(struct ADAPTER *prAdapter);
struct MDNS_PARAM_ENTRY_T *mdnsAllocateParamEntry(struct ADAPTER *prAdapter);

void kalSendMdnsEnableToFw(struct GLUE_INFO *prGlueInfo);
void kalSendMdnsDisableToFw(struct GLUE_INFO *prGlueInfo);
uint32_t kalAddMdnsRecord(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo);
void kalDelMdnsRecord(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo);
void kalDelMdnsRecordWithRecordKey(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo);
void kalShowMdnsRecord(struct GLUE_INFO *prGlueInfo);
struct MDNS_PASSTHROUGH_ENTRY_T *mdnsAllocatePassthroughEntry(
	 struct ADAPTER *prAdapter);
uint32_t kalAddMdnsPassthrough(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerPassthroughInfo);
void kalDelMdnsPassthrough(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerPassthroughInfo);
void kalDelMdnsPassthroughWithRecordKey(struct GLUE_INFO *prGlueInfo,
		struct MDNS_INFO_UPLAYER_T *prMdnsUplayerPassthroughInfo);
void kalShowMdnsPassthrough(struct GLUE_INFO *prGlueInfo);
uint32_t kalGetAndResetHitCounterToFw(struct GLUE_INFO *prGlueInfo,
		int recordKey);
uint32_t kalGetAndResetMissCounterToFw(struct GLUE_INFO *prGlueInfo);
void kalClearMdnsRecord(struct GLUE_INFO *prGlueInfo);
void kalClearMdnsPassthrough(struct GLUE_INFO *prGlueInfo);
void kalSendMdnsFlagsToFw(struct GLUE_INFO *prGlueInfo);

uint16_t kalGetMdnsUsedSize(struct GLUE_INFO *prGlueInfo);
uint16_t kalGetMaxAvailMdnsSize(void);

uint16_t kalGetMdnsUplRecSz(struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo);
uint16_t kalGetMdnsUplPTSz(struct MDNS_INFO_UPLAYER_T *prMdnsUplayerInfo);

uint16_t kalMdnsConvettoDataBlock(struct GLUE_INFO *prGlueInfo);
uint16_t kalMdnsAddToDataBlock(struct MDNS_DATABLOCK_T  *dataBlock,
	uint8_t *data, uint16_t dataLength);
uint16_t kalMdnsCopyPassToPayload(struct MDNS_PASSTHROUGH_T *passrthrough,
	uint8_t *payload, uint16_t start);
uint16_t kalMdnsCopyRecordToPayload(struct MDNS_RECORD_T *prMdnsRecordIndices,
	uint16_t indexCount, uint8_t *payload, uint16_t start);
uint16_t kalMdnsCopyDataToPayload(struct MDNS_DATABLOCK_T  *dataBlock,
	uint8_t *payload, uint16_t start);
uint8_t KalMdnsIncreTopHalf(uint8_t value);

#if CFG_SUPPORT_MDNS_OFFLOAD_GVA
void kalProcessMdnsRespPkt(struct GLUE_INFO *prGlueInfo, uint8_t *pucMdnsHdr);
#endif
#endif /* CFG_SUPPORT_MDNS_OFFLOAD */
#endif

int main_thread(void *data);

#if CFG_SUPPORT_MULTITHREAD
int hif_thread(void *data);
int rx_thread(void *data);
#endif
uint64_t kalGetBootTime(void);

uint8_t kalGetEapolKeyType(void *prPacket);

#if CFG_SUPPORT_WAKEUP_REASON_DEBUG
u_int8_t kalIsWakeupByWlan(struct ADAPTER *prAdapter);
#endif

int32_t kalHaltLock(uint32_t waitMs);
int32_t kalHaltTryLock(void);
void kalHaltUnlock(void);
void kalSetHalted(u_int8_t fgHalt);
u_int8_t kalIsHalted(void);
#if CFG_SUPPORT_MULTITHREAD
void kalFreeTxMsduWorker(struct work_struct *work);
void kalFreeTxMsdu(struct ADAPTER *prAdapter,
		   struct MSDU_INFO *prMsduInfo);
#endif
int32_t kalPerMonInit(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonDisable(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonEnable(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonStart(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonStop(struct GLUE_INFO *prGlueInfo);
int32_t kalPerMonDestroy(struct GLUE_INFO *prGlueInfo);
void kalPerMonHandler(struct ADAPTER *prAdapter,
		      unsigned long ulParam);
uint32_t kalPerMonGetInfo(struct ADAPTER *prAdapter,
			  uint8_t *pucBuf,
			  uint32_t u4Max);
uint32_t kalGetTpMbps(struct ADAPTER *prAdapter,
	enum ENUM_PKT_PATH ePath);
uint32_t kalGetTpMbpsByBssId(struct ADAPTER *prAdapter,
	enum ENUM_PKT_PATH ePath,
	uint8_t ucBssIdx);
u_int8_t kalIsTxHighTput(struct ADAPTER *prAdapter);
u_int8_t kalIsRxHighTput(struct ADAPTER *prAdapter);
#if CFG_SUPPORT_DISABLE_DATA_DDONE_INTR
u_int8_t kalIsTputMode(struct ADAPTER *prAdapter,
	enum ENUM_PKT_PATH ePath,
	uint8_t ucBssIdx);
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR */
void kalSetRunOnNonXCore(struct task_struct *task);
void kalSetCpuBoost(struct ADAPTER *prAdapter,
		struct BOOST_INFO *prBoostInfo);
void kalBoostCpuInit(struct ADAPTER *prAdapter);
int32_t kalBoostCpu(struct ADAPTER *prAdapter,
		    uint32_t u4TarPerfLevel,
		    uint32_t u4BoostCpuTh);
int32_t kalBoostCpuPolicy(struct ADAPTER *prAdapter);
u_int8_t kalCheckBoostCpuMargin(struct ADAPTER *prAdapter);
int32_t kalCheckTputLoad(struct ADAPTER *prAdapter,
			 uint32_t u4CurrPerfLevel,
			 uint32_t u4TarPerfLevel,
			 int32_t i4Pending,
			 uint32_t u4Used);
uint32_t kalGetCpuBoostThreshold(void);
#if CFG_SUPPORT_LITTLE_CPU_BOOST
uint32_t kalGetLittleCpuBoostThreshold(void);
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */
uint32_t kalGetChipID(void);
int32_t kalCheckVcoreBoost(struct ADAPTER *prAdapter, uint8_t uBssIndex);
uint32_t kalGetConnsysVersion(void);
uint32_t kalGetWfIpVersion(void);
uint32_t kalGetFwVerOffset(void);
uint32_t kalGetEmiMetOffset(void);
uint32_t kalGetProjectId(void);
void kalDumpPlatGPIOStat(void);
void kalSetEmiMetOffset(uint32_t newEmiMetOffset);
void kalSetRpsMap(struct GLUE_INFO *glue, unsigned long value);
extern int set_task_util_min_pct(pid_t pid, unsigned int min);
#if (CFG_COALESCING_INTERRUPT == 1)
int32_t kalCoalescingInt(struct ADAPTER *prAdapter,
			uint32_t u4TarPerfLevel,
			uint32_t u4CoalescingIntTh);
#endif

#if CFG_MTK_ANDROID_EMI
void kalSetEmiMpuProtection(phys_addr_t emiPhyBase, bool enable);
void kalSetDrvEmiMpuProtection(phys_addr_t emiPhyBase, uint32_t offset,
			       uint32_t size);
#endif
int32_t kalSetCpuNumFreq(uint32_t u4CoreNum,
			 uint32_t u4Freq);
int32_t kalGetFwFlavor(uint8_t *flavor);
int32_t kalGetFwFlavorByPlat(uint8_t *flavor);
int32_t kalGetConnsysVerId(void);
int32_t kalPerMonSetForceEnableFlag(uint8_t uFlag);
int32_t kalFbNotifierReg(struct GLUE_INFO *prGlueInfo);
void kalFbNotifierUnReg(void);
int32_t kalGetScpDumpInfo(u64 *addr, unsigned int *size);

#if KERNEL_VERSION(3, 0, 0) <= LINUX_VERSION_CODE
/* since: 0b5c9db1b11d3175bb42b80663a9f072f801edf5 */
static inline void kal_skb_reset_mac_len(struct sk_buff
		*skb)
{
	skb_reset_mac_len(skb);
}
#else
static inline void kal_skb_reset_mac_len(struct sk_buff
		*skb)
{
	skb->mac_len = skb->network_header - skb->mac_header;
}
#endif

void kalInitDevWakeup(struct ADAPTER *prAdapter, struct device *prDev);

u_int8_t kalIsValidMacAddr(const uint8_t *addr);

u_int8_t kalScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_scan_request *request, uint8_t *pucRandomMac);

u_int8_t kalSchedScanParseRandomMac(const struct net_device *ndev,
	const struct cfg80211_sched_scan_request *request,
	uint8_t *pucRandomMac, uint8_t *pucRandomMacMask);

void kalScanReqLog(struct cfg80211_scan_request *request);
void kalScanResultLog(struct ADAPTER *prAdapter, struct ieee80211_mgmt *mgmt);
void kalScanLogCacheFlushBSS(struct ADAPTER *prAdapter,
	const uint16_t logBufLen);
int kalMaskMemCmp(const void *cs, const void *ct,
	const void *mask, size_t count);

u_int8_t
kalChannelScoSwitch(enum nl80211_channel_type channel_type,
		enum ENUM_CHNL_EXT *prChnlSco);

u_int8_t
kalChannelFormatSwitch(struct cfg80211_chan_def *channel_def,
		struct ieee80211_channel *channel,
		struct RF_CHANNEL_INFO *prRfChnlInfo);

void kal_napi_complete_done(struct napi_struct *n, int work_done);
void kal_napi_schedule(struct napi_struct *n);

#if CFG_SUPPORT_RX_GRO
uint8_t kalRxGroInit(struct net_device *prDev);
uint32_t kal_is_skb_gro(struct ADAPTER *prAdapter, uint8_t ucBssIdx);
void kal_gro_flush(struct ADAPTER *prAdapter);
void kal_napi_schedule(struct napi_struct *n);
int kalNapiPoll(struct napi_struct *napi, int budget);
uint8_t kalNapiInit(struct GLUE_INFO *prGlueInfo);
uint8_t kalNapiUninit(struct GLUE_INFO *prGlueInfo);
uint8_t kalNapiRxDirectInit(struct GLUE_INFO *prGlueInfo);
uint8_t kalNapiRxDirectUninit(struct GLUE_INFO *prGlueInfo);
uint8_t kalNapiEnable(struct GLUE_INFO *prGlueInfo);
uint8_t kalNapiDisable(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_SUPPORT_RX_GRO */
uint8_t kalRxNapiValidSkb(struct GLUE_INFO *prGlueInfo,
	struct sk_buff *prSkb);

void kalRemoveBss(struct GLUE_INFO *prGlueInfo,
	uint8_t aucBSSID[],
	uint8_t ucChannelNum,
	enum ENUM_BAND eBand);


#if CFG_SUPPORT_WPA3
int kalExternalAuthRequest(
		struct GLUE_INFO *prGlueInfo,
		struct STA_RECORD *prStaRec);
#endif

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
int32_t kalBatNotifierReg(struct GLUE_INFO *prGlueInfo);
void kalEnableTxPwrBackoffByBattVolt(struct ADAPTER *prAdapter, bool ucEnable);
void kalSetTxPwrBackoffByBattVolt(struct ADAPTER *prAdapter, bool ucEnable);
void kalBatNotifierUnReg(void);
#endif

#if CFG_SUPPORT_NAN
void kalNanHandleVendorEvent(struct ADAPTER *prAdapter, uint8_t *prBuffer);
#endif

void kalWlanUeventInit(struct GLUE_INFO *prGlueInfo);
void kalWlanUeventDeinit(struct GLUE_INFO *prGlueInfo);
u_int8_t kalSendUevent(struct ADAPTER *prAdapter, const char *src);

int _kalSnprintf(char *buf, size_t size, const char *fmt, ...);
int _kalSprintf(char *buf, const char *fmt, ...);

uint32_t kalRoundUpPowerOf2(uint32_t v);

/* systrace utilities */
#if (CONFIG_WLAN_DRV_BUILD_IN == 0) && (BUILD_QA_DBG == 1)
void tracing_mark_write(const char *fmt, ...);
#endif

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
extern uint32_t getFWLogOnOff(void);
#endif

int kalTimeCompare(struct timespec64 *prTs1, struct timespec64 *prTs2);
u_int8_t kalGetDeltaTime(struct timespec64 *prTs1, struct timespec64 *prTs2,
			 struct timespec64 *prTsRst);

void setTimeParameter(
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT *prChipConfigInfo,
	int chipConfigInfoSize, unsigned int second, unsigned int usecond);
uint32_t kalSyncTimeToFW(struct ADAPTER *prAdapter,
	u_int8_t fgInitCmd);
void kalSyncTimeToFWByIoctl(void);

void kalUpdateCompHdlrRec(struct ADAPTER *prAdapter,
	PFN_OID_HANDLER_FUNC pfnOidHandler, struct CMD_INFO *prCmdInfo);

#if CFG_SUPPORT_SA_LOG
void kalPrintSALog(const char *fmt, ...);
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
void kalPwrLevelHdlrRegister(struct ADAPTER *prAdapter,
					PFN_PWR_LEVEL_HANDLER hdlr);
void kalPwrLevelHdlrUnregisterAll(struct ADAPTER *prAdapter);
void connsysPowerLevelNotify(struct ADAPTER *prAdapter);
void connsysPowerTempNotify(struct ADAPTER *prAdapter);
void connsysPowerTempUpdate(enum conn_pwr_msg_type status,
					int currentTemp);
uint32_t kalDumpPwrLevel(struct ADAPTER *prAdapter);
#endif
#if (CFG_SUPPORT_SINGLE_SKU == 1)
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
void kalApplyCustomRegulatory(const void *pRegdom, uint8_t fgNeedHoldRtnlLock);
void
kalUpdateCustomRegulatoryByWiphy(struct wiphy *pWiphy, const void *pRegdom,
	uint8_t fgNeedHoldRtnlLock);
const void *kalGetDefaultRegWW(void);
#endif
uint8_t kalGetRdmVal(uint8_t dfs_region);
u_int8_t kalIsETSIDfsRegin(void);
#endif
u_int8_t kalIsChFlagMatch(uint32_t uFlags, enum CHAN_FLAGS matchFlag);
void kal_sched_set(struct task_struct *p, int policy,
		const struct sched_param *param,
		int nice);
void kalSetThreadSchPolicyPriority(struct GLUE_INFO *prGlueInfo);
void kalSetLogTooMuch(uint32_t u4DriverLevel, uint32_t u4FwLevel);
void kalGetRealTime(struct REAL_TIME *prRealTime);
uint64_t kalGetUIntRealTime(void);
void kalVendorEventRssiBeyondRange(struct GLUE_INFO *prGlueInfo,
			uint8_t ucBssIdx, int rssi);
#if CFG_SUPPORT_TPENHANCE_MODE
inline uint64_t kalTpeTimeUs(void);
void kalTpeUpdate(struct GLUE_INFO *prGlueInfo, struct QUE *prSrcQue,
		uint8_t ucPktJump);
void kalTpeFlush(struct GLUE_INFO *prGlueInfo);

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void kalTpeTimeoutHandler(struct timer_list *timer);
#else
void kalTpeTimeoutHandler(unsigned long ulData);
#endif
void kalTpeInit(struct GLUE_INFO *prGlueInfo);
void kalTpeUninit(struct GLUE_INFO *prGlueInfo);
int kalTpeProcess(struct GLUE_INFO *prGlueInfo,
			struct sk_buff *prSkb,
			struct net_device *prDev);
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

void kalTxDirectClearSkbQ(struct GLUE_INFO *prGlueInfo);
void kalTxDirectStartCheckQTimer(
	struct GLUE_INFO *prGlueInfo,
	uint8_t offset);

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void kalTxDirectTimerCheckSkbQ(kal_timer_list *timer);
void kalTxDirectTimerCheckHifQ(kal_timer_list *timer);
#else
void kalTxDirectTimerCheckSkbQ(unsigned long data);
void kalTxDirectTimerCheckHifQ(unsigned long data);
#endif

uint32_t kalTxDirectStartXmit(struct sk_buff *prSkb,
	struct GLUE_INFO *prGlueInfo);
uint32_t kalGetTxDirectQueueLength(struct GLUE_INFO *prGlueInfo);
void kalKfreeSkb(void *pvPacket, u_int8_t fgIsFreeData);
void *kalBuildSkb(void *pvPacket, uint32_t u4MgmtLength,
	uint32_t u4TotLen, u_int8_t fgIsSetLen);
void *kalGetGlueNetDevHdl(struct GLUE_INFO *prGlueInfo);
struct device *kalGetGlueDevHdl(struct GLUE_INFO *prGlueInfo);
void kalGetPlatDev(struct platform_device **pdev);
void kalGetDev(void **dev);
void kalClearGlueScanReq(struct GLUE_INFO *prGlueInfo);
void *kalGetGlueScanReq(struct GLUE_INFO *prGlueInfo);
void *kalGetGlueSchedScanReq(struct GLUE_INFO *prGlueInfo);
void kalClearGlueSchedScanReq(struct GLUE_INFO *prGlueInfo);
void kalGetFtIeParam(void *pvftie,
	uint16_t *pu2MDID, uint32_t *pu4IeLength,
	const uint8_t **pucIe);
int kalRegulatoryHint(char *country);

uint32_t kalGetSKBSharedInfoSize(void);

#if (CFG_WLAN_ATF_SUPPORT == 1)
uint32_t kalSendAtfSmcCmd(uint32_t u4Opid, uint32_t u4Arg2,
	uint32_t u4Arg3, uint32_t u4Arg4);
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

#if defined(_HIF_PCIE)
void kalSetPcieKeepWakeup(struct GLUE_INFO *prGlueInfo,
			  u_int8_t fgKeepPcieWakeup);
void kalConfigWfdmaTh(struct GLUE_INFO *prGlueInfo, uint32_t u4Th);
#endif /* defined(_HIF_PCIE) */

void kalSetISRMask(struct ADAPTER *prAdapter, uint32_t set_mask);
void kalConfigWiFiSnappingForceDisable(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t fgForceDis);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
void kalConfigChksumOffload(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable);
#endif

void kalWlanHardStartXmit(void *pvPacket, void *pvDev);

int32_t kalNlaPut(void *pvPacket, uint32_t attrType,
		uint32_t attrLen, const void *data);

void *kalProcessRttReportDone(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4DataLen, uint32_t u4Count);

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef __no_kcsan
#define __no_kcsan
#endif

#if !defined(__GCC4_has_attribute___fallthrough__)
#define __GCC4_has_attribute___fallthrough__ 0
#endif

/* clone 'fallthrough' in include/linux/compiler_attributes.h */
#if __has_attribute(__fallthrough__)
#define kal_fallthrough __attribute__((__fallthrough__))
#else
#define kal_fallthrough do {} while (0)  /* fallthrough */
#endif

#define kalIcsWrite(buf, size) \
	wifi_ics_fwlog_write(buf, size)
#define kalIndexWrite(buf, size) \
	wifi_index_fwlog_write(buf, size)
#if (CFG_SUPPORT_CONNAC3X == 1 && CFG_SUPPORT_UPSTREAM_TOOL == 1)
#define kalWiphy_info(wiphy, format, ...) \
	wiphy_info(wiphy, format, ##__VA_ARGS__)
#endif
int32_t kalPlatOpsInit(void);

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

#if CFG_SUPPORT_CPU_STAT
#define CPU_STAT_INC_CNT(pr, idx) \
({ \
	int cpu; \
	\
	cpu = get_cpu(); \
	if (cpu < CPU_STAT_MAX_CPU) \
		GLUE_INC_REF_CNT(pr->aCpuStatCnt[idx][cpu]); \
	put_cpu(); \
})
#define CPU_STAT_GET_CNT(pr, idx, cpu) \
	(GLUE_GET_REF_CNT(pr->aCpuStatCnt[idx][cpu]))
#define CPU_STAT_RESET_ALL_CNTS(pr) \
do { \
	int i, j; \
	for (i = 0; i < CPU_STATISTICS_MAX; i++) { \
		for (j = 0; j < CPU_STAT_MAX_CPU; j++) { \
			GLUE_SET_REF_CNT(0, pr->aCpuStatCnt[i][j]); \
		} \
	} \
} while (0)
#endif /* CFG_SUPPORT_CPU_STAT */

#if CFG_SUPPORT_THERMAL_QUERY
int thermal_cbs_register(struct platform_device *pdev);
void thermal_cbs_unregister(struct platform_device *pdev);
void thermal_state_reset(struct ADAPTER *ad);
#endif

void kalTxDirectInit(struct GLUE_INFO *prGlueInfo);
void kalTxDirectUninit(struct GLUE_INFO *prGlueInfo);

#if (CFG_VOLT_INFO == 1)
void kalVnfActive(struct ADAPTER *prAdapter);
void kalVnfUninit(void);
void kalVnfInit(struct ADAPTER *prAdapter);
void kalVnfEventHandler(struct ADAPTER *prAdapter);
uint8_t kalVnfGetEnInitStatus(void);
uint32_t kalVnfGetVoltLowBnd(void);
#endif /* CFG_VOLT_INFO */

#if CFG_SUPPORT_SKB_ALLOC_WORK
uint32_t kalSkbAllocDeqSkb(struct GLUE_INFO *pr, void **pvPacket,
	uint8_t **ppucData);
void kalSkbAllocWorkSetCpu(struct GLUE_INFO *pr, enum CPU_CORE_TYPE eCoreType);
void kalSkbAllocWorkInit(struct GLUE_INFO *pr);
void kalSkbAllocWorkUninit(struct GLUE_INFO *pr);
void kalSkbAllocWorkSchedule(struct GLUE_INFO *pr, u_int8_t fgForce);
u_int8_t kalSkbAllocIsNoOOM(struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

#if CFG_SUPPORT_RETURN_WORK
void kalRxRfbReturnWorkSetCpu(struct GLUE_INFO *pr, int32_t cpu);
void kalRxRfbReturnWork(struct work_struct *work);
void kalRxRfbReturnWorkInit(struct GLUE_INFO *pr);
void kalRxRfbReturnWorkUninit(struct GLUE_INFO *pr);
void kalRxRfbReturnWorkSchedule(struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_RETURN_WORK */
void kalTxFreeMsduTaskSchedule(struct GLUE_INFO *prGlueInfo);
#if CFG_SUPPORT_TX_FREE_MSDU_WORK
void kalTxFreeMsduWorkSetCpu(struct GLUE_INFO *pr, int32_t cpu);
void kalTxFreeMsduWork(struct work_struct *work);
void kalTxFreeMsduWorkInit(struct GLUE_INFO *pr);
void kalTxFreeMsduWorkUninit(struct GLUE_INFO *pr);
void kalTxFreeMsduWorkSchedule(struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
void kalTxFreeSkbWorkSetCpu(struct GLUE_INFO *pr, enum CPU_CORE_TYPE eCoreType);
void kalTxFreeSkbWorkInit(struct GLUE_INFO *pr);
void kalTxFreeSkbWorkUninit(struct GLUE_INFO *pr);
uint32_t kalTxFreeSkbQueuePrepare(struct GLUE_INFO *pr,
	struct MSDU_INFO *prMsduInfo, struct QUE *prQue, uint8_t *pucIdx);
void kalTxFreeSkbQueueConcat(struct GLUE_INFO *pr, struct QUE *prQue);
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#if CFG_SUPPORT_TX_WORK
void kalTxWork(struct work_struct *work);
void kalTxWorkSetCpu(struct GLUE_INFO *pr, int32_t i4CpuIdx);
void kalTxWorkInit(struct GLUE_INFO *pr);
void kalTxWorkUninit(struct GLUE_INFO *pr);
uint32_t kalTxWorkSchedule(struct sk_buff *prSkb, struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_TX_WORK */

#if CFG_SUPPORT_PER_CPU_TX
uint32_t kalPerCpuTxXmit(struct sk_buff *prSkb, struct GLUE_INFO *pr);
void kalPerCpuTxInit(struct GLUE_INFO *pr);
void kalPerCpuTxUninit(struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_PER_CPU_TX */

#if CFG_SUPPORT_RX_NAPI
void kalNapiSchedule(struct ADAPTER *prAdapter);
#if CFG_SUPPORT_RX_NAPI_WORK
void kalRxNapiWork(struct work_struct *work);
void kalRxNapiWorkSetCpu(struct GLUE_INFO *pr, int32_t i4CpuIdx);
void kalRxNapiWorkInit(struct GLUE_INFO *pr);
void kalRxNapiWorkUninit(struct GLUE_INFO *pr);
void kalRxNapiWorkSchedule(struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_RX_NAPI_WORK */
#endif /* CFG_SUPPORT_RX_NAPI */
#if CFG_SUPPORT_RX_WORK
void kalRxWork(struct work_struct *work);
void kalRxWorkSetCpu(struct GLUE_INFO *pr, int32_t i4CpuIdx);
void kalRxWorkInit(struct GLUE_INFO *pr);
void kalRxWorkUninit(struct GLUE_INFO *pr);
void kalRxWorkSchedule(struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_SUPPORT_PCIE_GEN_SWITCH
void kalSetPcieGen(struct ADAPTER *prAdapter);
#endif /* CFG_SUPPORT_PCIE_GEN_SWITCH */

void kalIndicateControlPortTxStatus(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus);

#if CFG_SUPPORT_HIF_REG_WORK
void kalHifRegWork(struct work_struct *work);
void kalHifRegWorkInit(struct GLUE_INFO *pr);
void kalHifRegWorkUninit(struct GLUE_INFO *pr);
void kalHifRegWorkSchedule(struct GLUE_INFO *pr);
#endif /* CFG_SUPPORT_HIF_REG_WORK */
void kalPmicCtrl(u_int8_t fgIsEnabled);
void kalAisCsaNotifyWorkInit(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex);
void kalCsaNotifyWorkDeinit(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex);
#endif /* _GL_KAL_H */

