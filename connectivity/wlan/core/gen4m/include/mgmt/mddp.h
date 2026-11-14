/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "mddp.h"
 *    \brief  The declaration of nic functions
 *
 *    Detail description.
 */


#ifndef _MDDP_H
#define _MDDP_H

#if CFG_MTK_MDDP_SUPPORT

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 0)
#include "mddp_export.h"
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP == 0 */

#if CFG_MTK_CCCI_SUPPORT
#include "mtk_ccci_common.h"
#endif

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MD_ON_OFF_TIMEOUT			1000
#define MD_ON_OFF_TIMEOUT_CASAN		3000
#if (CFG_SUPPORT_CONNAC3X == 1)
#define MD_STATUS_SYNC_CR 0
#elif (CFG_SUPPORT_CONNAC2X == 1)
#define MD_STATUS_SYNC_CR 0x180600F4
#define MD_LPCTL_ADDR 0x7C060050
#else
#define MD_STATUS_SYNC_CR 0x1800701C
#define MD_LPCTL_ADDR 0x7030
#endif
#define MD_STATUS_INIT_SYNC_BIT BIT(0)
#define MD_STATUS_OFF_SYNC_BIT  BIT(1)
#define MD_STATUS_ON_SYNC_BIT   BIT(2)

#define MD_AOR_SET_CR_ADDR 0x10001BEC
#define MD_AOR_CLR_CR_ADDR 0x10001BF0
#define MD_AOR_RD_CR_ADDR  0x10001BF4
#define MD_AOR_MD_INIT_BIT BIT(8)
#define MD_AOR_MD_OFF_BIT  BIT(9)
#define MD_AOR_MD_ON_BIT   BIT(10)
#define MD_AOR_WIFI_ON_BIT BIT(11)

#define MD_SHM_AP_STAT_BIT BIT(4)
#define MD_SHM_MD_INIT_BIT BIT(8) /* md_stat */
#define MD_SHM_MD_OFF_BIT  BIT(9)
#define MD_SHM_MD_ON_BIT   BIT(10)
#define MD_SHM_WIFI_ON_BIT (BIT(11) | MD_SHM_AP_STAT_BIT) /* ap_stat */

#if (CFG_SUPPORT_CONNAC2X == 0 && CFG_SUPPORT_CONNAC3X == 0)
/* Use SER dummy register for mddp support flag */
#define MDDP_SUPPORT_CR 0x820600d0
#define MDDP_SUPPORT_CR_BIT BIT(23)
#endif

#define MDDP_LPCR_MD_SET_FW_OWN BIT(0)

#if (CFG_PCIE_GEN_SWITCH == 1)
#define MDDP_GEN_SWITCH_MSG_TIMEOUT	100 /* msec */
#endif /* CFG_PCIE_GET_SWITCH */

#define MDDP_EXP_RST_STR	"RST_MDDP_EXCEPTION:%u"
#define MDDP_EXP_RSN_SIZE	50

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct MDDP_SETTINGS;

enum ENUM_MDDP_SUPPORT_MODE {
	MDDP_SUPPORT_SHM = 0,
	MDDP_SUPPORT_AOP,
	MDDP_SUPPORT_NUM
};

enum ENUM_MDDP_EXCEPTION {
	MDDP_EXP_RX_STALL = 10,
	MDDP_EXP_NUM
};

struct MDDP_STATUS_SYNC_OPS {
	void (*rd)(struct MDDP_SETTINGS *prSettings, uint32_t *pu4Val);
	void (*set)(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit);
	void (*clr)(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit);
};

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
#define SEND_SIG_SRC_KERNEL 1
#define MDFPM_TTY_BUF_SZ            256

struct mdfpm_ctrl_msg_t {
	uint32_t                        dest_user_id;
	uint32_t                        msg_id;
	uint32_t                        buf_len;
	uint8_t	                        buf[MDFPM_TTY_BUF_SZ];
} __packed;
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

