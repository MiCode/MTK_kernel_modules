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

#if CFG_MTK_MCIF_WIFI_SUPPORT

#include "gl_os.h"
#include "mddp_export.h"
#include "mddp.h"

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
struct mddpw_drv_handle_t gMddpWFunc = {
	mddpMdNotifyInfo,
	NULL,
	NULL,
	NULL,
	NULL,
};

struct mddp_drv_conf_t gMddpDrvConf = {
	.app_type = MDDP_APP_TYPE_WH,
};

struct mddp_drv_handle_t gMddpFunc = {
	.change_state = mddpChangeState,
};

#define MD_ON_OFF_TIMEOUT 1000
#ifdef SOC3_0
#define MD_STATUS_SYNC_CR 0x180600F4
#else
#define MD_STATUS_SYNC_CR 0x1800701C
#endif
#define MD_SUPPORT_MDDP_STATUS_SYNC_CR_BIT BIT(0)
#define MD_STATUS_OFF_SYNC_BIT BIT(1)
#define MD_STATUS_ON_SYNC_BIT BIT(2)

#define MDDP_SUPPORT_CR 0x820600d0
#define MDDP_SUPPORT_CR_BIT BIT(23)

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
uint8_t txd_length;
uint8_t txd[0];
} __packed;

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static void clear_md_wifi_off_bit(void);
static void clear_md_wifi_on_bit(void);
static bool wait_for_md_off_complete(void);
static bool wait_for_md_on_complete(void);

int32_t mddpRegisterCb(IN struct ADAPTER *prAdapter)
{
	int32_t ret = 0;

	gMddpFunc.wifi_handle = &gMddpWFunc;

	ret = mddp_drv_attach(&gMddpDrvConf, &gMddpFunc);
	DBGLOG(INIT, INFO, "MDDP Wlan callback reqister result: %d\n", ret);

	prAdapter->fgMddpActivated = true;

	return ret;
}

int32_t mddpGetMdStats(IN struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct net_device_stats *prStats;
	struct GLUE_INFO *prGlueInfo;
	struct mddpw_net_stat_t mddpNetStats;
	int32_t ret;

	if (!gMddpWFunc.get_net_stat) {
		DBGLOG(INIT, ERROR, "get_net_stat is NULL.\n");
		return 0;
	}

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);
	prStats = &prNetDevPrivate->stats;
	prGlueInfo = prNetDevPrivate->prGlueInfo;

	if (!prGlueInfo || (prGlueInfo->u4ReadyFlag == 0) ||
			!prGlueInfo->prAdapter ||
			!prGlueInfo->prAdapter->fgMddpActivated)
		return 0;

	if (!prNetDevPrivate->ucMddpSupport)
		return 0;

	/* TODO: get stats by each netdev interface. */
	ret = gMddpWFunc.get_net_stat(&mddpNetStats);
	if (ret != 0) {
		DBGLOG(INIT, ERROR, "get_net_stat fail, ret: %d.\n", ret);
		return 0;
	}

	prStats->rx_packets += mddpNetStats.rx_packets;
	prStats->tx_packets += mddpNetStats.tx_packets;
	prStats->rx_bytes += mddpNetStats.rx_bytes;
	prStats->tx_bytes += mddpNetStats.tx_bytes;
	prStats->rx_errors += mddpNetStats.rx_errors;
	prStats->tx_errors += mddpNetStats.tx_errors;
	DBGLOG(INIT, TRACE, "name: %s, [%u, %u, %u, %u, %u, %u], ret: %d.\n",
			prDev->name,
			mddpNetStats.rx_packets,
			mddpNetStats.tx_packets,
			mddpNetStats.rx_bytes,
			mddpNetStats.tx_bytes,
			mddpNetStats.rx_errors,
			mddpNetStats.tx_errors,
			ret);

	return 0;
}

