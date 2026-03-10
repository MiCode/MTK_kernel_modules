/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*
** Id: @(#) mddp.c@@
*/

/*! \file   mddp.c
*    \brief  Main routines for modem direct path handling
*
*    This file contains the support routines of modem direct path operation.
*/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

#if CFG_MTK_MDDP_SUPPORT

#include "gl_os.h"
#include "mddp_export.h"
#include "mddp.h"
#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
#include "cnm_mem.h"
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
enum ENUM_MDDPW_DRV_INFO_STATUS {
	MDDPW_DRV_INFO_STATUS_ON_START   = 0,
	MDDPW_DRV_INFO_STATUS_ON_END     = 1,
	MDDPW_DRV_INFO_STATUS_OFF_START  = 2,
	MDDPW_DRV_INFO_STATUS_OFF_END    = 3,
	MDDPW_DRV_INFO_STATUS_ON_END_QOS = 4,
};

enum ENUM_MDDPW_MD_INFO {
	MDDPW_MD_INFO_RESET_IND     = 1,
	MDDPW_MD_INFO_DRV_EXCEPTION = 2,
	MDDPW_MD_EVENT_NOTIFY_MD_INFO_EMI = 3,
	MDDPW_MD_EVENT_COMMUNICATION = 4,
};

/* MDDPW_MD_INFO_DRV_EXCEPTION */
struct wsvc_md_event_exception_t {
	uint32_t u4RstReason;
	uint32_t u4RstFlag;
	uint32_t u4Line;
	char pucFuncName[64];
};

enum EMUM_MD_NOTIFY_REASON_TYPE_T {
	MD_INFORMATION_DUMP = 1,
	MD_DRV_OWN_FAIL,
	MD_INIT_FAIL,
	MD_STATE_ABNORMAL,
	MD_TX_DATA_HANG,
	MD_TX_CMD_FAIL,
	MD_L12_DISABLE, /* 7 */
	MD_L12_ENABLE, /* 8 */
	MD_ENUM_MAX,
};

/* MDDPW_MD_EVENT_COMMUNICATION */
struct wsvc_md_event_comm_t {
	uint32_t u4Reason;
	uint32_t u4RstFlag;
	uint32_t u4Line;
	uint32_t dump_payload[32];
	uint8_t  dump_size;
	char pucFuncName[64];
};

struct mddpw_drv_handle_t gMddpWFunc = {
	.notify_md_info = mddpMdNotifyInfo,
};

struct mddp_drv_conf_t gMddpDrvConf = {
	.app_type = MDDP_APP_TYPE_WH,
};

struct mddp_drv_handle_t gMddpFunc = {
	.change_state = mddpChangeState,
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#define MAC_ADDR_LEN            6

struct mddp_txd_t {
	uint8_t version;
	uint8_t wlan_idx;
	uint8_t sta_idx;
	uint8_t nw_if_name[8];
	uint8_t sta_mode;
	uint8_t bss_id;
	uint8_t wmmset;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t local_mac[MAC_ADDR_LEN];
	uint8_t txd_length;
	uint8_t txd[0];
} __packed;

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

enum BOOTMODE g_wifi_boot_mode = NORMAL_BOOT;
u_int8_t g_fgMddpEnabled = TRUE;
struct MDDP_SETTINGS g_rSettings;
enum ENUM_MDDPW_DRV_INFO_STATUS g_eMddpStatus;
struct mutex rMddpLock;

struct mddpw_net_stat_ext_t stats;
#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
struct wsvc_stat_lls_report_t cur_lls_stats;
struct wsvc_stat_lls_report_t base_lls_stats;
struct wsvc_stat_lls_report_t todo_lls_stats;
u_int8_t isMdResetSinceLastQuery;
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static bool wait_for_md_on_complete(void);
static bool wait_for_md_off_complete(void);
static void save_mddp_stats(void);
#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
static void save_mddp_lls_stats(void);
#endif

static void mddpRdFunc(struct MDDP_SETTINGS *prSettings, uint32_t *pu4Val)
{
	if (!prSettings->u4SyncAddr)
		return;

	wf_ioremap_read(prSettings->u4SyncAddr, pu4Val);
}

static void mddpSetFuncV1(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit)
{
	uint32_t u4Value = 0;

	if (!prSettings->u4SyncAddr || u4Bit == 0)
		return;

	wf_ioremap_read(prSettings->u4SyncAddr, &u4Value);
	u4Value |= u4Bit;
	wf_ioremap_write(prSettings->u4SyncAddr, u4Value);
}

static void mddpClrFuncV1(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit)
{
	uint32_t u4Value = 0;

	if (!prSettings->u4SyncAddr || u4Bit == 0)
		return;

	wf_ioremap_read(prSettings->u4SyncAddr, &u4Value);
	u4Value &= ~u4Bit;
	wf_ioremap_write(prSettings->u4SyncAddr, u4Value);
}

static void mddpSetFuncV2(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit)
{
	if (!prSettings->u4SyncSetAddr || u4Bit == 0)
		return;

	wf_ioremap_write(prSettings->u4SyncSetAddr, u4Bit);
}

static void mddpClrFuncV2(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit)
{
	if (!prSettings->u4SyncClrAddr || u4Bit == 0)
		return;

	wf_ioremap_write(prSettings->u4SyncClrAddr, u4Bit);
}

static void mddpRdFuncSHM(struct MDDP_SETTINGS *prSettings, uint32_t *pu4Val)
{
	struct mddpw_sys_stat_t *prSysStat = NULL;

	if (gMddpWFunc.get_sys_stat) {
		if (gMddpWFunc.get_sys_stat(&prSysStat)) {
			DBGLOG(NIC, ERROR, "get_sys_stat Error.\n");
			goto exit;
		}
		if (prSysStat == NULL) {
			DBGLOG(NIC, ERROR, "prSysStat Null.\n");
			goto exit;
		}
	} else {
		DBGLOG(NIC, ERROR, "get_sys_stat callback NOT exist.\n");
		goto exit;
	}

	*pu4Val = prSysStat->md_stat[0] &
		(prSettings->u4MdOnBit | prSettings->u4MdOffBit);

	if (prSysStat) {
		DBGLOG(QM, TRACE,
			"md_stat: %u, Val: %u\n",
			prSysStat->md_stat[0], *pu4Val);
	}

exit:
	return;
}

static void mddpSetFuncSHM(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit)
{
	uint32_t u4Value = 0;
	struct mddpw_sys_stat_t *prSysStat = NULL;

	if (gMddpWFunc.get_sys_stat) {
		if (gMddpWFunc.get_sys_stat(&prSysStat)) {
			DBGLOG(NIC, ERROR, "get_sys_stat Error.\n");
			goto exit;
		}
		if (prSysStat == NULL) {
			DBGLOG(NIC, ERROR, "prSysStat Null.\n");
			goto exit;
		}
	} else {
		DBGLOG(NIC, ERROR, "get_sys_stat callback NOT exist.\n");
		goto exit;
	}

	if (u4Bit & MD_SHM_AP_STAT_BIT) {
		u4Value = u4Bit & ~MD_SHM_AP_STAT_BIT;
		prSysStat->ap_stat[0] |= u4Value;
	} else
		prSysStat->md_stat[0] |= u4Value;

	if (prSysStat) {
		DBGLOG(QM, TRACE,
			"u4Bit: %u, ap_stat[0]: %u, md_stat[0]: %u val: %u\n",
			u4Bit, prSysStat->ap_stat[0],
			prSysStat->md_stat[0], u4Value);
	}

exit:
	return;
}

static void mddpClrFuncSHM(struct MDDP_SETTINGS *prSettings, uint32_t u4Bit)
{
	uint32_t u4Value = 0;
	struct mddpw_sys_stat_t *prSysStat = NULL;

	if (gMddpWFunc.get_sys_stat) {
		if (gMddpWFunc.get_sys_stat(&prSysStat)) {
			DBGLOG(NIC, ERROR, "get_sys_stat Error.\n");
			goto exit;
		}
		if (prSysStat == NULL) {
			DBGLOG(NIC, ERROR, "prSysStat Null.\n");
			goto exit;
		}
	} else {
		DBGLOG(NIC, ERROR, "get_sys_stat callback NOT exist.\n");
		goto exit;
	}

	if (u4Bit & MD_SHM_AP_STAT_BIT) {
		u4Value = u4Bit & ~MD_SHM_AP_STAT_BIT;
		prSysStat->ap_stat[0] &= ~u4Value;
	} else
		prSysStat->md_stat[0] &= ~u4Value;

	if (prSysStat) {
		DBGLOG(QM, TRACE,
			"u4Bit: %u, ap_stat[%u]: %u, md_stat[%u]: %u\n",
			u4Bit, u4Value, prSysStat->ap_stat[0],
			u4Value, prSysStat->md_stat[0]);
	}

exit:
	return;
}

static int32_t mddpRegisterCb(void)
{
	int32_t ret = 0;

	switch (g_wifi_boot_mode) {
	case RECOVERY_BOOT:
		g_fgMddpEnabled = FALSE;
		break;
	default:
		g_fgMddpEnabled = TRUE;
		break;
	}
	gMddpFunc.wifi_handle = &gMddpWFunc;

	ret = mddp_drv_attach(&gMddpDrvConf, &gMddpFunc);

	DBGLOG(INIT, INFO, "mddp_drv_attach ret: %d, g_fgMddpEnabled: %d\n",
			ret, g_fgMddpEnabled);

	kalMemZero(&stats, sizeof(struct mddpw_net_stat_ext_t));
#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
	kalMemZero(&cur_lls_stats, sizeof(struct wsvc_stat_lls_report_t));
	kalMemZero(&base_lls_stats, sizeof(struct wsvc_stat_lls_report_t));
	kalMemZero(&todo_lls_stats, sizeof(struct wsvc_stat_lls_report_t));
	isMdResetSinceLastQuery = FALSE;
#endif

	return ret;
}

static void mddpUnregisterCb(void)
{
	DBGLOG(INIT, INFO, "mddp_drv_detach\n");
	mddp_drv_detach(&gMddpDrvConf, &gMddpFunc);
	gMddpFunc.wifi_handle = NULL;
}

int32_t mddpGetMdStats(struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct net_device_stats *prStats;
	struct GLUE_INFO *prGlueInfo;
	struct mddpw_net_stat_ext_t mddpNetStats;
	int32_t ret;
	uint8_t i = 0;

	if (!mddpIsSupportMcifWifi() || !mddpIsSupportMddpWh())
		return 0;

	if (!gMddpWFunc.get_net_stat_ext)
		return 0;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);
	prStats = &prNetDevPrivate->stats;
	prGlueInfo = prNetDevPrivate->prGlueInfo;

	if (!prGlueInfo || (prGlueInfo->u4ReadyFlag == 0) ||
			!prGlueInfo->prAdapter)
		return 0;

	if (!prNetDevPrivate->ucMddpSupport)
		return 0;

	ret = gMddpWFunc.get_net_stat_ext(&mddpNetStats);
	if (ret != 0) {
		DBGLOG(INIT, ERROR, "get_net_stat fail, ret: %d.\n", ret);
		return 0;
	}
	for (i = 0; i < NW_IF_NUM_MAX; i++) {
		struct mddpw_net_stat_elem_ext_t *element, *prev;

		element = &mddpNetStats.ifs[0][i];
		prev = &stats.ifs[0][i];
		if (kalStrnCmp(element->nw_if_name, prDev->name,
				IFNAMSIZ) != 0)
			continue;

		prStats->rx_packets +=
			element->rx_packets + prev->rx_packets;
		prStats->tx_packets +=
			element->tx_packets + prev->tx_packets;
		prStats->rx_bytes +=
			element->rx_bytes + prev->rx_bytes;
		prStats->tx_bytes +=
			element->tx_bytes + prev->tx_bytes;
		prStats->rx_errors +=
			element->rx_errors + prev->rx_errors;
		prStats->tx_errors +=
			element->tx_errors + prev->tx_errors;
		prStats->rx_dropped +=
			element->rx_dropped + prev->rx_dropped;
		prStats->tx_dropped +=
			element->tx_dropped + prev->tx_dropped;
	}

	return 0;
}

#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
int32_t mddpGetMdLlsStats(struct ADAPTER *prAdapter)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct GLUE_INFO *prGlueInfo;
	struct STA_RECORD *prStaRec;
	struct net_device *prDev;
	uint8_t i, j, k, l;
	int32_t ret;

	if (!mddpIsSupportMcifWifi() || !mddpIsSupportMddpWh()) {
		DBGLOG(INIT, ERROR, "mddp is not supported.\n");
		return 0;
	}

	if (!prAdapter) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL\n");
		return 0;
	}

	prDev = prAdapter->prGlueInfo->prDevHandler;
	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);
	prGlueInfo = prNetDevPrivate->prGlueInfo;

	if (!prGlueInfo || (prGlueInfo->u4ReadyFlag == 0) ||
			!prGlueInfo->prAdapter) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return 0;
	}

	if (!prNetDevPrivate->ucMddpSupport) {
		DBGLOG(INIT, ERROR,
			"prNetDevPrivate->ucMddpSupport is not supported\n");
		return 0;
	}

	if (!gMddpWFunc.get_lls_stat) {
		DBGLOG(INIT, ERROR,
			"gMddpWFunc.get_lls_stat is not supported\n");
		return 0;
	}

	kalMemZero(&cur_lls_stats, sizeof(struct wsvc_stat_lls_report_t));
	ret = gMddpWFunc.get_lls_stat(&cur_lls_stats);
	if (ret != 0) {
		DBGLOG(INIT, ERROR, "get_lls_stat fail, ret: %d.\n", ret);
		return 0;
	}

	if (cur_lls_stats.version == 0) {
		DBGLOG(INIT, ERROR, "MD is resetting.\n");
		return 0;
	}

	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		for (j = 0; j < STATS_LLS_WIFI_AC_MAX; ++j) {
			prAdapter->aprBssInfo[i]->u4RxMpduAc[j] +=
				isMdResetSinceLastQuery ?
				(cur_lls_stats.wmm_ac_stat_rx_mpdu[i][j] +
				todo_lls_stats.wmm_ac_stat_rx_mpdu[i][j]) :
				(cur_lls_stats.wmm_ac_stat_rx_mpdu[i][j] -
				base_lls_stats.wmm_ac_stat_rx_mpdu[i][j]);
			base_lls_stats.wmm_ac_stat_rx_mpdu[i][j] =
				cur_lls_stats.wmm_ac_stat_rx_mpdu[i][j];
		}
	}

	for (i = 0; i < CFG_STA_REC_NUM; ++i) {
		struct rate_stat_rx_mpdu_t *cur, *base, *todo;

		cur = &cur_lls_stats.rate_stat_rx_mpdu[i];
		base = &base_lls_stats.rate_stat_rx_mpdu[i];
		todo = &todo_lls_stats.rate_stat_rx_mpdu[i];
		prStaRec = &prAdapter->arStaRec[i];

		for (k = 0; k < STATS_LLS_MAX_OFDM_BW_NUM; ++k) {
			for (l = 0; l < STATS_LLS_OFDM_NUM; ++l) {
				prStaRec->u4RxMpduOFDM[0][k][l] +=
					isMdResetSinceLastQuery ?
					(cur->u4RxMpduOFDM[0][k][l] +
					todo->u4RxMpduOFDM[0][k][l]) :
					(cur->u4RxMpduOFDM[0][k][l] -
					base->u4RxMpduOFDM[0][k][l]);
				base->u4RxMpduOFDM[0][k][l] =
					cur->u4RxMpduOFDM[0][k][l];
			}
		}

		for (k = 0; k < STATS_LLS_MAX_CCK_BW_NUM; ++k) {
			for (l = 0; l < STATS_LLS_CCK_NUM; ++l) {
				prStaRec->u4RxMpduCCK[0][k][l] +=
					isMdResetSinceLastQuery ?
					(cur->u4RxMpduCCK[0][k][l] +
					todo->u4RxMpduCCK[0][k][l]) :
					(cur->u4RxMpduCCK[0][k][l] -
					base->u4RxMpduCCK[0][k][l]);
				base->u4RxMpduCCK[0][k][l] =
					cur->u4RxMpduCCK[0][k][l];
			}
		}

		for (k = 0; k < STATS_LLS_MAX_HT_BW_NUM; ++k) {
			for (l = 0; l < STATS_LLS_HT_NUM; ++l) {
				prStaRec->u4RxMpduHT[0][k][l] +=
					isMdResetSinceLastQuery ?
					(cur->u4RxMpduHT[0][k][l] +
					todo->u4RxMpduHT[0][k][l]) :
					(cur->u4RxMpduHT[0][k][l] -
					base->u4RxMpduHT[0][k][l]);
				base->u4RxMpduHT[0][k][l] =
					cur->u4RxMpduHT[0][k][l];
			}
		}

		for (j = 0; j < STATS_LLS_MAX_NSS_NUM; ++j) {
			for (k = 0; k < STATS_LLS_MAX_VHT_BW_NUM; ++k) {
				for (l = 0; l < STATS_LLS_VHT_NUM; ++l) {
					prStaRec->u4RxMpduVHT[j][k][l] +=
						isMdResetSinceLastQuery ?
						(cur->u4RxMpduVHT[j][k][l] +
						todo->u4RxMpduVHT[j][k][l]) :
						(cur->u4RxMpduVHT[j][k][l] -
						base->u4RxMpduVHT[j][k][l]);
					base->u4RxMpduVHT[j][k][l] =
						cur->u4RxMpduVHT[j][k][l];
				}
			}
		}

		for (j = 0; j < STATS_LLS_MAX_NSS_NUM; ++j) {
			for (k = 0; k < STATS_LLS_MAX_HE_BW_NUM; ++k) {
				for (l = 0; l < STATS_LLS_HE_NUM; ++l) {
					prStaRec->u4RxMpduHE[j][k][l] +=
						isMdResetSinceLastQuery ?
						(cur->u4RxMpduHE[j][k][l] +
						todo->u4RxMpduHE[j][k][l]) :
						(cur->u4RxMpduHE[j][k][l] -
						base->u4RxMpduHE[j][k][l]);
					base->u4RxMpduHE[j][k][l] =
						cur->u4RxMpduHE[j][k][l];
				}
			}
		}

		for (j = 0; j < STATS_LLS_MAX_NSS_NUM; ++j) {
			for (k = 0; k < STATS_LLS_MAX_EHT_BW_NUM; ++k) {
				for (l = 0; l < STATS_LLS_EHT_NUM; ++l) {
					prStaRec->u4RxMpduEHT[j][k][l] +=
						isMdResetSinceLastQuery ?
						(cur->u4RxMpduEHT[j][k][l] +
						todo->u4RxMpduEHT[j][k][l]) :
						(cur->u4RxMpduEHT[j][k][l] -
						base->u4RxMpduEHT[j][k][l]);
					base->u4RxMpduEHT[j][k][l] =
						cur->u4RxMpduEHT[j][k][l];
				}
			}
		}
	}

	isMdResetSinceLastQuery = FALSE;
	kalMemZero(&todo_lls_stats,
		sizeof(struct wsvc_stat_lls_report_t));

	return 0;
}

