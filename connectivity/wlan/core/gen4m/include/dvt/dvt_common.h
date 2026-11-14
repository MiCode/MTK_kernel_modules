/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "dvt_common.h"
 *    \brief This file contains the declairations of sys dvt command
 */

#ifndef _DVT_COMMON_H
#define _DVT_COMMON_H

#if CFG_SUPPORT_WIFI_SYSDVT

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *			E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define PID_SIZE 256
#define TXS_LIST_ELEM_NUM 4096
#define TX_TEST_UNLIMITIED 0xFFFF
#define TX_TEST_UP_UNDEF   0xFF
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define IS_SKIP_CH_CHECK(_prAdapter) \
	((_prAdapter)->auto_dvt && (_prAdapter)->auto_dvt->skip_legal_ch_enable)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* UNI_CMD for MDVT */
enum ENUM_MDVT_MODULE_T {
	MDVT_MODULE_WFARB = 0,
	MDVT_MODULE_AGG,
	MDVT_MODULE_DMA,
	MDVT_MODULE_WFMIMO,
	MDVT_MODULE_WFCTRL,
	MDVT_MODULE_WFETXBF,
	MDVT_MODULE_WFCFG,
	MDVT_MODULE_WFHIF,
	MDVT_MODULE_WFOFF,
	MDVT_MODULE_WFON,
	MDVT_MODULE_WFPF = 10,
	MDVT_MODULE_WFRMAC,
	MDVT_MODULE_WFUMAC_PLE,
	MDVT_MODULE_WFUMAC_PSE,
	MDVT_MODULE_WFUMAC_PP,
	MDVT_MODULE_WFUMAC_AMSDU,
	MDVT_MODULE_WFSEC,
	MDVT_MODULE_WFTMAC,
	MDVT_MODULE_WFTMAC_TXPWR,
	MDVT_MODULE_WFTXCR,
	MDVT_MODULE_WFMIB = 20,
	MDVT_MODULE_WFSYSON,
	MDVT_MODULE_WFLPON,
	MDVT_MODULE_WFINT,
	MDVT_MODULE_CONNCFG,
	MDVT_MODULE_MUCOP,
	MDVT_MODULE_WFMDP,
	MDVT_MODULE_WFRDM_PHYRX,
	MDVT_MODULE_WFRDM_PHYDFS,
	MDVT_MODULE_WFRDM_PHYRX_COMM,
	MDVT_MODULE_WFRDM_WTBLOFF = 30,
	MDVT_MODULE_PHYDFE_CTRL_WF_TSSI,
	MDVT_MODULE_PHYDFE_RFINTF_WF_CMM,
	MDVT_MODULE_PHYRX_CTRL_WF_COMM_RDD,
	MDVT_MODULE_PHYRX_CTRL_WF_COMM_CSI,
	MDVT_MODULE_PHYRX_CTRL_WF_COMM_CMM,
	MDVT_MODULE_PHYRX_CTRL_WF_COMM_TOAE,
	MDVT_MODULE_PHYRX_CSD_WF_COMM_CMM,
	MDVT_MODULE_PHYRX_POST_CMM,
	MDVT_MODULE_PHYDFS_WF_COMM_RDD,
	MDVT_MODULE_PHYRX_CTRL_TOAE = 40,
	MDVT_MODULE_PHYRX_CTRL_MURU,
	MDVT_MODULE_PHYRX_CTRL_RDD,
	MDVT_MODULE_PHYRX_CTRL_MULQ,
	MDVT_MODULE_PHYRX_CTRL_CMM,
	MDVT_MODULE_PHYRX_CTRL_CSI,
	MDVT_MODULE_PHYDFE_CTRL_PWR_REGU,
	MDVT_MODULE_PHYRX_CTRL_BF,
	MDVT_MODULE_PHYDFE_CTRL_CMM,
	MDVT_MODULE_WFRBIST,
	MDVT_MODULE_WTBL = 50,
	MDVT_MODULE_RX,
	MDVT_MODULE_LPON,
	MDVT_MODULE_MDP_RX,
	MDVT_MODULE_TXCMD,
	MDVT_MODULE_SEC_ECC,
	MDVT_MODULE_MIB,
	MDVT_MODULE_WFTWT,
	MDVT_MODULE_DRR,
	MDVT_MODULE_RUOFDMA,
	MDVT_MODULE_WFCMDRPTTX = 60,
	MDVT_MODULE_WFCMDRPT_TRIG,
	MDVT_MODULE_MLO,
	MDVT_MODULE_TXD,
	MDVT_MODULE_PH_TPUT,
	MDVT_MODULE_SER,
	MDVT_MODULE_LIT_WTBL,
	MDVT_MODULE_LITMIB,
	MDVT_MODULE_LIT_WFRMAC,
	MDVT_MODULE_PTA_IDC_COEX,
	MDVT_MODULE_MUMIMO = 70,
	MDVT_MODULE_PRMBPUNC,
	MDVT_MODULE_CERT,
	MDVT_MODULE_SR,
	MDVT_MODULE_ARB_COEX,
	MDVT_MODULE_BF,
	MDVT_MODULE_CMD_DECODER,
	MDVT_MODULE_COSIM,
	MDVT_MODULE_RLM_CMM,
	MDVT_MODULE_WFDMA,
	MDVT_MODULE_SDO = 80,
	MDVT_MODULE_RRO,
	MDVT_MODULE_AIRTIME,
	MDVT_MODULE_BFTXD,
	MDVT_MODULE_PRMBPUNC_TXD,
	MDVT_MODULE_UMAC_CFG,
	MDVT_MODULE_FDD_COEX,
	MDVT_MODULE_MAX
};

