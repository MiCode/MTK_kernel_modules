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
 */


#ifndef _GL_OS_H
#define _GL_OS_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
/*------------------------------------------------------------------------------
 * Flags for LINUX(OS) dependent
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

#if defined(CONFIG_MTK_WIFI_HE160) || \
	defined(CONFIG_MTK_WIFI_BW320) || \
	defined(CONFIG_MTK_WIFI_EHT160)
#define CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD   4096	/* packets */
#define CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD  3072	/* packets */
#elif defined(CONFIG_MTK_WIFI_HE80)
#define CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD   1024	/* packets */
#define CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD  512	/* packets */
#else
#define CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD   256	/* packets */
#define CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD  128	/* packets */
#endif


#define CHIP_NAME    "MT6632"

#define DRV_NAME "["CHIP_NAME"]: "

#define	CONTROL_BUFFER_SIZE		(1025)
/* for CFG80211 IE buffering mechanism */
#define	CFG_CFG80211_IE_BUF_LEN		(640)
#define	GLUE_INFO_WSCIE_LENGTH		(500)
/* for non-wfa vendor specific IE buffer */
#define NON_WFA_VENDOR_IE_MAX_LEN	(128)

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define FW_LOG_CMD_ON_OFF		0
#define FW_LOG_CMD_SET_LEVEL		1

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include <linux/version.h>	/* constant of kernel version */

#include <linux/kernel.h>	/* bitops.h */

#include <linux/timer.h>	/* struct timer_list */
#include <linux/jiffies.h>	/* jiffies */
#include <linux/delay.h>	/* udelay and mdelay macro */
#include <linux/sched.h>
#include <linux/rtc.h>
#include <linux/limits.h>

#if (KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE)
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#else
#include <linux/wakelock.h>
#endif

#if KERNEL_VERSION(2, 6, 12) < LINUX_VERSION_CODE
#include <linux/irq.h>		/* IRQT_FALLING */
#endif

#include <linux/netdevice.h>	/* struct net_device, struct net_device_stats */
#include <linux/etherdevice.h>	/* for eth_type_trans() function */
#include <linux/wireless.h>	/* struct iw_statistics */
#include <linux/if_arp.h>
#include <linux/inetdevice.h>	/* struct in_device */

#include <linux/ip.h>		/* struct iphdr */

#include <linux/string.h>	/* for memcpy()/memset() function */
#include <linux/stddef.h>	/* for offsetof() macro */

#include <linux/proc_fs.h>	/* The proc filesystem constants/structures */

#include <linux/rtnetlink.h>	/* for rtnl_lock() and rtnl_unlock() */
#include <linux/kthread.h>	/* kthread_should_stop(), kthread_run() */
#include <linux/uaccess.h>	/* for copy_from_user() */
#include <linux/fs.h>		/* for firmware download */
#include <linux/vmalloc.h>

#include <linux/kfifo.h>	/* for kfifo interface */
#include <linux/cdev.h>		/* for cdev interface */

#include <linux/firmware.h>	/* for firmware download */
#include <linux/ctype.h>

#include <linux/interrupt.h>

#if defined(_HIF_USB)
#include <linux/usb.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#endif

#if defined(_HIF_PCIE)
#include <linux/pci.h>
#endif

#if defined(_HIF_SDIO)
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#endif

#include <linux/random.h>

#include <linux/io.h>		/* readw and writew */

#if WIRELESS_EXT > 12
#include <net/iw_handler.h>
#endif

#ifdef CFG_CFG80211_VERSION
#define CFG80211_VERSION_CODE CFG_CFG80211_VERSION
#else
#define CFG80211_VERSION_CODE LINUX_VERSION_CODE
#endif

#include "version.h"
#include "config.h"

#include <linux/wireless.h>
#include <net/cfg80211.h>

#include <linux/module.h>
#include <linux/can/netlink.h>
#include <net/netlink.h>
#include <linux/nl80211.h>

#if IS_ENABLED(CONFIG_IPV6)
#include <linux/ipv6.h>
#include <linux/in6.h>
#include <net/if_inet6.h>
#endif

#if CFG_SUPPORT_PASSPOINT
#include <net/addrconf.h>
#endif /* CFG_SUPPORT_PASSPOINT */

#if KERNEL_VERSION(3, 7, 0) <= CFG80211_VERSION_CODE
#include <uapi/linux/nl80211.h>
#endif

#ifdef UDP_SKT_WIFI
#if (KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE)
#include <linux/trace_events.h>
#else
#include <linux/ftrace_event.h>
#endif
#endif

#if CFG_SUPPORT_TX_WORK || CFG_SUPPORT_RX_WORK
#include <linux/workqueue.h>
#endif /* CFG_SUPPORT_TX_WORK || CFG_SUPPORT_RX_WORK */

#if CFG_SUPPORT_CRYPTO
extern struct ADAPTER *g_prAdapter;
#endif

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

#include "wlan_lib.h"
#include "wlan_oid.h"

#if CFG_ENABLE_AEE_MSG
#include <aee.h>
#endif

#if CFG_MTK_ANDROID_WMT && CFG_MTK_WIFI_PLAT_ALPS
#include <connectivity_build_in_adapter.h>
#endif

#if CFG_MET_TAG_SUPPORT
#include <mt-plat/met_drv.h>
#endif
#include <linux/time.h>
#include <linux/fb.h>
#if CFG_MTK_ANDROID_WMT && \
	KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
#include "mtk_disp_notify.h"
#endif

#if CFG_SUPPORT_NAN
#include "nan_base.h"
#include "nan_intf.h"
#endif

#if (CONFIG_WLAN_SERVICE == 1)
#include "agent.h"
#endif

extern u_int8_t fgIsMcuOff;
extern u_int8_t fgIsBusAccessFailed;
#if CFG_MTK_WIFI_PCIE_SUPPORT
extern u_int8_t fgIsPcieDataTransDisabled;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
extern u_int8_t fgTriggerDebugSop;
#endif
extern u_int32_t u4SdesDetectTime;
extern const struct ieee80211_iface_combination
	*p_mtk_iface_combinations_sta;
extern const int32_t mtk_iface_combinations_sta_num;
extern const struct ieee80211_iface_combination
	*p_mtk_iface_combinations_p2p;
extern const int32_t mtk_iface_combinations_p2p_num;
extern uint8_t g_aucNvram[];
extern uint8_t g_aucNvram_OnlyPreCal[];

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define GLUE_FLAG_HALT                  BIT(0)
#define GLUE_FLAG_INT                   BIT(1)
#define GLUE_FLAG_OID                   BIT(2)
#define GLUE_FLAG_TIMEOUT               BIT(3)
#define GLUE_FLAG_TXREQ                 BIT(4)
#define GLUE_FLAG_SER_TIMEOUT           BIT(5)
#define GLUE_FLAG_SUB_MOD_MULTICAST     BIT(7)
#define GLUE_FLAG_FRAME_FILTER          BIT(8)
#define GLUE_FLAG_FRAME_FILTER_AIS      BIT(9)

#define GLUE_FLAG_HALT_BIT              (0)
#define GLUE_FLAG_INT_BIT               (1)
#define GLUE_FLAG_OID_BIT               (2)
#define GLUE_FLAG_TIMEOUT_BIT           (3)
#define GLUE_FLAG_TXREQ_BIT             (4)
#define GLUE_FLAG_SER_TIMEOUT_BIT       (5)
#define GLUE_FLAG_SUB_MOD_MULTICAST_BIT (7)
#define GLUE_FLAG_FRAME_FILTER_BIT      (8)
#define GLUE_FLAG_FRAME_FILTER_AIS_BIT  (9)

#if CFG_SUPPORT_MULTITHREAD
#define GLUE_FLAG_TX_CMD_DONE			BIT(11)
#define GLUE_FLAG_HIF_TX			BIT(12)
#define GLUE_FLAG_HIF_TX_CMD			BIT(13)
#define GLUE_FLAG_RX_TO_OS			BIT(14)
#define GLUE_FLAG_HIF_FW_OWN			BIT(15)
#define GLUE_FLAG_HIF_MDDP			BIT(18)
#define GLUE_FLAG_DRV_INT			BIT(19)

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
#define GLUE_FLAG_HIF_MDDP_BIT			(18)
#define GLUE_FLAG_DRV_INT_BIT			(19)