static void save_mddp_lls_stats(void)
{
	uint8_t i, j, k, l;
	int32_t ret;

	if (!mddpIsSupportMcifWifi() || !mddpIsSupportMddpWh()) {
		DBGLOG(INIT, ERROR, "mddp is not supported.\n");
		return;
	}

	if (!gMddpWFunc.get_lls_stat) {
		DBGLOG(INIT, ERROR,
			"gMddpWFunc.get_lls_stat is not supported.\n");
		return;
	}

	kalMemZero(&cur_lls_stats, sizeof(struct wsvc_stat_lls_report_t));
	ret = gMddpWFunc.get_lls_stat(&cur_lls_stats);
	if (ret != 0) {
		DBGLOG(INIT, ERROR, "get_lls_stat fail, ret: %d.\n", ret);
		return;
	}

	if (cur_lls_stats.version == 0) {
		DBGLOG(INIT, ERROR, "MD is resetting.\n");
		return;
	}

	for (i = 0; i < MAX_BSSID_NUM; ++i) {
		for (j = 0; j < STATS_LLS_WIFI_AC_MAX; ++j) {
			todo_lls_stats.wmm_ac_stat_rx_mpdu[i][j] +=
				isMdResetSinceLastQuery ?
				cur_lls_stats.wmm_ac_stat_rx_mpdu[i][j] :
				(cur_lls_stats.wmm_ac_stat_rx_mpdu[i][j] -
				base_lls_stats.wmm_ac_stat_rx_mpdu[i][j]);
		}
	}

	for (i = 0; i < CFG_STA_REC_NUM; ++i) {
		struct rate_stat_rx_mpdu_t *cur, *base, *todo;

		cur = &cur_lls_stats.rate_stat_rx_mpdu[i];
		base = &base_lls_stats.rate_stat_rx_mpdu[i];
		todo = &todo_lls_stats.rate_stat_rx_mpdu[i];

		for (k = 0; k < STATS_LLS_MAX_OFDM_BW_NUM; ++k) {
			for (l = 0; l < STATS_LLS_OFDM_NUM; ++l) {
				todo->u4RxMpduOFDM[0][k][l] +=
					isMdResetSinceLastQuery ?
					cur->u4RxMpduOFDM[0][k][l] :
					(cur->u4RxMpduOFDM[0][k][l] -
					base->u4RxMpduOFDM[0][k][l]);
			}
		}

		for (k = 0; k < STATS_LLS_MAX_CCK_BW_NUM; ++k) {
			for (l = 0; l < STATS_LLS_CCK_NUM; ++l) {
				todo->u4RxMpduCCK[0][k][l] +=
					isMdResetSinceLastQuery ?
					cur->u4RxMpduCCK[0][k][l] :
					(cur->u4RxMpduCCK[0][k][l] -
					base->u4RxMpduCCK[0][k][l]);
			}
		}

		for (k = 0; k < STATS_LLS_MAX_HT_BW_NUM; ++k) {
			for (l = 0; l < STATS_LLS_HT_NUM; ++l) {
				todo->u4RxMpduHT[0][k][l] +=
					isMdResetSinceLastQuery ?
					cur->u4RxMpduHT[0][k][l] :
					(cur->u4RxMpduHT[0][k][l] -
					base->u4RxMpduHT[0][k][l]);
			}
		}

		for (j = 0; j < STATS_LLS_MAX_NSS_NUM; ++j) {
			for (k = 0; k < STATS_LLS_MAX_VHT_BW_NUM; ++k) {
				for (l = 0; l < STATS_LLS_VHT_NUM; ++l) {
					todo->u4RxMpduVHT[j][k][l] +=
						isMdResetSinceLastQuery ?
						cur->u4RxMpduVHT[j][k][l] :
						(cur->u4RxMpduVHT[j][k][l] -
						base->u4RxMpduVHT[j][k][l]);
				}
			}
		}

		for (j = 0; j < STATS_LLS_MAX_NSS_NUM; ++j) {
			for (k = 0; k < STATS_LLS_MAX_HE_BW_NUM; ++k) {
				for (l = 0; l < STATS_LLS_HE_NUM; ++l) {
					todo->u4RxMpduHE[j][k][l] +=
						isMdResetSinceLastQuery ?
						cur->u4RxMpduHE[j][k][l] :
						(cur->u4RxMpduHE[j][k][l] -
						base->u4RxMpduHE[j][k][l]);
				}
			}
		}

		for (j = 0; j < STATS_LLS_MAX_NSS_NUM; ++j) {
			for (k = 0; k < STATS_LLS_MAX_EHT_BW_NUM; ++k) {
				for (l = 0; l < STATS_LLS_EHT_NUM; ++l) {
					todo->u4RxMpduEHT[j][k][l] +=
						isMdResetSinceLastQuery ?
						cur->u4RxMpduEHT[j][k][l] :
						(cur->u4RxMpduEHT[j][k][l] -
						base->u4RxMpduEHT[j][k][l]);
				}
			}
		}
	}

	DBGLOG(INIT, INFO, "save_mddp_lls_stats done.\n");
}
#endif