enum ENUM_SYSDVT_CTRL_EXT_T {
	EXAMPLE_FEATURE_ID = 0,
	SYSDVT_CTRL_EXT_NUM
};

enum ENUM_AUTOMATION_TXS_TESTYPE {
	TXS_INIT = 0,
	TXS_COUNT_TEST,
	TXS_BAR_TEST,
	TXS_DEAUTH_TEST,
	TXS_RTS_TEST,
	TXS_BA_TEST,
	TXS_DUMP_DATA,
	TXS_NUM
};

#if CFG_TCP_IP_CHKSUM_OFFLOAD
enum ENUM_AUTOMATION_CSO_TESTYPE {
	CSO_TX_IPV4 = BIT(0),
	CSO_TX_IPV6 = BIT(1),
	CSO_TX_TCP = BIT(2),
	CSO_TX_UDP = BIT(3),
	CSO_RX_IPV4 = BIT(4),
	CSO_RX_IPV6 = BIT(5),
	CSO_RX_TCP = BIT(6),
	CSO_RX_UDP = BIT(7),
};
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

enum ENUM_AUTOMATION_INIT_TYPE {
	TXS = 0,
	RXV,
#if (CFG_SUPPORT_DMASHDL_SYSDVT)
	DMASHDL,
#endif
	CSO,
	SKIP_CH
};

enum ENUM_AUTOMATION_FRAME_TYPE {
	AUTOMATION_MANAGEMENT = 10,
	AUTOMATION_CONTROL,
	AUTOMATION_DATA
};

struct SYSDVT_CTRL_EXT_T {
	/* DWORD_0 - Common Part */
	uint8_t ucCmdVer;
	uint8_t aucPadding0[1];
	uint16_t u2CmdLen;	/* Cmd size including common part and body */

	/* DWORD_N - Body Part */
	uint32_t u4FeatureIdx;	/* Feature  ID */
	uint32_t u4Type;	/* Test case  ID (Type) */
	uint32_t u4Lth;	/* dvt parameter's data struct size (Length) */
	uint8_t u1cBuffer[];	/* dvt parameter's data struct (Value) */
};

struct SYS_DVT_HANDLER {
	uint32_t u4FeatureIdx;
	uint8_t *str_feature_dvt;
	int32_t (*pFeatureDvtHandler)(struct ADAPTER *, uint8_t *);
	void (*pDoneHandler)(struct ADAPTER *, struct CMD_INFO *, uint8_t *);
};

struct TXS_CHECK_ITEM {
	uint32_t time_stamp;
};

struct TXS_LIST_ENTRY {
	struct list_head mList;
	uint8_t wlan_idx;
};

struct TXS_LIST_POOL {
	struct TXS_LIST_ENTRY Entry[TXS_LIST_ELEM_NUM];
	struct list_head List;
};

struct TXS_FREE_LIST_POOL {
	struct TXS_LIST_ENTRY head;
	struct TXS_LIST_POOL pool_head;
	uint32_t entry_number;
	spinlock_t Lock;
	uint32_t txs_list_cnt;
};

struct TXS_LIST {
	uint32_t Num;
	spinlock_t lock;
	struct TXS_LIST_ENTRY pHead[PID_SIZE];
	struct TXS_FREE_LIST_POOL *pFreeEntrylist;
};

struct TXS_TEST {
	bool isinit;
	uint32_t test_type;
	uint8_t format;

	uint8_t	pid;
	uint8_t	received_pid;
	bool	stop_send_test;
	bool	duplicate_txs;

	/* statistic */
	uint32_t total_req;
	uint32_t total_rsp;
	struct TXS_LIST txs_list;
	struct TXS_CHECK_ITEM check_item[PID_SIZE];
};

struct RXV_TEST {
	bool enable;
	bool rxv_test_result;
	uint32_t rx_count;

	uint32_t rx_mode:3;
	uint32_t rx_rate:7;
	uint32_t rx_bw:2;
	uint32_t rx_sgi:1;
	uint32_t rx_stbc:2;
	uint32_t rx_ldpc:1;
	uint32_t rx_nss:1;
};

#if (CFG_SUPPORT_DMASHDL_SYSDVT)
struct DMASHDL_TEST {
	uint8_t dvt_item;
	uint8_t dvt_sub_item;
	uint8_t dvt_queue_idx;
	uint8_t dvt_ping_nums[32];
	uint8_t dvt_ping_seq[20];
};
#endif