struct MDDP_SETTINGS {
	struct MDDP_STATUS_SYNC_OPS rOps;
	uint32_t u4SyncAddr;
	uint32_t u4SyncSetAddr;
	uint32_t u4SyncClrAddr;
	uint32_t u4MdInitBit;
	uint32_t u4MdOnBit;
	uint32_t u4MdOffBit;
	uint32_t u4WifiOnBit;
	enum ENUM_MDDP_SUPPORT_MODE u4MDDPSupportMode;
	uint32_t u4MdDrvOwnTimeoutTime;
#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	atomic_t seq;
	int i4PortIdx;
	struct task_struct *notify_md_thread;
	struct mdfpm_ctrl_msg_t ctrl_msg;
	atomic_t md_status;
	uint32_t recv_seq;
	uint8_t drv_own_seq;
	uint8_t is_resp_drv_own;
	uint8_t is_port_open;
	uint8_t is_drv_own_acquired;
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */
};

#if (CFG_PCIE_GEN_SWITCH == 1)
struct mddpw_drv_info_genswitch {
	uint16_t u2Seq;
	uint16_t u2Result;
};


enum ENUM_MDDP_GEN_SWITCH_STATE {
	MDDP_GEN_SWITCH_NORMAL_STATE = 0,
	MDDP_GEN_SWITCH_START_BEGIN_STATE,
	MDDP_GEN_SWITCH_START_END_STATE,
	MDDP_GEN_SWITCH_START_END_SKIP_MD_STATE,
	MDDP_GEN_SWITCH_END_STATE,
	MDDP_GEN_SWITCH_BYPASS_STATE,
	MDDP_GEN_SWITCH_START_ACK_TIMEOUT_STATE
};
#endif /* CFG_PCIE_GET_SWITCH */

#if CFG_SUPPORT_LLS
#define MDDP_LLS_BSS_NUM_V1		4
#define MDDP_LLS_BSS_NUM_V2		6
#endif /* CFG_SUPPORT_LLS */

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
/* copy from mddp */
enum mddpw_drv_info_id {
	MDDPW_DRV_INFO_NONE       = 0,
	MDDPW_DRV_INFO_DEVICE_MAC = 1,
	MDDPW_DRV_INFO_NOTIFY_WIFI_ONOFF = 2,
};

struct mddpw_drv_info_t {
	uint8_t         info_id;
	uint8_t         info_len;
	uint8_t         info[];
};

struct mddpw_drv_info_t_v1 {
	uint8_t         info_id;
	uint8_t         reserve;
	uint16_t        info_len;
	uint8_t         info[];
};

struct mddpw_drv_own_t {
	uint8_t         version;
	uint8_t         resource;
	uint8_t         buf_len;
	uint8_t         buf[];
};

struct mddpw_drv_own_info_t {
	uint8_t         device_id;
	uint8_t         seq_num;
	uint8_t         status;
};

struct mddpw_md_notify_info_t {
	uint8_t         version;      /* current ver = 0 */
	uint8_t         info_type;    /* ref enum mddp_md_notify_info_type */
	uint8_t         buf_len;      /* length start from buf[0] */
	uint8_t         buf[];       /* content that MD need to send to DRV */
};

struct mddpw_coex_intf_info_t {
	unsigned char status; //intf status
	unsigned char ringNum; //useless, follow gen4m
	unsigned int seq; //packet sequence number
} __packed __aligned(1);

#define WIFI_ONOFF_NOTIFICATION_LEN 1
#define MAC_ADDR_LEN            6
struct mddpw_txd_t {
	uint8_t version;
	uint8_t wlan_idx;
	uint8_t sta_idx;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t txd_length;
	uint8_t txd[];
} __packed;

struct mddpw_net_stat_t {
	uint64_t        tx_packets;
	uint64_t        rx_packets;
	uint64_t        tx_bytes;
	uint64_t        rx_bytes;
	uint64_t        tx_errors;
	uint64_t        rx_errors;
};