int32_t mddpSetTxDescTemplate(IN struct ADAPTER *prAdapter,
	IN struct STA_RECORD *prStaRec,
	IN uint8_t fgActivate)
{
	struct mddpw_txd_t *prMddpTxd;
	uint32_t u32BufSize = 0;
	uint8_t *buff = NULL;

	if (gMddpWFunc.add_txd) {
		int32_t ret;

		u32BufSize = (sizeof(struct mddpw_txd_t) +
			NIC_TX_DESC_LONG_FORMAT_LENGTH);
		buff = kalMemAlloc(u32BufSize, VIR_MEM_TYPE);

		if (buff == NULL) {
			DBGLOG(NIC, ERROR, "Can't allocate TXD buffer.\n");
			return -1;
		}
		prMddpTxd = (struct mddpw_txd_t *) buff;
		prMddpTxd->version = 0;
		prMddpTxd->sta_idx = prStaRec->ucIndex;
		prMddpTxd->wlan_idx = prStaRec->ucWlanIndex;
		memcpy(prMddpTxd->aucMacAddr,
			prStaRec->aucMacAddr, MAC_ADDR_LEN);
		if (fgActivate)
			prMddpTxd->txd_length = NIC_TX_DESC_LONG_FORMAT_LENGTH;
		else
			prMddpTxd->txd_length = 0;
		memcpy(prMddpTxd->txd,
			prStaRec->aprTxDescTemplate[0], prMddpTxd->txd_length);
		ret = gMddpWFunc.add_txd(prMddpTxd);
		DBGLOG(NIC, INFO, "ret: %d\n", ret);
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);
	} else {
		DBGLOG(INIT, ERROR, "add_txd is NULL.\n");
	}

	return 0;
}

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

		prReorderQueParm->u2WinStart =
			(u2SSN % MAX_SEQ_NO_COUNT);
		prReorderQueParm->u2WinEnd =
			(((prReorderQueParm->u2WinStart) +
			  (prReorderQueParm->u2WinSize) - 1) %
			 MAX_SEQ_NO_COUNT);
		u2SSN = (u2SSN + 1) % MAX_SEQ_NO_COUNT;
		DBGLOG(QM, TRACE,
			"Update reorder window: SSN: %d, start: %d, end: %d.\n",
			u2SSN,
			prReorderQueParm->u2WinStart,
			prReorderQueParm->u2WinEnd);
	}

	prApBuf->start_idx = prReorderQueParm->u2WinStart;
	prApBuf->end_idx = prReorderQueParm->u2WinEnd;
}

int32_t mddpNotifyDrvTxd(IN struct ADAPTER *prAdapter,
	IN struct STA_RECORD *prStaRec,
	IN uint8_t fgActivate)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	struct mddp_txd_t *prMddpTxd;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	struct net_device *prNetdev;
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
	prNetdev = (struct net_device *) wlanGetNetInterfaceByBssIdx(
			prAdapter->prGlueInfo, prStaRec->ucBssIndex);

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
	/* (3 = version+buf_len+info_num) */
	prNotifyInfo->buf_len = (u32BufSize - 3);
	prNotifyInfo->info_num = 1;
	prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
	prDrvInfo->info_id = 3; /* MDDPW_DRV_INFO_TXD; */
	prDrvInfo->info_len = (sizeof(struct mddpw_txd_t) +
			NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prMddpTxd = (struct mddp_txd_t *) &(prDrvInfo->info[0]);
	prMddpTxd->version = 0;
	prMddpTxd->sta_idx = prStaRec->ucIndex;
	prMddpTxd->wlan_idx = prStaRec->ucWlanIndex;
	prMddpTxd->sta_mode = prStaRec->eStaType;
	prMddpTxd->bss_id = prStaRec->ucBssIndex;
	prMddpTxd->wmmset = prBssInfo->ucWmmQueSet;
	kalMemCopy(prMddpTxd->nw_if_name, prNetdev->name,
			sizeof(prMddpTxd->nw_if_name));
	kalMemCopy(prMddpTxd->aucMacAddr, prStaRec->aucMacAddr, MAC_ADDR_LEN);
	if (fgActivate) {
		prMddpTxd->txd_length = NIC_TX_DESC_LONG_FORMAT_LENGTH;
		kalMemCopy(prMddpTxd->txd, prStaRec->aprTxDescTemplate[0],
				prMddpTxd->txd_length);
	} else {
		prMddpTxd->txd_length = 0;
	}

	ret = gMddpWFunc.notify_drv_info(prNotifyInfo);

	DBGLOG(NIC, INFO,
		"ver:%d,idx:%d,w_idx:%d,mod:%d,bss:%d,wmm:%d,name:%s,act:%d\n",
			prMddpTxd->version,
			prMddpTxd->sta_idx,
			prMddpTxd->wlan_idx,
			prMddpTxd->sta_mode,
			prMddpTxd->bss_id,
			prMddpTxd->wmmset,
			prMddpTxd->nw_if_name,
			fgActivate);
	DBGLOG(NIC, INFO, "ret: %d\n", ret);

exit:
	if (buff)
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);
	return ret;
}