#define GLUE_FLAG_RST_START			BIT(18)
#define GLUE_FLAG_RST_START_BIT			(18)
#define GLUE_FLAG_RST_END			BIT(19)
#define GLUE_FLAG_RST_END_BIT			(19)

#if CFG_SUPPORT_NAN /* notice the bit differnet with 7668 */
#define GLUE_FLAG_NAN_MULTICAST			BIT(20)
#define GLUE_FLAG_NAN_MULTICAST_BIT		(20)
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
#define GLUE_FLAG_CNS_PWR_LEVEL_BIT		(21)
#define GLUE_FLAG_CNS_PWR_LEVEL			BIT(21)
#define GLUE_FLAG_CNS_PWR_TEMP_BIT		(22)
#define GLUE_FLAG_CNS_PWR_TEMP			BIT(22)
#endif

#define GLUE_FLAG_RX_DIRECT_INT_BIT		(23)
#define GLUE_FLAG_RX_DIRECT_INT			BIT(23)

#define GLUE_FLAG_MGMT_DIRECT_HIF_TX_BIT	(24)
#define GLUE_FLAG_MGMT_DIRECT_HIF_TX		BIT(24)

#define GLUE_FLAG_SER_INT_BIT			(25)
#define GLUE_FLAG_SER_INT			BIT(25)

#define GLUE_FLAG_DRV_OWN_INT_BIT		(26)
#define GLUE_FLAG_DRV_OWN_INT			BIT(26)

#define GLUE_FLAG_DISABLE_PERF_BIT		(27)
#define GLUE_FLAG_DISABLE_PERF			BIT(27)

#define GLUE_FLAG_TX_TIMEOUT_DUMP_BIT		(28)
#define GLUE_FLAG_TX_TIMEOUT_DUMP		BIT(28)

#define GLUE_FLAG_RST_FW_NOTIFY_L0_BIT		(29)
#define GLUE_FLAG_RST_FW_NOTIFY_L0		BIT(29)

#define GLUE_FLAG_RST_FW_NOTIFY_L05_BIT		(30)
#define GLUE_FLAG_RST_FW_NOTIFY_L05		BIT(30)

#define HIF_FLAG_AER_RESET		BIT(0)
#define HIF_FLAG_AER_RESET_BIT	(0)

#define HIF_FLAG_MSI_RECOVERY		BIT(1)
#define HIF_FLAG_MSI_RECOVERY_BIT	(1)

#define HIF_FLAG_ALL_TOKENS_UNUSED	BIT(2)
#define HIF_FLAG_ALL_TOKENS_UNUSED_BIT	(2)

#if CFG_SUPPORT_HIF_RX_NAPI
#define HIF_NAPI_SET_DRV_OWN_BIT		(0)
#define HIF_NAPI_SET_FW_OWN_BIT			(1)
#define HIF_NAPI_SCHE_NAPI_BIT			(2)
#endif

#define GLUE_BOW_KFIFO_DEPTH        (1024)
/* #define GLUE_BOW_DEVICE_NAME        "MT6620 802.11 AMP" */
#define GLUE_BOW_DEVICE_NAME        "ampc0"

#define WAKE_LOCK_RX_TIMEOUT                            300	/* ms */
#define WAKE_LOCK_THREAD_WAKEUP_TIMEOUT                 50	/* ms */

#define IW_AUTH_CIPHER_GCMP128  0x00000040
#define IW_AUTH_CIPHER_GCMP256  0x00000080

/* EFUSE Support */
#define LOAD_EFUSE 0
#define LOAD_EEPROM_BIN 1
#define LOAD_AUTO 2

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
/* copy from privacy.h */
#define MAX_KEY_NUM                             8
struct GL_REPLEY_PN_INFO {
	uint8_t auPN[16];
	u_int8_t fgRekey;
	u_int8_t fgFirstPkt;
};
struct GL_DETECT_REPLAY_INFO {
	uint8_t ucCurKeyId;
	uint8_t ucKeyType;
	struct GL_REPLEY_PN_INFO arReplayPNInfo[MAX_KEY_NUM];
};
#endif

#if CFG_SUPPORT_WOW_EINT
struct WOWLAN_DEV_NODE {
	struct sdio_func *func;
#if CFG_SUPPORT_WOW_EINT_KEYEVENT_WAKEUP
	KAL_WAKE_LOCK_T *pr_eint_wlock;
#endif

	unsigned int wowlan_irq;
	int wowlan_irqlevel;
	atomic_t irq_enable_count;

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

/* note: maximum of pkt flag is 16 */
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

enum ENUM_SHUTDOWN_STATE {
	SHUTDOWN_STATE_INIT, /*wifi on*/
	SHUTDOWN_STATE_ONGOING,
	SHUTDOWN_STATE_DONE,
	SHUTDOWN_STATE_NUM
};

enum ENUM_WLAN_DRV_BUF_TYPE_T {
	ENUM_BUF_TYPE_NVRAM,
	ENUM_BUF_TYPE_DRV_CFG,
	ENUM_BUF_TYPE_FW_CFG,
	ENUM_BUF_TYPE_XONV,
	ENUM_BUF_TYPE_NUM
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

#define WORKER_NAME_STR_MAX    32
#define CON_WORK_MAX           4 /* must be power of 2 */
#define CON_WORK_SHIFT         2 /* modify it when CON_WORK_MAX change */
#define CON_WORK_MASK          BITS(0, (CON_WORK_SHIFT - 1))
#define CPU_BIG_CORE_START_IDX 4
typedef void(*PFN_CON_WORK_FUNC) (struct GLUE_INFO *pr, uint8_t ucIdx);
/* concurrent worker */
struct CON_WORK {
	uint8_t ucIdx;
	KAL_WAKE_LOCK_T *wakelock;
	struct workqueue_struct *prWorkQueue;
	struct work_struct rWork;
	PFN_CON_WORK_FUNC func;
	struct GLUE_INFO *pr;
};

#if CFG_SUPPORT_CPU_STAT
enum ENUM_CPU_STAT_CNT {
	CPU_TX_IN,
	CPU_RX_IN,
#if CFG_SUPPORT_PER_CPU_TX
	CPU_TX_PER_CPU,
#endif /* CFG_SUPPORT_PER_CPU_TX */
#if CFG_SUPPORT_TX_WORK
	CPU_TX_WORK_DONE,
#endif /* CFG_SUPPORT_TX_WORK */
#if CFG_SUPPORT_RX_WORK
	CPU_RX_WORK_DONE,
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_SUPPORT_SKB_ALLOC_WORK
	CPU_SKB_ALLOC_DONE,
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
#if CFG_SUPPORT_TX_FREE_SKB_WORK
	CPU_TX_FREE_SKB_DONE,
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */
	CPU_STATISTICS_MAX
};
#endif /* CFG_SUPPORT_CPU_STAT */

enum ENUM_WORK {
#if CFG_SUPPORT_TX_FREE_MSDU_WORK
	TX_FREE_MSDU_WORK,
#endif /* CFG_SUPPORT_TX_FREE_MSDU_WORK */
#if CFG_SUPPORT_RETURN_WORK
	RX_RETURN_RFB_WORK,
#endif /* CFG_SUPPORT_RETURN_WORK */
#if CFG_SUPPORT_TX_WORK
	TX_WORK,
#endif /* CFG_SUPPORT_TX_WORK */
#if CFG_SUPPORT_RX_WORK
	RX_WORK,
#endif /* CFG_SUPPORT_RX_WORK */
#if CFG_SUPPORT_RX_NAPI_WORK
	RX_NAPI_WORK,
#endif /* CFG_SUPPORT_RX_NAPI_WORK */
#if CFG_SUPPORT_HIF_REG_WORK
	HIF_REG_WORK,
#endif /* CFG_SUPPORT_HIF_REG_WORK */
	WORK_MAX
};

enum ENUM_WORK_INDEX {
	WORKER_0,
	WORKER_1,
	WORKER_MAX
};

struct WORK_CONTAINER {
	struct work_struct rWork;
	struct GLUE_INFO *pr;
	enum ENUM_WORK eWork;
	enum ENUM_WORK_INDEX eIdx;
};

enum ENUM_WORK_FLAG {
	ENUM_WORK_FLAG_NONE,
	ENUM_WORK_FLAG_MULTIWORK,
	ENUM_WORK_FLAG_MAX
};

struct GL_WORK {
	int32_t i4WorkCpu; /* controlled by CPU Boost */
	struct workqueue_struct *prWorkQueue;
	uint8_t *sWorkQueueName;
	struct WORK_CONTAINER rWorkContainer[WORKER_MAX];
	enum ENUM_WORK_FLAG eWorkFlag;
	enum ENUM_WORK_INDEX eWorkIdx;
};

#define WORK_SET_FLAG(_w, _eWorkflag) \
	((_w)->eWorkFlag = _eWorkflag)

#define WORK_IS_FLAG(_w, _eWorkflag) \
	((_w)->eWorkFlag == _eWorkflag)

typedef void (*GL_WORK_FUNC) (struct work_struct *work);

#if CFG_SUPPORT_SKB_ALLOC_WORK
struct SKB_ALLOC_INFO {
	struct CON_WORK rConWork[CON_WORK_MAX];
	uint32_t u4ReqNum[CON_WORK_MAX];
	struct sk_buff_head rFreeSkbQ;
	uint32_t u4ScheCnt;
	uint32_t u4TotalReqNum;
	unsigned long ulScheMask;
	enum CPU_CORE_TYPE eCoreType;
	unsigned long ulNoMemMask;
};
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
struct TX_FREE_QUEUE_INFO {
	struct QUE rQue;
	spinlock_t lock;
	uint32_t u4TotalCnt;
};

struct TX_FREE_INFO {
	struct CON_WORK rConWork[CON_WORK_MAX];
	struct TX_FREE_QUEUE_INFO rQueInfo[CON_WORK_MAX];
	int32_t i4QueIdxCnt;
	enum CPU_CORE_TYPE eCoreType;
};
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

struct GL_IO_REQ {
	struct QUE_ENTRY rQueEntry;
	/* wait_queue_head_t       cmdwait_q; */
	struct ADAPTER *prAdapter;
	PFN_OID_HANDLER_FUNC pfnOidHandler;
	void *pvInfoBuf;
	uint32_t u4InfoBufLen;
	uint32_t *pu4QryInfoLen;
	uint32_t rStatus;
	uint8_t ucBssIndex;
};

#if CFG_ENABLE_BT_OVER_WIFI
struct GL_BOW_INFO {
	u_int8_t fgIsRegistered;
	dev_t u4DeviceNumber;	/* dynamic device number */
	/* struct kfifo *prKfifo; */ /* for buffering indicated events */
	struct kfifo rKfifo;	/* for buffering indicated events */
	spinlock_t rSpinLock;	/* spin lock for kfifo */
	struct cdev cdev;
	uint32_t u4FreqInKHz;	/* frequency */