static bool mddpIsSsnSent(struct ADAPTER *prAdapter,
			  uint8_t *prReorderBuf, uint16_t u2SSN)
{
	uint8_t ucSent = 0;
	uint16_t u2Idx = u2SSN / 8;
	uint8_t ucBit = u2SSN % 8;

	ucSent = (prReorderBuf[u2Idx] & BIT(ucBit)) != 0;
	prReorderBuf[u2Idx] &= ~(BIT(ucBit));

	return ucSent;
}

static void mddpGetRxReorderBuffer(struct ADAPTER *prAdapter,
				   struct SW_RFB *prSwRfb,
				   struct mddpw_md_virtual_buf_t **prMdBuf,
				   struct mddpw_ap_virtual_buf_t **prApBuf)
{
	struct mddpw_md_reorder_sync_table_t *prMdTable = NULL;
	struct mddpw_ap_reorder_sync_table_t *prApTable = NULL;
	uint8_t ucStaRecIdx = prSwRfb->ucStaRecIdx;
	uint8_t ucTid = prSwRfb->ucTid;
	int32_t u4Idx = 0;

	if (gMddpWFunc.get_md_rx_reorder_buf &&
	    gMddpWFunc.get_ap_rx_reorder_buf) {
		if (!gMddpWFunc.get_md_rx_reorder_buf(&prMdTable) &&
		    !gMddpWFunc.get_ap_rx_reorder_buf(&prApTable)) {
			u4Idx = prMdTable->reorder_info[ucStaRecIdx].buf_idx;
			*prMdBuf = &prMdTable->virtual_buf[u4Idx][ucTid];
			*prApBuf = &prApTable->virtual_buf[u4Idx][ucTid];
		}
	}
}