struct AUTOMATION_DVT {
	uint8_t skip_legal_ch_enable;
	struct TXS_TEST txs;
	struct RXV_TEST rxv;
#if (CFG_SUPPORT_DMASHDL_SYSDVT)
	struct DMASHDL_TEST dmashdl;
#endif
	uint8_t cso_ctrl;
};

struct _FRAME_RTS {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
};

/* 2-byte BAR CONTROL field in BAR frame */
struct _BAR_CONTROL {
	uint16_t ACKPolicy:1; /* 0:normal ack,  1:no ack. */
/*if this bit1, use  FRAME_MTBA_REQ,  if 0, use FRAME_BA_REQ */
	uint16_t MTID:1;
	uint16_t Compressed:1;
	uint16_t Rsv1:9;
	uint16_t TID:4;
};

/* 2-byte BA Starting Seq CONTROL field */
union _BASEQ_CONTROL {
	struct {
	uint16_t FragNum:4;	/* always set to 0 */
/* sequence number of the 1st MSDU for which this BAR is sent */
	uint16_t StartSeq:12;
	} field;
	uint16_t word;
};

struct _FRAME_BA {
	/* MAC header */
	uint16_t u2FrameCtrl;	/* Frame Control */
	uint16_t u2Duration;	/* Duration */
	uint8_t aucDestAddr[MAC_ADDR_LEN];	/* DA */
	uint8_t aucSrcAddr[MAC_ADDR_LEN];	/* SA */
	struct _BAR_CONTROL	BarControl;
	union _BASEQ_CONTROL	StartingSeq;
	unsigned char bitmask[8];
};

/* UNI_CMD for MDVT */
struct _MDVT_MODULE_T {
	uint8_t		eModuleId;
	uint8_t		*pucParserStr;
};

#endif	/* CFG_SUPPORT_WIFI_SYSDVT */

/*******************************************************************************
 *			P U B L I C   D A T A
 *******************************************************************************
 */
#if CFG_SUPPORT_WIFI_SYSDVT

extern const struct _MDVT_MODULE_T arMdvtModuleTable[];
extern uint32_t u4MdvtTableSize;


#endif	/* CFG_SUPPORT_WIFI_SYSDVT */


/*******************************************************************************
 *			P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *			M A C R O S
 *******************************************************************************
 */

#if CFG_TCP_IP_CHKSUM_OFFLOAD
#define CSO_TX_IPV4_ENABLED(pAd)            \
	(pAd->auto_dvt && (pAd->auto_dvt->cso_ctrl & CSO_TX_IPV4))
#define CSO_TX_IPV6_ENABLED(pAd)            \
	(pAd->auto_dvt && (pAd->auto_dvt->cso_ctrl & CSO_TX_IPV6))
#define CSO_TX_UDP_ENABLED(pAd)            \
	(pAd->auto_dvt && (pAd->auto_dvt->cso_ctrl & CSO_TX_UDP))
#define CSO_TX_TCP_ENABLED(pAd)            \
	(pAd->auto_dvt && (pAd->auto_dvt->cso_ctrl & CSO_TX_TCP))
#endif

#define RXV_AUTODVT_DNABLED(pAd)			\
	(pAd->auto_dvt && pAd->auto_dvt->rxv.enable)

/*******************************************************************************
 *			F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_SUPPORT_WIFI_SYSDVT
struct TXS_LIST_ENTRY *GetTxsEntryFromFreeList(void);

int SendRTS(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

int SendBA(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	PFN_TX_DONE_HANDLER pfTxDoneHandler);

bool send_add_txs_queue(uint8_t pid, uint8_t wlan_idx);

bool receive_del_txs_queue(
	uint32_t sn, uint8_t pid,
	uint8_t wlan_idx,
	uint32_t time_stamp);

int priv_driver_txs_test(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen);

int priv_driver_txs_test_result(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen);

int is_frame_test(struct ADAPTER *pAd, uint8_t send_received);

uint32_t AutomationTxDone(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus);

int priv_driver_set_tx_test(
	struct net_device *prNetDev, char *pcCommand,
	int i4TotalLen);
int priv_driver_set_tx_test_ac(
	struct net_device *prNetDev, char *pcCommand,
	int i4TotalLen);

/* RXV Test */
int priv_driver_rxv_test(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen);

int priv_driver_rxv_test_result(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen);

void connac2x_rxv_correct_test(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb);

/* CSO Test */
int priv_driver_cso_test(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen);

/* skip legal channel sanity check for fpga dvt */
int priv_driver_skip_legal_ch_check(
	struct net_device *prNetDev,
	char *pcCommand,
	int i4TotalLen);

bool AutomationInit(struct ADAPTER *pAd, int32_t auto_type);

#endif	/* CFG_SUPPORT_WIFI_SYSDVT */
#endif	/* _DVT_COMMON_H */