	uint8_t aucRole[CFG_BOW_PHYSICAL_LINK_NUM];	/* 0: Responder,
							 * 1: Initiator
							 */
	enum ENUM_BOW_DEVICE_STATE
	aeState[CFG_BOW_PHYSICAL_LINK_NUM];
	uint8_t arPeerAddr[CFG_BOW_PHYSICAL_LINK_NUM][PARAM_MAC_ADDR_LEN];

	wait_queue_head_t outq;

#if CFG_BOW_SEPARATE_DATA_PATH
	/* Device handle */
	struct net_device *prDevHandler;
	u_int8_t fgIsNetRegistered;
#endif

};
#endif

#if CFG_SUPPORT_SCAN_CACHE_RESULT
struct GL_SCAN_CACHE_INFO {
	struct GLUE_INFO *prGlueInfo;

	/* for cfg80211 scan done indication */
	struct cfg80211_scan_request *prRequest;

	/* total number of channels to scan */
	uint32_t n_channels;

	/* Scan period time */
	OS_SYSTIME u4LastScanTime;

	/* Bss index */
	uint8_t ucBssIndex;

	/* scan request flags */
	uint32_t u4Flags;
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

struct FT_IES {
	uint16_t u2MDID;
	struct IE_MOBILITY_DOMAIN *prMDIE;
	struct IE_FAST_TRANSITION *prFTIE;
	struct IE_TIMEOUT_INTERVAL *prTIE;
	struct RSN_INFO_ELEM *prRsnIE;
	uint8_t *pucIEBuf;
	uint32_t u4IeLength;
};

struct GL_CH_SWITCH_WORK {
#if (KERNEL_VERSION(6, 6, 0) <= CFG80211_VERSION_CODE)
	struct work_struct rChSwitchNotifyWork;
	u_int8_t fgWorkInit;
#endif
};

#if CFG_SUPPORT_PER_CPU_TX
struct _PER_CPU_TX_INFO {
	struct tasklet_struct rTask;
	struct sk_buff_head rSkbQ;
};

struct PER_CPU_TX_INFO {
	struct _PER_CPU_TX_INFO __percpu *prInfo;
	u_int8_t fgReady;
	unsigned long ulRunningMask;
};

#define PER_CPU_TX_WAITING_TIMEOUT 100 /* ms */
#define PER_CPU_TX_SET_RUN(prPerCpuTxInfo, cpu, fgRunning) \
	do { \
		if (fgRunning) \
			set_bit(cpu, &prPerCpuTxInfo->ulRunningMask); \
		else \
			clear_bit(cpu, &prPerCpuTxInfo->ulRunningMask); \
	} while (0)

#define PER_CPU_TX_IS_RUNNING(prPerCpuTxInfo) \
	(READ_ONCE(prPerCpuTxInfo->ulRunningMask) != 0)
#endif /* CFG_SUPPORT_PER_CPU_TX */

#if CFG_NAPI_DELAY
#define NAPI_DELAY_ENABLE_BIT    (0)
#define NAPI_DELAY_START_BIT     (1)
#define NAPI_DELAY_SCHEDULE_BIT  (2)
#endif /* CFG_NAPI_DELAY */

/*
 * type definition of pointer to p2p structure
 */
struct GL_P2P_INFO;	/* declare GL_P2P_INFO_T */
struct GL_P2P_DEV_INFO;	/* declare GL_P2P_DEV_INFO_T */

struct GLUE_INFO {
	/* Device handle */
	struct net_device *prDevHandler;

	/* Device */
	struct device *prDev;

	/* Device Index(index of arWlanDevInfo[]) */
	int32_t i4DevIdx;

	/* Device statistics */
	/* struct net_device_stats rNetDevStats; */

	/* Wireless statistics struct net_device */
	struct iw_statistics rIwStats[MAX_BSSID_NUM];

	/* spinlock to sync power save mechanism */
	spinlock_t rSpinLock[SPIN_LOCK_NUM];

	/* Mutex to protect interruptible section */
	struct mutex arMutex[MUTEX_NUM];
#if CFG_SUPPORT_RX_PAGE_POOL
	struct mutex arMutexPagePool[PAGE_POOL_NUM];
#endif

	/* semaphore for ioctl */
	struct semaphore ioctl_sem;

	uint64_t u8Cookie;

	unsigned long ulFlag;		/* GLUE_FLAG_XXX */
	unsigned long ulHifFlag;	/* HIF_FLAG_XXX */
	uint32_t u4PendFlag;
	/* UINT_32 u4TimeoutFlag; */
	u_int8_t fgOidWaiting; /* TRUE: waiter enters ioctl, FALSE: completed */
	uint32_t u4ReadyFlag;	/* check if card is ready */

	uint32_t u4OsMgmtFrameFilter;

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

	/* Tx: for NetDev to BSS index mapping */
	struct NET_INTERFACE_INFO arNetInterfaceInfo[MAX_BSSID_NUM];

	/* Rx: for BSS index to NetDev mapping */
	/* P_NET_INTERFACE_INFO_T  aprBssIdxToNetInterfaceInfo[HW_BSSID_NUM]; */

	/* current IO request for kalIoctl */
	struct GL_IO_REQ OidEntry;

	/* registry info */
	struct REG_INFO rRegInfo;

	/* firmware */
	struct firmware *prFw;

	/* Host interface related information */
	/* defined in related hif header file */
	struct GL_HIF_INFO rHifInfo;

	/* Pointer to ADAPTER_T - main data structure of internal protocol
	 * stack
	 */
	struct ADAPTER *prAdapter;

#if WLAN_INCLUDE_PROC
	struct proc_dir_entry *pProcRoot;
#endif