int32_t mddpNotifyDrvMac(IN struct ADAPTER *prAdapter)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	uint32_t u32BufSize = 0;
	uint8_t *buff = NULL;
	struct BSS_INFO *prAisBssInfo = (struct BSS_INFO *) NULL;

	if (gMddpWFunc.notify_drv_info) {
		int32_t ret;

		u32BufSize = (sizeof(struct mddpw_drv_notify_info_t) +
			sizeof(struct mddpw_drv_info_t) + MAC_ADDR_LEN);
		buff = kalMemAlloc(u32BufSize, VIR_MEM_TYPE);

		if (buff == NULL) {
			DBGLOG(NIC, ERROR, "Can't allocate TXD buffer.\n");
			return -1;
		}
		prNotifyInfo = (struct mddpw_drv_notify_info_t *) buff;
		prNotifyInfo->version = 0;
		/* (3= version+buf_len+info_num) */
		prNotifyInfo->buf_len = (u32BufSize - 3);
		prNotifyInfo->info_num = 1;
		prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
		prDrvInfo->info_id = MDDPW_DRV_INFO_DEVICE_MAC;
		prDrvInfo->info_len = MAC_ADDR_LEN;
    /*SY MCIF TBC 0916*/
		prAisBssInfo = prAdapter->prAisBssInfo[0];
		COPY_MAC_ADDR(prDrvInfo->info, prAisBssInfo->aucOwnMacAddr);

		ret = gMddpWFunc.notify_drv_info(prNotifyInfo);
		DBGLOG(INIT, INFO, "ret: %d.\n", ret);
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);
	} else {
		DBGLOG(INIT, ERROR, "notify_drv_info is NULL.\n");
	}

	return 0;
}

int32_t mddpNotifyWifiStatus(IN enum mddp_drv_onoff_status wifiOnOffStatus)
{
	struct mddpw_drv_notify_info_t *prNotifyInfo;
	struct mddpw_drv_info_t *prDrvInfo;
	uint32_t u32BufSize = 0;
	uint8_t *buff = NULL;

	DBGLOG(INIT, INFO, "mddpNotifyWifiStatus power: %d.\n",
			wifiOnOffStatus);

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
		prNotifyInfo->buf_len = u32BufSize;
		prNotifyInfo->info_num = 1;
		prDrvInfo = (struct mddpw_drv_info_t *) &(prNotifyInfo->buf[0]);
		prDrvInfo->info_id = MDDPW_DRV_INFO_NOTIFY_WIFI_ONOFF;
		prDrvInfo->info_len = WIFI_ONOFF_NOTIFICATION_LEN;
		prDrvInfo->info[0] = wifiOnOffStatus;

		DBGLOG(INIT, INFO, "notify_drv_info start.\n");
		ret = gMddpWFunc.notify_drv_info(prNotifyInfo);
		DBGLOG(INIT, INFO, "ret: %d.\n", ret);
		DBGLOG(INIT, INFO, "notify_drv_info end.\n");
		kalMemFree(buff, VIR_MEM_TYPE, u32BufSize);
	} else {
		DBGLOG(INIT, ERROR, "notify_drv_info is NULL.\n");
	}

	return 0;
}

void mddpNotifyWifiOnStart(void)
{
	mddpNotifyWifiStatus(MDDPW_DRV_INFO_WLAN_ON_START);
}

void mddpNotifyWifiOnEnd(void)
{
	int32_t ret;

	clear_md_wifi_on_bit();
	ret = mddpNotifyWifiStatus(MDDPW_DRV_INFO_WLAN_ON_END);
	if (ret == 0)
		wait_for_md_on_complete();
}

void mddpNotifyWifiOffStart(void)
{
	int32_t ret;

	clear_md_wifi_off_bit();
	ret = mddpNotifyWifiStatus(MDDPW_DRV_INFO_WLAN_OFF_START);
	if (ret == 0)
		wait_for_md_off_complete();
}

void mddpNotifyWifiOffEnd(void)
{
	mddpNotifyWifiStatus(MDDPW_DRV_INFO_WLAN_OFF_END);
}