void mddpUpdateReorderQueParm(struct ADAPTER *prAdapter,
			      struct RX_BA_ENTRY *prReorderQueParm,
			      struct SW_RFB *prSwRfb)
{
	struct mddpw_md_virtual_buf_t *prMdBuf = NULL;
	struct mddpw_ap_virtual_buf_t *prApBuf = NULL;
	uint16_t u2SSN = prReorderQueParm->u2WinStart, u2Idx;

	mddpGetRxReorderBuffer(prAdapter, prSwRfb, &prMdBuf, &prApBuf);
	if (!prMdBuf || !prApBuf) {
		DBGLOG(QM, ERROR, "Can't get reorder buffer.\n");
		return;
	}

	for (u2Idx = 0; u2Idx < prReorderQueParm->u2WinSize; u2Idx++) {
		if (prReorderQueParm->u2WinStart == prSwRfb->u2SSN ||
		    !mddpIsSsnSent(prAdapter, prMdBuf->virtual_buf, u2SSN))
			break;

		prReorderQueParm->u2WinStart = u2SSN % MAX_SEQ_NO_COUNT;
		prReorderQueParm->u2WinEnd =
			SEQ_ADD(prReorderQueParm->u2WinStart,
				prReorderQueParm->u2WinSize - 1);
		SEQ_INC(u2SSN);
		DBGLOG(QM, TRACE,
			"Update reorder window: SSN: %d, start: %d, end: %d.\n",
			u2SSN,
			prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinEnd);
	}

	prApBuf->start_idx = prReorderQueParm->u2WinStart;
	prApBuf->end_idx = prReorderQueParm->u2WinEnd;
}

int32_t mddpNotifyDrvTxd(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t fgActivate)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	struct mddp_txd_t *prMddpTxd;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	struct net_device *prNetdev;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	uint32_t u32BufSize = 0;
	uint8_t *buff = NULL;
	int32_t ret = 0;

	if (!gMddpWFunc.notify_drv_info) {
		DBGLOG(NIC, ERROR, "notify_drv_info callback NOT exist.\n");
		ret = -1;
		goto exit;
	}
	if (!prStaRec) {
		DBGLOG(NIC, ERROR, "sta NOT valid\n");
		ret = -1;
		goto exit;
	}
	if (prStaRec->ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(NIC, ERROR, "sta bssid NOT valid: %d.\n",
				prStaRec->ucBssIndex);
		ret = -1;
		goto exit;
	}
	if (fgActivate && !prStaRec->aprTxDescTemplate[0]) {
		DBGLOG(NIC, INFO,
			"sta[%d]'s TXD NOT generated done, maybe wait.\n",
			prStaRec->ucBssIndex);
		ret = -1;
		goto exit;
	}

	prBssInfo = prAdapter->aprBssInfo[prStaRec->ucBssIndex];
	prNetdev = wlanGetNetDev(prAdapter->prGlueInfo, prStaRec->ucBssIndex);
	if (prNetdev) {
		prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prNetdev);
		if (!prNetDevPrivate->ucMddpSupport) {
			DBGLOG(NIC, ERROR, "mddp not support\n");
			goto exit;
		}
	} else {
		DBGLOG(NIC, INFO, "NetDev is null BssIndex[%d]\n",
		       prStaRec->ucBssIndex);
	}

	u32BufSize = (sizeof(struct mddpw_drv_notify_info_t) +
			sizeof(struct mddpw_drv_info_t) +
			sizeof(struct mddp_txd_t) +
			NIC_TX_DESC_LONG_FORMAT_LENGTH);
	buff = kalMemAlloc(u32BufSize, VIR_MEM_TYPE);

	if (buff == NULL) {
		DBGLOG(NIC, ERROR, "buffer allocation failed.\n");
		ret = -1;
		goto exit;
	}
	prNotifyInfo = (struct mddpw_drv_notify_info_t *) buff;
	prNotifyInfo->version = 0;
	prNotifyInfo->buf_len = sizeof(struct mddpw_drv_info_t) +
			sizeof(struct mddp_txd_t) +
			NIC_TX_DESC_LONG_FORMAT_LENGTH;
	prNotifyInfo->info_num = 1;
	prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
	prDrvInfo->info_id = 3; /* MDDPW_DRV_INFO_TXD; */
	prDrvInfo->info_len = (sizeof(struct mddpw_txd_t) +
			NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prMddpTxd = (struct mddp_txd_t *) &(prDrvInfo->info[0]);
	prMddpTxd->version = 1;
	prMddpTxd->sta_idx = prStaRec->ucIndex;
	prMddpTxd->wlan_idx = prStaRec->ucWlanIndex;
	prMddpTxd->sta_mode = prStaRec->eStaType;
	prMddpTxd->bss_id = prStaRec->ucBssIndex;
	/* TODO: Create a new msg for DMASHDL BMP */
	prMddpTxd->wmmset = prBssInfo->ucWmmQueSet % 2;
	if (prNetdev) {
		kalMemCopy(prMddpTxd->nw_if_name, prNetdev->name,
			   sizeof(prMddpTxd->nw_if_name));
	} else {
		kalMemZero(prMddpTxd->nw_if_name,
			   sizeof(prMddpTxd->nw_if_name));
	}
	kalMemCopy(prMddpTxd->aucMacAddr, prStaRec->aucMacAddr, MAC_ADDR_LEN);
	kalMemCopy(prMddpTxd->local_mac,
		   prBssInfo->aucOwnMacAddr, MAC_ADDR_LEN);
	if (fgActivate) {
		prMddpTxd->txd_length = NIC_TX_DESC_LONG_FORMAT_LENGTH;
		kalMemCopy(prMddpTxd->txd, prStaRec->aprTxDescTemplate[0],
				prMddpTxd->txd_length);
	} else {
		prMddpTxd->txd_length = 0;
	}

	ret = gMddpWFunc.notify_drv_info(prNotifyInfo);

#define TEMP_LOG_TEMPLATE "ver:%d,idx:%d,w_idx:%d,mod:%d,bss:%d,wmm:%d," \
		"name:%s,act:%d,ret:%d"
	DBGLOG(NIC, INFO, TEMP_LOG_TEMPLATE,
		prMddpTxd->version,
		prMddpTxd->sta_idx,
		prMddpTxd->wlan_idx,
		prMddpTxd->sta_mode,
		prMddpTxd->bss_id,
		prMddpTxd->wmmset,
		prMddpTxd->nw_if_name,
		fgActivate,
		ret);
#undef TEMP_LOG_TEMPLATE

exit:
	if (buff)
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);
	return ret;
}