	/* Device power state D0~D3 */
	enum PARAM_DEVICE_POWER_STATE ePowerState;

	struct completion rScanComp;	/* indicate scan complete */
	struct completion rHaltComp;	/* indicate main thread halt complete */
	struct completion rPendComp;	/* Pending completion for OID */
#if CFG_SUPPORT_MULTITHREAD
	struct completion rHifHaltComp;	/* indicate hif_thread halt complete */
	struct completion rRxHaltComp;	/* indicate rx_thread halt complete */

	uint32_t u4TxThreadPid;
	uint32_t u4RxThreadPid;
	uint32_t u4HifThreadPid;
#endif
#if CFG_SUPPORT_NAN
	struct completion
		rNanHaltComp;	/* indicate halt complete in NAN initial flow */
#endif

#if CFG_SUPPORT_NCHO
	/* indicate Ais channel grant complete */
	struct completion rAisChGrntComp;
#endif

	uint32_t rPendStatus;

	struct QUE rTxQueue;

	/* OID related */
	struct QUE rCmdQueue;
	/* PVOID                   pvInformationBuffer; */
	/* UINT_32                 u4InformationBufferLength; */
	/* PVOID                   pvOidEntry; */
	/* PUINT_8                 pucIOReqBuff; */
	/* QUE_T                   rIOReqQueue; */
	/* QUE_T                   rFreeIOReqQueue; */

	wait_queue_head_t waitq;
	struct task_struct *main_thread;

#if CFG_SUPPORT_MULTITHREAD
	wait_queue_head_t waitq_hif;
	struct task_struct *hif_thread;

	wait_queue_head_t waitq_rx;
	struct task_struct *rx_thread;

#endif
#if CFG_SUPPORT_RX_NAPI_THREADED
	uint32_t u4RxNapiThreadPid;
	struct task_struct *napi_thread;
#endif /* CFG_SUPPORT_RX_NAPI_THREADED */
	struct timespec64 rLastMsduRptChangedTime;
#if CFG_SUPPORT_CPU_STAT
	/* cpu statistics */
	atomic_t aCpuStatCnt[CPU_STATISTICS_MAX][CPU_STAT_MAX_CPU];
#endif /* CFG_SUPPORT_CPU_STAT */
#if CFG_SUPPORT_SKB_ALLOC_WORK
	struct SKB_ALLOC_INFO rSkbAllocInfo;
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */
	struct GL_WORK arGlWork[WORK_MAX];
	struct tasklet_struct rRxTask;
#if CFG_SUPPORT_PER_CPU_TX
	struct PER_CPU_TX_INFO rPerCpuTxInfo;
#endif /* CFG_SUPPORT_PER_CPU_TX */
	uint8_t fgRxTaskReady;
	uint32_t u4RxTaskScheduleCnt;
#if (CFG_SUPPORT_RETURN_TASK == 1)
	struct tasklet_struct rRxRfbRetTask;
#endif
#if CFG_SUPPORT_TASKLET_FREE_MSDU
	struct tasklet_struct rTxMsduRetTask;
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */
	struct tasklet_struct rTxCompleteTask;
#if CFG_SUPPORT_MULTITHREAD
	struct work_struct rTxMsduFreeWork;
#endif
	struct delayed_work rRxPktDeAggWork;

	struct timer_list tickfn;
#if CFG_SUPPORT_TPENHANCE_MODE
	struct timer_list PeriodSecTimer;
#endif /* CFG_SUPPORT_TPENHANCE_MODE */
	/* check if an empty MsduInfo is available */
	kal_timer_list rTxDirectSkbTimer;
	/* check if HIF port is ready to accept a new Msdu */
	kal_timer_list rTxDirectHifTimer;
	struct sk_buff_head rTxDirectSkbQueue;

#if CFG_SUPPORT_EXT_CONFIG
	uint16_t au2ExtCfg[256];	/* NVRAM data buffer */
	uint32_t u4ExtCfgLength;	/* 0 means data is NOT valid */
#endif

#if CFG_ENABLE_BT_OVER_WIFI
	struct GL_BOW_INFO rBowInfo;
#endif

#if CFG_ENABLE_WIFI_DIRECT
	struct GL_P2P_DEV_INFO *prP2PDevInfo;
	struct GL_P2P_INFO *prP2PInfo[KAL_P2P_NUM];
#if CFG_SUPPORT_P2P_RSSI_QUERY
	/* Wireless statistics struct net_device */
	struct iw_statistics rP2pIwStats;
#endif
#endif
#if CFG_SUPPORT_NAN
	struct _GL_NAN_INFO_T *aprNANDevInfo[NAN_BSS_INDEX_NUM];
#endif

	/* NVRAM availability */
	u_int8_t fgNvramAvailable;

	u_int8_t fgMcrAccessAllowed;

	/* MAC Address Overridden by IOCTL */
	u_int8_t fgIsMacAddrOverride;
	uint8_t rMacAddrOverride[PARAM_MAC_ADDR_LEN];

	struct SET_TXPWR_CTRL rTxPwr;

	/* for cfg80211 scan done indication */
	struct cfg80211_scan_request *prScanRequest;

#if CFG_SUPPORT_SCHED_SCAN
	struct PARAM_SCHED_SCAN_REQUEST *prSchedScanRequest;
#else
	/* for cfg80211 scheduled scan */
	struct cfg80211_sched_scan_request *prSchedScanRequest;
#endif

	/* to indicate registered or not */
	u_int8_t fgIsRegistered;

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
	u_int8_t fgEnSdioTestPattern;
	u_int8_t fgSdioReadWriteMode;
	u_int8_t fgIsSdioTestInitialized;
	uint8_t aucSdioTestBuffer[256];
#endif

	u_int8_t fgIsInSuspendMode;

#if CFG_SUPPORT_PASSPOINT
	u_int8_t fgIsDad;
	uint8_t aucDADipv4[4];
	u_int8_t fgIs6Dad;
	uint8_t aucDADipv6[16];
#endif				/* CFG_SUPPORT_PASSPOINT */

	KAL_WAKE_LOCK_T *rIntrWakeLock;
	KAL_WAKE_LOCK_T *rTimeoutWakeLock;
#if CFG_ENABLE_WAKE_LOCK && \
	defined(CFG_MTK_WIFI_DRV_OWN_INT_MODE)
	KAL_WAKE_LOCK_T *prDrvOwnWakeLock;
#endif
#if CFG_ENABLE_WAKE_LOCK && CFG_SUPPORT_RX_WORK
	KAL_WAKE_LOCK_T *rRxWorkerLock;
#endif
#if CFG_ENABLE_WAKE_LOCK && CFG_SUPPORT_PWR_LMT_EMI
	KAL_WAKE_LOCK_T *rTxPowerEmiWakeLock;
#endif

#if CFG_MET_PACKET_TRACE_SUPPORT
	u_int8_t fgMetProfilingEn;
	uint16_t u2MetUdpPort;
#endif

#if CFG_SUPPORT_TASKLET_FREE_MSDU
	struct kfifo rTxMsduRetFifo;
	uint8_t *prTxMsduRetFifoBuf;
	uint32_t u4TxMsduRetFifoLen;
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
	struct TX_FREE_INFO rTxFreeInfo;
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#if CFG_SUPPORT_RX_GRO
	struct net_device dummy_dev;
	struct napi_struct napi;
	OS_SYSTIME tmGROFlushTimeout;
	spinlock_t napi_spinlock;
	uint32_t u4PendingFlushNum;
	struct sk_buff_head rRxNapiSkbQ;
#endif /* CFG_SUPPORT_RX_GRO */
#if CFG_SUPPORT_RX_NAPI
	struct napi_struct *prRxDirectNapi;
	struct kfifo rRxKfifoQ;
	uint8_t *prRxKfifoBuf;
	uint32_t u4RxKfifoBufLen;
	u_int8_t fgNapiScheduled;
	uint32_t u4LastScheduleCnt;
	uint32_t u4LastNapiPollCnt;
#if CFG_NAPI_DELAY
	struct hrtimer rNapiDelayTimer;
	unsigned long ulNapiDelayFlag;
#endif /* CFG_NAPI_DELAY */
#endif /* CFG_SUPPORT_RX_NAPI */

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