#define MAX_STAREC_NUM          32 // Max driver support station records
#define MAX_CLIENT_NUM          16 // AP client only support 16 station records
#define VIRTUAL_BUF_SIZE        512 // 4096 bits
#define TID_SIZE                8

/* Rx Reordering - AP to MD */
struct mddpw_ap_virtual_buf_t {
	uint16_t        start_idx;
	uint16_t        end_idx;
	uint8_t         virtual_buf[VIRTUAL_BUF_SIZE];
};

struct mddpw_drv_notify_info_t {
	uint8_t         version;
	uint8_t         buf_len;
	uint8_t         info_num;
	uint8_t         buf[];
};

struct mddpw_drv_notify_info_t_v1 {
	uint8_t         version;
	uint8_t         reserve_1;
	uint16_t        buf_len;
	uint8_t         reserve_2;
	uint8_t         info_num;
	uint8_t         buf[];
};

#define NW_IF_NAME_LEN_MAX      16
struct mddpw_net_stat_elem_ext_t {
	uint8_t         nw_if_name[NW_IF_NAME_LEN_MAX];
	uint32_t        reserved[2];
	uint64_t        tx_packets;
	uint64_t        rx_packets;
	uint64_t        tx_bytes;
	uint64_t        rx_bytes;
	uint64_t        tx_errors;
	uint64_t        rx_errors;
	uint64_t        tx_dropped;
	uint64_t        rx_dropped;
};

#define MDDP_DOUBLE_BUFFER      2
#define NW_IF_NUM_MAX           4
struct mddpw_net_stat_ext_t {
	uint32_t                         version;
	uint32_t                         reserved;
	uint32_t                         check_flag[2];
	struct mddpw_net_stat_elem_ext_t ifs[MDDP_DOUBLE_BUFFER][NW_IF_NUM_MAX];
};

struct mddpw_sys_stat_t {
	uint32_t        version[2];
	uint32_t        md_stat[4];
	uint32_t        ap_stat[4];
	uint32_t        reserved[2];
};

/* Rx Reordering - MD to AP */
struct mddpw_md_reorder_info_t {
	uint8_t         buf_idx;
	uint8_t         reserved[7];
};

struct mddpw_md_virtual_buf_t {
	uint16_t        start_idx;
	uint16_t        end_idx;
	uint8_t         virtual_buf[VIRTUAL_BUF_SIZE];
};

struct mddpw_md_reorder_sync_table_t {
	// Mapping station record and virtual buffer.
	struct mddpw_md_reorder_info_t reorder_info[MAX_STAREC_NUM];
	struct mddpw_md_virtual_buf_t  virtual_buf[MAX_CLIENT_NUM][TID_SIZE];
};

struct mddpw_ap_reorder_sync_table_t {
	// Mapping station record and virtual buffer.
	struct mddpw_ap_virtual_buf_t  virtual_buf[MAX_CLIENT_NUM][TID_SIZE];
};

typedef int32_t (*drv_cbf_notify_md_info_t) (
		struct mddpw_md_notify_info_t *);

typedef int32_t (*mddpw_cbf_add_txd_t)(struct mddpw_txd_t *);
typedef int32_t (*mddpw_cbf_get_net_stat_t)(struct mddpw_net_stat_t *);
typedef int32_t (*mddpw_cbf_get_ap_rx_reorder_buf_t)(
		struct mddpw_ap_reorder_sync_table_t **);
typedef int32_t (*mddpw_cbf_get_md_rx_reorder_buf_t)(
		struct mddpw_md_reorder_sync_table_t **);
typedef int32_t (*mddpw_cbf_notify_drv_info_t)(
		struct mddpw_drv_notify_info_t *);
typedef int32_t (*mddpw_cbf_get_net_stat_ext_t)(struct mddpw_net_stat_ext_t *);
typedef int32_t (*mddpw_cbf_get_sys_stat_t)(struct mddpw_sys_stat_t **);
typedef int32_t (*mddpw_cbf_get_mddp_feature_t)(void);