int32_t mddpMdNotifyInfo(struct mddpw_md_notify_info_t *prMdInfo)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	u_int8_t fgHalted = kalIsHalted();

	DBGLOG(INIT, INFO, "MD notify mddpMdNotifyInfo.\n");

	if (gPrDev == NULL) {
		DBGLOG(INIT, ERROR, "gPrDev is NULL.\n");
		return 0;
	}

	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(gPrDev));
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return 0;
	}
	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
		return 0;
	}

	if (fgHalted || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(INIT, INFO,
			"Skip update info. to MD, fgHalted: %d, u4ReadyFlag: %d\n",
			fgHalted, prGlueInfo->u4ReadyFlag);
		return 0;
	}

	if (prMdInfo->info_type == 1) {
		uint32_t i;
		struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;

		mddpNotifyWifiOnStart();
		mddpNotifyWifiOnEnd();
		mddpNotifyDrvMac(prAdapter);

		/* Notify STA's TXD to MD */
		for (i = 0; i < KAL_AIS_NUM; i++) {
			struct BSS_INFO *prAisBssInfo = aisGetAisBssInfo(
					prAdapter,
					i);

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
				mddpNotifyDrvTxd(prAdapter,
						prCurrStaRec,
						TRUE);
			}
		}
	}
	return 0;
}

int32_t mddpChangeState(enum mddp_state_e event, void *buf, uint32_t *buf_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;

	if (gPrDev == NULL) {
		DBGLOG(INIT, ERROR, "gPrDev is NULL.\n");
		return 0;
	}

	prGlueInfo = *((struct GLUE_INFO **)netdev_priv(gPrDev));
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
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

static void clear_md_wifi_off_bit(void)
{
	uint32_t u4Value = 0;

	DBGLOG(INIT, INFO, "md off start.\n");
	wf_ioremap_read(MD_STATUS_SYNC_CR, &u4Value);
	u4Value |= MD_STATUS_OFF_SYNC_BIT;
	wf_ioremap_write(MD_STATUS_SYNC_CR, u4Value);
}

static void clear_md_wifi_on_bit(void)
{
	uint32_t u4Value = 0;

	wf_ioremap_read(MD_STATUS_SYNC_CR, &u4Value);
	u4Value &= ~MD_STATUS_ON_SYNC_BIT;
	wf_ioremap_write(MD_STATUS_SYNC_CR, u4Value);
}

static bool wait_for_md_off_complete(void)
{
	uint32_t u4Value = 0;
	uint32_t u4StartTime, u4CurTime;
	bool fgTimeout = false;

	u4StartTime = kalGetTimeTick();

	do {
		wf_ioremap_read(MD_STATUS_SYNC_CR, &u4Value);

		if ((u4Value & MD_STATUS_OFF_SYNC_BIT) == 0) {
			DBGLOG(INIT, INFO, "md off end.\n");
			break;
		}

		u4CurTime = kalGetTimeTick();
		if (CHECK_FOR_TIMEOUT(u4CurTime, u4StartTime,
				MD_ON_OFF_TIMEOUT)) {
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
	bool fgTimeout = false;

	u4StartTime = kalGetTimeTick();

	do {
		wf_ioremap_read(MD_STATUS_SYNC_CR, &u4Value);

		if ((u4Value & MD_STATUS_ON_SYNC_BIT) > 0) {
			DBGLOG(INIT, INFO, "md on end.\n");
			break;
		}

		u4CurTime = kalGetTimeTick();
		if (CHECK_FOR_TIMEOUT(u4CurTime, u4StartTime,
				MD_ON_OFF_TIMEOUT)) {
			DBGLOG(INIT, ERROR, "wait for md on timeout\n");
			fgTimeout = true;
			break;
		}

		kalMsleep(CFG_RESPONSE_POLLING_DELAY);
	} while (TRUE);

	return !fgTimeout;
}

void setMddpSupportRegister(IN struct ADAPTER *prAdapter)
{
	uint32_t u4Val = 0;

	HAL_MCR_RD(prAdapter, MDDP_SUPPORT_CR, &u4Val);
	u4Val |= MDDP_SUPPORT_CR_BIT;
	HAL_MCR_WR(prAdapter, MDDP_SUPPORT_CR, u4Val);
}

#endif