	int32_t i4RssiCache[MAX_BSSID_NUM];
	uint32_t u4TxLinkSpeedCache[MAX_BSSID_NUM];
	uint32_t u4RxLinkSpeedCache[MAX_BSSID_NUM];
	uint32_t u4TxBwCache[MAX_BSSID_NUM];
	uint32_t u4RxBwCache[MAX_BSSID_NUM];

#if CFG_AP_80211KVR_INTERFACE
	struct delayed_work rChanNoiseControlWork;
	struct delayed_work rChanNoiseGetInfoWork;
#endif

	uint32_t u4InfType;

	uint32_t IsrCnt;
	uint32_t IsrPassCnt;
	uint32_t TaskIsrCnt;

	uint32_t IsrAbnormalCnt;
	uint32_t IsrSoftWareCnt;
	uint32_t IsrTxCnt;
	uint32_t IsrRxCnt;
	uint64_t u8HifIntTime;

	/* save partial scan channel information */
	/* PARTIAL_SCAN_INFO rScanChannelInfo; */
	uint8_t *pucScanChannel;

#if CFG_SUPPORT_SCAN_CACHE_RESULT
	struct GL_SCAN_CACHE_INFO scanCache;
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */
#if (CFG_SUPPORT_PERF_IND == 1)
	struct GL_PERF_IND_INFO PerfIndCache;
#endif

	/* Full2Partial */
	OS_SYSTIME u4LastFullScanTime;
	/* full scan or partial scan */
	uint8_t ucTrScanType;
	/* UINT_8 aucChannelNum[FULL_SCAN_MAX_CHANNEL_NUM]; */
	/* PARTIAL_SCAN_INFO rFullScanApChannel; */
	uint8_t *pucFullScan2PartialChannel;

	uint32_t u4RoamFailCnt;
	uint64_t u8RoamFailTime;
	u_int8_t fgTxDoneDelayIsARP;
	uint32_t u4ArriveDrvTick;
	uint32_t u4EnQueTick;
	uint32_t u4DeQueTick;
	uint32_t u4LeaveDrvTick;
	uint32_t u4CurrTick;
	uint64_t u8CurrTime;

	/* FW Roaming */
	/* store the FW roaming enable state which FWK determines */
	/* if it's = 0, ignore the block/allowlists settings from FWK */
	uint32_t u4FWRoamingEnable;

	/*service for test mode*/
#if (CONFIG_WLAN_SERVICE == 1)
	struct service rService;
#endif

#if CFG_SUPPORT_SCAN_EXT_FLAG
	uint32_t u4ScanExtFlag;
#endif

#if CFG_SUPPORT_NAN
	struct sock *NetLinkSK;
#endif

#if CFG_SUPPORT_TPENHANCE_MODE
	/* Tp Enhance */
	struct QUE rTpeAckQueue;
	uint32_t u4TpeMaxPktNum;
	uint64_t u8TpeTimestamp;
	uint32_t u4TpeTimeout;
	struct timer_list rTpeTimer;
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

#if CFG_CHIP_RESET_SUPPORT && !CFG_WMT_RESET_API_SUPPORT
	struct work_struct rWfsysResetWork;    /* work for Wfsys L0.5 reset  */
#endif

#if (CFG_CE_ASSERT_DUMP == 1)
	wait_queue_head_t waitq_coredump;
	struct sk_buff_head rCoreDumpSkbQueue;
#endif

#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
	struct CHANNEL_TIMING_T  rChanTimeRecord[CH_MAX_NUM];
	uint8_t u1NoiseLevel;
#endif

#if CFG_SUPPORT_CSI
	wait_queue_head_t waitq_csi;
#endif
	unsigned long fgIsInSuspend;

#if CFG_SUPPORT_RX_PAGE_POOL
	struct page_pool *aprPagePool[PAGE_POOL_NUM];
	uint32_t u4LastAllocIdx;
#endif

#if CFG_TESTMODE_L0P5_FWDL_SUPPORT
	bool fgTestFwDl;
	wait_queue_head_t waitQTestFwDl;
#endif
#if CFG_SUPPORT_HIF_REG_WORK
	struct kfifo rHifRegFifo;
	spinlock_t rHifRegFifoLock;
	uint8_t *prHifRegFifoBuf;
	uint32_t u4HifRegFifoLen;
	uint32_t u4HifRegStartCnt;
	uint32_t u4HifRegReqCnt;
#if CFG_MTK_WIFI_MBU
	uint32_t u4MbuTimeoutCnt;
#endif
#endif /* CFG_SUPPORT_HIF_REG_WORK */
	u_int8_t fgWlanUevent;
#if CFG_SUPPORT_TPUT_FACTOR
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	cpumask_t hif_cpu_mask;
#endif
#endif /* CFG_SUPPORT_TPUT_FACTOR */
};

typedef irqreturn_t(*PFN_WLANISR) (int irq, void *dev_id,
				   struct pt_regs *regs);

typedef void (*PFN_LINUX_TIMER_FUNC) (unsigned long);

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