struct mddpw_drv_handle_t {
	/* MDDPW invokes these APIs provided by driver. */
	drv_cbf_notify_md_info_t               notify_md_info;

	/* Driver invokes these APIs provided by MDDPW. */
	mddpw_cbf_add_txd_t                    add_txd;
	mddpw_cbf_get_net_stat_t               get_net_stat;
	mddpw_cbf_get_ap_rx_reorder_buf_t      get_ap_rx_reorder_buf;
	mddpw_cbf_get_md_rx_reorder_buf_t      get_md_rx_reorder_buf;
	mddpw_cbf_notify_drv_info_t            notify_drv_info;
	mddpw_cbf_get_net_stat_ext_t           get_net_stat_ext;
	mddpw_cbf_get_sys_stat_t               get_sys_stat;
	mddpw_cbf_get_mddp_feature_t           get_mddp_feature;
};

enum mddp_app_type_e {
	MDDP_APP_TYPE_RESERVED_1 = 0,
	MDDP_APP_TYPE_WH,

	MDDP_APP_TYPE_CNT,

	MDDP_APP_TYPE_ALL = 0xff,
	MDDP_APP_TYPE_DUMMY = 0x7fff /* Mark it a 2-byte enum. */
};
struct mddp_drv_conf_t {
	enum mddp_app_type_e app_type;
};

enum mddp_state_e {
	MDDP_STATE_UNINIT = 0,
	MDDP_STATE_ENABLING,
	MDDP_STATE_DEACTIVATED,
	MDDP_STATE_ACTIVATING,
	MDDP_STATE_ACTIVATED,
	MDDP_STATE_DEACTIVATING,
	MDDP_STATE_DISABLING,
	MDDP_STATE_DISABLED,

	MDDP_STATE_CNT,
	MDDP_STATE_DUMMY = 0x7fff /* Make it a 2-byte enum. */
};

enum md_notify_info_type {
	MD_NOTIFY_INFO_ONOFF = 5, //recv seq from MD
	MD_NOTIFY_INFO_DRV_OWN_RELEASE = 6,
	MD_NOTIFY_INFO_INVAILD
};

typedef int32_t (*drv_cbf_change_state_t)(
	enum mddp_state_e state, void *buf, uint32_t *buf_len);

struct mddp_drv_handle_t {
	/* MDDP invokes these APIs provided by driver. */
	drv_cbf_change_state_t          change_state;

	/* Application layer handler. */
	union {
		struct mddpw_drv_handle_t     *wifi_handle;
	};
};

#define MDDP_FEATURE_MCIF_WIFI (1<<1)
#define MDDP_FEATURE_MDDP_WH   (1<<2)

#define CCCI_PORT_NAME	"ccci_wifi7"

#define COEX_NOTIFY_MAX_SEQ 0x1FF
#define CHECK_MD_STATUS_TIME 50 //unit:ms
#define CHECK_MD_STATUS_MAX_COUNT 5 //unit:ms

#define STATUS_SUCCESS 0x00
#define STATUS_FAILURE 0x01
#define STATUS_TIMEOUT 0x02

enum ccci_user_id {
	CCCI_USER_ID_RESERVED,
	CCCI_USER_ID_COEX,
	CCCI_USER_ID_MAX
};