int32_t mddpNotifyWifiStatus(enum ENUM_MDDPW_DRV_INFO_STATUS status)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	uint32_t u32BufSize = 0;
	uint8_t *buff = NULL;
	int32_t ret = 0, feature = 0;

	if (gMddpWFunc.get_mddp_feature)
		feature = gMddpWFunc.get_mddp_feature();

	if (gMddpWFunc.notify_drv_info) {
		int32_t ret;

		u32BufSize = (sizeof(struct mddpw_drv_notify_info_t) +
			sizeof(struct mddpw_drv_info_t) + sizeof(bool));
		buff = kalMemAlloc(u32BufSize, VIR_MEM_TYPE);

		if (buff == NULL) {
			DBGLOG(NIC, ERROR, "Can't allocate buffer.\n");
			return -1;
		}
		prNotifyInfo = (struct mddpw_drv_notify_info_t *) buff;
		prNotifyInfo->version = 0;
		prNotifyInfo->buf_len = sizeof(struct mddpw_drv_info_t) +
				sizeof(bool);
		prNotifyInfo->info_num = 1;
		prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
		prDrvInfo->info_id = MDDPW_DRV_INFO_NOTIFY_WIFI_ONOFF;
		prDrvInfo->info_len = WIFI_ONOFF_NOTIFICATION_LEN;
		prDrvInfo->info[0] = status;

		ret = gMddpWFunc.notify_drv_info(prNotifyInfo);
		DBGLOG(INIT, INFO, "power: %d, ret: %d, feature:%d.\n",
		       status, ret, feature);
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);
		g_eMddpStatus = status;
	} else {
		DBGLOG(INIT, ERROR, "notify_drv_info is NULL.\n");
		ret = -1;
	}

	return ret;
}

static bool mddpIsCasanFWload(void)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	bool ret = FALSE;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		goto exit;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		goto exit;
	}

	if (prAdapter->u4CasanLoadType == 1)
		ret = TRUE;

exit:
	return ret;
}

int32_t mddpNotifyDrvOwnTimeoutTime(void)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	int32_t ret = 0;
	uint32_t u32BufSize = 0;
	uint32_t u32DrvOwnTimeoutTime = LP_OWN_BACK_TOTAL_DELAY_MD_MS;
	uint8_t *buff = NULL;

	DBGLOG(INIT, INFO, "Wi-Fi Notify MD Drv Own Timeout time.\n");

	if (!gMddpWFunc.notify_drv_info) {
		DBGLOG(NIC, ERROR, "notify_drv_info callback NOT exist.\n");
		ret = -1;
		goto exit;
	}

	/* align AP/MD drv own timeout */
	if (mddpIsCasanFWload() == TRUE)
		u32DrvOwnTimeoutTime = LP_OWN_BACK_TOTAL_DELAY_CASAN_MS;
	else
		goto exit;

	u32BufSize = (sizeof(struct mddpw_drv_notify_info_t) +
			sizeof(struct mddpw_drv_info_t) + sizeof(uint32_t));

	buff = kalMemAlloc(u32BufSize, VIR_MEM_TYPE);

	if (buff == NULL) {
		DBGLOG(NIC, ERROR, "buffer allocation failed.\n");
		ret = -ENODEV;
		goto exit;
	}

	prNotifyInfo = (struct mddpw_drv_notify_info_t *) buff;
	prNotifyInfo->version = 0;
	prNotifyInfo->buf_len = sizeof(struct mddpw_drv_info_t) +
			sizeof(uint32_t);
	prNotifyInfo->info_num = 1;
	prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
	prDrvInfo->info_id = 5; /* WSVC_DRVINFO_DRVOWN_TIME_SET */
	prDrvInfo->info_len = sizeof(uint32_t);

	kalMemCopy((uint32_t *) &(prDrvInfo->info[0]), &u32DrvOwnTimeoutTime,
			sizeof(uint32_t));

	ret = gMddpWFunc.notify_drv_info(prNotifyInfo);

exit:
	if (buff)
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);

	DBGLOG(INIT, INFO, "ret: %d, timeout: %d.\n",
				   ret, u32DrvOwnTimeoutTime);
	return ret;
}

#if defined(_HIF_PCIE)
#if CFG_SUPPORT_PCIE_ASPM
int32_t mddpNotifyMDPCIeL12Status(uint32_t u32Enable)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	int32_t ret = 0;
	uint32_t u32BufSize = 0;
	uint32_t u32InfoId = 7; /* TODO: use define */
	uint8_t *buff = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;

	DBGLOG(INIT, TRACE, "Notify PCIe L1.2 Status %lu\n", u32Enable);

	if (!gMddpWFunc.notify_drv_info) {
		DBGLOG(NIC, ERROR, "notify_drv_info callback NOT exist.\n");
		ret = -1;
		goto exit;
	}

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		goto exit;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		goto exit;
	}

	u32BufSize = (sizeof(struct mddpw_drv_notify_info_t) +
			sizeof(struct mddpw_drv_info_t) + sizeof(uint32_t));

	buff = kalMemAlloc(u32BufSize, VIR_MEM_TYPE);

	if (buff == NULL) {
		DBGLOG(NIC, ERROR, "buffer allocation failed.\n");
		ret = -ENODEV;
		goto exit;
	}

	if (!u32Enable) {
		u32InfoId = 7; /* Disable L1ss */
		GLUE_INC_REF_CNT(prAdapter->u4MddpPCIeL12SeqNum);
	} else
		u32InfoId = 8; /* Enable L1ss */

	prNotifyInfo = (struct mddpw_drv_notify_info_t *) buff;
	prNotifyInfo->version = 0;
	prNotifyInfo->buf_len = sizeof(struct mddpw_drv_info_t) +
			sizeof(uint32_t);
	prNotifyInfo->info_num = 1;
	prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
	prDrvInfo->info_id = u32InfoId;
	prDrvInfo->info_len = sizeof(uint32_t);

	kalMemCopy((uint32_t *) &(prDrvInfo->info[0]),
			&prAdapter->u4MddpPCIeL12SeqNum,
			sizeof(uint32_t));

	ret = gMddpWFunc.notify_drv_info(prNotifyInfo);

exit:
	if (buff)
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);

	DBGLOG(INIT, TRACE, "ret: %d, info_id: %lu, u32SeqNum:%lu.\n",
		ret, u32InfoId, prAdapter->u4MddpPCIeL12SeqNum);
	return ret;
}
#endif
#endif


#ifdef CFG_SUPPORT_UNIFIED_COMMAND
int32_t mddpNotifyMDUnifiedCmdVer(void)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	int32_t ret = 0;
	uint32_t u32BufSize = 0;
	uint32_t u32UnifiedCmdVer = 1;
	uint8_t *buff = NULL;

	DBGLOG(INIT, INFO, "Notify MD Unified Cmd version.\n");

	if (!gMddpWFunc.notify_drv_info) {
		DBGLOG(NIC, ERROR, "notify_drv_info callback NOT exist.\n");
		ret = -1;
		goto exit;
	}

	u32BufSize = (sizeof(struct mddpw_drv_notify_info_t) +
			sizeof(struct mddpw_drv_info_t) + sizeof(uint32_t));

	buff = kalMemAlloc(u32BufSize, VIR_MEM_TYPE);

	if (buff == NULL) {
		DBGLOG(NIC, ERROR, "buffer allocation failed.\n");
		ret = -ENODEV;
		goto exit;
	}

	prNotifyInfo = (struct mddpw_drv_notify_info_t *) buff;
	prNotifyInfo->version = 0;
	prNotifyInfo->buf_len = sizeof(struct mddpw_drv_info_t) +
			sizeof(uint32_t);
	prNotifyInfo->info_num = 1;
	prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
	prDrvInfo->info_id = 6;
	prDrvInfo->info_len = sizeof(uint32_t);

	kalMemCopy((uint32_t *) &(prDrvInfo->info[0]), &u32UnifiedCmdVer,
			sizeof(uint32_t));

	ret = gMddpWFunc.notify_drv_info(prNotifyInfo);

exit:
	if (buff)
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);

	DBGLOG(INIT, INFO, "ret: %d, Ver: %u.\n",
				   ret, u32UnifiedCmdVer);
	return ret;
}
#endif

void __mddpNotifyWifiOnStart(void)
{
#if CFG_MTK_CCCI_SUPPORT
	mtk_ccci_register_md_state_cb(&mddpMdStateChangedCb);
#endif

	mddpNotifyWifiStatus(MDDPW_DRV_INFO_STATUS_ON_START);
}

void mddpNotifyWifiOnStart(void)
{
	if (!mddpIsSupportMcifWifi())
		return;

	if (!is_cal_flow_finished())
		return;

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (is_pwr_on_notify_processing())
		return;
#endif
#endif

	mutex_lock(&rMddpLock);
	__mddpNotifyWifiOnStart();
	mutex_unlock(&rMddpLock);
}