	TESTMODE_CMD_ID_UPDATE_STA_PMKID = 1000,
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
	struct work_struct workq;
#if CFG_SUPPORT_SKIP_RX_GRO_FOR_TC
	u_int8_t fgSkipRxGro;
#endif /* CFG_SUPPORT_SKIP_RX_GRO_FOR_TC */
	uint8_t ucBssIdx;
	u_int8_t ucIsP2p;
	u_int8_t ucMddpSupport;
	struct net_device_stats stats;
#if CFG_SUPPORT_NAN
	unsigned char ucIsNan;
#endif
	uint8_t ucMldBssIdx;
	uint32_t u4OsMgmtFrameFilter;
};

struct PACKET_PRIVATE_COMMON_DATA {  /* total: 8byte */
	uint8_t ucBssIdx;            /* 1byte */
	uint8_t aucReserved[3];      /* 3byte */
	uint16_t u2Flag;             /* 2byte */
	uint16_t u2IpId;             /* 2byte */
};

struct PACKET_PRIVATE_TX_DATA {      /* total: 24byte */
	uint8_t ucTid;               /* 1byte */
	uint8_t ucHeaderLen;         /* 1byte */
	uint8_t ucFlag;              /* 1byte */
	uint8_t ucSeqNo;             /* 1byte */
	uint16_t u2FrameLen;         /* 2byte */
	uint8_t aucReserved[2];      /* 2byte */
	uint32_t u4Cookie;           /* 4byte */
	OS_SYSTIME rArrivalTime;     /* 4byte */
	uint64_t u8ArriveTime;       /* 8byte */
};

struct PACKET_PRIVATE_RX_DATA {      /* total: 24byte */
	u_int8_t fgIsIndependentPkt; /* 1byte */
#if CFG_SUPPORT_WED_PROXY
	uint8_t aucReserved[3];      /* 3byte */
	uint32_t u4PpeType;          /* 4byte */
#else
	uint8_t aucReserved[7];      /* 7byte */
#endif
	uint64_t u8IntTime;          /* 8byte */
	uint64_t u8RxTime;           /* 8byte */
};

/*
 * sizeof(cb): 48 bytes
 * compile time size check in glPacketDataTypeCheck
 */
struct PACKET_PRIVATE_DATA {
	struct QUE_ENTRY rQueEntry;                     /* 16byte */
	struct PACKET_PRIVATE_COMMON_DATA rCommonData;  /*  8byte */
	union {                                         /* 24byte */
		struct PACKET_PRIVATE_TX_DATA rTxData;
		struct PACKET_PRIVATE_RX_DATA rRxData;
	};
};

struct CMD_CONNSYS_FW_LOG {
	int32_t fgCmd;
	int32_t fgValue;
	u_int8_t fgEarlySet;
};

#if CFG_MTK_ANDROID_WMT
#if !CFG_SUPPORT_CONNAC1X
struct MTK_WCN_WLAN_CB_INFO {
	int (*wlan_probe_cb)(void);
	int (*wlan_remove_cb)(void);
};
#endif
#endif

struct tag_bootmode {
	u32 size;
	u32 tag;
	u32 bootmode;
	u32 boottype;
};

enum BOOTMODE {
	NORMAL_BOOT = 0,
	META_BOOT = 1,
	RECOVERY_BOOT = 2,
	SW_REBOOT = 3,
	FACTORY_BOOT = 4,
	ADVMETA_BOOT = 5,
	ATE_FACTORY_BOOT = 6,
	ALARM_BOOT = 7,
	KERNEL_POWER_OFF_CHARGING_BOOT = 8,
	LOW_POWER_OFF_CHARGING_BOOT = 9,
	FASTBOOT = 99,
	DOWNLOAD_BOOT = 100,
	UNKNOWN_BOOT
};

enum DFS_CHANNEL_CTRL_SOURCE {
	DFS_CHANNEL_CTRL_SOURCE_STA,
	DFS_CHANNEL_CTRL_SOURCE_SAP,
	DFS_CHANNEL_CTRL_SOURCE_DBG,
	DFS_CHANNEL_CTRL_SOURCE_NUM
};

struct WLAN_DFS_CHANNEL_REQ_ENTRY {
	enum DFS_CHANNEL_CTRL_SOURCE eSource;
	u_int8_t fgValid;
	struct RF_CHANNEL_INFO rRfChnlInfo;
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

#define TYPEOF(__F) (typeof(__F))

/*----------------------------------------------------------------------------*/
/* Macros of SPIN LOCK operations for using in Glue Layer                     */
/*----------------------------------------------------------------------------*/
#if CFG_USE_SPIN_LOCK_BOTTOM_HALF
#define GLUE_SPIN_LOCK_DECLARATION()
#define GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, rLockCategory)   \
	{ \
		if (rLockCategory < SPIN_LOCK_NUM) \
			spin_lock_bh(&(prGlueInfo->rSpinLock[rLockCategory])); \
	}
#define GLUE_RELEASE_SPIN_LOCK(prGlueInfo, rLockCategory)   \
	{ \
		if (rLockCategory < SPIN_LOCK_NUM) \
			spin_unlock_bh(\
			&(prGlueInfo->rSpinLock[rLockCategory])); \
	}
#else /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */
#define GLUE_SPIN_LOCK_DECLARATION() unsigned long __ulFlags = 0
#define GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, rLockCategory)   \
	{ \
		if (rLockCategory < SPIN_LOCK_NUM) \
			spin_lock_irqsave(\
			&(prGlueInfo)->rSpinLock[rLockCategory], __ulFlags); \
	}
#define GLUE_RELEASE_SPIN_LOCK(prGlueInfo, rLockCategory)   \
	{ \
		if (rLockCategory < SPIN_LOCK_NUM) \
			spin_unlock_irqrestore(\
			&(prGlueInfo->rSpinLock[rLockCategory]), __ulFlags); \
	}
#endif /* !CFG_USE_SPIN_LOCK_BOTTOM_HALF */

/*----------------------------------------------------------------------------*/
/* Macros for accessing Reserved Fields of native packet                      */
/*----------------------------------------------------------------------------*/

#define GLUE_GET_PKT_PRIVATE_DATA(_p) \
	((struct PACKET_PRIVATE_DATA *)(&(((struct sk_buff *)(_p))->cb[0])))

#define GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p) \
	(&(GLUE_GET_PKT_PRIVATE_DATA(_p)->rCommonData))

#define GLUE_GET_PKT_PRIVATE_RX_DATA(_p) \
	(&(GLUE_GET_PKT_PRIVATE_DATA(_p)->rRxData))

#define GLUE_GET_PKT_PRIVATE_TX_DATA(_p) \
	(&(GLUE_GET_PKT_PRIVATE_DATA(_p)->rTxData))

#define GLUE_GET_PKT_QUEUE_ENTRY(_p)    \
	    (&(GLUE_GET_PKT_PRIVATE_DATA(_p)->rQueEntry))

#define GLUE_GET_PKT_DESCRIPTOR(_prQueueEntry)  \
	    ((void *) (((unsigned long)_prQueueEntry) \
	    - offsetof(struct sk_buff, cb[0])))

#define GLUE_SET_PKT_TID(_p, _tid) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucTid = (uint8_t)(_tid))

#define GLUE_GET_PKT_TID(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucTid)

#define GLUE_SET_PKT_FLAG(_p, _flag) \
	(GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p)->u2Flag |= BIT(_flag))

#define GLUE_TEST_PKT_FLAG(_p, _flag) \
	(GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p)->u2Flag & BIT(_flag))

#define GLUE_IS_PKT_FLAG_SET(_p) \
	(GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p)->u2Flag)

#define GLUE_SET_PKT_BSS_IDX(_p, _ucBssIndex) \
	(GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p)->ucBssIdx = \
		(uint8_t)(_ucBssIndex))

#define GLUE_GET_PKT_BSS_IDX(_p) \
	(GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p)->ucBssIdx)

#define GLUE_SET_PKT_HEADER_LEN(_p, _ucMacHeaderLen) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucHeaderLen = \
		(uint8_t)(_ucMacHeaderLen))

#define GLUE_GET_PKT_HEADER_LEN(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucHeaderLen)

#define GLUE_SET_PKT_FRAME_LEN(_p, _u2PayloadLen) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->u2FrameLen = \
		(uint16_t)(_u2PayloadLen))

#define GLUE_GET_PKT_FRAME_LEN(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->u2FrameLen)

#define GLUE_SET_PKT_ARRIVAL_TIME(_p, _rSysTime) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->rArrivalTime = \
		(OS_SYSTIME)(_rSysTime))

#define GLUE_GET_PKT_ARRIVAL_TIME(_p)    \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->rArrivalTime)

#define GLUE_SET_PKT_IP_ID(_p, _u2IpId) \
	(GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p)->u2IpId = (uint16_t)(_u2IpId))

#define GLUE_GET_PKT_IP_ID(_p) \
	(GLUE_GET_PKT_PRIVATE_COMMON_DATA(_p)->u2IpId)

#define GLUE_SET_PKT_SEQ_NO(_p, _ucSeqNo) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucSeqNo = (uint8_t)(_ucSeqNo))

#define GLUE_GET_PKT_SEQ_NO(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucSeqNo)

#define GLUE_SET_PKT_FLAG_PROF_MET(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucFlag |= BIT(0))

#define GLUE_GET_PKT_IS_PROF_MET(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucFlag & BIT(0))

#define GLUE_SET_PKT_CONTROL_PORT_TX(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucFlag |= BIT(1))

#define GLUE_GET_PKT_IS_CONTROL_PORT_TX(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->ucFlag & BIT(1))

#define GLUE_SET_PKT_XTIME(_p, _rSysTime) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->u8ArriveTime = (uint64_t)(_rSysTime))

#define GLUE_GET_PKT_XTIME(_p)    \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->u8ArriveTime)

#define GLUE_SET_PKT_TX_COOKIE(_p, _cookie) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->u4Cookie = (_cookie))

#define GLUE_GET_PKT_TX_COOKIE(_p) \
	(GLUE_GET_PKT_PRIVATE_TX_DATA(_p)->u4Cookie)

#define GLUE_GET_INDEPENDENT_PKT(_p)    \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->fgIsIndependentPkt)

#define GLUE_SET_INDEPENDENT_PKT(_p, _fgIsIndePkt) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->fgIsIndependentPkt = _fgIsIndePkt)

#define GLUE_RX_SET_PKT_INT_TIME(_p, _rTime) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8IntTime = (uint64_t)(_rTime))

#define GLUE_RX_GET_PKT_INT_TIME(_p) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8IntTime)

#define GLUE_RX_SET_PKT_RX_TIME(_p, _rTime) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8RxTime = (uint64_t)(_rTime))

#define GLUE_RX_GET_PKT_RX_TIME(_p) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u8RxTime)

#if CFG_SUPPORT_WED_PROXY
#define GLUE_RX_SET_PKT_PPE_TYPE(_p, _idx) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u4PpeType = (uint32_t)(_idx))

#define GLUE_RX_GET_PKT_PPE_TYPE(_p) \
	(GLUE_GET_PKT_PRIVATE_RX_DATA(_p)->u4PpeType)
#endif

#define GLUE_GET_TX_PKT_ETHER_DEST_ADDR(_p)    \
		(((struct sk_buff *)(_p))->data)
#define GLUE_GET_TX_PKT_ETHER_SRC_ADDR(_p)    \
		(&((struct sk_buff *)(_p))->data[MAC_ADDR_LEN])