enum ccci_msg_id {
	CCCI_MSG_ID_COEX_NOTIFY = 15,
	CCCI_MSG_ID_MD_NOTIFY = 16,
	CCCI_MSG_ID_RESET_IND = 17,
	CCCI_MSG_ID_MD_REQ = 48,
	CCCI_MSG_ID_DRV_RSP = 49,
};
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
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
void mddpInit(int bootmode);
void mddpUninit(void);
void mddpNotifyMdCrash(struct ADAPTER *prAdapter);
void mddpInHifThread(struct ADAPTER *prAdapter);
void mddpTriggerMdFwOwnByFw(struct ADAPTER *prAdapter);
void mddpTriggerMdSerRecovery(struct ADAPTER *prAdapter);
void mddpTriggerReset(struct ADAPTER *prAdapter, uint32_t u4RstFlag);
void mddpTriggerMdSer(struct ADAPTER *prAdapter);
int32_t mddpMdNotifyInfo(struct mddpw_md_notify_info_t *prMdInfo);
int32_t mddpChangeState(enum mddp_state_e event, void *buf, uint32_t *buf_len);
int32_t mddpGetMdStats(struct net_device *prDev);
#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
int32_t mddpGetMdLlsStats(struct ADAPTER *prAdapter);
#endif
void mddpUpdateReorderQueParm(struct ADAPTER *prAdapter,
			      struct RX_BA_ENTRY *prReorderQueParm,
			      struct SW_RFB *prSwRfb);
int32_t mddpNotifyDrvTxd(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t fgActivate);
int32_t mddpNotifyCheckSer(uint32_t u4Status);
int32_t mddpNotifyStaTxd(struct ADAPTER *prAdapter);
void mddpNotifyWifiOnStart(u_int8_t fgIsForce);
int32_t mddpNotifyWifiOnEnd(u_int8_t fgIsForce);
void mddpNotifyWifiOffStart(void);
void mddpNotifyWifiOffEnd(void);
void mddpUnregisterMdStateCB(void);
void mddpNotifyWifiReset(void);
void setMddpSupportRegister(struct ADAPTER *prAdapter);
#if CFG_MTK_CCCI_SUPPORT
void mddpMdStateChangedCb(enum MD_STATE old_state,
		enum MD_STATE new_state);
#endif
void mddpSetMDFwOwn(void);
void mddpEnableMddpSupport(void);
void mddpDisableMddpSupport(void);
bool mddpIsSupportMcifWifi(void);
bool mddpIsSupportMddpWh(void);

#if (CFG_PCIE_GEN_SWITCH == 1)
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void mddpGenSwitchMsgTimeout(struct timer_list *timer);
#else
void mddpGenSwitchMsgTimeout(unsigned long arg);
#endif
int32_t mddpNotifyMDGenSwitchStart(struct ADAPTER *prAdapter);
int32_t mddpNotifyMDGenSwitchEnd(struct ADAPTER *prAdapter);
int32_t mddpMdNotifyInfoHandleGenSwitchByPassStart(
	struct ADAPTER *prAdapter,
	struct mddpw_md_notify_info_t *prMdInfo);
int32_t mddpMdNotifyInfoHandleGenSwitchByPassEnd(
	struct ADAPTER *prAdapter,
	struct mddpw_md_notify_info_t *prMdInfo);
uint32_t mddpGetGenSwitchState(struct ADAPTER *prAdapter);
#endif /* CFG_PCIE_GEN_SWITCH */

#if defined(_HIF_PCIE)
#if CFG_SUPPORT_PCIE_ASPM
int32_t mddpNotifyMDPCIeL12Status(uint8_t fgEnable);
#endif
#endif

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
/* add for mddp light */
void mddpStartMdRxThread(void);
void mddpStopMdRxThread(void);
int32_t mddpDrvNotifyInfo(struct mddpw_drv_notify_info_t *prDrvInfo);
bool mddpIsSupportCcci(void);
extern int mtk_ccci_open_port(char *port_name); /* return fd/index */
extern int mtk_ccci_read_data(int index, char *buf, size_t count);
extern int mtk_ccci_write_data(int index, char *buf, int size);
extern int mtk_ccci_close_port(int index);
extern enum MD_STATE ccci_fsm_get_md_state(int md_id);
int md_rx_handler(void *data);
void mddpNotifyDrvOwn(uint32_t u4Status);
bool mddpIsMdDrvOwnAcquired(void);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */
#endif /* CFG_MTK_MDDP_SUPPORT */

#endif /* _MDDP_H */