int32_t __mddpNotifyWifiOnEnd(void)
{
	int32_t ret = 0;

	if (g_eMddpStatus != MDDPW_DRV_INFO_STATUS_ON_START) {
		DBGLOG(NIC, ERROR, "mddp status mismatch[%u]\n", g_eMddpStatus);
		return ret;
	}

	/* Notify Driver own timeout time before Wi-Fi on end */
	mddpNotifyDrvOwnTimeoutTime();

	/* Notify MD Unified Cmd version */
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	mddpNotifyMDUnifiedCmdVer();
#endif

	if (g_rSettings.rOps.set)
		g_rSettings.rOps.set(&g_rSettings, g_rSettings.u4WifiOnBit);

	if (g_rSettings.u4MDDPSupportMode == MDDP_SUPPORT_AOP) {
		if (g_rSettings.rOps.clr)
			g_rSettings.rOps.clr(&g_rSettings,
				g_rSettings.u4MdOnBit);
	}

#if (CFG_SUPPORT_CONNAC2X == 0 && CFG_SUPPORT_CONNAC3X == 0)
	ret = mddpNotifyWifiStatus(MDDPW_DRV_INFO_STATUS_ON_END);
#else
	ret = mddpNotifyWifiStatus(MDDPW_DRV_INFO_STATUS_ON_END_QOS);
#endif
	if (ret == 0)
		ret = wait_for_md_on_complete() ?
				WLAN_STATUS_SUCCESS :
				WLAN_STATUS_FAILURE;
	return ret;
}

int32_t mddpNotifyWifiOnEnd(void)
{
	int32_t ret = 0;

	if (!mddpIsSupportMcifWifi())
		return ret;

	if (!is_cal_flow_finished())
		return ret;

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (is_pwr_on_notify_processing())
		return ret;
#endif
#endif

	mutex_lock(&rMddpLock);
	ret = __mddpNotifyWifiOnEnd();
	mutex_unlock(&rMddpLock);

	return ret;
}

void __mddpNotifyWifiOffStart(void)
{
	int32_t ret;

#if CFG_MTK_CCCI_SUPPORT
	mtk_ccci_register_md_state_cb(NULL);
#endif

	DBGLOG(INIT, INFO, "md off start.\n");
	if (g_rSettings.u4MDDPSupportMode == MDDP_SUPPORT_AOP) {
		if (g_rSettings.rOps.set)
			g_rSettings.rOps.set(&g_rSettings,
				g_rSettings.u4MdOffBit);
	}

	ret = mddpNotifyWifiStatus(MDDPW_DRV_INFO_STATUS_OFF_START);
	if (ret == 0)
		wait_for_md_off_complete();

	mddpSetMDFwOwn();
}

void mddpNotifyWifiOffStart(void)
{
#if defined(_HIF_PCIE)
	struct GLUE_INFO *prGlueInfo = NULL;
#endif

	if (!mddpIsSupportMcifWifi())
		return;

	if (!is_cal_flow_finished())
		return;

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (is_pwr_on_notify_processing())
		return;
#endif
#endif

	mutex_lock(&rMddpLock);
	__mddpNotifyWifiOffStart();
	mutex_unlock(&rMddpLock);

#if defined(_HIF_PCIE)
	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return;
	}

	/* avoid power off process MD SER */
	kalSetMdCrashEvent(prGlueInfo);
#endif
}

void __mddpNotifyWifiOffEnd(void)
{
	int32_t u4ClrBits = 0;

	if (g_eMddpStatus != MDDPW_DRV_INFO_STATUS_OFF_START) {
		DBGLOG(NIC, ERROR, "mddp status mismatch[%u]\n", g_eMddpStatus);
		return;
	}

	if (g_rSettings.u4MDDPSupportMode == MDDP_SUPPORT_SHM)
		u4ClrBits = g_rSettings.u4WifiOnBit;
	else
		u4ClrBits = g_rSettings.u4WifiOnBit | g_rSettings.u4MdInitBit;

	if (g_rSettings.rOps.clr) {
		g_rSettings.rOps.clr(
			&g_rSettings,
			u4ClrBits);
	}

	mddpNotifyWifiStatus(MDDPW_DRV_INFO_STATUS_OFF_END);
}

void mddpNotifyWifiOffEnd(void)
{
	if (!mddpIsSupportMcifWifi())
		return;

	if (!is_cal_flow_finished())
		return;

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (is_pwr_on_notify_processing())
		return;
#endif
#endif

	mutex_lock(&rMddpLock);
	__mddpNotifyWifiOffEnd();
	mutex_unlock(&rMddpLock);
}

void mddpNotifyWifiReset(void)
{
	if (!mddpIsSupportMcifWifi())
		return;

	if (g_rSettings.rOps.clr)
		g_rSettings.rOps.clr(&g_rSettings, g_rSettings.u4WifiOnBit);
}

int32_t mddpMdNotifyInfo(struct mddpw_md_notify_info_t *prMdInfo)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	int32_t ret = 0;
	u_int8_t fgHalted = kalIsHalted();
	struct BUS_INFO *prBusInfo = NULL;

	DBGLOG(INIT, TRACE, "MD notify mddpMdNotifyInfo.\n");

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		ret = -ENODEV;
		goto exit;
	}
	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		ret = -ENODEV;
		goto exit;
	}

	if (fgHalted || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(INIT, INFO,
			"Skip update info. to MD, fgHalted: %d, u4ReadyFlag: %d\n",
			fgHalted, prGlueInfo->u4ReadyFlag);
		ret = -ENODEV;
		goto exit;
	}

	prBusInfo = prAdapter->chip_info->bus_info;

	if (!mddpIsSupportMcifWifi()) {
		DBGLOG(INIT, ERROR, "mcif wifi not support.\n");
		ret = -ENODEV;
		goto exit;
	}

	if (!is_cal_flow_finished()) {
		DBGLOG(INIT, ERROR, "cal flow not finished.\n");
		ret = -ENODEV;
		goto exit;
	}

#if CFG_MTK_ANDROID_WMT
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (is_pwr_on_notify_processing()) {
		DBGLOG(INIT, ERROR, "is_pwr_on_notify_processing.\n");
		ret = -ENODEV;
		goto exit;
	}