#define GLUE_COPY_PRIV_DATA(_pDst, _pSrc) \
	(kalMemCopy(GLUE_GET_PKT_PRIVATE_DATA(_pDst), \
	GLUE_GET_PKT_PRIVATE_DATA(_pSrc), sizeof(struct PACKET_PRIVATE_DATA)))

/* Check validity of prDev, private data, and pointers */
#define GLUE_CHK_DEV(prDev) \
	((prDev && *((struct GLUE_INFO **) netdev_priv(prDev))) ? TRUE : FALSE)

#define GLUE_CHK_PR2(prDev, pr2) \
	((GLUE_CHK_DEV(prDev) && pr2) ? TRUE : FALSE)

#define GLUE_CHK_PR3(prDev, pr2, pr3) \
	((GLUE_CHK_PR2(prDev, pr2) && pr3) ? TRUE : FALSE)

#define GLUE_CHK_PR4(prDev, pr2, pr3, pr4) \
	((GLUE_CHK_PR3(prDev, pr2, pr3) && pr4) ? TRUE : FALSE)

#define GLUE_SET_EVENT(pr) \
	kalSetEvent(pr)

/*
 * For legacy kernel, create new MACRO by ourself
 * Otherwise, may use __atomic_op_fence()
 */
#define GLUE_FENCE_ATOMIC(op, args...) \
({ \
	typeof(op(args)) __ret;	 \
	KAL_MB_RW(); \
	__ret = op(args); \
	KAL_MB_RW(); \
	__ret; \
})
#define GLUE_FENCE_ATOMIC_NO_RETUEN(op, args...) \
({ \
	KAL_MB_RW(); \
	op(args); \
	KAL_MB_RW(); \
})
#define GLUE_INC_REF_CNT(_refCount) \
	GLUE_FENCE_ATOMIC(atomic_add_return, 1, ((atomic_t *)&(_refCount)))
#define GLUE_DEC_REF_CNT(_refCount) \
	GLUE_FENCE_ATOMIC(atomic_sub_return, 1, ((atomic_t *)&(_refCount)))
#define GLUE_SET_REF_CNT(_value, _refCount) \
	GLUE_FENCE_ATOMIC_NO_RETUEN( \
		atomic_set, ((atomic_t *)&(_refCount)), _value)
#define GLUE_GET_REF_CNT(_refCount)  \
	GLUE_FENCE_ATOMIC(atomic_read, ((atomic_t *)&(_refCount)))
#define GLUE_ADD_REF_CNT(_value, _refCount) \
	GLUE_FENCE_ATOMIC(atomic_add_return, _value, ((atomic_t *)&(_refCount)))
#define GLUE_SUB_REF_CNT(_value, _refCount) \
	GLUE_FENCE_ATOMIC(atomic_sub_return, _value, ((atomic_t *)&(_refCount)))

#define DbgPrint(...)

#define GLUE_SYMBOL_GET(fun_name)	__symbol_get(fun_name)
#define GLUE_SYMBOL_PUT(fun_name)	__symbol_put(fun_name)

#if (CFG_ENABLE_GKI_SUPPORT != 0)
#define GLUE_LOOKUP_FUN(fun_name)	NULL
#else
#if KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE
#define GLUE_LOOKUP_FUN(fun_name)	NULL
#else
#ifdef CONFIG_KALLSYMS
#define GLUE_LOOKUP_FUN(fun_name)	kallsyms_lookup_name
#else
#define GLUE_LOOKUP_FUN(fun_name)	NULL
#endif
#endif
#endif

#if CFG_MET_TAG_SUPPORT
#define GL_MET_TAG_START(_id, _name)	met_tag_start(_id, _name)
#define GL_MET_TAG_END(_id, _name)	met_tag_end(_id, _name)
#define GL_MET_TAG_ONESHOT(_id, _name, _value) \
	met_tag_oneshot(_id, _name, _value)
#define GL_MET_TAG_DISABLE(_id)		met_tag_disable(_id)
#define GL_MET_TAG_ENABLE(_id)		met_tag_enable(_id)
#define GL_MET_TAG_REC_ON()		met_tag_record_on()
#define GL_MET_TAG_REC_OFF()		met_tag_record_off()
#define GL_MET_TAG_INIT()		met_tag_init()
#define GL_MET_TAG_UNINIT()		met_tag_uninit()
#else
#define GL_MET_TAG_START(_id, _name)
#define GL_MET_TAG_END(_id, _name)
#define GL_MET_TAG_ONESHOT(_id, _name, _value)
#define GL_MET_TAG_DISABLE(_id)
#define GL_MET_TAG_ENABLE(_id)
#define GL_MET_TAG_REC_ON()
#define GL_MET_TAG_REC_OFF()
#define GL_MET_TAG_INIT()
#define GL_MET_TAG_UNINIT()
#endif

#define MET_TAG_ID	0

#if CFG_SUPPORT_PCIE_GEN_SWITCH
#define PCIE_GEN1    1
#define PCIE_GEN3    3
#endif

/*----------------------------------------------------------------------------*/
/* Macros of Data Type Check                                                  */
/*----------------------------------------------------------------------------*/
/* Kevin: we don't have to call following function to inspect the data
 * structure.
 * It will check automatically while at compile time.
 */
static __KAL_INLINE__ void glPacketDataTypeCheck(void)
{
	DATA_STRUCT_INSPECTING_ASSERT(sizeof(struct
		PACKET_PRIVATE_DATA) <= sizeof(((struct sk_buff *) 0)->cb));
}

#if KERNEL_VERSION(2, 6, 34) > LINUX_VERSION_CODE
#define netdev_for_each_mc_addr(mclist, dev) \
	for (mclist = dev->mc_list; mclist; mclist = mclist->next)
#endif

#if KERNEL_VERSION(2, 6, 34) > LINUX_VERSION_CODE
#define GET_ADDR(ha) (ha->da_addr)
#else
#define GET_ADDR(ha) (ha->addr)
#endif

#if KERNEL_VERSION(2, 6, 35) <= LINUX_VERSION_CODE
#define LIST_FOR_EACH_IPV6_ADDR(_prIfa, _ip6_ptr) \
	list_for_each_entry(_prIfa, &((struct inet6_dev *) \
	_ip6_ptr)->addr_list, if_list)
#else
#define LIST_FOR_EACH_IPV6_ADDR(_prIfa, _ip6_ptr) \
	for (_prIfa = ((struct inet6_dev *) _ip6_ptr)->addr_list; _prIfa; \
	_prIfa = _prIfa->if_next)
#endif

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if WLAN_INCLUDE_PROC
int32_t procCreateFsEntry(struct GLUE_INFO *prGlueInfo);
int32_t procRemoveProcfs(void);


int32_t procInitFs(void);
int32_t procUninitProcFs(void);



int32_t procInitProcfs(struct net_device *prDev,
		       char *pucDevName);
#endif /* WLAN_INCLUDE_PROC */

#if WLAN_INCLUDE_SYS
int sysFwLogInit(void);
void sysFwLogUninit(void);
int32_t sysCreateFsEntry(struct GLUE_INFO *prGlueInfo);
int32_t sysRemoveSysfs(void);
int32_t sysInitFs(void);
int32_t sysUninitSysFs(void);
void sysMacAddrOverride(uint8_t *prMacAddr);
void sysResetTrigger(void);
void sysHangRecoveryReport(void);
void sysInitWifiVer(void);
void sysGetExtCfg(struct ADAPTER *prAdapter);
#endif /* WLAN_INCLUDE_SYS */

#if CFG_ENABLE_BT_OVER_WIFI
u_int8_t glRegisterAmpc(struct GLUE_INFO *prGlueInfo);

u_int8_t glUnregisterAmpc(struct GLUE_INFO *prGlueInfo);
#endif

#if CFG_ENABLE_WIFI_DIRECT
void p2pSetMulticastListWorkQueueWrapper(struct GLUE_INFO
		*prGlueInfo);
#endif

struct GLUE_INFO *wlanGetGlueInfo(void);
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb,
		    struct net_device *sb_dev);
#elif KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		struct sk_buff *skb,
		struct net_device *sb_dev, select_queue_fallback_t fallback);
#elif KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb,
		    void *accel_priv, select_queue_fallback_t fallback);