#endif
#endif

	if (prMdInfo->info_type == MDDPW_MD_INFO_RESET_IND) {
		uint32_t i;
		struct BSS_INFO *prSapBssInfo = (struct BSS_INFO *) NULL;
		struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
		int32_t ret;

		DBGLOG(INIT, INFO, "MD resetting.\n");
		save_mddp_stats();
#if CFG_SUPPORT_LLS && CFG_SUPPORT_LLS_MDDP
		save_mddp_lls_stats();
		isMdResetSinceLastQuery = TRUE;
#endif
		mutex_lock(&rMddpLock);
		__mddpNotifyWifiOnStart();
		ret = __mddpNotifyWifiOnEnd();
		mutex_unlock(&rMddpLock);
		if (ret != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, INFO, "mddpNotifyWifiOnEnd failed.\n");
			return 0;
		}

		/* Notify STA's TXD to MD */
		for (i = 0; i < KAL_AIS_NUM; i++) {
 			struct BSS_INFO *prAisBssInfo =	aisGetMainLinkBssInfo(
					aisFsmGetInstance(prAdapter, i));

			if (prAisBssInfo && prAisBssInfo->eConnectionState ==
					MEDIA_STATE_CONNECTED)
				mddpNotifyDrvTxd(prAdapter,
						prAisBssInfo->prStaRecOfAP,
						TRUE);
		}
		/* Notify SAP clients' TXD to MD */
		prP2pBssInfo = cnmGetSapBssInfo(prAdapter);
		if (prP2pBssInfo) {
			struct LINK *prClientList;
			struct STA_RECORD *prCurrStaRec;

			prClientList = &prP2pBssInfo->rStaRecOfClientList;
			LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList,
					rLinkEntry, struct STA_RECORD) {
				if (!prCurrStaRec)
					break;
				mddpNotifyDrvTxd(prAdapter,
						prCurrStaRec,
						TRUE);
			}

			prSapBssInfo = cnmGetOtherSapBssInfo(prAdapter,
					prP2pBssInfo);
			if (prSapBssInfo) {
				struct LINK *prClientList;
				struct STA_RECORD *prCurrStaRec;

				prClientList =
					&prSapBssInfo->rStaRecOfClientList;
				LINK_FOR_EACH_ENTRY(prCurrStaRec, prClientList,
						rLinkEntry, struct STA_RECORD) {
					if (!prCurrStaRec)
						break;
					mddpNotifyDrvTxd(prAdapter,
							prCurrStaRec,
							TRUE);
				}
			}
		}
	} else if (prMdInfo->info_type == MDDPW_MD_INFO_DRV_EXCEPTION) {
		struct wsvc_md_event_exception_t *event;

		if (prMdInfo->buf_len != sizeof(
				struct wsvc_md_event_exception_t)) {
			DBGLOG(INIT, ERROR,
				"Invalid args from MD, expect %u but %u\n",
				sizeof(struct wsvc_md_event_exception_t),
				prMdInfo->buf_len);
			ret = -EINVAL;
			goto exit;
		}
		event = (struct wsvc_md_event_exception_t *)
				&(prMdInfo->buf[1]);
		DBGLOG(INIT, WARN, "reason: %d, flag: %d, line: %d, func: %s\n",
				event->u4RstReason,
				event->u4RstFlag,
				event->u4Line,
				event->pucFuncName);
		glSetRstReason(RST_MDDP_MD_TRIGGER_EXCEPTION);
		GL_USER_DEFINE_RESET_TRIGGER(prAdapter,
			RST_MDDP_MD_TRIGGER_EXCEPTION,
			event->u4RstFlag | RST_FLAG_DO_CORE_DUMP);
	} else if (prMdInfo->info_type == MDDPW_MD_EVENT_COMMUNICATION) {
		struct wsvc_md_event_comm_t *event;

		if (prMdInfo->buf_len != sizeof(
				struct wsvc_md_event_comm_t)) {
			DBGLOG(INIT, ERROR,
				"Invalid args from MD, expect %u but %u\n",
				sizeof(struct wsvc_md_event_comm_t),
				prMdInfo->buf_len);
			ret = -EINVAL;
			goto exit;
		}
		event = (struct wsvc_md_event_comm_t *)
				&(prMdInfo->buf[1]);
		if (event->u4Reason == MD_L12_DISABLE ||
			event->u4Reason == MD_L12_ENABLE) {
			DBGLOG_LIMITED(INIT, WARN,
				"reason:%d, flag:%d, line:%d, func:%s, bssIdx:%d\n",
					event->u4Reason,
					event->u4RstFlag,
					event->u4Line,
					event->pucFuncName,
					event->dump_payload[1]);
		} else {
			DBGLOG(INIT, WARN,
				"reason:%d, flag:%d, line:%d, func:%s, bssIdx:%d\n",
					event->u4Reason,
					event->u4RstFlag,
					event->u4Line,
					event->pucFuncName,
					event->dump_payload[1]);
		}
		if (event->u4Reason == MD_TX_DATA_HANG) {
			prAdapter->u4HifChkFlag |= HIF_CHK_TX_HANG;
			prAdapter->u4HifChkFlag |= HIF_CHK_MD_TX_HANG;
			prAdapter->ucMddpBssIndex =
				(uint8_t) event->dump_payload[1];
			kalSetHifDbgEvent(prAdapter->prGlueInfo);
#if defined(_HIF_PCIE)
#if CFG_SUPPORT_PCIE_ASPM
		} else if (event->u4Reason == MD_L12_DISABLE) {
			/* disable PCIe L1.2, 2 means md config */
			if (prBusInfo->configPcieAspm) {
				prBusInfo->configPcieAspm(prGlueInfo, FALSE, 2);
				mddpNotifyMDPCIeL12Status(0);
			}
		} else if (event->u4Reason == MD_L12_ENABLE) {
			/* enable PCIe L1.2, 2 means md config */
			if (prBusInfo->configPcieAspm) {
				prBusInfo->configPcieAspm(prGlueInfo, TRUE, 2);
				mddpNotifyMDPCIeL12Status(1);
			}
#endif
#endif
		} else if (event->u4Reason == MD_INFORMATION_DUMP ||
				event->u4Reason == MD_DRV_OWN_FAIL ||
				event->u4Reason == MD_INIT_FAIL ||
				event->u4Reason == MD_STATE_ABNORMAL ||
				event->u4Reason == MD_TX_CMD_FAIL) {
			glSetRstReason(RST_MDDP_MD_TRIGGER_EXCEPTION);
			GL_USER_DEFINE_RESET_TRIGGER(prAdapter,
				RST_MDDP_MD_TRIGGER_EXCEPTION,
				event->u4RstFlag | RST_FLAG_DO_CORE_DUMP);
		} else {
			DBGLOG(INIT, WARN,
				"MD event reason undefined reason:%d\n",
					event->u4Reason);
		}
	} else {
		DBGLOG(INIT, ERROR, "unknown MD info type: %d\n",
			prMdInfo->info_type);
		ret = -ENODEV;
		goto exit;
	}

exit:
	return ret;
}

int32_t mddpChangeState(enum mddp_state_e event, void *buf, uint32_t *buf_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	u_int8_t fgHalted = kalIsHalted();

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return 0;
	}

	if (fgHalted || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(INIT, ERROR, "fgHalted: %d, u4ReadyFlag: %d\n",
				fgHalted, prGlueInfo->u4ReadyFlag);
		return 0;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		return 0;
	}

	switch (event) {
	case MDDP_STATE_ENABLING:
		break;

	case MDDP_STATE_ACTIVATING:
		break;

	case MDDP_STATE_ACTIVATED:
		DBGLOG(INIT, INFO, "Mddp activated.\n");
		prAdapter->fgMddpActivated = true;
		break;

	case MDDP_STATE_DEACTIVATING:
		break;

	case MDDP_STATE_DEACTIVATED:
		DBGLOG(INIT, INFO, "Mddp deactivated.\n");
		prAdapter->fgMddpActivated = false;
		break;

	case MDDP_STATE_DISABLING:
	case MDDP_STATE_UNINIT:
	case MDDP_STATE_CNT:
	case MDDP_STATE_DUMMY:
	default:
		break;
	}

	return 0;

}

static bool wait_for_md_off_complete(void)
{
	uint32_t u4Value = 0;
	uint32_t u4StartTime, u4CurTime;
	bool fgTimeout = false;
	uint32_t u4MDOffTimeoutTime = MD_ON_OFF_TIMEOUT;

	if (mddpIsCasanFWload() == TRUE)
		u4MDOffTimeoutTime = MD_ON_OFF_TIMEOUT_CASAN;

	u4StartTime = kalGetTimeTick();

	do {
		if (g_rSettings.rOps.rd)
			g_rSettings.rOps.rd(&g_rSettings, &u4Value);

		if ((u4Value & g_rSettings.u4MdOffBit) == 0) {
			DBGLOG(INIT, INFO, "md off end.\n");
			break;
		}

		u4CurTime = kalGetTimeTick();
		if (CHECK_FOR_TIMEOUT(u4CurTime, u4StartTime,
				u4MDOffTimeoutTime)) {
			DBGLOG(INIT, ERROR, "wait for md off timeout\n");
			fgTimeout = true;
			break;
		}

		kalMsleep(CFG_RESPONSE_POLLING_DELAY);
	} while (TRUE);

	return !fgTimeout;
}

static bool wait_for_md_on_complete(void)
{
	uint32_t u4Value = 0;
	uint32_t u4StartTime, u4CurTime;
	bool fgCompletion = false;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4MDOnTimeoutTime = MD_ON_OFF_TIMEOUT;

	u4StartTime = kalGetTimeTick();
	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return false;
	}
	if (mddpIsCasanFWload() == TRUE)
		u4MDOnTimeoutTime = MD_ON_OFF_TIMEOUT_CASAN;

	do {
		if (g_rSettings.rOps.rd)
			g_rSettings.rOps.rd(&g_rSettings, &u4Value);

		if ((u4Value & g_rSettings.u4MdOnBit) > 0) {
			DBGLOG(INIT, INFO, "md on end.\n");
			fgCompletion = true;
			break;
		} else if (!prGlueInfo->u4ReadyFlag) {
			DBGLOG(INIT, WARN, "Skip waiting due to ready flag.\n");
			fgCompletion = false;
			break;
		}

		u4CurTime = kalGetTimeTick();
		if (CHECK_FOR_TIMEOUT(u4CurTime, u4StartTime,
				u4MDOnTimeoutTime)) {
			DBGLOG(INIT, ERROR, "wait for md on timeout\n");
			fgCompletion = false;
			break;
		}

		kalMsleep(CFG_RESPONSE_POLLING_DELAY);
	} while (TRUE);

	return fgCompletion;
}