#elif KERNEL_VERSION(3, 13, 0) <= LINUX_VERSION_CODE
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb,
		    void *accel_priv);
#else
u16 wlanSelectQueue(struct net_device *dev,
		    struct sk_buff *skb);
#endif

void wlanDebugInit(void);

uint32_t wlanSetDriverDbgLevel(uint32_t u4DbgIdx,
			       uint32_t u4DbgMask);

uint32_t wlanGetDriverDbgLevel(uint32_t u4DbgIdx,
			       uint32_t *pu4DbgMask);

void wlanSetSuspendMode(struct GLUE_INFO *prGlueInfo,
			u_int8_t fgEnable);

void wlanGetConfig(struct ADAPTER *prAdapter);

uint32_t wlanDownloadBufferBin(struct ADAPTER *prAdapter);

uint32_t wlanConnacDownloadBufferBin(struct ADAPTER
				     *prAdapter);

uint32_t wlanConnac2XDownloadBufferBin(struct ADAPTER *prAdapter);
#if (CFG_SUPPORT_CONNAC3X == 1)
uint32_t wlanConnac3XDownloadBufferBin(struct ADAPTER *prAdapter);
#endif

#if CFG_CHIP_RESET_SUPPORT
int32_t wlanOffAtReset(void);

int32_t wlanOnAtReset(void);
#endif

u_int8_t wlanIsProbing(void);
u_int8_t wlanIsRemoving(void);

/*******************************************************************************
 *			 E X T E R N A L   F U N C T I O N S / V A R I A B L E
 *******************************************************************************
 */
extern struct net_device *gPrP2pDev[KAL_P2P_NUM];
extern struct wireless_dev *gprWdev[KAL_AIS_NUM];
extern uint32_t g_u4DevIdx[KAL_P2P_NUM];
extern enum ENUM_NVRAM_STATE g_NvramFsm;

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
extern bool fgIsPreOnProcessing;
#endif
#ifdef CFG_DRIVER_INF_NAME_CHANGE
extern char *gprifnameap;
extern char *gprifnamep2p;
extern char *gprifnamesta;
#endif /* CFG_DRIVER_INF_NAME_CHANGE */

void wlanRegisterInetAddrNotifier(void);
void wlanUnregisterInetAddrNotifier(void);
void wlanRegisterNetdevNotifier(void);
void wlanUnregisterNetdevNotifier(void);
#if CFG_POWER_OFF_CTRL_SUPPORT
extern void wlanRegisterRebootNotifier(void);
extern void wlanUnregisterRebootNotifier(void);
#endif

#if CFG_MTK_ANDROID_WMT && CFG_TESTMODE_WMT_WIFI_ON_SUPPORT
typedef void (*set_test_mode_on) (const int);
extern void register_set_wifi_test_mode_fwdl_handler(
	set_test_mode_on handler);
#endif

#if CFG_MTK_ANDROID_WMT
typedef int (*set_p2p_mode) (struct net_device *netdev,
			     struct PARAM_CUSTOM_P2P_SET_STRUCT p2pmode);
extern void register_set_p2p_mode_handler(
	set_p2p_mode handler);
#endif

#if ((CFG_MTK_ANDROID_WMT) && (CFG_TESTMODE_WMT_WIFI_ON_SUPPORT))
typedef uint8_t (*is_wifi_in_test_mode) (struct net_device *netdev);
extern void register_is_wifi_in_test_mode_handler(
	is_wifi_in_test_mode handler);
#endif

#if CFG_ENABLE_EARLY_SUSPEND
extern int glRegisterEarlySuspend(struct early_suspend
				  *prDesc,
				  early_suspend_callback wlanSuspend,
				  late_resume_callback wlanResume);

extern int glUnregisterEarlySuspend(struct early_suspend
				    *prDesc);
#endif

#if CFG_MET_PACKET_TRACE_SUPPORT
void kalMetTagPacket(struct GLUE_INFO *prGlueInfo,
		     void *prPacket, enum ENUM_TX_PROFILING_TAG eTag);

void kalMetInit(struct GLUE_INFO *prGlueInfo);
#endif

void wlanUpdateChannelTable(struct GLUE_INFO *prGlueInfo);

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

#if (CFG_MTK_ANDROID_WMT || WLAN_INCLUDE_PROC) && CFG_ENABLE_WIFI_DIRECT
int set_p2p_mode_handler(struct net_device *netdev,
			 struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT p2pmode);
#endif

#if CFG_SUPPORT_NAN
int set_nan_handler(struct net_device *netdev, uint32_t ucEnable,
	uint8_t fgIsHoldRtnlLock);
#endif

const struct net_device_ops *wlanGetNdevOps(void);

netdev_tx_t wlanHardStartXmit(struct sk_buff *prSkb, struct net_device *prDev);

typedef uint8_t (*file_buf_handler) (void *ctx,
			const char __user *buf,
			uint16_t length);
extern void register_file_buf_handler(file_buf_handler handler,
			void *ctx,
			uint8_t ucType);
/* extern from wifi wmt cdev wifi */
extern uint32_t get_low_latency_mode(void);

extern const uint8_t *kalFindIeMatchMask(uint8_t eid,
				const uint8_t *ies, int len,
				const uint8_t *match,
				int match_len, int match_offset,
				const uint8_t *match_mask);

extern const uint8_t *kalFindIeExtIE(uint8_t eid,
				uint8_t exteid,
				const uint8_t *ies, int len);

extern const uint8_t *kalFindVendorIe(uint32_t oui, int type,
				const uint8_t *ies, int len);

void wlanNvramSetState(enum ENUM_NVRAM_STATE state);
enum ENUM_NVRAM_STATE wlanNvramGetState(void);

#if (CFG_SUPPORT_CONNFEM == 1 && CFG_CONNFEM_DEFAULT == 1)
uint32_t wlanConnFemGetId(void);
#endif

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
int connsys_power_event_notification(enum conn_pwr_event_type type,
	void *data);
void power_throttling_start(void);
void power_throttling_stop(void);
void power_throttling_pre_start(void);
void power_throttling_post_stop(void);
#endif

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
uint32_t getFWLogOnOff(void);
uint32_t getFWLogLevel(void);
uint32_t connsysFwLogControl(struct ADAPTER *prAdapter,
	void *pvSetBuffer, uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);
#endif
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
int32_t sysCreateMonDbgFs(struct GLUE_INFO *prGlueInfo);
void sysRemoveMonDbgFs(void);
#endif

#if CFG_CHIP_RESET_SUPPORT && !CFG_WMT_RESET_API_SUPPORT
extern void WfsysResetHdlr(struct work_struct *work);
#endif

#if CFG_MTK_ANDROID_WMT
extern void update_driver_loaded_status(uint8_t loaded);
#if CFG_SUPPORT_CONNAC1X
extern int mtk_wcn_consys_hw_wifi_paldo_ctrl(unsigned int enable);
#else
extern int mtk_wcn_wlan_reg(
	struct MTK_WCN_WLAN_CB_INFO *pWlanCbInfo);
extern int mtk_wcn_wlan_unreg(void);
#endif /* CFG_SUPPORT_CONNAC1X */
#endif /* CFG_MTK_ANDROID_WMT */

#if CFG_SUPPORT_CUSTOM_NETLINK
extern void glCustomGenlInit(void);
extern void glCustomGenlDeinit(void);
#endif

struct wiphy *wlanGetWiphy(void);

uint8_t wlanGetBssIdx(struct net_device *ndev);

struct net_device *wlanGetNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex);

struct net_device *wlanGetAisNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucAisIndex);

struct net_device *wlanGetP2pNetDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucP2pIndex);

u_int8_t wlanIsAisDev(struct net_device *prDev);

void wlanNotifyFwSuspend(struct GLUE_INFO *prGlueInfo,
			 struct net_device *prDev, u_int8_t fgSuspend);

#if CFG_MTK_ANDROID_WMT
uint8_t kalGetShutdownState(void);
#endif
#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
void wlanShutdown(void);
#endif
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
int wlanFuncPreOnImpl(void);
#endif
int wlanFuncOnImpl(void);
void wlanFuncOffImpl(void);
int wlanFuncOn(void);
int wlanFuncOff(void);

#endif /* _GL_OS_H */