void setMddpSupportRegister(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
#if (CFG_SUPPORT_CONNAC2X == 0 && CFG_SUPPORT_CONNAC3X == 0)
	uint32_t u4Val = 0;
#endif

	prChipInfo = prAdapter->chip_info;


	if (prChipInfo->isSupportMddpSHM) {
		g_rSettings.rOps.rd = mddpRdFuncSHM;
		g_rSettings.rOps.set = mddpSetFuncSHM;
		g_rSettings.rOps.clr = mddpClrFuncSHM;
		g_rSettings.u4MdInitBit = MD_SHM_MD_INIT_BIT;
		g_rSettings.u4MdOnBit = MD_SHM_MD_ON_BIT;
		g_rSettings.u4MdOffBit = MD_SHM_MD_OFF_BIT;
		g_rSettings.u4WifiOnBit = MD_SHM_WIFI_ON_BIT;
		g_rSettings.u4MDDPSupportMode = MDDP_SUPPORT_SHM;
	} else if (prChipInfo->isSupportMddpAOR) {
		g_rSettings.rOps.rd = mddpRdFunc;
		g_rSettings.rOps.set = mddpSetFuncV2;
		g_rSettings.rOps.clr = mddpClrFuncV2;
		g_rSettings.u4SyncAddr = MD_AOR_RD_CR_ADDR;
		g_rSettings.u4SyncSetAddr = MD_AOR_SET_CR_ADDR;
		g_rSettings.u4SyncClrAddr = MD_AOR_CLR_CR_ADDR;
		g_rSettings.u4MdInitBit = MD_AOR_MD_INIT_BIT;
		g_rSettings.u4MdOnBit = MD_AOR_MD_ON_BIT;
		g_rSettings.u4MdOffBit = MD_AOR_MD_OFF_BIT;
		g_rSettings.u4WifiOnBit = MD_AOR_WIFI_ON_BIT;
		g_rSettings.u4MDDPSupportMode = MDDP_SUPPORT_AOP;
	} else {
		g_rSettings.rOps.rd = mddpRdFunc;
		g_rSettings.rOps.set = mddpSetFuncV1;
		g_rSettings.rOps.clr = mddpClrFuncV1;
		g_rSettings.u4SyncAddr = MD_STATUS_SYNC_CR;
		g_rSettings.u4MdOnBit = MD_STATUS_ON_SYNC_BIT;
		g_rSettings.u4MdOffBit = MD_STATUS_OFF_SYNC_BIT;
		g_rSettings.u4MDDPSupportMode = MDDP_SUPPORT_AOP;
	}

#if (CFG_SUPPORT_CONNAC2X == 0 && CFG_SUPPORT_CONNAC3X == 0)
	HAL_MCR_RD(prAdapter, MDDP_SUPPORT_CR, &u4Val);
	if (g_fgMddpEnabled)
		u4Val |= MDDP_SUPPORT_CR_BIT;
	else
		u4Val &= ~MDDP_SUPPORT_CR_BIT;
	HAL_MCR_WR(prAdapter, MDDP_SUPPORT_CR, u4Val);
#endif
}

void mddpInit(void)
{
	struct device_node *np_chosen;
	struct tag_bootmode *tag = NULL;

	np_chosen = of_find_node_by_path("/chosen");
	if (!np_chosen)
		np_chosen = of_find_node_by_path("/chosen@0");

	if (!np_chosen)
		return;

	tag = (struct tag_bootmode *) of_get_property(np_chosen, "atag,boot",
			NULL);

	if (!tag)
		return;

	DBGLOG(INIT, INFO, "bootmode: 0x%x\n", tag->bootmode);
	g_wifi_boot_mode = tag->bootmode;

	g_eMddpStatus = MDDPW_DRV_INFO_STATUS_OFF_END;
	mutex_init(&rMddpLock);
	mddpRegisterCb();
}

void mddpUninit(void)
{
	g_fgMddpEnabled = FALSE;
	mddpUnregisterCb();
}

static void notifyMdCrash2FW(void)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(INIT, ERROR, "Invalid drv state.\n");
		return;
	}

	if (g_rSettings.u4MDDPSupportMode == MDDP_SUPPORT_SHM) {
		if (g_rSettings.rOps.clr)
			g_rSettings.rOps.clr(&g_rSettings,
				g_rSettings.u4MdOnBit);
	}

	/*
	 * Set MD FW Own before Notify FW MD crash
	 * Reason: MD cannt set FW own itself when MD crash
	 */
	mddpSetMDFwOwn();

	kalSetMdCrashEvent(prGlueInfo);
}

#if CFG_MTK_CCCI_SUPPORT
void  mddpMdStateChangedCb(enum MD_STATE old_state,
		enum MD_STATE new_state)
{
	DBGLOG(INIT, TRACE, "old_state: %d, new_state: %d.\n",
			old_state, new_state);

	switch (new_state) {
	case GATED: /* MD off */
	case EXCEPTION: /* MD crash */
		notifyMdCrash2FW();
		break;
	default:
		break;
	}
}
#endif

static void save_mddp_stats(void)
{
	struct mddpw_net_stat_ext_t temp;
	uint8_t i = 0;
	int32_t ret;

	if (!gMddpWFunc.get_net_stat_ext)
		return;

	ret = gMddpWFunc.get_net_stat_ext(&temp);
	if (ret != 0) {
		DBGLOG(INIT, ERROR, "get_net_stat fail, ret: %d.\n", ret);
		return;
	}

	for (i = 0; i < NW_IF_NUM_MAX; i++) {
		struct mddpw_net_stat_elem_ext_t *element, *curr;

		element = &temp.ifs[0][i];
		curr = &stats.ifs[0][i];

		curr->rx_packets += element->rx_packets;
		curr->tx_packets += element->tx_packets;
		curr->rx_bytes += element->rx_bytes;
		curr->tx_bytes += element->tx_bytes;
		curr->rx_errors += element->rx_errors;
		curr->tx_errors += element->tx_errors;
		curr->rx_dropped += element->rx_dropped;
		curr->tx_dropped += element->tx_dropped;
	}
}

void mddpSetMDFwOwn(void)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(INIT, ERROR, "Invalid drv state.\n");
		return;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		return;
	}

	prChipInfo = prAdapter->chip_info;

#if defined(_HIF_PCIE)
	if (prChipInfo->bus_info->hwControlVote)
		prChipInfo->bus_info->hwControlVote(prAdapter,
			FALSE, PCIE_VOTE_USER_MDDP);
#endif

	kalDevRegWrite(NULL, MD_LPCTL_ADDR, MDDP_LPCR_MD_SET_FW_OWN);
	DBGLOG(INIT, INFO, "Set MD Fw Own.\n");

#if defined(_HIF_PCIE)
	if (prChipInfo->bus_info->hwControlVote)
		prChipInfo->bus_info->hwControlVote(prAdapter,
			TRUE, PCIE_VOTE_USER_MDDP);
#endif
}

u_int8_t mddpIsMDFwOwn(void)
{
	uint32_t u4Val = 0;

	kalDevRegRead(NULL, MD_LPCTL_ADDR, &u4Val);
	DBGLOG(INIT, INFO, "Set MD Fw Status[0x%08x].\n", u4Val);

	return (u4Val & BIT(0)) == BIT(0);
}

void mddpEnableMddpSupport(void)
{
	if (!g_fgMddpEnabled)
		mddpRegisterCb();
}

void mddpDisableMddpSupport(void)
{
	if (gMddpFunc.wifi_handle)
		mddpUnregisterCb();
}

bool mddpIsSupportMcifWifi(void)
{
	if (!gMddpWFunc.get_mddp_feature || !g_fgMddpEnabled)
		return false;

	return (gMddpWFunc.get_mddp_feature() & MDDP_FEATURE_MCIF_WIFI) != 0;
}

bool mddpIsSupportMddpWh(void)
{
	if (!gMddpWFunc.get_mddp_feature || !g_fgMddpEnabled)
		return false;

	return (gMddpWFunc.get_mddp_feature() & MDDP_FEATURE_MDDP_WH) != 0;
}

#endif /* CFG_MTK_MDDP_SUPPORT */
