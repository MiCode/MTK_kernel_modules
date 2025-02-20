// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file wlanoid.c
 *   \brief This file contains the WLAN OID processing routines of Windows
 *          driver for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */


/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "precomp.h"
#include "gl_kal.h"
#include "mgmt/rsn.h"
#include "debug.h"
#if CFG_SUPPORT_NAN
#include "cmd_buf.h"
#include "nan_txm.h"
#endif
#if CFG_SUPPORT_CSI
#include "gl_csi.h"
#endif
#include "mddp.h"

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */
#if DBG && 0
static void SetRCID(u_int8_t fgOneTb3, u_int8_t *fgRCID);
#endif

#if CFG_SLT_SUPPORT
static void SetTestChannel(uint8_t *pucPrimaryChannel);
#endif

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */
#if CFG_ENABLE_WIFI_DIRECT
static void setApUapsdEnable(struct ADAPTER *prAdapter,
			     u_int8_t enable)
{
	struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT rUapsdParams;
	uint32_t u4SetInfoLen = 0;
	uint8_t ucBssIdx;

	/* FIX ME: Add p2p role index selection */
	if (p2pFuncRoleToBssIdx(
		    prAdapter, 0, &ucBssIdx) != WLAN_STATUS_SUCCESS)
		return;

	DBGLOG(OID, INFO, "setApUapsdEnable: %d, ucBssIdx: %d\n",
	       enable, ucBssIdx);

	rUapsdParams.ucBssIdx = ucBssIdx;

	if (enable) {
		prAdapter->rWifiVar.ucApUapsd = TRUE;
		rUapsdParams.fgEnAPSD = 1;
		rUapsdParams.fgEnAPSD_AcBe = 1;
		rUapsdParams.fgEnAPSD_AcBk = 1;
		rUapsdParams.fgEnAPSD_AcVi = 1;
		rUapsdParams.fgEnAPSD_AcVo = 1;
		/* default: 0, do not limit delivery pkt number */
		rUapsdParams.ucMaxSpLen = 0;
	} else {
		prAdapter->rWifiVar.ucApUapsd = FALSE;
		rUapsdParams.fgEnAPSD = 0;
		rUapsdParams.fgEnAPSD_AcBe = 0;
		rUapsdParams.fgEnAPSD_AcBk = 0;
		rUapsdParams.fgEnAPSD_AcVi = 0;
		rUapsdParams.fgEnAPSD_AcVo = 0;
		/* default: 0, do not limit delivery pkt number */
		rUapsdParams.ucMaxSpLen = 0;
	}
	wlanoidSetUApsdParam(prAdapter,
			     &rUapsdParams,
			     sizeof(struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT),
			     &u4SetInfoLen);
}
#endif

#if CFG_ENABLE_STATISTICS_BUFFERING
static u_int8_t IsBufferedStatisticsUsable(
	struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	if (prAdapter->fgIsStatValid == TRUE &&
	    (kalGetTimeTick() - prAdapter->rStatUpdateTime) <=
	    CFG_STATISTICS_VALID_CYCLE)
		return TRUE;
	else
		return FALSE;
}
#endif

#if DBG && 0
static void SetRCID(u_int8_t fgOneTb3, u_int8_t *fgRCID)
{
	if (fgOneTb3)
		*fgRCID = 0;
	else
		*fgRCID = 1;
}
#endif

#if CFG_SLT_SUPPORT
static void SetTestChannel(uint8_t *pucPrimaryChannel)
{
	if (*pucPrimaryChannel < 5)
		*pucPrimaryChannel = 8;
	else if (*pucPrimaryChannel > 10)
		*pucPrimaryChannel = 3;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the supported physical layer network
 *        type that can be used by the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryNetworkTypesSupported(struct ADAPTER
				  *prAdapter,
				  void *pvQueryBuffer,
				  uint32_t u4QueryBufferLen,
				  uint32_t *pu4QueryInfoLen)
{
	uint32_t u4NumItem = 0;
	enum ENUM_PARAM_NETWORK_TYPE
	eSupportedNetworks[PARAM_NETWORK_TYPE_NUM];
	struct PARAM_NETWORK_TYPE_LIST *prSupported;

	/* The array of all physical layer network subtypes that the driver
	 * supports.
	 */

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	/* Init. */
	for (u4NumItem = 0; u4NumItem < PARAM_NETWORK_TYPE_NUM;
	     u4NumItem++)
		eSupportedNetworks[u4NumItem] = 0;

	u4NumItem = 0;

	eSupportedNetworks[u4NumItem] = PARAM_NETWORK_TYPE_DS;
	u4NumItem++;

	eSupportedNetworks[u4NumItem] = PARAM_NETWORK_TYPE_OFDM24;
	u4NumItem++;

	*pu4QueryInfoLen =
		(uint32_t) OFFSET_OF(struct PARAM_NETWORK_TYPE_LIST,
				     eNetworkType) +
		(u4NumItem * sizeof(enum ENUM_PARAM_NETWORK_TYPE));

	if (u4QueryBufferLen < *pu4QueryInfoLen)
		return WLAN_STATUS_INVALID_LENGTH;

	prSupported = (struct PARAM_NETWORK_TYPE_LIST *)
		      pvQueryBuffer;
	prSupported->NumberOfItems = u4NumItem;
	kalMemCopy(prSupported->eNetworkType, eSupportedNetworks,
		   u4NumItem * sizeof(enum ENUM_PARAM_NETWORK_TYPE));

	DBGLOG(REQ, TRACE, "NDIS supported network type list: %u\n",
	       prSupported->NumberOfItems);
	DBGLOG_MEM8(REQ, INFO, prSupported, *pu4QueryInfoLen);

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryNetworkTypesSupported */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current physical layer network
 *        type used by the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *             the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                             bytes written into the query buffer. If the
 *                             call failed due to invalid length of the query
 *                             buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryNetworkTypeInUse(struct ADAPTER *prAdapter,
			     void *pvQueryBuffer,
			     uint32_t u4QueryBufferLen,
			     uint32_t *pu4QueryInfoLen)
{
	/* TODO: need to check the OID handler content again!! */

	enum ENUM_PARAM_NETWORK_TYPE rCurrentNetworkTypeInUse;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < sizeof(enum
				      ENUM_PARAM_NETWORK_TYPE)) {
		*pu4QueryInfoLen = sizeof(enum ENUM_PARAM_NETWORK_TYPE);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	prWlanInfo = &prAdapter->rWlanInfo;
	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
		ucBssIndex) == MEDIA_STATE_CONNECTED)
		rCurrentNetworkTypeInUse = (enum ENUM_PARAM_NETWORK_TYPE) (
			prWlanInfo->ucNetworkType[ucBssIndex]);
	else
		rCurrentNetworkTypeInUse = (enum ENUM_PARAM_NETWORK_TYPE) (
			prWlanInfo->ucNetworkTypeInUse);

	*(enum ENUM_PARAM_NETWORK_TYPE *) pvQueryBuffer =
		rCurrentNetworkTypeInUse;
	*pu4QueryInfoLen = sizeof(enum ENUM_PARAM_NETWORK_TYPE);

	DBGLOG(REQ, TRACE, "Network type in use: %d\n",
	       rCurrentNetworkTypeInUse);

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryNetworkTypeInUse */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the physical layer network type used
 *        by the driver.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns the
 *                          amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS The given network type is supported and accepted.
 * \retval WLAN_STATUS_INVALID_DATA The given network type is not in the
 *                                  supported list.
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetNetworkTypeInUse(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	/* TODO: need to check the OID handler content again!! */
	enum ENUM_PARAM_NETWORK_TYPE eNewNetworkType;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(enum ENUM_PARAM_NETWORK_TYPE)) {
		*pu4SetInfoLen = sizeof(enum ENUM_PARAM_NETWORK_TYPE);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prWlanInfo = &prAdapter->rWlanInfo;
	eNewNetworkType = *(enum ENUM_PARAM_NETWORK_TYPE *)
			  pvSetBuffer;
	*pu4SetInfoLen = sizeof(enum ENUM_PARAM_NETWORK_TYPE);

	DBGLOG(REQ, INFO, "New network type: %d mode\n",
	       eNewNetworkType);

	switch (eNewNetworkType) {

	case PARAM_NETWORK_TYPE_DS:
		prWlanInfo->ucNetworkTypeInUse = (uint8_t)PARAM_NETWORK_TYPE_DS;
		break;

	case PARAM_NETWORK_TYPE_OFDM5:
		prWlanInfo->ucNetworkTypeInUse =
			(uint8_t) PARAM_NETWORK_TYPE_OFDM5;
		break;

	case PARAM_NETWORK_TYPE_OFDM24:
		prWlanInfo->ucNetworkTypeInUse =
			(uint8_t) PARAM_NETWORK_TYPE_OFDM24;
		break;

	case PARAM_NETWORK_TYPE_AUTOMODE:
		prWlanInfo->ucNetworkTypeInUse =
			(uint8_t) PARAM_NETWORK_TYPE_AUTOMODE;
		break;

	case PARAM_NETWORK_TYPE_FH:
		DBGLOG(REQ, INFO, "Not support network type: %d\n",
		       eNewNetworkType);
		rStatus = WLAN_STATUS_NOT_SUPPORTED;
		break;

	default:
		DBGLOG(REQ, INFO, "Unknown network type: %d\n",
		       eNewNetworkType);
		rStatus = WLAN_STATUS_INVALID_DATA;
		break;
	}

	/* Verify if we support the new network type. */
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "Unknown network type: %d\n",
		       eNewNetworkType);

	return rStatus;
} /* wlanoidSetNetworkTypeInUse */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current BSSID.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                             bytes written into the query buffer. If the call
 *                             failed due to invalid length of the query buffer,
 *                             returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryBssid(struct ADAPTER *prAdapter,
		  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		  uint32_t *pu4QueryInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;
	struct PARAM_BSSID_EX *prCurrBssid;

	ASSERT(prAdapter);

	if (u4QueryBufferLen < MAC_ADDR_LEN) {
		ASSERT(pu4QueryInfoLen);
		*pu4QueryInfoLen = MAC_ADDR_LEN;
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	ASSERT(u4QueryBufferLen >= MAC_ADDR_LEN);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	prCurrBssid = aisGetCurrBssId(prAdapter, ucBssIndex);

	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo, ucBssIndex) ==
	    MEDIA_STATE_CONNECTED)
		COPY_MAC_ADDR(pvQueryBuffer, prCurrBssid->arMacAddress);
	else if (aisGetOPMode(prAdapter, ucBssIndex) == NET_TYPE_IBSS) {
		uint8_t aucTemp[PARAM_MAC_ADDR_LEN];	/*!< BSSID */

		COPY_MAC_ADDR(aucTemp, prCurrBssid->arMacAddress);
		aucTemp[0] &= ~BIT(0);
		aucTemp[1] |= BIT(1);
		COPY_MAC_ADDR(pvQueryBuffer, aucTemp);
	} else
		rStatus = WLAN_STATUS_ADAPTER_NOT_READY;

	*pu4QueryInfoLen = MAC_ADDR_LEN;
	return rStatus;
} /* wlanoidQueryBssid */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the bss idx
 *        with specific BSSID in the same MLD.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                             bytes written into the query buffer. If the call
 *                             failed due to invalid length of the query buffer,
 *                             returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryLinkBssInfo(struct ADAPTER *prAdapter,
		  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		  uint32_t *pu4QueryInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;
	struct PARAM_LINK_BSS_INFO *prParamLinkBss;
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	struct BSS_INFO *prLinkBss;
	bool fgFindMldBssId = FALSE;
#endif
	if (!prAdapter || !pu4QueryInfoLen || !pvQueryBuffer)
		return WLAN_STATUS_INVALID_DATA;

	if (u4QueryBufferLen < sizeof(struct PARAM_LINK_BSS_INFO)) {
		*pu4QueryInfoLen = sizeof(struct PARAM_LINK_BSS_INFO);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	} else if (u4QueryBufferLen > sizeof(struct PARAM_LINK_BSS_INFO))
		return WLAN_STATUS_INVALID_LENGTH;

	prParamLinkBss = (struct PARAM_LINK_BSS_INFO *)pvQueryBuffer;
	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "invalid bss info:%u", ucBssIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo, ucBssIndex) !=
		MEDIA_STATE_CONNECTED) {
		DBGLOG(REQ, WARN, "not yet connected\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (prMldBssInfo) {
		LINK_FOR_EACH_ENTRY(prLinkBss, &prMldBssInfo->rBssList,
			rLinkEntryMld, struct BSS_INFO) {
			if (EQUAL_MAC_ADDR(prLinkBss->aucBSSID,
				prParamLinkBss->aucMacAddr)) {
				fgFindMldBssId = TRUE;
				prParamLinkBss->ucBssIndex =
					prLinkBss->ucBssIndex;
				break;
			}
		}
		if (!fgFindMldBssId)
			rStatus = WLAN_STATUS_FAILURE;
	} else
#endif
	{
		/* 1. check input MAC address */
		/* On Android O, this might be wlan0 address */
		if (UNEQUAL_MAC_ADDR(prBssInfo->aucBSSID,
			prParamLinkBss->aucMacAddr)) {
			/* wrong MAC address */
			DBGLOG(REQ, WARN,
			       "incorrect BSSID: [" MACSTR
			       "] currently connected BSSID["
			       MACSTR "]\n",
			       MAC2STR(prParamLinkBss->aucMacAddr),
			       MAC2STR(prBssInfo->aucBSSID));
			rStatus = WLAN_STATUS_FAILURE;
		}
	}

	DBGLOG(REQ, TRACE, "bssidx:%u mac["MACSTR"]",
		prParamLinkBss->ucBssIndex,
		MAC2STR(prParamLinkBss->aucMacAddr));
	*pu4QueryInfoLen = sizeof(struct PARAM_LINK_BSS_INFO);
	return rStatus;
} /* wlanoidQueryBssid */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the list of all BSSIDs detected by
 *        the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                             bytes written into the query buffer. If the call
 *                             failed due to invalid length of the query buffer,
 *                             returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryBssidList(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	uint32_t i, u4BssidListExLen;
	struct PARAM_BSSID_LIST_EX *prList;
	struct PARAM_BSSID_EX *prBssidEx;
	uint8_t *cp;
	struct WLAN_INFO *prWlanInfo;
	struct PARAM_BSSID_EX *prScanResult;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen) {
		ASSERT(pvQueryBuffer);

		if (!pvQueryBuffer)
			return WLAN_STATUS_INVALID_DATA;
	}

	prGlueInfo = prAdapter->prGlueInfo;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in qeury BSSID list! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	prWlanInfo = &prAdapter->rWlanInfo;
	prScanResult = prWlanInfo->arScanResult;

	u4BssidListExLen = 0;

	if (prAdapter->fgIsRadioOff == FALSE) {
		for (i = 0; i < prWlanInfo->u4ScanResultNum; i++)
			u4BssidListExLen += ALIGN_4(prScanResult[i].u4Length);
	}

	if (u4BssidListExLen)
		u4BssidListExLen += 4;	/* u4NumberOfItems. */
	else
		u4BssidListExLen = sizeof(struct PARAM_BSSID_LIST_EX);

	*pu4QueryInfoLen = u4BssidListExLen;

	if (u4QueryBufferLen < *pu4QueryInfoLen)
		return WLAN_STATUS_INVALID_LENGTH;

	/* Clear the buffer */
	kalMemZero(pvQueryBuffer, u4BssidListExLen);

	prList = (struct PARAM_BSSID_LIST_EX *) pvQueryBuffer;
	cp = (uint8_t *) &prList->arBssid[0];

	if (prAdapter->fgIsRadioOff == FALSE &&
	    prWlanInfo->u4ScanResultNum > 0) {
		/* fill up for each entry */
		for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {
			prBssidEx = (struct PARAM_BSSID_EX *) cp;

			/* copy structure */
			*prBssidEx = prScanResult[i];

			/* assign ie buffer head*/
			prBssidEx->pucIE = (uint8_t *)(prBssidEx + 1);

			/* For WHQL test, Rssi should be
			 * in range -10 ~ -200 dBm
			 */
			if (prBssidEx->rRssi > PARAM_WHQL_RSSI_MAX_DBM)
				prBssidEx->rRssi = PARAM_WHQL_RSSI_MAX_DBM;

			if (prScanResult[i].u4IELength > 0) {
				/* copy IEs */
				kalMemCopy(prBssidEx->pucIE,
					   prScanResult[i].pucIE,
					   prScanResult[i].u4IELength);
			}
			/* 4-bytes alignement */
			prBssidEx->u4Length = ALIGN_4(prBssidEx->u4Length);

			cp += prBssidEx->u4Length;
			prList->u4NumberOfItems++;
		}
	}

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryBssidList */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to request the driver to perform
 *        scanning with attaching information elements(IEs) specified from user
 *        space and multiple SSID
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer Pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetBssidListScanAdv(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest;
	uint8_t ucBssIndex = 0;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(OID, WARN,
		       "Fail in set BSSID list scan! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (prAdapter->fgTestMode) {
		DBGLOG(OID, WARN, "didn't support Scan in test mode\n");
		return WLAN_STATUS_FAILURE;
	}

	ASSERT(pu4SetInfoLen);
	*pu4SetInfoLen = 0;

	if (u4SetBufferLen != sizeof(struct PARAM_SCAN_REQUEST_ADV))
		return WLAN_STATUS_INVALID_LENGTH;
	else if (pvSetBuffer == NULL)
		return WLAN_STATUS_INVALID_DATA;

	if (prAdapter->fgIsRadioOff) {
		DBGLOG(OID, WARN,
		       "Return from BSSID list scan! (radio off). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_SUCCESS;
	}

	prScanRequest = (struct PARAM_SCAN_REQUEST_ADV *) pvSetBuffer;
	ucBssIndex = prScanRequest->ucBssIndex;
#if CFG_SUPPORT_RDD_TEST_MODE
	if (prAdapter->prGlueInfo->prRegInfo->u4RddTestMode) {
		if (prAdapter->fgEnOnlineScan && prAdapter->ucRddStatus) {
			if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
				ucBssIndex) != MEDIA_STATE_CONNECTED) {
				aisFsmScanRequestAdv(prAdapter, prScanRequest);
			} else
				return WLAN_STATUS_FAILURE;
		} else
			return WLAN_STATUS_FAILURE;
	} else
#endif
	{
#if CFG_ENABLE_CSA_BLOCK_SCAN
		if (p2pFuncIsCsaBlockScan(prAdapter)) {
			DBGLOG(OID, WARN,
		       "Not to do scan during SAP CSA!!\n");
			return WLAN_STATUS_FAILURE;
		} else if (prAdapter->fgEnOnlineScan == TRUE) {
#ifdef CFG_SUPPORT_TWT_EXT
			if (IS_FEATURE_ENABLED(
				prAdapter->rWifiVar.ucTWTRequester))
				twtPlannerCheckTeardownSuspend(prAdapter,
					TRUE, FALSE);
#endif
			aisFsmScanRequestAdv(prAdapter, prScanRequest);
		} else if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex) != MEDIA_STATE_CONNECTED) {
			aisFsmScanRequestAdv(prAdapter, prScanRequest);
		} else
			return WLAN_STATUS_FAILURE;
#else
		if (prAdapter->fgEnOnlineScan == TRUE) {
			aisFsmScanRequestAdv(prAdapter, prScanRequest);
		} else if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex) != MEDIA_STATE_CONNECTED) {
			aisFsmScanRequestAdv(prAdapter, prScanRequest);
		} else
			return WLAN_STATUS_FAILURE;
#endif
	}
	cnmTimerStartTimer(prAdapter,
			   aisGetScanDoneTimer(prAdapter, ucBssIndex),
			   SEC_TO_MSEC(AIS_SCN_DONE_TIMEOUT_SEC));
	return WLAN_STATUS_SUCCESS;
} /* wlanoidSetBssidListScanAdv */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine will initiate the join procedure to attempt to associate
 *        with the specified BSSID.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetBssid(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	uint8_t *pAddr;
	uint32_t i;
	int32_t i4Idx = -1;
	struct MSG_AIS_ABORT *prAisAbortMsg;
	uint8_t ucReasonOfDisconnect;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct PARAM_BSSID_EX *prCurrBssid;
	uint8_t ucBssIndex = 0;
	struct WLAN_INFO *prWlanInfo;
	struct PARAM_BSSID_EX *prScanResult;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);

	prCurrBssid = aisGetCurrBssId(prAdapter,
		ucBssIndex);

	*pu4SetInfoLen = MAC_ADDR_LEN;
	if (u4SetBufferLen != MAC_ADDR_LEN) {
		*pu4SetInfoLen = MAC_ADDR_LEN;
		return WLAN_STATUS_INVALID_LENGTH;
	} else if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set ssid! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	pAddr = (uint8_t *) pvSetBuffer;
	prWlanInfo = &prAdapter->rWlanInfo;
	prScanResult = prWlanInfo->arScanResult;

	/* re-association check */
	if (kalGetMediaStateIndicated(prGlueInfo, ucBssIndex) ==
			MEDIA_STATE_CONNECTED) {
		if (EQUAL_MAC_ADDR(
		    prCurrBssid->arMacAddress, pAddr)) {
			kalSetMediaStateIndicated(prGlueInfo,
					MEDIA_STATE_TO_BE_INDICATED,
					ucBssIndex);
			ucReasonOfDisconnect =
					DISCONNECT_REASON_CODE_REASSOCIATION;
		} else {
			kalIndicateStatusAndComplete(prGlueInfo,
					WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0,
					ucBssIndex);
			ucReasonOfDisconnect =
					DISCONNECT_REASON_CODE_NEW_CONNECTION;
		}
	} else {
		ucReasonOfDisconnect =
			DISCONNECT_REASON_CODE_NEW_CONNECTION;
	}

	/* check if any scanned result matchs with the BSSID */
	for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {
		if (EQUAL_MAC_ADDR(prScanResult[i].arMacAddress, pAddr)) {
			i4Idx = (int32_t) i;
			break;
		}
	}

	/* prepare message to AIS */
	if (prConnSettings->eOPMode == NET_TYPE_IBSS ||
	    prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS) {
		/* IBSS *//* beacon period */
		prConnSettings->u2BeaconPeriod = prWlanInfo->u2BeaconPeriod;
		prConnSettings->u2AtimWindow = prWlanInfo->u2AtimWindow;
	}

	/* Set Connection Request Issued Flag */
	prConnSettings->eConnectionPolicy =
		CONNECT_BY_BSSID;

	/* Send AIS Abort Message */
	prAisAbortMsg = (struct MSG_AIS_ABORT *) cnmMemAlloc(
			prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_AIS_ABORT));
	if (!prAisAbortMsg) {
		DBGLOG(REQ, ERROR, "Fail in allocating AisAbortMsg.\n");
		return WLAN_STATUS_FAILURE;
	}

	prAisAbortMsg->rMsgHdr.eMsgId = MID_OID_AIS_FSM_JOIN_REQ;
	prAisAbortMsg->ucReasonOfDisconnect = ucReasonOfDisconnect;

	/* Update the information to CONNECTION_SETTINGS_T */
	prConnSettings->ucSSIDLen = 0;
	prConnSettings->aucSSID[0] = '\0';
	COPY_MAC_ADDR(prConnSettings->aucBSSID, pAddr);

	if (EQUAL_MAC_ADDR(prCurrBssid->arMacAddress, pAddr))
		prAisAbortMsg->fgDelayIndication = TRUE;
	else
		prAisAbortMsg->fgDelayIndication = FALSE;
	prAisAbortMsg->ucBssIndex = ucBssIndex;
	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prAisAbortMsg, MSG_SEND_METHOD_BUF);

	return WLAN_STATUS_SUCCESS;
} /* end of wlanoidSetBssid() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine will initiate the join procedure to attempt
 *        to associate with the new SSID. If the previous scanning
 *        result is aged, we will scan the channels at first.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer Pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetSsid(struct ADAPTER *prAdapter,
	       void *pvSetBuffer, uint32_t u4SetBufferLen,
	       uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_SSID *pParamSsid;
	uint32_t i;
	int32_t i4Idx = -1, i4MaxRSSI = INT_MIN;
	struct MSG_AIS_ABORT *prAisAbortMsg;
	u_int8_t fgIsValidSsid = TRUE;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct PARAM_BSSID_EX *prCurrBssid;
	uint8_t ucBssIndex = 0;
	struct WLAN_INFO *prWlanInfo;
	struct PARAM_BSSID_EX *prScanResult;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	/* MSDN:
	 * Powering on the radio if the radio is powered off through a setting
	 * of OID_802_11_DISASSOCIATE
	 */
	if (prAdapter->fgIsRadioOff == TRUE)
		prAdapter->fgIsRadioOff = FALSE;

	if (u4SetBufferLen < sizeof(struct PARAM_SSID)
	    || u4SetBufferLen > sizeof(struct PARAM_SSID))
		return WLAN_STATUS_INVALID_LENGTH;
	else if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set ssid! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	pParamSsid = (struct PARAM_SSID *) pvSetBuffer;

	if (pParamSsid->u4SsidLen > 32)
		return WLAN_STATUS_INVALID_LENGTH;

	prGlueInfo = prAdapter->prGlueInfo;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);
	prCurrBssid = aisGetCurrBssId(prAdapter,
		ucBssIndex);

	/* prepare for CMD_BUILD_CONNECTION & CMD_GET_CONNECTION_STATUS */
	/* re-association check */
	if (kalGetMediaStateIndicated(prGlueInfo, ucBssIndex) ==
			MEDIA_STATE_CONNECTED) {
		if (EQUAL_SSID(prCurrBssid->rSsid.aucSsid,
			       prCurrBssid->rSsid.u4SsidLen,
			       pParamSsid->aucSsid, pParamSsid->u4SsidLen)) {
			kalSetMediaStateIndicated(prGlueInfo,
					MEDIA_STATE_TO_BE_INDICATED,
					ucBssIndex);
		} else
			kalIndicateStatusAndComplete(prGlueInfo,
					WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0,
					ucBssIndex);
	}

	prWlanInfo = &prAdapter->rWlanInfo;
	prScanResult = prWlanInfo->arScanResult;

	/* check if any scanned result matchs with the SSID */
	for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {
		uint8_t *aucSsid = prScanResult[i].rSsid.aucSsid;
		uint8_t ucSsidLength = (uint8_t)prScanResult[i].rSsid.u4SsidLen;
		int32_t i4RSSI = prScanResult[i].rRssi;

		if (EQUAL_SSID(aucSsid, ucSsidLength, pParamSsid->aucSsid,
			       pParamSsid->u4SsidLen) &&
		    i4RSSI >= i4MaxRSSI) {
			i4Idx = (int32_t) i;
			i4MaxRSSI = i4RSSI;
		}
	}

	/* prepare message to AIS */
	if (prConnSettings->eOPMode == NET_TYPE_IBSS ||
	    prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS) {
		/* IBSS *//* beacon period */
		prConnSettings->u2BeaconPeriod = prWlanInfo->u2BeaconPeriod;
		prConnSettings->u2AtimWindow = prWlanInfo->u2AtimWindow;
	}

	if (prAdapter->rWifiVar.fgSupportWZCDisassociation) {
		if (pParamSsid->u4SsidLen == ELEM_MAX_LEN_SSID) {
			fgIsValidSsid = FALSE;

			for (i = 0; i < ELEM_MAX_LEN_SSID; i++) {
				if (!((pParamSsid->aucSsid[i] > 0)
				      && (pParamSsid->aucSsid[i] <= 0x1F))) {
					fgIsValidSsid = TRUE;
					break;
				}
			}
		}
	}

	/* Set Connection Request Issued Flag */
	if (fgIsValidSsid) {
		if (pParamSsid->u4SsidLen)
			prConnSettings->eConnectionPolicy =
				CONNECT_BY_SSID_BEST_RSSI;
		else
			/* wildcard SSID */
			prConnSettings->eConnectionPolicy =
				CONNECT_BY_SSID_ANY;
	}
	/* Init join status to a non-zero value to prevent returning
	 * status success due to auth/assoc no response or valid ap selection
	 */
	prConnSettings->u2JoinStatus = STATUS_CODE_AUTH_TIMEOUT;

	/* Send AIS Abort Message */
	prAisAbortMsg = (struct MSG_AIS_ABORT *) cnmMemAlloc(
			prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_AIS_ABORT));
	if (!prAisAbortMsg) {
		DBGLOG(REQ, ERROR, "Fail in allocating AisAbortMsg.\n");
		return WLAN_STATUS_FAILURE;
	}

	prAisAbortMsg->rMsgHdr.eMsgId = MID_OID_AIS_FSM_JOIN_REQ;
	prAisAbortMsg->ucReasonOfDisconnect =
		DISCONNECT_REASON_CODE_NEW_CONNECTION;
	COPY_SSID(prConnSettings->aucSSID,
		  prConnSettings->ucSSIDLen,
		  pParamSsid->aucSsid, (uint8_t) pParamSsid->u4SsidLen);

	if (EQUAL_SSID(
		    prCurrBssid->rSsid.aucSsid,
		    prCurrBssid->rSsid.u4SsidLen,
		    pParamSsid->aucSsid, pParamSsid->u4SsidLen)) {
		prAisAbortMsg->fgDelayIndication = TRUE;
	} else {
		/* Update the information to CONNECTION_SETTINGS_T */
		prAisAbortMsg->fgDelayIndication = FALSE;
	}

	prAisAbortMsg->ucBssIndex = ucBssIndex;

	DBGLOG(SCN, INFO, "ucBssIndex %d, SSID %s\n",
			ucBssIndex,
			HIDE(prConnSettings->aucSSID));

	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prAisAbortMsg, MSG_SEND_METHOD_BUF);

	return WLAN_STATUS_SUCCESS;

} /* end of wlanoidSetSsid() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine will initiate the join procedure to attempt
 *        to associate with the new BSS, base on given SSID, BSSID, and
 *        freqency.
 *	  If the target connecting BSS is in the same ESS as current connected
 *        BSS, roaming will be performed.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer Pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetConnect(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_CONNECT *pParamConn;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint32_t i;
	struct MSG_AIS_ABORT *prAisAbortMsg;
	u_int8_t fgIsValidSsid = TRUE;
	u_int8_t fgEqualSsid = FALSE;
	u_int8_t fgEqualBssid = FALSE;
	const uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;
	uint8_t ucBssIndex = 0;
	struct PARAM_BSSID_EX *prCurrBssid;
	struct WLAN_INFO *prWlanInfo;
#if CFG_SUPPORT_ROAMING
	struct ROAMING_INFO *roam;
#endif

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	/* MSDN:
	 * Powering on the radio if the radio is powered off through a setting
	 * of OID_802_11_DISASSOCIATE
	 */
	if (prAdapter->fgIsRadioOff == TRUE)
		prAdapter->fgIsRadioOff = FALSE;

	if (u4SetBufferLen != sizeof(struct PARAM_CONNECT))
		return WLAN_STATUS_INVALID_LENGTH;
	else if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set ssid! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}
	prAisAbortMsg = (struct MSG_AIS_ABORT *) cnmMemAlloc(
			prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_AIS_ABORT));
	if (!prAisAbortMsg) {
		DBGLOG(REQ, ERROR, "Fail in allocating AisAbortMsg.\n");
		return WLAN_STATUS_FAILURE;
	}
	prAisAbortMsg->rMsgHdr.eMsgId = MID_OID_AIS_FSM_JOIN_REQ;

	pParamConn = (struct PARAM_CONNECT *) pvSetBuffer;

	ucBssIndex = pParamConn->ucBssIdx;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	prCurrBssid = aisGetCurrBssId(prAdapter,
		ucBssIndex);
#if CFG_SUPPORT_ROAMING
	roam = aisGetRoamingInfo(prAdapter, ucBssIndex);
#endif
	if (pParamConn->u4SsidLen > 32) {
		cnmMemFree(prAdapter, prAisAbortMsg);
		DBGLOG(OID, WARN, "SsidLen [%d] is invalid!\n",
		       pParamConn->u4SsidLen);
		return WLAN_STATUS_INVALID_LENGTH;
	} else if (!pParamConn->pucBssid && !pParamConn->pucSsid) {
		cnmMemFree(prAdapter, prAisAbortMsg);
		DBGLOG(OID, WARN, "Bssid or ssid is invalid!\n");
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prWlanInfo = &prAdapter->rWlanInfo;

	kalMemZero(prConnSettings->aucSSID,
		   sizeof(prConnSettings->aucSSID));
	prConnSettings->ucSSIDLen = 0;
	kalMemZero(prConnSettings->aucBSSID,
		   sizeof(prConnSettings->aucBSSID));
	kalMemZero(prConnSettings->aucBSSIDHint,
			sizeof(prConnSettings->aucBSSIDHint));
	prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_ANY;
	prConnSettings->u2LinkIdBitmap = pParamConn->u2LinkIdBitmap;

	if (pParamConn->pucSsid) {
		prConnSettings->eConnectionPolicy =
			CONNECT_BY_SSID_BEST_RSSI;
		COPY_SSID(prConnSettings->aucSSID,
			  prConnSettings->ucSSIDLen, pParamConn->pucSsid,
			  (uint8_t) pParamConn->u4SsidLen);
		if (EQUAL_SSID(prCurrBssid->rSsid.aucSsid,
			       prCurrBssid->rSsid.u4SsidLen,
			       pParamConn->pucSsid, pParamConn->u4SsidLen))
			fgEqualSsid = TRUE;
	}
	if (pParamConn->pucBssid) {
		if (!EQUAL_MAC_ADDR(aucZeroMacAddr, pParamConn->pucBssid)
		    && IS_UCAST_MAC_ADDR(pParamConn->pucBssid)) {
			prConnSettings->eConnectionPolicy = CONNECT_BY_BSSID;
			COPY_MAC_ADDR(prConnSettings->aucBSSID,
				      pParamConn->pucBssid);
			if (EQUAL_MAC_ADDR(
			    prCurrBssid->arMacAddress,
			    pParamConn->pucBssid))
				fgEqualBssid = TRUE;
		} else
			DBGLOG(INIT, INFO, "wrong bssid " MACSTR "to connect\n",
			       MAC2STR(pParamConn->pucBssid));
	} else if (pParamConn->pucBssidHint) {
		if (!EQUAL_MAC_ADDR(aucZeroMacAddr, pParamConn->pucBssidHint)
			&& IS_UCAST_MAC_ADDR(pParamConn->pucBssidHint)) {
			if (AIS_INDEX(prAdapter, ucBssIndex) <
				prAdapter->rWifiVar.u4AisRoamingNumber) {
				prConnSettings->eConnectionPolicy =
					CONNECT_BY_BSSID_HINT;
				COPY_MAC_ADDR(prConnSettings->aucBSSIDHint,
					pParamConn->pucBssidHint);

				if (EQUAL_MAC_ADDR(
					prCurrBssid->arMacAddress,
					pParamConn->pucBssidHint))
					fgEqualBssid = TRUE;
			} else {
				prConnSettings->eConnectionPolicy =
					CONNECT_BY_BSSID;
				COPY_MAC_ADDR(prConnSettings->aucBSSID,
						pParamConn->pucBssidHint);
				if (EQUAL_MAC_ADDR(
				    prCurrBssid->arMacAddress,
				    pParamConn->pucBssidHint))
					fgEqualBssid = TRUE;
				DBGLOG(INIT, INFO,
					"Force to use bssid (%d)", ucBssIndex);
			}
		}
	} else
		DBGLOG(INIT, INFO, "No Bssid set\n");
	prConnSettings->u4FreqInMHz = pParamConn->u4CenterFreq;

#if CFG_SUPPORT_ROAMING
	/* prepare for CMD_BUILD_CONNECTION & CMD_GET_CONNECTION_STATUS */
	/* re-association check */
	if (kalGetMediaStateIndicated(prGlueInfo,
		ucBssIndex) ==
	    MEDIA_STATE_CONNECTED) {
		if (fgEqualSsid) {
			DBGLOG(INIT, INFO, "Same ssid\n");
			roam->eReason = ROAMING_REASON_UPPER_LAYER_TRIGGER;
			prAisAbortMsg->ucReasonOfDisconnect =
				pParamConn->fgTestMode ?
				DISCONNECT_REASON_CODE_TEST_MODE :
				DISCONNECT_REASON_CODE_ROAMING;
			if (fgEqualBssid) {
				DBGLOG(INIT, INFO, "Same bssid\n");
				kalSetMediaStateIndicated(prGlueInfo,
					MEDIA_STATE_TO_BE_INDICATED,
					ucBssIndex);
				prAisAbortMsg->ucReasonOfDisconnect =
					DISCONNECT_REASON_CODE_REASSOCIATION;
			}
		} else {
			DBGLOG(INIT, INFO, "DisBySsid\n");
			kalIndicateStatusAndComplete(prGlueInfo,
					WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0,
					ucBssIndex);
			prAisAbortMsg->ucReasonOfDisconnect =
					DISCONNECT_REASON_CODE_NEW_CONNECTION;
			cnmMemFree(prAdapter, prAisAbortMsg);
			/* reject this connect to avoid to install key fail */
			return WLAN_STATUS_FAILURE;
		}
	} else
#endif
		prAisAbortMsg->ucReasonOfDisconnect =
			DISCONNECT_REASON_CODE_NEW_CONNECTION;
#if 0
	/* check if any scanned result matchs with the SSID */
	for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {
		uint8_t *aucSsid = prScanResult[i].rSsid.aucSsid;
		uint8_t ucSsidLength = (uint8_t)prScanResult[i].rSsid.u4SsidLen;
		int32_t i4RSSI = prScanResult[i].rRssi;

		if (EQUAL_SSID(aucSsid, ucSsidLength, pParamConn->pucSsid,
			       pParamConn->u4SsidLen) &&
		    i4RSSI >= i4MaxRSSI) {
			i4Idx = (int32_t) i;
			i4MaxRSSI = i4RSSI;
		}
		if (EQUAL_MAC_ADDR(prScanResult[i].arMacAddress, pAddr)) {
			i4Idx = (int32_t) i;
			break;
		}
	}
#endif
	/* prepare message to AIS */
	if (prConnSettings->eOPMode == NET_TYPE_IBSS ||
	    prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS) {
		/* IBSS *//* beacon period */
		prConnSettings->u2BeaconPeriod = prWlanInfo->u2BeaconPeriod;
		prConnSettings->u2AtimWindow = prWlanInfo->u2AtimWindow;
	}

	if (prAdapter->rWifiVar.fgSupportWZCDisassociation) {
		if (pParamConn->u4SsidLen == ELEM_MAX_LEN_SSID) {
			fgIsValidSsid = FALSE;

			if (pParamConn->pucSsid) {
				for (i = 0; i < ELEM_MAX_LEN_SSID; i++) {
					if (!((pParamConn->pucSsid[i] > 0) &&
					    (pParamConn->pucSsid[i] <= 0x1F))) {
						fgIsValidSsid = TRUE;
						break;
					}
				}
			} else {
				DBGLOG(INIT, ERROR,
				       "pParamConn->pucSsid is NULL\n");
			}
		}
	}

	wlanoidUpdateConnect(prAdapter,
			pvSetBuffer, u4SetBufferLen,
			pu4SetInfoLen);

	if (fgEqualSsid || fgEqualBssid)
		prAisAbortMsg->fgDelayIndication = TRUE;
	else
		/* Update the information to CONNECTION_SETTINGS_T */
		prAisAbortMsg->fgDelayIndication = FALSE;
	prAisAbortMsg->ucBssIndex = ucBssIndex;
	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prAisAbortMsg, MSG_SEND_METHOD_BUF);

	DBGLOG(INIT, INFO,
		"ucBssIndex %d, ssid %s, bssid " MACSTR ", bssid_hint " MACSTR
		", conn policy %d, disc reason %d, freqInMHZ %d, AllowLinkID %d\n",
		ucBssIndex,
		HIDE(prConnSettings->aucSSID),
		MAC2STR(prConnSettings->aucBSSID),
		MAC2STR(prConnSettings->aucBSSIDHint),
		prConnSettings->eConnectionPolicy,
		prAisAbortMsg->ucReasonOfDisconnect,
		prConnSettings->u4FreqInMHz,
		prConnSettings->u2LinkIdBitmap);
	return WLAN_STATUS_SUCCESS;
} /* end of wlanoidSetConnect */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This interface aim to update the connect params.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer Pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidUpdateConnect(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;
	struct PARAM_CONNECT *pParamConn;

	if (u4SetBufferLen < sizeof(struct PARAM_CONNECT))
		return WLAN_STATUS_INVALID_LENGTH;

	pParamConn = (struct PARAM_CONNECT *) pvSetBuffer;
	ucBssIndex = pParamConn->ucBssIdx;
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	/* no need to replace conn ie for test mode */
	if (pParamConn->fgTestMode)
		return WLAN_STATUS_SUCCESS;

	/* Check former assocIE to prevent memory leakage in situations like
	* upper layer requests connection without disconnecting first, ...
	*/
	if (prConnSettings->assocIeLen > 0) {
		kalMemFree(prConnSettings->pucAssocIEs, VIR_MEM_TYPE,
			prConnSettings->assocIeLen);
		prConnSettings->assocIeLen = 0;
		prConnSettings->pucAssocIEs = NULL;
	}

	/*Should update Diffie-Hallmen params*/
	if (pParamConn->u4IesLen > 0) {
		prConnSettings->assocIeLen = pParamConn->u4IesLen;
		prConnSettings->pucAssocIEs =
			kalMemAlloc(prConnSettings->assocIeLen,
				    VIR_MEM_TYPE);

		if (prConnSettings->pucAssocIEs) {
			kalMemCopy(prConnSettings->pucAssocIEs,
				    pParamConn->pucIEs,
				    prConnSettings->assocIeLen);
		} else {
			DBGLOG(INIT, INFO,
				"allocate mem for prConnSettings->pucAssocIEs failed\n");
				prConnSettings->assocIeLen = 0;
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the currently associated SSID.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer Pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                             bytes written into the query buffer. If the call
 *                             failed due to invalid length of the query buffer,
 *                             returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQuerySsid(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen)
{
	struct PARAM_SSID *prAssociatedSsid;
	struct PARAM_BSSID_EX *prCurrBssid;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	*pu4QueryInfoLen = sizeof(struct PARAM_SSID);

	/* Check for query buffer length */
	if (u4QueryBufferLen < *pu4QueryInfoLen) {
		DBGLOG(REQ, WARN, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prAssociatedSsid = (struct PARAM_SSID *) pvQueryBuffer;

	kalMemZero(prAssociatedSsid->aucSsid,
		   sizeof(prAssociatedSsid->aucSsid));

	prCurrBssid = aisGetCurrBssId(prAdapter,
		ucBssIndex);

	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
		ucBssIndex) ==
	    MEDIA_STATE_CONNECTED) {
		prAssociatedSsid->u4SsidLen =
			prCurrBssid->rSsid.u4SsidLen;

		if (prAssociatedSsid->u4SsidLen) {
			kalMemCopy(prAssociatedSsid->aucSsid,
				prCurrBssid->rSsid.aucSsid,
				prAssociatedSsid->u4SsidLen);
		}
	} else {
		prAssociatedSsid->u4SsidLen = 0;

		DBGLOG(REQ, TRACE, "Null SSID\n");
	}

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQuerySsid */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current 802.11 network type.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer Pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                             bytes written into the query buffer. If the call
 *                             failed due to invalid length of the query buffer,
 *                             returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryInfrastructureMode(struct ADAPTER *prAdapter,
			       void *pvQueryBuffer,
			       uint32_t u4QueryBufferLen,
			       uint32_t *pu4QueryInfoLen)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);

	*pu4QueryInfoLen = sizeof(enum ENUM_PARAM_OP_MODE);

	if (u4QueryBufferLen < sizeof(enum ENUM_PARAM_OP_MODE))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*(enum ENUM_PARAM_OP_MODE *) pvQueryBuffer =
		prConnSettings->eOPMode;

	/*
	 ** According to OID_802_11_INFRASTRUCTURE_MODE
	 ** If there is no prior OID_802_11_INFRASTRUCTURE_MODE,
	 ** NDIS_STATUS_ADAPTER_NOT_READY shall be returned.
	 */
#if DBG
	switch (*(enum ENUM_PARAM_OP_MODE *) pvQueryBuffer) {
	case NET_TYPE_IBSS:
		DBGLOG(REQ, INFO, "IBSS mode\n");
		break;
	case NET_TYPE_INFRA:
		DBGLOG(REQ, INFO, "Infrastructure mode\n");
		break;
	default:
		DBGLOG(REQ, INFO, "Automatic mode\n");
	}
#endif

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryInfrastructureMode */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set mode to infrastructure or
 *        IBSS, or automatic switch between the two.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer Pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *             bytes read from the set buffer. If the call failed due to invalid
 *             length of the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetInfrastructureMode(struct ADAPTER *prAdapter,
			     void *pvSetBuffer, uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_OP_MODE *pOpMode;
	enum ENUM_PARAM_OP_MODE eOpMode;
	/* P_WLAN_TABLE_T       prWlanTable; */
#if CFG_SUPPORT_802_11W
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo;
#endif
	/* UINT_8 i; */
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	prGlueInfo = prAdapter->prGlueInfo;

	if (u4SetBufferLen < sizeof(struct PARAM_OP_MODE))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set infrastructure mode! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	pOpMode = (struct PARAM_OP_MODE *) pvSetBuffer;

	ucBssIndex = pOpMode->ucBssIdx;
	prAisSpecBssInfo =
		aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo =
		aisGetAisBssInfo(prAdapter, ucBssIndex);

	eOpMode = pOpMode->eOpMode;
	/* Verify the new infrastructure mode. */
	if (eOpMode >= NET_TYPE_NUM) {
		DBGLOG(REQ, TRACE, "Invalid mode value %d\n", eOpMode);
		return WLAN_STATUS_INVALID_DATA;
	}

	/* check if possible to switch to AdHoc mode */
	if (eOpMode == NET_TYPE_IBSS
	    || eOpMode == NET_TYPE_DEDICATED_IBSS) {
		if (cnmAisIbssIsPermitted(prAdapter) == FALSE) {
			DBGLOG(REQ, TRACE, "Mode value %d unallowed\n",
			       eOpMode);
			return WLAN_STATUS_FAILURE;
		}
	}

	/* Save the new infrastructure mode setting. */
	prConnSettings->eOPMode = eOpMode;

	prConnSettings->fgWapiMode = FALSE;

	/* prWlanTable = prAdapter->rWifiVar.arWtbl; */
	/* prWlanTable[prAisBssInfo->ucBMCWlanIndex].ucKeyId = 0; */

	DBGLOG(RSN, LOUD, "ucBssIndex %d\n", ucBssIndex);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_INFRASTRUCTURE,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon, nicOidCmdTimeoutCommon,
				   0, NULL, pvSetBuffer, u4SetBufferLen);
} /* wlanoidSetInfrastructureMode */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current 802.11 authentication
 *        mode.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryAuthMode(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen)
{
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(enum ENUM_PARAM_AUTH_MODE);

	if (u4QueryBufferLen < sizeof(enum ENUM_PARAM_AUTH_MODE))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	*(enum ENUM_PARAM_AUTH_MODE *) pvQueryBuffer =
		aisGetAuthMode(prAdapter, ucBssIndex);

#if DBG
	switch (*(enum ENUM_PARAM_AUTH_MODE *) pvQueryBuffer) {
	case AUTH_MODE_OPEN:
		DBGLOG(REQ, INFO, "Current auth mode: Open\n");
		break;

	case AUTH_MODE_SHARED:
		DBGLOG(REQ, INFO, "Current auth mode: Shared\n");
		break;

	case AUTH_MODE_AUTO_SWITCH:
		DBGLOG(REQ, INFO, "Current auth mode: Auto-switch\n");
		break;

	case AUTH_MODE_WPA:
		DBGLOG(REQ, INFO, "Current auth mode: WPA\n");
		break;

	case AUTH_MODE_WPA_PSK:
		DBGLOG(REQ, INFO, "Current auth mode: WPA PSK\n");
		break;

	case AUTH_MODE_WPA_NONE:
		DBGLOG(REQ, INFO, "Current auth mode: WPA None\n");
		break;

	case AUTH_MODE_WPA2:
		DBGLOG(REQ, INFO, "Current auth mode: WPA2\n");
		break;

	case AUTH_MODE_WPA2_PSK:
		DBGLOG(REQ, INFO, "Current auth mode: WPA2 PSK\n");
		break;

	default:
		DBGLOG(REQ, INFO, "Current auth mode: %d\n",
		       *(enum ENUM_PARAM_AUTH_MODE *) pvQueryBuffer);
		break;
	}
#endif
	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryAuthMode */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the IEEE 802.11 authentication mode
 *        to the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_NOT_ACCEPTED
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetAuthMode(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	/* UINT_32       i, u4AkmSuite; */
	/* P_DOT11_RSNA_CONFIG_AUTHENTICATION_SUITES_ENTRY prEntry; */
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	prGlueInfo = prAdapter->prGlueInfo;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	*pu4SetInfoLen = sizeof(enum ENUM_PARAM_AUTH_MODE);

	if (u4SetBufferLen < sizeof(enum ENUM_PARAM_AUTH_MODE))
		return WLAN_STATUS_INVALID_LENGTH;

	/* RF Test */
	/* if (IS_ARB_IN_RFTEST_STATE(prAdapter)) { */
	/* return WLAN_STATUS_SUCCESS; */
	/* } */

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set Authentication mode! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	/* Check if the new authentication mode is valid. */
	if (*(enum ENUM_PARAM_AUTH_MODE *) pvSetBuffer >=
	    AUTH_MODE_NUM) {
		DBGLOG(REQ, TRACE, "Invalid auth mode %d\n",
		       *(enum ENUM_PARAM_AUTH_MODE *) pvSetBuffer);
		return WLAN_STATUS_INVALID_DATA;
	}

	switch (*(enum ENUM_PARAM_AUTH_MODE *) pvSetBuffer) {
	case AUTH_MODE_WPA_OSEN:
	case AUTH_MODE_WPA:
	case AUTH_MODE_WPA_PSK:
	case AUTH_MODE_WPA2:
	case AUTH_MODE_WPA2_PSK:
	case AUTH_MODE_WPA2_FT:
	case AUTH_MODE_WPA2_FT_PSK:
	case AUTH_MODE_WPA3_SAE:
	case AUTH_MODE_WPA3_OWE:
	case AUTH_MODE_FILS:
		/* infrastructure mode only */
		if (prConnSettings->eOPMode != NET_TYPE_INFRA)
			return WLAN_STATUS_NOT_ACCEPTED;
		break;

	case AUTH_MODE_WPA_NONE:
		/* ad hoc mode only */
		if (prConnSettings->eOPMode !=
		    NET_TYPE_IBSS)
			return WLAN_STATUS_NOT_ACCEPTED;
		break;

	default:
		break;
	}

	/* Save the new authentication mode. */
	prConnSettings->eAuthMode = *(enum ENUM_PARAM_AUTH_MODE *) pvSetBuffer;

	switch (prConnSettings->eAuthMode) {
	case AUTH_MODE_OPEN:
		DBGLOG(RSN, TRACE, "New auth mode: open\n");
		break;
	case AUTH_MODE_SHARED:
		DBGLOG(RSN, TRACE, "New auth mode: shared\n");
		break;
	case AUTH_MODE_AUTO_SWITCH:
		DBGLOG(RSN, TRACE, "New auth mode: auto-switch\n");
		break;
	case AUTH_MODE_WPA:
		DBGLOG(RSN, TRACE, "New auth mode: WPA\n");
		break;
	case AUTH_MODE_WPA_PSK:
		DBGLOG(RSN, TRACE, "New auth mode: WPA PSK\n");
		break;
	case AUTH_MODE_WPA_NONE:
		DBGLOG(RSN, TRACE, "New auth mode: WPA None\n");
		break;
	case AUTH_MODE_WPA2:
		DBGLOG(RSN, TRACE, "New auth mode: WPA2\n");
		break;
	case AUTH_MODE_WPA2_PSK:
		DBGLOG(RSN, TRACE, "New auth mode: WPA2 PSK\n");
		break;
	case AUTH_MODE_WPA3_SAE:
		DBGLOG(RSN, INFO, "New auth mode: SAE\n");
		break;
	case AUTH_MODE_FILS:
		DBGLOG(RSN, INFO, "New auth mode: FILS\n");
		break;
	default:
		DBGLOG(RSN, TRACE, "New auth mode: unknown (%d)\n",
		       prConnSettings->eAuthMode);
	}

	return WLAN_STATUS_SUCCESS;
} /* wlanoidSetAuthMode */

uint32_t
wlanoidSetAuthorized(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen) {

	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo = (struct AIS_FSM_INFO *) NULL;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	uint8_t ucBssIndex = 0;
	uint8_t fgConnReqMloSupport = FALSE;
	uint8_t fgEqualMacAddr = FALSE;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < MAC_ADDR_LEN)
		return WLAN_STATUS_INVALID_LENGTH;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	prAisBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prAisBssInfo == NULL)
		return WLAN_STATUS_FAILURE;

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);
	if (prConnSettings == NULL)
		return WLAN_STATUS_FAILURE;

	if (IS_BSS_AIS(prAisBssInfo) &&
		prAisBssInfo->prStaRecOfAP) {

		fgConnReqMloSupport = !!(prConnSettings->u4ConnFlags &
					CONNECT_REQ_MLO_SUPPORT);

		if (fgConnReqMloSupport)
			/* pvSetBuffer is mld addr */
			fgEqualMacAddr = EQUAL_MAC_ADDR(
				cnmStaRecAuthAddr(
					prAdapter, prAisBssInfo->prStaRecOfAP),
					pvSetBuffer);
		else
			/* pvSetBuffer is link addr */
			fgEqualMacAddr = EQUAL_MAC_ADDR(
				prAisBssInfo->prStaRecOfAP->aucMacAddr,
				pvSetBuffer);

		if (!fgEqualMacAddr)
			return WLAN_STATUS_NOT_SUPPORTED;

		rlmReqGenerateOMIIE(prAdapter, prAisBssInfo);

		prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
		if (!timerPendingTimer(&prAisFsmInfo->rJoinTimeoutTimer)) {
			DBGLOG(QM, ERROR, "No channel occupation\n");
		} else {
			DBGLOG(QM, INFO, "Authorized, stop join timer.\n");
			cnmTimerStopTimer(prAdapter,
				&prAisFsmInfo->rJoinTimeoutTimer);
			aisFsmRunEventJoinTimeout(prAdapter, ucBssIndex);
		}

		if (prAisBssInfo->eConnectionState == MEDIA_STATE_CONNECTED)
			/* remove all deauthing AP from blacklist */
			aisRemoveDeauthBlocklist(prAdapter, FALSE);

		return WLAN_STATUS_SUCCESS;
	}
	return WLAN_STATUS_NOT_SUPPORTED;
}

#if 0
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current 802.11 privacy filter
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryPrivacyFilter(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen)
{
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);

	ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(enum ENUM_PARAM_PRIVACY_FILTER);

	if (u4QueryBufferLen < sizeof(enum
				      ENUM_PARAM_PRIVACY_FILTER))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	prWlanInfo = &prAdapter->rWlanInfo;
	*(enum ENUM_PARAM_PRIVACY_FILTER *) pvQueryBuffer =
		prWlanInfo->ePrivacyFilter;

#if DBG
	switch (*(enum ENUM_PARAM_PRIVACY_FILTER *) pvQueryBuffer) {
	case PRIVACY_FILTER_ACCEPT_ALL:
		DBGLOG(REQ, INFO, "Current privacy mode: open mode\n");
		break;

	case PRIVACY_FILTER_8021xWEP:
		DBGLOG(REQ, INFO, "Current privacy mode: filtering mode\n");
		break;

	default:
		DBGLOG(REQ, INFO, "Current auth mode: %d\n",
		       *(enum ENUM_PARAM_AUTH_MODE *) pvQueryBuffer);
	}
#endif
	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryPrivacyFilter */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the IEEE 802.11 privacy filter
 *        to the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_NOT_ACCEPTED
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetPrivacyFilter(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	prGlueInfo = prAdapter->prGlueInfo;
	prWlanInfo = &prAdapter->rWlanInfo;

	*pu4SetInfoLen = sizeof(enum ENUM_PARAM_PRIVACY_FILTER);

	if (u4SetBufferLen < sizeof(enum ENUM_PARAM_PRIVACY_FILTER))
		return WLAN_STATUS_INVALID_LENGTH;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set Authentication mode! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	/* Check if the new authentication mode is valid. */
	if (*(enum ENUM_PARAM_PRIVACY_FILTER *) pvSetBuffer >=
	    PRIVACY_FILTER_NUM) {
		DBGLOG(REQ, TRACE, "Invalid privacy filter %d\n",
		       *(enum ENUM_PARAM_PRIVACY_FILTER *) pvSetBuffer);
		return WLAN_STATUS_INVALID_DATA;
	}

	switch (*(enum ENUM_PARAM_PRIVACY_FILTER *) pvSetBuffer) {
	default:
		break;
	}

	/* Save the new authentication mode. */
	prWlanInfo->ePrivacyFilter =
				*(enum ENUM_PARAM_PRIVACY_FILTER) pvSetBuffer;

	return WLAN_STATUS_SUCCESS;

} /* wlanoidSetPrivacyFilter */
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set a WEP key to the driver.
 *
 * \param[in]  prAdapter Pointer to the Adapter structure.
 * \param[in]  pvSetBuffer A pointer to the buffer that holds the data to be
 *             set.
 * \param[in]  u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
#ifdef LINUX
uint8_t keyBuffer[sizeof(struct PARAM_KEY) +
				16 /* LEGACY_KEY_MAX_LEN */];
uint8_t aucBCAddr[] = BC_MAC_ADDR;
#endif
uint32_t
wlanoidSetAddWep(struct ADAPTER *prAdapter,
		 void *pvSetBuffer, uint32_t u4SetBufferLen,
		 uint32_t *pu4SetInfoLen)
{
#ifndef LINUX
	uint8_t keyBuffer[sizeof(struct PARAM_KEY) +
					16 /* LEGACY_KEY_MAX_LEN */];
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
#endif
	struct PARAM_WEP *prNewWepKey;
	struct PARAM_KEY *prParamKey = (struct PARAM_KEY *)
				       keyBuffer;
	uint32_t u4KeyId, u4SetLen;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	*pu4SetInfoLen = OFFSET_OF(struct PARAM_WEP,
				   aucKeyMaterial);

	if (u4SetBufferLen < OFFSET_OF(struct PARAM_WEP,
				       aucKeyMaterial)) {
		ASSERT(pu4SetInfoLen);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set add WEP! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	prNewWepKey = (struct PARAM_WEP *) pvSetBuffer;

	/* Verify the total buffer for minimum length. */
	if (u4SetBufferLen < OFFSET_OF(struct PARAM_WEP,
	    aucKeyMaterial) + prNewWepKey->u4KeyLength) {
		DBGLOG(REQ, WARN,
		       "Invalid total buffer length (%d) than minimum length (%d)\n",
		       (uint8_t) u4SetBufferLen,
		       (uint8_t) OFFSET_OF(struct PARAM_WEP, aucKeyMaterial));

		*pu4SetInfoLen = OFFSET_OF(struct PARAM_WEP,
					   aucKeyMaterial);
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Verify the key structure length. */
	if (prNewWepKey->u4Length > u4SetBufferLen) {
		DBGLOG(REQ, WARN,
		       "Invalid key structure length (%d) greater than total buffer length (%d)\n",
		       (uint8_t) prNewWepKey->u4Length,
		       (uint8_t) u4SetBufferLen);

		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Verify the key material length for maximum key material length:16 */
	if (prNewWepKey->u4KeyLength >
	    16 /* LEGACY_KEY_MAX_LEN */) {
		DBGLOG(REQ, WARN,
		       "Invalid key material length (%d) greater than maximum key material length (16)\n",
		       (uint8_t) prNewWepKey->u4KeyLength);

		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_DATA;
	}

	*pu4SetInfoLen = u4SetBufferLen;

	u4KeyId = prNewWepKey->u4KeyIndex & BITS(0,
			29) /* WEP_KEY_ID_FIELD */;

	/* Verify whether key index is valid or not, current version
	 *  driver support only 4 global WEP keys setting by this OID
	 */
	if (u4KeyId > MAX_KEY_NUM - 1) {
		DBGLOG(REQ, ERROR, "Error, invalid WEP key ID: %d\n",
		       (uint8_t) u4KeyId);
		return WLAN_STATUS_INVALID_DATA;
	}

	prParamKey->u4KeyIndex = u4KeyId;

	/* Transmit key */
	if (prNewWepKey->u4KeyIndex & IS_TRANSMIT_KEY)
		prParamKey->u4KeyIndex |= IS_TRANSMIT_KEY;

	/* Per client key */
	if (prNewWepKey->u4KeyIndex & IS_UNICAST_KEY)
		prParamKey->u4KeyIndex |= IS_UNICAST_KEY;

	prParamKey->u4KeyLength = prNewWepKey->u4KeyLength;

	kalMemCopy(prParamKey->arBSSID, aucBCAddr, MAC_ADDR_LEN);

	kalMemCopy(prParamKey->aucKeyMaterial,
		   prNewWepKey->aucKeyMaterial, prNewWepKey->u4KeyLength);

	prParamKey->ucBssIdx = ucBssIndex;

	if (prParamKey->u4KeyLength == WEP_40_LEN)
		prParamKey->ucCipher = CIPHER_SUITE_WEP40;
	else if (prParamKey->u4KeyLength == WEP_104_LEN)
		prParamKey->ucCipher = CIPHER_SUITE_WEP104;
	else if (prParamKey->u4KeyLength == WEP_128_LEN)
		prParamKey->ucCipher = CIPHER_SUITE_WEP128;

	prParamKey->u4Length = OFFSET_OF(
		struct PARAM_KEY, aucKeyMaterial) + prNewWepKey->u4KeyLength;

	wlanoidSetAddKey(prAdapter, (void *) prParamKey,
			 prParamKey->u4Length, &u4SetLen);

	return WLAN_STATUS_PENDING;
} /* wlanoidSetAddWep */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to request the driver to remove the WEP key
 *          at the specified key index.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetRemoveWep(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen)
{
	uint32_t u4KeyId, u4SetLen;
	struct PARAM_REMOVE_KEY rRemoveKey;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	*pu4SetInfoLen = sizeof(uint32_t);

	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	u4KeyId = *(uint32_t *) pvSetBuffer;

	/* Dump PARAM_WEP content. */
	DBGLOG(REQ, INFO, "Set: Dump PARAM_KEY_INDEX content\n");
	DBGLOG(REQ, INFO, "Index : %u\n", u4KeyId);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set remove WEP! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (u4KeyId & IS_TRANSMIT_KEY) {
		/* Bit 31 should not be set */
		DBGLOG(REQ, ERROR, "Invalid WEP key index: %u\n", u4KeyId);
		return WLAN_STATUS_INVALID_DATA;
	}

	u4KeyId &= BITS(0, 7);

	/* Verify whether key index is valid or not. Current version
	 *  driver support only 4 global WEP keys.
	 */
	if (u4KeyId > MAX_KEY_NUM - 1) {
		DBGLOG(REQ, ERROR, "invalid WEP key ID %u\n", u4KeyId);
		return WLAN_STATUS_INVALID_DATA;
	}

	rRemoveKey.u4Length = sizeof(struct PARAM_REMOVE_KEY);
	rRemoveKey.u4KeyIndex = *(uint32_t *) pvSetBuffer;
	rRemoveKey.ucBssIdx = ucBssIndex;
	kalMemCopy(rRemoveKey.arBSSID, aucBCAddr, MAC_ADDR_LEN);

	wlanoidSetRemoveKey(prAdapter, (void *)&rRemoveKey,
			    sizeof(struct PARAM_REMOVE_KEY), &u4SetLen);

	return WLAN_STATUS_PENDING;
} /* wlanoidSetRemoveWep */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidPresetLinkId(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	uint32_t *pParam = NULL;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	*pu4SetInfoLen = sizeof(uint32_t);

	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	pParam = (uint32_t *) pvSetBuffer;
	prAdapter->rWifiVar.ucPresetLinkId = *pParam;

	DBGLOG(ML, INFO, "Preset link id %d\n", *pParam);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_SUPPORT_802_11W
static void assignPmfFlag(struct STA_RECORD *prStaRec,
	struct BSS_INFO *prBssInfo,
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo,
	struct CMD_802_11_KEY *prCmdKey)
{
	if (IS_BSS_AIS(prBssInfo)) {
		prCmdKey->ucMgmtProtection =
			prAisSpecBssInfo->fgMgmtProtection;
		DBGLOG(RSN, INFO,
			"Ais PMF flag = %d\n",
			prAisSpecBssInfo->fgMgmtProtection);
	} else {
		/* AP PMF */
		DBGLOG_LIMITED(RSN, INFO,
			"Client PMF flag = %d\n",
			prStaRec->rPmfCfg.fgApplyPmf);
		prCmdKey->ucMgmtProtection =
			prStaRec->rPmfCfg.fgApplyPmf;
	}
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set a key to the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 *
 * \note The setting buffer PARAM_KEY_T, which is set by NDIS, is unpacked.
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanSetAddKeyImpl(struct ADAPTER *prAdapter, void *pvSetBuffer,
		uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen,
		uint8_t fgIsOID)
{
	struct PARAM_KEY *prNewKey;
	struct CMD_802_11_KEY rCmdKey;
	struct CMD_802_11_KEY *prCmdKey = &rCmdKey;
	struct BSS_INFO *prBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	u_int8_t fgNoHandshakeSec = FALSE;
#if CFG_SUPPORT_TDLS
	struct STA_RECORD *prTmpStaRec;
#endif

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);
	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(RSN, WARN,
			"Fail in set add key! (Adapter not ready). ACPI=D%d, Radio=%d\n",
			prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}
	prNewKey = (struct PARAM_KEY *) pvSetBuffer;
	/* Verify the key structure length. */
	if (prNewKey->u4Length > u4SetBufferLen) {
		DBGLOG_LIMITED(RSN, WARN,
		       "Invalid key structure length (%d) greater than total buffer length (%d)\n",
		       (uint8_t) prNewKey->u4Length, (uint8_t) u4SetBufferLen);
		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_LENGTH;
	}
	/* Verify the key material length for key material buffer */
	if (prNewKey->u4KeyLength > prNewKey->u4Length -
	    OFFSET_OF(struct PARAM_KEY, aucKeyMaterial)) {
		DBGLOG_LIMITED(RSN, WARN, "Invalid key material length (%d)\n",
			(uint8_t) prNewKey->u4KeyLength);
		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Exception check */
	if (prNewKey->u4KeyIndex & 0x0fffff00)
		return WLAN_STATUS_INVALID_DATA;
	/* Exception check, pairwise key must with transmit bit enabled */
	if ((prNewKey->u4KeyIndex & BITS(30, 31)) == IS_UNICAST_KEY)
		return WLAN_STATUS_INVALID_DATA;
	if (!(prNewKey->u4KeyLength == WEP_40_LEN ||
	    prNewKey->u4KeyLength == WEP_104_LEN ||
	    prNewKey->u4KeyLength == CCMP_KEY_LEN ||
	    prNewKey->u4KeyLength == TKIP_KEY_LEN)) {
		return WLAN_STATUS_INVALID_DATA;
	}
	/* Exception check, pairwise key must with transmit bit enabled */
	if ((prNewKey->u4KeyIndex & BITS(30, 31)) == BITS(30, 31)) {
		if (/* ((prNewKey->u4KeyIndex & 0xff) != 0) || */
		    ((prNewKey->arBSSID[0] == 0xff) &&
		     (prNewKey->arBSSID[1] == 0xff) &&
		     (prNewKey->arBSSID[2] == 0xff) &&
		     (prNewKey->arBSSID[3] == 0xff) &&
		     (prNewKey->arBSSID[4] == 0xff) &&
		     (prNewKey->arBSSID[5] == 0xff))) {
			return WLAN_STATUS_INVALID_DATA;
		}
	}
	*pu4SetInfoLen = u4SetBufferLen;

	/* Dump PARAM_KEY content. */
	DBGLOG(RSN, TRACE,
		"Set: Dump PARAM_KEY content, Len: 0x%08x, BSSID: "
		MACSTR
		", KeyIdx: 0x%08x, KeyLen: 0x%08x, Cipher: %d, Material:\n",
		prNewKey->u4Length, MAC2STR(prNewKey->arBSSID),
		prNewKey->u4KeyIndex, prNewKey->u4KeyLength,
		prNewKey->ucCipher);
	DBGLOG_MEM8(RSN, TRACE, prNewKey->aucKeyMaterial,
		    prNewKey->u4KeyLength);
	DBGLOG(RSN, TRACE, "Key RSC:\n");
	DBGLOG_MEM8(RSN, TRACE, &prNewKey->rKeyRSC, sizeof(uint64_t));

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prNewKey->ucBssIdx);
	if (!prBssInfo) {
		DBGLOG_LIMITED(REQ, INFO, "BSS Info not exist !!\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (IS_BSS_AIS(prBssInfo)) {
		prAisSpecBssInfo = aisGetAisSpecBssInfo(
			prAdapter, prNewKey->ucBssIdx);
	}
	/*         Tx  Rx KeyType addr
	 *  STA, GC:
	 *  case1: 1   1   0  BC addr (no sta record of AP at this moment)  WEP,
	 *  notice: tx at default key setting WEP key now save to BSS_INFO
	 *  case2: 0   1   0  BSSID (sta record of AP)        RSN BC key
	 *  case3: 1   1   1  AP addr (sta record of AP)      RSN STA key
	 *
	 *  GO:
	 *  case1: 1   1   0  BSSID (no sta record)           WEP -- Not support
	 *  case2: 1   0   0  BSSID (no sta record)           RSN BC key
	 *  case3: 1   1   1  STA addr                        STA key
	 */
	if (prNewKey->ucCipher == CIPHER_SUITE_WEP40 ||
	    prNewKey->ucCipher == CIPHER_SUITE_WEP104 ||
	    prNewKey->ucCipher == CIPHER_SUITE_WEP128) {
		/* check if the key no need handshake, then save to bss wep key
		 * for global usage
		 */
		fgNoHandshakeSec = TRUE;
	}
	if (fgNoHandshakeSec) {
#if DBG
		if (IS_BSS_AIS(prBssInfo)) {
			if (aisGetAuthMode(prAdapter, prNewKey->ucBssIdx)
			    >= AUTH_MODE_WPA &&
			    aisGetAuthMode(prAdapter, prNewKey->ucBssIdx) !=
			    AUTH_MODE_WPA_NONE) {
				DBGLOG_LIMITED(RSN, WARN,
					"Set wep at not open/shared setting\n");
				return WLAN_STATUS_SUCCESS;
			}
		}
#endif
	}
	if ((prNewKey->u4KeyIndex & IS_UNICAST_KEY) == IS_UNICAST_KEY) {
		prStaRec = cnmGetStaRecByAddress(prAdapter,
				prBssInfo->ucBssIndex, prNewKey->arBSSID);
		if (!prStaRec) {	/* Already disconnected ? */
			DBGLOG_LIMITED(REQ, INFO,
				"[wlan] Not set the peer key while disconnect\n");
			return WLAN_STATUS_SUCCESS;
		}
#if CFG_SUPPORT_FRAG_AGG_VALIDATION
		/* clear fragment cache when rekey */
		nicRxClearFrag(prAdapter, prStaRec);
#endif /* CFG_SUPPORT_FRAG_AGG_VALIDATION */
	}

	kalMemZero(prCmdKey, sizeof(struct CMD_802_11_KEY));
	prCmdKey->ucAddRemove = 1; /* Add */
	prCmdKey->ucTxKey =
		((prNewKey->u4KeyIndex & IS_TRANSMIT_KEY) == IS_TRANSMIT_KEY)
		? 1 : 0;
	prCmdKey->ucKeyType =
		((prNewKey->u4KeyIndex & IS_UNICAST_KEY) == IS_UNICAST_KEY)
		? 1 : 0;
	prCmdKey->ucIsAuthenticator =
		((prNewKey->u4KeyIndex & IS_AUTHENTICATOR) == IS_AUTHENTICATOR)
		? 1 : 0;
	/* Copy the addr of the key */
	if ((prNewKey->u4KeyIndex & IS_UNICAST_KEY) == IS_UNICAST_KEY) {
		if (prStaRec) {
			/* Overwrite the fgNoHandshakeSec in case */
			fgNoHandshakeSec = FALSE; /* Legacy 802.1x wep case ? */
			/* ASSERT(FALSE); */
		}
	} else {
		if (!IS_BSS_ACTIVE(prBssInfo))
			DBGLOG_LIMITED(REQ, INFO,
				"[wlan] BSS info (%d) not active yet!",
				prNewKey->ucBssIdx);
	}
	prCmdKey->ucBssIdx = prBssInfo->ucBssIndex;
	prCmdKey->ucKeyId = (uint8_t) (prNewKey->u4KeyIndex & 0xff);
	/* Note: the key length may not correct for WPA-None */
	prCmdKey->ucKeyLen = (uint8_t) prNewKey->u4KeyLength;
	kalMemCopy(prCmdKey->aucKeyMaterial,
		(uint8_t *)prNewKey->aucKeyMaterial, prCmdKey->ucKeyLen);
	if (prNewKey->ucCipher) {
		prCmdKey->ucAlgorithmId = prNewKey->ucCipher;
		if (IS_BSS_AIS(prBssInfo)) {
#if CFG_SUPPORT_802_11W
			if (prCmdKey->ucAlgorithmId == CIPHER_SUITE_BIP ||
			    prCmdKey->ucAlgorithmId ==
					CIPHER_SUITE_BIP_GMAC_256) {
				if (prCmdKey->ucKeyId >= 4 &&
				    prCmdKey->ucKeyId <= 5) {
					prAisSpecBssInfo->fgBipKeyInstalled =
						TRUE;

#if (CFG_WIFI_IGTK_GTK_SEPARATE == 1)
					DBGLOG(RSN, INFO,
						"Change BIP BC keyId from %d to 3\n",
						prCmdKey->ucKeyId);
					/* Reserve keyid 3 for IGTK */
					prCmdKey->ucKeyId = 3;
#endif
				}
			}

			if (prCmdKey->ucAlgorithmId ==
					CIPHER_SUITE_BIP_GMAC_256) {
				prAisSpecBssInfo->fgBipGmacKeyInstalled = TRUE;
				/* save IGTK and IPN for SW BIP handling */
				kalMemCopy(prAisSpecBssInfo->aucIPN,
					prNewKey->aucKeyPn, 6);
				kalMemCopy(prAisSpecBssInfo->aucIGTK,
					prNewKey->aucKeyMaterial,
					prNewKey->u4KeyLength);
				DBGLOG_MEM8(RSN, INFO,
					prAisSpecBssInfo->aucIPN, 6);
				DBGLOG_MEM8(RSN, INFO,
					prAisSpecBssInfo->aucIGTK,
					prCmdKey->ucKeyLen);
			}
#endif
			if (prCmdKey->ucAlgorithmId == CIPHER_SUITE_TKIP) {
				/* Todo:: Support AP mode defragment */
				/* for pairwise key only */
				if ((prNewKey->u4KeyIndex & BITS(30, 31)) ==
				    ((IS_UNICAST_KEY) | (IS_TRANSMIT_KEY))) {
					kalMemCopy(
					  prAisSpecBssInfo->aucRxMicKey,
					  &prCmdKey->aucKeyMaterial[16],
					  MIC_KEY_LEN);
					kalMemCopy(
					  prAisSpecBssInfo->aucTxMicKey,
					  &prCmdKey->aucKeyMaterial[24],
					  MIC_KEY_LEN);
				}
			}
			/* BIGTK */
			if (prCmdKey->ucKeyId >= 6 && prCmdKey->ucKeyId <= 7) {
				prBssInfo->ucBcnProtInstalled[prCmdKey->ucKeyId]
					= TRUE;
			}
		} else {
#if CFG_SUPPORT_802_11W
			/* AP PMF */
			if ((prCmdKey->ucKeyId >= 4 && prCmdKey->ucKeyId <= 5)
			    && (prCmdKey->ucAlgorithmId == CIPHER_SUITE_BIP ||
				prCmdKey->ucAlgorithmId ==
						CIPHER_SUITE_BIP_GMAC_256)) {
				DBGLOG_LIMITED(RSN, INFO, "AP mode set BIP\n");
				prBssInfo->rApPmfCfg.fgBipKeyInstalled = TRUE;
#if (CFG_WIFI_IGTK_GTK_SEPARATE == 1)
				DBGLOG(RSN, INFO,
					"Change BIP BC keyId from %d to 3\n",
					prCmdKey->ucKeyId);
				/* Reserve keyid 3 for IGTK */
				prCmdKey->ucKeyId = 3;
#endif
			}
#endif
		}
	} else { /* Legacy windows NDIS no cipher info */
#if 0
		if (prNewKey->u4KeyLength == 5) {
			prCmdKey->ucAlgorithmId = CIPHER_SUITE_WEP40;
		} else if (prNewKey->u4KeyLength == 13) {
			prCmdKey->ucAlgorithmId = CIPHER_SUITE_WEP104;
		} else if (prNewKey->u4KeyLength == 16) {
			if (prAdapter->rWifiVar.rConnSettings.eAuthMode <
			    AUTH_MODE_WPA)
				prCmdKey->ucAlgorithmId = CIPHER_SUITE_WEP128;
			else {
				if (IS_BSS_AIS(prBssInfo)) {
#if CFG_SUPPORT_802_11W
					if (prCmdKey->ucKeyId >= 4) {
						struct AIS_SPECIFIC_BSS_INFO
							*prAisSpecBssInfo;

						prCmdKey->ucAlgorithmId =
							CIPHER_SUITE_BIP;
						prAisSpecBssInfo =
							&prAdapter->rWifiVar
							.rAisSpecificBssInfo;
						prAisSpecBssInfo
							->fgBipKeyInstalled =
							TRUE;
					} else
#endif
					{
				prCmdKey->ucAlgorithmId = CIPHER_SUITE_CCMP;
				if (rsnCheckPmkidCandicate(prAdapter)) {
					DBGLOG(RSN, TRACE,
					  "Add key: Prepare a timer to indicate candidate PMKID\n");
					cnmTimerStopTimer(prAdapter,
					  &prAisSpecBssInfo
						->rPreauthenticationTimer);
					cnmTimerStartTimer(prAdapter,
					  &prAisSpecBssInfo
						->rPreauthenticationTimer,
					SEC_TO_MSEC(
					  WAIT_TIME_IND_PMKID_CANDICATE_SEC));
					}
				}
					}
			}
		} else if (prNewKey->u4KeyLength == 32) {
			if (IS_BSS_AIS(prBssInfo)) {
				if (prAdapter->rWifiVar.rConnSettings.eAuthMode
				    == AUTH_MODE_WPA_NONE) {
					if (prAdapter->rWifiVar.rConnSettings
						.eEncStatus ==
						ENUM_ENCRYPTION2_ENABLED) {
						prCmdKey->ucAlgorithmId =
							CIPHER_SUITE_TKIP;
					} else if (prAdapter->rWifiVar
						.rConnSettings.eEncStatus ==
						ENUM_ENCRYPTION3_ENABLED) {
						prCmdKey->ucAlgorithmId =
							CIPHER_SUITE_CCMP;
						prCmdKey->ucKeyLen =
							CCMP_KEY_LEN;
					}
				} else {
					prCmdKey->ucAlgorithmId =
						CIPHER_SUITE_TKIP;
					kalMemCopy(
						prAdapter->rWifiVar
							.rAisSpecificBssInfo
							.aucRxMicKey,
						&prCmdKey->aucKeyMaterial[16],
						MIC_KEY_LEN);
					kalMemCopy(
						prAdapter->rWifiVar
							.rAisSpecificBssInfo
							.aucTxMicKey,
						&prCmdKey->aucKeyMaterial[24],
						MIC_KEY_LEN);
			if (0 /* Todo::GCMP & GCMP-BIP ? */) {
				if (rsnCheckPmkidCandicate(prAdapter)) {
					DBGLOG(RSN, TRACE,
					  "Add key: Prepare a timer to indicate candidate PMKID\n");
					cnmTimerStopTimer(prAdapter,
					  &prAisSpecBssInfo->
						rPreauthenticationTimer);
					cnmTimerStartTimer(prAdapter,
					  &prAisSpecBssInfo->
						rPreauthenticationTimer,
					  SEC_TO_MSEC(
					    WAIT_TIME_IND_PMKID_CANDICATE_SEC));
				}
			} else {
				prCmdKey->ucAlgorithmId = CIPHER_SUITE_TKIP;
			}
		}
}
#endif
	}
	{
#if CFG_SUPPORT_TDLS
		prTmpStaRec = cnmGetStaRecByAddress(prAdapter,
				prBssInfo->ucBssIndex, prNewKey->arBSSID);
		if (prTmpStaRec) {
			if (IS_DLS_STA(prTmpStaRec)) {
				prStaRec = prTmpStaRec;

				/*128 ,TODO  GCMP 256 */
				prCmdKey->ucAlgorithmId = CIPHER_SUITE_CCMP;

				kalMemCopy(prCmdKey->aucPeerAddr,
					prStaRec->aucMacAddr, MAC_ADDR_LEN);
			}
		}
#endif

#if CFG_SUPPORT_802_11W
		/* AP PMF */
		if (prCmdKey->ucAlgorithmId == CIPHER_SUITE_BIP ||
		    prCmdKey->ucAlgorithmId == CIPHER_SUITE_BIP_GMAC_256) {
			if (prCmdKey->ucIsAuthenticator) {
				DBGLOG_LIMITED(RSN, INFO,
				"Authenticator BIP bssid:%d\n",
				prBssInfo->ucBssIndex);

				prCmdKey->ucWlanIndex =
					secPrivacySeekForBcEntry(prAdapter,
						prBssInfo->ucBssIndex,
						prBssInfo->aucOwnMacAddr,
						STA_REC_INDEX_NOT_FOUND,
						prCmdKey->ucAlgorithmId,
						prCmdKey->ucKeyId);
			} else {
				if (prBssInfo->prStaRecOfAP) {
					prCmdKey->ucWlanIndex =
					    secPrivacySeekForBcEntry(prAdapter,
						    prBssInfo->ucBssIndex,
						    prBssInfo->prStaRecOfAP
							->aucMacAddr,
						    prBssInfo->prStaRecOfAP
							->ucIndex,
						    prCmdKey->ucAlgorithmId,
						    prCmdKey->ucKeyId);
					kalMemCopy(prCmdKey->aucPeerAddr,
						prBssInfo->prStaRecOfAP
						->aucMacAddr, MAC_ADDR_LEN);

#if (CFG_WIFI_IGTK_GTK_SEPARATE == 1)
					prBssInfo->ucBMCWlanIndexS[
						prCmdKey->ucKeyId] =
						prCmdKey->ucWlanIndex;
					prBssInfo->ucBMCWlanIndexSUsed[
						prCmdKey->ucKeyId] = TRUE;
					DBGLOG_LIMITED(RSN, INFO,
					       "BMCWlanIndex kid = %d, index = %d\n",
					       prCmdKey->ucKeyId,
					       prCmdKey->ucWlanIndex);
#endif
				}
			}

			DBGLOG_LIMITED(RSN, INFO, "BIP BC wtbl index:%d\n",
				prCmdKey->ucWlanIndex);
		} else
#endif
		if (1) {
			if (prStaRec) {
				if (prCmdKey->ucKeyType) {	/* RSN STA */
					struct WLAN_TABLE *prWtbl;

					prWtbl = prAdapter->rWifiVar.arWtbl;
					prWtbl[prStaRec->ucWlanIndex].ucKeyId =
						prCmdKey->ucKeyId;
					prCmdKey->ucWlanIndex =
						prStaRec->ucWlanIndex;

					/* wait for CMD Done ? */
					prStaRec->fgTransmitKeyExist = TRUE;

					kalMemCopy(prCmdKey->aucPeerAddr,
						prNewKey->arBSSID,
						MAC_ADDR_LEN);
#if CFG_SUPPORT_802_11W
					assignPmfFlag(prStaRec,
						prBssInfo,
						prAisSpecBssInfo,
						prCmdKey);
#endif
				} else {
					ASSERT(FALSE);
				}
			} else { /* Overwrite the old one for AP and STA WEP */
				if (prBssInfo->prStaRecOfAP) {
					DBGLOG_LIMITED(RSN, INFO, "AP REC\n");
					prCmdKey->ucWlanIndex =
					    secPrivacySeekForBcEntry(
						prAdapter,
						prBssInfo->ucBssIndex,
						prBssInfo->prStaRecOfAP
						    ->aucMacAddr,
						prBssInfo->prStaRecOfAP
						    ->ucIndex,
						prCmdKey->ucAlgorithmId,
						prCmdKey->ucKeyId);
					kalMemCopy(prCmdKey->aucPeerAddr,
						   prBssInfo->prStaRecOfAP
						   ->aucMacAddr,
						   MAC_ADDR_LEN);
				} else {
					DBGLOG_LIMITED(RSN, INFO,
						"!AP && !STA REC\n");
					prCmdKey->ucWlanIndex =
						secPrivacySeekForBcEntry(
						prAdapter,
						prBssInfo->ucBssIndex,
						prBssInfo->aucOwnMacAddr,
						STA_REC_INDEX_NOT_FOUND,
						prCmdKey->ucAlgorithmId,
						prCmdKey->ucKeyId);
					kalMemCopy(prCmdKey->aucPeerAddr,
						prBssInfo->aucOwnMacAddr,
						MAC_ADDR_LEN);
				}
				if (prCmdKey->ucKeyId >= MAX_KEY_NUM) {
					DBGLOG_LIMITED(RSN, ERROR,
						"prCmdKey->ucKeyId [%u] overrun\n",
						prCmdKey->ucKeyId);
					return WLAN_STATUS_FAILURE;
				}
				if (fgNoHandshakeSec) {
					/* WEP: STA and AP */
					prBssInfo->wepkeyWlanIdx =
						prCmdKey->ucWlanIndex;
					prBssInfo->wepkeyUsed[
						prCmdKey->ucKeyId] = TRUE;
				} else if (!prBssInfo->prStaRecOfAP) {
					/* AP WPA/RSN */
					prBssInfo->ucBMCWlanIndexS[
						prCmdKey->ucKeyId] =
						prCmdKey->ucWlanIndex;
					prBssInfo->ucBMCWlanIndexSUsed[
						prCmdKey->ucKeyId] = TRUE;
				} else {
					/* STA WPA/RSN, should not have tx but
					 * no sta record
					 */
					prBssInfo->ucBMCWlanIndexS[
						prCmdKey->ucKeyId] =
						prCmdKey->ucWlanIndex;
					prBssInfo->ucBMCWlanIndexSUsed[
						prCmdKey->ucKeyId] = TRUE;
					DBGLOG_LIMITED(RSN, INFO,
					       "BMCWlanIndex kid = %d, index = %d\n",
					       prCmdKey->ucKeyId,
					       prCmdKey->ucWlanIndex);
				}
				if (prCmdKey->ucTxKey) { /* */
					prBssInfo->fgBcDefaultKeyExist = TRUE;
					prBssInfo->ucBcDefaultKeyIdx =
							prCmdKey->ucKeyId;
				}
			}
		}
	}
#if 1
	DBGLOG(RSN, INFO, "Add key to wlanIdx %d,BSS=%d," MACSTR
		       "Tx=%d,type=%d,Auth=%d,cipher=%d,keyid=%d,keylen=%d\n",
		       prCmdKey->ucWlanIndex, prCmdKey->ucBssIdx,
		       MAC2STR(prCmdKey->aucPeerAddr), prCmdKey->ucTxKey,
		       prCmdKey->ucKeyType, prCmdKey->ucIsAuthenticator,
		       prCmdKey->ucAlgorithmId, prCmdKey->ucKeyId,
		       prCmdKey->ucKeyLen);
	DBGLOG_MEM8(RSN, TRACE, prCmdKey->aucKeyMaterial, prCmdKey->ucKeyLen);
	if (prCmdKey->ucKeyId < MAX_KEY_NUM) {
		DBGLOG_LIMITED(RSN, INFO, "wepkeyUsed=%d,wepkeyWlanIdx=%d\n",
		       prBssInfo->wepkeyUsed[prCmdKey->ucKeyId],
		       prBssInfo->wepkeyWlanIdx);

		DBGLOG(RSN, INFO,
		       "ucBMCWlanIndexSUsed=%d,ucBMCWlanIndexS=%d,ucBcnProtInstalled=%d\n",
		       prBssInfo->ucBMCWlanIndexSUsed[prCmdKey->ucKeyId],
		       prBssInfo->ucBMCWlanIndexS[prCmdKey->ucKeyId],
		       prBssInfo->ucBcnProtInstalled[prCmdKey->ucKeyId]);
	}
#endif
	if (prAisSpecBssInfo)
		prAisSpecBssInfo->ucKeyAlgorithmId = prCmdKey->ucAlgorithmId;

	return wlanSendSetQueryCmd(prAdapter,
				  CMD_ID_ADD_REMOVE_KEY,
				  TRUE,
				  FALSE,
				  fgIsOID,
#if CFG_SUPPORT_REPLAY_DETECTION
				  nicCmdEventSetAddKey,
				  nicOidCmdTimeoutSetAddKey,
#else
				  nicCmdEventSetCommon,
				  nicOidCmdTimeoutCommon,
#endif
				  sizeof(struct CMD_802_11_KEY),
				  (uint8_t *)&rCmdKey,
				  pvSetBuffer,
				  u4SetBufferLen);
} /* wlanoidSetAddKey */

uint32_t
wlanSetAddKey(struct ADAPTER *prAdapter, void *pvSetBuffer,
		 uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen,
		 uint8_t fgIsOID)
{
	struct PARAM_KEY *prNewKey;
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	uint32_t ret = WLAN_STATUS_SUCCESS;
	struct MLD_STA_RECORD *prMldStaRec = NULL;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	uint8_t ucLinkId;
#endif

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(RSN, WARN,
			"Fail in set add key! (Adapter not ready). ACPI=D%d, Radio=%d\n",
			prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}
	prNewKey = (struct PARAM_KEY *) pvSetBuffer;
	/* Verify the key structure length. */
	if (prNewKey->u4Length > u4SetBufferLen) {
		DBGLOG_LIMITED(RSN, WARN,
		       "Invalid key structure length (%d) greater than total buffer length (%d)\n",
		       (uint8_t) prNewKey->u4Length, (uint8_t) u4SetBufferLen);
		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_LENGTH;
	}
	/* Verify the key material length for key material buffer */
	if (prNewKey->u4KeyLength > prNewKey->u4Length -
	    OFFSET_OF(struct PARAM_KEY, aucKeyMaterial)) {
		DBGLOG_LIMITED(RSN, WARN, "Invalid key material length (%d)\n",
			(uint8_t) prNewKey->u4KeyLength);
		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_DATA;
	}


	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prNewKey->ucBssIdx);
	if (!prBssInfo) {
		DBGLOG_LIMITED(REQ, INFO, "BSS Info not exist !!\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* Dump PARAM_KEY content. */
	DBGLOG(RSN, TRACE,
		"Set: Dump PARAM_KEY content, Len: 0x%08x, BSSID: "
		MACSTR
		", KeyIdx: 0x%08x, KeyLen: 0x%08x, Cipher: %d, Material:\n",
		prNewKey->u4Length, MAC2STR(prNewKey->arBSSID),
		prNewKey->u4KeyIndex, prNewKey->u4KeyLength,
		prNewKey->ucCipher);
	DBGLOG_MEM8(RSN, TRACE, prNewKey->aucKeyMaterial,
		    prNewKey->u4KeyLength);
	DBGLOG(RSN, TRACE, "Key RSC:\n");
	DBGLOG_MEM8(RSN, TRACE, &prNewKey->rKeyRSC, sizeof(uint64_t));

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	/* KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE */
	if (prNewKey->i4LinkId != MLD_LINK_ID_NONE) {
		prMldStaRec = mldStarecGetByMldAddr(prAdapter,
			prMldBssInfo, prNewKey->arBSSID);
		ucLinkId = prNewKey->i4LinkId == -1 ?
			MLD_LINK_ID_NONE : prNewKey->i4LinkId;
	} else { /* old kernel version */
		prMldStaRec = mldStarecGetByLinkAddr(prAdapter,
			prMldBssInfo, prNewKey->arBSSID);
		ucLinkId = prAdapter->rWifiVar.ucPresetLinkId;
		prAdapter->rWifiVar.ucPresetLinkId = MLD_LINK_ID_NONE;
	}
	if (prMldStaRec && prNewKey->u4KeyIndex & IS_UNICAST_KEY) {
		struct STA_RECORD *sta;

		LINK_FOR_EACH_ENTRY(sta, &prMldStaRec->rStarecList,
					rLinkEntryMld, struct STA_RECORD) {
			/* overwrite key info by link */
			prNewKey->ucBssIdx = sta->ucBssIndex;
			COPY_MAC_ADDR(prNewKey->arBSSID, sta->aucMacAddr);

			/* set oid is true only for the last cmd
			 * otherwise oid may complete wrongly
			 */
			ret = wlanSetAddKeyImpl(prAdapter, pvSetBuffer,
				u4SetBufferLen, pu4SetInfoLen, fgIsOID &&
				sta == LINK_PEEK_TAIL(&prMldStaRec->rStarecList,
				struct STA_RECORD, rLinkEntryMld));
			if (ret != WLAN_STATUS_SUCCESS &&
			    ret != WLAN_STATUS_PENDING)
				return ret;
		}
	} else if (IS_MLD_BSSINFO_MULTI(prMldBssInfo) &&
		ucLinkId != MLD_LINK_ID_NONE) {
		struct BSS_INFO *bss;

		LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList,
					rLinkEntryMld, struct BSS_INFO) {
			if (ucLinkId == bss->ucLinkIndex) {
				/* overwrite key info by link */
				prNewKey->ucBssIdx = bss->ucBssIndex;

				ret = wlanSetAddKeyImpl(prAdapter,
					pvSetBuffer, u4SetBufferLen,
					pu4SetInfoLen, fgIsOID);
				if (ret != WLAN_STATUS_SUCCESS &&
				    ret != WLAN_STATUS_PENDING)
					return ret;
			}
		}
	} else {
		ret = wlanSetAddKeyImpl(prAdapter, pvSetBuffer,
				u4SetBufferLen, pu4SetInfoLen, fgIsOID);
	}

	return ret;
#else /*  (CFG_SUPPORT_802_11BE_MLO == 1) */
	return wlanSetAddKeyImpl(prAdapter, pvSetBuffer,
				u4SetBufferLen, pu4SetInfoLen, fgIsOID);
#endif /*  (CFG_SUPPORT_802_11BE_MLO == 1) */
}

uint32_t
wlanoidSetAddKey(struct ADAPTER *prAdapter, void *pvSetBuffer,
		 uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	return wlanSetAddKey(prAdapter, pvSetBuffer,
		u4SetBufferLen, pu4SetInfoLen, TRUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to request the driver to remove the key at
 *        the specified key index.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetRemoveKey(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen)
{
	return wlanSetRemoveKey(prAdapter, pvSetBuffer, u4SetBufferLen,
				pu4SetInfoLen, TRUE);
}				/* wlanoidSetRemoveKey */

uint32_t
wlanSetRemoveKeyImpl(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen, uint8_t fgIsOid)
{
	struct PARAM_REMOVE_KEY *prRemovedKey;
	struct CMD_802_11_KEY rCmdKey;
	struct STA_RECORD *prStaRec = NULL;
	struct BSS_INFO *prBssInfo;
	/*	UINT_8 i = 0;	*/
	uint8_t fgRemoveWepKey = FALSE;
	uint32_t ucRemoveBCKeyAtIdx = WTBL_RESERVED_ENTRY;
	uint32_t u4KeyIndex;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_REMOVE_KEY);

	if (u4SetBufferLen < sizeof(struct PARAM_REMOVE_KEY))
		return WLAN_STATUS_INVALID_LENGTH;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set remove key! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	ASSERT(pvSetBuffer);
	prRemovedKey = (struct PARAM_REMOVE_KEY *) pvSetBuffer;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prRemovedKey->ucBssIdx);
	ASSERT(prBssInfo);

	u4KeyIndex = prRemovedKey->u4KeyIndex & 0x000000FF;

	ASSERT(u4KeyIndex < MAX_KEY_NUM);

#if (CFG_WIFI_IGTK_GTK_SEPARATE == 0)
	if (u4KeyIndex >= 4 && u4KeyIndex <= 5) {
#if CFG_SUPPORT_802_11W
		if (IS_BSS_AIS(prBssInfo)) {
			struct AIS_SPECIFIC_BSS_INFO *prAisSpecBssInfo =
				aisGetAisSpecBssInfo(prAdapter,
				prRemovedKey->ucBssIdx);

			prAisSpecBssInfo->fgBipKeyInstalled = FALSE;
			prAisSpecBssInfo->fgBipGmacKeyInstalled = FALSE;
		}
#endif
		DBGLOG(RSN, INFO, "Remove bip key Index (IGTK) : 0x%08x\n",
		       u4KeyIndex);
		return WLAN_STATUS_SUCCESS;
	}
#endif

	/* Clean up the Tx key flag */
	if (prRemovedKey->u4KeyIndex & IS_UNICAST_KEY) {
		prStaRec = cnmGetStaRecByAddress(prAdapter,
				prRemovedKey->ucBssIdx, prRemovedKey->arBSSID);
		if (!prStaRec)
			return WLAN_STATUS_SUCCESS;
	} else {
		if (u4KeyIndex == prBssInfo->ucBcDefaultKeyIdx) {
			prBssInfo->fgBcDefaultKeyExist = FALSE;
			prBssInfo->ucBcDefaultKeyIdx = 0xff;
		}
	}

	/* BIGTK */
	if (u4KeyIndex >= 6 && u4KeyIndex <= 7) {
		if (prBssInfo->ucBcnProtInstalled[u4KeyIndex] == TRUE) {
			prBssInfo->ucBcnProtInstalled[u4KeyIndex] = FALSE;
			DBGLOG(RSN, INFO,
				"Remove BIGTK, key id = %d", u4KeyIndex);
		} else
			return WLAN_STATUS_SUCCESS;
	} else if (!prStaRec) {
		if (prBssInfo->wepkeyUsed[u4KeyIndex] == TRUE)
			fgRemoveWepKey = TRUE;

		if (fgRemoveWepKey) {
			DBGLOG(RSN, INFO, "Remove wep key id = %d", u4KeyIndex);
			prBssInfo->wepkeyUsed[u4KeyIndex] = FALSE;
			if (prBssInfo->fgBcDefaultKeyExist &&
			    prBssInfo->ucBcDefaultKeyIdx == u4KeyIndex) {
				prBssInfo->fgBcDefaultKeyExist = FALSE;
				prBssInfo->ucBcDefaultKeyIdx = 0xff;
			}
			ASSERT(prBssInfo->wepkeyWlanIdx < WTBL_SIZE);
			ucRemoveBCKeyAtIdx = prBssInfo->wepkeyWlanIdx;
			secPrivacyFreeForEntry(prAdapter,
					prBssInfo->wepkeyWlanIdx);
			prBssInfo->wepkeyWlanIdx = WTBL_RESERVED_ENTRY;
		} else {
			DBGLOG(RSN, INFO, "Remove group key id = %d",
			       u4KeyIndex);

			if (prBssInfo->ucBMCWlanIndexSUsed[u4KeyIndex]) {
				if (prBssInfo->fgBcDefaultKeyExist &&
				    prBssInfo->ucBcDefaultKeyIdx ==
				    u4KeyIndex) {
					prBssInfo->fgBcDefaultKeyExist = FALSE;
					prBssInfo->ucBcDefaultKeyIdx = 0xff;
				}
				if (u4KeyIndex != 0)
					ASSERT(prBssInfo->ucBMCWlanIndexS[
						u4KeyIndex] < WTBL_SIZE);
				ucRemoveBCKeyAtIdx =
					prBssInfo->ucBMCWlanIndexS[u4KeyIndex];

				secPrivacyFreeForEntry(prAdapter,
				    prBssInfo->ucBMCWlanIndexS[u4KeyIndex]);
				prBssInfo->ucBMCWlanIndexSUsed[u4KeyIndex]
					= FALSE;
				prBssInfo->ucBMCWlanIndexS[u4KeyIndex]
					= WTBL_RESERVED_ENTRY;
			}
		}

		if (ucRemoveBCKeyAtIdx >= WTBL_SIZE)
			return WLAN_STATUS_SUCCESS;
	}

	/* Dump PARAM_REMOVE_KEY content. */
	DBGLOG(RSN, INFO, "PARAM_REMOVE_KEY: BSSID(" MACSTR
		"), BSS_INDEX (%d), Length(0x%08x), Key Index(0x%08x, %d) ucRemoveBCKeyAtIdx = %d\n",
		MAC2STR(prRemovedKey->arBSSID),
		prRemovedKey->ucBssIdx,
		prRemovedKey->u4Length, prRemovedKey->u4KeyIndex, u4KeyIndex,
		ucRemoveBCKeyAtIdx);


	/* compose CMD_802_11_KEY cmd pkt */
	kalMemZero((uint8_t *) &rCmdKey, sizeof(struct CMD_802_11_KEY));

	rCmdKey.ucAddRemove = 0;	/* Remove */
	rCmdKey.ucKeyId = (uint8_t) u4KeyIndex;
	kalMemCopy(rCmdKey.aucPeerAddr,
		   (uint8_t *) prRemovedKey->arBSSID, MAC_ADDR_LEN);
	rCmdKey.ucBssIdx = prRemovedKey->ucBssIdx;

	if (prStaRec) {
		rCmdKey.ucKeyType = 1;
		rCmdKey.ucWlanIndex = prStaRec->ucWlanIndex;
		prStaRec->fgTransmitKeyExist = FALSE;
	} else if (ucRemoveBCKeyAtIdx < WTBL_SIZE) {
		rCmdKey.ucWlanIndex = ucRemoveBCKeyAtIdx;
	} else if (u4KeyIndex >= 6 && u4KeyIndex <= 7) {
		/* BIGTK */
	} else {
		ASSERT(FALSE);
	}

	return wlanSendSetQueryCmd(
			  prAdapter,
			  CMD_ID_ADD_REMOVE_KEY,
			  TRUE,
			  FALSE,
			  fgIsOid,
			  nicCmdEventSetCommon,
			  nicOidCmdTimeoutCommon,
			  sizeof(struct CMD_802_11_KEY),
			  (uint8_t *)&rCmdKey,
			  pvSetBuffer,
			  u4SetBufferLen);
}

uint32_t
wlanSetRemoveKey(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen, uint8_t fgIsOid)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct PARAM_REMOVE_KEY *prRemovedKey;
	struct BSS_INFO *prBssInfo;
	uint32_t ret = WLAN_STATUS_SUCCESS;
	struct MLD_STA_RECORD *prMldStaRec = NULL;
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
	uint8_t ucLinkId;
	uint32_t u4KeyIndex;

	*pu4SetInfoLen = sizeof(struct PARAM_REMOVE_KEY);

	if (u4SetBufferLen < sizeof(struct PARAM_REMOVE_KEY))
		return WLAN_STATUS_INVALID_LENGTH;

	prRemovedKey = (struct PARAM_REMOVE_KEY *) pvSetBuffer;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prRemovedKey->ucBssIdx);
	u4KeyIndex = prRemovedKey->u4KeyIndex & 0x000000FF;

	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	/* KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE */
	if (prRemovedKey->i4LinkId != MLD_LINK_ID_NONE) {
		prMldStaRec = mldStarecGetByMldAddr(prAdapter,
			prMldBssInfo, prRemovedKey->arBSSID);
		ucLinkId = prRemovedKey->i4LinkId == -1 ?
			MLD_LINK_ID_NONE : prRemovedKey->i4LinkId;
	} else { /* old kernel version */
		prMldStaRec = mldStarecGetByLinkAddr(prAdapter,
			prMldBssInfo, prRemovedKey->arBSSID);
		ucLinkId = prAdapter->rWifiVar.ucPresetLinkId;
		prAdapter->rWifiVar.ucPresetLinkId = MLD_LINK_ID_NONE;
	}

	DBGLOG(RSN, INFO, "Remove: BSSID(" MACSTR
		"), BSS_INDEX (%d), Length(0x%08x), Key Index(0x%08x, %d) LinkId = %d\n",
		MAC2STR(prRemovedKey->arBSSID),
		prRemovedKey->ucBssIdx,
		prRemovedKey->u4Length, prRemovedKey->u4KeyIndex, u4KeyIndex,
		ucLinkId);

	if (prMldStaRec && u4KeyIndex & IS_UNICAST_KEY) {
		struct STA_RECORD *sta;

		LINK_FOR_EACH_ENTRY(sta, &prMldStaRec->rStarecList,
					rLinkEntryMld, struct STA_RECORD) {
			/* overwrite key info by link */
			prRemovedKey->ucBssIdx = sta->ucBssIndex;
			COPY_MAC_ADDR(prRemovedKey->arBSSID, sta->aucMacAddr);

			/* set oid is true only for the last cmd
			 * otherwise oid may complete wrongly
			 */
			ret = wlanSetRemoveKeyImpl(prAdapter, pvSetBuffer,
				u4SetBufferLen, pu4SetInfoLen,
				fgIsOid &&
				sta == LINK_PEEK_TAIL(&prMldStaRec->rStarecList,
				struct STA_RECORD, rLinkEntryMld));
			if (ret != WLAN_STATUS_SUCCESS &&
			    ret != WLAN_STATUS_PENDING)
				return ret;
		}
	} else if (IS_MLD_BSSINFO_MULTI(prMldBssInfo) &&
		ucLinkId != MLD_LINK_ID_NONE) {
		struct BSS_INFO *bss;

		LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList,
					rLinkEntryMld, struct BSS_INFO) {
			if (ucLinkId == bss->ucLinkIndex) {
				/* overwrite key info by link */
				prRemovedKey->ucBssIdx = bss->ucBssIndex;

				ret = wlanSetRemoveKeyImpl(prAdapter,
					pvSetBuffer, u4SetBufferLen,
					pu4SetInfoLen, fgIsOid);
				if (ret != WLAN_STATUS_SUCCESS &&
				    ret != WLAN_STATUS_PENDING)
					return ret;
			}
		}
	} else {
		ret = wlanSetRemoveKeyImpl(prAdapter, pvSetBuffer,
				u4SetBufferLen, pu4SetInfoLen, fgIsOid);
	}

	return ret;
#else
	return wlanSetRemoveKeyImpl(prAdapter,
		    pvSetBuffer, u4SetBufferLen,
		    pu4SetInfoLen, fgIsOid);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the default key
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 *
 * \note The setting buffer PARAM_KEY_T, which is set by NDIS, is unpacked.
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetDefaultKey(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	struct PARAM_DEFAULT_KEY *prDefaultKey;
	struct CMD_DEFAULT_KEY rCmdDefaultKey;
	struct BSS_INFO *prBssInfo;
	u_int8_t fgSetWepKey = FALSE;
	uint8_t ucWlanIndex = WTBL_RESERVED_ENTRY;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set add key! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (u4SetBufferLen < sizeof(struct PARAM_DEFAULT_KEY))
		return WLAN_STATUS_INVALID_LENGTH;

	prDefaultKey = (struct PARAM_DEFAULT_KEY *) pvSetBuffer;
	if (prDefaultKey->ucKeyID >= MAX_KEY_NUM) {
		DBGLOG(REQ, ERROR, "wrong keyidx=%d\n", prDefaultKey->ucKeyID);
		return WLAN_STATUS_FAILURE;
	}

	*pu4SetInfoLen = u4SetBufferLen;

	/* Dump PARAM_DEFAULT_KEY_T content. */
	DBGLOG(RSN, INFO,
	       "ucBssIndex %d, Key Index : %d, Unicast Key : %d, Multicast Key : %d\n",
	       prDefaultKey->ucBssIdx,
	       prDefaultKey->ucKeyID, prDefaultKey->ucUnicast,
	       prDefaultKey->ucMulticast);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prDefaultKey->ucBssIdx);

	DBGLOG(RSN, INFO, "WlanIdx = %d\n", prBssInfo->wepkeyWlanIdx);

	if (prDefaultKey->ucMulticast) {
		ASSERT(prBssInfo);
		if (prBssInfo->prStaRecOfAP) {	/* Actually GC not have wep */
			if (prBssInfo->wepkeyUsed[prDefaultKey->ucKeyID]) {
				prBssInfo->ucBcDefaultKeyIdx =
							prDefaultKey->ucKeyID;
				prBssInfo->fgBcDefaultKeyExist = TRUE;
				ucWlanIndex = prBssInfo->wepkeyWlanIdx;
			} else {
				if (prDefaultKey->ucUnicast) {
					DBGLOG(RSN, ERROR,
					       "Set STA Unicast default key");
					return WLAN_STATUS_SUCCESS;
				}
				ASSERT(FALSE);
			}
		} else {	/* For AP mode only */

			if (prBssInfo->wepkeyUsed[prDefaultKey->ucKeyID]
			    == TRUE)
				fgSetWepKey = TRUE;

			if (fgSetWepKey) {
				ucWlanIndex = prBssInfo->wepkeyWlanIdx;
			} else {
				if (!prBssInfo->ucBMCWlanIndexSUsed[
				    prDefaultKey->ucKeyID]) {
					DBGLOG(RSN, ERROR,
					       "Set AP wep default but key not exist!");
					return WLAN_STATUS_SUCCESS;
				}
				ucWlanIndex = prBssInfo->ucBMCWlanIndexS[
							prDefaultKey->ucKeyID];
			}
			prBssInfo->ucBcDefaultKeyIdx = prDefaultKey->ucKeyID;
			prBssInfo->fgBcDefaultKeyExist = TRUE;
		}
		if (ucWlanIndex > WTBL_SIZE) {
			DBGLOG(RSN, ERROR, "ucWlanIndex = %d, wepUsed = %d\n",
			       ucWlanIndex,
			       prBssInfo->wepkeyUsed[prDefaultKey->ucKeyID]);
			return WLAN_STATUS_FAILURE;
		}

	} else {
		DBGLOG(RSN, ERROR,
		       "Check the case set unicast default key!");
		ASSERT(FALSE);
	}

	/* compose CMD_DEFAULT_KEY cmd pkt */
	kalMemZero(&rCmdDefaultKey, sizeof(struct CMD_DEFAULT_KEY));

	rCmdDefaultKey.ucBssIdx = prDefaultKey->ucBssIdx;
	rCmdDefaultKey.ucKeyId = prDefaultKey->ucKeyID;
	rCmdDefaultKey.ucWlanIndex = ucWlanIndex;
	rCmdDefaultKey.ucMulticast = prDefaultKey->ucMulticast;

	DBGLOG(RSN, INFO,
	       "CMD_ID_DEFAULT_KEY_ID (%d) with wlan idx = %d\n",
	       prDefaultKey->ucKeyID, ucWlanIndex);

	return wlanSendSetQueryCmd(
		  prAdapter,
		  CMD_ID_DEFAULT_KEY_ID,
		  TRUE,
		  FALSE,
		  TRUE,
		  nicCmdEventSetCommon,
		  nicOidCmdTimeoutCommon,
		  sizeof(struct CMD_DEFAULT_KEY),
		  (uint8_t *)&rCmdDefaultKey,
		  pvSetBuffer,
		  u4SetBufferLen);
}				/* wlanoidSetDefaultKey */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current encryption status.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryEncryptionStatus(struct ADAPTER *prAdapter,
			     void *pvQueryBuffer,
			     uint32_t u4QueryBufferLen,
			     uint32_t *pu4QueryInfoLen) {
	u_int8_t fgTransmitKeyAvailable;
	enum ENUM_WEP_STATUS eEncStatus = 0;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo =
		aisGetAisBssInfo(prAdapter, ucBssIndex);
	if (prConnSettings == NULL || prAisBssInfo == NULL) {
		DBGLOG(REQ, ERROR, "prConnSettings or prAisBssInfo is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	*pu4QueryInfoLen = sizeof(enum ENUM_WEP_STATUS);

	fgTransmitKeyAvailable =
		prAisBssInfo->fgBcDefaultKeyExist;

	switch (prConnSettings->eEncStatus) {
	case ENUM_ENCRYPTION4_ENABLED:
		if (fgTransmitKeyAvailable)
			eEncStatus = ENUM_ENCRYPTION4_ENABLED;
		else
			eEncStatus = ENUM_ENCRYPTION4_KEY_ABSENT;
		break;

	case ENUM_ENCRYPTION3_ENABLED:
		if (fgTransmitKeyAvailable)
			eEncStatus = ENUM_ENCRYPTION3_ENABLED;
		else
			eEncStatus = ENUM_ENCRYPTION3_KEY_ABSENT;
		break;

	case ENUM_ENCRYPTION2_ENABLED:
		if (fgTransmitKeyAvailable) {
			eEncStatus = ENUM_ENCRYPTION2_ENABLED;
			break;
		}
		eEncStatus = ENUM_ENCRYPTION2_KEY_ABSENT;
		break;

	case ENUM_ENCRYPTION1_ENABLED:
		if (fgTransmitKeyAvailable)
			eEncStatus = ENUM_ENCRYPTION1_ENABLED;
		else
			eEncStatus = ENUM_ENCRYPTION1_KEY_ABSENT;
		break;

	case ENUM_ENCRYPTION_DISABLED:
		eEncStatus = ENUM_ENCRYPTION_DISABLED;
		break;

	default:
		DBGLOG(REQ, ERROR, "Unknown Encryption Status Setting:%d\n",
		       prConnSettings->eEncStatus);
	}

#if DBG
	DBGLOG(REQ, INFO,
	       "Encryption status: %d Return:%d\n",
	       prConnSettings->eEncStatus, eEncStatus);
#endif

	*(enum ENUM_WEP_STATUS *) pvQueryBuffer = eEncStatus;

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryEncryptionStatus */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the encryption status to the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_NOT_SUPPORTED
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetEncryptionStatus(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen) {
	struct GLUE_INFO *prGlueInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	enum ENUM_WEP_STATUS eEewEncrypt;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	prGlueInfo = prAdapter->prGlueInfo;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);

	*pu4SetInfoLen = sizeof(enum ENUM_WEP_STATUS);

	/* if (IS_ARB_IN_RFTEST_STATE(prAdapter)) { */
	/* return WLAN_STATUS_SUCCESS; */
	/* } */

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set encryption status! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (u4SetBufferLen < sizeof(enum ENUM_WEP_STATUS)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	eEewEncrypt = *(enum ENUM_WEP_STATUS *) pvSetBuffer;
	DBGLOG(REQ, INFO, "ENCRYPTION_STATUS %d\n", eEewEncrypt);

	switch (eEewEncrypt) {
	case ENUM_ENCRYPTION_DISABLED:	/* Disable WEP, TKIP, AES */
		DBGLOG(RSN, INFO, "Disable Encryption\n");
		secSetCipherSuite(prAdapter,
				  CIPHER_FLAG_WEP40 | CIPHER_FLAG_WEP104 |
				  CIPHER_FLAG_WEP128,
				  ucBssIndex);
		break;

	case ENUM_ENCRYPTION1_ENABLED:	/* Enable WEP. Disable TKIP, AES */
		DBGLOG(RSN, INFO, "Enable Encryption1\n");
		secSetCipherSuite(prAdapter,
				  CIPHER_FLAG_WEP40 | CIPHER_FLAG_WEP104 |
				  CIPHER_FLAG_WEP128,
				  ucBssIndex);
		break;

	case ENUM_ENCRYPTION2_ENABLED:	/* Enable WEP, TKIP. Disable AES */
		secSetCipherSuite(prAdapter,
				  CIPHER_FLAG_WEP40 | CIPHER_FLAG_WEP104 |
				  CIPHER_FLAG_WEP128 | CIPHER_FLAG_TKIP,
				  ucBssIndex);
		DBGLOG(RSN, INFO, "Enable Encryption2\n");
		break;

	case ENUM_ENCRYPTION3_ENABLED:	/* Enable WEP, TKIP, AES */
		secSetCipherSuite(prAdapter,
				  CIPHER_FLAG_WEP40 |
				  CIPHER_FLAG_WEP104 | CIPHER_FLAG_WEP128 |
				  CIPHER_FLAG_TKIP | CIPHER_FLAG_CCMP,
				  ucBssIndex);
		DBGLOG(RSN, INFO, "Enable Encryption3\n");
		break;

	case ENUM_ENCRYPTION4_ENABLED: /* Eanble GCMP256 */
		secSetCipherSuite(prAdapter,
				  CIPHER_FLAG_CCMP | CIPHER_FLAG_GCMP256 |
				  CIPHER_FLAG_GCMP128,
				  ucBssIndex);
		DBGLOG(RSN, INFO, "Enable Encryption4\n");
		break;

	default:
		DBGLOG(RSN, INFO, "Unacceptible encryption status: %d\n",
		       *(enum ENUM_WEP_STATUS *) pvSetBuffer);

		rStatus = WLAN_STATUS_NOT_SUPPORTED;
	}

	if (rStatus == WLAN_STATUS_SUCCESS) {
		/* Save the new encryption status. */
		prConnSettings->eEncStatus = *
				(enum ENUM_WEP_STATUS *) pvSetBuffer;
	}

	return rStatus;
}				/* wlanoidSetEncryptionStatus */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the driver's WPA2 status.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryCapability(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	struct PARAM_CAPABILITY *prCap;
	struct PARAM_AUTH_ENCRYPTION
		*prAuthenticationEncryptionSupported;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = 4 * sizeof(uint32_t) + 14 * sizeof(
				   struct PARAM_AUTH_ENCRYPTION);

	if (u4QueryBufferLen < *pu4QueryInfoLen)
		return WLAN_STATUS_INVALID_LENGTH;

	prCap = (struct PARAM_CAPABILITY *) pvQueryBuffer;

	prCap->u4Length = *pu4QueryInfoLen;
	prCap->u4Version = 2;	/* WPA2 */
	prCap->u4NoOfAuthEncryptPairsSupported = 16;

	prAuthenticationEncryptionSupported =
		&prCap->arAuthenticationEncryptionSupported[0];

	/* fill 16 entries of supported settings */
	prAuthenticationEncryptionSupported[0].eAuthModeSupported =
		AUTH_MODE_OPEN;

	prAuthenticationEncryptionSupported[0].eEncryptStatusSupported
		= ENUM_ENCRYPTION_DISABLED;

	prAuthenticationEncryptionSupported[1].eAuthModeSupported =
		AUTH_MODE_OPEN;
	prAuthenticationEncryptionSupported[1].eEncryptStatusSupported
		= ENUM_ENCRYPTION1_ENABLED;

	prAuthenticationEncryptionSupported[2].eAuthModeSupported =
		AUTH_MODE_SHARED;
	prAuthenticationEncryptionSupported[2].eEncryptStatusSupported
		= ENUM_ENCRYPTION_DISABLED;

	prAuthenticationEncryptionSupported[3].eAuthModeSupported =
		AUTH_MODE_SHARED;
	prAuthenticationEncryptionSupported[3].eEncryptStatusSupported
		= ENUM_ENCRYPTION1_ENABLED;

	prAuthenticationEncryptionSupported[4].eAuthModeSupported =
		AUTH_MODE_WPA;
	prAuthenticationEncryptionSupported[4].eEncryptStatusSupported
		= ENUM_ENCRYPTION2_ENABLED;

	prAuthenticationEncryptionSupported[5].eAuthModeSupported =
		AUTH_MODE_WPA;
	prAuthenticationEncryptionSupported[5].eEncryptStatusSupported
		= ENUM_ENCRYPTION3_ENABLED;

	prAuthenticationEncryptionSupported[6].eAuthModeSupported =
		AUTH_MODE_WPA_PSK;
	prAuthenticationEncryptionSupported[6].eEncryptStatusSupported
		= ENUM_ENCRYPTION2_ENABLED;

	prAuthenticationEncryptionSupported[7].eAuthModeSupported =
		AUTH_MODE_WPA_PSK;
	prAuthenticationEncryptionSupported[7].eEncryptStatusSupported
		= ENUM_ENCRYPTION3_ENABLED;

	prAuthenticationEncryptionSupported[8].eAuthModeSupported =
		AUTH_MODE_WPA_NONE;
	prAuthenticationEncryptionSupported[8].eEncryptStatusSupported
		= ENUM_ENCRYPTION2_ENABLED;

	prAuthenticationEncryptionSupported[9].eAuthModeSupported =
		AUTH_MODE_WPA_NONE;
	prAuthenticationEncryptionSupported[9].eEncryptStatusSupported
		= ENUM_ENCRYPTION3_ENABLED;

	prAuthenticationEncryptionSupported[10].eAuthModeSupported =
		AUTH_MODE_WPA2;
	prAuthenticationEncryptionSupported[10].eEncryptStatusSupported
		= ENUM_ENCRYPTION2_ENABLED;

	prAuthenticationEncryptionSupported[11].eAuthModeSupported =
		AUTH_MODE_WPA2;
	prAuthenticationEncryptionSupported[11].eEncryptStatusSupported
		= ENUM_ENCRYPTION3_ENABLED;

	prAuthenticationEncryptionSupported[12].eAuthModeSupported =
		AUTH_MODE_WPA2_PSK;
	prAuthenticationEncryptionSupported[12].eEncryptStatusSupported
		= ENUM_ENCRYPTION2_ENABLED;

	prAuthenticationEncryptionSupported[13].eAuthModeSupported =
		AUTH_MODE_WPA2_PSK;
	prAuthenticationEncryptionSupported[13].eEncryptStatusSupported
		= ENUM_ENCRYPTION3_ENABLED;

	prAuthenticationEncryptionSupported[14].eAuthModeSupported
		= AUTH_MODE_WPA2_PSK;
	prAuthenticationEncryptionSupported[14].eEncryptStatusSupported
		= ENUM_ENCRYPTION4_ENABLED;

	prAuthenticationEncryptionSupported[15].eAuthModeSupported
		= AUTH_MODE_WPA2_PSK;
	prAuthenticationEncryptionSupported[15].eEncryptStatusSupported
		= ENUM_ENCRYPTION4_ENABLED;

	return WLAN_STATUS_SUCCESS;

}				/* wlanoidQueryCapability */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the PMKID to the PMK cache in the
 *        driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval status
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetPmkid(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct PARAM_PMKID *prPmkid;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(struct PARAM_PMKID))
		return WLAN_STATUS_INVALID_DATA;

	*pu4SetInfoLen = u4SetBufferLen;
	prPmkid = (struct PARAM_PMKID *) pvSetBuffer;

	return rsnSetPmkid(prAdapter, prPmkid);
} /* wlanoidSetPmkid */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to delete the PMKID in the PMK cache.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval status
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidDelPmkid(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct PARAM_PMKID *prPmkid;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(struct PARAM_PMKID))
		return WLAN_STATUS_INVALID_DATA;

	*pu4SetInfoLen = u4SetBufferLen;
	prPmkid = (struct PARAM_PMKID *) pvSetBuffer;

	return rsnDelPmkid(prAdapter, prPmkid);
} /* wlanoidDelPmkid */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to delete all the PMKIDs in the PMK cache.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval status
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidFlushPmkid(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	return rsnFlushPmkid(prAdapter, ucBssIndex);
} /* wlanoidFlushPmkid */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the set of supported data rates that
 *          the radio is capable of running
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query
 * \param[in] u4QueryBufferLen The length of the query buffer
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number
 *                             of bytes written into the query buffer. If the
 *                             call failed due to invalid length of the query
 *                             buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQuerySupportedRates(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen) {
	uint8_t eRate[PARAM_MAX_LEN_RATES] = {
		/* BSSBasicRateSet for 802.11n Non-HT rates */
		0x8C,		/* 6M */
		0x92,		/* 9M */
		0x98,		/* 12M */
		0xA4,		/* 18M */
		0xB0,		/* 24M */
		0xC8,		/* 36M */
		0xE0,		/* 48M */
		0xEC		/* 54M */
	};

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = (sizeof(uint8_t) *
			    PARAM_MAX_LEN_RATES_EX);

	if (u4QueryBufferLen < *pu4QueryInfoLen) {
		DBGLOG(REQ, WARN, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	kalMemCopy(pvQueryBuffer, (void *) &eRate,
		   (sizeof(uint8_t) * PARAM_MAX_LEN_RATES));

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidQuerySupportedRates() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the maximum frame size in bytes,
 *        not including the header.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                               bytes written into the query buffer. If the
 *                               call failed due to invalid length of the query
 *                               buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryMaxFrameSize(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < sizeof(uint32_t)) {
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*(uint32_t *) pvQueryBuffer = ETHERNET_MAX_PKT_SZ -
				      ETHERNET_HEADER_SZ;
	*pu4QueryInfoLen = sizeof(uint32_t);

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryMaxFrameSize */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the maximum total packet length
 *        in bytes.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryMaxTotalSize(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < sizeof(uint32_t)) {
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*(uint32_t *) pvQueryBuffer = ETHERNET_MAX_PKT_SZ;
	*pu4QueryInfoLen = sizeof(uint32_t);

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryMaxTotalSize */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the vendor ID of the NIC.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryVendorId(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen) {
#if DBG
	uint8_t *cp;
#endif
	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < sizeof(uint32_t)) {
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	kalMemCopy(pvQueryBuffer, prAdapter->aucMacAddress, 3);
	*((uint8_t *) pvQueryBuffer + 3) = 1;
	*pu4QueryInfoLen = sizeof(uint32_t);

#if DBG
	cp = (uint8_t *) pvQueryBuffer;
	DBGLOG(REQ, LOUD, "Vendor ID=%02x-%02x-%02x-%02x\n", cp[0],
	       cp[1], cp[2], cp[3]);
#endif

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryVendorId */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current RSSI value.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer Pointer to the buffer that holds the result of the
 *	      query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *   bytes written into the query buffer. If the call failed due to invalid
 *   length of the query buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryRssi(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen) {
	uint8_t ucBssIndex;
	struct PARAM_LINK_SPEED_EX *prLinkSpeed;
	struct LINK_SPEED_EX_ *prLq;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	if (ucBssIndex >= MAX_BSSID_NUM)
		return WLAN_STATUS_NOT_SUPPORTED;

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex) ||
	    ucBssIndex == prAdapter->ucP2PDevBssIdx)
		return WLAN_STATUS_NOT_SUPPORTED;

	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (prAdapter->fgIsEnableLpdvt)
		return WLAN_STATUS_NOT_SUPPORTED;

	*pu4QueryInfoLen = sizeof(struct PARAM_LINK_SPEED_EX);
	prLq = &prAdapter->rLinkQuality.rLq[ucBssIndex];
	/* Check for query buffer length */
	if (u4QueryBufferLen < *pu4QueryInfoLen) {
		DBGLOG(REQ, WARN, "Too short length %u\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo, ucBssIndex) ==
	    MEDIA_STATE_DISCONNECTED) {
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (prLq->fgIsLinkQualityValid == TRUE &&
		   (kalGetTimeTick() - prLq->rLinkQualityUpdateTime) <=
		   CFG_LINK_QUALITY_VALID_PERIOD) {
		prLinkSpeed = (struct PARAM_LINK_SPEED_EX *) pvQueryBuffer;

		prLinkSpeed->rLq[ucBssIndex].
			u2TxLinkSpeed = prLq->u2TxLinkSpeed * 5000;
		prLinkSpeed->rLq[ucBssIndex].cRssi = prLq->cRssi;

		DBGLOG(REQ, TRACE, "ucBssIdx = %d, TxRate = %u, signal = %d\n",
		       ucBssIndex,
		       prLinkSpeed->rLq[ucBssIndex].u2TxLinkSpeed,
		       prLinkSpeed->rLq[ucBssIndex].cRssi);
		return WLAN_STATUS_SUCCESS;
	}
#ifdef LINUX
	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_GET_LINK_QUALITY,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryLinkQuality,
				   nicOidCmdTimeoutCommon,
				   *pu4QueryInfoLen, pvQueryBuffer,
				   pvQueryBuffer,
				   u4QueryBufferLen);
#else
	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_GET_LINK_QUALITY,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryLinkQuality,
				   nicOidCmdTimeoutCommon, 0, NULL,
				   pvQueryBuffer,
				   u4QueryBufferLen);

#endif
} /* end of wlanoidQueryRssi() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current RSSI trigger value.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer Pointer to the buffer that holds the result of the
 *	      query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *   bytes written into the query buffer. If the call failed due to invalid
 *   length of the query buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryRssiTrigger(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen)
{
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	prWlanInfo = &prAdapter->rWlanInfo;

	if (prWlanInfo->eRssiTriggerType == ENUM_RSSI_TRIGGER_NONE)
		return WLAN_STATUS_ADAPTER_NOT_READY;

	*pu4QueryInfoLen = sizeof(int32_t);

	/* Check for query buffer length */
	if (u4QueryBufferLen < *pu4QueryInfoLen) {
		DBGLOG(REQ, WARN, "Too short length %u\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	*(int32_t *) pvQueryBuffer = prWlanInfo->rRssiTriggerValue;
	DBGLOG(REQ, INFO, "RSSI trigger: %d dBm\n",
	       *(int32_t *) pvQueryBuffer);

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryRssiTrigger */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set a trigger value of the RSSI event.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns the
 *                          amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetRssiTrigger(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
	int32_t rRssiTriggerValue;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(int32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prWlanInfo = &prAdapter->rWlanInfo;

	*pu4SetInfoLen = sizeof(int32_t);
	rRssiTriggerValue = *(int32_t *) pvSetBuffer;

	if (rRssiTriggerValue > PARAM_WHQL_RSSI_MAX_DBM ||
	    rRssiTriggerValue < PARAM_WHQL_RSSI_MIN_DBM)
		return prWlanInfo->rRssiTriggerValue = rRssiTriggerValue;
		/* Save the RSSI trigger value to the Adapter structure */

	/* If the RSSI trigger value is equal to the current RSSI value, the
	 * indication triggers immediately. We need to indicate the protocol
	 * that an RSSI status indication event triggers.
	 */
	if (rRssiTriggerValue == (int32_t) (
		    prAdapter->rLinkQuality.rLq[ucBssIndex].cRssi)) {
		prWlanInfo->eRssiTriggerType = ENUM_RSSI_TRIGGER_TRIGGERED;

		kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
			     WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
			     &prWlanInfo->rRssiTriggerValue,
			     sizeof(int32_t),
			     ucBssIndex);
	} else if (rRssiTriggerValue < (int32_t) (
			   prAdapter->rLinkQuality.rLq[ucBssIndex].cRssi))
		prWlanInfo->eRssiTriggerType = ENUM_RSSI_TRIGGER_GREATER;
	else if (rRssiTriggerValue > (int32_t) (
			 prAdapter->rLinkQuality.rLq[ucBssIndex].cRssi))
		prWlanInfo->eRssiTriggerType = ENUM_RSSI_TRIGGER_LESS;

	return WLAN_STATUS_SUCCESS;
} /* wlanoidSetRssiTrigger */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set a suggested value for the number of
 *        bytes of received packet data that will be indicated to the protocol
 *        driver. We just accept the set and ignore this value.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetCurrentLookahead(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen) {
	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(uint32_t)) {
		*pu4SetInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = sizeof(uint32_t);
	return WLAN_STATUS_SUCCESS;
} /* wlanoidSetCurrentLookahead */

/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to query the current 802.11 statistics.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure
 * \param[in] pvQueryBuf A pointer to the buffer that holds the result of the
 *                          query buffer
 * \param[in] u4QueryBufLen The length of the query buffer
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryStatistics(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	DBGLOG(REQ, LOUD, "\n");

	return wlanQueryStatistics(prAdapter, pvQueryBuffer, u4QueryBufferLen,
				pu4QueryInfoLen, TRUE);
} /* wlanoidQueryStatistics */

uint32_t
wlanoidQueryBugReport(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(struct EVENT_BUG_REPORT);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(OID, WARN,
		       "Fail in query receive error! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct
					     EVENT_BUG_REPORT)) {
		DBGLOG(OID, WARN, "Too short length %u\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_GET_BUG_REPORT,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryBugReport,
				   nicOidCmdTimeoutCommon,
				   0, NULL, pvQueryBuffer, u4QueryBufferLen);
}				/* wlanoidQueryBugReport */

/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to query the permanent MAC address of the
 *	    NIC.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure
 * \param[in] pvQueryBuf A pointer to the buffer that holds the result of the
 *                          query buffer
 * \param[in] u4QueryBufLen The length of the query buffer
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryPermanentAddr(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen) {
	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < MAC_ADDR_LEN)
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	COPY_MAC_ADDR(pvQueryBuffer,
		      prAdapter->rWifiVar.aucPermanentAddress);
	*pu4QueryInfoLen = MAC_ADDR_LEN;

	return WLAN_STATUS_SUCCESS;
}				/* wlanoidQueryPermanentAddr */

/*----------------------------------------------------------------------------*/
/*! \brief  This routine is called to query the MAC address the NIC is
 *          currently using.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure
 * \param[in] pvQueryBuf A pointer to the buffer that holds the result of the
 *                          query buffer
 * \param[in] u4QueryBufLen The length of the query buffer
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryCurrentAddr(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen) {
	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < MAC_ADDR_LEN)
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	COPY_MAC_ADDR(pvQueryBuffer,
		      prAdapter->rWifiVar.aucMacAddress[*pu4QueryInfoLen]);
	*pu4QueryInfoLen = MAC_ADDR_LEN;

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryCurrentAddr */

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
uint32_t
wlanoidQueryLinkSpeed(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen)
{
	uint8_t ucBssIndex;
	OS_SYSTIME rUpdateDeltaTime;
	struct PARAM_LINK_SPEED_EX *pu4LinkSpeed;
	struct LINK_SPEED_EX_ *prLq;
	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (prAdapter->fgIsEnableLpdvt)
		return WLAN_STATUS_NOT_SUPPORTED;

	*pu4QueryInfoLen = sizeof(struct PARAM_LINK_SPEED_EX);

	if (u4QueryBufferLen < sizeof(struct PARAM_LINK_SPEED_EX))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	if (unlikely(ucBssIndex >= MAX_BSSID_NUM))
		return WLAN_STATUS_INVALID_DATA;
	prLq = &prAdapter->rLinkQuality.rLq[ucBssIndex];
	rUpdateDeltaTime = kalGetTimeTick() - prLq->rLinkRateUpdateTime;
	if (IS_BSS_INDEX_AIS(prAdapter, ucBssIndex) &&
		prLq->fgIsLinkRateValid == TRUE &&
		rUpdateDeltaTime <= CFG_LQ_MONITOR_FREQUENCY) {
		pu4LinkSpeed = (struct PARAM_LINK_SPEED_EX *) (pvQueryBuffer);
		pu4LinkSpeed->rLq[ucBssIndex].cRssi = prLq->cRssi;
		pu4LinkSpeed->rLq[ucBssIndex].u2TxLinkSpeed =
			prLq->u2TxLinkSpeed;
		pu4LinkSpeed->rLq[ucBssIndex].u2RxLinkSpeed =
			prLq->u2RxLinkSpeed;
		pu4LinkSpeed->rLq[ucBssIndex].u4RxBw =
			prLq->u4RxBw;
		return WLAN_STATUS_SUCCESS;
	} else {
		return wlanSendSetQueryCmd(prAdapter,
					CMD_ID_GET_LINK_QUALITY,
					FALSE,
					TRUE,
					TRUE,
					nicCmdEventQueryLinkQuality,
					nicOidCmdTimeoutCommon, 0, NULL,
					pvQueryBuffer, u4QueryBufferLen);
	}
}
#endif

#if CFG_REPORT_MAX_TX_RATE
uint32_t
wlanoidQueryMaxLinkSpeed(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen)
{
	uint32_t u4CurRate = 0, u4MaxRate = 0;
	uint32_t rv = WLAN_STATUS_FAILURE;
	uint8_t ucBssIndex;
	struct STA_RECORD *prStaRecOfAP;
	struct BSS_INFO *prBssInfo;
	struct PARAM_LINK_SPEED_EX *prLinkSpeed;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (prAdapter->fgIsEnableLpdvt)
		return WLAN_STATUS_NOT_SUPPORTED;

	*pu4QueryInfoLen = sizeof(uint32_t);

	if (u4QueryBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	prStaRecOfAP = aisGetStaRecOfAP(prAdapter, ucBssIndex);
	prBssInfo =  aisGetAisBssInfo(prAdapter, ucBssIndex);
	prLinkSpeed = (struct PARAM_LINK_SPEED_EX *)pvQueryBuffer;

	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo, ucBssIndex) !=
	    MEDIA_STATE_CONNECTED || prStaRecOfAP == NULL) {
		rv = WLAN_STATUS_ADAPTER_NOT_READY;
	} else {
		if (wlanGetMaxTxRate(prAdapter, prBssInfo, prStaRecOfAP,
				    &u4CurRate, &u4MaxRate) >= 0) {
			u4MaxRate = u4MaxRate * 1000;
			prLinkSpeed->rLq[ucBssIndex].u2TxLinkSpeed = u4MaxRate;
			prLinkSpeed->rLq[ucBssIndex].u2RxLinkSpeed = u4MaxRate;
			rv = WLAN_STATUS_SUCCESS;
		}
	}
	return rv;
}
#endif /* CFG_REPORT_MAX_TX_RATE */

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
uint32_t
wlanoidQueryStatsOneCmd(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen)
{
	uint32_t rResult = WLAN_STATUS_FAILURE;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (prAdapter->fgIsEnableLpdvt)
		return WLAN_STATUS_NOT_SUPPORTED;

	if (u4QueryBufferLen < sizeof(struct PARAM_GET_STATS_ONE_CMD))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	rResult = wlanQueryStatsOneCmd(prAdapter,
				pvQueryBuffer,
				u4QueryBufferLen,
				pu4QueryInfoLen,
				TRUE,
				GET_IOCTL_BSSIDX(prAdapter));
	return rResult;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set tx latency monitor parameter
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetTxLatMontrParam(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
	struct TX_LAT_MONTR_PARAM_STRUCT *prParam;

	if (u4SetBufferLen < sizeof(struct TX_LAT_MONTR_PARAM_STRUCT)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prParam = (struct TX_LAT_MONTR_PARAM_STRUCT *)pvSetBuffer;
	return wlanSetTxDelayOverLimitReport(prAdapter, prParam->fgEnabled,
		prParam->fgIsAvg, prParam->u4Intvl, prParam->u4DriverCrit,
		prParam->u4MacCrit);
}


/*----------------------------------------------------------------------------*/
/*!
* \brief extend command packet generation utility
*
* \param[in] prAdapter Pointer to the Adapter structure.
* \param[in] ucCID Command ID
* \param[in] ucExtCID Extend command ID
* \param[in] fgSetQuery Set or Query
* \param[in] fgNeedResp Need for response
* \param[in] pfCmdDoneHandler Function pointer when command is done
* \param[in] u4SetQueryInfoLen The length of the set/query buffer
* \param[in] pucInfoBuffer Pointer to set/query buffer
*
*
* \retval WLAN_STATUS_PENDING
* \retval WLAN_STATUS_FAILURE
*/
/*----------------------------------------------------------------------------*/
uint32_t
wlanSendSetQueryExtCmd(
	struct ADAPTER *prAdapter,
	uint8_t ucCID,
	uint8_t ucExtCID,
	u_int8_t fgSetQuery,
	u_int8_t fgNeedResp,
	u_int8_t fgIsOid,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
	uint32_t u4SetQueryInfoLen,
	uint8_t *pucInfoBuffer,
	void *pvSetQueryBuffer,
	uint32_t u4SetQueryBufferLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return wlanSendSetQueryCmdHelper(
#else
	return wlanSendSetQueryCmdAdv(
#endif
		prAdapter, ucCID, ucExtCID, fgSetQuery,
		fgNeedResp, fgIsOid, pfCmdDoneHandler,
		pfCmdTimeoutHandler, u4SetQueryInfoLen,
		pucInfoBuffer, pvSetQueryBuffer, u4SetQueryBufferLen,
		CMD_SEND_METHOD_ENQUEUE);
}

#if CFG_SUPPORT_BUFFER_MODE
uint32_t
wlanoidSetEfusBufferMode(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_EFUSE_BUFFER_MODE
		*prSetEfuseBufModeInfo;
	struct CMD_EFUSE_BUFFER_MODE *prCmdSetEfuseBufModeInfo =
			NULL;
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler;
	uint32_t u4EfuseContentSize, u4QueryInfoLen;
	u_int8_t fgSetQuery, fgNeedResp;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	/* get the buffer mode info */
	prSetEfuseBufModeInfo =
			(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE *) pvSetBuffer;

	/* copy command header */
	prCmdSetEfuseBufModeInfo = (struct CMD_EFUSE_BUFFER_MODE *)
		kalMemAlloc(sizeof(struct CMD_EFUSE_BUFFER_MODE),
			    VIR_MEM_TYPE);
	if (prCmdSetEfuseBufModeInfo == NULL)
		return WLAN_STATUS_FAILURE;
	kalMemZero(prCmdSetEfuseBufModeInfo,
		   sizeof(struct CMD_EFUSE_BUFFER_MODE));
	prCmdSetEfuseBufModeInfo->ucSourceMode =
		prSetEfuseBufModeInfo->ucSourceMode;
	prCmdSetEfuseBufModeInfo->ucCount      =
		prSetEfuseBufModeInfo->ucCount;
	prCmdSetEfuseBufModeInfo->ucCmdType    =
		prSetEfuseBufModeInfo->ucCmdType;
	prCmdSetEfuseBufModeInfo->ucReserved   =
		prSetEfuseBufModeInfo->ucReserved;

	/* decide content size and SetQuery / NeedResp flag */
	if (prAdapter->fgIsSupportBufferBinSize16Byte == TRUE) {
		u4EfuseContentSize  = sizeof(struct BIN_CONTENT) *
				      EFUSE_CONTENT_SIZE;
		pfCmdDoneHandler = nicCmdEventSetCommon;
		fgSetQuery = TRUE;
		fgNeedResp = FALSE;
	} else {
#if (CFG_FW_Report_Efuse_Address == 1)
		u4EfuseContentSize = (prAdapter->u4EfuseEndAddress) -
				     (prAdapter->u4EfuseStartAddress) + 1;
#else
		u4EfuseContentSize = EFUSE_CONTENT_BUFFER_SIZE;
#endif
		pfCmdDoneHandler = NULL;
		fgSetQuery = FALSE;
		fgNeedResp = TRUE;
	}

	u4QueryInfoLen = OFFSET_OF(struct CMD_EFUSE_BUFFER_MODE,
				   aBinContent) + u4EfuseContentSize;

	if (u4SetBufferLen < u4QueryInfoLen) {
		kalMemFree(prCmdSetEfuseBufModeInfo, VIR_MEM_TYPE,
			   sizeof(struct CMD_EFUSE_BUFFER_MODE));
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = u4QueryInfoLen;
	kalMemCopy(prCmdSetEfuseBufModeInfo->aBinContent,
		   prSetEfuseBufModeInfo->aBinContent,
		   u4EfuseContentSize);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				EXT_CMD_ID_EFUSE_BUFFER_MODE,
				fgSetQuery,
				fgNeedResp,
				TRUE,
				pfCmdDoneHandler,
				nicOidCmdTimeoutCommon,
				u4QueryInfoLen,
				(uint8_t *) (prCmdSetEfuseBufModeInfo),
				pvSetBuffer, u4SetBufferLen);

	kalMemFree(prCmdSetEfuseBufModeInfo, VIR_MEM_TYPE,
		   sizeof(struct CMD_EFUSE_BUFFER_MODE));

	return rWlanStatus;
}

uint32_t
wlanoidConnacSetEfusBufferMode(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T
		*prSetEfuseBufModeInfo;
	struct CMD_EFUSE_BUFFER_MODE_CONNAC_T
		*prCmdSetEfuseBufModeInfo = NULL;
	uint32_t u4EfuseContentSize, u4QueryInfoLen;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	DBGLOG(OID, INFO, "u4SetBufferLen = %d\n", u4SetBufferLen);
	/* get the buffer mode info */
	prSetEfuseBufModeInfo =
		(struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T *) pvSetBuffer;

	/* copy command header */
	prCmdSetEfuseBufModeInfo = (struct CMD_EFUSE_BUFFER_MODE_CONNAC_T *)
		kalMemAlloc(sizeof(struct CMD_EFUSE_BUFFER_MODE_CONNAC_T),
			    VIR_MEM_TYPE);
	if (prCmdSetEfuseBufModeInfo == NULL)
		return WLAN_STATUS_FAILURE;
	kalMemZero(prCmdSetEfuseBufModeInfo,
		   sizeof(struct CMD_EFUSE_BUFFER_MODE_CONNAC_T));
	prCmdSetEfuseBufModeInfo->ucSourceMode =
		prSetEfuseBufModeInfo->ucSourceMode;
	prCmdSetEfuseBufModeInfo->ucContentFormat =
		prSetEfuseBufModeInfo->ucContentFormat;
	prCmdSetEfuseBufModeInfo->u2Count =
		prSetEfuseBufModeInfo->u2Count;

	DBGLOG(OID, INFO, "[%d] ucSourceMode = %d, ucContentFormat = 0x%x, u2Count = %d\n"
		, __LINE__,
		prCmdSetEfuseBufModeInfo->ucSourceMode,
		prCmdSetEfuseBufModeInfo->ucContentFormat,
		prCmdSetEfuseBufModeInfo->u2Count);

	u4EfuseContentSize = prCmdSetEfuseBufModeInfo->u2Count;

	u4QueryInfoLen = OFFSET_OF(struct
				   CMD_EFUSE_BUFFER_MODE_CONNAC_T,
				   aBinContent) + u4EfuseContentSize;

	if (u4SetBufferLen < u4QueryInfoLen) {
		kalMemFree(prCmdSetEfuseBufModeInfo, VIR_MEM_TYPE,
			   sizeof(struct CMD_EFUSE_BUFFER_MODE_CONNAC_T));
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = u4QueryInfoLen;
	kalMemCopy(prCmdSetEfuseBufModeInfo->aBinContent,
		   prSetEfuseBufModeInfo->aBinContent,
		   u4EfuseContentSize);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				EXT_CMD_ID_EFUSE_BUFFER_MODE,
				FALSE,
				TRUE,
				TRUE,
				NULL,
				nicOidCmdTimeoutCommon,
				u4QueryInfoLen,
				(uint8_t *) (prCmdSetEfuseBufModeInfo),
				pvSetBuffer, u4SetBufferLen);

	kalMemFree(prCmdSetEfuseBufModeInfo, VIR_MEM_TYPE,
		   sizeof(struct CMD_EFUSE_BUFFER_MODE_CONNAC_T));

	return rWlanStatus;
}

uint32_t
wlanoidConnacSetEfuseBufferRD(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_EFUSE_BUFFER_RD_T
		*prSetEfuseBufModeInfo;
	struct CMD_EFUSE_BUFFER_RD_T
		*prCmdSetEfuseBufRDInfo = NULL;
	uint32_t u4EfuseContentSize;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	DBGLOG(OID, INFO, "u4SetBufferLen = %d\n", u4SetBufferLen);
	/* get the buffer mode info */
	prSetEfuseBufModeInfo =
		(struct PARAM_CUSTOM_EFUSE_BUFFER_RD_T *) pvSetBuffer;

	/* copy command header */
	prCmdSetEfuseBufRDInfo = (struct CMD_EFUSE_BUFFER_RD_T *)
		kalMemAlloc(sizeof(struct CMD_EFUSE_BUFFER_RD_T),
			    VIR_MEM_TYPE);
	if (prCmdSetEfuseBufRDInfo == NULL)
		return WLAN_STATUS_FAILURE;
	kalMemZero(prCmdSetEfuseBufRDInfo,
		   sizeof(struct CMD_EFUSE_BUFFER_RD_T));
	prCmdSetEfuseBufRDInfo->ucSourceMode =
		prSetEfuseBufModeInfo->ucSourceMode;
	prCmdSetEfuseBufRDInfo->ucContentFormat =
		prSetEfuseBufModeInfo->ucContentFormat;
	prCmdSetEfuseBufRDInfo->u2Count =
		prSetEfuseBufModeInfo->u2Count;

	DBGLOG(OID, INFO,
		"[%d] ucSourceMode = %d, ucContentFormat = 0x%x, u2Count = %d\n"
		, __LINE__,
		prCmdSetEfuseBufRDInfo->ucSourceMode,
		prCmdSetEfuseBufRDInfo->ucContentFormat,
		prCmdSetEfuseBufRDInfo->u2Count);

	u4EfuseContentSize = prCmdSetEfuseBufRDInfo->u2Count;
#if 0
	u4QueryInfoLen = OFFSET_OF(struct
				   CMD_EFUSE_BUFFER_RD_T,
				   NULL) + u4EfuseContentSize;

	if (u4SetBufferLen < u4QueryInfoLen) {
		kalMemFree(prCmdSetEfuseBufRDInfo, VIR_MEM_TYPE,
			   sizeof(struct CMD_EFUSE_BUFFER_RD_T));
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = u4QueryInfoLen;
	kalMemCopy(prCmdSetEfuseBufRDInfo->aBinContent,
		   prSetEfuseBufModeInfo->aBinContent,
		   u4EfuseContentSize);
#endif
	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
				CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				EXT_CMD_ID_EFUSE_BUFFER_RD,
				FALSE,
				TRUE,
				TRUE,
				NULL,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_EFUSE_BUFFER_RD_T),
				(uint8_t *) (prCmdSetEfuseBufRDInfo),
				pvSetBuffer, u4SetBufferLen);

	kalMemFree(prCmdSetEfuseBufRDInfo, VIR_MEM_TYPE,
		   sizeof(struct CMD_EFUSE_BUFFER_RD_T));

	return rWlanStatus;
}


/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to read efuse content.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryProcessAccessEfuseRead(struct ADAPTER *prAdapter,
				   void *pvSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_ACCESS_EFUSE *prSetAccessEfuseInfo;
	struct CMD_ACCESS_EFUSE rCmdSetAccessEfuse;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_ACCESS_EFUSE))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prSetAccessEfuseInfo = (struct PARAM_CUSTOM_ACCESS_EFUSE *)
			       pvSetBuffer;

	kalMemSet(&rCmdSetAccessEfuse, 0,
		  sizeof(struct CMD_ACCESS_EFUSE));

	rCmdSetAccessEfuse.u4Address =
		prSetAccessEfuseInfo->u4Address;
	rCmdSetAccessEfuse.u4Valid = prSetAccessEfuseInfo->u4Valid;

	DBGLOG(INIT, INFO,
	       "MT6632 : wlanoidQueryProcessAccessEfuseRead, address=%d\n",
	       rCmdSetAccessEfuse.u4Address);

	kalMemCopy(rCmdSetAccessEfuse.aucData,
		   prSetAccessEfuseInfo->aucData,
		   sizeof(uint8_t) * 16);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_EFUSE_ACCESS,
			     FALSE, /* Query Bit: True->write False->read */
			     TRUE,
			     TRUE,
			     NULL, /* No Tx done function wait until fw ack */
			     nicOidCmdTimeoutCommon,
			     sizeof(struct CMD_ACCESS_EFUSE),
			     (uint8_t *) (&rCmdSetAccessEfuse), pvSetBuffer,
			     u4SetBufferLen);

	return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to write efuse content.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryProcessAccessEfuseWrite(struct ADAPTER *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_ACCESS_EFUSE *prSetAccessEfuseInfo;
	struct CMD_ACCESS_EFUSE rCmdSetAccessEfuse;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO,
	       "MT6632 : wlanoidQueryProcessAccessEfuseWrite\n");


	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_ACCESS_EFUSE))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prSetAccessEfuseInfo = (struct PARAM_CUSTOM_ACCESS_EFUSE *)
			       pvSetBuffer;

	kalMemSet(&rCmdSetAccessEfuse, 0,
		  sizeof(struct CMD_ACCESS_EFUSE));

	rCmdSetAccessEfuse.u4Address =
		prSetAccessEfuseInfo->u4Address;
	rCmdSetAccessEfuse.u4Valid = prSetAccessEfuseInfo->u4Valid;

	DBGLOG(INIT, INFO,
	       "MT6632 : wlanoidQueryProcessAccessEfuseWrite, address=%d\n",
	       rCmdSetAccessEfuse.u4Address);


	kalMemCopy(rCmdSetAccessEfuse.aucData,
		   prSetAccessEfuseInfo->aucData,
		   sizeof(uint8_t) * 16);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_EFUSE_ACCESS,
			TRUE, /* Query Bit: True->write False->read*/
			TRUE,
			TRUE,
			NULL, /* No Tx done function wait until fw ack */
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_ACCESS_EFUSE),
			(uint8_t *) (&rCmdSetAccessEfuse), pvSetBuffer,
			u4SetBufferLen);

	return rWlanStatus;
}




uint32_t
wlanoidQueryEfuseFreeBlock(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_EFUSE_FREE_BLOCK
		*prGetEfuseFreeBlockInfo;
	struct CMD_EFUSE_FREE_BLOCK rCmdGetEfuseFreeBlock;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_EFUSE_FREE_BLOCK);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_EFUSE_FREE_BLOCK))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prGetEfuseFreeBlockInfo = (struct
				   PARAM_CUSTOM_EFUSE_FREE_BLOCK *) pvSetBuffer;

	kalMemSet(&rCmdGetEfuseFreeBlock, 0,
		  sizeof(struct CMD_EFUSE_FREE_BLOCK));

	rCmdGetEfuseFreeBlock.ucVersion = 1; /*1:new version, 0:old version*/
	rCmdGetEfuseFreeBlock.ucDieIndex = prGetEfuseFreeBlockInfo->ucDieIdx;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_EFUSE_FREE_BLOCK,
			FALSE, /* Query Bit: True->write False->read */
			TRUE,
			TRUE,
			NULL, /* No Tx done function wait until fw ack */
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_EFUSE_FREE_BLOCK),
			(uint8_t *) (&rCmdGetEfuseFreeBlock), pvSetBuffer,
			u4SetBufferLen);

	return rWlanStatus;
}

uint32_t
wlanoidQueryGetTxPower(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_GET_TX_POWER *prGetTxPowerInfo;
	struct CMD_GET_TX_POWER rCmdGetTxPower;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_GET_TX_POWER *);

	if (u4SetBufferLen < sizeof(struct PARAM_CUSTOM_GET_TX_POWER))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prGetTxPowerInfo = (struct PARAM_CUSTOM_GET_TX_POWER *)
			   pvSetBuffer;

	kalMemSet(&rCmdGetTxPower, 0,
		  sizeof(struct CMD_GET_TX_POWER));

	rCmdGetTxPower.ucTxPwrType = EXT_EVENT_TARGET_TX_POWER;
	rCmdGetTxPower.ucCenterChannel =
		prGetTxPowerInfo->ucCenterChannel;
	rCmdGetTxPower.ucDbdcIdx = prGetTxPowerInfo->ucDbdcIdx;
	rCmdGetTxPower.ucBand = prGetTxPowerInfo->ucBand;


	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			EXT_CMD_ID_GET_TX_POWER,
			FALSE, /* Query Bit: True->write False->read*/
			TRUE,
			TRUE,
			NULL, /* No Tx done function wait until fw ack */
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_GET_TX_POWER),
			(uint8_t *) (&rCmdGetTxPower),
			pvSetBuffer, u4SetBufferLen);

	return rWlanStatus;
}


/*#endif*/

#endif /* CFG_SUPPORT_BUFFER_MODE */

#if CFG_SUPPORT_QA_TOOL

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query RX statistics.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryRxStatistics(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_ACCESS_RX_STAT *prRxStatistics;
	struct CMD_ACCESS_RX_STAT *prCmdAccessRxStat;
	struct CMD_ACCESS_RX_STAT rCmdAccessRxStat;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	prCmdAccessRxStat = &rCmdAccessRxStat;

	DBGLOG(INIT, LOUD, "\n");

	DBGLOG(INIT, ERROR, "MT6632 : wlanoidQueryRxStatistics\n");

	if (u4QueryBufferLen < sizeof(struct PARAM_CUSTOM_ACCESS_RX_STAT)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prRxStatistics = (struct PARAM_CUSTOM_ACCESS_RX_STAT *)
			 pvQueryBuffer;

	*pu4QueryInfoLen = 8 + prRxStatistics->u4TotalNum;

	do {
#if (CFG_SUPPORT_CONNAC3X == 0)
		prCmdAccessRxStat->u4SeqNum =
			prRxStatistics->u4SeqNum;
		prCmdAccessRxStat->u4TotalNum =
			prRxStatistics->u4TotalNum;
#else
		prCmdAccessRxStat->u2SeqNum =
			prRxStatistics->u2SeqNum;
		prCmdAccessRxStat->u4TotalNum =
			prRxStatistics->u4TotalNum;
		prCmdAccessRxStat->ucDbdcIdx =
			prRxStatistics->ucDbdcIdx;
		prCmdAccessRxStat->ucData =
			prRxStatistics->ucData;
#endif

		rStatus = wlanSendSetQueryCmd(prAdapter,
			      CMD_ID_ACCESS_RX_STAT,
			      FALSE,
			      TRUE,
			      TRUE,
			      nicCmdEventQueryRxStatistics,
			      nicOidCmdTimeoutCommon,
			      sizeof(struct CMD_ACCESS_RX_STAT),
			      (uint8_t *) prCmdAccessRxStat,
			      pvQueryBuffer,
			      u4QueryBufferLen);
	} while (FALSE);

	return rStatus;
}

uint32_t
wlanoidManualAssoc(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen) {
	struct CMD_STAREC_UPDATE *prStaRecManualAssoc;
	struct CMD_MANUAL_ASSOC_STRUCT *prManualAssoc;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct CMD_STAREC_UPDATE);
	if (u4SetBufferLen < sizeof(struct CMD_STAREC_UPDATE))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prStaRecManualAssoc = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				(CMD_STAREC_UPDATE_HDR_SIZE + u4SetBufferLen));
	if (!prStaRecManualAssoc) {
		DBGLOG(INIT, ERROR,
		       "Allocate P_CMD_STAREC_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	prManualAssoc = (struct CMD_MANUAL_ASSOC_STRUCT *)
			pvSetBuffer;
	prStaRecManualAssoc->ucWlanIdx = prManualAssoc->ucWtbl;
	prStaRecManualAssoc->ucBssIndex = prManualAssoc->ucOwnmac;
	prStaRecManualAssoc->u2TotalElementNum = 1;
	kalMemCopy(prStaRecManualAssoc->aucBuffer, pvSetBuffer,
		   u4SetBufferLen);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_STAREC_UPDATE,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     (CMD_STAREC_UPDATE_HDR_SIZE + u4SetBufferLen),
			     (uint8_t *) prStaRecManualAssoc, NULL, 0);

	cnmMemFree(prAdapter, prStaRecManualAssoc);

	return rWlanStatus;
}

#if CFG_SUPPORT_MU_MIMO
uint32_t
wlanoidMuMimoAction(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT
		*prMuMimoActionInfo;
	union CMD_MUMIMO_ACTION rCmdMuMimoActionInfo;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	u_int8_t fgSetQuery, fgNeedResp;
	uint32_t u4MuMimoCmdId;
	void (*pFunc)(struct ADAPTER *, struct CMD_INFO *,
		      uint8_t *);

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_MUMIMO_ACTION_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_MUMIMO_ACTION_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prMuMimoActionInfo = (struct
			      PARAM_CUSTOM_MUMIMO_ACTION_STRUCT *) pvSetBuffer;

	memcpy(&rCmdMuMimoActionInfo, prMuMimoActionInfo,
	       sizeof(union CMD_MUMIMO_ACTION));

	u4MuMimoCmdId = rCmdMuMimoActionInfo.ucMuMimoCategory;
	if (MU_CMD_NEED_TO_RESPONSE(u4MuMimoCmdId) == 0) {
		fgSetQuery = TRUE;
		fgNeedResp = FALSE;
	} else {
		fgSetQuery = FALSE;
		fgNeedResp = TRUE;
	}

	pFunc = nicCmdEventSetCommon;
	if (u4MuMimoCmdId == MU_HQA_GET_QD)
		pFunc = nicCmdEventGetQd;
	else if (u4MuMimoCmdId == MU_HQA_GET_CALC_LQ)
		pFunc = nicCmdEventGetCalcLq;
	else if (u4MuMimoCmdId == MU_GET_CALC_INIT_MCS)
		pFunc = nicCmdEventGetCalcInitMcs;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
					     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
					     EXT_CMD_ID_MU_CTRL,
					     fgSetQuery,
					     fgNeedResp,
					     TRUE,
					     pFunc,
					     nicOidCmdTimeoutCommon,
					     sizeof(union CMD_MUMIMO_ACTION),
					     (uint8_t *) &rCmdMuMimoActionInfo,
					     pvSetBuffer,
					     u4SetBufferLen);

	return rWlanStatus;
}
#endif /* CFG_SUPPORT_MU_MIMO */

#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_SUPPORT_TX_BF
uint32_t
wlanoidStaRecUpdate(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen) {
	struct CMD_STAREC_UPDATE *prStaRecUpdateInfo;
	struct STAREC_COMMON *prStaRecCmm;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct STAREC_COMMON);
	if (u4SetBufferLen < sizeof(struct STAREC_COMMON))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prStaRecUpdateInfo =
		(struct CMD_STAREC_UPDATE *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, (CMD_STAREC_UPDATE_HDR_SIZE +
					       u4SetBufferLen));
	if (!prStaRecUpdateInfo) {
		DBGLOG(INIT, ERROR,
		       "Allocate P_CMD_DEV_INFO_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* fix me: configurable ucBssIndex */
	prStaRecCmm = (struct STAREC_COMMON *) pvSetBuffer;
	prStaRecUpdateInfo->ucBssIndex = 0;
	prStaRecUpdateInfo->ucWlanIdx = (prStaRecCmm->u2ExtraInfo & 0xFF00) >> 8;
	prStaRecUpdateInfo->u2TotalElementNum = 1;
	kalMemCopy(prStaRecUpdateInfo->aucBuffer, pvSetBuffer,
		   u4SetBufferLen);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_STAREC_UPDATE,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     (CMD_STAREC_UPDATE_HDR_SIZE + u4SetBufferLen),
			     (uint8_t *) prStaRecUpdateInfo, NULL, 0);

	cnmMemFree(prAdapter, prStaRecUpdateInfo);

	return rWlanStatus;
}

uint32_t
wlanoidStaRecBFUpdate(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	struct CMD_STAREC_UPDATE *prStaRecUpdateInfo;
	struct CMD_STAREC_BF *prStaRecBF;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct CMD_STAREC_BF);
	if (u4SetBufferLen < sizeof(struct CMD_STAREC_BF))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prStaRecUpdateInfo =
		(struct CMD_STAREC_UPDATE *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, (CMD_STAREC_UPDATE_HDR_SIZE +
					       u4SetBufferLen));
	if (!prStaRecUpdateInfo) {
		DBGLOG(INIT, ERROR,
		       "Allocate P_CMD_DEV_INFO_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* fix me: configurable ucBssIndex */
	prStaRecBF = (struct CMD_STAREC_BF *) pvSetBuffer;
	prStaRecUpdateInfo->ucBssIndex = prStaRecBF->ucReserved[0];
	prStaRecUpdateInfo->ucWlanIdx = prStaRecBF->ucReserved[1];
	prStaRecUpdateInfo->u2TotalElementNum = 1;
	kalMemCopy(prStaRecUpdateInfo->aucBuffer, pvSetBuffer,
		   u4SetBufferLen);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_STAREC_UPDATE,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     (CMD_STAREC_UPDATE_HDR_SIZE + u4SetBufferLen),
			     (uint8_t *) prStaRecUpdateInfo, NULL, 0);

	cnmMemFree(prAdapter, prStaRecUpdateInfo);

	return rWlanStatus;
}

uint32_t
wlanoidStaRecBFRead(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint16_t wlan_id;
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_BF *uni_cmd;
	struct UNI_CMD_BF_STAREC_READ *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BF) +
	     		       sizeof(struct UNI_CMD_BF_STAREC_READ);

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(uint16_t);
	if (u4SetBufferLen < sizeof(uint16_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	wlan_id = *(uint16_t *) pvSetBuffer;
	uni_cmd = (struct UNI_CMD_BF *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_BF_STAREC_READ *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_STA_REC_READ;
	tag->u2Length = sizeof(*tag);
	tag->u2WlanIdx = wlan_id;

	DBGLOG(RFTEST, INFO, "Read StaRec WlanIdx=%d.\n", wlan_id);

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_BF,
			     FALSE,
			     TRUE,
			     TRUE,
			     nicUniEventBFStaRec,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

struct TXBF_CMD_DONE_HANDLER {
	uint32_t u4TxBfCmdId;
	void (*pFunc)(struct ADAPTER *, struct CMD_INFO *,
		      uint8_t *);
};

struct TXBF_CMD_DONE_HANDLER rTxBfCmdDoneHandler[] = {
	{BF_SOUNDING_OFF, nicCmdEventSetCommon},
	{BF_SOUNDING_ON, nicCmdEventSetCommon},
	{BF_DATA_PACKET_APPLY, nicCmdEventSetCommon},
	{BF_PFMU_MEM_ALLOCATE, nicCmdEventSetCommon},
	{BF_PFMU_MEM_RELEASE, nicCmdEventSetCommon},
	{BF_PFMU_TAG_READ, nicCmdEventPfmuTagRead},
	{BF_PFMU_TAG_WRITE, nicCmdEventSetCommon},
	{BF_PROFILE_READ, nicCmdEventPfmuDataRead},
	{BF_PROFILE_WRITE, nicCmdEventSetCommon},
	{BF_PN_READ, nicCmdEventSetCommon},
	{BF_PN_WRITE, nicCmdEventSetCommon},
	{BF_PFMU_MEM_ALLOC_MAP_READ, nicCmdEventSetCommon},
#if CFG_SUPPORT_TX_BF_FPGA
	{BF_PFMU_SW_TAG_WRITE, nicCmdEventSetCommon}
#endif
};

uint32_t
wlanoidTxBfAction(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen) {
	union PARAM_CUSTOM_TXBF_ACTION_STRUCT *prTxBfActionInfo;
	union CMD_TXBF_ACTION rCmdTxBfActionInfo;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	u_int8_t fgSetQuery, fgNeedResp;
	uint32_t u4TxBfCmdId;
	uint8_t  ucIdx;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(union
				PARAM_CUSTOM_TXBF_ACTION_STRUCT);

	if (u4SetBufferLen < sizeof(union
				    PARAM_CUSTOM_TXBF_ACTION_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prTxBfActionInfo = (union PARAM_CUSTOM_TXBF_ACTION_STRUCT *)
			   pvSetBuffer;

	memcpy(&rCmdTxBfActionInfo, prTxBfActionInfo,
	       sizeof(union CMD_TXBF_ACTION));

	u4TxBfCmdId =
		rCmdTxBfActionInfo.rProfileTagRead.ucTxBfCategory;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	/* unified command doesn't need to wait resposne */
	fgSetQuery = TRUE;
	fgNeedResp = FALSE;
#else
	if (TXBF_CMD_NEED_TO_RESPONSE(u4TxBfCmdId) ==
	    0) {	/* don't need response */
		fgSetQuery = TRUE;
		fgNeedResp = FALSE;
	} else {
		fgSetQuery = FALSE;
		fgNeedResp = TRUE;
	}
#endif

	for (ucIdx = 0; ucIdx < ARRAY_SIZE(rTxBfCmdDoneHandler);
	     ucIdx++) {
		if (u4TxBfCmdId == rTxBfCmdDoneHandler[ucIdx].u4TxBfCmdId)
			break;
	}

	if (ucIdx == ARRAY_SIZE(rTxBfCmdDoneHandler)) {
		DBGLOG(RFTEST, ERROR,
		       "ucIdx [%d] overrun of rTxBfCmdDoneHandler\n", ucIdx);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
					     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
					     EXT_CMD_ID_BF_ACTION,
					     fgSetQuery,
					     fgNeedResp,
					     TRUE,
					     rTxBfCmdDoneHandler[ucIdx].pFunc,
					     nicOidCmdTimeoutCommon,
					     sizeof(union CMD_TXBF_ACTION),
					     (uint8_t *) &rCmdTxBfActionInfo,
					     pvSetBuffer,
					     u4SetBufferLen);

	return rWlanStatus;
}
#endif /* CFG_SUPPORT_TX_BF */

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
uint32_t
wlanoidBssInfoBasicUnify(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen)
{
	struct UNI_BASIC_BSSINFO_UPDATE *prBssInfo;
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_BASIC *tag;
	struct BSS_INFO *bss;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
			       sizeof(struct UNI_CMD_BSSINFO_BASIC);

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = u4SetBufferLen;
	if (u4SetBufferLen < sizeof(struct UNI_BASIC_BSSINFO_UPDATE))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prBssInfo = (struct UNI_BASIC_BSSINFO_UPDATE *) pvSetBuffer;
	bss = GET_BSS_INFO_BY_INDEX(prAdapter, prBssInfo->ucBssIdx);
	uni_cmd = (struct UNI_CMD_BSSINFO *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BSSINFO ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}
	uni_cmd->ucBssInfoIdx = prBssInfo->ucBssIdx;
	tag = (struct UNI_CMD_BSSINFO_BASIC *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BSSINFO_TAG_BASIC;
	tag->u2Length = sizeof(*tag);
	tag->ucActive = TRUE;
	tag->ucOwnMacIdx = prBssInfo->ucOwnMacIdx;
	tag->ucHwBSSIndex = prBssInfo->ucBssIdx;
	tag->ucDbdcIdx = prBssInfo->ucBandIdx;
	tag->u4ConnectionType = CONNECTION_INFRA_STA;
	COPY_MAC_ADDR(tag->aucBSSID, prBssInfo->ucBssId);
	tag->u2BcMcWlanidx = prBssInfo->ucBssIdx;
	tag->u2BcnInterval = 100;
	tag->ucDtimPeriod = 1;

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_BSSINFO,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
}
#endif


uint32_t
wlanoidBssInfoBasic(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen) {
	struct CMD_BSS_INFO_UPDATE *prBssInfoUpdateBasic;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct BSSINFO_BASIC *prBssinfoBasic = NULL;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct BSSINFO_BASIC);
	if (u4SetBufferLen < sizeof(struct BSSINFO_BASIC))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prBssInfoUpdateBasic = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			   (CMD_BSSINFO_UPDATE_HDR_SIZE + u4SetBufferLen));
	if (!prBssInfoUpdateBasic) {
		DBGLOG(INIT, ERROR,
		       "Allocate P_CMD_DEV_INFO_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	if (pvSetBuffer == NULL) {
		prBssInfoUpdateBasic->ucBssIndex = 0;
		DBGLOG(RFTEST, INFO,
			"prBssInfoUpdateBasic->ucBssIndex=0(default)\n");
	} else {
		prBssinfoBasic =
			(struct BSSINFO_BASIC *)(pvSetBuffer);
		prBssInfoUpdateBasic->ucBssIndex =
			prBssinfoBasic->ucBcMcWlanidx;
		DBGLOG(RFTEST, INFO,
			"prBssInfoUpdateBasic->ucBssIndex =%d\n",
			prBssInfoUpdateBasic->ucBssIndex);
	}
	prBssInfoUpdateBasic->u2TotalElementNum = 1;
	kalMemCopy(prBssInfoUpdateBasic->aucBuffer, pvSetBuffer,
		   u4SetBufferLen);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_BSSINFO_UPDATE,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     (CMD_BSSINFO_UPDATE_HDR_SIZE + u4SetBufferLen),
			     (uint8_t *) prBssInfoUpdateBasic, NULL, 0);

	cnmMemFree(prAdapter, prBssInfoUpdateBasic);

	return rWlanStatus;
}

uint32_t
wlanoidBssInfoConOwnDev(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen) {
	struct CMD_BSS_INFO_UPDATE *prBssInfoUpdateBasic;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct BSSINFO_CONNECT_OWN_DEV *prBssinfoConOwnDev = NULL;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct BSSINFO_CONNECT_OWN_DEV);
	if (u4SetBufferLen < sizeof(struct BSSINFO_CONNECT_OWN_DEV))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prBssInfoUpdateBasic = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			   (CMD_BSSINFO_UPDATE_HDR_SIZE + u4SetBufferLen));
	if (!prBssInfoUpdateBasic) {
		DBGLOG(INIT, ERROR,
		       "Allocate P_CMD_DEV_INFO_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	if (pvSetBuffer == NULL) {
		prBssInfoUpdateBasic->ucBssIndex = 0;
		DBGLOG(RFTEST, INFO,
			"prBssInfoUpdateBasic->ucBssIndex=0(default)\n");
	} else {
		prBssinfoConOwnDev =
			(struct BSSINFO_CONNECT_OWN_DEV *)(pvSetBuffer);
		prBssInfoUpdateBasic->ucBssIndex =
			prBssinfoConOwnDev->ucHwBSSIndex;
		DBGLOG(RFTEST, INFO,
			"prBssInfoUpdateBasic->ucBssIndex =%d\n",
			prBssInfoUpdateBasic->ucBssIndex);
	}
	prBssInfoUpdateBasic->u2TotalElementNum = 1;
	kalMemCopy(prBssInfoUpdateBasic->aucBuffer, pvSetBuffer,
		   u4SetBufferLen);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_BSSINFO_UPDATE,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     (CMD_BSSINFO_UPDATE_HDR_SIZE + u4SetBufferLen),
			     (uint8_t *) prBssInfoUpdateBasic, NULL, 0);

	cnmMemFree(prAdapter, prBssInfoUpdateBasic);

	return rWlanStatus;
}

uint32_t
wlanoidDevInfoActive(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	struct CMD_DEV_INFO_UPDATE *prDevInfoUpdateActive;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct CMD_DEVINFO_ACTIVE *prCmdDevinfoActive = NULL;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct CMD_DEVINFO_ACTIVE);
	if (u4SetBufferLen < sizeof(struct CMD_DEVINFO_ACTIVE))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prDevInfoUpdateActive = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			    (CMD_DEVINFO_UPDATE_HDR_SIZE + u4SetBufferLen));
	if (!prDevInfoUpdateActive) {
		DBGLOG(INIT, ERROR,
		       "Allocate P_CMD_DEV_INFO_UPDATE_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* fix me: configurable ucOwnMacIdx */
	if (pvSetBuffer == NULL) {
		prDevInfoUpdateActive->ucOwnMacIdx = 0;
		DBGLOG(RFTEST, INFO,
			"prDevInfoUpdateActive->ucOwnMacIdx = 0(default)\n");
	} else {
		prCmdDevinfoActive =
			(struct CMD_DEVINFO_ACTIVE *)pvSetBuffer;
		prDevInfoUpdateActive->ucOwnMacIdx =
			prCmdDevinfoActive->ucOwnMacIdx;
		DBGLOG(RFTEST, INFO,
			"prDevInfoUpdateActive->ucOwnMacIdx = %d\n",
			prDevInfoUpdateActive->ucOwnMacIdx);
	}
	prDevInfoUpdateActive->ucDbdcIdx =
			prCmdDevinfoActive->ucBandNum;
	prDevInfoUpdateActive->ucAppendCmdTLV = 0;
	prDevInfoUpdateActive->u2TotalElementNum = 1;
	kalMemCopy(prDevInfoUpdateActive->aucBuffer, pvSetBuffer,
		   u4SetBufferLen);

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_DEVINFO_UPDATE,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     (CMD_DEVINFO_UPDATE_HDR_SIZE + u4SetBufferLen),
			     (uint8_t *) prDevInfoUpdateActive, NULL, 0);

	cnmMemFree(prAdapter, prDevInfoUpdateActive);

	return rWlanStatus;
}

uint32_t
wlanoidInitAisFsm(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint8_t ucAisIndex;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (!prAdapter || !pvSetBuffer || u4SetBufferLen < sizeof(uint8_t)) {
		DBGLOG(REQ, WARN, "AIS Uninit Error, %p %p",
			prAdapter, pvSetBuffer);
		return WLAN_STATUS_FAILURE;
	}

	ucAisIndex = *((uint8_t *)pvSetBuffer);

	aisFsmInit(prAdapter, &prAdapter->prGlueInfo->rRegInfo, ucAisIndex);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidUninitAisFsm(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint8_t ucAisIndex;

	if (!prAdapter || !pvSetBuffer || u4SetBufferLen < sizeof(uint8_t)) {
		DBGLOG(REQ, WARN, "AIS Uninit Error, %p %p",
			prAdapter, pvSetBuffer);
		return WLAN_STATUS_FAILURE;
	}

	ucAisIndex = *((uint8_t *)pvSetBuffer);

	aisFsmUninit(prAdapter, ucAisIndex);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_SMART_GEAR
uint32_t
wlandioSetSGStatus(struct ADAPTER *prAdapter,
			uint8_t ucSGEnable,
			uint8_t ucSGSpcCmd,
			uint8_t ucNSS)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct CMD_SMART_GEAR_PARAM *prCmdSGStatus;

	prCmdSGStatus = (struct CMD_SMART_GEAR_PARAM *)cnmMemAlloc(prAdapter,
					RAM_TYPE_MSG,
					sizeof(struct CMD_SMART_GEAR_PARAM));

	if (!prCmdSGStatus) {
		DBGLOG(SW4, ERROR,
			"[SG]cnmMemAlloc for wlandioSetSGStatus failed!\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	prCmdSGStatus->ucSGEnable = ucSGEnable;
	prCmdSGStatus->ucSGSpcCmd = ucSGSpcCmd;

	prCmdSGStatus->ucSGCfg = 0xFF;

	if (ucSGSpcCmd == 0xFF) {
		prCmdSGStatus->ucSGCfg = prAdapter->rWifiVar.ucSGCfg;
		prCmdSGStatus->ucNSSCap = ucNSS;
		prCmdSGStatus->ucSG24GFavorANT =
				prAdapter->rWifiVar.ucSG24GFavorANT;
		prCmdSGStatus->ucSG5GFavorANT =
				prAdapter->rWifiVar.ucSG5GFavorANT;
	}

	DBGLOG(SW4, INFO,
			"[SG]Status[%d][%d][%d][%d][%d]\n",
			prCmdSGStatus->ucSGEnable, prCmdSGStatus->ucSGSpcCmd,
			prCmdSGStatus->ucNSSCap, prCmdSGStatus->ucSG24GFavorANT,
			prCmdSGStatus->ucSG5GFavorANT);

	wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SG_PARAM,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(*prCmdSGStatus),
			(uint8_t *) prCmdSGStatus, NULL, 0);

	cnmMemFree(prAdapter, prCmdSGStatus);
	return rWlanStatus;

}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query MCR value.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryMcrRead(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrRdInfo;
	struct CMD_ACCESS_REG rCmdAccessReg;
	struct mt66xx_chip_info *prChipInfo = NULL;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_MCR_RW_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prMcrRdInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)
		      pvQueryBuffer;

	/* 0x9000 - 0x9EFF reserved for FW */
#if CFG_SUPPORT_SWCR
	if ((prMcrRdInfo->u4McrOffset >> 16) == 0x9F00) {
		swCrReadWriteCmd(prAdapter, SWCR_READ,
			 (uint16_t) (prMcrRdInfo->u4McrOffset & BITS(0, 15)),
			 &prMcrRdInfo->u4McrData);
		return WLAN_STATUS_SUCCESS;
	}
#endif /* CFG_SUPPORT_SWCR */

	/* Check if access F/W Domain MCR (due to WiFiSYS is placed from
	 * 0x6000-0000
	 */
	if (prMcrRdInfo->u4McrOffset & 0xFFFF0000) {
		/* fill command */
		rCmdAccessReg.u4Address = prMcrRdInfo->u4McrOffset;
		rCmdAccessReg.u4Data = 0;

		return wlanSendSetQueryCmd(prAdapter,
					   CMD_ID_ACCESS_REG,
					   FALSE,
					   TRUE,
					   TRUE,
					   nicCmdEventQueryMcrRead,
					   nicOidCmdTimeoutCommon,
					   sizeof(struct CMD_ACCESS_REG),
					   (uint8_t *) &rCmdAccessReg,
					   pvQueryBuffer,
					   u4QueryBufferLen);
	} else {
		if (prMcrRdInfo->u4McrOffset == 0 &&
		    prChipInfo->asicGetChipID) {
			prMcrRdInfo->u4McrData =
				prChipInfo->asicGetChipID(prAdapter);
			log_dbg(INIT, INFO,
				"Get Chip ID [0x%08x] from FW\n",
				prMcrRdInfo->u4McrData);
		} else {
			ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
			HAL_RMCR_RD(OID_DBG, prAdapter,
				/* address is in DWORD unit */
				(prMcrRdInfo->u4McrOffset & BITS(2, 31)),
				   &prMcrRdInfo->u4McrData);
			RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

			DBGLOG(INIT, TRACE,
			       "MCR Read: Offset = %#08x, Data = %#08x\n",
			       prMcrRdInfo->u4McrOffset,
			       prMcrRdInfo->u4McrData);
		}
		return WLAN_STATUS_SUCCESS;
	}
}				/* end of wlanoidQueryMcrRead() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to write MCR and enable specific function.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetMcrWrite(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrWrInfo;
	struct CMD_ACCESS_REG rCmdAccessReg;
#if CFG_STRESS_TEST_SUPPORT
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prBssInfo =
		aisGetAisBssInfo(prAdapter, ucBssIndex);
	struct STA_RECORD *prStaRec = prBssInfo->prStaRecOfAP;
	uint32_t u4McrOffset, u4McrData;
#endif

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prMcrWrInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)
		      pvSetBuffer;

	/* 0x9000 - 0x9EFF reserved for FW */
	/* 0xFFFE          reserved for FW */

	/* -- Puff Stress Test Begin */
#if CFG_STRESS_TEST_SUPPORT

	/* 0xFFFFFFFE for Control Rate */
	if (prMcrWrInfo->u4McrOffset == 0xFFFFFFFE) {
		if (prMcrWrInfo->u4McrData < FIXED_RATE_NUM
		    && prMcrWrInfo->u4McrData > 0)
			prAdapter->rWifiVar.eRateSetting =
						(enum ENUM_REGISTRY_FIXED_RATE)
						(prMcrWrInfo->u4McrData);
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
		DBGLOG(INIT, TRACE,
		       "[Stress Test] Rate is Changed to index %d...\n",
		       prAdapter->rWifiVar.eRateSetting);
	}
	/* 0xFFFFFFFD for Switch Channel */
	else if (prMcrWrInfo->u4McrOffset == 0xFFFFFFFD) {
		if (prMcrWrInfo->u4McrData <= 11
		    && prMcrWrInfo->u4McrData >= 1)
			prBssInfo->ucPrimaryChannel = prMcrWrInfo->u4McrData;
		nicUpdateBss(prAdapter, prBssInfo->ucNetTypeIndex);
		DBGLOG(INIT, TRACE,
		       "[Stress Test] Channel is switched to %d ...\n",
		       prBssInfo->ucPrimaryChannel);

		return WLAN_STATUS_SUCCESS;
	}
	/* 0xFFFFFFFFC for Control RF Band and SCO */
	else if (prMcrWrInfo->u4McrOffset == 0xFFFFFFFC) {
		/* Band */
		if (prMcrWrInfo->u4McrData & 0x80000000) {
		    /* prBssInfo->eBand = BAND_5G;
		     * prBssInfo->ucPrimaryChannel = 52; // Bond to Channel 52
		     */
		} else {
		    prBssInfo->eBand = BAND_2G4;
		    prBssInfo->ucPrimaryChannel = 8; /* Bond to Channel 6 */
		}

		/* Bandwidth */
		if (prMcrWrInfo->u4McrData & 0x00010000) {
			prStaRec->u2HtCapInfo |= HT_CAP_INFO_SUP_CHNL_WIDTH;
			prStaRec->ucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;

			if (prMcrWrInfo->u4McrData == 0x00010002) {
				prBssInfo->eBssSCO = CHNL_EXT_SCB; /* U20 */
				prBssInfo->ucPrimaryChannel += 2;
			} else if (prMcrWrInfo->u4McrData == 0x00010001) {
				prBssInfo->eBssSCO = CHNL_EXT_SCA; /* L20 */
				prBssInfo->ucPrimaryChannel -= 2;
			} else {
				prBssInfo->eBssSCO = CHNL_EXT_SCA; /* 40 */
			}
		}

		rlmBssInitForAPandIbss(prAdapter, prBssInfo);
	}
	/* 0xFFFFFFFB for HT Capability */
	else if (prMcrWrInfo->u4McrOffset == 0xFFFFFFFB) {
		/* Enable HT Capability */
		if (prMcrWrInfo->u4McrData & 0x00000001) {
			prStaRec->u2HtCapInfo |= HT_CAP_INFO_HT_GF;
		} else {
			prStaRec->u2HtCapInfo &= (~HT_CAP_INFO_HT_GF);
		}
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
	}
	/* 0xFFFFFFFA for Enable Random Rx Reset */
	else if (prMcrWrInfo->u4McrOffset == 0xFFFFFFFA) {
		rCmdAccessReg.u4Address = prMcrWrInfo->u4McrOffset;
		rCmdAccessReg.u4Data = prMcrWrInfo->u4McrData;

		return wlanSendSetQueryCmd(prAdapter,
					   CMD_ID_RANDOM_RX_RESET_EN,
					   TRUE,
					   FALSE,
					   TRUE,
					   nicCmdEventSetCommon,
					   nicOidCmdTimeoutCommon,
					   sizeof(struct CMD_ACCESS_REG),
					   (uint8_t *) &rCmdAccessReg,
					   pvSetBuffer, u4SetBufferLen);
	}
	/* 0xFFFFFFF9 for Disable Random Rx Reset */
	else if (prMcrWrInfo->u4McrOffset == 0xFFFFFFF9) {
		rCmdAccessReg.u4Address = prMcrWrInfo->u4McrOffset;
		rCmdAccessReg.u4Data = prMcrWrInfo->u4McrData;

		return wlanSendSetQueryCmd(prAdapter,
					   CMD_ID_RANDOM_RX_RESET_DE,
					   TRUE,
					   FALSE,
					   TRUE,
					   nicCmdEventSetCommon,
					   nicOidCmdTimeoutCommon,
					   sizeof(struct CMD_ACCESS_REG),
					   (uint8_t *) &rCmdAccessReg,
					   pvSetBuffer, u4SetBufferLen);
	}
	/* 0xFFFFFFF8 for Enable SAPP */
	else if (prMcrWrInfo->u4McrOffset == 0xFFFFFFF8) {
		rCmdAccessReg.u4Address = prMcrWrInfo->u4McrOffset;
		rCmdAccessReg.u4Data = prMcrWrInfo->u4McrData;

		return wlanSendSetQueryCmd(prAdapter,
					   CMD_ID_SAPP_EN,
					   TRUE,
					   FALSE,
					   TRUE,
					   nicCmdEventSetCommon,
					   nicOidCmdTimeoutCommon,
					   sizeof(struct CMD_ACCESS_REG),
					   (uint8_t *) &rCmdAccessReg,
					   pvSetBuffer, u4SetBufferLen);
	}
	/* 0xFFFFFFF7 for Disable SAPP */
	else if (prMcrWrInfo->u4McrOffset == 0xFFFFFFF7) {
		rCmdAccessReg.u4Address = prMcrWrInfo->u4McrOffset;
		rCmdAccessReg.u4Data = prMcrWrInfo->u4McrData;

		return wlanSendSetQueryCmd(prAdapter,
					   CMD_ID_SAPP_DE,
					   TRUE,
					   FALSE,
					   TRUE,
					   nicCmdEventSetCommon,
					   nicOidCmdTimeoutCommon,
					   sizeof(struct CMD_ACCESS_REG),
					   (uint8_t *) &rCmdAccessReg,
					   pvSetBuffer, u4SetBufferLen);
	}

	else
#endif
		/* -- Puff Stress Test End */

		/* Check if access F/W Domain MCR */
		if (prMcrWrInfo->u4McrOffset & 0xFFFF0000) {

			/* 0x9000 - 0x9EFF reserved for FW */
#if CFG_SUPPORT_SWCR
			if ((prMcrWrInfo->u4McrOffset >> 16) == 0x9F00) {
				swCrReadWriteCmd(prAdapter, SWCR_WRITE,
					(uint16_t)(prMcrWrInfo->u4McrOffset &
								BITS(0, 15)),
					&prMcrWrInfo->u4McrData);
				return WLAN_STATUS_SUCCESS;
			}
#endif /* CFG_SUPPORT_SWCR */

#if 1
			/* low power test special command */
			if (prMcrWrInfo->u4McrOffset == 0x11111110) {
				uint32_t rStatus = WLAN_STATUS_SUCCESS;
				/* DbgPrint("Enter test mode\n"); */
				prAdapter->fgTestMode = TRUE;
				return rStatus;
			}
			if (prMcrWrInfo->u4McrOffset == 0x11111111) {
				/* DbgPrint("nicpmSetAcpiPowerD3\n"); */

				nicpmSetAcpiPowerD3(prAdapter);
				kalDevSetPowerState(prAdapter->prGlueInfo,
					    (uint32_t) ParamDeviceStateD3);
				return WLAN_STATUS_SUCCESS;
			}
			if (prMcrWrInfo->u4McrOffset == 0x11111112) {

				/* DbgPrint("LP enter sleep\n"); */

				/* fill command */
				rCmdAccessReg.u4Address =
						prMcrWrInfo->u4McrOffset;
				rCmdAccessReg.u4Data =
						prMcrWrInfo->u4McrData;

				return wlanSendSetQueryCmd(prAdapter,
						CMD_ID_ACCESS_REG,
						TRUE,
						FALSE,
						TRUE,
						nicCmdEventSetCommon,
						nicOidCmdTimeoutCommon,
						sizeof(struct CMD_ACCESS_REG),
						(uint8_t *) &rCmdAccessReg,
						pvSetBuffer, u4SetBufferLen);
			}
#endif

#if 1
			/* low power test special command */
			if (prMcrWrInfo->u4McrOffset == 0x11111110) {
				uint32_t rStatus = WLAN_STATUS_SUCCESS;
				/* DbgPrint("Enter test mode\n"); */
				prAdapter->fgTestMode = TRUE;
				return rStatus;
			}
			if (prMcrWrInfo->u4McrOffset == 0x11111111) {
				/* DbgPrint("nicpmSetAcpiPowerD3\n"); */

				nicpmSetAcpiPowerD3(prAdapter);
				kalDevSetPowerState(prAdapter->prGlueInfo,
						(uint32_t) ParamDeviceStateD3);
				return WLAN_STATUS_SUCCESS;
			}
			if (prMcrWrInfo->u4McrOffset == 0x11111112) {

				/* DbgPrint("LP enter sleep\n"); */

				/* fill command */
				rCmdAccessReg.u4Address =
						prMcrWrInfo->u4McrOffset;
				rCmdAccessReg.u4Data =
						prMcrWrInfo->u4McrData;

				return wlanSendSetQueryCmd(prAdapter,
						CMD_ID_ACCESS_REG,
						TRUE,
						FALSE,
						TRUE,
						nicCmdEventSetCommon,
						nicOidCmdTimeoutCommon,
						sizeof(struct CMD_ACCESS_REG),
						(uint8_t *) &rCmdAccessReg,
						pvSetBuffer, u4SetBufferLen);
			}
#endif

#if CFG_SUPPORT_SDIO_READ_WRITE_PATTERN
			if (prMcrWrInfo->u4McrOffset == 0x22220000) {
				/* read test mode */
				kalSetSdioTestPattern(prAdapter->prGlueInfo,
								TRUE, TRUE);

				return WLAN_STATUS_SUCCESS;
			}

			if (prMcrWrInfo->u4McrOffset == 0x22220001) {
				/* write test mode */
				kalSetSdioTestPattern(prAdapter->prGlueInfo,
								TRUE, FALSE);

				return WLAN_STATUS_SUCCESS;
			}

			if (prMcrWrInfo->u4McrOffset == 0x22220002) {
				/* leave from test mode */
				kalSetSdioTestPattern(prAdapter->prGlueInfo,
								FALSE, FALSE);

				return WLAN_STATUS_SUCCESS;
			}
#endif

			/* fill command */
			rCmdAccessReg.u4Address = prMcrWrInfo->u4McrOffset;
			rCmdAccessReg.u4Data = prMcrWrInfo->u4McrData;

			return wlanSendSetQueryCmd(prAdapter,
					   CMD_ID_ACCESS_REG,
					   TRUE,
					   FALSE,
					   TRUE,
					   nicCmdEventSetCommon,
					   nicOidCmdTimeoutCommon,
					   sizeof(struct CMD_ACCESS_REG),
					   (uint8_t *) &rCmdAccessReg,
					   pvSetBuffer, u4SetBufferLen);
		} else {
			HAL_MCR_WR(prAdapter, (prMcrWrInfo->u4McrOffset &
				BITS(2, 31)),	/* address is in DWORD unit */
				prMcrWrInfo->u4McrData);

			DBGLOG(INIT, TRACE,
			       "MCR Write: Offset = %#08x, Data = %#08x\n",
			       prMcrWrInfo->u4McrOffset,
			       prMcrWrInfo->u4McrData);

			return WLAN_STATUS_SUCCESS;
		}
}				/* wlanoidSetMcrWrite */

#if CFG_SUPPORT_WIFI_ICCM
uint32_t
wlanoidSetIccm(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen)
{
	struct PARAM_CUSTOM_ICCM_STRUCT *prIccmInfo;
	struct CMD_ICCM_INFO_T rCmdIccm;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_ICCM_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_ICCM_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prIccmInfo = (struct PARAM_CUSTOM_ICCM_STRUCT *)
		      pvSetBuffer;

	rCmdIccm.u4Enable = prIccmInfo->u4Enable;
	rCmdIccm.u4EnablePrintFw = prIccmInfo->u4EnablePrintFw;
	rCmdIccm.u4Value = prIccmInfo->u4Value;

	return wlanSendSetQueryCmd(prAdapter,
					CMD_ID_SET_ICCM,
					TRUE,
					FALSE,
					FALSE,
					NULL,
					NULL,
					sizeof(struct CMD_ICCM_INFO_T),
					(uint8_t *) &rCmdIccm,
					pvSetBuffer, u4SetBufferLen);
}	/* wlanoidSetIccm */
#endif

#if CFG_SUPPORT_WIFI_POWER_METRICS
uint32_t
wlanoidSetPowerMetrics(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_POWER_METRICS_STRUCT *prPwrMetInfo;
	struct CMD_POWER_METRICS_INFO_T rCmdPowerMetrics;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_POWER_METRICS_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_POWER_METRICS_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prPwrMetInfo = (struct PARAM_CUSTOM_POWER_METRICS_STRUCT *)
		      pvSetBuffer;

	rCmdPowerMetrics.u4Enable = prPwrMetInfo->u4Enable;
	rCmdPowerMetrics.u4Value = prPwrMetInfo->u4Value;

	return wlanSendSetQueryCmd(prAdapter,
					CMD_ID_POWER_METRICS,
					TRUE,
					FALSE,
					FALSE,
					NULL,
					NULL,
					sizeof(struct CMD_POWER_METRICS_INFO_T),
					(uint8_t *) &rCmdPowerMetrics,
					pvSetBuffer, u4SetBufferLen);
}	/* wlanoidSetPowerMetrics */
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query driver MCR value.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryDrvMcrRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrRdInfo;
	/* CMD_ACCESS_REG rCmdAccessReg; */

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_MCR_RW_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prMcrRdInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)
		      pvQueryBuffer;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
	HAL_RMCR_RD(OID_DBG, prAdapter,
		       (prMcrRdInfo->u4McrOffset & BITS(2, 31)),
		       &prMcrRdInfo->u4McrData);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	DBGLOG(INIT, TRACE,
	       "DRV MCR Read: Offset = %#08x, Data = %#08x\n",
	       prMcrRdInfo->u4McrOffset, prMcrRdInfo->u4McrData);

	return WLAN_STATUS_SUCCESS;

}				/* end of wlanoidQueryMcrRead() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to write MCR and enable specific function.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetDrvMcrWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrWrInfo;
	/* CMD_ACCESS_REG rCmdAccessReg;  */

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prMcrWrInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)
		      pvSetBuffer;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
	HAL_MCR_WR(prAdapter, (prMcrWrInfo->u4McrOffset & BITS(2,
			       31)), prMcrWrInfo->u4McrData);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	DBGLOG(INIT, TRACE,
	       "DRV MCR Write: Offset = %#08x, Data = %#08x\n",
	       prMcrWrInfo->u4McrOffset, prMcrWrInfo->u4McrData);

	return WLAN_STATUS_SUCCESS;
}				/* wlanoidSetMcrWrite */

#if CFG_SUPPORT_WED_PROXY
uint32_t
wlanoidQueryDrvMcrReadDirectly(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen)
{
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrRdInfo;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT);
	if (u4QueryBufferLen < sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prMcrRdInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)pvQueryBuffer;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
	kalDevRegReadDirectly(prAdapter->prGlueInfo,
			      (prMcrRdInfo->u4McrOffset & BITS(2, 31)),
			      &prMcrRdInfo->u4McrData);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	DBGLOG(INIT, TRACE,
	       "DRV MCR Read: Offset = %#08x, Data = %#08x\n",
	       prMcrRdInfo->u4McrOffset, prMcrRdInfo->u4McrData);

	return WLAN_STATUS_SUCCESS;
}  /* end of wlanoidQueryMcrRead() */

uint32_t
wlanoidSetDrvMcrWriteDirectly(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrWrInfo;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT);
	if (u4SetBufferLen < sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prMcrWrInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)pvSetBuffer;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
	kalDevRegWriteDirectly(prAdapter->prGlueInfo,
			       prMcrWrInfo->u4McrOffset & BITS(2, 31),
			       prMcrWrInfo->u4McrData);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	DBGLOG(INIT, TRACE,
	       "DRV MCR Write: Offset = %#08x, Data = %#08x\n",
	       prMcrWrInfo->u4McrOffset, prMcrWrInfo->u4McrData);

	return WLAN_STATUS_SUCCESS;
}  /* wlanoidSetMcrWrite */
#endif

#if CFG_MTK_WIFI_SW_EMI_RING
uint32_t wlanoidQueryEmiMcrRead(
	struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen)
{
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	struct BUS_INFO *prBusInfo;
	struct SW_EMI_RING_INFO *prSwEmiRingInfo;
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrRdInfo;
	u_int8_t fgRet = FALSE;

	prBusInfo = prAdapter->chip_info->bus_info;
	prSwEmiRingInfo = &prBusInfo->rSwEmiRingInfo;

	if (!prSwEmiRingInfo->fgIsEnable)
		return WLAN_STATUS_NOT_ACCEPTED;

	*pu4QueryInfoLen = sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT);
	if (u4QueryBufferLen < sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prMcrRdInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)pvQueryBuffer;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
	HAL_MCR_EMI_RD(prAdapter,
		       prMcrRdInfo->u4McrOffset & BITS(2, 31),
		       &prMcrRdInfo->u4McrData, &fgRet);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	DBGLOG(INIT, TRACE,
	       "EMI MCR Read: Offset = %#08x, Data = %#08x\n",
	       prMcrRdInfo->u4McrOffset, prMcrRdInfo->u4McrData);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_ACCEPTED;
#endif
}
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query driver MCR value through UHW.
 *        This routine take effects only on chip supporting both USB and UHW.
 *        The userspace command format is
 *                $ iwpriv wlan0 driver "get_uhw_mcr <CR_ADDR>".
 *        Please do not use this command on chip without both USB and UHW.
 *        Comparison between get get_mcr, get_drv_mcr, get_uhw_mcr commands:
 *        1. get_mcr command is encapsulated as fw command. The recipient is
 *           mcu. You can read any CR as long as mcu can access it.
 *        2. get_drv_mcr command is sent differently, depending on HIF. Ex:
 *           It's sent on EP0 control pipe and the recipient is mcu if applying
 *           on USB.
 *        3. get_uhw_mcr command is sent on USB EP0 control pipe. The recipient
 *           is UHW without mcu intervention. You can read any CR as long as
 *           UHW can access it. In buzzard, UHW can access CBTOP.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryUhwMcrRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrRdInfo;
	u_int8_t fgStatus = FALSE;
	uint32_t u4WlanStatus = WLAN_STATUS_SUCCESS;
	/* CMD_ACCESS_REG rCmdAccessReg; */

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_MCR_RW_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prMcrRdInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)
		      pvQueryBuffer;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
	HAL_UHW_RD(prAdapter, (prMcrRdInfo->u4McrOffset & BITS(2, 31)),
		   &prMcrRdInfo->u4McrData, &fgStatus);
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	if (fgStatus)
		DBGLOG(OID, TRACE,
		       "DRV MCR Read: Offset = %#08x, Data = %#08x\n",
		       prMcrRdInfo->u4McrOffset, prMcrRdInfo->u4McrData);
	else {
		u4WlanStatus = WLAN_STATUS_FAILURE;

		DBGLOG(OID, WARN, "UHW read fail\n");
	}

	return u4WlanStatus;
}				/* end of wlanoidQueryUhwMcrRead() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to write MCR through UHW.
 *        This routine take effects only on chip supporting both USB and UHW.
 *        The userspace command format is
 *                $ iwpriv wlan0 driver "set_uhw_mcr <CR_ADDR> <CR_VALUE>".
 *        Please do not use this command on chip without both USB and UHW.
 *        Comparison between get set_mcr, set_drv_mcr, set_uhw_mcr commands can
 *        be refer to the description of wlanoidQueryUhwMcrRead.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetUhwMcrWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrWrInfo;
	u_int8_t fgStatus = FALSE;
	uint32_t u4WlanStatus = WLAN_STATUS_SUCCESS;
	/* CMD_ACCESS_REG rCmdAccessReg;  */

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_MCR_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prMcrWrInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)
		      pvSetBuffer;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
	HAL_UHW_WR(prAdapter, (prMcrWrInfo->u4McrOffset & BITS(2, 31)),
		   prMcrWrInfo->u4McrData, &fgStatus);

	if (fgStatus)
		DBGLOG(INIT, TRACE,
		       "DRV MCR Write: Offset = %#08x, Data = %#08x\n",
		       prMcrWrInfo->u4McrOffset, prMcrWrInfo->u4McrData);
	else {
		u4WlanStatus = WLAN_STATUS_FAILURE;

		DBGLOG(OID, WARN, "UHW write fail\n");
	}

	return u4WlanStatus;
}				/* wlanoidSetUhwMcrWrite */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query SW CTRL
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQuerySwCtrlRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_SW_CTRL_STRUCT *prSwCtrlInfo;
	uint32_t rWlanStatus;
	uint16_t u2Id, u2SubId;
	uint32_t u4Data;

	struct CMD_SW_DBG_CTRL rCmdSwCtrl;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_SW_CTRL_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_CUSTOM_SW_CTRL_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prSwCtrlInfo = (struct PARAM_CUSTOM_SW_CTRL_STRUCT *)
		       pvQueryBuffer;

	u2Id = (uint16_t) (prSwCtrlInfo->u4Id >> 16);
	u2SubId = (uint16_t) (prSwCtrlInfo->u4Id & BITS(0, 15));
	u4Data = 0;
	rWlanStatus = WLAN_STATUS_SUCCESS;

	switch (u2Id) {
		/* 0x9000 - 0x9EFF reserved for FW */
		/* 0xFFFE          reserved for FW */

#if CFG_SUPPORT_SWCR
	case 0x9F00:
		swCrReadWriteCmd(prAdapter, SWCR_READ /* Read */,
				 (uint16_t) u2SubId, &u4Data);
		break;
#endif /* CFG_SUPPORT_SWCR */

	case 0xFFFF: {
		u4Data = 0x5AA56620;
	}
	break;

	case 0xBABA:
		switch ((u2SubId >> 8) & BITS(0, 7)) {
		case 0x00:
			/* Dump Tx resource and queue status */
			qmDumpQueueStatus(prAdapter, NULL, 0);
			cnmDumpMemoryStatus(prAdapter, NULL, 0);
			break;

		case 0x01:
			/* Dump StaRec info by index */
			cnmDumpStaRec(prAdapter,
					(uint8_t) (u2SubId & BITS(0, 7)));
			break;

		case 0x02:
			/* Dump BSS info by index */
			bssDumpBssInfo(prAdapter,
				       (uint8_t) (u2SubId & BITS(0, 7)));
			break;

		case 0x03:
			/*Dump BSS statistics by index */
			wlanDumpBssStatistics(prAdapter,
					      (uint8_t) (u2SubId & BITS(0, 7)));
			break;

		case 0x04:
			halDumpHifStatus(prAdapter, NULL, 0);
			break;

		default:
			break;
		}

		u4Data = 0xBABABABA;
		break;

	case 0x9000:
	default: {
		kalMemSet(&rCmdSwCtrl, 0, sizeof(struct CMD_SW_DBG_CTRL));
		rCmdSwCtrl.u4Id = prSwCtrlInfo->u4Id;
		rCmdSwCtrl.u4Data = 0;
		rWlanStatus = wlanSendSetQueryCmd(prAdapter,
					CMD_ID_SW_DBG_CTRL,
					FALSE,
					TRUE,
					TRUE,
					nicCmdEventQuerySwCtrlRead,
					nicOidCmdTimeoutCommon,
					sizeof(struct CMD_SW_DBG_CTRL),
					(uint8_t *) &rCmdSwCtrl,
					pvQueryBuffer, u4QueryBufferLen);
		return rWlanStatus;
	}
	}			/* switch(u2Id) */

	prSwCtrlInfo->u4Data = u4Data;

	return rWlanStatus;

}

/* end of wlanoidQuerySwCtrlRead() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to write SW CTRL
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetSwCtrlWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_SW_CTRL_STRUCT *prSwCtrlInfo;
	struct CMD_SW_DBG_CTRL rCmdSwCtrl;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint16_t u2Id, u2SubId;
	uint32_t u4Data;
	uint8_t ucOpRxNss, ucOpTxNss;
	uint8_t ucChannelWidth;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_SW_CTRL_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_SW_CTRL_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prSwCtrlInfo = (struct PARAM_CUSTOM_SW_CTRL_STRUCT *)
		       pvSetBuffer;

	u2Id = (uint16_t) (prSwCtrlInfo->u4Id >> 16);
	u2SubId = (uint16_t) (prSwCtrlInfo->u4Id & BITS(0, 15));
	u4Data = prSwCtrlInfo->u4Data;

	switch (u2Id) {

		/* 0x9000 - 0x9EFF reserved for FW */
		/* 0xFFFE          reserved for FW */

#if CFG_SUPPORT_SWCR
	case 0x9F00:
		swCrReadWriteCmd(prAdapter, SWCR_WRITE, (uint16_t) u2SubId,
				 &u4Data);
		break;
#endif /* CFG_SUPPORT_SWCR */

	case 0x2222:
		ucOpRxNss = (uint8_t)(u4Data & BITS(0, 3));
		ucOpTxNss = (uint8_t)((u4Data & BITS(4, 7)) >> 4);
		ucChannelWidth = (uint8_t)((u4Data & BITS(8, 11)) >> 8);
		ucBssIndex = (uint8_t) u2SubId;

		if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
			DBGLOG(RLM, ERROR,
				"Invalid bssidx:%d\n", ucBssIndex);
			break;
		}

		if ((u2SubId & BITS(8, 15)) != 0) { /* Debug OP change
						     * parameters
						     */
			DBGLOG(RLM, INFO,
			       "[UT_OP] BSS[%d] IsBwChange[%d] BW[%d] IsRxNssChange[%d] RxNss[%d]",
			       ucBssIndex,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       fgIsOpChangeChannelWidth,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucOpChangeChannelWidth,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       fgIsOpChangeRxNss,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucOpChangeRxNss
			       );
			DBGLOG(RLM, INFO,
			       " IsTxNssChange[%d] TxNss[%d]\n",
			       prAdapter->aprBssInfo[ucBssIndex]->
			       fgIsOpChangeTxNss,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucOpChangeTxNss
			       );

			DBGLOG(RLM, INFO,
			       "[UT_OP] current OP mode: w[%d] s1[%d] s2[%d] sco[%d] RxNss[%d] TxNss[%d]\n",
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucVhtChannelWidth,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucVhtChannelFrequencyS1,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucVhtChannelFrequencyS2,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       eBssSCO,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucOpRxNss,
			       prAdapter->aprBssInfo[ucBssIndex]->
			       ucOpTxNss);
		} else {
			/* ucChannelWidth 0:20MHz, 1:40MHz, 2:80MHz, 3:160MHz
			 *                4:80+80MHz
			 */
			DBGLOG(RLM, INFO,
				"[UT_OP] Change BSS[%d] OpMode to BW[%d] RxNss[%d] TxNss[%d]\n",
				ucBssIndex, ucChannelWidth,
				ucOpRxNss, ucOpTxNss);
			rlmChangeOperationMode(
				prAdapter, ucBssIndex, ucChannelWidth,
				ucOpRxNss, ucOpTxNss,
				OP_CHANGE_SEND_ACT_DEFAULT,
				rlmDummyChangeOpHandler);
		}
		break;

	case 0x1000:
		if (u2SubId == 0x8000) {
			/* CTIA power save mode setting (code: 0x10008000) */
			prAdapter->u4CtiaPowerMode = u4Data;
			prAdapter->fgEnCtiaPowerMode = TRUE;

			/*  */
			{
				enum PARAM_POWER_MODE ePowerMode;

				if (prAdapter->u4CtiaPowerMode == 0)
					/* force to keep in CAM mode */
					ePowerMode = Param_PowerModeCAM;
				else if (prAdapter->u4CtiaPowerMode == 1)
					ePowerMode = Param_PowerModeMAX_PSP;
				else
					ePowerMode = Param_PowerModeFast_PSP;

				rWlanStatus = nicConfigPowerSaveProfile(
					prAdapter,
					ucBssIndex,
					ePowerMode, TRUE, PS_CALLER_SW_WRITE);
			}
		}
		break;
	case 0x1001:
		if (u2SubId == 0x0)
			prAdapter->fgEnOnlineScan = (u_int8_t) u4Data;
		else if (u2SubId == 0x1)
			prAdapter->fgDisBcnLostDetection = (u_int8_t) u4Data;
		else if (u2SubId == 0x2)
			prAdapter->rWifiVar.ucUapsd = (u_int8_t) u4Data;
		else if (u2SubId == 0x3) {
			prAdapter->u4UapsdAcBmp = u4Data & BITS(0, 15);

			if (!IS_BSS_INDEX_VALID(u4Data >> 16)) {
				DBGLOG(OID, ERROR,
					"Invalid bssidx:%d\n", u4Data >> 16);
				break;
			}
			GET_BSS_INFO_BY_INDEX(prAdapter,
			      u4Data >> 16)->rPmProfSetupInfo.ucBmpDeliveryAC =
					      (uint8_t) prAdapter->u4UapsdAcBmp;
			GET_BSS_INFO_BY_INDEX(prAdapter,
			      u4Data >> 16)->rPmProfSetupInfo.ucBmpTriggerAC =
					      (uint8_t) prAdapter->u4UapsdAcBmp;
		} else if (u2SubId == 0x4)
			prAdapter->fgDisStaAgingTimeoutDetection =
				(u_int8_t) u4Data;
		else if (u2SubId == 0x5)
			prAdapter->rWifiVar.uc2G4BandwidthMode =
				(uint8_t) u4Data;
		else if (u2SubId == 0x0100) {
			if (u4Data == 2)
				prAdapter->rWifiVar.ucRxGf = FEATURE_DISABLED;
			else
				prAdapter->rWifiVar.ucRxGf = FEATURE_ENABLED;
		} else if (u2SubId == 0x0101)
			prAdapter->rWifiVar.ucRxShortGI = (uint8_t) u4Data;
		else if (u2SubId == 0x0103) { /* AP Mode WMMPS */
#if CFG_ENABLE_WIFI_DIRECT
			DBGLOG(OID, INFO,
			       "ApUapsd 0x10010103 cmd received: %d\n",
			       u4Data);
			setApUapsdEnable(prAdapter, (u_int8_t) u4Data);
#endif
		} else if (u2SubId == 0x0110) {
			prAdapter->fgIsEnableLpdvt = (u_int8_t) u4Data;
			prAdapter->fgEnOnlineScan = (u_int8_t) u4Data;
			DBGLOG(INIT, INFO, "--- Enable LPDVT [%d] ---\n",
			       prAdapter->fgIsEnableLpdvt);
		}

		break;

#if CFG_SUPPORT_SWCR
	case 0x1002:
		if (u2SubId == 0x1) {
			u_int8_t fgIsEnable;
			uint8_t ucType;
			uint32_t u4Timeout;

			fgIsEnable = (u_int8_t) (u4Data & 0xff);
			ucType = 0;	/* ((u4Data>>4) & 0xf); */
			u4Timeout = ((u4Data >> 8) & 0xff);
			swCrDebugCheckEnable(prAdapter, fgIsEnable, ucType,
					     u4Timeout);
		}
		break;
#endif
	case 0x1003: /* for debug switches */
		switch (u2SubId) {
		case 1:
			DBGLOG(OID, INFO,
			       "Enable VoE 5.7 Packet Jitter test\n");
			prAdapter->rDebugInfo.fgVoE5_7Test = !!u4Data;
			break;
		case 0x0002:
		{
			struct CMD_TX_AMPDU rTxAmpdu;
			uint32_t rStatus;

			kalMemSet(&rTxAmpdu, 0, sizeof(struct CMD_TX_AMPDU));
			rTxAmpdu.fgEnable = !!u4Data;

			rStatus = wlanSendSetQueryCmd(
				prAdapter, CMD_ID_TX_AMPDU, TRUE, FALSE, FALSE,
				NULL, NULL, sizeof(struct CMD_TX_AMPDU),
				(uint8_t *)&rTxAmpdu, NULL, 0);
			DBGLOG(OID, INFO, "disable tx ampdu status %u\n",
			       rStatus);
			break;
		}
		default:
			break;
		}
		break;

#if CFG_SUPPORT_802_11W
	case 0x2000:
		DBGLOG(RSN, INFO, "802.11w test 0x%x\n", u2SubId);
		if (u2SubId == 0x0)
			rsnStartSaQuery(prAdapter, ucBssIndex);
		if (u2SubId == 0x1)
			rsnStopSaQuery(prAdapter, ucBssIndex);
		if (u2SubId == 0x2)
			rsnSaQueryRequest(prAdapter, NULL);
		if (u2SubId == 0x3) {
			struct BSS_INFO *prBssInfo =
				aisGetAisBssInfo(prAdapter, ucBssIndex);

			authSendDeauthFrame(prAdapter, prBssInfo,
					prBssInfo->prStaRecOfAP, NULL, 7, NULL);
		}
		/* wext_set_mode */
		/*
		 *  if (u2SubId == 0x3) {
		 *      prAdapter->prGlueInfo->rWpaInfo.u4Mfp =
		 *                              RSN_AUTH_MFP_DISABLED;
		 *  }
		 *  if (u2SubId == 0x4) {
		 *      //prAdapter->rWifiVar.rAisSpecificBssInfo
		 *      //                      .fgMgmtProtection = TRUE;
		 *      prAdapter->prGlueInfo->rWpaInfo.u4Mfp =
		 *				RSN_AUTH_MFP_OPTIONAL;
		 *  }
		 *  if (u2SubId == 0x5) {
		 *      //prAdapter->rWifiVar.rAisSpecificBssInfo
		 *      //			.fgMgmtProtection = TRUE;
		 *      prAdapter->prGlueInfo->rWpaInfo.u4Mfp =
		 *				RSN_AUTH_MFP_REQUIRED;
		 *  }
		 */
		break;
#endif
	case 0xFFFF: {
		/* CMD_ACCESS_REG rCmdAccessReg; */
#if 1				/* CFG_MT6573_SMT_TEST */
		if (u2SubId == 0x0123) {

			DBGLOG(HAL, INFO, "set smt fixed rate: %u\n", u4Data);

			if ((enum ENUM_REGISTRY_FIXED_RATE) (u4Data) <
			    FIXED_RATE_NUM)
				prAdapter->rWifiVar.eRateSetting =
					(enum ENUM_REGISTRY_FIXED_RATE)(u4Data);
			else
				prAdapter->rWifiVar.eRateSetting =
							FIXED_RATE_NONE;

			if (prAdapter->rWifiVar.eRateSetting == FIXED_RATE_NONE)
				/* Enable Auto (Long/Short) Preamble */
				prAdapter->rWifiVar.ePreambleType =
							PREAMBLE_TYPE_AUTO;
			else if ((prAdapter->rWifiVar.eRateSetting >=
				  FIXED_RATE_MCS0_20M_400NS &&
				  prAdapter->rWifiVar.eRateSetting <=
				  FIXED_RATE_MCS7_20M_400NS)
				 || (prAdapter->rWifiVar.eRateSetting >=
				     FIXED_RATE_MCS0_40M_400NS &&
				     prAdapter->rWifiVar.eRateSetting <=
				     FIXED_RATE_MCS32_400NS))
				/* Force Short Preamble */
				prAdapter->rWifiVar.ePreambleType =
							PREAMBLE_TYPE_SHORT;
			else
				/* Force Long Preamble */
				prAdapter->rWifiVar.ePreambleType =
							PREAMBLE_TYPE_LONG;

			/* abort to re-connect */
#if 1
			kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
					     WLAN_STATUS_MEDIA_DISCONNECT,
					     NULL, 0,
					     ucBssIndex);
#else
			aisBssBeaconTimeout(prAdapter);
#endif

			return WLAN_STATUS_SUCCESS;

		} else if (u2SubId == 0x1234) {
			/* 1. Disable On-Lin Scan */
			/* 3. Disable FIFO FULL no ack */
			/* 4. Disable Roaming */
			/* Disalbe auto tx power */
			/* 2. Keep at CAM mode */
			/* 5. Disable Beacon Timeout Detection */
			rWlanStatus = nicEnterCtiaMode(prAdapter, TRUE, TRUE);
		} else if (u2SubId == 0x1235) {
			/* 1. Enaable On-Lin Scan */
			/* 3. Enable FIFO FULL no ack */
			/* 4. Enable Roaming */
			/* Enable auto tx power */
			/* 2. Keep at Fast PS */
			/* 5. Enable Beacon Timeout Detection */
			rWlanStatus = nicEnterCtiaMode(prAdapter, FALSE, TRUE);
		} else if (u2SubId == 0x1260) {
			/* Disable On-Line Scan */
			rWlanStatus = nicEnterCtiaModeOfScan(prAdapter,
					TRUE, TRUE);
		} else if (u2SubId == 0x1261) {
			/* Enable On-Line Scan */
			rWlanStatus = nicEnterCtiaModeOfScan(prAdapter,
					FALSE, TRUE);
		} else if (u2SubId == 0x1262) {
			/* Disable Roaming */
			rWlanStatus = nicEnterCtiaModeOfRoaming(prAdapter,
					TRUE, TRUE);
		} else if (u2SubId == 0x1263) {
			/* Enable Roaming */
			rWlanStatus = nicEnterCtiaModeOfRoaming(prAdapter,
					FALSE, TRUE);
		} else if (u2SubId == 0x1264) {
			/* Keep at CAM mode */
			rWlanStatus = nicEnterCtiaModeOfCAM(prAdapter,
					TRUE, TRUE);
		} else if (u2SubId == 0x1265) {
			/* Keep at Fast PS */
			rWlanStatus = nicEnterCtiaModeOfCAM(prAdapter,
					FALSE, TRUE);
		} else if (u2SubId == 0x1266) {
			/* Disable Beacon Timeout Detection */
			rWlanStatus = nicEnterCtiaModeOfBCNTimeout(prAdapter,
					TRUE, TRUE);
		} else if (u2SubId == 0x1267) {
			/* Enable Beacon Timeout Detection */
			rWlanStatus = nicEnterCtiaModeOfBCNTimeout(prAdapter,
					FALSE, TRUE);
		} else if (u2SubId == 0x1268) {
			/* Disalbe auto tx power */
			rWlanStatus = nicEnterCtiaModeOfAutoTxPower(prAdapter,
					TRUE, TRUE);
		} else if (u2SubId == 0x1269) {
			/* Enable auto tx power */
			rWlanStatus = nicEnterCtiaModeOfAutoTxPower(prAdapter,
					FALSE, TRUE);
		} else if (u2SubId == 0x1270) {
			/* Disalbe FIFO FULL no ack  */
			rWlanStatus = nicEnterCtiaModeOfFIFOFullNoAck(prAdapter,
					TRUE, TRUE);
		} else if (u2SubId == 0x1271) {
			/* Enable FIFO FULL no ack */
			rWlanStatus = nicEnterCtiaModeOfFIFOFullNoAck(prAdapter,
					FALSE, TRUE);
		}
#endif
#if CFG_MTK_STAGE_SCAN
		else if (u2SubId == 0x1250)
			prAdapter->aePreferBand[KAL_NETWORK_TYPE_AIS_INDEX] =
				BAND_NULL;
		else if (u2SubId == 0x1251)
			prAdapter->aePreferBand[KAL_NETWORK_TYPE_AIS_INDEX] =
				BAND_2G4;
		else if (u2SubId == 0x1252) {
			if (prAdapter->fgEnable5GBand)
				prAdapter->aePreferBand
				[KAL_NETWORK_TYPE_AIS_INDEX] = BAND_5G;
			else
				/* Skip this setting if 5G band is disabled */
				DBGLOG(SCN, INFO,
				       "Skip 5G stage scan request due to 5G is disabled\n");
		}
#endif
	}
	break;

	case 0x9000:
	default: {
		kalMemSet(&rCmdSwCtrl, 0, sizeof(struct CMD_SW_DBG_CTRL));
		rCmdSwCtrl.u4Id = prSwCtrlInfo->u4Id;
		rCmdSwCtrl.u4Data = prSwCtrlInfo->u4Data;
		rWlanStatus = wlanSendSetQueryCmd(prAdapter,
					  CMD_ID_SW_DBG_CTRL,
					  TRUE,
					  FALSE,
					  TRUE,
					  nicCmdEventSetCommon,
					  nicOidCmdTimeoutCommon,
					  sizeof(struct CMD_SW_DBG_CTRL),
					  (uint8_t *) &rCmdSwCtrl,
					  pvSetBuffer, u4SetBufferLen);
	}
	}			/* switch(u2Id)  */

	return rWlanStatus;
}				/* wlanoidSetSwCtrlWrite */

uint32_t
wlanoidSetFixRate(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_RA *uni_cmd;
	struct UNI_CMD_RA_SET_FIXED_RATE *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RA) +
			       sizeof(struct UNI_CMD_RA_SET_FIXED_RATE) +
			       sizeof(struct UNI_CMD_RA_SET_FIXED_RATE_V1);

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct UNI_CMD_RA_SET_FIXED_RATE_V1);
	if (u4SetBufferLen < sizeof(struct UNI_CMD_RA_SET_FIXED_RATE_V1))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	uni_cmd = (struct UNI_CMD_RA *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_RA_SET_FIXED_RATE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_RA_TAG_SET_FIXED_RATE;
	tag->u2Length = sizeof(*tag) +
			sizeof(struct UNI_CMD_RA_SET_FIXED_RATE_V1);
	tag->u2Version = UNI_CMD_RA_FIXED_RATE_VER1;
	kalMemCopy(tag->aucBuffer, pvSetBuffer,
		sizeof(struct UNI_CMD_RA_SET_FIXED_RATE_V1));

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_RA,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t
wlanoidSetAutoRate(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_RA *uni_cmd;
	struct UNI_CMD_RA_SET_AUTO_RATE *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RA) +
			       sizeof(struct UNI_CMD_RA_SET_AUTO_RATE);
	struct UNI_CMD_RA_SET_AUTO_RATE *para;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct UNI_CMD_RA_SET_AUTO_RATE);
	if (u4SetBufferLen < sizeof(struct UNI_CMD_RA_SET_AUTO_RATE))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	para = (struct UNI_CMD_RA_SET_AUTO_RATE *)pvSetBuffer;

	uni_cmd = (struct UNI_CMD_RA *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_RA_SET_AUTO_RATE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_RA_TAG_SET_AUTO_RATE;
	tag->u2Length = sizeof(*tag);
	tag->u2WlanIdx = para->u2WlanIdx;
	tag->u1AutoRateEn = para->u1AutoRateEn;
	tag->u1Mode = para->u1Mode;

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_RA,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidSetMloAgcTx(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen)
{
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_MLO *uni_cmd;
	struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MLO) +
			       sizeof(struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX);
	struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX *para;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	*pu4SetInfoLen = sizeof(struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX);
	if (u4SetBufferLen < sizeof(struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX))
		return WLAN_STATUS_INVALID_LENGTH;

	para = (struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX *)pvSetBuffer;

	uni_cmd = (struct UNI_CMD_MLO *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_MLO ==> FAILED\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX *) uni_cmd->au1TlvBuffer;
	tag->u2Tag = UNI_CMD_MLO_TAG_MLD_REC_LINK_AGC_TX;
	tag->u2Length = sizeof(*tag);
	tag->u1MldRecIdx = para->u1MldRecIdx;
	tag->u1MldRecLinkIdx = para->u1MldRecLinkIdx;
	tag->u1AcIdx = para->u1AcIdx;
	tag->u1DispPolTx = para->u1DispPolTx;
	tag->u1DispRatioTx = para->u1DispRatioTx;
	tag->u1DispOrderTx = para->u1DispOrderTx;
	tag->u2DispMgfTx = para->u2DispMgfTx;

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_MLO,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);

	return status;
}

uint32_t
wlanoidGetMldRec(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen)
{
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_MLO *uni_cmd;
	struct UNI_CMD_SET_MLD_REC *tag;
	struct PARAM_MLD_REC *para;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MLO) +
			       sizeof(struct UNI_CMD_SET_MLD_REC);

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct PARAM_MLD_REC);

	if (u4QueryBufferLen < sizeof(struct PARAM_MLD_REC))
		return WLAN_STATUS_INVALID_LENGTH;

	para = (struct PARAM_MLD_REC *)pvQueryBuffer;

	uni_cmd = (struct UNI_CMD_MLO *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);

	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_MLO ==> FAILED\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_SET_MLD_REC *) uni_cmd->au1TlvBuffer;
	tag->u2Tag = UNI_CMD_MLO_TAG_MLD_REC;
	tag->u2Length = sizeof(*tag);
	tag->u4Value = para->u1MldRecIdx;

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_MLO,
			     FALSE,
			     TRUE,
			     TRUE,
			     nicUniCmdEventQueryMldRec,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd,
			     pvQueryBuffer,
			     u4QueryBufferLen);

	cnmMemFree(prAdapter, uni_cmd);

	return status;
}
#endif
#endif

uint32_t
wlanoidSetPpCap(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_PP *uni_cmd;
	struct UNI_CMD_PP_EN_CTRL_T *tag;
	struct UNI_CMD_PP_EN_CTRL_T *para;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_PP) +
			       sizeof(struct UNI_CMD_PP_EN_CTRL_T);

	if (prAdapter == NULL || pu4SetInfoLen == NULL)
		return WLAN_STATUS_ADAPTER_NOT_READY;

	*pu4SetInfoLen = sizeof(struct UNI_CMD_PP_EN_CTRL_T);

	if (u4SetBufferLen < sizeof(struct UNI_CMD_PP_EN_CTRL_T))
		return WLAN_STATUS_INVALID_LENGTH;
	else if (pvSetBuffer == NULL)
		return WLAN_STATUS_INVALID_DATA;

	para = (struct UNI_CMD_PP_EN_CTRL_T *)pvSetBuffer;

	uni_cmd = (struct UNI_CMD_PP *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_PP ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_PP_EN_CTRL_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_PP_TAG_EN_CTRL;
	tag->u2Length =  sizeof(*tag);
	tag->u1DbdcIdx = para->u1DbdcIdx;
	tag->u1PpMgmtMode = para->u1PpMgmtMode;
	tag->u1PpMgmtEn = para->u1PpMgmtEn;
	tag->u1PpCtrl = para->u1PpCtrl;
	tag->u1PpBitMap = para->u1PpBitMap;

	DBGLOG(INIT, ERROR, "pp_cap_ctrl: %d-%d-%d-%d\n",
			tag->u1PpMgmtMode,
			tag->u1PpMgmtEn,
			tag->u1PpCtrl,
			tag->u1PpBitMap);

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_PP,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t
wlanoidSetPpAlgCtrl(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_PP *uni_cmd;
	struct UNI_CMD_PP_ALG_CTRL *tag;
	struct UNI_CMD_PP_ALG_CTRL *para;
	uint32_t u4MaxCmdLen = sizeof(struct UNI_CMD_PP) +
			       sizeof(struct UNI_CMD_PP_ALG_CTRL);

	if (!prAdapter) {
		DBGLOG(REQ, ERROR,
			"\x1b[31m prAdapter is null !!!!\x1b[m\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pu4SetInfoLen) {
		DBGLOG(REQ, ERROR,
			"\x1b[31m pu4SetInfoLen is null !!!!\x1b[m\n");
		return WLAN_STATUS_FAILURE;
	}

	*pu4SetInfoLen = sizeof(struct UNI_CMD_PP_ALG_CTRL);
	if (u4SetBufferLen < sizeof(struct UNI_CMD_PP_ALG_CTRL))
		return WLAN_STATUS_INVALID_LENGTH;

	if (!pvSetBuffer) {
		DBGLOG(REQ, ERROR,
			"\x1b[31m pvSetBuffer is null !!!!\x1b[m\n");
		return WLAN_STATUS_FAILURE;
	}
	para = (struct UNI_CMD_PP_ALG_CTRL *)pvSetBuffer;

	uni_cmd = (struct UNI_CMD_PP *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, u4MaxCmdLen);
	if (!uni_cmd) {
		DBGLOG(REQ, ERROR,
			"\x1b[31m uni_cmd is null !!!!\x1b[m\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_PP_ALG_CTRL *)uni_cmd->aucTlvBuffer;
	kalMemCopy(tag, para, sizeof(*tag));
	tag->u2Tag = UNI_CMD_PP_TAG_ALG_CTRL;
	tag->u2Length =  sizeof(*tag);

	switch (tag->u1PpAction) {
	case UNI_CMD_PP_ALG_SET_TIMER:
		DBGLOG(REQ, INFO, "\x1b[32m u4PpAction = %d\x1b[m\n"
			, tag->u1PpAction);
		DBGLOG(REQ, INFO, "\x1b[32m u1DbdcIdx = %d\x1b[m\n"
			, tag->u1DbdcIdx);
		DBGLOG(REQ, INFO, "\x1b[32m u4PpTimerIntv = %d\x1b[m\n"
			, tag->u4PpTimerIntv);

		break;
	case UNI_CMD_PP_ALG_SET_THR:
		DBGLOG(REQ, INFO, "\x1b[32m u4PpAction = %d\x1b[m\n"
			, tag->u1PpAction);
		DBGLOG(REQ, INFO, "\x1b[32m u1DbdcIdx = %d\x1b[m\n"
			, tag->u1DbdcIdx);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX2_Value = %d\x1b[m\n"
			, tag->u4ThrX2_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX2_Shift = %d\x1b[m\n"
			, tag->u4ThrX2_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX3_Value = %d\x1b[m\n"
			, tag->u4ThrX3_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX3_Shift = %d\x1b[m\n"
			, tag->u4ThrX3_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX4_Value = %d\x1b[m\n"
			, tag->u4ThrX4_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX4_Shift = %d\x1b[m\n"
			, tag->u4ThrX4_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX5_Value = %d\x1b[m\n"
			, tag->u4ThrX5_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX5_Shift = %d\x1b[m\n"
			, tag->u4ThrX5_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX6_Value = %d\x1b[m\n"
			, tag->u4ThrX6_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX6_Shift = %d\x1b[m\n"
			, tag->u4ThrX6_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX7_Value = %d\x1b[m\n"
			, tag->u4ThrX7_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX7_Shift = %d\x1b[m\n"
			, tag->u4ThrX7_Shift);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX8_Value = %d\x1b[m\n"
			, tag->u4ThrX8_Value);
		DBGLOG(REQ, INFO, "\x1b[32m u4ThrX8_Shift = %d\x1b[m\n"
			, tag->u4ThrX8_Shift);


		break;

	case UNI_CMD_PP_ALG_GET_STATISTICS:
		DBGLOG(REQ, INFO, "\x1b[32m u4PpAction = %d\x1b[m\n"
			, tag->u1PpAction);
		DBGLOG(REQ, INFO, "\x1b[32m u1DbdcIdx = %d\x1b[m\n"
			, tag->u1DbdcIdx);
		DBGLOG(REQ, INFO, "\x1b[32m u1Reset = %d\x1b[m\n"
			, tag->u1Reset);

		break;

	default:
		DBGLOG(REQ, ERROR,
			"\x1b[31m u4PpAction = %d is not supported !!\x1b[m\n"
			, tag->u1PpAction);
		break;
	}

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_PP,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     u4MaxCmdLen,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t
wlanoidSetHmAlg(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_HM *uni_cmd;
	struct UNI_CMD_HM_ALG_CTRL_T *tag;
	struct UNI_CMD_HM_ALG_CTRL_T *para;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_HM) +
			       sizeof(struct UNI_CMD_HM_ALG_CTRL_T);

	if (prAdapter == NULL || pu4SetInfoLen == NULL)
		return WLAN_STATUS_ADAPTER_NOT_READY;

	*pu4SetInfoLen = sizeof(struct UNI_CMD_HM_ALG_CTRL_T);

	if (u4SetBufferLen < sizeof(struct UNI_CMD_HM_ALG_CTRL_T))
		return WLAN_STATUS_INVALID_LENGTH;
	else if (pvSetBuffer == NULL)
		return WLAN_STATUS_INVALID_DATA;

	para = (struct UNI_CMD_HM_ALG_CTRL_T *)pvSetBuffer;

	uni_cmd = (struct UNI_CMD_HM *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_HM ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_HM_ALG_CTRL_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_HM_TAG_ALG_CTRL;
	tag->u2Length =  sizeof(*tag);
	tag->u1HmManualModeEn = para->u1HmManualModeEn;
	tag->u1ForceObss = para->u1ForceObss;
	tag->u1ForceBT = para->u1ForceBT;
	tag->u1HmForcePlan = para->u1HmForcePlan;
	tag->u1ObssTimePercntg = para->u1ObssTimePercntg;
	tag->u1BTPercntg = para->u1BTPercntg;

	DBGLOG(INIT, INFO, "hm_alg_ctrl: %d-%d-%d-%d-%d-%d\n",
			tag->u1HmManualModeEn,
			tag->u1ForceObss,
			tag->u1ForceBT,
			tag->u1HmForcePlan,
			tag->u1ObssTimePercntg,
			tag->u1BTPercntg);

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_HM,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}


#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is set ICS sniffer
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetIcsSniffer(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_ICS_SNIFFER_INFO_STRUCT *prSnifferInfo;
	struct CMD_ICS_SNIFFER_INFO rCmdSniffer;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	uint32_t count = 0;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_ICS_SNIFFER_INFO_STRUCT);
	if (u4SetBufferLen <
		sizeof(struct PARAM_CUSTOM_ICS_SNIFFER_INFO_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prSnifferInfo =
	(struct PARAM_CUSTOM_ICS_SNIFFER_INFO_STRUCT *)pvSetBuffer;
	kalMemZero(&rCmdSniffer, sizeof(struct CMD_ICS_SNIFFER_INFO));
	rCmdSniffer.ucModule = prSnifferInfo->ucModule;
	rCmdSniffer.ucAction = prSnifferInfo->ucAction;
	rCmdSniffer.ucFilter = prSnifferInfo->ucFilter;
	rCmdSniffer.ucOperation = prSnifferInfo->ucOperation;
	while (count <= 6) {
		rCmdSniffer.ucCondition[count] =
			prSnifferInfo->ucCondition[count];
		count += 1;
	}
	DBGLOG(INIT, INFO, "ICS_CMD_DRIVER: %d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d\n",
		rCmdSniffer.ucModule,
		rCmdSniffer.ucAction,
		rCmdSniffer.ucFilter,
		rCmdSniffer.ucOperation,
		rCmdSniffer.ucCondition[0],
		rCmdSniffer.ucCondition[1],
		rCmdSniffer.ucCondition[2],
		rCmdSniffer.ucCondition[3],
		rCmdSniffer.ucCondition[4],
		rCmdSniffer.ucCondition[5],
		rCmdSniffer.ucCondition[6]
		);

	if (rCmdSniffer.ucAction < 2) {
		switch (rCmdSniffer.ucCondition[0]) {
		case 0:
			prAdapter->fgEnTmacICS = rCmdSniffer.ucAction;
			break;
		case 1:
			prAdapter->fgEnRmacICS = rCmdSniffer.ucAction;
			break;
		case 2:
			prAdapter->fgEnTmacICS = rCmdSniffer.ucAction;
			prAdapter->fgEnRmacICS = rCmdSniffer.ucAction;
			break;
		case 3:
			prAdapter->fgEnPhyICS = rCmdSniffer.ucAction;
			prAdapter->uPhyICSBandIdx = rCmdSniffer.ucCondition[1];
			break;
		default:
			DBGLOG(INIT, ERROR, "ICS Action ERROR\n");
			break;
		}
	}

	if ((prAdapter->fgEnTmacICS || prAdapter->fgEnRmacICS
		|| prAdapter->fgEnPhyICS) == FALSE) {
		DBGLOG(INIT, TRACE, "ICS STOP\n");
	}

	rWlanStatus = wlanSendSetQueryCmd(prAdapter,
				  CMD_ID_SET_ICS_SNIFFER,
				  TRUE,
				  FALSE,
				  TRUE,
				  nicCmdEventSetCommon,
				  nicOidCmdTimeoutCommon,
				  sizeof(struct CMD_ICS_SNIFFER_INFO),
				  (uint8_t *) &rCmdSniffer,
				  pvSetBuffer, u4SetBufferLen);

	return rWlanStatus;
}
#endif /* CFG_SUPPORT_ICS */

uint32_t
wlanoidQueryChipConfig(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT *prChipConfigInfo;
	struct CMD_CHIP_CONFIG rCmdChipConfig;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_CHIP_CONFIG_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_CUSTOM_CHIP_CONFIG_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prChipConfigInfo = (struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT
			    *) pvQueryBuffer;
	kalMemZero(&rCmdChipConfig, sizeof(rCmdChipConfig));

	rCmdChipConfig.u2Id = prChipConfigInfo->u2Id;
	rCmdChipConfig.ucType = prChipConfigInfo->ucType;
	rCmdChipConfig.ucRespType = prChipConfigInfo->ucRespType;
	rCmdChipConfig.u2MsgSize = prChipConfigInfo->u2MsgSize;
	if (rCmdChipConfig.u2MsgSize > CHIP_CONFIG_RESP_SIZE) {
		DBGLOG(REQ, INFO,
		       "Chip config Msg Size %u is not valid (query)\n",
		       rCmdChipConfig.u2MsgSize);
		rCmdChipConfig.u2MsgSize = CHIP_CONFIG_RESP_SIZE;
	}
	kalMemCopy(rCmdChipConfig.aucCmd, prChipConfigInfo->aucCmd,
		   rCmdChipConfig.u2MsgSize);

	rWlanStatus = wlanSendSetQueryCmd(prAdapter,
					  CMD_ID_CHIP_CONFIG, FALSE, TRUE, TRUE,
					  /*nicCmdEventQuerySwCtrlRead, */
					  nicCmdEventQueryChipConfig,
					  nicOidCmdTimeoutCommon,
					  sizeof(struct CMD_CHIP_CONFIG),
					  (uint8_t *) &rCmdChipConfig,
					  pvQueryBuffer,
					  u4QueryBufferLen);

	return rWlanStatus;

}

/* end of wlanoidQueryChipConfig() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set chip
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetChipConfig(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	return wlanSetChipConfig(prAdapter, pvSetBuffer, u4SetBufferLen,
		pu4SetInfoLen, TRUE);
}

uint32_t
wlanSetChipConfig(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen, uint8_t fgIsOid) {
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT *prChipConfigInfo;
	struct CMD_CHIP_CONFIG rCmdChipConfig;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	DATA_STRUCT_INSPECTING_ASSERT(
		sizeof(prChipConfigInfo->aucCmd) == CHIP_CONFIG_RESP_SIZE);

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_CHIP_CONFIG_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_CHIP_CONFIG_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prChipConfigInfo = (struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT
			    *) pvSetBuffer;
	kalMemZero(&rCmdChipConfig, sizeof(rCmdChipConfig));

	rCmdChipConfig.u2Id = prChipConfigInfo->u2Id;
	rCmdChipConfig.ucType = prChipConfigInfo->ucType;
	rCmdChipConfig.ucRespType = prChipConfigInfo->ucRespType;
	rCmdChipConfig.u2MsgSize = prChipConfigInfo->u2MsgSize;
	if (rCmdChipConfig.u2MsgSize > CHIP_CONFIG_RESP_SIZE) {
		DBGLOG(REQ, INFO,
		       "Chip config Msg Size %u is not valid (set)\n",
		       rCmdChipConfig.u2MsgSize);
		rCmdChipConfig.u2MsgSize = CHIP_CONFIG_RESP_SIZE;
	}
	kalMemCopy(rCmdChipConfig.aucCmd, prChipConfigInfo->aucCmd,
		   rCmdChipConfig.u2MsgSize);

	rWlanStatus = wlanSendSetQueryCmd(prAdapter,
					  CMD_ID_CHIP_CONFIG,
					  TRUE,
					  FALSE,
					  fgIsOid,
					  nicCmdEventSetCommon,
					  nicOidCmdTimeoutCommon,
					  sizeof(struct CMD_CHIP_CONFIG),
					  (uint8_t *) &rCmdChipConfig,
					  pvSetBuffer, u4SetBufferLen);

	return rWlanStatus;
} /* wlanoidSetChipConfig */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set cfg and callback
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetKeyCfg(struct ADAPTER *prAdapter,
		 void *pvSetBuffer, uint32_t u4SetBufferLen,
		 uint32_t *pu4SetInfoLen) {
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_CUSTOM_KEY_CFG_STRUCT *prKeyCfgInfo;
	uint8_t *pucKey = NULL;
	uint8_t aucKey[MAX_CMD_NAME_MAX_LENGTH] = {0};
	uint32_t u4TargetCfg = 0;
	int32_t i4Ret = 0;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_KEY_CFG_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_KEY_CFG_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	prKeyCfgInfo = (struct PARAM_CUSTOM_KEY_CFG_STRUCT *)
		       pvSetBuffer;

	if (kalMemCmp(prKeyCfgInfo->aucKey, "reload", 6) == 0) {
		wlanGetConfig(prAdapter); /* Reload config file */
		wlanInitFeatureOption(prAdapter);
	} else {
		wlanCfgSet(prAdapter, prKeyCfgInfo->aucKey,
			   prKeyCfgInfo->aucValue, prKeyCfgInfo->u4Flag);
		kalStrnCpy(&aucKey[0], prKeyCfgInfo->aucKey,
			MAX_CMD_NAME_MAX_LENGTH);
		pucKey = &aucKey[0];
		wlanInitFeatureOptionImpl(prAdapter, prKeyCfgInfo->aucKey);
#if CFG_SUPPORT_IOT_AP_BLOCKLIST
		if (kalMemCmp(prKeyCfgInfo->aucKey, "IOTAP", 5) == 0)
			wlanCfgLoadIotApRule(prAdapter);
#endif
	}

#if CFG_SUPPORT_MLR
	if (kalMemCmp(prKeyCfgInfo->aucKey, "MlrCfg", 6) == 0) {
		i4Ret = kalkStrtou32(prKeyCfgInfo->aucValue, 0, &u4TargetCfg);
		if (!i4Ret) {
			DBGLOG(INIT, INFO, "MLR SET_CFG [%s]:[0x%x]\n",
				prKeyCfgInfo->aucKey, u4TargetCfg, i4Ret);
			prAdapter->u4MlrSupportBitmap =
				prAdapter->u4MlrCapSupportBitmap & u4TargetCfg;
		} else {
			DBGLOG(INIT, ERROR,
				"MLR SET_CFG: Parse error i4Ret[%d]\n", i4Ret);
		}
	}
#endif
#if CFG_SUPPORT_EASY_DEBUG
	wlanFeatureToFw(prAdapter, prKeyCfgInfo->u4Flag, pucKey);
#endif

	return rWlanStatus;
}

/* wlanoidSetSwCtrlWrite */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query EEPROM value.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryEepromRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_EEPROM_RW_STRUCT *prEepromRwInfo;
	struct CMD_ACCESS_EEPROM rCmdAccessEeprom;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_EEPROM_RW_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_CUSTOM_EEPROM_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	prEepromRwInfo = (struct PARAM_CUSTOM_EEPROM_RW_STRUCT *)
			 pvQueryBuffer;

	kalMemZero(&rCmdAccessEeprom,
		   sizeof(struct CMD_ACCESS_EEPROM));
	rCmdAccessEeprom.u2Offset = prEepromRwInfo->info.rEeprom.ucEepromIndex;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_ACCESS_EEPROM,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryEepromRead,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_ACCESS_EEPROM),
				   (uint8_t *) &rCmdAccessEeprom, pvQueryBuffer,
				   u4QueryBufferLen);

}				/* wlanoidQueryEepromRead */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to write EEPROM value.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetEepromWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_EEPROM_RW_STRUCT *prEepromRwInfo;
	struct CMD_ACCESS_EEPROM rCmdAccessEeprom;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_EEPROM_RW_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_EEPROM_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prEepromRwInfo = (struct PARAM_CUSTOM_EEPROM_RW_STRUCT *)
			 pvSetBuffer;

	kalMemZero(&rCmdAccessEeprom,
		   sizeof(struct CMD_ACCESS_EEPROM));
	rCmdAccessEeprom.u2Offset = prEepromRwInfo->info.rEeprom.ucEepromIndex;
	rCmdAccessEeprom.u2Data = prEepromRwInfo->info.rEeprom.u2EepromData;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_ACCESS_EEPROM,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_ACCESS_EEPROM),
				   (uint8_t *) &rCmdAccessEeprom, pvSetBuffer,
				   u4SetBufferLen);

} /* wlanoidSetEepromWrite */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query current the OID interface version,
 *        which is the interface between the application and driver.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryOidInterfaceVersion(struct ADAPTER *
				prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	if (!pvQueryBuffer || u4QueryBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*(uint32_t *) pvQueryBuffer =
		prAdapter->chip_info->custom_oid_interface_version;
	*pu4QueryInfoLen = sizeof(uint32_t);

	DBGLOG(REQ, WARN, "Custom OID interface version: %#08X\n",
	       *(uint32_t *) pvQueryBuffer);

	return WLAN_STATUS_SUCCESS;
}				/* wlanoidQueryOidInterfaceVersion */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query current Multicast Address List.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryMulticastList(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen) {
#ifndef LINUX
	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_MAC_MCAST_ADDR,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryMcastAddr,
				   nicOidCmdTimeoutCommon, 0,
				   NULL, pvQueryBuffer,
				   u4QueryBufferLen);
#else
	return WLAN_STATUS_SUCCESS;
#endif
} /* end of wlanoidQueryMulticastList() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set Multicast Address List.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetMulticastList(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen) {
#define DBG_BUFFER_SZ		1024

	struct PARAM_MULTICAST_LIST *prMcAddrList;
	struct CMD_MAC_MCAST_ADDR rCmdMacMcastAddr;
	int32_t i4Written = 0;
	uint8_t *prDbgBuf;
	uint8_t i;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	/* NOTE(Kevin): Windows may set u4SetBufferLen == 0 &&
	 * pvSetBuffer == NULL to clear exist Multicast List.
	 */
	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	prMcAddrList = (struct PARAM_MULTICAST_LIST *) pvSetBuffer;

	*pu4SetInfoLen = u4SetBufferLen;

	/* Verify if we can support so many multicast addresses. */
	if (prMcAddrList->ucAddrNum > MAX_NUM_GROUP_ADDR) {
		DBGLOG(REQ, WARN, "Too many MC addresses\n");

		return WLAN_STATUS_MULTICAST_FULL;
	}

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set multicast list! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	kalMemZero(&rCmdMacMcastAddr, sizeof(rCmdMacMcastAddr));
	rCmdMacMcastAddr.u4NumOfGroupAddr = prMcAddrList->ucAddrNum;
	rCmdMacMcastAddr.ucBssIndex = prMcAddrList->ucBssIdx;
	kalMemCopy(rCmdMacMcastAddr.arAddress, prMcAddrList->aucMcAddrList,
		   prMcAddrList->ucAddrNum * MAC_ADDR_LEN);

	prDbgBuf = kalMemZAlloc(DBG_BUFFER_SZ, VIR_MEM_TYPE);
	if (prDbgBuf) {
		i4Written +=
			kalScnprintf(prDbgBuf + i4Written,
				     DBG_BUFFER_SZ - i4Written,
				     "BssIdx %d allow list: total=%d",
				     rCmdMacMcastAddr.ucBssIndex,
				     rCmdMacMcastAddr.u4NumOfGroupAddr);
		for (i = 0; i < rCmdMacMcastAddr.u4NumOfGroupAddr; i++) {
			i4Written +=
				kalScnprintf(prDbgBuf + i4Written,
					     DBG_BUFFER_SZ - i4Written,
					     "\nmac[%u]="MACSTR,
					     i, MAC2STR(
					     rCmdMacMcastAddr.arAddress[i]));
		}
		DBGLOG(OID, INFO, "%s\n", prDbgBuf);
		kalMemFree(prDbgBuf, VIR_MEM_TYPE, DBG_BUFFER_SZ);
	} else {
		DBGLOG(OID, WARN, "Alloc debug buffer(%u) failed.\n",
			DBG_BUFFER_SZ);
	}

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_MAC_MCAST_ADDR,
				   TRUE,
				   FALSE,
				   prMcAddrList->fgIsOid,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_MAC_MCAST_ADDR),
				   (uint8_t *) &rCmdMacMcastAddr,
				   pvSetBuffer, u4SetBufferLen);
}				/* end of wlanoidSetMulticastList() */

#if CFG_SUPPORT_NAN
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set Multicast Address List.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *                           set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetNANMulticastList(struct ADAPTER *prAdapter, uint8_t ucBssIdx,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	struct CMD_MAC_MCAST_ADDR rCmdMacMcastAddr;

	kalMemZero(&rCmdMacMcastAddr, sizeof(struct CMD_MAC_MCAST_ADDR));

	if (!prAdapter || !pu4SetInfoLen)
		return WLAN_STATUS_FAILURE;

	/* The data must be a multiple of the Ethernet address size. */
	if ((u4SetBufferLen % MAC_ADDR_LEN)) {
		DBGLOG(REQ, WARN, "Invalid MC list length %ld\n",
		       u4SetBufferLen);

		*pu4SetInfoLen =
			(((u4SetBufferLen + MAC_ADDR_LEN) - 1) / MAC_ADDR_LEN) *
			MAC_ADDR_LEN;

		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = u4SetBufferLen;

	/* Verify if we can support so many multicast addresses. */
	if (u4SetBufferLen > MAX_NUM_GROUP_ADDR * MAC_ADDR_LEN) {
		DBGLOG(REQ, WARN, "Too many MC addresses\n");

		return WLAN_STATUS_MULTICAST_FULL;
	}

	/* NOTE(Kevin): Windows may set u4SetBufferLen == 0 &&
	 * pvSetBuffer == NULL to clear exist Multicast List.
	 */
	if (u4SetBufferLen)
		if (pvSetBuffer == NULL) {
			DBGLOG(REQ, ERROR, "pvSetBuffer is NULL\n");
			return WLAN_STATUS_INVALID_DATA;
		}

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set multicast list! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	rCmdMacMcastAddr.u4NumOfGroupAddr = u4SetBufferLen / MAC_ADDR_LEN;
	rCmdMacMcastAddr.ucBssIndex = ucBssIdx;
	kalMemCopy(rCmdMacMcastAddr.arAddress, pvSetBuffer, u4SetBufferLen);

	return wlanSendSetQueryCmd(
		prAdapter, CMD_ID_MAC_MCAST_ADDR, TRUE, FALSE, FALSE,
		nicCmdEventSetCommon, nicOidCmdTimeoutCommon,
		sizeof(struct CMD_MAC_MCAST_ADDR), (uint8_t *)&rCmdMacMcastAddr,
		pvSetBuffer, u4SetBufferLen);
} /* end of wlanoidSetMulticastList() */

#endif

uint32_t
wlanoidRssiMonitor(struct ADAPTER *prAdapter,
		   void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		   uint32_t *pu4QueryInfoLen) {
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct PARAM_RSSI_MONITOR_T rRssi;
	int8_t orig_max_rssi_value;
	int8_t orig_min_rssi_value;
	uint32_t rStatus1 = WLAN_STATUS_SUCCESS;
	uint32_t rStatus2;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct PARAM_RSSI_MONITOR_T);
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);

	/* Check for query buffer length */
	if (u4QueryBufferLen < *pu4QueryInfoLen) {
		DBGLOG(OID, WARN, "Too short length %u\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	kalMemZero(&rRssi, sizeof(struct PARAM_RSSI_MONITOR_T));

	orig_max_rssi_value = rRssi.max_rssi_value;
	orig_min_rssi_value = rRssi.min_rssi_value;

	kalMemCopy(&rRssi, pvQueryBuffer,
		   sizeof(struct PARAM_RSSI_MONITOR_T));

	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo, ucBssIndex) ==
	    MEDIA_STATE_DISCONNECTED) {
		DBGLOG(OID, TRACE,
			"Set RSSI monitor when disconnected, enable=%d\n",
			rRssi.enable);
		if (rRssi.enable)
			return WLAN_STATUS_ADAPTER_NOT_READY;
		rStatus1 = WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!rRssi.enable) {
		rRssi.max_rssi_value = 0;
		rRssi.min_rssi_value = 0;
	}
	kalMemCopy(&prAisFsmInfo->rRSSIMonitor, &rRssi,
		   sizeof(struct PARAM_RSSI_MONITOR_T));

	DBGLOG(OID, TRACE,
	       "enable=%d, max_rssi_value=%d, min_rssi_value=%d, orig_max_rssi_value=%d, orig_min_rssi_value=%d\n",
	       rRssi.enable, rRssi.max_rssi_value, rRssi.min_rssi_value,
	       orig_max_rssi_value, orig_min_rssi_value);

	/*
	 * If status == WLAN_STATUS_ADAPTER_NOT_READY
	 * driver needs to info FW to stop mointor but set oid flag to false
	 * to prevent from multiple complete
	 */
	rStatus2 = wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_RSSI_MONITOR,
				   TRUE,
				   FALSE,
				   (rStatus1 != WLAN_STATUS_ADAPTER_NOT_READY),
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct PARAM_RSSI_MONITOR_T),
				   (uint8_t *)&rRssi, NULL, 0);

	return (rStatus1 == WLAN_STATUS_ADAPTER_NOT_READY) ?
		rStatus1 : rStatus2;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set Packet Filter.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_NOT_SUPPORTED
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetCurrentPacketFilter(struct ADAPTER *prAdapter,
			      void *pvSetBuffer, uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen) {
	uint32_t u4NewPacketFilter;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t rResult;
	struct CMD_RX_PACKET_FILTER rSetRxPacketFilter;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(uint32_t)) {
		*pu4SetInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_INVALID_LENGTH;
	}
	ASSERT(pvSetBuffer);

	/* Set the new packet filter. */
	u4NewPacketFilter = *(uint32_t *) pvSetBuffer;

	DBGLOG(REQ, TRACE, "New packet filter: %#08x\n",
	       u4NewPacketFilter);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set current packet filter! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	do {
		/* Verify the bits of the new packet filter. If any bits are
		 * set that we don't support, leave.
		 */
		if (u4NewPacketFilter & ~(PARAM_PACKET_FILTER_SUPPORTED)) {
			rStatus = WLAN_STATUS_NOT_SUPPORTED;
			DBGLOG(REQ, WARN, "some flags we don't support\n");
			break;
		}
#if DBG
		/* Need to enable or disable promiscuous support depending on
		 * the new filter.
		 */
		if (u4NewPacketFilter & PARAM_PACKET_FILTER_PROMISCUOUS)
			DBGLOG(REQ, INFO, "Enable promiscuous mode\n");
		else
			DBGLOG(REQ, INFO, "Disable promiscuous mode\n");

		if (u4NewPacketFilter & PARAM_PACKET_FILTER_ALL_MULTICAST)
			DBGLOG(REQ, INFO, "Enable all-multicast mode\n");
		else if (u4NewPacketFilter & PARAM_PACKET_FILTER_MULTICAST)
			DBGLOG(REQ, INFO, "Enable multicast\n");
		else
			DBGLOG(REQ, INFO, "Disable multicast\n");

		if (u4NewPacketFilter & PARAM_PACKET_FILTER_BROADCAST)
			DBGLOG(REQ, INFO, "Enable Broadcast\n");
		else
			DBGLOG(REQ, INFO, "Disable Broadcast\n");
#endif

		prAdapter->fgAllMulicastFilter = FALSE;
		if (u4NewPacketFilter & PARAM_PACKET_FILTER_ALL_MULTICAST)
			prAdapter->fgAllMulicastFilter = TRUE;
	} while (FALSE);

	if (rStatus == WLAN_STATUS_SUCCESS) {
		/* Store the packet filter */

		prAdapter->u4OsPacketFilter &= PARAM_PACKET_FILTER_P2P_MASK;
		prAdapter->u4OsPacketFilter |= u4NewPacketFilter;

		rSetRxPacketFilter.u4RxPacketFilter =
			prAdapter->u4OsPacketFilter;
		rResult = wlanoidSetPacketFilter(prAdapter,
						 &rSetRxPacketFilter,
						 TRUE, pvSetBuffer,
						 u4SetBufferLen);
		DBGLOG(OID, TRACE, "[MC debug] u4OsPacketFilter=0x%x\n",
		       prAdapter->u4OsPacketFilter);
		return rResult;
	} else {
		return rStatus;
	}
}				/* wlanoidSetCurrentPacketFilter */

uint32_t wlanoidSetPacketFilter(struct ADAPTER *prAdapter,
				void *pvPacketFiltr,
				u_int8_t fgIsOid, void *pvSetBuffer,
				uint32_t u4SetBufferLen) {
	struct CMD_RX_PACKET_FILTER *prSetRxPacketFilter = NULL;

	prSetRxPacketFilter = (struct CMD_RX_PACKET_FILTER *)
			      pvPacketFiltr;
#if CFG_SUPPORT_DROP_ALL_MC_PACKET
	if (prAdapter->prGlueInfo->fgIsInSuspendMode)
		prSetRxPacketFilter->u4RxPacketFilter &=
			~(PARAM_PACKET_FILTER_MULTICAST |
			  PARAM_PACKET_FILTER_ALL_MULTICAST);
#else
	if (prAdapter->prGlueInfo->fgIsInSuspendMode) {
		prSetRxPacketFilter->u4RxPacketFilter &=
			~(PARAM_PACKET_FILTER_ALL_MULTICAST);
		prSetRxPacketFilter->u4RxPacketFilter |=
			(PARAM_PACKET_FILTER_MULTICAST);
	}
#endif
	DBGLOG(OID, TRACE,
	       "[MC debug] u4PacketFilter=%x, IsSuspend=%d\n",
	       prSetRxPacketFilter->u4RxPacketFilter,
	       prAdapter->prGlueInfo->fgIsInSuspendMode);
	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_RX_FILTER,
				   TRUE,
				   FALSE,
				   fgIsOid,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_RX_PACKET_FILTER),
				   (uint8_t *)prSetRxPacketFilter,
				   pvSetBuffer, u4SetBufferLen);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query current packet filter.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryCurrentPacketFilter(struct ADAPTER *prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(uint32_t);

	if (u4QueryBufferLen >= sizeof(uint32_t)) {
		ASSERT(pvQueryBuffer);
		*(uint32_t *) pvQueryBuffer = prAdapter->u4OsPacketFilter;
	}

	return WLAN_STATUS_SUCCESS;
} /* wlanoidQueryCurrentPacketFilter */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query ACPI device power state.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryAcpiDevicePowerState(struct ADAPTER *
				 prAdapter,
				 void *pvQueryBuffer,
				 uint32_t u4QueryBufferLen,
				 uint32_t *pu4QueryInfoLen) {
#if DBG
	enum PARAM_DEVICE_POWER_STATE *prPowerState;
#endif

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(enum PARAM_DEVICE_POWER_STATE);

#if DBG
	prPowerState = (enum PARAM_DEVICE_POWER_STATE *)
		       pvQueryBuffer;
	switch (*prPowerState) {
	case ParamDeviceStateD0:
		DBGLOG(REQ, INFO, "Query Power State: D0\n");
		break;
	case ParamDeviceStateD1:
		DBGLOG(REQ, INFO, "Query Power State: D1\n");
		break;
	case ParamDeviceStateD2:
		DBGLOG(REQ, INFO, "Query Power State: D2\n");
		break;
	case ParamDeviceStateD3:
		DBGLOG(REQ, INFO, "Query Power State: D3\n");
		break;
	default:
		break;
	}
#endif

	/* Since we will disconnect the newwork, therefore we do not
	 *  need to check queue empty
	 */
	*(enum PARAM_DEVICE_POWER_STATE *) pvQueryBuffer =
		ParamDeviceStateD3;
	/* WARNLOG(("Ready to transition to D3\n")); */
	return WLAN_STATUS_SUCCESS;

}				/* pwrmgtQueryPower */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set ACPI device power state.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetAcpiDevicePowerState(struct ADAPTER *
			       prAdapter,
			       void *pvSetBuffer, uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen) {
	enum PARAM_DEVICE_POWER_STATE *prPowerState;
	u_int8_t fgRetValue = TRUE;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(enum PARAM_DEVICE_POWER_STATE);

	ASSERT(pvSetBuffer);
	prPowerState = (enum PARAM_DEVICE_POWER_STATE *)
		       pvSetBuffer;
	switch (*prPowerState) {
	case ParamDeviceStateD0:
		DBGLOG(REQ, INFO, "Set Power State: D0\n");
		kalDevSetPowerState(prAdapter->prGlueInfo,
				    (uint32_t) ParamDeviceStateD0);
		fgRetValue = nicpmSetAcpiPowerD0(prAdapter);
		break;
	case ParamDeviceStateD1:
		DBGLOG(REQ, INFO, "Set Power State: D1\n");
		kal_fallthrough;
	case ParamDeviceStateD2:
		DBGLOG(REQ, INFO, "Set Power State: D2\n");
		kal_fallthrough;
	case ParamDeviceStateD3:
		DBGLOG(REQ, INFO, "Set Power State: D3\n");
		fgRetValue = nicpmSetAcpiPowerD3(prAdapter);
		kalDevSetPowerState(prAdapter->prGlueInfo,
				    (uint32_t) ParamDeviceStateD3);
		break;
	default:
		break;
	}

	if (fgRetValue == TRUE)
		return WLAN_STATUS_SUCCESS;
	else
		return WLAN_STATUS_FAILURE;
}				/* end of wlanoidSetAcpiDevicePowerState() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current fragmentation threshold.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryFragThreshold(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	DBGLOG(REQ, LOUD, "\n");

#if CFG_TX_FRAGMENT

	return WLAN_STATUS_SUCCESS;

#else

	return WLAN_STATUS_NOT_SUPPORTED;
#endif /* CFG_TX_FRAGMENT */

}				/* end of wlanoidQueryFragThreshold() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set a new fragmentation threshold to the
 *        driver.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetFragThreshold(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen) {
#if CFG_TX_FRAGMENT
	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif /* CFG_TX_FRAGMENT */

} /* end of wlanoidSetFragThreshold() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the current RTS threshold.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryRtsThreshold(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen)
{
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	DBGLOG(REQ, LOUD, "\n");

	prWlanInfo = &prAdapter->rWlanInfo;

	if (u4QueryBufferLen < sizeof(uint32_t)) {
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	*((uint32_t *) pvQueryBuffer) = prWlanInfo->eRtsThreshold;

	return WLAN_STATUS_SUCCESS;

}				/* wlanoidQueryRtsThreshold */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set a new RTS threshold to the driver.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetRtsThreshold(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen)
{
	uint32_t *prRtsThreshold;
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	prWlanInfo = &prAdapter->rWlanInfo;

	*pu4SetInfoLen = sizeof(uint32_t);
	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, WARN, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prRtsThreshold = (uint32_t *) pvSetBuffer;
	*prRtsThreshold = prWlanInfo->eRtsThreshold;

	return WLAN_STATUS_SUCCESS;

} /* wlanoidSetRtsThreshold */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to turn radio off.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetDisassociate(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen) {
	struct MSG_AIS_ABORT *prAisAbortMsg;
	uint32_t u4DisconnectReason = DISCONNECT_REASON_CODE_LOCALLY;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;
	struct AIS_FSM_INFO *prAisFsmInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	*pu4SetInfoLen = 0;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set disassociate! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		DBGLOG(REQ, WARN, "ucBssIndex %d not ais\n", ucBssIndex);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);

	/* Send AIS Abort Message */
	prAisAbortMsg = (struct MSG_AIS_ABORT *) cnmMemAlloc(
						prAdapter, RAM_TYPE_MSG,
						sizeof(struct MSG_AIS_ABORT));
	if (!prAisAbortMsg) {
		DBGLOG(REQ, ERROR, "Fail in creating AisAbortMsg.\n");
		return WLAN_STATUS_FAILURE;
	}

	prAisAbortMsg->rMsgHdr.eMsgId = MID_OID_AIS_FSM_JOIN_REQ;
	if (pvSetBuffer == NULL || u4SetBufferLen == 0) {
		prAisAbortMsg->ucReasonOfDisconnect =
			DISCONNECT_REASON_CODE_LOCALLY;
	} else {
		u4DisconnectReason = *((uint32_t *)pvSetBuffer);
		prAisAbortMsg->ucReasonOfDisconnect =
			u4DisconnectReason == DISCONNECT_REASON_CODE_DEL_IFACE ?
			DISCONNECT_REASON_CODE_LOCALLY :
			u4DisconnectReason;
	}

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_SCAN ||
			prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN)
		prAisFsmInfo->fgIsScanOidAborted = TRUE;
	if (u4DisconnectReason == DISCONNECT_REASON_CODE_DEL_IFACE)
		prAisFsmInfo->fgIsDelIface = TRUE;

	prAisAbortMsg->fgDelayIndication = FALSE;
	prAisAbortMsg->ucBssIndex = ucBssIndex;
	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prAisAbortMsg, MSG_SEND_METHOD_BUF);

	/* indicate for disconnection */
	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
		ucBssIndex) ==
	    MEDIA_STATE_CONNECTED)
		kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
			     WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY, NULL,
			     0, ucBssIndex);

#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
	prAdapter->fgSuppSmeLinkDownPend = TRUE;

	return WLAN_STATUS_PENDING;
#else
	if (u4DisconnectReason == DISCONNECT_REASON_CODE_DEL_IFACE) {
		prAdapter->fgSuppSmeLinkDownPend = TRUE;

		return WLAN_STATUS_PENDING;
	}

	return WLAN_STATUS_SUCCESS;
#endif
}				/* wlanoidSetDisassociate */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to query the power save profile.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQuery802dot11PowerSaveProfile(struct ADAPTER *prAdapter,
				     void *pvQueryBuffer,
				     uint32_t u4QueryBufferLen,
				     uint32_t *pu4QueryInfoLen) {
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	struct BSS_INFO *prBssInfo;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "ucBssIndex:%d not found\n", ucBssIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u4QueryBufferLen != 0 && pvQueryBuffer) {
		*(enum PARAM_POWER_MODE *) pvQueryBuffer =
			prBssInfo->ePwrMode;
		*pu4QueryInfoLen = sizeof(enum PARAM_POWER_MODE);

		/* hack for CTIA power mode setting function */
		if (prAdapter->fgEnCtiaPowerMode) {
			/* set to non-zero value (to prevent MMI query 0, */
			/* before it intends to set 0, which will skip its
			 * following state machine)
			 */
			*(enum PARAM_POWER_MODE *) pvQueryBuffer =
				(enum PARAM_POWER_MODE) 2;
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set the power save profile.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSet802dot11PowerSaveProfile(struct ADAPTER *
				   prAdapter,
				   void *pvSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen) {
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct PARAM_POWER_MODE_ *prPowerMode;
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo = NULL;
#endif

	const uint8_t *apucPsMode[Param_PowerModeMax] = {
		(uint8_t *) "CAM",
		(uint8_t *) "MAX PS",
		(uint8_t *) "FAST PS"
	};

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_POWER_MODE_);
	prPowerMode = (struct PARAM_POWER_MODE_ *) pvSetBuffer;

	if (u4SetBufferLen < sizeof(struct PARAM_POWER_MODE_)) {
		DBGLOG(REQ, WARN,
		       "Set power mode error: Invalid length %u\n",
		       u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	} else if (prPowerMode->ePowerMode >= Param_PowerModeMax) {
		DBGLOG(REQ, WARN,
		       "Set power mode error: Invalid power mode(%u)\n",
		       prPowerMode->ePowerMode);
		return WLAN_STATUS_INVALID_DATA;
	} else if (prPowerMode->ucBssIdx >=
		   prAdapter->ucSwBssIdNum) {
		DBGLOG(REQ, WARN,
		       "Set power mode error: Invalid BSS index(%u)\n",
		       prPowerMode->ucBssIdx);
		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  prPowerMode->ucBssIdx);

	if (prBssInfo == NULL) {
		DBGLOG(REQ, WARN, "BSS Info not exist !!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (prAdapter->fgEnCtiaPowerMode) {
		if (prPowerMode->ePowerMode != Param_PowerModeCAM) {
			/* User setting to PS mode (Param_PowerModeMAX_PSP or
			 * Param_PowerModeFast_PSP)
			 */

			if (prAdapter->u4CtiaPowerMode == 0)
				/* force to keep in CAM mode */
				prPowerMode->ePowerMode = Param_PowerModeCAM;
			else if (prAdapter->u4CtiaPowerMode == 1)
				prPowerMode->ePowerMode =
							Param_PowerModeMAX_PSP;
			else if (prAdapter->u4CtiaPowerMode == 2)
				prPowerMode->ePowerMode =
							Param_PowerModeFast_PSP;
		}
	}

	/* only CAM mode allowed when TP/Sigma on */
	if ((prAdapter->rWifiVar.ucTpTestMode ==
	     ENUM_TP_TEST_MODE_THROUGHPUT) ||
	    (prAdapter->rWifiVar.ucTpTestMode ==
	     ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF))
		prPowerMode->ePowerMode = Param_PowerModeCAM;
	else if (prAdapter->rWifiVar.ePowerMode !=
		 Param_PowerModeMax)
		prPowerMode->ePowerMode = prAdapter->rWifiVar.ePowerMode;

	/* for WMM PS Sigma certification, keep WiFi in ps mode continuously */
	/* force PS == Param_PowerModeMAX_PSP */
	if ((prAdapter->rWifiVar.ucTpTestMode ==
	     ENUM_TP_TEST_MODE_SIGMA_WMM_PS) &&
	    (prPowerMode->ePowerMode >= Param_PowerModeMAX_PSP))
		prPowerMode->ePowerMode = Param_PowerModeMAX_PSP;

	if (prBssInfo->eNetworkType >= NETWORK_TYPE_NUM) {
		DBGLOG(INIT, WARN,
			   "Invalid eNetworkType: %d\n",
			   prBssInfo->eNetworkType);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBssInfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (prMldBssInfo) {
		struct BSS_INFO *bss;

		LINK_FOR_EACH_ENTRY(bss, &prMldBssInfo->rBssList,
					rLinkEntryMld, struct BSS_INFO) {
			if (bss->eNetworkType >= NETWORK_TYPE_NUM ||
			    bss->eNetworkType != prBssInfo->eNetworkType) {
				DBGLOG(INIT, WARN,
					   "Bss%d invalid eNetworkType: %d\n",
					   bss->ucBssIndex,
					   bss->eNetworkType);
				return WLAN_STATUS_NOT_ACCEPTED;
			}

			if (prAdapter->rWifiVar.ucPresetLinkId ==
							MLD_LINK_ID_NONE ||
			    prAdapter->rWifiVar.ucPresetLinkId ==
							bss->ucLinkIndex) {
				status = nicConfigPowerSaveProfile(prAdapter,
					bss->ucBssIndex,
					prPowerMode->ePowerMode,
					bss == LINK_PEEK_TAIL(
					&prMldBssInfo->rBssList,
					struct BSS_INFO, rLinkEntryMld),
					PS_CALLER_COMMON);
			}
		}

		prAdapter->rWifiVar.ucPresetLinkId = MLD_LINK_ID_NONE;
	} else
#endif
	{
		status = nicConfigPowerSaveProfile(prAdapter,
			prPowerMode->ucBssIdx, prPowerMode->ePowerMode,
			TRUE, PS_CALLER_COMMON);
	}

	if (prPowerMode->ePowerMode < Param_PowerModeMax &&
		prPowerMode->ePowerMode >= 0) {
		DBGLOG(INIT, TRACE,
		       "Set %s Network BSS(%u) PS mode to %s (%d)\n",
		       apucNetworkType[prBssInfo->eNetworkType],
		       prPowerMode->ucBssIdx,
		       apucPsMode[prPowerMode->ePowerMode],
		       prPowerMode->ePowerMode);
	} else {
		DBGLOG(INIT, TRACE,
		       "Invalid PS mode setting (%d) for %s Network BSS(%u)\n",
		       prPowerMode->ePowerMode,
		       apucNetworkType[prBssInfo->eNetworkType],
		       prPowerMode->ucBssIdx);
	}

	return status;
} /* end of wlanoidSetAcpiDevicePowerStateMode() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query current status of AdHoc Mode.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryAdHocMode(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen) {
	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidQueryAdHocMode() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set AdHoc Mode.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetAdHocMode(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen) {
	return WLAN_STATUS_SUCCESS;
} /* end of wlanoidSetAdHocMode() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query RF frequency.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryFrequency(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen) {
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);
	prAisBssInfo =
		aisGetAisBssInfo(prAdapter, ucBssIndex);

	if (prConnSettings->eOPMode ==
	    NET_TYPE_INFRA) {
		if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex) ==
		    MEDIA_STATE_CONNECTED)
			*(uint32_t *) pvQueryBuffer = nicChannelNum2Freq(
				prAisBssInfo->ucPrimaryChannel,
				prAisBssInfo->eBand);
		else
			*(uint32_t *) pvQueryBuffer = 0;
	} else
		*(uint32_t *) pvQueryBuffer = nicChannelNum2Freq(
			prConnSettings->ucAdHocChannelNum,
			prConnSettings->eAdHocBand);

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidQueryFrequency() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set RF frequency by User Settings.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetFrequency(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen) {
	struct CONNECTION_SETTINGS *prConnSettings;
	uint32_t *pu4FreqInKHz;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);

	*pu4SetInfoLen = sizeof(uint32_t);

	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	pu4FreqInKHz = (uint32_t *) pvSetBuffer;

	prConnSettings->ucAdHocChannelNum =
		(uint8_t) nicFreq2ChannelNum(*pu4FreqInKHz);
	prConnSettings->eAdHocBand = *pu4FreqInKHz
			< 5000000 ? BAND_2G4 : BAND_5G;

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidSetFrequency() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set 802.11 channel of the radio frequency.
 *        This is a proprietary function call to Lunux currently.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetChannel(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen) {
	ASSERT(0);		/* // */

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the Beacon Interval from User
 *	  Settings.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryBeaconInterval(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen) {
	struct CONNECTION_SETTINGS *prConnSettings;
	struct PARAM_BSSID_EX *prCurrBssid;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	struct WLAN_INFO *prWlanInfo;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);

	prCurrBssid = aisGetCurrBssId(prAdapter,
		ucBssIndex);

	*pu4QueryInfoLen = sizeof(uint32_t);

	if (u4QueryBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	prWlanInfo = &prAdapter->rWlanInfo;

	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
		ucBssIndex) ==
	    MEDIA_STATE_CONNECTED) {
		if (prConnSettings->eOPMode == NET_TYPE_INFRA)
			*(uint32_t *) pvQueryBuffer =
				prCurrBssid->rConfiguration
				.u4BeaconPeriod;
		else
			*(uint32_t *) pvQueryBuffer =
				(uint32_t)prWlanInfo->u2BeaconPeriod;
	} else {
		if (prConnSettings->eOPMode ==
		    NET_TYPE_INFRA)
			*(uint32_t *) pvQueryBuffer = 0;
		else
			*(uint32_t *) pvQueryBuffer =
				(uint32_t)prWlanInfo->u2BeaconPeriod;
	}

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidQueryBeaconInterval() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the Beacon Interval to User Settings.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetBeaconInterval(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen)
{
	struct WLAN_INFO *prWlanInfo;
	uint32_t *pu4BeaconInterval;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(uint32_t);
	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	pu4BeaconInterval = (uint32_t *) pvSetBuffer;

	if ((*pu4BeaconInterval < DOT11_BEACON_PERIOD_MIN)
	    || (*pu4BeaconInterval > DOT11_BEACON_PERIOD_MAX)) {
		DBGLOG(REQ, TRACE, "Invalid Beacon Interval = %u\n",
		       *pu4BeaconInterval);
		return WLAN_STATUS_INVALID_DATA;
	}

	prWlanInfo = &prAdapter->rWlanInfo;
	prWlanInfo->u2BeaconPeriod = (uint16_t)*pu4BeaconInterval;

	DBGLOG(REQ, INFO, "Set beacon interval: %d\n",
	       prWlanInfo->u2BeaconPeriod);

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidSetBeaconInterval() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query the ATIM window from User Settings.
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryAtimWindow(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen)
{
	struct WLAN_INFO *prWlanInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	*pu4QueryInfoLen = sizeof(uint32_t);

	if (u4QueryBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	prWlanInfo = &prAdapter->rWlanInfo;
	if (prConnSettings->eOPMode == NET_TYPE_INFRA)
		*(uint32_t *) pvQueryBuffer = 0;
	else
		*(uint32_t *)pvQueryBuffer = (uint32_t)prWlanInfo->u2AtimWindow;

	return WLAN_STATUS_SUCCESS;

}				/* end of wlanoidQueryAtimWindow() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the ATIM window to User Settings.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetAtimWindow(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	struct WLAN_INFO *prWlanInfo;

	uint32_t *pu4AtimWindow;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	prWlanInfo = &prAdapter->rWlanInfo;
	*pu4SetInfoLen = sizeof(uint32_t);

	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	pu4AtimWindow = (uint32_t *) pvSetBuffer;

	prWlanInfo->u2AtimWindow = (uint16_t) *pu4AtimWindow;

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidSetAtimWindow() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to Set the MAC address which is currently used
 *	  by the NIC.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetCurrentAddr(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	ASSERT(0);		/* // */

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanoidSetCurrentAddr() */

#if CFG_TCP_IP_CHKSUM_OFFLOAD
/*----------------------------------------------------------------------------*/
/*!
 * \brief Setting the checksum offload function.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetCSUMOffload(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	uint32_t u4CSUMFlags;
	struct CMD_BASIC_CONFIG rCmdBasicConfig;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(uint32_t);

	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	u4CSUMFlags = *(uint32_t *) pvSetBuffer;

	kalMemZero(&rCmdBasicConfig,
		   sizeof(struct CMD_BASIC_CONFIG));

	rCmdBasicConfig.ucNative80211 = 0;	/* @FIXME: for Vista */

	if (u4CSUMFlags & CSUM_OFFLOAD_EN_TX_TCP)
		rCmdBasicConfig.rCsumOffload.u2TxChecksum |= BIT(2);

	if (u4CSUMFlags & CSUM_OFFLOAD_EN_TX_UDP)
		rCmdBasicConfig.rCsumOffload.u2TxChecksum |= BIT(1);

	if (u4CSUMFlags & CSUM_OFFLOAD_EN_TX_IP)
		rCmdBasicConfig.rCsumOffload.u2TxChecksum |= BIT(0);

	if (u4CSUMFlags & CSUM_OFFLOAD_EN_RX_TCP)
		rCmdBasicConfig.rCsumOffload.u2RxChecksum |= BIT(2);

	if (u4CSUMFlags & CSUM_OFFLOAD_EN_RX_UDP)
		rCmdBasicConfig.rCsumOffload.u2RxChecksum |= BIT(1);

	if (u4CSUMFlags & (CSUM_OFFLOAD_EN_RX_IPv4 |
			   CSUM_OFFLOAD_EN_RX_IPv6))
		rCmdBasicConfig.rCsumOffload.u2RxChecksum |= BIT(0);

	prAdapter->u4CSUMFlags = u4CSUMFlags;
	rCmdBasicConfig.ucCtrlFlagAssertPath =
		prWifiVar->ucCtrlFlagAssertPath;
	rCmdBasicConfig.ucCtrlFlagDebugLevel =
		prWifiVar->ucCtrlFlagDebugLevel;

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_BASIC_CONFIG,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    nicOidCmdTimeoutCommon,
			    sizeof(struct CMD_BASIC_CONFIG),
			    (uint8_t *) &rCmdBasicConfig,
			    pvSetBuffer, u4SetBufferLen);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Setting the IP address for pattern search function.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_ADAPTER_NOT_READY
 * \return WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetNetworkAddress(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen) {
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t i, u4IPv4AddrIdx;
	struct CMD_SET_NETWORK_ADDRESS_LIST *prCmdNetworkAddressList;
	struct PARAM_NETWORK_ADDRESS_LIST *prNetworkAddressList;
	struct PARAM_NETWORK_ADDRESS *prNetworkAddress;
	uint32_t u4IPv4AddrCount, u4CmdSize;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = 4;

	if (u4SetBufferLen < sizeof(struct PARAM_NETWORK_ADDRESS_LIST))
		return WLAN_STATUS_INVALID_DATA;

	prNetworkAddressList = pvSetBuffer;
	*pu4SetInfoLen = 0;
	u4IPv4AddrCount = 0;

	/* 4 <1.1> Get IPv4 address count */
	/* We only suppot IPv4 address setting */
	prNetworkAddress =
		(struct PARAM_NETWORK_ADDRESS *)(prNetworkAddressList + 1);
	for (i = 0; i < prNetworkAddressList->u4AddressCount; i++) {
		if ((prNetworkAddress->u2AddressType ==
		     PARAM_PROTOCOL_ID_TCP_IP) &&
		    (prNetworkAddress->u2AddressLength == IPV4_ADDR_LEN)) {
			u4IPv4AddrCount++;
			prNetworkAddress = (struct PARAM_NETWORK_ADDRESS *)
			((uintptr_t) prNetworkAddress +
			(uintptr_t) (prNetworkAddress->u2AddressLength*2 +
			OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress)));
		} else {
			DBGLOG(OID, INFO, "type[%d] length[%d]",
				prNetworkAddress->u2AddressType,
				prNetworkAddress->u2AddressLength);

			prNetworkAddress = (struct PARAM_NETWORK_ADDRESS *)
				((uintptr_t) prNetworkAddress +
				(uintptr_t)
					(prNetworkAddress->u2AddressLength +
				OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
					aucAddress)));
		}
	}

	/* 4 <2> Calculate command buffer size */
	/* construct payload of command packet */
	if (u4IPv4AddrCount == 0)
		u4CmdSize = sizeof(struct CMD_SET_NETWORK_ADDRESS_LIST);
	else
		u4CmdSize =
			OFFSET_OF(struct CMD_SET_NETWORK_ADDRESS_LIST,
				  arNetAddress) +
			(sizeof(struct CMD_IPV4_NETWORK_ADDRESS) *
			u4IPv4AddrCount);

	/* 4 <3> Allocate command buffer */
	prCmdNetworkAddressList = (struct CMD_SET_NETWORK_ADDRESS_LIST *)
					kalMemAlloc(u4CmdSize, VIR_MEM_TYPE);

	if (prCmdNetworkAddressList == NULL)
		return WLAN_STATUS_FAILURE;

	kalMemZero(prCmdNetworkAddressList, u4CmdSize);
	prCmdNetworkAddressList->ucVersion = 1;

	/* 4 <4> Fill P_CMD_SET_NETWORK_ADDRESS_LIST */
	prCmdNetworkAddressList->ucBssIndex = prNetworkAddressList->ucBssIdx;

	/* only to set IP address to FW once ARP filter is enabled */
	if (prAdapter->fgEnArpFilter) {
		prCmdNetworkAddressList->ucAddressCount =
			(uint8_t) u4IPv4AddrCount;
		prNetworkAddress = (struct PARAM_NETWORK_ADDRESS *)
					(prNetworkAddressList + 1);

		/* DBGLOG(INIT, INFO, ("%s: u4IPv4AddrCount (%lu)\n",
		 *        __FUNCTION__, u4IPv4AddrCount));
		 */

		for (i = 0, u4IPv4AddrIdx = 0;
		     i < prNetworkAddressList->u4AddressCount; i++) {
			if (prNetworkAddress->u2AddressType ==
			    PARAM_PROTOCOL_ID_TCP_IP &&
			    prNetworkAddress->u2AddressLength ==
			    IPV4_ADDR_LEN) {

				kalMemCopy(prCmdNetworkAddressList->
					arNetAddress[u4IPv4AddrIdx].aucIpAddr,
					prNetworkAddress->aucAddress,
					sizeof(uint32_t));
				kalMemCopy(prCmdNetworkAddressList->
					arNetAddress[u4IPv4AddrIdx].aucIpMask,
					prNetworkAddress->
					aucAddress+sizeof(uint32_t),
					sizeof(uint32_t));

				DBGLOG(OID, INFO,
				"%s:IPv4 Addr[%u]["IPV4STR"]Mask["IPV4STR
				"]BSS[%d] ver[%d]\n",
				__func__,
				u4IPv4AddrIdx,
				IPV4TOSTR(prNetworkAddress->aucAddress),
				IPV4TOSTR(prNetworkAddress->
				aucAddress+sizeof(uint32_t)),
				prCmdNetworkAddressList->ucBssIndex,
				prCmdNetworkAddressList->ucVersion);

				u4IPv4AddrIdx++;

				prNetworkAddress =
					(struct PARAM_NETWORK_ADDRESS *)
					((uintptr_t)prNetworkAddress +
					(uintptr_t)(prNetworkAddress->
					u2AddressLength*2 +
					OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
					aucAddress)));
			} else {
				prNetworkAddress =
					(struct PARAM_NETWORK_ADDRESS *)
					((uintptr_t) prNetworkAddress +
					(uintptr_t)
					(prNetworkAddress->u2AddressLength +
					OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
					aucAddress)));
			}


		}

	} else {
		prCmdNetworkAddressList->ucAddressCount = 0;
	}

	/* 4 <5> Send command */
	rStatus = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_SET_IP_ADDRESS,
				      TRUE,
				      FALSE,
				      TRUE,
				      nicCmdEventSetIpAddress,
				      nicOidCmdTimeoutCommon,
				      u4CmdSize,
				      (uint8_t *) prCmdNetworkAddressList,
				      pvSetBuffer,
				      u4SetBufferLen);

	kalMemFree(prCmdNetworkAddressList, VIR_MEM_TYPE,
		   u4CmdSize);
	return rStatus;
}

/* fos_change begin */
#if CFG_SUPPORT_SET_IPV6_NETWORK
/*----------------------------------------------------------------------------*/
/*!
 * \brief Setting the IPV6 address for pattern search function.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_ADAPTER_NOT_READY
 * \return WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetIPv6NetworkAddress(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t i, u4CmdSize;
	uint32_t u4IPv6AddrCount = 0;
	struct CMD_IPV6_NETWORK_ADDRESS_LIST *prCmdIPv6NetworkAddressList;
	struct PARAM_NETWORK_ADDRESS_LIST *prNetworkAddressList;
	struct PARAM_NETWORK_ADDRESS *prNetworkAddress;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = 4;

	if (u4SetBufferLen < sizeof(struct PARAM_NETWORK_ADDRESS_LIST))
		return WLAN_STATUS_INVALID_DATA;

	prNetworkAddressList = pvSetBuffer;
	*pu4SetInfoLen = 0;

	/* 4 <1.1> Get IPv6 address count */
	prNetworkAddress =
		(struct PARAM_NETWORK_ADDRESS *)(prNetworkAddressList + 1);
	for (i = 0; i < prNetworkAddressList->u4AddressCount; i++) {
		if ((prNetworkAddress->u2AddressType ==
		     PARAM_PROTOCOL_ID_TCP_IP) &&
		    (prNetworkAddress->u2AddressLength == IPV6_ADDR_LEN)) {
			u4IPv6AddrCount++;
			prNetworkAddress = (struct PARAM_NETWORK_ADDRESS *)
				((uintptr_t) prNetworkAddress +
				(uintptr_t)
					(prNetworkAddress->u2AddressLength +
				OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
					aucAddress)));
		} else {
			prNetworkAddress = (struct PARAM_NETWORK_ADDRESS *)
				((uintptr_t)
					prNetworkAddress +
				(uintptr_t)
					(prNetworkAddress->u2AddressLength * 2 +
				OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
					aucAddress)));
		}
	}


	/* 4 <2> Calculate command buffer size */
	/* construct payload of command packet */
	if (u4IPv6AddrCount == 0)
		u4CmdSize = sizeof(struct CMD_IPV6_NETWORK_ADDRESS_LIST);
	else
		u4CmdSize =
			OFFSET_OF(struct CMD_IPV6_NETWORK_ADDRESS_LIST,
				  arNetAddress) +
			(sizeof(struct CMD_IPV6_NETWORK_ADDRESS) *
			u4IPv6AddrCount);

	/* 4 <3> Allocate command buffer */
	prCmdIPv6NetworkAddressList =
		(struct CMD_IPV6_NETWORK_ADDRESS_LIST *)
		kalMemAlloc(u4CmdSize, VIR_MEM_TYPE);

	if (prCmdIPv6NetworkAddressList == NULL)
		return WLAN_STATUS_FAILURE;

	/* 4 <4> Fill P_CMD_SET_NETWORK_ADDRESS_LIST */
	prCmdIPv6NetworkAddressList->ucBssIndex =
		prNetworkAddressList->ucBssIdx;

	/* only to set IP address to FW once ARP filter is enabled */
	if (prAdapter->fgEnArpFilter) {
		prCmdIPv6NetworkAddressList->ucAddressCount =
			(uint8_t) u4IPv6AddrCount;
		prNetworkAddress = (struct PARAM_NETWORK_ADDRESS *)
			(prNetworkAddressList + 1);

		for (i = 0, u4IPv6AddrCount = 0;
		     i < prNetworkAddressList->u4AddressCount; i++) {
			if (prNetworkAddress->u2AddressType ==
			    PARAM_PROTOCOL_ID_TCP_IP &&
			    prNetworkAddress->u2AddressLength ==
			    IPV6_ADDR_LEN) {

				kalMemCopy(prCmdIPv6NetworkAddressList
				->arNetAddress[u4IPv6AddrCount].aucIpAddr,
				prNetworkAddress->aucAddress,
				sizeof(struct CMD_IPV6_NETWORK_ADDRESS));
				DBGLOG(INIT, INFO,
					"%s: IPv6 Addr [%u][" IPV6STR
					"]BSS[%d]\n",
					__func__, u4IPv6AddrCount,
					IPV6TOSTR(prNetworkAddress->aucAddress),
				       prCmdIPv6NetworkAddressList->ucBssIndex);

				u4IPv6AddrCount++;
				prNetworkAddress =
					(struct PARAM_NETWORK_ADDRESS *)
					((uintptr_t) prNetworkAddress +
					(uintptr_t)
					(prNetworkAddress->u2AddressLength +
					OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
							aucAddress)));
			} else {

				prNetworkAddress =
					(struct PARAM_NETWORK_ADDRESS *)
					((uintptr_t)prNetworkAddress +
					(uintptr_t)
					(prNetworkAddress->u2AddressLength * 2 +
					OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
							aucAddress)));
			}

		}

	} else {
		prCmdIPv6NetworkAddressList->ucAddressCount = 0;
	}

	/* 4 <5> Send command */
	rStatus = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_SET_IPV6_ADDRESS,
				      TRUE,
				      FALSE,
				      TRUE,
				      nicCmdEventSetIpv6Address,
				      nicOidCmdTimeoutCommon,
				      u4CmdSize,
				      (uint8_t *) prCmdIPv6NetworkAddressList,
				      pvSetBuffer,
				      u4SetBufferLen);

	kalMemFree(prCmdIPv6NetworkAddressList, VIR_MEM_TYPE, u4CmdSize);
	return rStatus;
}
#endif /* fos_change end */

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
 * \brief Set driver to switch into RF test mode
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set,
 *                        should be NULL
 * \param[in] u4SetBufferLen The length of the set buffer, should be 0
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_ADAPTER_NOT_READY
 * \return WLAN_STATUS_INVALID_DATA
 * \return WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidRftestSetTestMode(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen) {
	uint32_t rStatus;
	struct CMD_TEST_CTRL rCmdTestCtrl = {0};

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = 0;

	if (u4SetBufferLen == 0) {
		if (prAdapter->fgTestMode == FALSE) {
			/* switch to RF Test mode */
			rCmdTestCtrl.ucAction = CMD_TEST_CTRL_ACT_SWITCH_MODE;
			rCmdTestCtrl.u.u4OpMode = CMD_TEST_CTRL_ACT_SWITCH_MODE_RF_TEST;

			rStatus = wlanSendSetQueryCmd(prAdapter,
						CMD_ID_TEST_CTRL,
						TRUE,
						FALSE,
						TRUE,
						nicCmdEventEnterRfTest,
						nicOidCmdEnterRFTestTimeout,
						sizeof(struct CMD_TEST_CTRL),
						(uint8_t *) &rCmdTestCtrl,
						pvSetBuffer, u4SetBufferLen);
		} else {
			/* already in test mode .. */
			rStatus = WLAN_STATUS_SUCCESS;
		}
	} else {
		rStatus = WLAN_STATUS_INVALID_DATA;
	}

	return rStatus;
}

#if CFG_SUPPORT_XONVRAM
uint32_t
wlanoidRftestDoXOCal(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return nicUniCmdTestmodeXOCal(prAdapter,
		pvQueryBuffer,
		u4QueryBufferLen);
#else
	DBGLOG(OID, WARN, "NOT supported.\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}
#endif /* CFG_SUPPORT_XONVRAM */

#if CFG_SUPPORT_PLCAL
uint32_t
wlanoidRftestDoPlCal(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return nicUniCmdTestmodePlCal(prAdapter,
		pvQueryBuffer,
		u4QueryBufferLen);
#else
	DBGLOG(OID, WARN, "NOT supported.\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}
#endif /* CFG_SUPPORT_PLCAL */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set driver to switch into RF test ICAP mode
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set,
 *                        should be NULL
 * \param[in] u4SetBufferLen The length of the set buffer, should be 0
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_ADAPTER_NOT_READY
 * \return WLAN_STATUS_INVALID_DATA
 * \return WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidRftestSetTestIcapMode(struct ADAPTER *prAdapter,
			     void *pvSetBuffer, uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen) {
	uint32_t rStatus;
	struct CMD_TEST_CTRL rCmdTestCtrl = {0};
	struct ICAP_INFO_T *prIcapInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	prIcapInfo = &prAdapter->rIcapInfo;
	ASSERT(prIcapInfo);

	*pu4SetInfoLen = 0;

	if (u4SetBufferLen == 0) {
		if (prIcapInfo->eIcapState == ICAP_STATE_INIT) {
			/* switch to RF Test mode */
			rCmdTestCtrl.ucAction = CMD_TEST_CTRL_ACT_SWITCH_MODE;
			rCmdTestCtrl.u.u4OpMode = CMD_TEST_CTRL_ACT_SWITCH_MODE_ICAP;

			rStatus = wlanSendSetQueryCmd(prAdapter,
					      CMD_ID_TEST_CTRL,
					      TRUE,
					      FALSE,
					      TRUE,
					      nicCmdEventEnterRfTest,
					      nicOidCmdEnterRFTestTimeout,
					      sizeof(struct CMD_TEST_CTRL),
					      (uint8_t *) &rCmdTestCtrl,
					      pvSetBuffer, u4SetBufferLen);
		} else {
			/* already in ICAP mode .. */
			DBGLOG(RFTEST, WARN,
		       "Switch ICAP FAil in State(%d)\n",
		       prIcapInfo->eIcapState);
			rStatus = WLAN_STATUS_SUCCESS;
		}
	} else {
		rStatus = WLAN_STATUS_INVALID_DATA;
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set driver to switch into normal operation mode from RF test mode
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set
 *                        should be NULL
 * \param[in] u4SetBufferLen The length of the set buffer, should be 0
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_ADAPTER_NOT_READY
 * \return WLAN_STATUS_INVALID_DATA
 * \return WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidRftestSetAbortTestMode(struct ADAPTER *prAdapter,
			      void *pvSetBuffer, uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen) {
	uint32_t rStatus;
	struct CMD_TEST_CTRL rCmdTestCtrl = {0};

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = 0;

	if (u4SetBufferLen == 0) {
		if (prAdapter->fgTestMode == TRUE) {
			/* switch to normal mode */
			rCmdTestCtrl.ucAction = CMD_TEST_CTRL_ACT_SWITCH_MODE;
			rCmdTestCtrl.u.u4OpMode = CMD_TEST_CTRL_ACT_SWITCH_MODE_NORMAL;

			rStatus = wlanSendSetQueryCmd(prAdapter,
						CMD_ID_TEST_CTRL,
						TRUE,
						FALSE,
						TRUE,
						nicCmdEventLeaveRfTest,
						nicOidCmdTimeoutCommon,
						sizeof(struct CMD_TEST_CTRL),
						(uint8_t *) &rCmdTestCtrl,
						pvSetBuffer, u4SetBufferLen);
		} else {
			/* already in normal mode .. */
			rStatus = WLAN_STATUS_SUCCESS;
		}
	} else {
		rStatus = WLAN_STATUS_INVALID_DATA;
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief query for RF test parameter
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 * \retval WLAN_STATUS_NOT_SUPPORTED
 * \retval WLAN_STATUS_NOT_ACCEPTED
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidRftestQueryAutoTest(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen) {
	struct PARAM_MTK_WIFI_TEST_STRUCT *prRfATInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_MTK_WIFI_TEST_STRUCT);

	if (u4QueryBufferLen < sizeof(struct PARAM_MTK_WIFI_TEST_STRUCT)) {
		DBGLOG(REQ, ERROR, "Invalid data. QueryBufferLen: %u.\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prRfATInfo = (struct PARAM_MTK_WIFI_TEST_STRUCT *)
		     pvQueryBuffer;

	DBGLOG(RFTEST, INFO,
	       "Get AT_CMD BufferLen = %d, AT Index = %d, Data = %d\n",
	       u4QueryBufferLen,
	       prRfATInfo->u4FuncIndex,
	       prRfATInfo->u4FuncData);

	rStatus = rftestQueryATInfo(prAdapter,
				    prRfATInfo->u4FuncIndex,
				    prRfATInfo->u4FuncData,
				    pvQueryBuffer, u4QueryBufferLen);

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set RF test parameter
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_ADAPTER_NOT_READY
 * \return WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidRftestSetAutoTest(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen) {
	struct PARAM_MTK_WIFI_TEST_STRUCT *prRfATInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_MTK_WIFI_TEST_STRUCT);

	if (u4SetBufferLen < sizeof(struct PARAM_MTK_WIFI_TEST_STRUCT)) {
		DBGLOG(REQ, ERROR, "Invalid data. SetBufferLen: %u.\n",
		       u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prRfATInfo = (struct PARAM_MTK_WIFI_TEST_STRUCT *)
		     pvSetBuffer;

	DBGLOG(RFTEST, INFO,
	       "Set AT_CMD BufferLen = %d, AT Index = %d, Data = %d\n",
	       u4SetBufferLen,
	       prRfATInfo->u4FuncIndex,
	       prRfATInfo->u4FuncData);

	rStatus = rftestSetATInfo(prAdapter,
			  prRfATInfo->u4FuncIndex, prRfATInfo->u4FuncData);

	return rStatus;
}

/* RF test OID set handler */
uint32_t rftestSetATInfo(struct ADAPTER *prAdapter,
			 uint32_t u4FuncIndex, uint32_t u4FuncData)
{
	struct CMD_TEST_CTRL rCmdTestCtrl = {0};

	ASSERT(prAdapter);

	rCmdTestCtrl.ucAction = CMD_TEST_CTRL_ACT_SET_AT;
	rCmdTestCtrl.u.rRfATInfo.u4FuncIndex = u4FuncIndex;
	rCmdTestCtrl.u.rRfATInfo.u4FuncData = u4FuncData;

	if ((u4FuncIndex == RF_AT_FUNCID_COMMAND)
	    && (u4FuncData == RF_AT_COMMAND_ICAP))
		prAdapter->rIcapInfo.eIcapState = ICAP_STATE_START;

	/* ICAP dump name Reset */
	if ((u4FuncIndex == RF_AT_FUNCID_COMMAND)
	    && (u4FuncData == RF_AT_COMMAND_RESET_DUMP_NAME))
		prAdapter->rIcapInfo.u2DumpIndex = 0;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_TEST_CTRL,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_TEST_CTRL),
				   (uint8_t *) &rCmdTestCtrl,
				   NULL, 0);
}

uint32_t wlanoidExtRfTestICapStart(struct ADAPTER *prAdapter,
				   void *pvSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen)
{
	struct CMD_TEST_CTRL_EXT_T rCmdTestCtrl = {0};
	struct RBIST_CAP_START_T *prCmdICapInfo;
	struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T *prRfATInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_MTK_WIFI_TEST_STRUCT_EXT_T);

	prRfATInfo = (struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T *)
		     pvSetBuffer;

	DBGLOG(RFTEST, INFO,
	       "Set AT_CMD BufferLen = %d, AT Index = %d\n",
	       u4SetBufferLen,
	       prRfATInfo->u4FuncIndex);

	rCmdTestCtrl.ucAction = ACTION_IN_RFTEST;
	rCmdTestCtrl.u.rRfATInfo.u4FuncIndex =
		SET_ICAP_CAPTURE_START;

	prCmdICapInfo = &(rCmdTestCtrl.u.rRfATInfo.Data.rICapInfo);
	kalMemCopy(prCmdICapInfo, &(prRfATInfo->Data.rICapInfo),
		   sizeof(struct RBIST_CAP_START_T));

	if (prCmdICapInfo->u4Trigger == TRUE)
		prAdapter->rIcapInfo.eIcapState = ICAP_STATE_START;
	else
		/* ICAP STOP, reset state to INIT state*/
		prAdapter->rIcapInfo.eIcapState = ICAP_STATE_INIT;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
			 CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			 EXT_CMD_ID_RF_TEST,
			 TRUE, /* Query Bit: True->write False->read */
			 FALSE,/*fgNeedRsp*/
			 TRUE, /*fgIsOid*/
			 nicCmdEventSetCommon,
			 nicOidCmdTimeoutCommon,
			 sizeof(struct CMD_TEST_CTRL_EXT_T),
			 (uint8_t *)&rCmdTestCtrl, pvSetBuffer,
			 u4SetBufferLen);
	return rStatus;
}

uint32_t wlanoidExtRfTestICapStatus(struct ADAPTER *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	struct CMD_TEST_CTRL_EXT_T rCmdTestCtrl = {0};
	struct RBIST_CAP_START_T *prCmdICapInfo;
	struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T *prRfATInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_MTK_WIFI_TEST_STRUCT_EXT_T);

	prRfATInfo = (struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T *)
		     pvSetBuffer;

	DBGLOG(RFTEST, INFO,
	       "Set AT_CMD BufferLen = %d, AT Index = %d\n",
	       u4SetBufferLen,
	       prRfATInfo->u4FuncIndex);

	rCmdTestCtrl.ucAction = ACTION_IN_RFTEST;
	rCmdTestCtrl.u.rRfATInfo.u4FuncIndex =
		GET_ICAP_CAPTURE_STATUS;


	prAdapter->rIcapInfo.eIcapState = ICAP_STATE_QUERY_STATUS;


	prCmdICapInfo = &(rCmdTestCtrl.u.rRfATInfo.Data.rICapInfo);
	kalMemCopy(prCmdICapInfo, &(prRfATInfo->Data.rICapInfo),
		   sizeof(struct RBIST_CAP_START_T));

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
				 CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				 EXT_CMD_ID_RF_TEST,
				 FALSE, /* Query Bit: True->write False->read */
				 TRUE,
				 TRUE,
				 NULL,
				 nicOidCmdTimeoutCommon,
				 sizeof(struct CMD_TEST_CTRL_EXT_T),
				 (uint8_t *)(&rCmdTestCtrl), pvSetBuffer,
				 u4SetBufferLen);
	return rStatus;
}

void wlanoidRfTestICapRawDataProc(struct ADAPTER *
				  prAdapter, uint32_t u4CapStartAddr,
				  uint32_t u4TotalBufferSize)
{
	struct CMD_TEST_CTRL_EXT_T rCmdTestCtrl = {0};
	struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T *prRfATInfo;
	uint32_t u4SetBufferLen = 0;
	void *pvSetBuffer = NULL;
	int32_t rStatus;

	DBGLOG(RFTEST, INFO, "wlanoidRfTestICapRawDataProc\n");
	ASSERT(prAdapter);

	prRfATInfo = &(rCmdTestCtrl.u.rRfATInfo);

	rCmdTestCtrl.ucAction = ACTION_IN_RFTEST;
	prRfATInfo->u4FuncIndex = GET_ICAP_RAW_DATA;
	prRfATInfo->Data.rICapDump.u4Address = u4CapStartAddr;
	prRfATInfo->Data.rICapDump.u4AddrOffset = 0x04;
	prRfATInfo->Data.rICapDump.u4Bank = 1;
	prRfATInfo->Data.rICapDump.u4BankSize = u4TotalBufferSize;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
				 CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				 EXT_CMD_ID_RF_TEST,
				 FALSE, /* Query Bit: True->write False->read */
				 TRUE,
				 FALSE, /*fgIsOid = FALSE, main thread trigger*/
				 NULL,
				 nicOidCmdTimeoutCommon,
				 sizeof(struct CMD_TEST_CTRL_EXT_T),
				 (uint8_t *)(&rCmdTestCtrl),
				 pvSetBuffer, u4SetBufferLen);

}

#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief wifi driver response IQ data for QA agent
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] ucCID              Command ID
 * \param[in] fgSetQuery         Set or Query
 * \param[in] fgNeedResp         Need for response
 * \param[in] pfCmdDoneHandler   Function pointer when command is done
 * \param[in] u4SetQueryInfoLen  The length of the set/query buffer
 * \param[in] pucInfoBuffer      Pointer to set/query buffer
 *
 *
 * \retval WLAN_STATUS_PENDING
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/

uint32_t wlanoidRfTestICapGetIQData(struct ADAPTER *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct _RBIST_IQ_DATA_T *prIQArray = NULL;
	struct ICAP_INFO_T *prICapInfo = NULL;
	struct RBIST_DUMP_IQ_T *prRbistDump = NULL;
	struct RBIST_DUMP_IQ_T *prTmpRbistDump = NULL;
	struct CMD_TEST_CTRL_EXT_T rCmdTestCtrl = {0};
	struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T *prRfATInfo;

	if (!prAdapter) {
		DBGLOG(RFTEST, ERROR, "prAdapter is null\n");
		return WLAN_STATUS_FAILURE;
	}
	if (!pvSetBuffer) {
		DBGLOG(RFTEST, ERROR, "pvSetBuffer is null\n");
		return WLAN_STATUS_FAILURE;
	}
	if (!pu4SetInfoLen) {
		DBGLOG(RFTEST, ERROR, "pu4SetInfoLen is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prICapInfo = &prAdapter->rIcapInfo;
	prIQArray = prICapInfo->prIQArray;
	prTmpRbistDump = (struct RBIST_DUMP_IQ_T *)pvSetBuffer;

	/* store parameter to adpater */
	prRbistDump = &prAdapter->QAICapInfo;
	prRbistDump->u4WfNum = prTmpRbistDump->u4WfNum;
	prRbistDump->u4IQType = prTmpRbistDump->u4IQType;
	prRbistDump->pIcapData = prTmpRbistDump->pIcapData;
	prRbistDump->u4IcapDataLen = prTmpRbistDump->u4IcapDataLen;


	prICapInfo->eIcapState = ICAP_STATE_QA_TOOL_CAPTURE;


	prRfATInfo = &(rCmdTestCtrl.u.rRfATInfo);

	rCmdTestCtrl.ucAction = ACTION_IN_RFTEST;
	prRfATInfo->u4FuncIndex = GET_ICAP_RAW_DATA;
	prRfATInfo->Data.rICapDump.u4Address = 0;
	prRfATInfo->Data.rICapDump.u4AddrOffset = 0x04;
	prRfATInfo->Data.rICapDump.u4Bank = 1;
	prRfATInfo->Data.rICapDump.u4BankSize = 0;
	prRfATInfo->Data.rICapDump.u4WFNum = prRbistDump->u4WfNum;
	prRfATInfo->Data.rICapDump.u4IQType = prRbistDump->u4IQType;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
				 CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				 EXT_CMD_ID_RF_TEST,
				 FALSE, /* Query Bit: True->write False->read */
				 TRUE,
				 TRUE, /*fgIsOid = FALSE, main thread trigger*/
				 NULL,
				 nicOidCmdTimeoutCommon,
				 sizeof(struct CMD_TEST_CTRL_EXT_T),
				 (uint8_t *)(&rCmdTestCtrl),
				 pvSetBuffer, u4SetBufferLen);

	return rStatus;
}

uint32_t wlanoidRfTestICapCopyDataToQA(struct ADAPTER *prAdapter,
					void *pvSetBuffer,
					uint32_t u4SetBufferLen,
					uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct RBIST_DUMP_IQ_T *prRbistDump = NULL;
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prICapEvent = NULL;
	struct RBIST_DUMP_IQ_T *prQAICapInfo = NULL;
	uint32_t u4WFNum = 0, u4IQType = 0;

	DBGLOG(RFTEST, INFO, "wlanoidRfTestICapCopyDataToQA\n");

	if (!prAdapter) {
		DBGLOG(RFTEST, ERROR, "prAdapter is null\n");
		return WLAN_STATUS_FAILURE;
	}
	if (!pvSetBuffer) {
		DBGLOG(RFTEST, ERROR, "pvSetBuffer is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prRbistDump = (struct RBIST_DUMP_IQ_T *)pvSetBuffer;
	u4WFNum = prRbistDump->u4WfNum;
	u4IQType = prRbistDump->u4IQType;

	prICapEvent = &prAdapter->IcapDumpEvent;
	ASSERT(prICapEvent);

	prQAICapInfo = &prAdapter->QAICapInfo;
	ASSERT(prQAICapInfo);

	/* update QATool's int32 IcapData */
	prRbistDump->pIcapData = prQAICapInfo->pIcapData;

	/* update IQ sample count to QATool's IQNumberCount */
	prRbistDump->u4IcapCnt = prICapEvent->u4DataLength;

	/* update QATool's response IQ data length */
	prRbistDump->u4IcapDataLen =
				prICapEvent->u4DataLength * sizeof(uint32_t);


	DBGLOG(RFTEST, INFO, "CurrICapDumpIndex[WF%d][%c],IQCnt=%d,len=%d\n",
						u4WFNum,
						(u4IQType == CAP_I_TYPE) ?
						'I' : 'Q',
						prRbistDump->u4IcapCnt,
						prRbistDump->u4IcapDataLen);

	return rStatus;
}

#else /* #if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1) */

uint32_t wlanoidRfTestICapGetIQData(struct ADAPTER *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct _RBIST_IQ_DATA_T *prIQArray = NULL;
	struct ICAP_INFO_T *prICapInfo = NULL;
	struct RBIST_DUMP_IQ_T *prRbistDump = NULL;
	int32_t i = 0;
	uint32_t u4MaxIQDataCount = 0;
	uint32_t u4DumpIndex = 0;
	uint32_t u4Value, u4DataLen = 0;
	uint32_t u4WFNum = 0, u4IQType = 0;
	uint8_t *pData;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	prICapInfo = &prAdapter->rIcapInfo;
	prIQArray = prICapInfo->prIQArray;
	prRbistDump = (struct RBIST_DUMP_IQ_T *)pvSetBuffer;
	u4WFNum = prRbistDump->u4WfNum;
	u4IQType = prRbistDump->u4IQType;
	pData = prRbistDump->pucIcapData;

	u4DumpIndex = prICapInfo->au4ICapDumpIndex[u4WFNum][u4IQType];

	prICapInfo->eIcapState = ICAP_STATE_QA_TOOL_CAPTURE;



	/* 1. Maximum 1KB = ICAP_EVENT_DATA_SAMPLE (256) slots */
	u4MaxIQDataCount = prICapInfo->u4IQArrayIndex - u4DumpIndex;
	if (u4MaxIQDataCount > ICAP_EVENT_DATA_SAMPLE)
		u4MaxIQDataCount = ICAP_EVENT_DATA_SAMPLE;

	/* 2. update IQ Sample Count*/
	prRbistDump->u4IcapCnt = u4MaxIQDataCount;

	/* 3. Copy to buffer */
	for (i = 0; i < u4MaxIQDataCount; i++) {
		u4Value = prIQArray[u4DumpIndex++].u4IQArray[u4WFNum][u4IQType];
		kalMemCopy(pData + u4DataLen, (uint8_t *) &u4Value,
						sizeof(u4Value));
		u4DataLen += sizeof(u4Value);
	}

	/* 4. update response IQ data length */
	prRbistDump->u4IcapDataLen = u4DataLen;


	prICapInfo->au4ICapDumpIndex[u4WFNum][u4IQType] = u4DumpIndex;

	DBGLOG(RFTEST, INFO, "CurrICapDumpIndex[WF%d][%c]=%d,IQCnt=%d,len=%d\n",
						u4WFNum,
						(u4IQType == CAP_I_TYPE) ?
						'I' : 'Q',
						u4DumpIndex,
						u4MaxIQDataCount,
						u4DataLen);

	return rStatus;
}
#endif /* #if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1) */

uint32_t
rftestQueryATInfo(struct ADAPTER *prAdapter,
		  uint32_t u4FuncIndex, uint32_t u4FuncData,
		  void *pvQueryBuffer, uint32_t u4QueryBufferLen) {
	struct CMD_TEST_CTRL rCmdTestCtrl = {0};
	union EVENT_TEST_STATUS *prTestStatus;

	ASSERT(prAdapter);

	if (u4FuncIndex == RF_AT_FUNCID_FW_INFO) {
		/* driver implementation */
		prTestStatus = (union EVENT_TEST_STATUS *) pvQueryBuffer;

		prTestStatus->rATInfo.u4FuncData =
			(prAdapter->chip_info->em_interface_version << 16) |
			(prAdapter->rVerInfo.u2FwOwnVersion);

		DBGLOG(RFTEST, INFO, "RF_AT_FUNCID_FW_INFO=0x%x\n",
					prTestStatus->rATInfo.u4FuncData);

		u4QueryBufferLen = sizeof(union EVENT_TEST_STATUS);

		return WLAN_STATUS_SUCCESS;
	} else if (u4FuncIndex == RF_AT_FUNCID_DRV_INFO) {
		/* driver implementation */
		prTestStatus = (union EVENT_TEST_STATUS *) pvQueryBuffer;

		prTestStatus->rATInfo.u4FuncData = CFG_DRV_OWN_VERSION;
		u4QueryBufferLen = sizeof(union EVENT_TEST_STATUS);

		return WLAN_STATUS_SUCCESS;
	} else if (u4FuncIndex ==
		   RF_AT_FUNCID_QUERY_ICAP_DUMP_FILE) {
		/* driver implementation */
		prTestStatus = (union EVENT_TEST_STATUS *) pvQueryBuffer;

		prTestStatus->rATInfo.u4FuncData =
			prAdapter->rIcapInfo.u2DumpIndex;
		u4QueryBufferLen = sizeof(union EVENT_TEST_STATUS);

		return WLAN_STATUS_SUCCESS;
	}

	rCmdTestCtrl.ucAction = CMD_TEST_CTRL_ACT_GET_AT;
	rCmdTestCtrl.u.rRfATInfo.u4FuncIndex = u4FuncIndex;
	rCmdTestCtrl.u.rRfATInfo.u4FuncData = u4FuncData;

	DBGLOG(RFTEST, INFO, "rftestQueryATInfo\n");

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_TEST_CTRL,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryRfTestATInfo,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_TEST_CTRL),
				   (uint8_t *) &rCmdTestCtrl,
				   pvQueryBuffer, u4QueryBufferLen);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief command packet generation utility
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] ucCID              Command ID
 * \param[in] fgSetQuery         Set or Query
 * \param[in] fgNeedResp         Need for response
 * \param[in] pfCmdDoneHandler   Function pointer when command is done
 * \param[in] u4SetQueryInfoLen  The length of the set/query buffer
 * \param[in] pucInfoBuffer      Pointer to set/query buffer
 *
 *
 * \retval WLAN_STATUS_PENDING
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanSendSetQueryCmdAdv(struct ADAPTER *prAdapter,
		    uint8_t ucCID,
		    uint8_t ucExtCID,
		    uint8_t fgSetQuery,
		    uint8_t fgNeedResp,
		    uint8_t fgIsOid,
		    PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
		    PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
		    uint32_t u4SetQueryInfoLen,
		    uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
		    uint32_t u4SetQueryBufferLen,
		    enum EUNM_CMD_SEND_METHOD eMethod) {
	struct GLUE_INFO *prGlueInfo;
	struct CMD_INFO *prCmdInfo;
	uint8_t *pucCmfBuf;
	struct mt66xx_chip_info *prChipInfo;
	uint16_t cmd_size;
	uint32_t status = WLAN_STATUS_PENDING;

	if (kalIsResetting()) {
		DBGLOG(INIT, WARN, "Chip resetting, skip\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	cmd_size = prChipInfo->u2CmdTxHdrSize + u4SetQueryInfoLen;
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter, cmd_size);
	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T FAILED ID[0x%x]\n",
			ucCID);
		return WLAN_STATUS_RESOURCES;
	}

	/* Setup common CMD Info Packet */
	prCmdInfo->eCmdType = COMMAND_TYPE_NETWORK_IOCTL;
	prCmdInfo->u2InfoBufLen = cmd_size;
	prCmdInfo->pfCmdDoneHandler = pfCmdDoneHandler;
	prCmdInfo->pfCmdTimeoutHandler = pfCmdTimeoutHandler;
	prCmdInfo->fgIsOid = fgIsOid;
	prCmdInfo->ucCID = ucCID;
	prCmdInfo->fgSetQuery = fgSetQuery;
	prCmdInfo->fgNeedResp = fgNeedResp;
	prCmdInfo->u4SetInfoLen = u4SetQueryInfoLen;
	prCmdInfo->pvInformationBuffer = pvSetQueryBuffer;
	prCmdInfo->u4InformationBufferLength = u4SetQueryBufferLen;

	/* Setup WIFI_CMD_T (no payload) */
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		CMD_PACKET_TYPE_ID,
		&prCmdInfo->ucCmdSeqNum,
		prCmdInfo->fgSetQuery,
		&pucCmfBuf, FALSE, ucExtCID, S2D_INDEX_CMD_H2N);
	prCmdInfo->pucSetInfoBuffer = pucCmfBuf;
	if (u4SetQueryInfoLen > 0 && pucInfoBuffer != NULL)
		kalMemCopy(pucCmfBuf, pucInfoBuffer,
			   u4SetQueryInfoLen);

	switch (eMethod) {
	case CMD_SEND_METHOD_ENQUEUE:
		/* insert into prCmdQueue */
		kalEnqueueCommand(prGlueInfo,
				  (struct QUE_ENTRY *) prCmdInfo);

		/* wakeup txServiceThread later */
		GLUE_SET_EVENT(prGlueInfo);
		break;
	case CMD_SEND_METHOD_REQ_RESOURCE:
		status = wlanSendCommand(prAdapter, prCmdInfo);
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		break;
	case CMD_SEND_METHOD_TX:
		status = nicTxCmd(prAdapter, prCmdInfo, TC4_INDEX);
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		break;
	}

	return status;
}

static uint32_t wlanWaitInitEvt(struct ADAPTER *prAdapter,
	u_int8_t fgSkipCheckSeq,
	uint8_t ucSeq,
	uint8_t ucEvtId,
	uint8_t *pucEvtBuf,
	uint32_t u4EvtSz,
	uint32_t u4EvtWaitInterval,
	uint32_t u4EvtWaitTimeout)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct INIT_WIFI_EVENT *prInitEvtHeader = NULL;
	uint8_t *pucBuf = NULL;
	uint32_t u4PktLen = 0;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (!prAdapter) {
		DBGLOG(INIT, ERROR, "NULL prAdapter.\n");
		u4Status = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	prChipInfo = prAdapter->chip_info;

	pucBuf = kalMemAlloc(CFG_RX_MAX_PKT_SIZE, VIR_MEM_TYPE);
	if (!pucBuf) {
		DBGLOG(INIT, ERROR, "Alloc Buffer failed.\n");
		u4Status = WLAN_STATUS_RESOURCES;
		goto exit;
	}

	do {
		if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE ||
		    fgIsBusAccessFailed == TRUE) {
			DBGLOG(INIT, ERROR, "kalIsCardRemoved failed\n");
			u4Status = WLAN_STATUS_FAILURE;
			break;
		} else if (nicRxWaitResponseByWaitingInterval(prAdapter,
					     IMG_DL_STATUS_PORT_IDX,
					     pucBuf, CFG_RX_MAX_PKT_SIZE,
					     &u4PktLen,
					     u4EvtWaitInterval,
					     u4EvtWaitTimeout) !=
				WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "nicRxWaitResponse failed\n");
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}

		prInitEvtHeader = (struct INIT_WIFI_EVENT *)
			(pucBuf + prChipInfo->rxd_size);
		u4PktLen = prInitEvtHeader->u2RxByteCount - offsetof(
			struct INIT_WIFI_EVENT, aucBuffer);

		/* EID / SeqNum check */
		if (ucEvtId != prInitEvtHeader->ucEID) {
			DBGLOG(INIT, ERROR,
				"Expect eid: 0x%x, but 0x%x\n",
				ucEvtId,
				prInitEvtHeader->ucEID);
			u4Status = WLAN_STATUS_FAILURE;
		} else if (!fgSkipCheckSeq &&
			   ucSeq != prInitEvtHeader->ucSeqNum) {
			DBGLOG(INIT, ERROR,
				"Expect seq: 0x%x, but 0x%x\n",
				ucSeq,
				prInitEvtHeader->ucSeqNum);
			u4Status = WLAN_STATUS_FAILURE;
		} else if (u4EvtSz > 0 && u4PktLen > u4EvtSz) {
			DBGLOG(INIT, ERROR,
				"Buffer len(%d) NOT enough for pkt(%d)\n",
				u4EvtSz, u4PktLen);
			u4Status = WLAN_STATUS_FAILURE;
		}

		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG_MEM8(INIT, ERROR, pucBuf,
				prChipInfo->rxd_size +
				prInitEvtHeader->u2RxByteCount);
			break;
		}

		if (u4EvtSz && pucEvtBuf)
			kalMemCopy(pucEvtBuf,
				prInitEvtHeader->aucBuffer,
				u4PktLen);
	} while (FALSE);

exit:
	if (pucBuf)
		kalMemFree(pucBuf, VIR_MEM_TYPE, CFG_RX_MAX_PKT_SIZE);

	return u4Status;
}

uint32_t wlanSendInitSetQueryCmdImpl(struct ADAPTER *prAdapter,
	uint8_t ucCmdId,
	void *pucCmdBuf,
	uint32_t u4CmdSz,
	u_int8_t fgWaitResp,
	u_int8_t fgSkipCheckSeq,
	uint8_t ucEvtId,
	void *pucEvtBuf,
	uint32_t u4EvtSz,
	uint32_t u4EvtWaitInterval,
	uint32_t u4EvtWaitTimeout)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CMD_INFO *prCmdInfo = NULL;
	uint8_t *pucCmdInfoBuf = NULL;
	uint32_t u4CmdInfoSz;
	uint8_t ucPort, ucTc, ucCmdSeq = 0;
	uint32_t u4Status = WLAN_STATUS_FAILURE;

	if (!prAdapter)
		return WLAN_STATUS_NOT_SUPPORTED;

	prChipInfo = prAdapter->chip_info;

#define TEMP_LOG_TEMPLATE \
	"ucCmdId: 0x%x, pucCmdBuf: 0x%p, u4CmdSz: 0x%x, " \
	"fgWaitResp: %d, fgSkipCheckSeq: %d, " \
	"ucEvtId: 0x%x, pucEvtBuf: 0x%p, u4EvtSz: 0x%x, " \
	"u4EvtWaitInterval: %d, u4EvtWaitTimeout: %d\n"
	if (ucCmdId > 0 && pucCmdBuf && u4CmdSz) {
		DBGLOG(INIT, LOUD,
			TEMP_LOG_TEMPLATE,
			ucCmdId, pucCmdBuf, u4CmdSz,
			fgWaitResp, fgSkipCheckSeq,
			ucEvtId, pucEvtBuf, u4EvtSz,
			u4EvtWaitInterval, u4EvtWaitTimeout);
		DBGLOG_MEM32(INIT, LOUD, pucCmdBuf, u4CmdSz);
	}
#undef TEMP_LOG_TEMPLATE

	if (ucCmdId == 0) {
		u4CmdInfoSz = u4CmdSz;
		ucPort = prChipInfo->u2TxFwDlPort;
	} else {
		u4CmdInfoSz = u4CmdSz +
		    sizeof(struct INIT_HIF_TX_HEADER) +
		    sizeof(struct INIT_HIF_TX_HEADER_PENDING_FOR_HW_32BYTES);
		ucPort = prChipInfo->u2TxInitCmdPort;
	}
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter, u4CmdInfoSz);
	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		u4Status = WLAN_STATUS_RESOURCES;
		goto exit;
	}
	prCmdInfo->u2InfoBufLen = u4CmdInfoSz;

#if (CFG_USE_TC4_RESOURCE_FOR_INIT_CMD == 1)
	ucTc = TC4_INDEX;
#else
	ucTc = TC0_INDEX;
#endif

	prCmdInfo->ucCID = ucCmdId;
	NIC_FILL_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		INIT_CMD_PACKET_TYPE_ID,
		prCmdInfo->ucCID != 0 ? &ucCmdSeq : NULL,
		FALSE,
		(void **)&pucCmdInfoBuf,
		TRUE, 0, S2D_INDEX_CMD_H2N);
	if (u4CmdSz > 0 && pucCmdBuf != NULL)
		kalMemCopy(pucCmdInfoBuf, pucCmdBuf, u4CmdSz);

	while (TRUE) {
		uint32_t u4PageCnt = 0;

		/*
		 * No need to do resource control for download packets
		 */
		if (ucCmdId == 0)
			break;

		u4PageCnt = nicTxGetCmdPageCount(prAdapter, prCmdInfo);

		if (nicTxAcquireResource(prAdapter, ucTc, u4PageCnt, TRUE) ==
				WLAN_STATUS_SUCCESS)
			break;

		if (nicTxPollingResource(prAdapter, ucTc) !=
				WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR,
				"Fail to get TX resource\n");
			u4Status = WLAN_STATUS_RESOURCES;
			goto exit;
		}
	};

	if (nicTxInitCmd(prAdapter, prCmdInfo, ucPort) !=
			WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "Fail to send cmd.\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	if (fgWaitResp)
		u4Status = wlanWaitInitEvt(prAdapter,
			fgSkipCheckSeq,
			ucCmdSeq,
			ucEvtId, pucEvtBuf, u4EvtSz,
			u4EvtWaitInterval, u4EvtWaitTimeout);
	else
		u4Status = WLAN_STATUS_SUCCESS;

exit:
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

	return u4Status;
}

uint32_t wlanSendInitSetQueryCmd(struct ADAPTER *prAdapter,
	uint8_t ucCmdId,
	void *pucCmdBuf,
	uint32_t u4CmdSz,
	u_int8_t fgWaitResp,
	u_int8_t fgSkipCheckSeq,
	uint8_t ucEvtId,
	void *pucEvtBuf,
	uint32_t u4EvtSz)
{
	return wlanSendInitSetQueryCmdImpl(prAdapter,
		ucCmdId, pucCmdBuf, u4CmdSz,
		fgWaitResp, fgSkipCheckSeq,
		ucEvtId, pucEvtBuf, u4EvtSz,
		CFG_DEFAULT_SLEEP_WAITING_INTERVAL,
		CFG_DEFAULT_RX_RESPONSE_TIMEOUT);
}

#if CFG_SUPPORT_WAPI
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called by WAPI ui to set wapi mode, which is needed to
 *        info the the driver to operation at WAPI mode while driver initialize.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set
 * \param[in] u4SetBufferLen The length of the set buffer
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *   bytes read from the set buffer. If the call failed due to invalid length of
 *   the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA If new setting value is wrong.
 * \retval WLAN_STATUS_INVALID_LENGTH
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetWapiMode(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen) {
	DBGLOG(REQ, LOUD, "\r\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	/* Todo:: For support WAPI and Wi-Fi at same driver, use the set wapi
	 *        assoc ie at the check point
	 *        The Adapter Connection setting fgUseWapi will cleat whil oid
	 *        set mode (infra),
	 *        And set fgUseWapi True while set wapi assoc ie
	 *        policay selection, add key all depend on this flag,
	 *        The fgUseWapi may remove later
	 */
	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (*(uint32_t *) pvSetBuffer)
		prAdapter->fgUseWapi = TRUE;
	else
		prAdapter->fgUseWapi = FALSE;

#if 0
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter,
					  (CMD_HDR_SIZE + 4));

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}
	/* increase command sequence number */
	ucCmdSeqNum = nicIncreaseCmdSeqNum(prAdapter);

	/* compose CMD_BUILD_CONNECTION cmd pkt */
	prCmdInfo->eCmdType = COMMAND_TYPE_NETWORK_IOCTL;
	prCmdInfo->ucBssIndex = AIS_DEFAULT_INDEX;
	prCmdInfo->u2InfoBufLen = CMD_HDR_SIZE + 4;
	prCmdInfo->pfCmdDoneHandler = nicCmdEventSetCommon;
	prCmdInfo->pfCmdTimeoutHandler = NULL;
	prCmdInfo->fgIsOid = TRUE;
	prCmdInfo->ucCID = CMD_ID_WAPI_MODE;
	prCmdInfo->fgSetQuery = TRUE;
	prCmdInfo->fgNeedResp = FALSE;
	prCmdInfo->fgDriverDomainMCR = FALSE;
	prCmdInfo->ucCmdSeqNum = ucCmdSeqNum;
	prCmdInfo->u4SetInfoLen = u4SetBufferLen;
	prCmdInfo->pvInformationBuffer = pvSetBuffer;
	prCmdInfo->u4InformationBufferLength = u4SetBufferLen;

	/* Setup WIFI_CMD_T */
	prWifiCmd = (struct WIFI_CMD *) (prCmdInfo->pucInfoBuffer);
	prWifiCmd->u2TxByteCount = prCmdInfo->u2InfoBufLen;
	prWifiCmd->u2PQ_ID = CMD_PQ_ID;
	prWifiCmd->ucPktTypeID = CMD_PACKET_TYPE_ID;
	prWifiCmd->ucCID = prCmdInfo->ucCID;
	prWifiCmd->ucSetQuery = prCmdInfo->fgSetQuery;
	prWifiCmd->ucSeqNum = prCmdInfo->ucCmdSeqNum;

	cp = (uint8_t *) (prWifiCmd->aucBuffer);

	kalMemCopy(cp, (uint8_t *) pvSetBuffer, 4);

	/* insert into prCmdQueue */
	kalEnqueueCommand(prGlueInfo,
			  (struct QUE_ENTRY *) prCmdInfo);

	/* wakeup txServiceThread later */
	GLUE_SET_EVENT(prGlueInfo);

	return WLAN_STATUS_PENDING;
#else
	return WLAN_STATUS_SUCCESS;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called by WAPI to set the assoc info, which is needed
 *        to add to Association request frame while join WAPI AP.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set
 * \param[in] u4SetBufferLen The length of the set buffer
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *   bytes read from the set buffer. If the call failed due to invalid length of
 *   the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA If new setting value is wrong.
 * \retval WLAN_STATUS_INVALID_LENGTH
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetWapiAssocInfo(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen) {
	struct WAPI_INFO_ELEM *prWapiInfo = NULL;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	prConnSettings =
		aisGetConnSettings(prAdapter, ucBssIndex);

	prConnSettings->fgWapiMode = FALSE;

	if (u4SetBufferLen < 20 /* From EID to Group cipher */)
		return WLAN_STATUS_INVALID_LENGTH;

	if (!wextSrchDesiredWAPIIE((uint8_t *) pvSetBuffer,
				   u4SetBufferLen, (uint8_t **) &prWapiInfo))
		return WLAN_STATUS_INVALID_LENGTH;

	if (!prWapiInfo || prWapiInfo->ucLength < 18)
		return WLAN_STATUS_INVALID_LENGTH;

	/* Skip Version check */

	/*Cipher suite count check, only one of each for now*/
	if (prWapiInfo->u2AKMSuiteCount > 1 ||
	    prWapiInfo->u2PairSuiteCount > 1)
		return WLAN_STATUS_INVALID_LENGTH;

	DBGLOG(SEC, TRACE,
	       "WAPI: Assoc Info auth mgt suite [%d]: %02x-%02x-%02x-%02x\n",
	       prWapiInfo->u2AKMSuiteCount,
	       (uint8_t) (prWapiInfo->u4AKMSuite & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4AKMSuite >> 8) & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4AKMSuite >> 16) & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4AKMSuite >> 24) & 0x000000FF));

	if (prWapiInfo->u4AKMSuite != WAPI_AKM_SUITE_802_1X
	    && prWapiInfo->u4AKMSuite != WAPI_AKM_SUITE_PSK)
		return WLAN_STATUS_NOT_SUPPORTED;

	DBGLOG(SEC, TRACE,
	       "WAPI: Assoc Info pairwise cipher suite [%d]: %02x-%02x-%02x-%02x\n",
	       prWapiInfo->u2PairSuiteCount,
	       (uint8_t) (prWapiInfo->u4PairSuite & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4PairSuite >> 8) & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4PairSuite >> 16) & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4PairSuite >> 24) & 0x000000FF));

	if (prWapiInfo->u4PairSuite != WAPI_CIPHER_SUITE_WPI)
		return WLAN_STATUS_NOT_SUPPORTED;

	DBGLOG(SEC, TRACE,
	       "WAPI: Assoc Info group cipher suite : %02x-%02x-%02x-%02x\n",
	       (uint8_t) (prWapiInfo->u4GroupSuite & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4GroupSuite >> 8) & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4GroupSuite >> 16) & 0x000000FF),
	       (uint8_t) ((prWapiInfo->u4GroupSuite >> 24) & 0x000000FF));

	if (prWapiInfo->u4GroupSuite != WAPI_CIPHER_SUITE_WPI)
		return WLAN_STATUS_NOT_SUPPORTED;

	prConnSettings->u4WapiSelectedAKMSuite
		= prWapiInfo->u4AKMSuite;
	prConnSettings->u4WapiSelectedPairwiseCipher
		= prWapiInfo->u4PairSuite;
	prConnSettings->u4WapiSelectedGroupCipher
		= prWapiInfo->u4GroupSuite;

	prConnSettings->fgWapiMode = TRUE;

	return WLAN_STATUS_SUCCESS;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the wpi key to the driver.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 *
 * \note The setting buffer P_PARAM_WPI_KEY, which is set by NDIS, is unpacked.
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetWapiKey(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen) {
	struct PARAM_WPI_KEY *prNewKey;
	struct CMD_802_11_KEY rCmdKey;
	uint8_t *pc;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set add key! (Adapter not ready). ACPI=D%d, Radio=%d\r\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}
	if (u4SetBufferLen < sizeof(struct PARAM_WPI_KEY)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prNewKey = (struct PARAM_WPI_KEY *) pvSetBuffer;

	ucBssIndex = prNewKey->ucBssIdx;
	DBGLOG(REQ, LOUD, "ucBssIndex %d\n", ucBssIndex);

	DBGLOG_MEM8(REQ, TRACE, (uint8_t *) pvSetBuffer, 560);
	pc = (uint8_t *) pvSetBuffer;

	*pu4SetInfoLen = u4SetBufferLen;

	/* Todo:: WAPI AP mode !!!!! */
	prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	prNewKey->ucKeyID = prNewKey->ucKeyID & BIT(0);

	/* Dump P_PARAM_WPI_KEY_T content. */
	DBGLOG(REQ, TRACE,
	       "Set: Dump P_PARAM_WPI_KEY_T content\r\n");
	DBGLOG(REQ, TRACE, "TYPE      : %d\r\n",
	       prNewKey->eKeyType);
	DBGLOG(REQ, TRACE, "Direction : %d\r\n",
	       prNewKey->eDirection);
	DBGLOG(REQ, TRACE, "KeyID     : %d\r\n", prNewKey->ucKeyID);
	DBGLOG(REQ, TRACE, "AddressIndex:\r\n");
	DBGLOG_MEM8(REQ, TRACE, prNewKey->aucAddrIndex, 12);
	prNewKey->u4LenWPIEK = 16;

	DBGLOG_MEM8(REQ, TRACE, (uint8_t *) prNewKey->aucWPIEK,
		    (uint8_t) prNewKey->u4LenWPIEK);
	prNewKey->u4LenWPICK = 16;

	DBGLOG(REQ, TRACE, "CK Key(%d):\r\n",
	       (uint8_t) prNewKey->u4LenWPICK);
	DBGLOG_MEM8(REQ, TRACE, (uint8_t *) prNewKey->aucWPICK,
		    (uint8_t) prNewKey->u4LenWPICK);
	DBGLOG(REQ, TRACE, "PN:\r\n");
	if (prNewKey->eKeyType == 0) {
		prNewKey->aucPN[0] = 0x5c;
		prNewKey->aucPN[1] = 0x36;
		prNewKey->aucPN[2] = 0x5c;
		prNewKey->aucPN[3] = 0x36;
		prNewKey->aucPN[4] = 0x5c;
		prNewKey->aucPN[5] = 0x36;
		prNewKey->aucPN[6] = 0x5c;
		prNewKey->aucPN[7] = 0x36;
		prNewKey->aucPN[8] = 0x5c;
		prNewKey->aucPN[9] = 0x36;
		prNewKey->aucPN[10] = 0x5c;
		prNewKey->aucPN[11] = 0x36;
		prNewKey->aucPN[12] = 0x5c;
		prNewKey->aucPN[13] = 0x36;
		prNewKey->aucPN[14] = 0x5c;
		prNewKey->aucPN[15] = 0x36;
	}

	DBGLOG_MEM8(REQ, TRACE, (uint8_t *) prNewKey->aucPN, 16);

	kalMemZero(&rCmdKey, sizeof(struct CMD_802_11_KEY));

	rCmdKey.ucAddRemove = 1;	/* Add */

	if (prNewKey->eKeyType == ENUM_WPI_PAIRWISE_KEY) {
		rCmdKey.ucTxKey = 1;
		rCmdKey.ucKeyType = 1;
	}
	kalMemCopy(rCmdKey.aucPeerAddr,
		   (uint8_t *) prNewKey->aucAddrIndex, MAC_ADDR_LEN);
	if ((rCmdKey.aucPeerAddr[0] & rCmdKey.aucPeerAddr[1] &
	     rCmdKey.aucPeerAddr[2] &
	     rCmdKey.aucPeerAddr[3] & rCmdKey.aucPeerAddr[4] &
	     rCmdKey.aucPeerAddr[5]) == 0xFF) {
		prStaRec = cnmGetStaRecByAddress(prAdapter,
				prBssInfo->ucBssIndex, prBssInfo->aucBSSID);
		if (prStaRec == NULL) {
			DBGLOG(REQ, WARN, "Can't find station.\n");
			return WLAN_STATUS_FAILURE;
		}
		/* AIS RSN Group key, addr is BC addr */
		kalMemCopy(rCmdKey.aucPeerAddr, prStaRec->aucMacAddr,
			   MAC_ADDR_LEN);
	} else {
		prStaRec = cnmGetStaRecByAddress(prAdapter,
				prBssInfo->ucBssIndex, rCmdKey.aucPeerAddr);
	}

	rCmdKey.ucBssIdx = prBssInfo->ucBssIndex; /* AIS */

	rCmdKey.ucKeyId = prNewKey->ucKeyID;

	rCmdKey.ucKeyLen = 32;

	rCmdKey.ucAlgorithmId = CIPHER_SUITE_WPI;

	kalMemCopy(rCmdKey.aucKeyMaterial,
		   (uint8_t *) prNewKey->aucWPIEK, 16);

	kalMemCopy(rCmdKey.aucKeyMaterial + 16,
		   (uint8_t *) prNewKey->aucWPICK, 16);

	kalMemCopy(rCmdKey.aucKeyRsc, (uint8_t *) prNewKey->aucPN,
		   16);

	if (rCmdKey.ucTxKey) {
		if (prStaRec) {
			if (rCmdKey.ucKeyType) {	/* AIS RSN STA */
				rCmdKey.ucWlanIndex = prStaRec->ucWlanIndex;
				prStaRec->fgTransmitKeyExist =
					TRUE;	/* wait for CMD Done ? */
			} else {
				DBGLOG(REQ, WARN, "Key type is invalid.\n");
				return WLAN_STATUS_INVALID_DATA;
			}
		}
#if 0
		if (fgAddTxBcKey || !prStaRec) {

			if ((prCmdKey->aucPeerAddr[0]
			    & prCmdKey->aucPeerAddr[1]
			    & prCmdKey->aucPeerAddr[2]
			    & prCmdKey->aucPeerAddr[3]
			    & prCmdKey->aucPeerAddr[4]
			    & prCmdKey->aucPeerAddr[5]) == 0xFF) {
				prCmdKey->ucWlanIndex =
						255; /* AIS WEP Tx key */
			} else {	/* Exist this case ? */
				ASSERT(FALSE);
				/* prCmdKey->ucWlanIndex = */
				/* secPrivacySeekForBcEntry(prAdapter, */
				/* prBssInfo->ucBssIndex, */
				/* NETWORK_TYPE_AIS, */
				/* prCmdKey->aucPeerAddr, */
				/* prCmdKey->ucAlgorithmId, */
				/* prCmdKey->ucKeyId, */
			}

			prBssInfo->fgBcDefaultKeyExist = TRUE;
			prBssInfo->ucBMCWlanIndex =
				prCmdKey->ucWlanIndex;	/* Saved for AIS WEP */
			prBssInfo->ucTxBcDefaultIdx = prCmdKey->ucKeyId;
		}
#endif
	} else {
		/* Including IBSS RSN Rx BC key ? */
		if ((rCmdKey.aucPeerAddr[0] & rCmdKey.aucPeerAddr[1] &
		     rCmdKey.aucPeerAddr[2] & rCmdKey.aucPeerAddr[3] &
		     rCmdKey.aucPeerAddr[4] & rCmdKey.aucPeerAddr[5]) ==
		    0xFF) {
			rCmdKey.ucWlanIndex =
				WTBL_RESERVED_ENTRY; /* AIS WEP, should not have
						      * this case!!
						      */
		} else {
			if (prStaRec) {	/* AIS RSN Group key but addr is BSSID
					 */
				/* ASSERT(prStaRec->ucBMCWlanIndex < WTBL_SIZE)
				 */
				rCmdKey.ucWlanIndex =
					secPrivacySeekForBcEntry(prAdapter,
							prStaRec->ucBssIndex,
							prStaRec->aucMacAddr,
							prStaRec->ucIndex,
							rCmdKey.ucAlgorithmId,
							rCmdKey.ucKeyId);
				prStaRec->ucWlanIndex = rCmdKey.ucWlanIndex;
			} else {	/* Exist this case ? */
				DBGLOG(REQ, WARN, "Can't find station.\n");
				return WLAN_STATUS_FAILURE;

				/* prCmdKey->ucWlanIndex = */
				/* secPrivacySeekForBcEntry(prAdapter, */
				/* prBssInfo->ucBssIndex, */
				/* NETWORK_TYPE_AIS, */
				/* prCmdKey->aucPeerAddr, */
				/* prCmdKey->ucAlgorithmId, */
				/* prCmdKey->ucKeyId, */
			}
		}
	}

	return wlanSendSetQueryCmd(
			  prAdapter,
			  CMD_ID_ADD_REMOVE_KEY,
			  TRUE,
			  FALSE,
			  TRUE,
			  nicCmdEventSetCommon,
			  nicOidCmdTimeoutCommon,
			  sizeof(struct CMD_802_11_KEY),
			  (uint8_t *)&rCmdKey,
			  pvSetBuffer,
			  u4SetBufferLen);
}				/* wlanoidSetAddKey */
#endif

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
uint32_t
wlanoidSetFilsConnInfo(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	struct PARAM_FILS *prFils = NULL;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	if (u4SetBufferLen < sizeof(*prFils))
		return WLAN_STATUS_INVALID_LENGTH;

	/* always clean old erp key */
	kalMemZero(&prConnSettings->rErpKey, sizeof(prConnSettings->rErpKey));

	prFils = (struct PARAM_FILS *)pvSetBuffer;

	/* no need to update without erp */
	if (prFils->u2ErpUsernameLen == 0)
		return WLAN_STATUS_SUCCESS;

	if (checkAddOverflow(prFils->u2ErpUsernameLen + 1,
		prFils->pucErpRealmLen)) {
		DBGLOG(FILS, ERROR, "nai sum overflow, name=%d realm=%d\n",
			prFils->u2ErpUsernameLen, prFils->pucErpRealmLen);
		return WLAN_STATUS_INVALID_LENGTH;
	} else if (prFils->u2ErpUsernameLen + 1 + prFils->pucErpRealmLen >
		FILS_MAX_KEY_NAME_NAI_LEN) {
		DBGLOG(FILS, ERROR, "nai too long, name=%d realm=%d\n",
			prFils->u2ErpUsernameLen, prFils->pucErpRealmLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (prFils->u2ErpRrkLen > ERP_MAX_KEY_LEN) {
		DBGLOG(FILS, ERROR, "rrk too long %d\n",
			prFils->u2ErpRrkLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	filsUpdateFilsInfo(prAdapter, &prConnSettings->rErpKey, prFils);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#if CFG_ENABLE_WAKEUP_ON_LAN
uint32_t
wlanoidSetAddWakeupPattern(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen) {
	struct PARAM_PM_PACKET_PATTERN *prPacketPattern;

	DBGLOG(REQ, LOUD, "\r\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_PM_PACKET_PATTERN);

	if (u4SetBufferLen < sizeof(struct PARAM_PM_PACKET_PATTERN))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prPacketPattern = (struct PARAM_PM_PACKET_PATTERN *)
			  pvSetBuffer;

	/* FIXME: Send the struct to firmware */

	return WLAN_STATUS_FAILURE;
}

uint32_t
wlanoidSetRemoveWakeupPattern(struct ADAPTER *prAdapter,
			      void *pvSetBuffer, uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen) {
	struct PARAM_PM_PACKET_PATTERN *prPacketPattern;

	DBGLOG(REQ, LOUD, "\r\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_PM_PACKET_PATTERN);

	if (u4SetBufferLen < sizeof(struct PARAM_PM_PACKET_PATTERN))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prPacketPattern = (struct PARAM_PM_PACKET_PATTERN *)
			  pvSetBuffer;

	/* FIXME: Send the struct to firmware */

	return WLAN_STATUS_FAILURE;
}

uint32_t
wlanoidQueryEnableWakeup(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen) {
	uint32_t *pu4WakeupEventEnable;

	DBGLOG(REQ, LOUD, "\r\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(uint32_t);

	if (u4QueryBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	pu4WakeupEventEnable = (uint32_t *) pvQueryBuffer;

	*pu4WakeupEventEnable = prAdapter->u4WakeupEventEnable;

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidSetEnableWakeup(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen) {
	uint32_t *pu4WakeupEventEnable;

	DBGLOG(REQ, LOUD, "\r\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(uint32_t);

	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	pu4WakeupEventEnable = (uint32_t *) pvSetBuffer;
	prAdapter->u4WakeupEventEnable = *pu4WakeupEventEnable;

	/* FIXME: Send Command Event for setting
	 *        wakeup-pattern / Magic Packet to firmware
	 */

	return WLAN_STATUS_FAILURE;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to configure PS related settings for WMM-PS
 *        test.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetWiFiWmmPsTest(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_WMM_PS_TEST_STRUCT *prWmmPsTestInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct CMD_SET_WMM_PS_TEST_STRUCT rSetWmmPsTestParam;
	uint16_t u2CmdBufLen;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_WMM_PS_TEST_STRUCT);

	if (u4SetBufferLen < sizeof(struct PARAM_CUSTOM_WMM_PS_TEST_STRUCT)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prWmmPsTestInfo = (struct PARAM_CUSTOM_WMM_PS_TEST_STRUCT *)
			  pvSetBuffer;

	rSetWmmPsTestParam.ucBssIndex =
		prWmmPsTestInfo->ucBssIdx;
	rSetWmmPsTestParam.bmfgApsdEnAc =
		prWmmPsTestInfo->bmfgApsdEnAc;
	rSetWmmPsTestParam.ucIsEnterPsAtOnce =
		prWmmPsTestInfo->ucIsEnterPsAtOnce;
	rSetWmmPsTestParam.ucIsDisableUcTrigger =
		prWmmPsTestInfo->ucIsDisableUcTrigger;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					  rSetWmmPsTestParam.ucBssIndex);

	if (prBssInfo == NULL) {
		DBGLOG(REQ, WARN, "BSS Info not exist !!\n");
		return WLAN_STATUS_FAILURE;
	}

	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;
	prPmProfSetupInfo->ucBmpDeliveryAC =
		(rSetWmmPsTestParam.bmfgApsdEnAc >> 4) & BITS(0, 3);
	prPmProfSetupInfo->ucBmpTriggerAC =
		rSetWmmPsTestParam.bmfgApsdEnAc & BITS(0, 3);

	u2CmdBufLen = sizeof(struct CMD_SET_WMM_PS_TEST_STRUCT);

#if 0
	/* it will apply the disable trig or not immediately */
	if (prPmInfo->ucWmmPsDisableUcPoll
	    && prPmInfo->ucWmmPsConnWithTrig)
		NIC_PM_WMM_PS_DISABLE_UC_TRIG(prAdapter, TRUE);
	else
		NIC_PM_WMM_PS_DISABLE_UC_TRIG(prAdapter, FALSE);
#endif

	DBGLOG(INIT, INFO, "BSS[%d] DeAC=%x TrigAC=%x Once=%d DisUcTrig=%d\n",
		rSetWmmPsTestParam.ucBssIndex,
		prPmProfSetupInfo->ucBmpDeliveryAC,
		prPmProfSetupInfo->ucBmpTriggerAC,
		rSetWmmPsTestParam.ucIsEnterPsAtOnce,
		rSetWmmPsTestParam.ucIsDisableUcTrigger);

	rStatus = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_SET_WMM_PS_TEST_PARMS,
				      TRUE, FALSE, TRUE,
				      nicCmdEventSetCommon,/* TODO? */
				      nicCmdTimeoutCommon, u2CmdBufLen,
				      (uint8_t *) &rSetWmmPsTestParam, NULL, 0);

	return rStatus;
}				/* wlanoidSetWiFiWmmPsTest */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to configure enable/disable TX A-MPDU feature.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetTxAmpdu(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen) {
	struct CMD_TX_AMPDU rTxAmpdu;
	struct CMD_TX_AMPDU *p;
	uint16_t u2CmdBufLen;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct CMD_TX_AMPDU);

	if (u4SetBufferLen < sizeof(struct CMD_TX_AMPDU)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	p = (struct CMD_TX_AMPDU *) pvSetBuffer;

	rTxAmpdu.fgEnable = p->fgEnable;
	rTxAmpdu.fgApply = p->fgApply;

	u2CmdBufLen = sizeof(struct CMD_TX_AMPDU);

	/* return value of wlanSendSetQueryCmd is WLAN_STATUS_PENDING */
	wlanSendSetQueryCmd(prAdapter, CMD_ID_TX_AMPDU,
				      TRUE, FALSE, FALSE, NULL, NULL,
				      u2CmdBufLen,
				      (uint8_t *) &rTxAmpdu, NULL, 0);

	return WLAN_STATUS_SUCCESS;
}				/* wlanoidSetTxAmpdu */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to configure TX A-MPDU size.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer Pointer to the buffer that holds the data to be set.
 * \param[in] u4QueryBufferLen The length of the set buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetTxAggLimit(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_NOT_SUPPORTED;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint8_t ucBssIndex;
	struct UNI_CMD_BA_OFFLOAD *uni_cmd;
	struct UNI_CMD_TX_AGG_LIMIT *prLimit;
	uint32_t cmd_len;
	struct PARAM_SET_TX_AGG_LIMIT_INFO *prParam;
#endif

	if (!prAdapter || !pvQueryBuffer || !pu4QueryInfoLen)
		return rStatus;

	*pu4QueryInfoLen = sizeof(struct PARAM_SET_TX_AGG_LIMIT_INFO);

	if (u4QueryBufferLen < sizeof(struct PARAM_SET_TX_AGG_LIMIT_INFO)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	if (unlikely(ucBssIndex >= MAX_BSSID_NUM))
		return WLAN_STATUS_INVALID_DATA;

	prParam = (struct PARAM_SET_TX_AGG_LIMIT_INFO *)pvQueryBuffer;

	/* prepare staStats driver stuff */
	cmd_len = sizeof(struct UNI_CMD_BA_OFFLOAD) +
		sizeof(struct UNI_CMD_TX_AGG_LIMIT);

	uni_cmd = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BA_OFFLOAD ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* prepare unified cmd tags */
	prLimit = (struct UNI_CMD_TX_AGG_LIMIT *)uni_cmd->aucTlvBuffer;
	prLimit->u2Tag = UNI_CMD_BA_OFFLOAD_TAG_TX_AGG_LIMIT;
	prLimit->u2Length = sizeof(*prLimit);
	prLimit->ucBssIdx = ucBssIndex;
	prLimit->u2TxAmpduNum = prParam->u2TxAmpduNum;
	prLimit->ucSet = prParam->ucSet;

	/* return value of wlanSendSetQueryCmd is WLAN_STATUS_PENDING */
	rStatus = wlanSendSetQueryUniCmd(prAdapter,
			UNI_CMD_ID_BA_OFFLOAD,
			FALSE,
			TRUE,
			TRUE,
			nicUniSolicitEventBaOffload,
			nicUniCmdTimeoutCommon,
			cmd_len,
			(void *)uni_cmd,
			pvQueryBuffer, u4QueryBufferLen);
	DBGLOG(REQ, TRACE, "rStatus=%u, pvQueryBuffer=%p",
			rStatus, pvQueryBuffer);
	cnmMemFree(prAdapter, uni_cmd);

#endif
	return rStatus;
}				/* wlanoidSetTxAggLimit */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to configure max TX A-MSDU number.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer Pointer to the buffer that holds the data to be set.
 * \param[in] u4QueryBufferLen The length of the set buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetTxAmsduNumLimit(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_NOT_SUPPORTED;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint8_t ucBssIndex;
	struct UNI_CMD_BA_OFFLOAD *uni_cmd;
	struct UNI_CMD_TX_AMSDU_NUM_LIMIT *prLimit;
	uint32_t cmd_len;
	struct PARAM_SET_TX_AMSDU_NUM_LIMIT_INFO *prParam;
#endif

	if (!prAdapter || !pvQueryBuffer || !pu4QueryInfoLen)
		return rStatus;

	*pu4QueryInfoLen = sizeof(struct PARAM_SET_TX_AMSDU_NUM_LIMIT_INFO);

	if (u4QueryBufferLen <
		sizeof(struct PARAM_SET_TX_AMSDU_NUM_LIMIT_INFO)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	if (unlikely(ucBssIndex >= MAX_BSSID_NUM))
		return WLAN_STATUS_INVALID_DATA;

	prParam = (struct PARAM_SET_TX_AMSDU_NUM_LIMIT_INFO *)pvQueryBuffer;

	/* prepare staStats driver stuff */
	cmd_len = sizeof(struct UNI_CMD_BA_OFFLOAD) +
		sizeof(struct UNI_CMD_TX_AMSDU_NUM_LIMIT);

	uni_cmd = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BA_OFFLOAD ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* prepare unified cmd tags */
	prLimit = (struct UNI_CMD_TX_AMSDU_NUM_LIMIT *)uni_cmd->aucTlvBuffer;
	prLimit->u2Tag = UNI_CMD_BA_OFFLOAD_TAG_TX_AMSDU_NUM_LIMIT;
	prLimit->u2Length = sizeof(*prLimit);
	prLimit->ucBssIdx = ucBssIndex;
	prLimit->ucTxAmsduNum = prParam->ucTxAmsduNum;
	prLimit->ucSet = prParam->ucSet;

	/* return value of wlanSendSetQueryCmd is WLAN_STATUS_PENDING */
	rStatus = wlanSendSetQueryUniCmd(prAdapter,
			UNI_CMD_ID_BA_OFFLOAD,
			FALSE,
			TRUE,
			TRUE,
			nicUniSolicitEventBaOffload,
			nicUniCmdTimeoutCommon,
			cmd_len,
			(void *)uni_cmd,
			pvQueryBuffer, u4QueryBufferLen);
	DBGLOG(REQ, TRACE, "rStatus=%u, pvQueryBuffer=%p",
			rStatus, pvQueryBuffer);
	cnmMemFree(prAdapter, uni_cmd);

#endif
	return rStatus;
}				/* wlanoidSetTxAmsduNumLimit */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to configure reject/accept ADDBA Request.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetAddbaReject(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	struct CMD_ADDBA_REJECT rAddbaReject;
	struct CMD_ADDBA_REJECT *p;
	uint16_t u2CmdBufLen;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct CMD_ADDBA_REJECT);

	if (u4SetBufferLen < sizeof(struct CMD_ADDBA_REJECT)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	p = (struct CMD_ADDBA_REJECT *) pvSetBuffer;

	rAddbaReject.fgEnable = p->fgEnable;
	rAddbaReject.fgApply = p->fgApply;

	u2CmdBufLen = sizeof(struct CMD_ADDBA_REJECT);

	/* return value of wlanSendSetQueryCmd is WLAN_STATUS_PENDING */
	wlanSendSetQueryCmd(prAdapter, CMD_ID_ADDBA_REJECT,
				      TRUE, FALSE, FALSE, NULL, NULL,
				      u2CmdBufLen,
				      (uint8_t *) &rAddbaReject, NULL, 0);

	return WLAN_STATUS_SUCCESS;
}				/* wlanoidSetAddbaReject */

#if CFG_SLT_SUPPORT

uint32_t
wlanoidQuerySLTStatus(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen) {
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_MTK_SLT_TEST_STRUCT *prMtkSltInfo =
		(struct PARAM_MTK_SLT_TEST_STRUCT *) NULL;
	struct SLT_INFO *prSltInfo = (struct SLT_INFO *) NULL;

	DBGLOG(REQ, LOUD, "\r\n");

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(struct PARAM_MTK_SLT_TEST_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_MTK_SLT_TEST_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvQueryBuffer);

	prMtkSltInfo = (struct PARAM_MTK_SLT_TEST_STRUCT *)
		       pvQueryBuffer;

	prSltInfo = &(prAdapter->rWifiVar.rSltInfo);

	switch (prMtkSltInfo->rSltFuncIdx) {
	case ENUM_MTK_SLT_FUNC_LP_SET: {
		struct PARAM_MTK_SLT_LP_TEST_STRUCT *prLpSetting =
			(struct PARAM_MTK_SLT_LP_TEST_STRUCT *) NULL;

		ASSERT(prMtkSltInfo->u4FuncInfoLen == sizeof(
			       struct PARAM_MTK_SLT_LP_TEST_STRUCT));

		prLpSetting = (struct PARAM_MTK_SLT_LP_TEST_STRUCT *)
			      &prMtkSltInfo->unFuncInfoContent;

		prLpSetting->u4BcnRcvNum = prSltInfo->u4BeaconReceiveCnt;
	}
	break;
	default:
		/* TBD... */
		break;
	}

	return rWlanStatus;
}				/* wlanoidQuerySLTStatus */

uint32_t
wlanoidUpdateSLTMode(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_MTK_SLT_TEST_STRUCT *prMtkSltInfo =
		(struct PARAM_MTK_SLT_TEST_STRUCT *) NULL;
	struct SLT_INFO *prSltInfo = (struct SLT_INFO *) NULL;
	struct BSS_DESC *prBssDesc = (struct BSS_DESC *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	/* 1. Action: Update or Initial Set
	 * 2. Role.
	 * 3. Target MAC address.
	 * 4. RF BW & Rate Settings
	 */

	DBGLOG(REQ, LOUD, "\r\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_MTK_SLT_TEST_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_MTK_SLT_TEST_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prMtkSltInfo = (struct PARAM_MTK_SLT_TEST_STRUCT *)
		       pvSetBuffer;

	prSltInfo = &(prAdapter->rWifiVar.rSltInfo);
	prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	switch (prMtkSltInfo->rSltFuncIdx) {
	case ENUM_MTK_SLT_FUNC_INITIAL: {	/* Initialize */
		struct PARAM_MTK_SLT_INITIAL_STRUCT *prMtkSltInit =
			(struct PARAM_MTK_SLT_INITIAL_STRUCT *) NULL;

		ASSERT(prMtkSltInfo->u4FuncInfoLen == sizeof(
			       struct PARAM_MTK_SLT_INITIAL_STRUCT));

		prMtkSltInit = (struct PARAM_MTK_SLT_INITIAL_STRUCT *)
			       &prMtkSltInfo->unFuncInfoContent;

		if (prSltInfo->prPseudoStaRec != NULL) {
			/* The driver has been initialized. */
			prSltInfo->prPseudoStaRec = NULL;
		}

		prSltInfo->prPseudoBssDesc = scanSearchExistingBssDesc(
						prAdapter, BSS_TYPE_IBSS,
						prMtkSltInit->aucTargetMacAddr,
						prMtkSltInit->aucTargetMacAddr);

		prSltInfo->u2SiteID = prMtkSltInit->u2SiteID;

		/* Bandwidth 2.4G: Channel 1~14
		 * Bandwidth 5G: *36, 40, 44, 48, 52, 56, 60, 64,
		 *       *100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140,
		 *       149, 153, *157, 161,
		 *       184, 188, 192, 196, 200, 204, 208, 212, *216
		 */
		prSltInfo->ucChannel2G4 = 1 + (prSltInfo->u2SiteID % 4) * 5;

		switch (prSltInfo->ucChannel2G4) {
		case 1:
			prSltInfo->ucChannel5G = 36;
			break;
		case 6:
			prSltInfo->ucChannel5G = 52;
			break;
		case 11:
			prSltInfo->ucChannel5G = 104;
			break;
		case 16:
			prSltInfo->ucChannel2G4 = 14;
			prSltInfo->ucChannel5G = 161;
			break;
		default:
			ASSERT(FALSE);
		}

		if (prSltInfo->prPseudoBssDesc == NULL) {
			do {
				prSltInfo->prPseudoBssDesc =
						scanAllocateBssDesc(prAdapter);

				if (prSltInfo->prPseudoBssDesc == NULL) {
					rWlanStatus = WLAN_STATUS_FAILURE;
					break;
				}
				prBssDesc = prSltInfo->prPseudoBssDesc;

			} while (FALSE);
		} else {
			prBssDesc = prSltInfo->prPseudoBssDesc;
		}

		if (prBssDesc) {
			prBssDesc->eBSSType = BSS_TYPE_IBSS;

			COPY_MAC_ADDR(prBssDesc->aucSrcAddr,
				      prMtkSltInit->aucTargetMacAddr);
			COPY_MAC_ADDR(prBssDesc->aucBSSID,
				      prBssInfo->aucOwnMacAddr);

			prBssDesc->u2BeaconInterval = 100;
			prBssDesc->u2ATIMWindow = 0;
			prBssDesc->ucDTIMPeriod = 1;

			prBssDesc->u2IELength = 0;

			prBssDesc->fgIsERPPresent = TRUE;
			prBssDesc->fgIsHTPresent = TRUE;

			prBssDesc->u2OperationalRateSet = BIT(RATE_36M_INDEX);
			prBssDesc->u2BSSBasicRateSet = BIT(RATE_36M_INDEX);
			prBssDesc->fgIsUnknownBssBasicRate = FALSE;

			prBssDesc->fgIsLargerTSF = TRUE;

			prBssDesc->eBand = BAND_2G4;

			prBssDesc->ucChannelNum = prSltInfo->ucChannel2G4;

			prBssDesc->ucPhyTypeSet = PHY_TYPE_SET_802_11ABGN;

			GET_CURRENT_SYSTIME(&prBssDesc->rUpdateTime);
		}
	}
	break;
	case ENUM_MTK_SLT_FUNC_RATE_SET:	/* Update RF Settings. */
		if (prSltInfo->prPseudoStaRec == NULL) {
			rWlanStatus = WLAN_STATUS_FAILURE;
		} else {
			struct PARAM_MTK_SLT_TR_TEST_STRUCT *prTRSetting =
				(struct PARAM_MTK_SLT_TR_TEST_STRUCT *) NULL;

			ASSERT(prMtkSltInfo->u4FuncInfoLen == sizeof(
				       struct PARAM_MTK_SLT_TR_TEST_STRUCT));

			prStaRec = prSltInfo->prPseudoStaRec;
			prTRSetting = (struct PARAM_MTK_SLT_TR_TEST_STRUCT *)
				      &prMtkSltInfo->unFuncInfoContent;

			if (prTRSetting->rNetworkType ==
			    PARAM_NETWORK_TYPE_OFDM5) {
				prBssInfo->eBand = BAND_5G;
				prBssInfo->ucPrimaryChannel =
							prSltInfo->ucChannel5G;
			}
			if (prTRSetting->rNetworkType ==
			    PARAM_NETWORK_TYPE_OFDM24) {
				prBssInfo->eBand = BAND_2G4;
				prBssInfo->ucPrimaryChannel =
							prSltInfo->ucChannel2G4;
			}

			if ((prTRSetting->u4FixedRate & FIXED_BW_DL40) != 0) {
				/* RF 40 */
				/* It would controls RFBW capability in WTBL. */
				prStaRec->u2HtCapInfo |=
						HT_CAP_INFO_SUP_CHNL_WIDTH;
				/* This controls RF BW, RF BW would be 40
				 * only if
				 * 1. PHY_TYPE_BIT_HT is TRUE.
				 * 2. SCO is SCA/SCB.
				 */
				prStaRec->ucDesiredPhyTypeSet = PHY_TYPE_BIT_HT;

				/* U20/L20 Control. */
				switch (prTRSetting->u4FixedRate & 0xC000) {
				case FIXED_EXT_CHNL_U20:
					prBssInfo->eBssSCO =
							CHNL_EXT_SCB; /* +2 */
					if (prTRSetting->rNetworkType ==
					    PARAM_NETWORK_TYPE_OFDM5) {
						prBssInfo->ucPrimaryChannel
									+= 2;
					} else {
						/* For channel 1, testing L20 at
						 * channel 8. AOSP
						 */
						SetTestChannel(
						&prBssInfo->ucPrimaryChannel);
					}
					break;
				case FIXED_EXT_CHNL_L20:
				default:	/* 40M */
					prBssInfo->eBssSCO =
							CHNL_EXT_SCA; /* -2 */
					if (prTRSetting->rNetworkType ==
					    PARAM_NETWORK_TYPE_OFDM5) {
						prBssInfo->ucPrimaryChannel
									-= 2;
					} else {
						/* For channel 11 / 14. testing
						 * U20 at channel 3. AOSP
						 */
						SetTestChannel(
						&prBssInfo->ucPrimaryChannel);
					}
					break;
				}
			} else {
				/* RF 20 */
				prStaRec->u2HtCapInfo &=
						~HT_CAP_INFO_SUP_CHNL_WIDTH;
				prBssInfo->eBssSCO = CHNL_EXT_SCN;
			}

			prBssInfo->fgErpProtectMode = FALSE;
			prBssInfo->eHtProtectMode = HT_PROTECT_MODE_NONE;
			prBssInfo->eGfOperationMode = GF_MODE_NORMAL;

			nicUpdateBss(prAdapter, prBssInfo->ucNetTypeIndex);

			prStaRec->u2HtCapInfo &= ~(HT_CAP_INFO_SHORT_GI_20M |
						   HT_CAP_INFO_SHORT_GI_40M);

			switch (prTRSetting->u4FixedRate & 0xFF) {
			case RATE_OFDM_54M:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_54M_SW_INDEX);
				break;
			case RATE_OFDM_48M:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_48M_SW_INDEX);
				break;
			case RATE_OFDM_36M:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_36M_SW_INDEX);
				break;
			case RATE_OFDM_24M:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_24M_SW_INDEX);
				break;
			case RATE_OFDM_6M:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_6M_SW_INDEX);
				break;
			case RATE_CCK_11M_LONG:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_11M_SW_INDEX);
				break;
			case RATE_CCK_1M_LONG:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_1M_SW_INDEX);
				break;
			case RATE_GF_MCS_0:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_HT_PHY_SW_INDEX);
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_HT_GF;
				break;
			case RATE_MM_MCS_7:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_HT_PHY_SW_INDEX);
				prStaRec->u2HtCapInfo &= ~HT_CAP_INFO_HT_GF;
#if 0				/* Only for Current Measurement Mode. */
				prStaRec->u2HtCapInfo |=
						(HT_CAP_INFO_SHORT_GI_20M |
						HT_CAP_INFO_SHORT_GI_40M);
#endif
				break;
			case RATE_GF_MCS_7:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_HT_PHY_SW_INDEX);
				prStaRec->u2HtCapInfo |= HT_CAP_INFO_HT_GF;
				break;
			default:
				prStaRec->u2DesiredNonHTRateSet =
						BIT(RATE_36M_SW_INDEX);
				break;
			}

			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

		}
		break;
	case ENUM_MTK_SLT_FUNC_LP_SET: {	/* Reset LP Test Result. */
		struct PARAM_MTK_SLT_LP_TEST_STRUCT *prLpSetting =
			(struct PARAM_MTK_SLT_LP_TEST_STRUCT *) NULL;

		ASSERT(prMtkSltInfo->u4FuncInfoLen == sizeof(
			       struct PARAM_MTK_SLT_LP_TEST_STRUCT));

		prLpSetting = (struct PARAM_MTK_SLT_LP_TEST_STRUCT *)
			      &prMtkSltInfo->unFuncInfoContent;

		if (prSltInfo->prPseudoBssDesc == NULL) {
			/* Please initial SLT Mode first. */
			break;
		}
		prBssDesc = prSltInfo->prPseudoBssDesc;

		switch (prLpSetting->rLpTestMode) {
		case ENUM_MTK_LP_TEST_NORMAL:
			/* In normal mode, we would use target MAC address to be
			 * the BSSID.
			 */
			COPY_MAC_ADDR(prBssDesc->aucBSSID,
				      prBssInfo->aucOwnMacAddr);
			prSltInfo->fgIsDUT = FALSE;
			break;
		case ENUM_MTK_LP_TEST_GOLDEN_SAMPLE:
			/* 1. Lower AIFS of BCN queue.
			 * 2. Fixed Random Number tobe 0.
			 */
			prSltInfo->fgIsDUT = FALSE;
			/* In LP test mode, we would use MAC address of Golden
			 * Sample to be the BSSID.
			 */
			COPY_MAC_ADDR(prBssDesc->aucBSSID,
				      prBssInfo->aucOwnMacAddr);
			break;
		case ENUM_MTK_LP_TEST_DUT:
			/* 1. Enter Sleep Mode.
			 * 2. Fix random number a large value & enlarge AIFN of
			 *    BCN queue.
			 */
			COPY_MAC_ADDR(prBssDesc->aucBSSID,
				      prBssDesc->aucSrcAddr);
			prSltInfo->u4BeaconReceiveCnt = 0;
			prSltInfo->fgIsDUT = TRUE;
			break;
		}

	}

	break;
	default:
		break;
	}

	return WLAN_STATUS_FAILURE;

	return rWlanStatus;
}				/* wlanoidUpdateSLTMode */
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query NVRAM value.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryNvramRead(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen) {
	struct PARAM_CUSTOM_EEPROM_RW_STRUCT *rNvRwInfo;
	uint16_t u2Data = 0;
	u_int8_t fgStatus;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Ofs = 0;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_EEPROM_RW_STRUCT);

	if (u4QueryBufferLen < sizeof(struct
				      PARAM_CUSTOM_EEPROM_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	rNvRwInfo = (struct PARAM_CUSTOM_EEPROM_RW_STRUCT *)
			pvQueryBuffer;

	if (rNvRwInfo->ucMethod ==
			PARAM_EEPROM_READ_METHOD_READ) {
		u4Ofs = rNvRwInfo->info.rEeprom.ucEepromIndex << 1;
		fgStatus = kalCfgDataRead16(prAdapter->prGlueInfo,
					    u4Ofs, /* change to byte offset */
					    &u2Data);
		if (fgStatus) {
			rNvRwInfo->info.rEeprom.u2EepromData = u2Data;
			DBGLOG(REQ, INFO,
			       "NVRAM Read: index=%#X, data=%#02X\r\n",
			       rNvRwInfo->info.rEeprom.ucEepromIndex,
			       u2Data);
		} else {
			DBGLOG(REQ, ERROR, "NVRAM Read Failed: index=%#x.\r\n",
			       rNvRwInfo->info.rEeprom.ucEepromIndex);
			rStatus = WLAN_STATUS_FAILURE;
		}
	} else if (rNvRwInfo->ucMethod ==
		   PARAM_EEPROM_READ_METHOD_GETSIZE) {
		rNvRwInfo->info.rEeprom.u2EepromData =
			MAX_CFG_FILE_WIFI_REC_SIZE;
		DBGLOG(REQ, INFO, "EEPROM size =%d\r\n",
		       rNvRwInfo->info.rEeprom.u2EepromData);
	} else if (rNvRwInfo->ucMethod ==
		   PARAM_EEPROM_READ_NVRAM) {
		u4Ofs = rNvRwInfo->info.rNvram.u2NvIndex;
		fgStatus = kalCfgDataRead16(prAdapter->prGlueInfo,
					    u4Ofs,
					    &u2Data);

		if (fgStatus) {
			rNvRwInfo->info.rNvram.u2NvData = u2Data & 0x00FF;
			DBGLOG(REQ, INFO,
				   "index=%#X, data=%#02X\r\n",
				   rNvRwInfo->info.rNvram.u2NvIndex,
				   rNvRwInfo->info.rNvram.u2NvData);
		} else {
			DBGLOG(REQ, ERROR, "NVRAM Read Failed: index=%#x.\r\n",
				   rNvRwInfo->info.rNvram.u2NvIndex);
			rStatus = WLAN_STATUS_FAILURE;
		}

	}

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_CUSTOM_EEPROM_RW_STRUCT);

	return rStatus;
}				/* wlanoidQueryNvramRead */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to write NVRAM value.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetNvramWrite(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_EEPROM_RW_STRUCT *rNvRwInfo;
	u_int8_t fgStatus = FALSE;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_EEPROM_RW_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_EEPROM_RW_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	rNvRwInfo = (struct PARAM_CUSTOM_EEPROM_RW_STRUCT *)
			pvSetBuffer;

	if (rNvRwInfo->ucMethod == PARAM_EEPROM_WRITE_NVRAM) {
		fgStatus = kalCfgDataWrite8(prAdapter->prGlueInfo,
			rNvRwInfo->info.rNvram.u2NvIndex,
			rNvRwInfo->info.rNvram.u2NvData & 0x00FF);

		DBGLOG(REQ, INFO, "status(%d),index=%#X, data=%#02X\n",
			fgStatus,
			rNvRwInfo->info.rNvram.u2NvIndex,
			rNvRwInfo->info.rNvram.u2NvData);

		/*update nvram to firmware*/
		if (fgStatus == TRUE)
			wlanLoadManufactureData(prAdapter,
				kalGetConfiguration(prAdapter->prGlueInfo));
	} else {
		fgStatus = kalCfgDataWrite16(prAdapter->prGlueInfo,
				     rNvRwInfo->info.rEeprom.ucEepromIndex <<
				     1, /* change to byte offset */
				     rNvRwInfo->info.rEeprom.u2EepromData);
	}

	if (fgStatus == FALSE) {
		DBGLOG(REQ, ERROR, "NVRAM Write Failed.\r\n");
		rStatus = WLAN_STATUS_FAILURE;
	}

	return rStatus;
}				/* wlanoidSetNvramWrite */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get the config data source type.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryCfgSrcType(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);

	*pu4QueryInfoLen = sizeof(enum ENUM_CFG_SRC_TYPE);

	if (u4QueryBufferLen < sizeof(enum ENUM_CFG_SRC_TYPE)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (kalIsConfigurationExist(prAdapter->prGlueInfo) == TRUE)
		*(enum ENUM_CFG_SRC_TYPE *) pvQueryBuffer =
			CFG_SRC_TYPE_NVRAM;
	else
		*(enum ENUM_CFG_SRC_TYPE *) pvQueryBuffer =
			CFG_SRC_TYPE_EEPROM;

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get the config data source type.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryEepromType(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen) {
	ASSERT(prAdapter);

	*pu4QueryInfoLen = sizeof(enum ENUM_EEPROM_TYPE *);

#if CFG_SUPPORT_NIC_CAPABILITY
	if (u4QueryBufferLen < sizeof(enum ENUM_EEPROM_TYPE)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (prAdapter->fgIsEepromUsed == TRUE)
		*(enum ENUM_EEPROM_TYPE *) pvQueryBuffer =
			EEPROM_TYPE_PRESENT;
	else
		*(enum ENUM_EEPROM_TYPE *) pvQueryBuffer = EEPROM_TYPE_NO;
#else
	*(enum ENUM_EEPROM_TYPE *) pvQueryBuffer = EEPROM_TYPE_NO;
#endif

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get the config data source type.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetCountryCode(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen) {
	uint8_t *pucCountry;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (regd_is_single_sku_en()) {
		struct COUNTRY_CODE_SETTING *prCountrySetting = NULL;

		if (sizeof(struct COUNTRY_CODE_SETTING) != u4SetBufferLen) {
			DBGLOG(REQ, ERROR, "Invalid length %u != %u\n",
				sizeof(struct COUNTRY_CODE_SETTING),
				u4SetBufferLen);
			return WLAN_STATUS_INVALID_LENGTH;
		}

		prCountrySetting = (struct COUNTRY_CODE_SETTING *) pvSetBuffer;
		rlmDomainOidSetCountry(prAdapter,
					prCountrySetting->aucCountryCode,
					prCountrySetting->ucCountryLength,
					prCountrySetting->fgNeedHoldRtnlLock);

		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_SUCCESS;
	}

	ASSERT(u4SetBufferLen == 2);
	if (u4SetBufferLen != 2) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = 2;

	pucCountry = (uint8_t *) pvSetBuffer;

	prAdapter->rWifiVar.u2CountryCode =
		(((uint16_t) pucCountry[0]) << 8) | ((uint16_t) pucCountry[1]);

	/* Force to re-search country code in regulatory domains */
	prAdapter->prDomainInfo = NULL;
	rlmDomainSendCmd(prAdapter, TRUE);

	/* Update supported channel list in channel table based on current
	 * country domain
	 */
	wlanUpdateChannelTable(prAdapter->prGlueInfo);

#if CFG_SUPPORT_SAP_DFS_CHANNEL
	if (aisGetConnectedBssInfo(prAdapter)) {
		struct BSS_INFO *prAisBssInfo =
				aisGetConnectedBssInfo(prAdapter);

		if (prAisBssInfo == NULL) {
			return WLAN_STATUS_FAILURE;
		}

		/* restore DFS channels table */
		wlanDfsChannelsReqAdd(prAdapter,
			DFS_CHANNEL_CTRL_SOURCE_SAP,
			prAisBssInfo->ucPrimaryChannel, /* primary channel */
			0, /* bandwidth */
			0, /* sco */
			0, /* center frequency */
			prAisBssInfo->eBand /* eBand */);
	}
#endif

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidSetScanMacOui(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct PARAM_BSS_MAC_OUI *prParamMacOui;
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(prAdapter->prGlueInfo);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(struct PARAM_BSS_MAC_OUI)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prParamMacOui = (struct PARAM_BSS_MAC_OUI *)pvSetBuffer;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prParamMacOui->ucBssIndex);
	if (!prBssInfo) {
		log_dbg(REQ, ERROR, "Invalid bss info (ind=%u)\n",
			prParamMacOui->ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	kalMemCopy(prBssInfo->ucScanOui, prParamMacOui->ucMacOui, MAC_OUI_LEN);
	prBssInfo->fgIsScanOuiSet = TRUE;
	*pu4SetInfoLen = MAC_OUI_LEN;

	return WLAN_STATUS_SUCCESS;
}

#if 0
uint32_t
wlanoidSetNoaParam(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_NOA_PARAM_STRUCT *prNoaParam;
	struct CMD_CUSTOM_NOA_PARAM_STRUCT rCmdNoaParam;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_NOA_PARAM_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_NOA_PARAM_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prNoaParam = (struct PARAM_CUSTOM_NOA_PARAM_STRUCT *)
		     pvSetBuffer;

	kalMemZero(&rCmdNoaParam,
		   sizeof(struct CMD_CUSTOM_NOA_PARAM_STRUCT));
	rCmdNoaParam.u4NoaDurationMs = prNoaParam->u4NoaDurationMs;
	rCmdNoaParam.u4NoaIntervalMs = prNoaParam->u4NoaIntervalMs;
	rCmdNoaParam.u4NoaCount = prNoaParam->u4NoaCount;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_NOA_PARAM,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_CUSTOM_NOA_PARAM_STRUCT),
				   (uint8_t *) &rCmdNoaParam, pvSetBuffer,
				   u4SetBufferLen);
}

uint32_t
wlanoidSetOppPsParam(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT *prOppPsParam;
	struct CMD_CUSTOM_OPPPS_PARAM_STRUCT rCmdOppPsParam;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_OPPPS_PARAM_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_OPPPS_PARAM_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prOppPsParam = (struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT *)
		       pvSetBuffer;

	kalMemZero(&rCmdOppPsParam,
		   sizeof(struct CMD_CUSTOM_OPPPS_PARAM_STRUCT));
	rCmdOppPsParam.u4CTwindowMs = prOppPsParam->u4CTwindowMs;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_OPPPS_PARAM,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_CUSTOM_OPPPS_PARAM_STRUCT),
				   (uint8_t *) &rCmdOppPsParam, pvSetBuffer,
				   u4SetBufferLen);
}

uint32_t
wlanoidSetUApsdParam(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT *prUapsdParam;
	struct CMD_CUSTOM_UAPSD_PARAM_STRUCT rCmdUapsdParam;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	struct BSS_INFO *prBssInfo;

	DBGLOG(INIT, LOUD, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_UAPSD_PARAM_STRUCT);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_UAPSD_PARAM_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prBssInfo = &
		    (prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_P2P_INDEX]);
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;

	prUapsdParam = (struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT *)
		       pvSetBuffer;

	kalMemZero(&rCmdUapsdParam,
		   sizeof(struct CMD_CUSTOM_OPPPS_PARAM_STRUCT));
	rCmdUapsdParam.fgEnAPSD = prUapsdParam->fgEnAPSD;
	prAdapter->rWifiVar.fgSupportUAPSD = prUapsdParam->fgEnAPSD;

	rCmdUapsdParam.fgEnAPSD_AcBe = prUapsdParam->fgEnAPSD_AcBe;
	rCmdUapsdParam.fgEnAPSD_AcBk = prUapsdParam->fgEnAPSD_AcBk;
	rCmdUapsdParam.fgEnAPSD_AcVo = prUapsdParam->fgEnAPSD_AcVo;
	rCmdUapsdParam.fgEnAPSD_AcVi = prUapsdParam->fgEnAPSD_AcVi;
	prPmProfSetupInfo->ucBmpDeliveryAC =
		((prUapsdParam->fgEnAPSD_AcBe << 0) |
		 (prUapsdParam->fgEnAPSD_AcBk << 1) |
		 (prUapsdParam->fgEnAPSD_AcVi << 2) |
		 (prUapsdParam->fgEnAPSD_AcVo << 3));
	prPmProfSetupInfo->ucBmpTriggerAC =
		((prUapsdParam->fgEnAPSD_AcBe << 0) |
		 (prUapsdParam->fgEnAPSD_AcBk << 1) |
		 (prUapsdParam->fgEnAPSD_AcVi << 2) |
		 (prUapsdParam->fgEnAPSD_AcVo << 3));

	rCmdUapsdParam.ucMaxSpLen = prUapsdParam->ucMaxSpLen;
	prPmProfSetupInfo->ucUapsdSp = prUapsdParam->ucMaxSpLen;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_UAPSD_PARAM,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_CUSTOM_OPPPS_PARAM_STRUCT),
				   (uint8_t *)&rCmdUapsdParam, pvSetBuffer,
				   u4SetBufferLen);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set BT profile or BT information and the
 *        driver will set the built-in PTA configuration into chip.
 *
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetBT(struct ADAPTER *prAdapter,
	     void *pvSetBuffer, uint32_t u4SetBufferLen,
	     uint32_t *pu4SetInfoLen) {

	struct PTA_IPC *prPtaIpc;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PTA_IPC);
	if (u4SetBufferLen != sizeof(struct PTA_IPC)) {
		/* WARNLOG(("Invalid length %ld\n", u4SetBufferLen)); */
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail to set BT profile because of ACPI_D3\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	ASSERT(pvSetBuffer);
	prPtaIpc = (struct PTA_IPC *) pvSetBuffer;

#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS && CFG_SUPPORT_BCM_BWCS_DEBUG
	DBGLOG(INIT, INFO,
	       "BCM BWCS CMD: BWCS CMD = %02x%02x%02x%02x\n",
	       prPtaIpc->u.aucBTPParams[0], prPtaIpc->u.aucBTPParams[1],
	       prPtaIpc->u.aucBTPParams[2], prPtaIpc->u.aucBTPParams[3]);

	DBGLOG(INIT, INFO,
	       "BCM BWCS CMD: aucBTPParams[0]=%02x, aucBTPParams[1]=%02x, aucBTPParams[2]=%02x, aucBTPParams[3]=%02x.\n",
	       prPtaIpc->u.aucBTPParams[0], prPtaIpc->u.aucBTPParams[1],
	       prPtaIpc->u.aucBTPParams[2], prPtaIpc->u.aucBTPParams[3]);

#endif

	wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_BWCS, TRUE, FALSE, FALSE,
			    NULL, NULL, sizeof(struct PTA_IPC),
			    (uint8_t *) prPtaIpc, NULL, 0);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query current BT profile and BTCR values
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] pvQueryBuffer      Pointer to the buffer that holds the result of
 *                               the query.
 * \param[in] u4QueryBufferLen   The length of the query buffer.
 * \param[out] pu4QueryInfoLen   If the call is successful, returns the number
 *				 of bytes written into the query buffer. If the
 *				 call failed due to invalid length of the query
 *				 buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryBT(struct ADAPTER *prAdapter,
	       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
	       uint32_t *pu4QueryInfoLen) {
	/* P_PARAM_PTA_IPC_T prPtaIpc; */
	/* UINT_32 u4QueryBuffLen; */

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(struct PTA_IPC);

	/* Check for query buffer length */
	if (u4QueryBufferLen != sizeof(struct PTA_IPC)) {
		DBGLOG(REQ, WARN, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	ASSERT(pvQueryBuffer);
	/* prPtaIpc = (P_PTA_IPC_T)pvQueryBuffer; */
	/* prPtaIpc->ucCmd = BT_CMD_PROFILE; */
	/* prPtaIpc->ucLen = sizeof(prPtaIpc->u); */
	/* nicPtaGetProfile(prAdapter, (PUINT_8)&prPtaIpc->u, &u4QueryBuffLen);
	 */

	return WLAN_STATUS_SUCCESS;
}

#if 0
uint32_t
wlanoidQueryBtSingleAntenna(struct ADAPTER *prAdapter,
			    void *pvQueryBuffer,
			    uint32_t u4QueryBufferLen,
			    uint32_t *pu4QueryInfoLen) {
	P_PTA_INFO_T prPtaInfo;
	uint32_t *pu4SingleAntenna;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(uint32_t);

	/* Check for query buffer length */
	if (u4QueryBufferLen != sizeof(uint32_t)) {
		DBGLOG(REQ, WARN, "Invalid length %lu\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	ASSERT(pvQueryBuffer);

	prPtaInfo = &prAdapter->rPtaInfo;
	pu4SingleAntenna = (uint32_t *) pvQueryBuffer;

	if (prPtaInfo->fgSingleAntenna) {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME
		 *        "Q Single Ant = 1\r\n"));
		 */
		*pu4SingleAntenna = 1;
	} else {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME
		 *        "Q Single Ant = 0\r\n"));
		 */
		*pu4SingleAntenna = 0;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidSetBtSingleAntenna(struct ADAPTER *prAdapter,
			  void *pvSetBuffer, uint32_t u4SetBufferLen,
			  uint32_t *pu4SetInfoLen) {

	uint32_t *pu4SingleAntenna;
	uint32_t u4SingleAntenna;
	P_PTA_INFO_T prPtaInfo;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	prPtaInfo = &prAdapter->rPtaInfo;

	*pu4SetInfoLen = sizeof(uint32_t);
	if (u4SetBufferLen != sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	if (IS_ARB_IN_RFTEST_STATE(prAdapter))
		return WLAN_STATUS_SUCCESS;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail to set antenna because of ACPI_D3\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	ASSERT(pvSetBuffer);
	pu4SingleAntenna = (uint32_t *) pvSetBuffer;
	u4SingleAntenna = *pu4SingleAntenna;

	if (u4SingleAntenna == 0) {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME
		 * "Set Single Ant = 0\r\n"));
		 */
		prPtaInfo->fgSingleAntenna = FALSE;
	} else {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME
		 *        "Set Single Ant = 1\r\n"));
		 */
		prPtaInfo->fgSingleAntenna = TRUE;
	}
	ptaFsmRunEventSetConfig(prAdapter, &prPtaInfo->rPtaParam);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
uint32_t
wlanoidQueryPta(struct ADAPTER *prAdapter,
		void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen) {
	P_PTA_INFO_T prPtaInfo;
	uint32_t *pu4Pta;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	*pu4QueryInfoLen = sizeof(uint32_t);

	/* Check for query buffer length */
	if (u4QueryBufferLen != sizeof(uint32_t)) {
		DBGLOG(REQ, WARN, "Invalid length %lu\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	ASSERT(pvQueryBuffer);

	prPtaInfo = &prAdapter->rPtaInfo;
	pu4Pta = (uint32_t *) pvQueryBuffer;

	if (prPtaInfo->fgEnabled) {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME"PTA = 1\r\n")); */
		*pu4Pta = 1;
	} else {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME"PTA = 0\r\n")); */
		*pu4Pta = 0;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidSetPta(struct ADAPTER *prAdapter,
	      void *pvSetBuffer, uint32_t u4SetBufferLen,
	      uint32_t *pu4SetInfoLen) {
	uint32_t *pu4PtaCtrl;
	uint32_t u4PtaCtrl;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(uint32_t);
	if (u4SetBufferLen != sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	if (IS_ARB_IN_RFTEST_STATE(prAdapter))
		return WLAN_STATUS_SUCCESS;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail to set BT setting because of ACPI_D3\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	ASSERT(pvSetBuffer);
	pu4PtaCtrl = (uint32_t *) pvSetBuffer;
	u4PtaCtrl = *pu4PtaCtrl;

	if (u4PtaCtrl == 0) {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME"Set Pta= 0\r\n"));
		 */
		nicPtaSetFunc(prAdapter, FALSE);
	} else {
		/* DBGLOG(INIT, INFO, (KERN_WARNING DRV_NAME"Set Pta= 1\r\n"));
		 */
		nicPtaSetFunc(prAdapter, TRUE);
	}

	return WLAN_STATUS_SUCCESS;
}
#endif

#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set Tx power profile.
 *
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetTxPower(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen) {
	struct SET_TXPWR_CTRL *pTxPwr;
	struct SET_TXPWR_CTRL *prCmd;
	uint32_t i;
	uint32_t rStatus;

	DBGLOG(REQ, LOUD, "\r\n");

	if (u4SetBufferLen < sizeof(struct SET_TXPWR_CTRL)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	pTxPwr = (struct SET_TXPWR_CTRL *) pvSetBuffer;
	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
			    sizeof(struct SET_TXPWR_CTRL));

	if (!prCmd) {
		DBGLOG(REQ, ERROR, "prCmd not available\n");
		return WLAN_STATUS_FAILURE;
	}
	kalMemZero(prCmd, sizeof(struct SET_TXPWR_CTRL));
	prCmd->c2GLegacyStaPwrOffset =
		pTxPwr->c2GLegacyStaPwrOffset;
	prCmd->c2GHotspotPwrOffset = pTxPwr->c2GHotspotPwrOffset;
	prCmd->c2GP2pPwrOffset = pTxPwr->c2GP2pPwrOffset;
	prCmd->c2GBowPwrOffset = pTxPwr->c2GBowPwrOffset;
	prCmd->c5GLegacyStaPwrOffset =
		pTxPwr->c5GLegacyStaPwrOffset;
	prCmd->c5GHotspotPwrOffset = pTxPwr->c5GHotspotPwrOffset;
	prCmd->c5GP2pPwrOffset = pTxPwr->c5GP2pPwrOffset;
	prCmd->c5GBowPwrOffset = pTxPwr->c5GBowPwrOffset;
	prCmd->ucConcurrencePolicy = pTxPwr->ucConcurrencePolicy;
	for (i = 0; i < 14; i++)
		prCmd->acTxPwrLimit2G[i] = pTxPwr->acTxPwrLimit2G[i];

	for (i = 0; i < 4; i++)
		prCmd->acTxPwrLimit5G[i] = pTxPwr->acTxPwrLimit5G[i];

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

#if 0
	DBGLOG(INIT, INFO, "c2GLegacyStaPwrOffset=%d\n",
	       pTxPwr->c2GLegacyStaPwrOffset);
	DBGLOG(INIT, INFO, "c2GHotspotPwrOffset=%d\n",
	       pTxPwr->c2GHotspotPwrOffset);
	DBGLOG(INIT, INFO, "c2GP2pPwrOffset=%d\n",
	       pTxPwr->c2GP2pPwrOffset);
	DBGLOG(INIT, INFO, "c2GBowPwrOffset=%d\n",
	       pTxPwr->c2GBowPwrOffset);
	DBGLOG(INIT, INFO, "c5GLegacyStaPwrOffset=%d\n",
	       pTxPwr->c5GLegacyStaPwrOffset);
	DBGLOG(INIT, INFO, "c5GHotspotPwrOffset=%d\n",
	       pTxPwr->c5GHotspotPwrOffset);
	DBGLOG(INIT, INFO, "c5GP2pPwrOffset=%d\n",
	       pTxPwr->c5GP2pPwrOffset);
	DBGLOG(INIT, INFO, "c5GBowPwrOffset=%d\n",
	       pTxPwr->c5GBowPwrOffset);
	DBGLOG(INIT, INFO, "ucConcurrencePolicy=%d\n",
	       pTxPwr->ucConcurrencePolicy);

	for (i = 0; i < 14; i++)
		DBGLOG(INIT, INFO, "acTxPwrLimit2G[%d]=%d\n", i,
		       pTxPwr->acTxPwrLimit2G[i]);

	for (i = 0; i < 4; i++)
		DBGLOG(INIT, INFO, "acTxPwrLimit5G[%d]=%d\n", i,
		       pTxPwr->acTxPwrLimit5G[i]);
#endif

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_TXPWR_CTRL,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			TRUE,	/* fgIsOid */
			nicCmdEventSetCommon, nicOidCmdTimeoutCommon,
			sizeof(struct SET_TXPWR_CTRL),	/* u4SetQueryInfoLen */
			(uint8_t *) prCmd,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
			);

	/* ASSERT(rStatus == WLAN_STATUS_PENDING); */
	cnmMemFree(prAdapter, prCmd);

	return rStatus;

}

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set the p2p mode.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetP2pMode(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen) {
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT rSetP2P;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT);
	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT)) {
		DBGLOG(REQ, WARN, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	memcpy(&rSetP2P, pvSetBuffer, *pu4SetInfoLen);

	DBGLOG(P2P, TRACE, "Set P2P enable[%d] mode[%d]\n",
	       rSetP2P.u4Enable, rSetP2P.u4Mode);

	/*
	 *    enable = 1, mode = 0  => init P2P network
	 *    enable = 1, mode = 1  => init Soft AP network
	 *    enable = 0            => uninit P2P/AP network
	 *    enable = 1, mode = 2  => init dual Soft AP network
	 *    enable = 1, mode = 3  => init AP+P2P network
	 */


	DBGLOG(P2P, TRACE, "P2P Compile as (%d)p2p-like interface\n",
	       KAL_P2P_NUM);

	if (rSetP2P.u4Mode >= RUNNING_P2P_MODE_NUM) {
		DBGLOG(P2P, ERROR, "P2P interface mode(%d) is wrong\n",
		       rSetP2P.u4Mode);
		ASSERT(0);
	}

	if (rSetP2P.u4Enable) {
		p2pSetMode(rSetP2P.u4Mode);

		if (p2pLaunch(prAdapter->prGlueInfo)) {
			/* ToDo:: ASSERT */
			ASSERT(prAdapter->fgIsP2PRegistered);
			if (prAdapter->rWifiVar.ucApUapsd
			    && (rSetP2P.u4Mode != RUNNING_P2P_MODE)) {
				DBGLOG(OID, INFO,
				       "wlanoidSetP2pMode Default enable ApUapsd\n");
				setApUapsdEnable(prAdapter, TRUE);
			}
			prAdapter->u4P2pMode = rSetP2P.u4Mode;
		} else {
			DBGLOG(P2P, ERROR, "P2P Launch Failed\n");
			status = WLAN_STATUS_FAILURE;
		}

	} else {
		if (prAdapter->fgIsP2PRegistered)
			p2pRemove(prAdapter->prGlueInfo,
				rSetP2P.fgIsRtnlLockAcquired);

	}

#if 0
	prP2pNetdevRegMsg = (struct MSG_P2P_NETDEV_REGISTER *)
				cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				(sizeof(struct MSG_P2P_NETDEV_REGISTER)));

	if (prP2pNetdevRegMsg == NULL) {
		ASSERT(FALSE);
		status = WLAN_STATUS_RESOURCES;
		return status;
	}

	prP2pNetdevRegMsg->rMsgHdr.eMsgId =
		MID_MNY_P2P_NET_DEV_REGISTER;
	prP2pNetdevRegMsg->fgIsEnable = (prSetP2P->u4Enable == 1) ?
					TRUE : FALSE;
	prP2pNetdevRegMsg->ucMode = (uint8_t) prSetP2P->u4Mode;

	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prP2pNetdevRegMsg, MSG_SEND_METHOD_BUF);
#endif

	return status;

}

uint32_t
wlanoidP2pDelIface(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	p2pRoleFsmDelIface(prAdapter,
		GET_IOCTL_BSSIDX(prAdapter));

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidP2pDelIfaceDone(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	p2pRoleFsmDelIfaceDone(prAdapter,
		GET_IOCTL_BSSIDX(prAdapter));

	return WLAN_STATUS_SUCCESS;
}

#endif

#if CFG_SUPPORT_NAN
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set the nan mode.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetNANMode(struct ADAPTER *prAdapter, void *pvSetBuffer,
		  uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t *prEnable = (uint32_t *)NULL;

	if (!prAdapter || !pu4SetInfoLen || !pvSetBuffer)
		return WLAN_STATUS_FAILURE;

	*pu4SetInfoLen = sizeof(uint32_t);
	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, WARN, "Invalid length %ld\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prEnable = (uint32_t *)pvSetBuffer;

	DBGLOG(INIT, INFO, "Set nan enable[%ld]\n", *prEnable);

	if (*prEnable == 2) {
		DBGLOG(INIT, INFO, "Set nan Unsync\n", *prEnable);
		prAdapter->rNanDiscType = NAN_UNSYNC_DISC;
	} else {
		prAdapter->rNanDiscType = NAN_EXISTING_DISC;
	}

	if (*prEnable) {
		if (nanLaunch(prAdapter->prGlueInfo)) {
			/* ToDo:: ASSERT */
			if (!prAdapter->fgIsNANRegistered) {
				DBGLOG(REQ, ERROR,
					"fgIsNANRegistered is NULL\n");
				return WLAN_STATUS_FAILURE;
			}
		} else {
			status = WLAN_STATUS_FAILURE;
		}
		prAdapter->rPublishInfo.ucNanPubNum = 0;
		prAdapter->rSubscribeInfo.ucNanSubNum = 0;
	} else {
		if (prAdapter->fgIsNANRegistered)
			nanRemove(prAdapter->prGlueInfo);
	}
	return status;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the GTK rekey data
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_BUFFER_TOO_SHORT
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetGtkRekeyData(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen) {
	struct PARAM_GTK_REKEY_DATA *prGtkData;
	uint8_t ucBssIndex;
	struct BSS_INFO *prBssInfo;
	OS_SYSTIME rCurrent;

	DBGLOG(REQ, INFO, "wlanoidSetGtkRekeyData\n");

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(RSN, WARN,
		       "Fail in set rekey! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (u4SetBufferLen < sizeof(struct PARAM_GTK_REKEY_DATA)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prGtkData = (struct PARAM_GTK_REKEY_DATA *)pvSetBuffer;
	ucBssIndex = prGtkData->ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (prBssInfo && IS_BSS_AIS(prBssInfo)) {
		GET_CURRENT_SYSTIME(&rCurrent);
		if (prBssInfo->u2RekeyCount > REKEY_COUNT_LIMIT) {
			DBGLOG(RSN, WARN,
				"[%d] Disconnect due to rekey %u times within %u ms.\n",
				ucBssIndex,
				prBssInfo->u2RekeyCount,
				rCurrent - prBssInfo->rRekeyCountResetTime);
			aisFsmStateAbort(prAdapter,
					DISCONNECT_REASON_CODE_LOCALLY,
					FALSE,
					ucBssIndex);
			return WLAN_STATUS_FAILURE;
		}

		if (CHECK_FOR_TIMEOUT(rCurrent,
				prBssInfo->rRekeyCountResetTime,
				SEC_TO_SYSTIME(
					MSEC_TO_SEC(REKEY_COUNT_RESET_TIME)))) {
			GET_CURRENT_SYSTIME(&prBssInfo->rRekeyCountResetTime);
			prBssInfo->u2RekeyCount = 0;
		}
		prBssInfo->u2RekeyCount++;
	}

	return wlanSendSetQueryCmd(
			  prAdapter,
			  CMD_ID_SET_GTK_REKEY_DATA,
			  TRUE,
			  FALSE,
			  TRUE,
			  nicCmdEventSetCommon,
			  nicOidCmdTimeoutCommon,
			  u4SetBufferLen,
			  pvSetBuffer,
			  pvSetBuffer,
			  u4SetBufferLen);
}				/* wlanoidSetGtkRekeyData */

#if CFG_SUPPORT_SCHED_SCAN
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to request starting of schedule scan
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 *
 * \note The setting buffer PARAM_SCHED_SCAN_REQUEST_EXT_T
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetStartSchedScan(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen)
{
	struct PARAM_SCHED_SCAN_REQUEST *prSchedScanRequest;
	uint8_t ucBssIndex;

	if (pvSetBuffer == NULL)
		return WLAN_STATUS_INVALID_DATA;
	if (u4SetBufferLen < sizeof(struct PARAM_SCHED_SCAN_REQUEST)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prSchedScanRequest = (struct PARAM_SCHED_SCAN_REQUEST *) pvSetBuffer;
	ucBssIndex = prSchedScanRequest->ucBssIndex;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set scheduled scan! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	ASSERT(pu4SetInfoLen);
	*pu4SetInfoLen = 0;

	if (u4SetBufferLen != sizeof(struct
				     PARAM_SCHED_SCAN_REQUEST))
		return WLAN_STATUS_INVALID_LENGTH;
	else if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
		ucBssIndex) ==
		 MEDIA_STATE_CONNECTED
		 && prAdapter->fgEnOnlineScan == FALSE)
		return WLAN_STATUS_FAILURE;

	if (prAdapter->fgIsRadioOff) {
		DBGLOG(REQ, WARN,
		       "Return from BSSID list scan! (radio off). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		goto success;
	}

	if (!scnFsmSchedScanRequest(prAdapter, prSchedScanRequest)) {
		DBGLOG(REQ, WARN, "scnFsmSchedScanRequest failure !!\n");
		return WLAN_STATUS_FAILURE;
	}

success:
	prAdapter->prGlueInfo->prSchedScanRequest = prSchedScanRequest;
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to request termination of schedule scan
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_INVALID_DATA
 *
 * \note The setting buffer PARAM_SCHED_SCAN_REQUEST_EXT_T
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetStopSchedScan(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen) {
	uint32_t ret;
	struct PARAM_SCHED_SCAN_REQUEST *prSchedScanRequest =
		prAdapter->prGlueInfo->prSchedScanRequest;

	DBGLOG(REQ, INFO, "--> wlanoidSetStopSchedScan()\n");
	ASSERT(prAdapter);

	/* ask SCN module to stop scan request */
	if (scnFsmSchedScanStopRequest(prAdapter) == TRUE) {
		kalMemFree(prSchedScanRequest->pucIE,
			   VIR_MEM_TYPE,
			   prSchedScanRequest->u4IELength);
		kalMemFree(prSchedScanRequest,
			   VIR_MEM_TYPE,
			   sizeof(struct PARAM_SCHED_SCAN_REQUEST));
		prAdapter->prGlueInfo->prSchedScanRequest = NULL;
		ret = WLAN_STATUS_SUCCESS;
	} else {
		DBGLOG(REQ, WARN, "scnFsmSchedScanStopRequest failed.\n");
		ret = WLAN_STATUS_RESOURCES;
	}
	return ret;
}
#endif /* CFG_SUPPORT_SCHED_SCAN */

#if CFG_SUPPORT_PASSPOINT
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called by HS2.0 to set the assoc info, which is needed
 *	  to add to Association request frame while join HS2.0 AP.
 *
 * \param[in] prAdapter Pointer to the Adapter structure
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set
 * \param[in] u4SetBufferLen The length of the set buffer
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *   bytes read from the set buffer. If the call failed due to invalid length of
 *   the set buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_DATA If new setting value is wrong.
 * \retval WLAN_STATUS_INVALID_LENGTH
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetHS20Info(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen) {
	struct IE_HS20_INDICATION *prHS20IndicationIe;
	struct HS20_INFO *prHS20Info;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	DBGLOG(OID, LOUD, "\r\n");

	if (u4SetBufferLen == 0)
		return WLAN_STATUS_INVALID_LENGTH;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	prHS20Info = aisGetHS20Info(prAdapter, ucBssIndex);

	*pu4SetInfoLen = u4SetBufferLen;

	prHS20IndicationIe = (struct IE_HS20_INDICATION *)
			     pvSetBuffer;

	prHS20Info->ucHotspotConfig =
		prHS20IndicationIe->ucHotspotConfig;
	prHS20Info->fgConnectHS20AP = TRUE;

	DBGLOG(SEC, TRACE, "HS20 IE sz %u\n", u4SetBufferLen);

	return WLAN_STATUS_SUCCESS;

}
#endif /* CFG_SUPPORT_PASSPOINT */

#if CFG_SUPPORT_MSP
uint32_t
wlanoidQueryWlanInfo(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen) {
	return wlanQueryWlanInfo(prAdapter, pvQueryBuffer, u4QueryBufferLen,
				 pu4QueryInfoLen, TRUE);
}

uint32_t
wlanQueryWlanInfo(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen,
		 uint8_t fgIsOid) {
	struct PARAM_HW_WLAN_INFO *prHwWlanInfo;

	DBGLOG(REQ, LOUD, "\n");

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(struct PARAM_HW_WLAN_INFO);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in query receive error! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct PARAM_HW_WLAN_INFO)) {
		DBGLOG(REQ, WARN, "Too short length %u\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prHwWlanInfo = (struct PARAM_HW_WLAN_INFO *)pvQueryBuffer;

	/*  *pu4QueryInfoLen = 8 + prRxStatistics->u4TotalNum; */

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_WTBL_INFO,
				   FALSE,
				   TRUE,
				   fgIsOid,
				   nicCmdEventQueryWlanInfo,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct PARAM_HW_WLAN_INFO),
				   (uint8_t *)prHwWlanInfo,
				   pvQueryBuffer, u4QueryBufferLen);

}				/* wlanoidQueryWlanInfo */

uint32_t
wlanoidQueryMibInfo(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen) {
	return wlanQueryMibInfo(prAdapter, pvQueryBuffer, u4QueryBufferLen,
				pu4QueryInfoLen, TRUE);
}

uint32_t
wlanQueryMibInfo(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen,
		 uint8_t fgIsOid)
{
	struct PARAM_HW_MIB_INFO *prHwMibInfo;

	DBGLOG(REQ, LOUD, "\n");

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(struct PARAM_HW_MIB_INFO);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in query receive error! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct
					     PARAM_HW_MIB_INFO)) {
		DBGLOG(REQ, WARN, "Too short length %u\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prHwMibInfo = (struct PARAM_HW_MIB_INFO *)pvQueryBuffer;

	/* *pu4QueryInfoLen = 8 + prRxStatistics->u4TotalNum; */

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_MIB_INFO,
				   FALSE,
				   TRUE,
				   fgIsOid,
				   nicCmdEventQueryMibInfo,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct PARAM_HW_MIB_INFO),
				   (uint8_t *)prHwMibInfo,
				   pvQueryBuffer, u4QueryBufferLen);

}				/* wlanoidQueryMibInfo */
#endif


/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set FW log to Host.
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *			     set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_NOT_SUPPORTED
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetFwLog2Host(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen) {
	struct CMD_FW_LOG_2_HOST_CTRL *prFwLog2HostCtrl;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct CMD_FW_LOG_2_HOST_CTRL);

	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set FW log to Host! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4SetBufferLen < sizeof(struct
					   CMD_FW_LOG_2_HOST_CTRL)) {
		DBGLOG(REQ, WARN, "Too short length %d\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prFwLog2HostCtrl = (struct CMD_FW_LOG_2_HOST_CTRL *)
			   pvSetBuffer;
	DBGLOG(REQ, INFO, "McuDest %d, LogType %d\n",
	       prFwLog2HostCtrl->ucMcuDest,
	       prFwLog2HostCtrl->ucFwLog2HostCtrl);

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_FW_LOG_2_HOST,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_FW_LOG_2_HOST_CTRL),
				   (uint8_t *)prFwLog2HostCtrl,
				   pvSetBuffer, u4SetBufferLen);
}

uint32_t
wlanoidSetPhyCtrl(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	struct UNI_CMD_PHY_CTRL_LIST_DUMP *phycrcmdbuf;
	struct UNI_CMD_PHY_LIST_DUMP_CR *tag;

	uint32_t max_cmd_len = sizeof(struct UNI_CMD_PHY_CTRL_LIST_DUMP) +
				sizeof(struct UNI_CMD_PHY_LIST_DUMP_CR);

	if (prAdapter == NULL || pu4SetInfoLen == NULL)
		return WLAN_STATUS_ADAPTER_NOT_READY;
	if (u4SetBufferLen < sizeof(struct UNI_CMD_PHY_CTRL_LIST_DUMP))
		return WLAN_STATUS_INVALID_LENGTH;
	if (!pvSetBuffer)
		return WLAN_STATUS_INVALID_DATA;

	phycrcmdbuf = (struct UNI_CMD_PHY_CTRL_LIST_DUMP *)
				cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);

	if (!phycrcmdbuf) {
		DBGLOG(INIT, ERROR,
				"Allocate UNI_CMD_PHY_CTRL_LIST_DUMP ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	memcpy(phycrcmdbuf, pvSetBuffer, max_cmd_len);

	phycrcmdbuf->ucAction =
		((struct UNI_CMD_PHY_CTRL_LIST_DUMP *)pvSetBuffer)->ucAction;
	tag = (struct UNI_CMD_PHY_LIST_DUMP_CR *) phycrcmdbuf->aucTlvBuffer;
	tag->u2Length = sizeof(struct UNI_CMD_PHY_LIST_DUMP_CR);

	return wlanSendSetQueryUniCmd(prAdapter,
				   UNI_CMD_ID_PHY_LIST_DUMP,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicUniCmdEventSetPhyCtrl,
				   nicOidCmdTimeoutCommon,
				   max_cmd_len,
				   (uint8_t *)phycrcmdbuf,
				   pvSetBuffer, u4SetBufferLen);
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t
wlanoidNotifyFwSuspend(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen) {
	struct CMD_SUSPEND_MODE_SETTING *prSuspendCmd;

	if (!prAdapter || !pvSetBuffer)
		return WLAN_STATUS_INVALID_DATA;
	if (u4SetBufferLen < sizeof(struct CMD_SUSPEND_MODE_SETTING)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prSuspendCmd = (struct CMD_SUSPEND_MODE_SETTING *)
		       pvSetBuffer;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_SUSPEND_MODE,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_SUSPEND_MODE_SETTING),
				   (uint8_t *)prSuspendCmd,
				   NULL,
				   0);
}

uint32_t
wlanoidQueryCnm(
	struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen) {
	struct PARAM_GET_CNM_T *prCnmInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (prAdapter->fgIsEnableLpdvt)
		return WLAN_STATUS_NOT_SUPPORTED;

	*pu4QueryInfoLen = sizeof(struct PARAM_GET_CNM_T);

	if (u4QueryBufferLen < sizeof(struct PARAM_GET_CNM_T))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	prCnmInfo = (struct PARAM_GET_CNM_T *)pvQueryBuffer;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_GET_CNM,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryCnmInfo,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct PARAM_GET_CNM_T),
				   (uint8_t *)prCnmInfo,
				   pvQueryBuffer, u4QueryBufferLen);
}

uint32_t
wlanoidPacketKeepAlive(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen) {
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_PACKET_KEEPALIVE_T *prPacket;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	*pu4SetInfoLen = sizeof(struct PARAM_PACKET_KEEPALIVE_T);

	/* Check for query buffer length */
	if (u4SetBufferLen < *pu4SetInfoLen) {
		DBGLOG(OID, WARN, "Too short length %u\n", u4SetBufferLen);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	prPacket = (struct PARAM_PACKET_KEEPALIVE_T *)
		   kalMemAlloc(sizeof(struct PARAM_PACKET_KEEPALIVE_T),
			       VIR_MEM_TYPE);
	if (!prPacket) {
		DBGLOG(OID, ERROR,
		       "Can not alloc memory for struct PARAM_PACKET_KEEPALIVE_T\n");
		return -ENOMEM;
	}
	kalMemCopy(prPacket, pvSetBuffer,
		   sizeof(struct PARAM_PACKET_KEEPALIVE_T));

	DBGLOG(OID, INFO, "enable=%d, index=%d\r\n",
	       prPacket->enable, prPacket->index);

	rStatus = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_WFC_KEEP_ALIVE,
				      TRUE,
				      FALSE,
				      TRUE,
				      nicCmdEventSetCommon,
				      nicOidCmdTimeoutCommon,
				      sizeof(struct PARAM_PACKET_KEEPALIVE_T),
				      (uint8_t *)prPacket, NULL, 0);
	kalMemFree(prPacket, VIR_MEM_TYPE,
		   sizeof(struct PARAM_PACKET_KEEPALIVE_T));
	return rStatus;
}

#if CFG_SUPPORT_DBDC
uint32_t
wlanoidSetDbdcEnable(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen) {
	uint8_t ucDBDCEnable;

	if (!prAdapter || !pvSetBuffer || !u4SetBufferLen)
		return WLAN_STATUS_INVALID_DATA;

	/* Be careful.
	 * We only use the test cmd "set_dbdc" to enable DBDC HW
	 * wo/ OP Mode Change. Besides, it may also confuse original
	 * DBDC FSM.
	 */
	kalMemCopy(&ucDBDCEnable, pvSetBuffer, u4SetBufferLen);
	cnmUpdateDbdcSetting(prAdapter, ucDBDCEnable);

	return WLAN_STATUS_SUCCESS;
}
#endif /*#if CFG_SUPPORT_DBDC*/

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set tx target power base.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQuerySetTxTargetPower(struct ADAPTER *prAdapter,
			     void *pvSetBuffer, uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_SET_TX_TARGET_POWER
		*prSetTxTargetPowerInfo;
	struct CMD_SET_TX_TARGET_POWER rCmdSetTxTargetPower;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct
				PARAM_CUSTOM_SET_TX_TARGET_POWER *);

	if (u4SetBufferLen < sizeof(struct
				    PARAM_CUSTOM_SET_TX_TARGET_POWER *))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prSetTxTargetPowerInfo =
		(struct PARAM_CUSTOM_SET_TX_TARGET_POWER *) pvSetBuffer;

	kalMemSet(&rCmdSetTxTargetPower, 0,
		  sizeof(struct CMD_SET_TX_TARGET_POWER));

	rCmdSetTxTargetPower.ucTxTargetPwr =
		prSetTxTargetPowerInfo->ucTxTargetPwr;

	DBGLOG(INIT, INFO,
	       "MT6632 : wlanoidQuerySetTxTargetPower =%x dbm\n",
	       rCmdSetTxTargetPower.ucTxTargetPwr);

	rWlanStatus = wlanSendSetQueryCmd(prAdapter,
			  CMD_ID_SET_TX_PWR,
			  TRUE,  /* fgSetQuery Bit:  True->write  False->read */
			  FALSE, /* fgNeedResp */
			  TRUE,  /* fgIsOid*/
			  nicCmdEventSetCommon, /* REF: wlanoidSetDbdcEnable */
			  nicOidCmdTimeoutCommon,
			  sizeof(struct CMD_ACCESS_EFUSE),
			  (uint8_t *) (&rCmdSetTxTargetPower), pvSetBuffer,
			  u4SetBufferLen);

	return rWlanStatus;
}
#endif /*CFG_SUPPORT_QA_TOOL*/

#if (CFG_SUPPORT_DFS_MASTER == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set rdd report.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQuerySetRddReport(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_SET_RDD_REPORT *prSetRddReport;
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_SET_RDD_REPORT
				*);

	ASSERT(pvSetBuffer);
	if (u4SetBufferLen < sizeof(struct PARAM_CUSTOM_SET_RDD_REPORT)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prSetRddReport = (struct PARAM_CUSTOM_SET_RDD_REPORT *)
			 pvSetBuffer;

	prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)
			    cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					sizeof(*prCmdRddOnOffCtrl));

	ASSERT(prCmdRddOnOffCtrl);
	if (prCmdRddOnOffCtrl == NULL) {
		DBGLOG(INIT, ERROR, "prCmdRddOnOffCtrl is NULL");
		return WLAN_STATUS_FAILURE;
	}

	prCmdRddOnOffCtrl->ucDfsCtrl = RDD_RADAR_EMULATE;

	prCmdRddOnOffCtrl->ucRddIdx = prSetRddReport->ucDbdcIdx;

	if (prCmdRddOnOffCtrl->ucRddIdx)
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_1;
	else
		prCmdRddOnOffCtrl->ucRddRxSel = RDD_IN_SEL_0;

	DBGLOG(INIT, INFO,
	       "MT6632 : wlanoidQuerySetRddReport -  DFS ctrl: %.d, RDD index: %d\n",
	       prCmdRddOnOffCtrl->ucDfsCtrl, prCmdRddOnOffCtrl->ucRddIdx);

	rWlanStatus = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_RDD_ON_OFF_CTRL,
			TRUE,  /* fgSetQuery Bit: True->write False->read */
			FALSE, /* fgNeedResp */
			TRUE,  /* fgIsOid*/
			nicCmdEventSetCommon, /* REF: wlanoidSetDbdcEnable */
			nicOidCmdTimeoutCommon,
			sizeof(*prCmdRddOnOffCtrl),
			(uint8_t *) (prCmdRddOnOffCtrl), pvSetBuffer,
			u4SetBufferLen);

	cnmMemFree(prAdapter, prCmdRddOnOffCtrl);

	return rWlanStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set rdd report.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQuerySetRadarDetectMode(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen) {
	struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE
		*prSetRadarDetectMode;
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen =
		sizeof(struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE *);

	ASSERT(pvSetBuffer);
	if (u4SetBufferLen <
		sizeof(struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prSetRadarDetectMode =
		(struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE *) pvSetBuffer;

	prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)cnmMemAlloc(
						prAdapter, RAM_TYPE_MSG,
						sizeof(*prCmdRddOnOffCtrl));

	ASSERT(prCmdRddOnOffCtrl);
	if (prCmdRddOnOffCtrl == NULL) {
		DBGLOG(INIT, ERROR, "prCmdRddOnOffCtrl is NULL");
		return WLAN_STATUS_FAILURE;
	}

	prCmdRddOnOffCtrl->ucDfsCtrl = RDD_DET_MODE;

	prCmdRddOnOffCtrl->ucSetVal =
		prSetRadarDetectMode->ucRadarDetectMode;

	DBGLOG(INIT, INFO,
	       "MT6632 : wlanoidQuerySetRadarDetectMode -  DFS ctrl: %.d, Radar Detect Mode: %d\n",
	       prCmdRddOnOffCtrl->ucDfsCtrl, prCmdRddOnOffCtrl->ucSetVal);

	rWlanStatus = wlanSendSetQueryCmd(prAdapter,
			  CMD_ID_RDD_ON_OFF_CTRL,
			  TRUE,   /* fgSetQuery Bit: True->write False->read */
			  FALSE,   /* fgNeedResp */
			  TRUE,   /* fgIsOid*/
			  nicCmdEventSetCommon, /* REF: wlanoidSetDbdcEnable */
			  nicOidCmdTimeoutCommon,
			  sizeof(*prCmdRddOnOffCtrl),
			  (uint8_t *) (prCmdRddOnOffCtrl),
			  pvSetBuffer,
			  u4SetBufferLen);

	cnmMemFree(prAdapter, prCmdRddOnOffCtrl);

	return rWlanStatus;
}
#endif /*(CFG_SUPPORT_DFS_MASTER == 1)*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set rdd report.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidInitRddLog(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t size,
			 uint32_t *pu4SetInfoLen)
{
	struct _ATE_LOG_DUMP_CB *log_cb;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	size = 2000;
	ASSERT(prAdapter);
	log_cb = &prAdapter->rRddRawData;
	if (!log_cb->entry) {
		kalMemZero(log_cb, sizeof(*log_cb));
		log_cb->entry =
			kalMemAlloc(size * sizeof(struct _ATE_LOG_DUMP_ENTRY),
			VIR_MEM_TYPE);

		if (log_cb->entry == NULL)
			goto err0;

		kalMemZero(log_cb->entry, size * sizeof
			(struct _ATE_LOG_DUMP_ENTRY));
		log_cb->len = size;

		DBGLOG(INIT, ERROR,
			"Init log cb size: %u, log_cb->len: %u\n",
			size, log_cb->len);
		log_cb->idx = 0;
	}

	log_cb->overwritable = FALSE;
	log_cb->is_overwritten = FALSE;
#ifdef LOGDUMP_TO_FILE
	log_cb->file_idx = 0;
#endif

	return rWlanStatus;
err0:
	DBGLOG(INIT, ERROR, "%s: Alcated memory fail! size %u\n",
		__func__, size);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set rdd report.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetRddStart(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen)
{
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

#if (CFG_SUPPORT_DFS_MASTER == 1)
		prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(*prCmdRddOnOffCtrl));

		if (!prCmdRddOnOffCtrl)
			return WLAN_STATUS_FAILURE;

		prCmdRddOnOffCtrl->ucDfsCtrl = TESTMODE_RDD_START;
		prCmdRddOnOffCtrl->ucRddIdx = 0x0;

		DBGLOG(RFTEST, ERROR,
			"TESTMODE RDD_START - DFS ctrl: %d, RDD index: %d\n",
			prCmdRddOnOffCtrl->ucDfsCtrl,
			prCmdRddOnOffCtrl->ucRddIdx);

		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_RDD_ON_OFF_CTRL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(*prCmdRddOnOffCtrl),
			(uint8_t *) prCmdRddOnOffCtrl, NULL, 0);

		cnmMemFree(prAdapter, prCmdRddOnOffCtrl);
#endif /*(CFG_SUPPORT_DFS_MASTER == 1)*/

	return WLAN_STATUS_SUCCESS;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set rdd report.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetRddStop(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen)
{
	struct CMD_RDD_ON_OFF_CTRL *prCmdRddOnOffCtrl;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

#if (CFG_SUPPORT_DFS_MASTER == 1)
		prCmdRddOnOffCtrl = (struct CMD_RDD_ON_OFF_CTRL *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			sizeof(*prCmdRddOnOffCtrl));

		if (!prCmdRddOnOffCtrl)
			return WLAN_STATUS_FAILURE;

		prCmdRddOnOffCtrl->ucDfsCtrl = TESTMODE_RDD_STOP;

		prCmdRddOnOffCtrl->ucRddIdx = 0x0;

		DBGLOG(RFTEST, ERROR,
			"Testmode RDD_STOP - DFS ctrl: %d, RDD index: %d\n",
			prCmdRddOnOffCtrl->ucDfsCtrl,
			prCmdRddOnOffCtrl->ucRddIdx);

		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_RDD_ON_OFF_CTRL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(*prCmdRddOnOffCtrl),
			(uint8_t *) prCmdRddOnOffCtrl, NULL, 0);

		cnmMemFree(prAdapter, prCmdRddOnOffCtrl);
#endif /*(CFG_SUPPORT_DFS_MASTER == 1)*/

	return WLAN_STATUS_SUCCESS;
}

static int MT_ATERDDParseResult(struct _ATE_LOG_DUMP_ENTRY entry, int8_t idx)
{
	struct _ATE_RDD_LOG *result = NULL;
	uint32_t *pulse = 0;

	result = &entry.rdd;
	pulse = (uint32_t *)result->aucBuffer;

	DBGLOG(INIT, ERROR,
		"[RDD]%08x %08x[RDD0]\n", pulse[0], pulse[1]);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set rdd report.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryRddLog(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen)
{
	struct _ATE_LOG_DUMP_CB *log_cb;
	struct test_rdd_dump_params_s *log_cb_qa;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	log_cb = &prAdapter->rRddRawData;

	log_cb_qa = (struct test_rdd_dump_params_s *)pvSetBuffer;
	DBGLOG(REQ, ERROR, "[DUMP START] idx : %d, log_cb : %d\n",
		log_cb_qa->rdd_cnt, log_cb_qa->rdd_dw_num);

	log_cb_qa->rdd_cnt = log_cb->idx;
	log_cb_qa->rdd_dw_num = log_cb->len;
	DBGLOG(REQ, ERROR, "[DUMP START] idx : %d, log_cb : %d\n",
		log_cb_qa->rdd_cnt, log_cb_qa->rdd_dw_num);

	return rWlanStatus;
}

uint32_t
wlanoidQueryRddLogContent(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen)
{
	struct _ATE_LOG_DUMP_CB *log_cb;
	struct _ATE_LOG_DUMP_CB *log_cb_qa_a;
	int32_t idx = 0;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	int32_t (*dump_func)(struct _ATE_LOG_DUMP_ENTRY, int8_t idx) = NULL;

	log_cb = &prAdapter->rRddRawData;

	log_cb_qa_a = (struct _ATE_LOG_DUMP_CB *)pvSetBuffer;

	if (!log_cb || !log_cb->entry)
		goto err0;

	if (!log_cb->idx)
		return WLAN_STATUS_INVALID_DATA;

	if (log_cb->is_overwritten)
		idx = log_cb->idx;

	dump_func = MT_ATERDDParseResult;

	if (!dump_func)
		goto err0;

	log_cb->is_dumping = TRUE;

	kalMemCopy(log_cb_qa_a, log_cb, sizeof(*log_cb));

	if (!log_cb_qa_a
	|| log_cb->entry[idx].un_dumped == 0
	|| log_cb_qa_a->entry[idx].un_dumped == 0)
		goto err0;

	do {
		if (log_cb->entry[idx].un_dumped) {
			dump_func(log_cb->entry[idx], idx);
			log_cb->entry[idx].un_dumped = FALSE;
		}
		INC_RING_INDEX3(idx, log_cb->len);
	} while (idx != log_cb->idx);

	if (idx == log_cb->idx) {
		DBGLOG(REQ, ERROR,
		"[DUMP START] idx : %d, log_cb : %d, log_cb_qa_a : %d\n",
		idx, log_cb->entry[0].un_dumped,
		log_cb_qa_a->entry[0].un_dumped);
	}

	log_cb->idx = 0;
	log_cb->is_dumping = FALSE;
	DBGLOG(REQ, ERROR, "[DUMP END], log_cb->len : %d, %d\n",
	log_cb_qa_a->idx, log_cb_qa_a->len);
	return rWlanStatus;

err0:
	return -1;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to turn radio off.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidLinkDown(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen) {
	uint8_t ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = 0;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in set link down! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	aisBssLinkDown(prAdapter, ucBssIndex);

	return WLAN_STATUS_SUCCESS;
} /* wlanoidSetDisassociate */

#if CFG_WIFI_TXPWR_TBL_DUMP
#define WIFI_TXPWR_TBL_DUMP_VER 0x01
uint32_t
wlanoidGetTxPwrTbl(struct ADAPTER *prAdapter,
		   void *pvQueryBuffer,
		   uint32_t u4QueryBufferLen,
		   uint32_t *pu4QueryInfoLen)
{
	struct CMD_GET_TXPWR_TBL CmdPwrTbl;
	struct PARAM_CMD_GET_TXPWR_TBL *prPwrTbl = NULL;

	DBGLOG(REQ, LOUD, "\n");

	if (!prAdapter || (!pvQueryBuffer && u4QueryBufferLen) ||
	    !pu4QueryInfoLen)
		return WLAN_STATUS_INVALID_DATA;

	*pu4QueryInfoLen = sizeof(struct PARAM_CMD_GET_TXPWR_TBL);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in query receive error! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct PARAM_CMD_GET_TXPWR_TBL)) {
		DBGLOG(REQ, WARN, "Too short length %ld\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prPwrTbl = (struct PARAM_CMD_GET_TXPWR_TBL *)pvQueryBuffer;

	CmdPwrTbl.ucCmdVer = WIFI_TXPWR_TBL_DUMP_VER;
	CmdPwrTbl.u2CmdLen = sizeof(struct CMD_GET_TXPWR_TBL);
	CmdPwrTbl.ucDbdcIdx = prPwrTbl->ucDbdcIdx;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_GET_TXPWR_TBL,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventGetTxPwrTbl,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_GET_TXPWR_TBL),
				   (uint8_t *)&CmdPwrTbl,
				   pvQueryBuffer,
				   u4QueryBufferLen);

}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to do AIS pre-Suspend flow.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                          bytes read from the set buffer. If the call failed
 *                          due to invalid length of the set buffer, returns
 *                          the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidAisPreSuspend(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen) {
#if CFG_WOW_SUPPORT
	struct WIFI_VAR *prWifiVar = NULL;
	struct BSS_INFO *prAisBssInfo = NULL;

	if (prAdapter == NULL || pu4SetInfoLen == NULL)
		return WLAN_STATUS_ADAPTER_NOT_READY;

	*pu4SetInfoLen = 0;
	prWifiVar = &prAdapter->rWifiVar;

	aisPreSuspendFlow(prAdapter);

#if CFG_ENABLE_WIFI_DIRECT
	p2pRoleProcessPreSuspendFlow(prAdapter);
#endif

	if (IS_FEATURE_ENABLED(prWifiVar->ucWow) &&
		IS_FEATURE_DISABLED(prAdapter->rWowCtrl.fgWowEnable) &&
		IS_FEATURE_DISABLED(prWifiVar->ucAdvPws))
	{
		prAisBssInfo = aisGetConnectedBssInfo(prAdapter);
		if (prAisBssInfo != NULL) {
			/* Need Link down, and connected AIS BSS existed,
			 * ->pending oid until disconnection done (i.e. AIS switch to IDLE)
			 */
			prAdapter->fgWowLinkDownPendFlag = TRUE;
			return WLAN_STATUS_PENDING;
		}
	}

	prAdapter->fgWowLinkDownPendFlag = FALSE;
#endif

	return WLAN_STATUS_SUCCESS;
} /* wlanoidPreSuspend */

#if CFG_SUPPORT_CSI
uint32_t
wlanoidSetCSIControl(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct CMD_CSI_CONTROL_T *pCSICtrl;

	*pu4SetInfoLen = sizeof(struct CMD_CSI_CONTROL_T);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
			"[CSI] (Adapter not ready). ACPI=D%d, Radio=%d\n",
			   prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4SetBufferLen < sizeof(struct CMD_CSI_CONTROL_T)) {
		DBGLOG(REQ, WARN,
			"[CSI] Too short length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	pCSICtrl = (struct CMD_CSI_CONTROL_T *)pvSetBuffer;

	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_CSI_CONTROL,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_CSI_CONTROL_T),
				(uint8_t *)pCSICtrl,
				pvSetBuffer, u4SetBufferLen);
}
#endif

#if CFG_SUPPORT_EASY_DEBUG
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set fw cfg info
 *
 * \param[in] prAdapter      Pointer to the Adapter structure.
 * \param[in] pvSetBuffer    Pointer to the buffer that holds the data to be
 *                           set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_INVALID_DATA
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanoidSetFwParam(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	ASSERT(prAdapter);

	if (!pvSetBuffer || !u4SetBufferLen) {
		DBGLOG(OID, ERROR, "Buffer is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}
	DBGLOG(OID, INFO, "Fw Params: %s\n", (uint8_t *)pvSetBuffer);
	return wlanFwCfgParse(prAdapter, (uint8_t *)pvSetBuffer);
}
#endif /* CFG_SUPPORT_EASY_DEBUG */

uint32_t
wlanoidAbortScan(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen) {

	struct AIS_FSM_INFO *prAisFsmInfo = NULL;
	uint8_t ucBssIndex = 0;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_SCAN ||
			prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
		DBGLOG(OID, INFO, "ucBssIndex = %d\n", ucBssIndex);
		prAisFsmInfo->fgIsScanOidAborted = TRUE;
		aisFsmStateAbort_SCAN(prAdapter, ucBssIndex);
	}
	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidDisableTdlsPs(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	struct CMD_TDLS_PS_T rTdlsPs;

	if (!prAdapter || !pvSetBuffer)
		return WLAN_STATUS_INVALID_DATA;
	if (u4SetBufferLen < sizeof(uint8_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	kalMemSet(&rTdlsPs, 0, sizeof(struct CMD_TDLS_PS_T));
	rTdlsPs.ucIsEnablePs = *(uint8_t *)pvSetBuffer;
	DBGLOG(OID, INFO, "enable tdls ps %d\n",
	       rTdlsPs.ucIsEnablePs);
	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_TDLS_PS,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(rTdlsPs),
				   (uint8_t *)&rTdlsPs,
				   NULL,
				   0);
}

uint32_t wlanoidSetSer(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen)
{
	uint32_t u4CmdId;

	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	if (u4SetBufferLen != sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	u4CmdId = *((uint32_t *)pvSetBuffer);

	DBGLOG(OID, INFO, "Set SER CMD[%d]\n", u4CmdId);

	switch (u4CmdId) {
	case SER_USER_CMD_DISABLE:
		prAdapter->rWifiVar.eEnableSerL1 = FEATURE_OPT_SER_DISABLE;

		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET,
				 SER_SET_DISABLE, 0);
		break;

	case SER_USER_CMD_ENABLE:
		prAdapter->rWifiVar.eEnableSerL1 = FEATURE_OPT_SER_ENABLE;

		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET, SER_SET_ENABLE, 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_TRACKING_ONLY:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 SER_ENABLE_TRACKING, 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_L1_RECOVER_ONLY:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 SER_ENABLE_TRACKING | SER_ENABLE_L1_RECOVER,
				 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_L2_RECOVER_ONLY:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 SER_ENABLE_TRACKING | SER_ENABLE_L2_RECOVER,
				 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_L3_RX_ABORT_ONLY:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 SER_ENABLE_TRACKING | SER_ENABLE_L3_RX_ABORT,
				 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_L3_TX_ABORT_ONLY:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 SER_ENABLE_TRACKING | SER_ENABLE_L3_TX_ABORT,
				 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_L3_TX_DISABLE_ONLY:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 SER_ENABLE_TRACKING |
				 SER_ENABLE_L3_TX_DISABLE, 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_L3_BFRECOVER_ONLY:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 SER_ENABLE_TRACKING |
				 SER_ENABLE_L3_BF_RECOVER, 0);
		break;

	case SER_USER_CMD_ENABLE_MASK_RECOVER_ALL:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_SET_ENABLE_MASK,
				 (SER_ENABLE_TRACKING |
				  SER_ENABLE_L1_RECOVER |
				  SER_ENABLE_L2_RECOVER |
				  SER_ENABLE_L3_RX_ABORT |
				  SER_ENABLE_L3_TX_ABORT |
				  SER_ENABLE_L3_TX_DISABLE |
				  SER_ENABLE_L3_BF_RECOVER), 0);
		break;

	case SER_USER_CMD_L0_RECOVER:
		GL_USER_DEFINE_RESET_TRIGGER(prAdapter, RST_CMD_TRIGGER,
				RST_FLAG_DO_WHOLE_RESET);
		break;

	case SER_USER_CMD_L0P5_RECOVER:
		GL_USER_DEFINE_RESET_TRIGGER(prAdapter, RST_CMD_TRIGGER,
				RST_FLAG_DO_L0P5_RESET);
		break;

	case SER_USER_CMD_L1_RECOVER:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L1_RECOVER, 0);
		break;

	case SER_USER_CMD_L2_BN0_RECOVER:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L2_RECOVER, ENUM_BAND_0);
		break;

	case SER_USER_CMD_L2_BN1_RECOVER:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L2_RECOVER, ENUM_BAND_1);
		break;

	case SER_USER_CMD_L3_RX0_ABORT:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L3_RX_ABORT, ENUM_BAND_0);
		break;

	case SER_USER_CMD_L3_RX1_ABORT:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L3_RX_ABORT, ENUM_BAND_1);
		break;

	case SER_USER_CMD_L3_TX0_ABORT:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L3_TX_ABORT, ENUM_BAND_0);
		break;

	case SER_USER_CMD_L3_TX1_ABORT:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L3_TX_ABORT, ENUM_BAND_1);
		break;

	case SER_USER_CMD_L3_TX0_DISABLE:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L3_TX_DISABLE, ENUM_BAND_0);
		break;

	case SER_USER_CMD_L3_TX1_DISABLE:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L3_TX_DISABLE, ENUM_BAND_1);
		break;

	case SER_USER_CMD_L3_BF_RECOVER:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L3_BF_RECOVER, 0);
		break;

	case SER_USER_CMD_L0P5_PAUSE_WDT:
		prAdapter->rWifiVar.eEnableSerL0p5 = FEATURE_OPT_SER_DISABLE;

		wlanoidSerExtCmd(prAdapter, SER_ACTION_L0P5_CTRL,
				 SER_ACTION_L0P5_CTRL_PAUSE_WDT, 0);
		break;

	case SER_USER_CMD_L0P5_RESUME_WDT:
		prAdapter->rWifiVar.eEnableSerL0p5 = FEATURE_OPT_SER_ENABLE;

		wlanoidSerExtCmd(prAdapter, SER_ACTION_L0P5_CTRL,
				 SER_ACTION_L0P5_CTRL_RESUME_WDT, 0);
		break;

	case SER_USER_CMD_L0P5_WM_HANG:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_L0P5_CTRL,
				 SER_ACTION_L0P5_CTRL_WM_HANG, 0);
		break;

	case SER_USER_CMD_L0P5_BUS_HANG:
		wlanoidSerExtCmd(prAdapter, SER_ACTION_L0P5_CTRL,
				 SER_ACTION_L0P5_CTRL_BUS_HANG, 0);
		break;
	default:
		DBGLOG(OID, ERROR, "Error SER CMD\n");
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidSerExtCmd(struct ADAPTER *prAdapter, uint8_t ucAction,
			 uint8_t ucSerSet, uint8_t ucDbdcIdx)
{
	struct EXT_CMD_SER_T rCmdSer = {0};
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	rCmdSer.ucAction = ucAction;
	rCmdSer.ucSerSet = ucSerSet;
	rCmdSer.ucDbdcIdx = ucDbdcIdx;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
					 CMD_ID_LAYER_0_EXT_MAGIC_NUM,
					 EXT_CMD_ID_SER,
					 TRUE,
					 FALSE,
					 TRUE,
					 NULL,
					 nicOidCmdTimeoutCommon,
					 sizeof(struct EXT_CMD_SER_T),
					 (uint8_t *)&rCmdSer, NULL, 0);
	return rStatus;
}

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set rdd report.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuf A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryTxPowerInfo(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen) {
	struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T *prTxPowerInfo =
			NULL;
	struct CMD_TX_POWER_SHOW_INFO_T rCmdTxPowerShowInfo;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;
	if (!pvQueryBuffer)
		return WLAN_STATUS_FAILURE;
	if (!pu4QueryInfoLen)
		return WLAN_STATUS_FAILURE;

	if (u4QueryBufferLen <
	    sizeof(struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T)) {
		*pu4QueryInfoLen = sizeof(struct
					  PARAM_TXPOWER_ALL_RATE_POWER_INFO_T);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	*pu4QueryInfoLen = sizeof(struct
				  PARAM_TXPOWER_ALL_RATE_POWER_INFO_T);

	prTxPowerInfo = (struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T
			 *) pvQueryBuffer;

	kalMemSet(&rCmdTxPowerShowInfo, 0,
		  sizeof(struct CMD_TX_POWER_SHOW_INFO_T));

	rCmdTxPowerShowInfo.ucPowerCtrlFormatId =
		TX_POWER_SHOW_INFO;
	rCmdTxPowerShowInfo.ucTxPowerInfoCatg =
		prTxPowerInfo->ucTxPowerCategory;
	rCmdTxPowerShowInfo.ucBandIdx = prTxPowerInfo->ucBandIdx;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_TX_POWER_FEATURE_CTRL,
			     FALSE, /* Query Bit: True->write False->read */
			     TRUE,
			     TRUE,
			     nicCmdEventQueryTxPowerInfo,
			     nicOidCmdTimeoutCommon,
			     sizeof(struct CMD_TX_POWER_SHOW_INFO_T),
			     (uint8_t *) (&rCmdTxPowerShowInfo),
			     pvQueryBuffer,
			     u4QueryBufferLen);

	return rWlanStatus;
}
#endif
uint32_t
wlanoidSetTxPowerByRateManual(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen) {

	struct PARAM_TXPOWER_BY_RATE_SET_T *prPwrParam;
	struct CMD_POWER_RATE_TXPOWER_CTRL_T rCmdPwrCtl;
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;
	if (!pvSetBuffer)
		return WLAN_STATUS_FAILURE;
	if (u4SetBufferLen < sizeof(struct PARAM_TXPOWER_BY_RATE_SET_T)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prPwrParam = (struct PARAM_TXPOWER_BY_RATE_SET_T
			 *) pvSetBuffer;

	kalMemSet(&rCmdPwrCtl, 0,
		  sizeof(struct CMD_POWER_RATE_TXPOWER_CTRL_T));

	rCmdPwrCtl.u1PowerCtrlFormatId = TX_RATE_POWER_CTRL;
	rCmdPwrCtl.u1PhyMode = prPwrParam->u1PhyMode;
	rCmdPwrCtl.u1TxRate = prPwrParam->u1TxRate;
	rCmdPwrCtl.u1BW = prPwrParam->u1BW;
	rCmdPwrCtl.i1TxPower = prPwrParam->i1TxPower;

	rWlanStatus = wlanSendSetQueryExtCmd(prAdapter,
			     CMD_ID_LAYER_0_EXT_MAGIC_NUM,
			     EXT_CMD_ID_TX_POWER_FEATURE_CTRL,
			     TRUE, /* Query Bit: True->write False->read */
			     FALSE,
			     TRUE,
			     nicCmdEventSetCommon,
			     nicOidCmdTimeoutCommon,
			     sizeof(rCmdPwrCtl),
			     (uint8_t *) (&rCmdPwrCtl),
			     NULL,
			     0);

	return rWlanStatus;
}

#if CFG_SUPPORT_MBO
uint32_t
wlanoidBssDisallowedList(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen) {
	struct WIFI_VAR *prWifiVar = NULL;


	if (!prAdapter || !u4SetBufferLen || !pvSetBuffer ||
		u4SetBufferLen != sizeof(struct PARAM_BSS_DISALLOWED_LIST))
		return WLAN_STATUS_NOT_ACCEPTED;

	prWifiVar = &prAdapter->rWifiVar;
	kalMemCopy(&prWifiVar->rBssDisallowedList, pvSetBuffer, u4SetBufferLen);
	DBGLOG(OID, INFO, "Set disallowed list size: %d\n",
		prWifiVar->rBssDisallowedList.u4NumBssDisallowed);
	return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_SUPPORT_ROAMING

uint32_t
wlanoidSetDrvRoamingPolicy(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen) {
	uint32_t u4RoamingPoily = 0;
	struct ROAMING_INFO *prRoamingFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint32_t u4CurConPolicy;
	uint8_t ucBssIndex = 0;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	prRoamingFsmInfo = aisGetRoamingInfo(prAdapter, ucBssIndex);
	prConnSettings = aisGetConnSettings(prAdapter, ucBssIndex);

	u4RoamingPoily = *(uint32_t *)pvSetBuffer;
	u4CurConPolicy = prConnSettings->eConnectionPolicy;

	/* enable/disable fw roaming only when roaming fsm started */
	if (prRoamingFsmInfo->eCurrentState != ROAMING_STATE_IDLE) {
		if (u4RoamingPoily == 1) {
			prConnSettings->eConnectionPolicy =
				CONNECT_BY_SSID_BEST_RSSI;
			/* roaming fsm already started, enable fw roaming */
			roamingFsmSendStartCmd(prAdapter, ucBssIndex);
		} else {
			roamingFsmSendAbortCmd(prAdapter, ucBssIndex);
		}
	}

	DBGLOG(REQ, INFO, "RoamingPoily=%d, conn policy [%d] -> [%d]\n",
	       u4RoamingPoily, u4CurConPolicy,
	       prConnSettings->eConnectionPolicy);
	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_ANDROID_DUAL_STA == 1)
uint32_t wlanoidSetMultiStaPrimaryInterface(struct ADAPTER
				    *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	uint32_t u4PrevPrimaryInterface;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	u4PrevPrimaryInterface = prAdapter->u4MultiStaPrimaryInterface;
	prAdapter->u4MultiStaPrimaryInterface = *(uint32_t *)pvSetBuffer;

	DBGLOG(REQ, INFO, "Update multista primary interface:[%s]\n",
			prAdapter->u4MultiStaPrimaryInterface ==
			AIS_DEFAULT_INDEX ? "wlan0" : "wlan1");

	if (prAdapter->ucIsMultiStaConnected && u4PrevPrimaryInterface !=
			prAdapter->u4MultiStaPrimaryInterface)
		aisMultiStaSetQuoteTime(prAdapter, TRUE);

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidSetMultiStaUseCase(struct ADAPTER
				    *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prAdapter->u4MultiStaUseCase = *(uint32_t *)pvSetBuffer;
	prAdapter->fgForceDualStaInMCCMode =
		prAdapter->u4MultiStaUseCase ==
		WIFI_DUAL_STA_TRANSIENT_PREFER_PRIMARY ? TRUE : FALSE;

	DBGLOG(REQ, INFO, "Update multista use case:[%d]\n",
			prAdapter->u4MultiStaUseCase);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t wlanoidUpdateFtIes(struct ADAPTER *prAdapter, void *pvSetBuffer,
			    uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct FT_IES *prFtIes = NULL;
	uint32_t u4IeLen = 0;
	uint8_t *pucIEStart = NULL;
	uint16_t u2MD = 0;
	uint32_t u4IeFtLen = 0;
	const uint8_t *pucIe = NULL;
	struct STA_RECORD *prStaRec = NULL;
	struct MSG_SAA_FT_CONTINUE *prFtContinueMsg = NULL;
	uint8_t ucBssIndex = 0, ucR0R1 = 0;


	if (!pvSetBuffer || u4SetBufferLen == 0) {
		DBGLOG(OID, ERROR,
		       "FT: pvSetBuffer is Null %d, Buffer Len %u\n",
		       !pvSetBuffer, u4SetBufferLen);
		return WLAN_STATUS_INVALID_DATA;
	}

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	prStaRec = aisGetTargetStaRec(prAdapter, ucBssIndex);
	if (!prStaRec) {
		DBGLOG(OID, WARN, "FT: invalid StaRec\n");
		return WLAN_STATUS_SUCCESS;
	}

	kalGetFtIeParam(pvSetBuffer, &u2MD, &u4IeFtLen, &pucIe);
	/*
	 * state = STA_STATE_3: update R0 for next auth frame
	 * state = STA_STATE_1: update R1 for upcoming assoc frame
	 */
	ucR0R1 = prStaRec->ucStaState == STA_STATE_1 ? AIS_FT_R1 : AIS_FT_R0;

	DBGLOG(OID, INFO, "FT: STA state:%d for %s\n",
		prStaRec->ucStaState, ucR0R1 == AIS_FT_R0 ? "R0" : "R1");

	prFtIes = aisGetFtIe(prAdapter, ucBssIndex, ucR0R1);
	if (!prFtIes) {
		DBGLOG(OID, ERROR, "FT: bss%d is not ais\n", ucBssIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u4IeFtLen == 0) {
		DBGLOG(OID, WARN, "FT: FT Ies length is 0\n");
		return WLAN_STATUS_SUCCESS;
	}
	if (prFtIes->u4IeLength != u4IeFtLen) {
		kalMemFree(prFtIes->pucIEBuf, VIR_MEM_TYPE,
			   prFtIes->u4IeLength);
		prFtIes->pucIEBuf = kalMemAlloc(u4IeFtLen, VIR_MEM_TYPE);
		prFtIes->u4IeLength = u4IeFtLen;
	}

	if (!prFtIes->pucIEBuf) {
		DBGLOG(OID, ERROR,
		       "FT: prFtIes->pucIEBuf memory allocation failed, ft ie_len=%u\n",
		       u4IeFtLen);
		return WLAN_STATUS_FAILURE;
	}
	pucIEStart = prFtIes->pucIEBuf;
	u4IeLen = prFtIes->u4IeLength;
	prFtIes->u2MDID = u2MD;
	prFtIes->prFTIE = NULL;
	prFtIes->prMDIE = NULL;
	prFtIes->prRsnIE = NULL;
	prFtIes->prTIE = NULL;

	if (u4IeLen)
		kalMemCopy(pucIEStart, pucIe, u4IeLen);

	while (u4IeLen >= 2) {
		uint32_t u4InfoElemLen = IE_SIZE(pucIEStart);

		if (u4InfoElemLen > u4IeLen)
			break;
		switch (pucIEStart[0]) {
		case ELEM_ID_MOBILITY_DOMAIN:
			prFtIes->prMDIE =
				(struct IE_MOBILITY_DOMAIN *)pucIEStart;
			break;
		case ELEM_ID_FAST_TRANSITION:
			prFtIes->prFTIE =
				(struct IE_FAST_TRANSITION *)pucIEStart;
			break;
		case ELEM_ID_RESOURCE_INFO_CONTAINER:
			break;
		case ELEM_ID_TIMEOUT_INTERVAL:
			prFtIes->prTIE =
				(struct IE_TIMEOUT_INTERVAL *)pucIEStart;
			break;
		case ELEM_ID_RSN:
			prFtIes->prRsnIE = (struct RSN_INFO_ELEM *)pucIEStart;
			break;
		}
		u4IeLen -= u4InfoElemLen;
		pucIEStart += u4InfoElemLen;
	}

	DBGLOG(OID, INFO,
	       "FT: MdId 0x%x IesLen %u, MDIE %d FTIE %d RSN %d TIE %d\n",
	       u2MD, prFtIes->u4IeLength, !!prFtIes->prMDIE,
	       !!prFtIes->prFTIE, !!prFtIes->prRsnIE, !!prFtIes->prTIE);

	/* check if SAA is waiting to send Reassoc req */
	if (prStaRec->ucAuthTranNum != AUTH_TRANSACTION_SEQ_2 ||
		!prStaRec->fgIsReAssoc || prStaRec->ucStaState != STA_STATE_1)
		return WLAN_STATUS_SUCCESS;

	prFtContinueMsg = (struct MSG_SAA_FT_CONTINUE *)cnmMemAlloc(
		prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_SAA_FT_CONTINUE));
	if (!prFtContinueMsg) {
		DBGLOG(OID, WARN, "FT: failed to allocate Join Req Msg\n");
		return WLAN_STATUS_FAILURE;
	}
	prFtContinueMsg->rMsgHdr.eMsgId = MID_OID_SAA_FSM_CONTINUE;
	prFtContinueMsg->prStaRec = prStaRec;
	/* We don't support resource request protocol */
	prFtContinueMsg->fgFTRicRequest = FALSE;
	DBGLOG(OID, INFO, "FT: continue to do auth/assoc\n");
	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prFtContinueMsg,
		    MSG_SEND_METHOD_BUF);
	return WLAN_STATUS_SUCCESS;
}

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
uint32_t wlanoidSetMonitor(struct ADAPTER *prAdapter,
		  void *pvSetBuffer, uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo;
	struct CMD_MONITOR_SET_INFO *prCmdMonitor;

	prCmdMonitor = kalMemAlloc(
		sizeof(struct CMD_MONITOR_SET_INFO), VIR_MEM_TYPE);
	if (!prCmdMonitor) {
		log_dbg(OID, ERROR, "alloc CmdMonitor fail\n");
		return WLAN_STATUS_FAILURE;
	}

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;

	DBGLOG(REQ, INFO,
		"en[%d],bn[%d],pc[%d],sco[%d],bw[%d],cc1[%d],cc2[%d],bidx[%d],aid[%d],fcs[%d]\n",
		prGlueInfo->fgIsEnableMon,
		prGlueInfo->ucBand,
		prGlueInfo->ucPriChannel,
		prGlueInfo->ucSco,
		prGlueInfo->ucChannelWidth,
		prGlueInfo->ucChannelS1,
		prGlueInfo->ucChannelS2,
		prGlueInfo->ucBandIdx,
		prGlueInfo->u2Aid,
		prGlueInfo->fgDropFcsErrorFrame);

	if (prGlueInfo->ucBandIdx < CFG_MONITOR_BAND_NUM) {
		prGlueInfo->aucBandIdxEn[prGlueInfo->ucBandIdx] =
			prGlueInfo->fgIsEnableMon;
	}

	prCmdMonitor->ucEnable = prGlueInfo->fgIsEnableMon;
	prCmdMonitor->ucBand = prGlueInfo->ucBand;
	prCmdMonitor->ucPriChannel = prGlueInfo->ucPriChannel;
	prCmdMonitor->ucSco = prGlueInfo->ucSco;
	prCmdMonitor->ucChannelWidth = prGlueInfo->ucChannelWidth;
	prCmdMonitor->ucChannelS1 = prGlueInfo->ucChannelS1;
	prCmdMonitor->ucChannelS2 = prGlueInfo->ucChannelS2;
	prCmdMonitor->ucBandIdx = prGlueInfo->ucBandIdx;
	prCmdMonitor->u2Aid = prGlueInfo->u2Aid;
	prCmdMonitor->fgDropFcsErrorFrame = prGlueInfo->fgDropFcsErrorFrame;

	rStatus = wlanSendSetQueryCmd(prAdapter,
		CMD_ID_SET_MONITOR,
		TRUE,
		FALSE,
		TRUE,
		nicCmdEventSetCommon,
		nicOidCmdTimeoutCommon,
		sizeof(struct CMD_MONITOR_SET_INFO),
		(uint8_t *)prCmdMonitor, pvSetBuffer, u4SetBufferLen);
#if CFG_SUPPORT_TX_BEACON_STA_MODE
	if (prCmdMonitor->ucEnable != 0)
		nicActivateNetwork(prAdapter, 0);
	else
		nicDeactivateNetwork(prAdapter, 0);
#endif
	kalMemFree(prCmdMonitor, VIR_MEM_TYPE, sizeof(struct CMD_MONITOR_SET_INFO));

	return rStatus;
}
#endif

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to send EPCS frames (REQ, RSP, TEARDOWN)
 *        from oid
 *
 * \param[in]  prAdapter       A pointer to the Adapter structure.
 * \param[in]  pvSetBuffer     A pointer to the buffer that holds the
 *                             OID-specific data to be set.
 * \param[in]  u4SetBufferLen  The number of bytes the set buffer.
 * \param[out] pu4SetInfoLen   Points to the number of bytes it read or is
 *                             needed
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanoidSendEpcs(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen)
{
	struct BSS_INFO *prBssInfo = NULL;
	struct WIFI_VAR *prWifiVar;
	uint8_t ucBssIndex = 0;
	uint8_t ucAction = 0;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;

	prWifiVar = &prAdapter->rWifiVar;
	if (IS_FEATURE_DISABLED(prWifiVar->fgEnEpcs))
		return WLAN_STATUS_FAILURE;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return WLAN_STATUS_INVALID_DATA;
	if (prBssInfo->eConnectionState != MEDIA_STATE_CONNECTED) {
		DBGLOG(OID, ERROR, "didn't connected any Access Point\n");
		return WLAN_STATUS_FAILURE;
	}
	if (u4SetBufferLen == 0 || !pvSetBuffer)
		return WLAN_STATUS_INVALID_LENGTH;

	ucAction = hexDigitToInt(*(uint8_t *)pvSetBuffer);
	switch (ucAction) {
	case EPCS_ENABLE_REQUEST:
	case EPCS_ENABLE_RESPONSE:
	case EPCS_TEARDOWN:
		DBGLOG(OID, INFO, "Send EPCS, action=%u\n", ucAction);
		epcsSend(prAdapter, ucAction, prBssInfo);
		break;
	default:
		DBGLOG(OID, ERROR, "action invalid %u\n", ucAction);
		return WLAN_STATUS_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_802_11BE_EPCS */

uint32_t wlanoidSendNeighborRequest(struct ADAPTER *prAdapter,
				    void *pvSetBuffer, uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	struct SUB_ELEMENT_LIST *prSSIDIE = NULL;
	struct BSS_INFO *prAisBssInfo = NULL;
	uint8_t ucSSIDIELen = 0;
	uint8_t *pucSSID;
	uint8_t ucBssIndex = 0;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
	if (!prAisBssInfo)
		return WLAN_STATUS_INVALID_DATA;
	if (prAisBssInfo->eConnectionState != MEDIA_STATE_CONNECTED) {
		DBGLOG(OID, ERROR, "didn't connected any Access Point\n");
		return WLAN_STATUS_FAILURE;
	}
	if (u4SetBufferLen == 0 || !pvSetBuffer) {
		rrmTxNeighborReportRequest(prAdapter,
					   prAisBssInfo->prStaRecOfAP, NULL);
		return WLAN_STATUS_SUCCESS;
	}

	pucSSID = (uint8_t *)pvSetBuffer;
	ucSSIDIELen = (uint8_t)(u4SetBufferLen + sizeof(*prSSIDIE));
	prSSIDIE = kalMemAlloc(ucSSIDIELen, PHY_MEM_TYPE);
	if (!prSSIDIE) {
		DBGLOG(OID, ERROR, "No Memory\n");
		return WLAN_STATUS_FAILURE;
	}
	prSSIDIE->prNext = NULL;
	prSSIDIE->rSubIE.ucSubID = ELEM_ID_SSID;
	prSSIDIE->rSubIE.ucLength = (uint8_t)u4SetBufferLen;
	kalMemCopy(&prSSIDIE->rSubIE.aucOptInfo[0], pucSSID,
		   (uint8_t)u4SetBufferLen);
	DBGLOG(OID, INFO, "Send Neighbor Request, SSID=%s\n", HIDE(pucSSID));
	rrmTxNeighborReportRequest(prAdapter, prAisBssInfo->prStaRecOfAP,
				   prSSIDIE);
	kalMemFree(prSSIDIE, PHY_MEM_TYPE, ucSSIDIELen);
	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidSendBTMQuery(struct ADAPTER *prAdapter, void *pvSetBuffer,
			     uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct STA_RECORD *prStaRec = NULL;
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = 0;
	int32_t u4Ret = 0;
	uint8_t ucQueryReason = BSS_TRANSITION_LOW_RSSI;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	if (!prAisBssInfo ||
	    prAisBssInfo->eConnectionState !=
		MEDIA_STATE_CONNECTED) {
		DBGLOG(OID, INFO, "Not connected yet\n");
		return WLAN_STATUS_FAILURE;
	}
	prStaRec = prAisBssInfo->prStaRecOfAP;
	if (!prStaRec) {
		DBGLOG(OID, INFO, "No target starec\n");
		return WLAN_STATUS_FAILURE;
	}
	if (pvSetBuffer) {
		u4Ret = kalkStrtou8(pvSetBuffer, 0, &ucQueryReason);
		if (u4Ret)
			DBGLOG(OID, WARN, "parse reason u4Ret=%d\n", u4Ret);
	}

	wnmSendBTMQueryFrame(prAdapter, prStaRec, ucQueryReason);
	DBGLOG(OID, INFO, "Send BTM Query, Reason %d\n", ucQueryReason);

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidTspecOperation(struct ADAPTER *prAdapter, void *pvBuffer,
			       uint32_t u4BufferLen, uint32_t *pu4InfoLen)
{
	struct PARAM_QOS_TSPEC *prTspecParam = NULL;
	struct MSG_TS_OPERATE *prMsgTsOperate = NULL;
	uint8_t *pucCmd;
	char *pucSavedPtr = NULL;
	uint8_t *pucItem = NULL;
	uint32_t u4Ret = 1;
	uint8_t ucApsdSetting = 2; /* 0: legacy; 1: u-apsd; 2: not set yet */
	uint8_t ucBssIndex = 0;
	enum TSPEC_OP_CODE eTsOp;
	struct BSS_INFO *prAisBssInfo;

#if !CFG_SUPPORT_WMM_AC
	DBGLOG(OID, INFO, "WMM AC is not supported\n");
	return WLAN_STATUS_FAILURE;
#endif

	if (!pvBuffer || !u4BufferLen) {
		DBGLOG(OID, ERROR, "pvBuffer is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	pucCmd = (uint8_t *)pvBuffer;
	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	if (kalStrniCmp(pucCmd, "dumpts", 6) == 0) {
		*pu4InfoLen = kalSnprintf(pucCmd, u4BufferLen, "%s",
					  "\nAll Active Tspecs:\n");
		u4BufferLen -= *pu4InfoLen;
		pucCmd += *pu4InfoLen;
		*pu4InfoLen +=
			wmmDumpActiveTspecs(prAdapter, pucCmd, u4BufferLen,
			ucBssIndex);
		return WLAN_STATUS_SUCCESS;
	}

	if (kalStrniCmp(pucCmd, "addts", 5) == 0)
		eTsOp = TX_ADDTS_REQ;
	else if (kalStrniCmp(pucCmd, "delts", 5) == 0)
		eTsOp = TX_DELTS_REQ;
	else {
		DBGLOG(OID, INFO, "wrong operation %s\n", pucCmd);
		return WLAN_STATUS_FAILURE;
	}
	/* addts token n,tid n,dir n,psb n,up n,fixed n,size n,maxsize
	** n,maxsrvint n, minsrvint n,
	** inact n, suspension n, srvstarttime n, minrate n,meanrate n,peakrate
	** n,burst n,delaybound n,
	** phyrate n,SBA n,mediumtime n
	*/
	prMsgTsOperate = (struct MSG_TS_OPERATE *)cnmMemAlloc(
		prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_TS_OPERATE));
	if (!prMsgTsOperate)
		return WLAN_STATUS_FAILURE;

	kalMemZero(prMsgTsOperate, sizeof(struct MSG_TS_OPERATE));
	prMsgTsOperate->rMsgHdr.eMsgId = MID_OID_WMM_TSPEC_OPERATE;
	prMsgTsOperate->eOpCode = eTsOp;
	prTspecParam = &prMsgTsOperate->rTspecParam;
	pucCmd += 6;
	pucItem = kalStrtokR(pucCmd, ",", &pucSavedPtr);
	while (pucItem) {
		if (kalStrniCmp(pucItem, "token ", 6) == 0)
			u4Ret = kalkStrtou8(pucItem + 6, 0,
					 &prTspecParam->ucDialogToken);
		else if (kalStrniCmp(pucItem, "tid ", 4) == 0) {
			u4Ret = kalkStrtou8(pucItem + 4, 0,
					 &prMsgTsOperate->ucTid);
			prTspecParam->rTsInfo.ucTid = prMsgTsOperate->ucTid;
		} else if (kalStrniCmp(pucItem, "dir ", 4) == 0)
			u4Ret = kalkStrtou8(pucItem + 4, 0,
					 &prTspecParam->rTsInfo.ucDirection);
		else if (kalStrniCmp(pucItem, "psb ", 4) == 0)
			u4Ret = kalkStrtou8(pucItem+4, 0, &ucApsdSetting);
		else if (kalStrniCmp(pucItem, "up ", 3) == 0)
			u4Ret = kalkStrtou8(pucItem + 3, 0,
					 &prTspecParam->rTsInfo.ucuserPriority);
		else if (kalStrniCmp(pucItem, "size ", 5) == 0) {
			uint16_t u2Size = 0;

			u4Ret = kalkStrtou16(pucItem+5, 0, &u2Size);
			prTspecParam->u2NominalMSDUSize |= u2Size;
		} else if (kalStrniCmp(pucItem, "fixed ", 6) == 0) {
			uint8_t ucFixed = 0;

			u4Ret = kalkStrtou8(pucItem+6, 0, &ucFixed);
			if (ucFixed)
				prTspecParam->u2NominalMSDUSize |= BIT(15);
		} else if (kalStrniCmp(pucItem, "maxsize ", 8) == 0)
			u4Ret = kalkStrtou16(pucItem + 8, 0,
					  &prTspecParam->u2MaxMSDUsize);
		else if (kalStrniCmp(pucItem, "maxsrvint ", 10) == 0)
			u4Ret = kalkStrtou32(pucItem + 10, 0,
					     &prTspecParam->u4MaxSvcIntv);
		else if (kalStrniCmp(pucItem, "minsrvint ", 10) == 0)
			u4Ret = kalkStrtou32(pucItem + 10, 0,
					     &prTspecParam->u4MinSvcIntv);
		else if (kalStrniCmp(pucItem, "inact ", 6) == 0)
			u4Ret = kalkStrtou32(pucItem + 6, 0,
					     &prTspecParam->u4InactIntv);
		else if (kalStrniCmp(pucItem, "suspension ", 11) == 0)
			u4Ret = kalkStrtou32(pucItem + 11, 0,
					     &prTspecParam->u4SpsIntv);
		else if (kalStrniCmp(pucItem, "srvstarttime ", 13) == 0)
			u4Ret = kalkStrtou32(pucItem + 13, 0,
					     &prTspecParam->u4SvcStartTime);
		else if (kalStrniCmp(pucItem, "minrate ", 8) == 0)
			u4Ret = kalkStrtou32(pucItem + 8, 0,
					     &prTspecParam->u4MinDataRate);
		else if (kalStrniCmp(pucItem, "meanrate ", 9) == 0)
			u4Ret = kalkStrtou32(pucItem + 9, 0,
					     &prTspecParam->u4MeanDataRate);
		else if (kalStrniCmp(pucItem, "peakrate ", 9) == 0)
			u4Ret = kalkStrtou32(pucItem + 9, 0,
					     &prTspecParam->u4PeakDataRate);
		else if (kalStrniCmp(pucItem, "burst ", 6) == 0)
			u4Ret = kalkStrtou32(pucItem + 6, 0,
					     &prTspecParam->u4MaxBurstSize);
		else if (kalStrniCmp(pucItem, "delaybound ", 11) == 0)
			u4Ret = kalkStrtou32(pucItem + 11, 0,
					     &prTspecParam->u4DelayBound);
		else if (kalStrniCmp(pucItem, "phyrate ", 8) == 0)
			u4Ret = kalkStrtou32(pucItem + 8, 0,
					     &prTspecParam->u4MinPHYRate);
		else if (kalStrniCmp(pucItem, "sba ", 4) == 0)
			u4Ret = wlanDecimalStr2Hexadecimals(
				pucItem + 4, &prTspecParam->u2Sba);
		else if (kalStrniCmp(pucItem, "mediumtime ", 11) == 0)
			u4Ret = kalkStrtou16(pucItem + 11, 0,
					  &prTspecParam->u2MediumTime);

		if (u4Ret) {
			DBGLOG(OID, ERROR, "Parse %s error\n", pucItem);
			cnmMemFree(prAdapter, prMsgTsOperate);
			return WLAN_STATUS_FAILURE;
		}
		pucItem = kalStrtokR(NULL, ",", &pucSavedPtr);
	}
	/* if APSD is not set in addts request, use global wmmps settings */
	prAisBssInfo =
		aisGetAisBssInfo(prAdapter, ucBssIndex);
	if (!prAisBssInfo)
		DBGLOG(OID, ERROR, "AisBssInfo is NULL!\n");
	else if (ucApsdSetting == 2) {
		struct PM_PROFILE_SETUP_INFO *prPmProf = NULL;
		enum ENUM_ACI eAc;

		if (prTspecParam->rTsInfo.ucuserPriority
			>= (ARRAY_SIZE(aucUp2ACIMap))) {
			DBGLOG(OID, WARN,
				"userPriority=%d is invalid, resest to 0\n",
				prTspecParam->rTsInfo.ucuserPriority);
			prTspecParam->rTsInfo.ucuserPriority = 0;
		}

		eAc = aucUp2ACIMap[prTspecParam->rTsInfo.ucuserPriority];

		prPmProf = &prAisBssInfo->rPmProfSetupInfo;
		switch (prTspecParam->rTsInfo.ucDirection) {
		case UPLINK_TS: /* UpLink*/
			if (prPmProf->ucBmpTriggerAC & BIT(eAc))
				prTspecParam->rTsInfo.ucApsd = 1;
			break;
		case DOWNLINK_TS:/* DownLink */
			if (prPmProf->ucBmpDeliveryAC & BIT(eAc))
				prTspecParam->rTsInfo.ucApsd = 1;
			break;
		case BI_DIR_TS: /* Bi-directional */
			if ((prPmProf->ucBmpTriggerAC & BIT(eAc)) &&
				(prPmProf->ucBmpDeliveryAC & BIT(eAc)))
				prTspecParam->rTsInfo.ucApsd = 1;
			break;
		}
	} else
		prTspecParam->rTsInfo.ucApsd = ucApsdSetting;
	*(--pucCmd) = 0;
	pucCmd -= 5;
	DBGLOG(OID, INFO,
	       "%d: %s %d %d %d %d %d %d %d %u %u %u %u %u %u %u %u %u %u %u 0x%04x %d\n",
	       ucBssIndex,
	       pucCmd, prTspecParam->ucDialogToken, prTspecParam->rTsInfo.ucTid,
	       prTspecParam->rTsInfo.ucDirection, prTspecParam->rTsInfo.ucApsd,
	       prTspecParam->rTsInfo.ucuserPriority,
	       prTspecParam->u2NominalMSDUSize, prTspecParam->u2MaxMSDUsize,
	       prTspecParam->u4MaxSvcIntv, prTspecParam->u4MinSvcIntv,
	       prTspecParam->u4InactIntv, prTspecParam->u4SpsIntv,
	       prTspecParam->u4SvcStartTime, prTspecParam->u4MinDataRate,
	       prTspecParam->u4MeanDataRate, prTspecParam->u4PeakDataRate,
	       prTspecParam->u4MaxBurstSize, prTspecParam->u4DelayBound,
	       prTspecParam->u4MinPHYRate, prTspecParam->u2Sba,
	       prTspecParam->u2MediumTime);
	prMsgTsOperate->ucBssIdx = ucBssIndex;
	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prMsgTsOperate,
		    MSG_SEND_METHOD_BUF);
	return WLAN_STATUS_SUCCESS;
}

/* It's a Integretion Test function for RadioMeasurement. If you found errors
** during doing Radio Measurement,
** you can run this IT function with iwpriv wlan0 driver \"RM-IT
** xx,xx,xx, xx\"
** xx,xx,xx,xx is the RM request frame data
*/
uint32_t wlanoidPktProcessIT(struct ADAPTER *prAdapter, void *pvBuffer,
			     uint32_t u4BufferLen, uint32_t *pu4InfoLen)
{

	char *pucSavedPtr;
	uint8_t ucBssIndex = 0;
	struct BSS_INFO *ais;
	struct AIS_SPECIFIC_BSS_INFO *aiss = NULL;
	struct LINK *ess = NULL;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	ais = aisGetAisBssInfo(prAdapter, ucBssIndex);
	aiss = aisGetAisSpecBssInfo(prAdapter, ucBssIndex);
	ess = &aiss->rCurEssLink;

	if (!pvBuffer || !u4BufferLen) {
		DBGLOG(OID, ERROR, "pvBuffer is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	pucSavedPtr = (int8_t *)pvBuffer;

	if (!kalStrniCmp(pucSavedPtr, "RM-IT", 5)) {
		pucSavedPtr += 5;
	} else if (!kalStrniCmp(pucSavedPtr, "BTM-IT", 6)) {
#if (CFG_SUPPORT_CONNAC3X == 0)
		static uint8_t aucPacket[500] = {0,};
		struct SW_RFB rSwRfb;
		struct BSS_DESC *target;
		uint8_t *pos = NULL;
		int32_t i4Ret = 0;
		struct BSS_DESC *bssDesc;
		struct ACTION_BTM_REQ_FRAME *rxframe = NULL;
		int32_t i4Argc = 0;
		int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};

		/*
		 * BTM-IT 0x7 200 220 5
		 * 0x07: request mode
		 * 200: disassoc timer, which is timer x beacon interval (ms)
		 * 220: preference for sending btm AP
		 * 5: diff to decrease preference for each candidate
		 */
		DBGLOG(INIT, INFO, "BTM command is [%s]\n", pucSavedPtr);
		wlanCfgParseArgument(pucSavedPtr, &i4Argc, apcArgv);
		target = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		if (!target) {
			DBGLOG(OID, INFO, "sta is not connected!!!\n");
			return WLAN_STATUS_FAILURE;
		}

		kalMemZero(aucPacket, sizeof(aucPacket));
		kalMemZero(&rSwRfb, sizeof(rSwRfb));
		rSwRfb.pvHeader = (void *)&aucPacket[0];
		rSwRfb.u2PacketLen = sizeof(struct ACTION_BTM_REQ_FRAME);
		rSwRfb.u2HeaderLen = WLAN_MAC_MGMT_HEADER_LEN;
		rSwRfb.ucStaRecIdx = KAL_NETWORK_TYPE_AIS_INDEX;

		rxframe = (struct ACTION_BTM_REQ_FRAME *) rSwRfb.pvHeader;
		COPY_MAC_ADDR(rxframe->aucDestAddr, ais->aucOwnMacAddr);
		COPY_MAC_ADDR(rxframe->aucSrcAddr, target->aucBSSID);
		COPY_MAC_ADDR(rxframe->aucBSSID, target->aucBSSID);

		rxframe->ucAction = ACTION_WNM_BSS_TRANSITION_MANAGEMENT_REQ;
		rxframe->ucDialogToken = 111;
		rxframe->ucRequestMode = 0;
		rxframe->u2DisassocTimer = 600;
		rxframe->ucValidityInterval = 255;

		if (i4Argc > 1) {
			i4Ret = kalkStrtou8(
				apcArgv[1], 0, &rxframe->ucRequestMode);
			DBGLOG(OID, TRACE,
				"parse ucRequestMode error i4Ret=%d\n",
				i4Ret);
		}
		if (i4Argc > 2) {
			i4Ret = kalkStrtou16(
				apcArgv[2], 0, &rxframe->u2DisassocTimer);
			DBGLOG(OID, TRACE,
				"parse u2DisassocTimer error i4Ret=%d\n",
				i4Ret);
		}

		pos = aucPacket + sizeof(struct ACTION_BTM_REQ_FRAME);

		/*
		 * WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED BIT(0)
		 * WNM_BSS_TM_REQ_ABRIDGED BIT(1)
		 * WNM_BSS_TM_REQ_DISASSOC_IMMINENT BIT(2)
		 * WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED BIT(3)
		 * WNM_BSS_TM_REQ_ESS_DISASSOC_IMMINENT BIT(4)
		 */
		if (rxframe->ucRequestMode &
				WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED) {
			int32_t pref = 255;
			struct IE_NEIGHBOR_REPORT *neig = NULL;
			uint8_t diff = 30;
			uint8_t targetPref = 255;
			uint8_t len = sizeof(struct IE_NEIGHBOR_REPORT) -
				ELEM_HDR_LEN + 3;

			if (i4Argc > 3) {
				i4Ret = kalkStrtou8(
					apcArgv[3], 0, &targetPref);
				DBGLOG(OID, TRACE,
					"parse targetPref error i4Ret=%d\n",
					i4Ret);
			}
			if (i4Argc > 4) {
				i4Ret = kalkStrtou8(
					apcArgv[4], 0, &diff);
				DBGLOG(OID, TRACE,
					"parse diff error i4Ret=%d\n",
					i4Ret);
			}

			neig = (struct IE_NEIGHBOR_REPORT *) pos;
			pos += sizeof(struct IE_NEIGHBOR_REPORT);
			neig->ucId = ELEM_ID_NEIGHBOR_REPORT;
			neig->ucLength = len;
			COPY_MAC_ADDR(neig->aucBSSID,
				target->aucBSSID);

			if (target->eBand == BAND_5G) {
				WLAN_SET_FIELD_32(&neig->u4BSSIDInfo,
					0x1c9b);
			} else {
				WLAN_SET_FIELD_32(&neig->u4BSSIDInfo,
					0x0c9b);
			}
			neig->ucChnlNumber = target->ucChannelNum;
			neig->ucPhyType = 0x9;

			/* bss transition candidate preference */
			*pos++ = 3;
			*pos++ = 1;
			*pos++ = targetPref;

			rSwRfb.u2PacketLen += neig->ucLength;

			LINK_FOR_EACH_ENTRY(bssDesc, ess,
				rLinkEntryEss[ucBssIndex], struct BSS_DESC) {

				if (EQUAL_MAC_ADDR(target->aucBSSID,
				    bssDesc->aucBSSID))
					continue;

				if (rSwRfb.u2PacketLen + len >
				    sizeof(aucPacket))
					break;

				neig = (struct IE_NEIGHBOR_REPORT *) pos;
				pos += sizeof(struct IE_NEIGHBOR_REPORT);
				neig->ucId = ELEM_ID_NEIGHBOR_REPORT;
				neig->ucLength = len;
				COPY_MAC_ADDR(neig->aucBSSID,
					bssDesc->aucBSSID);

				if (bssDesc->eBand == BAND_5G) {
					WLAN_SET_FIELD_32(&neig->u4BSSIDInfo,
						0x1c9b);
				} else {
					WLAN_SET_FIELD_32(&neig->u4BSSIDInfo,
						0x0c9b);
				}
				neig->ucChnlNumber = bssDesc->ucChannelNum;
				neig->ucPhyType = 0x9;

				/* bss transition candidate preference */
				*pos++ = 3;
				*pos++ = 1;
				*pos++ = pref > 0 ? (uint8_t) pref : 0;
				pref -= diff;

				rSwRfb.u2PacketLen += pos - (uint8_t *) neig;
			}
		}

		dumpMemory8(rSwRfb.pvHeader, rSwRfb.u2PacketLen);

		wnmWNMAction(prAdapter, &rSwRfb);
		return WLAN_STATUS_SUCCESS;
#else
		DBGLOG(OID, INFO, "connac3 doesn't support!!!\n");
		return WLAN_STATUS_FAILURE;
#endif
	} else if (!kalStrniCmp(pucSavedPtr, "BT-IT", 5)) {
		DBGLOG(OID, INFO, "Simulate beacon timeout!!!\n");
		aisBssBeaconTimeout(prAdapter, ucBssIndex);
		return WLAN_STATUS_SUCCESS;
	} else {
		pucSavedPtr[10] = 0;
		DBGLOG(OID, ERROR, "IT type %s is not supported\n",
		       pucSavedPtr);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	return WLAN_STATUS_SUCCESS;
}

/* Firmware Integration Test functions
** This function receives commands that are input by a firmware IT test script
** By using IT test script, RD no need to run IT with a real Access Point
** For example: iwpriv wlan0 driver \"Fw-Event Roaming ....\"
*/
uint32_t wlanoidFwEventIT(struct ADAPTER *prAdapter, void *pvBuffer,
			  uint32_t u4BufferLen, uint32_t *pu4InfoLen)
{
	uint8_t *pucCmd;
	uint8_t ucBssIndex = 0;

	if (!pvBuffer || !u4BufferLen) {
		DBGLOG(OID, ERROR, "pvBuffer is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	pucCmd = (uint8_t *)pvBuffer;
	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	/*
	 * Firmware roaming Integration Test case
	 * Roaming 1 2 3 (parameter is optional)
	 * Parameter 1: Scan type, 0: normal, 1: partial only, 2: full only
	 * Parameter 2: Scan count, 0, normal, N: scan at most N times
	 * Parameter 3: Scan mode, 0: normal, 1: low latency scan
	 */
	if (!kalStrniCmp(pucCmd, "Roaming", 7)) {
#if CFG_SUPPORT_ROAMING
		struct CMD_ROAMING_TRANSIT rTransit = {0};
		struct BSS_DESC *prBssDesc =
			aisGetTargetBssDesc(prAdapter, ucBssIndex);
		int32_t i4Argc = 0;
		int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
		struct ROAMING_INFO *prRoamingFsmInfo =
			aisGetRoamingInfo(prAdapter, ucBssIndex);
		struct AIS_FSM_INFO *prAisFsmInfo =
			aisGetAisFsmInfo(prAdapter, ucBssIndex);

		DBGLOG(OID, INFO, "FW event is [%s]\n", pucCmd);
		wlanCfgParseArgument(pucCmd, &i4Argc, apcArgv);

		if (i4Argc > 1 && i4Argc != 4) {
			DBGLOG(OID, ERROR,
				"Unexpected argument counts %d\n", i4Argc);
			return WLAN_STATUS_SUCCESS;
		} else if (i4Argc == 4) {
			kalkStrtou8(apcArgv[1], 0,
				&prRoamingFsmInfo->rRoamScanParam.ucScanType);
			kalkStrtou8(apcArgv[2], 0,
				&prRoamingFsmInfo->rRoamScanParam.ucScanCount);
			kalkStrtou8(apcArgv[3], 0,
				&prRoamingFsmInfo->rRoamScanParam.ucScanMode);
			prAisFsmInfo->ucScanTrialCount = 0;
		}

		/* Check roaming FSM and CSA states*/
		if (!roamingFsmInDecision(prAdapter, FALSE, ucBssIndex)) {
			DBGLOG(OID, WARN,
				"Ignore FW-EVENT Roaming if not in decision or in CSA.\n");
			return WLAN_STATUS_SUCCESS;
		}

		if (prBssDesc)
			rTransit.u2Data = prBssDesc->ucRCPI;
		rTransit.u2Event = ROAMING_EVENT_DISCOVERY;
		rTransit.eReason = ROAMING_REASON_POOR_RCPI;
		rTransit.ucBssidx = ucBssIndex;
		roamingFsmRunEventDiscovery(prAdapter, &rTransit);
#endif
	} else {
		DBGLOG(OID, ERROR, "Not supported Fw Event IT type %s\n",
		       pucCmd);
		return WLAN_STATUS_FAILURE;
	}
	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidDumpUapsdSetting(struct ADAPTER *prAdapter, void *pvBuffer,
				 uint32_t u4BufferLen, uint32_t *pu4InfoLen)
{
	uint8_t *pucCmd;
	uint8_t ucFinalSetting = 0;
	uint8_t ucStaticSetting = 0;
	struct PM_PROFILE_SETUP_INFO *prPmProf = NULL;
	struct BSS_INFO *prAisBssInfo;
	uint8_t ucBssIndex = 0;

	if (!pvBuffer) {
		DBGLOG(OID, ERROR, "pvBuffer is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	pucCmd = (uint8_t *)pvBuffer;
	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	prAisBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);

	if (!prAisBssInfo)
		return WLAN_STATUS_FAILURE;
	prPmProf = &prAisBssInfo->rPmProfSetupInfo;
	ucStaticSetting =
		(prPmProf->ucBmpDeliveryAC << 4) | prPmProf->ucBmpTriggerAC;
	ucFinalSetting = wmmCalculateUapsdSetting(prAdapter, ucBssIndex);
	*pu4InfoLen = kalSnprintf(
		pucCmd, u4BufferLen,
		"\nStatic Uapsd Setting:0x%02x\nFinal Uapsd Setting:0x%02x",
		ucStaticSetting, ucFinalSetting);
	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_OSHARE
uint32_t
wlanoidSetOshareMode(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen) {
	if (!prAdapter || !pvSetBuffer)
		return WLAN_STATUS_INVALID_DATA;

	DBGLOG(OID, TRACE, "wlanoidSetOshareMode\n");

	if (u4SetBufferLen < sizeof(struct OSHARE_MODE_T)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	return wlanSendSetQueryCmd(prAdapter, /* prAdapter */
			   CMD_ID_SET_OSHARE_MODE, /* ucCID */
			   TRUE, /* fgSetQuery */
			   FALSE, /* fgNeedResp */
			   TRUE, /* fgIsOid */
			   nicCmdEventSetCommon, /* pfCmdDoneHandler*/
			   nicOidCmdTimeoutCommon, /* pfCmdTimeoutHandler */
			   u4SetBufferLen, /* u4SetQueryInfoLen */
			   (uint8_t *) pvSetBuffer,/* pucInfoBuffer */
			   NULL, /* pvSetQueryBuffer */
			   0); /* u4SetQueryBufferLen */
}
#endif

uint32_t
wlanoidQueryWifiLogLevelSupport(struct ADAPTER *prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen) {
	struct PARAM_WIFI_LOG_LEVEL_UI *pparam;

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen < sizeof(struct PARAM_WIFI_LOG_LEVEL_UI)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	pparam = (struct PARAM_WIFI_LOG_LEVEL_UI *) pvQueryBuffer;
	pparam->u4Enable = wlanDbgLevelUiSupport(prAdapter,
			   pparam->u4Version, pparam->u4Module);

	DBGLOG(OID, INFO, "version: %d, module: %d, enable: %d\n",
	       pparam->u4Version,
	       pparam->u4Module,
	       pparam->u4Enable);

	*pu4QueryInfoLen = sizeof(struct PARAM_WIFI_LOG_LEVEL_UI);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidQueryWifiLogLevel(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen) {
	struct PARAM_WIFI_LOG_LEVEL *pparam;

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen < sizeof(struct PARAM_WIFI_LOG_LEVEL)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	pparam = (struct PARAM_WIFI_LOG_LEVEL *) pvQueryBuffer;
	pparam->u4Level = wlanDbgGetLogLevelImpl(prAdapter,
			  pparam->u4Version,
			  pparam->u4Module);

	DBGLOG(OID, INFO, "version: %d, module: %d, level: %d\n",
	       pparam->u4Version,
	       pparam->u4Module,
	       pparam->u4Level);

	*pu4QueryInfoLen = sizeof(struct PARAM_WIFI_LOG_LEVEL);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidSetWifiLogLevel(struct ADAPTER *prAdapter,
		       void *pvSetBuffer, uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen) {
	struct PARAM_WIFI_LOG_LEVEL *pparam;

	ASSERT(prAdapter);
	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(struct PARAM_WIFI_LOG_LEVEL)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	pparam = (struct PARAM_WIFI_LOG_LEVEL *) pvSetBuffer;

	DBGLOG(OID, INFO, "version: %d, module: %d, level: %d\n",
	       pparam->u4Version,
	       pparam->u4Module,
	       pparam->u4Level);

	wlanDbgSetLogLevelImpl(prAdapter,
			       pparam->u4Version,
			       pparam->u4Module,
			       pparam->u4Level);

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidSetDrvSer(struct ADAPTER *prAdapter,
			  void *pvSetBuffer,
			  uint32_t u4SetBufferLen,
			  uint32_t *pu4SetInfoLen)
{
	ASSERT(prAdapter);

	prAdapter->u4HifChkFlag |= HIF_DRV_SER;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);

	return 0;
}

uint32_t wlanoidSetAmsduNum(struct ADAPTER *prAdapter,
			    void *pvSetBuffer,
			    uint32_t u4SetBufferLen,
			    uint32_t *pu4SetInfoLen)
{
	struct mt66xx_chip_info *prChipInfo = NULL;

	ASSERT(prAdapter);
	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prChipInfo = prAdapter->chip_info;
	prChipInfo->ucMaxSwAmsduNum = (uint8_t)*((uint32_t *)pvSetBuffer);
	DBGLOG(OID, INFO, "Set SW AMSDU Num: %d\n",
	       prChipInfo->ucMaxSwAmsduNum);
	return 0;
}

uint32_t wlanoidSetAmsduSize(struct ADAPTER *prAdapter,
			     void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	uint32_t u4TxMaxAmsduInAmpduLen;

	ASSERT(prAdapter);
	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(uint32_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prChipInfo = prAdapter->chip_info;
	prWifiVar = &prAdapter->rWifiVar;
	u4TxMaxAmsduInAmpduLen = *((uint32_t *)pvSetBuffer);
	if (u4TxMaxAmsduInAmpduLen > WLAN_TX_MAX_AMSDU_IN_AMPDU_LEN) {
		DBGLOG(OID, WARN, "AMSDU max Size exceeds limit %d!",
		       WLAN_TX_MAX_AMSDU_IN_AMPDU_LEN);
		u4TxMaxAmsduInAmpduLen = WLAN_TX_MAX_AMSDU_IN_AMPDU_LEN;
	}
	prWifiVar->u4HtTxMaxAmsduInAmpduLen = u4TxMaxAmsduInAmpduLen;
	prWifiVar->u4VhtTxMaxAmsduInAmpduLen = u4TxMaxAmsduInAmpduLen;
	prWifiVar->u4TxMaxAmsduInAmpduLen = u4TxMaxAmsduInAmpduLen;

	DBGLOG(OID, INFO, "Set SW AMSDU max Size: %d\n",
	   prWifiVar->u4TxMaxAmsduInAmpduLen);
	return 0;
}

uint32_t
wlanoidShowPdmaInfo(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen)
{
	prAdapter->u4HifDbgFlag |= DEG_HIF_PDMA;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);

	return 0;
}

uint32_t
wlanoidShowPseInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen)
{
	prAdapter->u4HifDbgFlag |= DEG_HIF_PSE;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);

	return 0;
}

uint32_t
wlanoidShowPleInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen)
{
	prAdapter->u4HifDbgFlag |= DEG_HIF_PLE;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);

	return 0;
}

uint32_t
wlanoidShowCsrInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen)
{
	prAdapter->u4HifDbgFlag |= DEG_HIF_HOST_CSR;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);

	return 0;
}

uint32_t
wlanoidShowDmaschInfo(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
	prAdapter->u4HifDbgFlag |= DEG_HIF_DMASCH;
	kalSetHifDbgEvent(prAdapter->prGlueInfo);

	return 0;
}

uint32_t
wlanoidShowAhdbgInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen)
{
	char *pucSavedPtr;
	int32_t i4Argc = 0;
	int32_t i4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4BssIndex = 0;
	uint32_t u4Module = 0;
	uint32_t u4Reason = 0;
	struct CHIP_DBG_OPS *prDbgOps = prAdapter->chip_info->prDebugOps;

	if (!pvSetBuffer || !u4SetBufferLen) {
		DBGLOG(OID, ERROR, "pvBuffer is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	pucSavedPtr = (int8_t *)pvSetBuffer;
	DBGLOG(INIT, INFO, "AHDBG command is [%s]\n", pucSavedPtr);
	wlanCfgParseArgument(pucSavedPtr, &i4Argc, apcArgv);
	DBGLOG(INIT, INFO, "argc [%d]\n", i4Argc);

	if (i4Argc != 4) {
		DBGLOG(REQ, ERROR, "argc(%d) is error\n", i4Argc);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	i4Ret = kalkStrtou32(
		apcArgv[1], 0, &u4BssIndex);
	DBGLOG(OID, INFO,
		"parse u4BssIndex %u i4Ret=%d\n",
		u4BssIndex, i4Ret);

	i4Ret = kalkStrtou32(
		apcArgv[2], 0, &u4Module);
	DBGLOG(OID, INFO,
		"parse u4Module %u i4Ret=%d\n",
		u4Module, i4Ret);

	i4Ret = kalkStrtou32(
		apcArgv[3], 0, &u4Reason);
	DBGLOG(OID, INFO,
		"parse u4Reason %u i4Ret=%d\n",
		u4Reason, i4Ret);

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);

	if (prDbgOps && prDbgOps->setFwDebug) {
		/* trigger tx debug sop */
		prDbgOps->setFwDebug(
			prAdapter,
			true,
			0xffff,
			(u4Module << DBG_PLE_INT_MODULE_SHIFT) |
			(u4BssIndex << DBG_PLE_INT_BAND_BSS_SHIFT) |
			(1 << DBG_PLE_INT_VER_SHIFT) |
			(u4Reason)
			);
		DBGLOG(HAL, INFO, "Trigger Fw Debug SOP[%d][%d][%d]\n",
		       u4Module, u4BssIndex, u4Reason);
	}

	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	return WLAN_STATUS_SUCCESS;
}

#if (CFG_PCIE_GEN_SWITCH == 1)
uint32_t
wlanoidSetMddpGenSwitch(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen)
{
#if CFG_MTK_MDDP_SUPPORT
	char *pucSavedPtr;
	int32_t i4Argc = 0;
	int32_t i4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t u4MsgType = 0;
	struct mddpw_md_notify_info_t *prMdInfo;
	uint32_t u4genSwitchBuf[2] = {0};
	uint8_t *buff = NULL;
	uint32_t u4seq = 1, u4status = 2;
	uint32_t u4Cnt = 0, u4BufSize = 0;
	uint32_t u4delayTime = 0;

	if (!pvSetBuffer || !u4SetBufferLen) {
		DBGLOG(OID, ERROR, "pvBuffer is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	pucSavedPtr = (int8_t *)pvSetBuffer;
	DBGLOG(INIT, INFO, "mddp GenSwitchcommand is [%s]\n", pucSavedPtr);
	wlanCfgParseArgument(pucSavedPtr, &i4Argc, apcArgv);
	DBGLOG(INIT, INFO, "argc [%d]\n", i4Argc);

	if (i4Argc != 3) {
		DBGLOG(REQ, ERROR, "argc(%d) is error\n", i4Argc);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	i4Ret = kalkStrtou32(
		apcArgv[1], 0, &u4MsgType);
	DBGLOG(OID, INFO,
		"parse u4MsgType %u i4Ret=%d\n",
		u4MsgType, i4Ret);

	i4Ret = kalkStrtou32(
		apcArgv[2], 0, &u4delayTime);
	DBGLOG(OID, INFO,
		"parse u4delayTime %u i4Ret=%d\n",
		u4delayTime, i4Ret);

	u4BufSize = (sizeof(struct mddpw_md_notify_info_t) +
		      sizeof(u4genSwitchBuf));

	buff = kalMemAlloc(u4BufSize, PHY_MEM_TYPE);
	if (buff == NULL) {
		DBGLOG(NIC, ERROR, "Can't allocate buffer.\n");
		goto exit;
	}

	prMdInfo = (struct mddpw_md_notify_info_t *) buff;


	if (u4MsgType == 0) {
		mddpNotifyMDGenSwitchStart(prAdapter);
		while (mddpGetGenSwitchState(prAdapter) ==
				MDDP_GEN_SWITCH_START_BEGIN_STATE) {
			u4Cnt++;
			kalMdelay(10);
			if (u4Cnt > 150)
				break;
		}
		kalMdelay(u4delayTime);
		mddpNotifyMDGenSwitchEnd(prAdapter);
	} else if (u4MsgType == 1)
		mddpNotifyMDGenSwitchEnd(prAdapter);
	else if (u4MsgType == 2) {
		prMdInfo->buf_len = 8;
		kalMemCopy(&prMdInfo->buf[0],
					&u4seq,
					sizeof(uint32_t));
		kalMemCopy(&prMdInfo->buf[4],
					&u4status,
					sizeof(uint32_t));
		mddpMdNotifyInfoHandleGenSwitchByPassStart(prAdapter, prMdInfo);
	} else if (u4MsgType == 3) {
		prMdInfo->buf_len = 8;
		kalMemCopy(&prMdInfo->buf[0],
					&u4seq,
					sizeof(uint32_t));
		kalMemCopy(&prMdInfo->buf[4],
					&u4status,
					sizeof(uint32_t));
		mddpMdNotifyInfoHandleGenSwitchByPassEnd(prAdapter, prMdInfo);
	}

	if (buff)
		kalMemFree(buff, PHY_MEM_TYPE, u4BufSize);

exit:
#endif /* CFG_MTK_MDDP_SUPPORT */
	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_PCIE_GEN_SWITCH */


#if CFG_SUPPORT_LOWLATENCY_MODE
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to enable/disable low latency mode from oid
 *
 * \param[in]  prAdapter       A pointer to the Adapter structure.
 * \param[in]  pvSetBuffer     A pointer to the buffer that holds the
 *                             OID-specific data to be set.
 * \param[in]  u4SetBufferLen  The number of bytes the set buffer.
 * \param[out] pu4SetInfoLen   Points to the number of bytes it read or is
 *                             needed
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanoidSetLowLatencyMode(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct PARAM_LOWLATENCY_DATA rParams;
	uint8_t ucBssIndex;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	if (u4SetBufferLen != (sizeof(uint32_t) * 7)) {
		*pu4SetInfoLen = (sizeof(uint32_t) * 7);
		return WLAN_STATUS_INVALID_LENGTH;
	}
	ASSERT(pu4SetInfoLen);

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	/* Initialize */
	kalMemCopy(&rParams, pvSetBuffer, u4SetBufferLen);

	/* Set low latency mode */
	DBGLOG(OID, INFO,
		"LowLatencySet(from oid set) event:0x%x, delay bound:udp(%d) tcp(%d), phy rate:%d, priority:udp(%d) tcp(%d), protocol:%d\n",
		rParams.u4Events, rParams.u4UdpDelayBound,
		rParams.u4TcpDelayBound, rParams.u4DataPhyRate,
		rParams.u4UdpPriority, rParams.u4TcpPriority,
		rParams.u4SupportProtocol);

	prAdapter->rWifiVar.ucUdpTspecUp = (uint8_t) rParams.u4UdpPriority;
	prAdapter->rWifiVar.ucTcpTspecUp = (uint8_t) rParams.u4TcpPriority;
	prAdapter->rWifiVar.u4UdpDelayBound = rParams.u4UdpDelayBound;
	prAdapter->rWifiVar.u4TcpDelayBound = rParams.u4TcpDelayBound;
	prAdapter->rWifiVar.ucDataRate = (uint8_t) rParams.u4DataPhyRate;
	prAdapter->rWifiVar.ucSupportProtocol =
		(uint8_t) rParams.u4SupportProtocol;

	wlanSetLowLatencyMode(prAdapter, rParams.u4Events, ucBssIndex);

	*pu4SetInfoLen = 0; /* We do not need to read */

	return WLAN_STATUS_SUCCESS;
}

#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

uint32_t wlanoidGetWifiType(struct ADAPTER *prAdapter,
			    void *pvSetBuffer,
			    uint32_t u4SetBufferLen,
			    uint32_t *pu4SetInfoLen)
{
	struct PARAM_GET_WIFI_TYPE *prParamGetWifiType;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct BSS_INFO *prBssInfo = NULL;
	uint8_t ucBssIdx;
	uint8_t ucPhyType;
	uint8_t ucMaxCopySize;
	uint8_t *pNameBuf;
	*pu4SetInfoLen = 0;
	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(OID, ERROR,
		       "Fail in query receive error! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}
	if (u4SetBufferLen < sizeof(struct PARAM_GET_WIFI_TYPE)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}
	prParamGetWifiType = (struct PARAM_GET_WIFI_TYPE *)pvSetBuffer;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
				kalGetNetDevPriv(prParamGetWifiType->prNetDev);

	if (prNetDevPrivate == NULL) {
		DBGLOG(OID, ERROR, "invalid prNetDevPrivate NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	ucBssIdx = prNetDevPrivate->ucBssIdx;
	DBGLOG(OID, INFO, "bss index=%d\n", ucBssIdx);
	kalMemZero(prParamGetWifiType->arWifiTypeName,
		   sizeof(prParamGetWifiType->arWifiTypeName));
	pNameBuf = &prParamGetWifiType->arWifiTypeName[0];
	ucMaxCopySize = sizeof(prParamGetWifiType->arWifiTypeName) - 1;
	if (ucBssIdx > prAdapter->ucSwBssIdNum) {
		DBGLOG(OID, ERROR, "invalid bss index: %u\n", ucBssIdx);
		return WLAN_STATUS_INVALID_DATA;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if ((!prBssInfo) || (!IS_BSS_ACTIVE(prBssInfo))) {
		DBGLOG(OID, ERROR, "invalid BssInfo: %p, %u\n",
		       prBssInfo, ucBssIdx);
		return WLAN_STATUS_INVALID_DATA;
	}
	ucPhyType = prBssInfo->ucPhyTypeSet;
	if (ucPhyType & PHY_TYPE_SET_802_11AC)
		kalStrnCpy(pNameBuf, "11AC", ucMaxCopySize);
	else if (ucPhyType & PHY_TYPE_SET_802_11N)
		kalStrnCpy(pNameBuf, "11N", ucMaxCopySize);
	else if (ucPhyType & PHY_TYPE_SET_802_11G)
		kalStrnCpy(pNameBuf, "11G", ucMaxCopySize);
	else if (ucPhyType & PHY_TYPE_SET_802_11A)
		kalStrnCpy(pNameBuf, "11A", ucMaxCopySize);
	else if (ucPhyType & PHY_TYPE_SET_802_11B)
		kalStrnCpy(pNameBuf, "11B", ucMaxCopySize);
	else
		DBGLOG(OID, INFO,
		       "unknown wifi type, prBssInfo->ucPhyTypeSet: %u\n",
		       ucPhyType);
	*pu4SetInfoLen = kalStrLen(pNameBuf);
	DBGLOG(OID, INFO, "wifi type=[%s](%d), phyType=%u\n",
	       pNameBuf, *pu4SetInfoLen, ucPhyType);
	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
uint32_t wlanoidGetLinkQualityInfo(struct ADAPTER *prAdapter,
				   void *pvSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen)
{
	struct PARAM_GET_LINK_QUALITY_INFO *prParam;
	struct WIFI_LINK_QUALITY_INFO *prSrcLinkQualityInfo = NULL;
	struct WIFI_LINK_QUALITY_INFO *prDstLinkQualityInfo = NULL;

	if (u4SetBufferLen < sizeof(struct PARAM_GET_LINK_QUALITY_INFO)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prParam = (struct PARAM_GET_LINK_QUALITY_INFO *)pvSetBuffer;
	prSrcLinkQualityInfo = &(prAdapter->rLinkQualityInfo);
	prDstLinkQualityInfo = prParam->prLinkQualityInfo;
	kalMemCopy(prDstLinkQualityInfo, prSrcLinkQualityInfo,
		   sizeof(struct WIFI_LINK_QUALITY_INFO));

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if CFG_SUPPORT_ANT_SWAP
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query antenna swap capablity
 *
 * \param[in]  prAdapter       A pointer to the Adapter structure.
 * \param[in]  pvSetBuffer     A pointer to the buffer that holds the
 *                             OID-specific data to be set.
 * \param[in]  u4SetBufferLen  The number of bytes the set buffer.
 * \param[out] pu4SetInfoLen   Points to the number of bytes it read or is
 *                             needed
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanoidQueryAntennaSwap(struct ADAPTER *prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen)

{
	uint32_t *puSupportSwpAntenn = 0;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!pu4QueryInfoLen) {
		DBGLOG(REQ, ERROR, "pu4QueryInfoLen is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	*pu4QueryInfoLen = sizeof(uint32_t);

	/* Check for query buffer length */
	if (u4QueryBufferLen != sizeof(uint32_t)) {
		DBGLOG(REQ, WARN, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	ASSERT(pvQueryBuffer);

	puSupportSwpAntenn = (uint32_t *) pvQueryBuffer;

	*puSupportSwpAntenn = !!(prAdapter->fgIsSupportAntSwp);
	DBGLOG(REQ, WARN, "*puSupportSwpAntenn : %u\n",
			*puSupportSwpAntenn);
	return WLAN_STATUS_SUCCESS;
}
#endif	/* CFG_SUPPORT_ANT_SWAP */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control oid function */
uint32_t wlanoidTxPowerControl(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen)
{
	struct PARAM_TX_PWR_CTRL_IOCTL *prPwrCtrlParam;
	struct TX_PWR_CTRL_ELEMENT *oldElement;
	u_int8_t fgApplied;

	if (!pvSetBuffer)
		return WLAN_STATUS_INVALID_DATA;

	if (u4SetBufferLen < sizeof(struct PARAM_TX_PWR_CTRL_IOCTL)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prPwrCtrlParam = (struct PARAM_TX_PWR_CTRL_IOCTL *)pvSetBuffer;
	if ((prPwrCtrlParam == NULL) || (prPwrCtrlParam->name == NULL)) {
		DBGLOG(OID, ERROR, "prPwrCtrlParam is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	fgApplied = prPwrCtrlParam->fgApplied;

	oldElement = txPwrCtrlFindElement(prAdapter,
					  prPwrCtrlParam->name, 0, TRUE,
					  PWR_CTRL_TYPE_DYNAMIC_LIST);
	if (oldElement != NULL)
		oldElement->fgApplied = FALSE;

	if (fgApplied == TRUE) {
		oldElement = txPwrCtrlFindElement(prAdapter,
				prPwrCtrlParam->name, prPwrCtrlParam->index,
				FALSE, PWR_CTRL_TYPE_DYNAMIC_LIST);
		if (oldElement != NULL) {
			if (prPwrCtrlParam->newSetting != NULL) {
				struct TX_PWR_CTRL_ELEMENT *newElement;

				newElement = txPwrCtrlStringToStruct(
					prPwrCtrlParam->newSetting, TRUE);
				if (newElement == NULL) {
					DBGLOG(OID, ERROR,
						"parse new setting fail, <%s>\n",
						prPwrCtrlParam->newSetting);
					return WLAN_STATUS_FAILURE;
				}

				kalMemCopy(newElement->name, oldElement->name,
					MAX_TX_PWR_CTRL_ELEMENT_NAME_SIZE);
				newElement->index = oldElement->index;
				newElement->eCtrlType = oldElement->eCtrlType;
				txPwrCtrlDeleteElement(prAdapter,
				       newElement->name, newElement->index,
				       PWR_CTRL_TYPE_DYNAMIC_LIST);
				oldElement = newElement;
				txPwrCtrlAddElement(prAdapter, oldElement);
			}
			oldElement->fgApplied = TRUE;
		}
	}

	if (oldElement != NULL)
		rlmDomainSendPwrLimitCmd(prAdapter);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t
wlanoidExternalAuthDone(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss;
#endif
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIndex = 0;
	struct PARAM_EXTERNAL_AUTH *params;
	struct MSG_SAA_EXTERNAL_AUTH_DONE *prExternalAuthMsg = NULL;

	if (u4SetBufferLen < sizeof(struct PARAM_EXTERNAL_AUTH)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	params = (struct PARAM_EXTERNAL_AUTH *) pvSetBuffer;
	ucBssIndex = params->ucBssIdx;
	if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
		DBGLOG(REQ, ERROR,
		       "SAE-confirm failed with invalid BssIdx in ndev\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prExternalAuthMsg = (struct MSG_SAA_EXTERNAL_AUTH_DONE *)cnmMemAlloc(
			    prAdapter, RAM_TYPE_MSG,
			    sizeof(struct MSG_SAA_EXTERNAL_AUTH_DONE));
	if (!prExternalAuthMsg) {
		DBGLOG(OID, WARN,
		       "SAE-confirm failed to allocate Msg\n");
		return WLAN_STATUS_RESOURCES;
	}

	DBGLOG(REQ, INFO, "SAE-confirm with bssid:"MACSTR", status:%d\n",
		MAC2STR(params->bssid), params->status);

	prStaRec = cnmGetStaRecByAddress(prAdapter, ucBssIndex, params->bssid);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = mldBssGetByBss(prAdapter,
		GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex));
	if (!prStaRec && IS_MLD_BSSINFO_MULTI(prMldBss)) {
		struct LINK *prBssList;
		struct BSS_INFO *prTempBss;
		struct STA_RECORD *prTempStaRec;

		prBssList = &prMldBss->rBssList;
		LINK_FOR_EACH_ENTRY(prTempBss, prBssList, rLinkEntryMld,
				    struct BSS_INFO) {
			prTempStaRec = cnmGetStaRecByAddress(prAdapter,
				prTempBss->ucBssIndex, params->bssid);
			if (prTempStaRec) {
				prStaRec = prTempStaRec;
				break;
			}
		}
	}
#endif
	if (!prStaRec) {
		DBGLOG(REQ, WARN, "SAE-confirm failed with bssid:" MACSTR "\n",
		       MAC2STR(params->bssid));
		cnmMemFree(prAdapter, prExternalAuthMsg);
		return WLAN_STATUS_INVALID_DATA;
	}

	prExternalAuthMsg->rMsgHdr.eMsgId = MID_OID_SAA_FSM_EXTERNAL_AUTH;
	prExternalAuthMsg->prStaRec = prStaRec;
	prExternalAuthMsg->status = params->status;

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *)prExternalAuthMsg,
		    MSG_SEND_METHOD_BUF);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidIndicateBssInfo(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo;
	struct BSS_DESC **pprBssDesc = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t i = 0;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;
	pprBssDesc = &prAdapter->rWifiVar.rScanInfo.rSchedScanParam.
		     aprPendingBssDescToInd[0];

	for (; i < SCN_SSID_MATCH_MAX_NUM; i++) {
		if (pprBssDesc[i] == NULL)
			break;
		if (pprBssDesc[i]->u2RawLength == 0)
			continue;
		kalIndicateBssInfo(prGlueInfo,
				   (uint8_t *) pprBssDesc[i]->aucRawBuf,
				   pprBssDesc[i]->u2RawLength,
				   pprBssDesc[i]->ucChannelNum,
				   pprBssDesc[i]->eBand,
				   RCPI_TO_dBm(pprBssDesc[i]->ucRCPI));
	}

	if (i > 0) {
		DBGLOG(SCN, INFO, "pending %d sched scan results\n", i);
		kalMemZero(&pprBssDesc[0], i * sizeof(struct BSS_DESC *));
	} else
		DBGLOG(SCN, TRACE, "pending %d sched scan results\n", i);

	return rStatus;
}	/* wlanoidIndicateBssInfo */

uint32_t wlanoidSetAxBlocklist(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	struct PARAM_AX_BLOCKLIST *pParamAxBlocklist;
	uint8_t count = 0;
	uint8_t ucBssIndex = 0;
	uint8_t i = 0;
	uint8_t aucTemp[MAC_ADDR_LEN];

	ASSERT(prAdapter);

	if (u4SetBufferLen < sizeof(struct PARAM_AX_BLOCKLIST))
		return WLAN_STATUS_INVALID_LENGTH;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	pParamAxBlocklist = (struct PARAM_AX_BLOCKLIST *) pvSetBuffer;
	count = pParamAxBlocklist->ucCount;

	if (count > MAX_AX_BLOCKLIST_ENTRIES) {
		DBGLOG(OID, WARN,
		    "Could only set %d BSSID in blocklist!\n",
		    MAX_AX_BLOCKLIST_ENTRIES);
		count = MAX_AX_BLOCKLIST_ENTRIES;
	}

	clearAxBlocklist(prAdapter, ucBssIndex, pParamAxBlocklist->ucType);
	for (i = 0; i < count ; i++) {
		COPY_MAC_ADDR(aucTemp, &pParamAxBlocklist->aucList[i]);
		addAxBlocklist(prAdapter, aucTemp, ucBssIndex,
				pParamAxBlocklist->ucType);
		DBGLOG(OID, INFO,
			"Set BSSID " MACSTR " into %s blocklist!\n",
			MAC2STR(aucTemp),
			pParamAxBlocklist->ucType == 0 ? "AX" : "+HTC");
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidSetCusBlocklist(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	ASSERT(prAdapter);

	if (u4SetBufferLen < sizeof(struct PARAM_CUS_BLOCKLIST))
		return WLAN_STATUS_INVALID_LENGTH;

	aisAddCusBlocklist(prAdapter,
			   (struct PARAM_CUS_BLOCKLIST *) pvSetBuffer,
			   GET_IOCTL_BSSIDX(prAdapter));

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidForceStbcMrc(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct PARAM_STBC_MRC *pParam = (struct PARAM_STBC_MRC *) pvSetBuffer;
	struct BSS_INFO *prBssInfo;
	uint32_t ret = WLAN_STATUS_SUCCESS;

	if (prAdapter == NULL)
		return WLAN_STATUS_FAILURE;

	DBGLOG(OID, INFO, "Set STBC MRC: type = %d, bss = %d, enable = %d\n",
		pParam->ucType, pParam->ucBssIndex, pParam->fgEnable);

	if (pParam->ucBssIndex >= MAX_BSSID_NUM) {
		DBGLOG(OID, ERROR, "Invalid bss = %d\n", pParam->ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	prBssInfo = prAdapter->aprBssInfo[pParam->ucBssIndex];
	if (prBssInfo == NULL)
		return WLAN_STATUS_FAILURE;

	if (IS_BSS_NOT_ALIVE(prAdapter, prBssInfo)) {
		DBGLOG(OID, ERROR, "Bss = %d is not alive\n",
			pParam->ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	if (prBssInfo->eNetworkType != NETWORK_TYPE_AIS) {
		DBGLOG(OID, ERROR, "Bss = %d is not STA\n", pParam->ucBssIndex);
		return WLAN_STATUS_FAILURE;
	}

	if (pParam->ucType == 0) {
		/* STBC */
		if (pParam->fgEnable == 1) {
			/* Enable STBC */
			if (prBssInfo->eForceStbc == STBC_MRC_STATE_DISABLED)
				ret = rlmUpdateStbcSetting(prAdapter,
					pParam->ucBssIndex, 1, TRUE);
			else
				DBGLOG(OID, INFO, "STBC originally enabled\n");
		} else if (pParam->fgEnable == 0) {
			/* Disable STBC */
			if (prBssInfo->eForceStbc == STBC_MRC_STATE_ENABLED)
				ret = rlmUpdateStbcSetting(prAdapter,
					pParam->ucBssIndex, 0, TRUE);
			else
				DBGLOG(OID, INFO, "STBC originally disabled\n");
		} else {
			DBGLOG(OID, ERROR, "Invalid enable flag = %d\n",
				pParam->fgEnable);
			ret = WLAN_STATUS_FAILURE;
		}
		DBGLOG(OID, INFO, "STBC state = %d\n", prBssInfo->eForceStbc);
	} else if (pParam->ucType == 1) {
		/* MRC */
		if (pParam->fgEnable == 1) {
			/* Enable MRC */
			switch (prBssInfo->eForceMrc) {
			case STBC_MRC_STATE_DISABLED:
				ret = rlmUpdateMrcSetting(prAdapter,
					pParam->ucBssIndex, 1);
				break;
			case STBC_MRC_STATE_ENABLED:
			case STBC_MRC_STATE_ENABLING:
				/* Already enabled or eanling */
				DBGLOG(OID, INFO, "STBC enabled or enabling");
				break;
			default:
				break;
			}
		} else if (pParam->fgEnable == 0) {
			/* Disable MRC */
			switch (prBssInfo->eForceMrc) {
			case STBC_MRC_STATE_DISABLED:
				/* Already disabled */
				DBGLOG(OID, INFO, "STBC is disabled");
				break;
			case STBC_MRC_STATE_ENABLED:
				ret = rlmUpdateMrcSetting(prAdapter,
					pParam->ucBssIndex, 0);
				break;
			case STBC_MRC_STATE_ENABLING:
				DBGLOG(OID, INFO, "STBC is enabling");
				ret = WLAN_STATUS_FAILURE;
				break;
			default:
				break;
			}
		} else {
			DBGLOG(OID, ERROR, "Invalid enable flag = %d\n",
				pParam->fgEnable);
			ret = WLAN_STATUS_FAILURE;
		}
		DBGLOG(OID, INFO, "MRC state = %d\n", prBssInfo->eForceMrc);
	} else {
		DBGLOG(OID, ERROR, "Invalid type = %d\n", pParam->ucType);
		ret = WLAN_STATUS_FAILURE;
	}

	return ret;
}

uint32_t wlanoidThermalProtectAct(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (pvSetBuffer == NULL)
		return WLAN_STATUS_INVALID_DATA;

	rStatus = wlanSendSetQueryExtCmd(prAdapter,
		CMD_ID_LAYER_0_EXT_MAGIC_NUM,
		EXT_CMD_ID_THERMAL_PROTECT,
		TRUE,
		FALSE,
		TRUE,
		nicCmdEventSetCommon,
		nicOidCmdTimeoutCommon,
		u4SetBufferLen,
		(uint8_t *) pvSetBuffer, (uint8_t *) pvSetBuffer,
		u4SetBufferLen);

	return rStatus;
}

uint32_t
wlanoidSetATXOP(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	struct CMD_ATXOP_CFG *cmd;
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_GAMING_MODE *uni_cmd;
	struct UNI_CMD_GAMING_MODE_ATXOP_SET_T *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GAMING_MODE) +
			       sizeof(struct UNI_CMD_GAMING_MODE_ATXOP_SET_T);

	if ((prAdapter == NULL) || (pvSetBuffer == NULL) ||
		(pu4SetInfoLen == NULL))
		return WLAN_STATUS_INVALID_DATA;

	*pu4SetInfoLen = sizeof(struct CMD_ATXOP_CFG);

	if (u4SetBufferLen < sizeof(struct CMD_ATXOP_CFG))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	cmd = (struct CMD_ATXOP_CFG *)pvSetBuffer;

	uni_cmd = (struct UNI_CMD_GAMING_MODE *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_GAMING_MODE_ATXOP_SET_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GAMING_MODE_ATXOP_SET;
	tag->u2Length = sizeof(*tag);

	tag->u4Cmd = cmd->u4Cmd;

	memcpy(tag->au4Param, cmd->au4Param,
			sizeof(uint32_t) * MAX_ATXOP_PARAM_NUM);

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_GAMING_MODE,
			     TRUE,
			     FALSE,
			     TRUE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);
	return status;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}	/* wlanoidSetATXOP */

uint32_t
wlanoidSetMdvt(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	struct PARAM_MDVT_STRUCT *prMdvtInfo;
	struct CMD_MDVT_CFG rCmdMdvtCfg = {0};

	ASSERT(prAdapter);

	*pu4SetInfoLen = sizeof(struct PARAM_MDVT_STRUCT);

	if (u4SetBufferLen < sizeof(struct PARAM_MDVT_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prMdvtInfo = (struct PARAM_MDVT_STRUCT *)pvSetBuffer;
	rCmdMdvtCfg.u4ModuleId = prMdvtInfo->u4ModuleId;
	rCmdMdvtCfg.u4CaseId = prMdvtInfo->u4CaseId;
	rCmdMdvtCfg.ucCapId = prMdvtInfo->ucCapId;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_SET_MDVT,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_MDVT_CFG),
				   (uint8_t *) &rCmdMdvtCfg,
				   pvSetBuffer, u4SetBufferLen);
}	/* wlanoidSetMdvt */
#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
uint32_t
wlanoidSetMdnsCmdToFw(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct CMD_MDNS_PARAM_T *cmdMdnsParam;

	if (!prAdapter) {
		DBGLOG(REQ, WARN, "NULL prAdapter!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pvSetBuffer) {
		DBGLOG(REQ, WARN, "NULL pvSetBuffer!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pu4SetInfoLen) {
		DBGLOG(REQ, WARN, "NULL pu4SetInfoLen!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (u4SetBufferLen < sizeof(struct CMD_MDNS_PARAM_T)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = sizeof(struct CMD_MDNS_PARAM_T);

	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	cmdMdnsParam = (struct CMD_MDNS_PARAM_T *)
			   pvSetBuffer;

	DBGLOG(SW4, STATE, "set cmd %u.\n", cmdMdnsParam->ucCmd);

	return wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_MDNS_RECORD,
			TRUE,
			FALSE,
			TRUE,
			nicCmdEventSetCommon,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_MDNS_PARAM_T),
			(uint8_t *)cmdMdnsParam,
			NULL,
			0);
}

uint32_t wlanoidGetMdnsHitMiss(struct ADAPTER *prAdapter,
	 void *pvSetBuffer,
	 uint32_t u4SetBufferLen,
	 uint32_t *pu4SetInfoLen)
{

	struct CMD_MDNS_PARAM_T *cmdMdnsParam;
	struct EVENT_ID_MDNS_RECORD_T *prMdnsRecordEvent;
	uint32_t u4QueryBufLen = sizeof(struct EVENT_ID_MDNS_RECORD_T);
	uint32_t u4QueryInfoLen = sizeof(struct CMD_MDNS_PARAM_T);

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!pu4SetInfoLen) {
		DBGLOG(NIC, ERROR, "NULL pu4SetInfoLen!\n");
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (!pvSetBuffer) {
		DBGLOG(NIC, ERROR, "NULL pvSetBuffer!\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prMdnsRecordEvent = &prAdapter->rMdnsInfo.rMdnsRecordEvent;
	*pu4SetInfoLen = sizeof(struct CMD_MDNS_PARAM_T);

	cmdMdnsParam = (struct CMD_MDNS_PARAM_T *)pvSetBuffer;

	DBGLOG(SW4, STATE, "set cmd %u.\n", cmdMdnsParam->ucCmd);

	return wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			    CMD_ID_SET_MDNS_RECORD,	/* ucCID */
			    TRUE,	/* fgSetQuery */
			    TRUE,	/* fgNeedResp */
			    TRUE,	/* fgIsOid */
			    nicCmdEventQueryMdnsStats,    /* pfCmdDoneHandler */
			    nicOidCmdTimeoutCommon, /* pfCmdTimeoutHandler */
			    u4QueryInfoLen,    /* u4SetQueryInfoLen */
			    (uint8_t *)cmdMdnsParam,  /* pucInfoBuffer */
			    (void *)prMdnsRecordEvent, /* pvSetQueryBuffer */
			    u4QueryBufLen);   /* u4SetQueryBufferLen */
}

#endif /* #if CFG_SUPPORT_MDNS_OFFLOAD */

#endif /* #if CFG_WOW_SUPPORT */

#if (CFG_SUPPORT_TSF_SYNC == 1)
uint32_t
wlanoidLatchTSF(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen)
{
	struct CMD_TSF_SYNC *prCmdTSF;

	if (!prAdapter) {
		DBGLOG(REQ, WARN, "NULL prAdapter!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pvQueryBuffer) {
		DBGLOG(REQ, WARN, "NULL pvQueryBuffer!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pu4QueryInfoLen) {
		DBGLOG(REQ, WARN, "NULL pu4QueryInfoLen!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (u4QueryBufferLen < sizeof(struct CMD_TSF_SYNC)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prCmdTSF = (struct CMD_TSF_SYNC *)pvQueryBuffer;

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_BEACON_TSF_SYNC,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventLatchTSF,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_TSF_SYNC),
				   (uint8_t *) pvQueryBuffer,
				   pvQueryBuffer,
				   u4QueryBufferLen);
}				/* end of wlanoidLatchTSF() */
#endif

#if (CFG_SUPPORT_PKT_OFLD == 1)

uint32_t
wlanoidSetOffloadInfo(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(struct PARAM_OFLD_INFO)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_PKT_OFLD,
				   TRUE,
				   FALSE,
				   TRUE,
				   nicCmdEventSetCommon,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_OFLD_INFO),
				   (uint8_t *) pvSetBuffer,
				   pvSetBuffer, u4SetBufferLen);

}	/* wlanoidSetOffloadInfo */

uint32_t
wlanoidQueryOffloadInfo(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen < sizeof(struct PARAM_OFLD_INFO)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_PKT_OFLD,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventQueryOfldInfo,
				   nicOidCmdTimeoutCommon,
				   sizeof(struct CMD_OFLD_INFO),
				   (uint8_t *) pvSetBuffer,
				   pvSetBuffer, u4SetBufferLen);

}	/* wlanoidQueryOffloadInfo */

#endif /* CFG_SUPPORT_PKT_OFLD */

#if (CFG_WIFI_ISO_DETECT == 1)
/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to do Coex Isolation Detection.

* \param[in] pvAdapter Pointer to the Adapter structure.
* \param[out] pvQueryBuf A pointer to the buffer that holds the result of
*                                   the query.
* \param[in] u4QueryBufLen The length of the query buffer.
* \param[out] pu4QueryInfoLen If the call is successful, returns the number of
*                             bytes written into the query buffer. If the call
*                             failed due to invalid length of the query buffer,
*                            eturns the amount of storage needed.
*
* \retval WLAN_STATUS_SUCCESS* \retval WLAN_STATUS_INVALID_LENGTH
*/
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryCoexIso(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen)
{
	struct PARAM_COEX_HANDLER *prParaCoexHandler;
	struct PARAM_COEX_ISO_DETECT *prParaCoexIsoDetect;
	struct COEX_CMD_HANDLER rCoexCmdHandler;
	struct COEX_CMD_ISO_DETECT rCoexCmdIsoDetect;

	if ((!prAdapter) ||
		(!pu4QueryInfoLen)) {
		DBGLOG(REQ, ERROR, "%s null pointer\n",
		__func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u4QueryBufferLen) {
		DBGLOG(REQ, ERROR, "%s invalid length\n",
		__func__);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4QueryInfoLen = sizeof(struct PARAM_COEX_HANDLER);

	if (u4QueryBufferLen < sizeof(struct PARAM_COEX_HANDLER))
		return WLAN_STATUS_INVALID_LENGTH;

	prParaCoexHandler =
	(struct PARAM_COEX_HANDLER *) pvQueryBuffer;
	prParaCoexIsoDetect =
	(struct PARAM_COEX_ISO_DETECT *) &prParaCoexHandler->aucBuffer[0];

	rCoexCmdIsoDetect.u4Channel = prParaCoexIsoDetect->u4Channel;
	rCoexCmdIsoDetect.u4IsoPath = prParaCoexIsoDetect->u4IsoPath;
	rCoexCmdIsoDetect.u4Isolation = prParaCoexIsoDetect->u4Isolation;

	rCoexCmdHandler.u4SubCmd = prParaCoexHandler->u4SubCmd;

	/* Copy Memory */
	kalMemCopy(rCoexCmdHandler.aucBuffer,
			&rCoexCmdIsoDetect,
			sizeof(rCoexCmdIsoDetect));

	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_COEX_CTRL,
				FALSE,
				TRUE,
				TRUE,
				nicCmdEventQueryCoexIso,
				nicOidCmdTimeoutCommon,
				sizeof(struct COEX_CMD_HANDLER),
				(unsigned char *) &rCoexCmdHandler,
				pvQueryBuffer,
				u4QueryBufferLen);

}
#endif

#if (CFG_WIFI_GET_DPD_CACHE == 1)
uint32_t
wlanoidQueryDpdCache(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen)
{
	struct PARAM_GET_DPD_CACHE *prDpdCache;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Adapter not ready. ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct PARAM_GET_DPD_CACHE)) {
		DBGLOG(REQ, WARN, "Too short length %ld\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	} else if (prAdapter->fgTestMode == TRUE) {
		DBGLOG(REQ, WARN, "Not supported in Test Mode\n");
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	prDpdCache = (struct PARAM_GET_DPD_CACHE *)pvQueryBuffer;

	return wlanSendSetQueryCmd(prAdapter,
			   CMD_ID_GET_DPD_CACHE,
			   FALSE,
			   TRUE,
			   TRUE,
			   nicCmdEventQueryDpdCache,
			   nicOidCmdTimeoutCommon,
			   sizeof(struct PARAM_GET_DPD_CACHE),
			   (uint8_t *) prDpdCache,
			   pvQueryBuffer, u4QueryBufferLen);
}
#endif /* CFG_WIFI_GET_DPD_CACHE */

#if (CFG_WIFI_GET_MCS_INFO == 1)
uint32_t
wlanoidTxQueryMcsInfo(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen)
{
	struct PARAM_TX_MCS_INFO *prMcsInfo;
	struct STA_RECORD *prStaRecOfAP;

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Adapter not ready. ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct PARAM_TX_MCS_INFO)) {
		DBGLOG(REQ, WARN, "Too short length %ld\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	} else if (prAdapter->fgTestMode == TRUE) {
		DBGLOG(REQ, WARN, "Not supported in Test Mode\n");
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	prStaRecOfAP = aisGetStaRecOfAP(prAdapter, AIS_DEFAULT_INDEX);
	if (prStaRecOfAP == NULL)
		return WLAN_STATUS_FAILURE;

	prMcsInfo = (struct PARAM_TX_MCS_INFO *)pvQueryBuffer;
	prMcsInfo->ucStaIndex = prStaRecOfAP->ucIndex;

	return wlanSendSetQueryCmd(prAdapter,
			   CMD_ID_TX_MCS_INFO,
			   FALSE,
			   TRUE,
			   TRUE,
			   nicCmdEventQueryTxMcsInfo,
			   nicOidCmdTimeoutCommon,
			   sizeof(struct PARAM_TX_MCS_INFO),
			   (uint8_t *) prMcsInfo,
			   pvQueryBuffer, u4QueryBufferLen);
}
#endif /* CFG_WIFI_GET_MCS_INFO */

#if CFG_AP_80211K_SUPPORT
uint32_t wlanoidSendBeaconReportRequest(struct ADAPTER *prAdapter,
					void *pvSetBuffer,
					uint32_t u4SetBufferLen,
					uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	struct SUB_ELEMENT_LIST *prIE = NULL;
	struct IE_MEASUREMENT_REQ *prMeasureReqIE = NULL;
	struct RM_BCN_REQ *prBeaconReqIE = NULL;
	struct IE_SSID *prSSIDIe = NULL;
	struct SUB_IE_BEACON_REPORTING *prBcnReport = NULL;
	struct SUB_IE_REPORTING_DETAIL *prReportDetail = NULL;
	struct SUB_IE_REQUEST *prRequest = NULL;
	struct SUB_IE_AP_CHANNEL_REPORT *prAPChanReport = NULL;
	uint8_t *prTmpElem = NULL;
	uint8_t ucIELen = 0;
	uint8_t ucSsidLen = 0;
	struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT *prSetBcnRepReqInfo = NULL;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;
	prGlueInfo = prAdapter->prGlueInfo;

	/* check parameter */
	if (pvSetBuffer == NULL
			|| u4SetBufferLen !=
			sizeof(struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT)) {
		DBGLOG(REQ, WARN, "need BCN Rep Req Info\n");
		return WLAN_STATUS_FAILURE;
	}
	prSetBcnRepReqInfo =
		(struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT *) pvSetBuffer;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0)
		return WLAN_STATUS_FAILURE;
	if (p2pFuncRoleToBssIdx(prAdapter, ucRoleIdx, &ucBssIdx)
			!= WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		return WLAN_STATUS_FAILURE;
	}

	/* get Station Record */
	prStaRec = bssGetClientByMac(prAdapter,
				prBssInfo,
				prSetBcnRepReqInfo->aucPeerMac);
	if (prStaRec == NULL) {
		DBGLOG(REQ, WARN, "can't find station\n");
		return WLAN_STATUS_FAILURE;
	}

	prStaRec->u2BcnReqRepetition = prSetBcnRepReqInfo->u2Repetition;

	/* allocate IE memory */
	ucIELen = sizeof(*prIE) + 3
			+ sizeof(*prBeaconReqIE)
			+ sizeof(*prBcnReport)
			+ sizeof(*prReportDetail);
	if (kalStrLen(prSetBcnRepReqInfo->aucSsid))
		ucIELen += sizeof(*prSSIDIe);

	if (prSetBcnRepReqInfo->ucReportingDetail == 1)
		ucIELen += sizeof(*prRequest);

	if (prSetBcnRepReqInfo->ucChannel == 255)
		ucIELen += sizeof(*prAPChanReport);

	prIE = kalMemAlloc(ucIELen, PHY_MEM_TYPE);
	if (!prIE) {
		DBGLOG(OID, ERROR, "No Memory\n");
		return WLAN_STATUS_FAILURE;
	}
	prIE->prNext = NULL;
	prMeasureReqIE = (struct IE_MEASUREMENT_REQ *) &prIE->rSubIE;
	prBeaconReqIE =
		(struct RM_BCN_REQ *) &prMeasureReqIE->aucRequestFields[0];

	/* beacon request ie */
	prBeaconReqIE->ucRegulatoryClass = prSetBcnRepReqInfo->ucOperClass;
	prBeaconReqIE->ucChannel = prSetBcnRepReqInfo->ucChannel;
	prBeaconReqIE->u2RandomInterval = prSetBcnRepReqInfo->u2RandomInterval;
	prBeaconReqIE->u2Duration = prSetBcnRepReqInfo->u2MeasureDuration;
	prBeaconReqIE->ucMeasurementMode =
		prSetBcnRepReqInfo->ucMeasurementMode;
	COPY_MAC_ADDR(prBeaconReqIE->aucBssid, prSetBcnRepReqInfo->aucBssid);

	prTmpElem = &prBeaconReqIE->aucSubElements[0];
	/* ssid ie */
	if (kalStrLen(prSetBcnRepReqInfo->aucSsid)) {
		prSSIDIe = (struct IE_SSID *) prTmpElem;
		prSSIDIe->ucId = BCN_REQ_ELEM_SUBID_SSID;
		ucSsidLen = kalStrLen(prSetBcnRepReqInfo->aucSsid);
		prSSIDIe->ucLength = ucSsidLen;
		if (ucSsidLen > ELEM_MAX_LEN_SSID) {
			DBGLOG(REQ, WARN, "ssid length %u is too long\n",
				ucSsidLen);
			kalMemFree(prIE, PHY_MEM_TYPE, ucIELen);
			return WLAN_STATUS_FAILURE;
		}
		kalMemCopy(&prSSIDIe->aucSSID,
			&prSetBcnRepReqInfo->aucSsid,
			ucSsidLen);
		prTmpElem += (2 + prSSIDIe->ucLength);
	}

	/* Beacon Report information */
	prBcnReport = (struct SUB_IE_BEACON_REPORTING *) prTmpElem;
	prBcnReport->ucId = BCN_REQ_ELEM_SUBID_BEACON_REPORTING;
	prBcnReport->ucLength = 2;
	prBcnReport->ucReportingCond = prSetBcnRepReqInfo->ucReportCondition;
	prBcnReport->ucReportingRef = prSetBcnRepReqInfo->ucReportReference;
	prTmpElem += (2 + prBcnReport->ucLength);

	/* Reporting detail ie */
	prReportDetail = (struct SUB_IE_REPORTING_DETAIL *) prTmpElem;
	prReportDetail->ucSubID = BCN_REQ_ELEM_SUBID_REPORTING_DETAIL;
	prReportDetail->ucLength = 1;
	prReportDetail->ucDetailValue = prSetBcnRepReqInfo->ucReportingDetail;
	prTmpElem += (2 + prReportDetail->ucLength);

	/* Request */
	if (prSetBcnRepReqInfo->ucReportingDetail == 1) {
		prRequest = (struct SUB_IE_REQUEST *) prTmpElem;
		prRequest->ucId = BCN_REQ_ELEM_SUBID_REQUEST;
		prRequest->ucLength = prSetBcnRepReqInfo->ucNumberOfRequest;
		kalMemCopy(prRequest->aucElems,
				prSetBcnRepReqInfo->ucRequestElemList,
				prSetBcnRepReqInfo->ucNumberOfRequest);
		prTmpElem += (2 + prRequest->ucLength);
	}

	/* AP Channel Report ie */
	if (prSetBcnRepReqInfo->ucChannel == 255) {
		prAPChanReport = (struct SUB_IE_AP_CHANNEL_REPORT *) prTmpElem;
		prAPChanReport->ucId = BCN_REQ_ELEM_SUBID_AP_CHANNEL_REPORT;
		prAPChanReport->ucLength =
			1 + prSetBcnRepReqInfo->ucNumberOfAPChanReport;
		prAPChanReport->ucOpClass = prSetBcnRepReqInfo->ucOperClass;
		kalMemCopy(prAPChanReport->aucElems,
			prSetBcnRepReqInfo->ucChanList,
			prSetBcnRepReqInfo->ucNumberOfAPChanReport);
	}

	/* measurement ie */
	prMeasureReqIE->ucId = ELEM_ID_MEASUREMENT_REQ;
	prMeasureReqIE->ucLength =
		3 + OFFSET_OF(struct RM_BCN_REQ, aucSubElements);

	if (kalStrLen(prSetBcnRepReqInfo->aucSsid))
		prMeasureReqIE->ucLength += (2 + prSSIDIe->ucLength);

	prMeasureReqIE->ucLength += (2 + prBcnReport->ucLength);
	prMeasureReqIE->ucLength += (2 + prReportDetail->ucLength);

	if (prSetBcnRepReqInfo->ucReportingDetail == 1)
		prMeasureReqIE->ucLength += (2 + prRequest->ucLength);

	if (prSetBcnRepReqInfo->ucChannel == 255)
		prMeasureReqIE->ucLength += (2 + prAPChanReport->ucLength);

	prMeasureReqIE->ucToken = 0;
	prMeasureReqIE->ucRequestMode = 0;
	prMeasureReqIE->ucMeasurementType = ELEM_RM_TYPE_BEACON_REQ;

	DBGLOG(OID, INFO, "Send Beacon Report Request\n");
	rlmMulAPAgentTxMeasurementRequest(prAdapter,
		prStaRec, prIE);

	kalMemFree(prIE, PHY_MEM_TYPE, ucIELen);
	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_AP_80211K_SUPPORT */

#if CFG_AP_80211V_SUPPORT
uint32_t wlanoidSendBTMRequest(struct ADAPTER *prAdapter,
				    void *pvSetBuffer, uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo = NULL;
	struct STA_RECORD *prStaRec = NULL;
	struct PARAM_CUSTOM_BTM_REQ_STRUCT *prSetBtmReqInfo = NULL;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;
	prGlueInfo = prAdapter->prGlueInfo;

	/* check parameter */
	if (pvSetBuffer == NULL
		|| u4SetBufferLen
			!= sizeof(struct PARAM_CUSTOM_BTM_REQ_STRUCT)) {
		DBGLOG(REQ, WARN, "need BTM Req Info\n");
		return WLAN_STATUS_FAILURE;
	}
	prSetBtmReqInfo = (struct PARAM_CUSTOM_BTM_REQ_STRUCT *) pvSetBuffer;

	/* get Bss Index from ndev */
	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) != 0)
		return WLAN_STATUS_FAILURE;
	if (p2pFuncRoleToBssIdx(prAdapter, ucRoleIdx, &ucBssIdx)
			!= WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;
	DBGLOG(REQ, INFO, "ucRoleIdx = %d\n", ucRoleIdx);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(REQ, WARN, "bss is not active\n");
		return WLAN_STATUS_FAILURE;
	}

	/* get Station Record */
	prStaRec = bssGetClientByMac(prAdapter,
				prBssInfo,
				prSetBtmReqInfo->aucPeerMac);
	if (prStaRec == NULL) {
		DBGLOG(REQ, WARN, "can't find station\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(OID, INFO, "Send BTM Request\n");
	wnmMulAPAgentSendBTMRequestFrame(prGlueInfo->prAdapter,
		prStaRec, prSetBtmReqInfo);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_AP_80211V_SUPPORT */

uint32_t wlanoidEnableVendorSpecifiedRpt(struct ADAPTER *prAdapter,
				    void *pvSetBuffer, uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen)
{
	uint8_t *pucEnable = NULL;

	if (u4SetBufferLen < sizeof(uint8_t)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	pucEnable = (uint8_t *) pvSetBuffer;
	prAdapter->ucEnVendorSpecifiedRpt = *pucEnable;
	DBGLOG(OID, INFO, "%s vendor specified packet to host\n",
		*pucEnable ? "Enable" : "Disable");

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query SER information.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer A pointer to the buffer that holds the result of
 *                           the query.
 * \param[in] u4QueryBufLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen If the call is successful, returns the number of
 *                            bytes written into the query buffer. If the call
 *                            failed due to invalid length of the query buffer,
 *                            returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_PENDING
 * \retval WLAN_STATUS_FAILURE
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanoidQuerySerInfo(struct ADAPTER *prAdapter,
			     void *pvQueryBuffer,
			     uint32_t u4QueryBufferLen,
			     uint32_t *pu4QueryInfoLen)
{
	struct PARAM_SER_INFO_T *prSerInfo;
	struct EXT_CMD_SER_T rCmdSer = {0};

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!pu4QueryInfoLen) {
		DBGLOG(REQ, ERROR, "pu4QueryInfoLen is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u4QueryBufferLen < sizeof(struct PARAM_SER_INFO_T)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (!pvQueryBuffer) {
		DBGLOG(REQ, ERROR, "pvQueryBuffer is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	*pu4QueryInfoLen = sizeof(struct PARAM_SER_INFO_T);

	prSerInfo = (struct PARAM_SER_INFO_T *) pvQueryBuffer;

	rCmdSer.ucAction = SER_ACTION_QUERY;

	return wlanSendSetQueryExtCmd(prAdapter,
				      CMD_ID_LAYER_0_EXT_MAGIC_NUM,
				      EXT_CMD_ID_SER,
				      FALSE,
				      TRUE,
				      TRUE,
				      nicCmdEventQuerySerInfo,
				      nicOidCmdTimeoutCommon,
				      sizeof(struct EXT_CMD_SER_T),
				      (uint8_t *)&rCmdSer,
				      pvQueryBuffer,
				      u4QueryBufferLen);
}

uint32_t
wlanoidQueryThermalAdieTemp(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return nicUniCmdQueryThermalAdieTemp(prAdapter,
		pvQueryBuffer,
		u4QueryBufferLen);
#else
	DBGLOG(OID, WARN, "NOT supported.\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t
wlanoidQueryThermalDdieTemp(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return nicUniCmdQueryThermalDdieTemp(prAdapter,
		pvQueryBuffer,
		u4QueryBufferLen);
#else
	DBGLOG(OID, WARN, "NOT supported.\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t
wlanoidQueryThermalAdcTemp(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND

	return nicUniCmdQueryThermalAdcTemp(prAdapter,
		pvQueryBuffer,
		u4QueryBufferLen);
#else
	DBGLOG(OID, WARN, "NOT supported.\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

#if CFG_SUPPORT_RTT
uint32_t wlanoidGetRttCapabilities(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen)
{
#if CFG_RTT_TEST_MODE
	struct RTT_CAPABILITIES *capa;
#endif

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(struct RTT_CAPABILITIES);

	DBGLOG(RTT, INFO, "pu4QueryInfoLen %d", *pu4QueryInfoLen);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(RTT, WARN, "Adapter is not ready. ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct RTT_CAPABILITIES)) {
		DBGLOG(RTT, WARN, "Too short length %u\n", u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

#if CFG_RTT_TEST_MODE
	capa = (struct RTT_CAPABILITIES *) pvQueryBuffer;
	capa->fgRttOneSidedSupported = 0;
	capa->fgRttFtmSupported = 1;
	capa->fgLciSupported = 0;
	capa->fgLcrSupported = 0;
	capa->ucPreambleSupport = 0x07;
	capa->ucBwSupport = 0x1c;
	capa->fgResponderSupported = 0;
	capa->fgMcVersion = 43;

	return WLAN_STATUS_SUCCESS;
#else
	return wlanSendSetQueryCmd(prAdapter,
				   CMD_ID_RTT_GET_CAPABILITIES,
				   FALSE,
				   TRUE,
				   TRUE,
				   nicCmdEventRttCapabilities,
				   nicOidCmdTimeoutCommon,
				   0, NULL, pvQueryBuffer, u4QueryBufferLen);
#endif

}

uint32_t wlanoidHandleRttRequest(struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct PARAM_RTT_REQUEST *prRttRequest;
	uint8_t ucBssIndex = 0;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);

	ASSERT(prAdapter);
	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_RTT_REQUEST);

	DBGLOG(RTT, INFO, "u4SetBufferLen %d", u4SetBufferLen);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(RTT, WARN, "Adapter is not ready. ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4SetBufferLen != sizeof(struct PARAM_RTT_REQUEST)) {
		DBGLOG(RTT, WARN, "Too short length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	prRttRequest = (struct PARAM_RTT_REQUEST *) pvSetBuffer;

	return rttHandleRttRequest(prAdapter, prRttRequest, ucBssIndex);
}
#endif /* CFG_SUPPORT_RTT */

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
uint32_t wlanoidListMode(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen)
{
	uint8_t *pCmdBuf = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter || !pvQueryBuffer)
		return WLAN_STATUS_INVALID_DATA;

	DBGLOG(OID, TRACE, "wlanoidListMode\n");

	pCmdBuf = kalMemAlloc(u4QueryBufferLen, VIR_MEM_TYPE);

	if (pCmdBuf == NULL)
		return WLAN_STATUS_RESOURCES;

	kalMemCopy(pCmdBuf, pvQueryBuffer, u4QueryBufferLen);

	rStatus = wlanSendSetQueryCmd(prAdapter,
		      CMD_ID_LIST_MODE,
		      FALSE,
		      TRUE,
		      TRUE,
		      nicCmdEventListmode,
		      nicOidCmdTimeoutCommon,
		      u4QueryBufferLen,
		      pCmdBuf,
		      pvQueryBuffer,
		      u4QueryBufferLen);

	/* Prevent list mode command takes more than 2 seconds */
	if (rStatus == WLAN_STATUS_FAILURE)
		rStatus = WLAN_STATUS_SUCCESS;

	kalMemFree(pCmdBuf, VIR_MEM_TYPE, u4QueryBufferLen);
	return rStatus;
}
#endif
#endif

uint32_t wlanoidGetSleepCntInfo(
	struct ADAPTER *prAdapter,
	void *pvGetBuffer,
	uint32_t u4GetBufferLen,
	uint32_t *pu4GetInfoLen)
{
	struct PARAM_SLEEP_CNT_INFO *prLpInfo;
	struct CMD_LP_DBG_CTRL rCmdLp = {0};

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!pvGetBuffer) {
		DBGLOG(REQ, ERROR, "pvGetBuffer is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u4GetBufferLen < sizeof(struct PARAM_SLEEP_CNT_INFO)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4GetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (!pu4GetInfoLen) {
		DBGLOG(REQ, ERROR, "pu4GetInfoLen is NULL\n");
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4GetInfoLen = sizeof(struct PARAM_SLEEP_CNT_INFO);

	prLpInfo = (struct PARAM_SLEEP_CNT_INFO *)pvGetBuffer;

	rCmdLp.ucSubCmdId = LP_CMD_QUERY;
	rCmdLp.ucTag = LP_TAG_GET_SLP_CNT_INFO;

	return wlanSendSetQueryCmd(prAdapter,
			CMD_ID_LP_DBG_CTRL,
			FALSE,
			TRUE,
			TRUE,
			nicCmdEventGetSlpCntInfo,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_LP_DBG_CTRL),
			(uint8_t *)&rCmdLp,
			pvGetBuffer,
			u4GetBufferLen);

}

uint32_t wlanoidSetLpKeepPwrCtrl(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct CMD_LP_DBG_CTRL *prCmdLp;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!pvSetBuffer) {
		DBGLOG(REQ, ERROR, "pvGetBuffer is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u4SetBufferLen < sizeof(struct CMD_LP_DBG_CTRL)) {
		DBGLOG(REQ, ERROR, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}

	if (!pu4SetInfoLen) {
		DBGLOG(REQ, ERROR, "pu4SetInfoLen is NULL\n");
		return WLAN_STATUS_INVALID_LENGTH;
	}

	*pu4SetInfoLen = sizeof(struct CMD_LP_DBG_CTRL);

	prCmdLp = (struct CMD_LP_DBG_CTRL *)pvSetBuffer;

	prCmdLp->ucSubCmdId = LP_CMD_SET;
	prCmdLp->ucTag = LP_TAG_SET_KEEP_PWR_CTRL;

	return wlanSendSetQueryCmd(prAdapter,
			CMD_ID_LP_DBG_CTRL,
			TRUE,
			TRUE,
			TRUE,
			nicCmdEventLpKeepPwrCtrl,
			nicOidCmdTimeoutCommon,
			sizeof(struct CMD_LP_DBG_CTRL),
			(uint8_t *)prCmdLp,
			pvSetBuffer,
			u4SetBufferLen);
}


#if CFG_SUPPORT_MANIPULATE_TID
uint32_t
wlanoidManipulateTid(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	struct PARAM_MANIUPLATE_TID *prSetTid = NULL;

	*pu4SetInfoLen = sizeof(struct PARAM_MANIUPLATE_TID);

	if (u4SetBufferLen < sizeof(struct PARAM_MANIUPLATE_TID))
		return WLAN_STATUS_INVALID_LENGTH;

	prSetTid = (struct PARAM_MANIUPLATE_TID *) pvSetBuffer;

	if (prAdapter->rManipulateTidInfo.ucUserPriority >= TX_DESC_TID_NUM) {
		DBGLOG(INIT, ERROR, "Manipilate Tid Invalid TID=[%d]\n",
			prAdapter->rManipulateTidInfo.ucUserPriority);
		return WLAN_STATUS_INVALID_DATA;
	}

	prAdapter->rManipulateTidInfo.fgManipulateTidEnabled =
		prSetTid->ucMode;
	prAdapter->rManipulateTidInfo.ucUserPriority =
		prSetTid->ucTid;

	DBGLOG(INIT, INFO, "Manipilate Tid [Enable:TID]=[%d:%d]\n",
	       prSetTid->ucMode, prSetTid->ucTid);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_MANIPULATE_TID */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidQueryEmlInfo(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return nicUniCmdQueryEmlInfo(prAdapter,
		pvQueryBuffer,
		u4QueryBufferLen);
#else
	DBGLOG(OID, WARN, "NOT supported.\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is use to update 6G power mode
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSet6GPwrMode(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint8_t ucBssIdx = 0, ucMode = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(uint8_t);

	if (u4SetBufferLen < sizeof(uint8_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	ucMode = *(uint8_t *)pvSetBuffer;

	for (ucBssIdx = 0; ucBssIdx < MAX_BSSID_NUM; ucBssIdx++) {
		rStatus = rlmDomain6GPwrModeUpdate(prAdapter,
						ucBssIdx,
						ucMode);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return WLAN_STATUS_FAILURE;

	}

	prAdapter->fg6GPwrModeForce = TRUE;

	return rStatus;
}	/* wlanoidSet6GPwrMode */
#endif

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is use to generate power limit data, write data to EMI
 *        and send cmd to FW update cache table.
 *        Note : Because share memory with FW, so we need to make sure race
 *               condition scenrio - concurrent fw read/driver write.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 * \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set.
 * \param[in] u4SetBufferLen The length of the set buffer.
 * \param[out] pu4SetInfoLen If the call is successful, returns the number of
 *                           bytes read from the set buffer. If the call failed
 *                           due to invalid length of the set buffer, returns
 *                           the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSendPwrLimitToEmi(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{

	uint8_t *pu1PwrLmtCountryCode = NULL;

	/***************ANT Power Setting******************/
	rlmDomainSendAntPowerSetting(prAdapter);

	/***************Country Power limit****************/
	/* Step 1  -  Patch power limit type*/
	rlmDomainPatchPwrLimitType();

	/* Step 2  -  Fill country code */
	pu1PwrLmtCountryCode =
		rlmDomainPwrLmtGetMatchCountryCode(
			prAdapter,
			prAdapter->rWifiVar.u2CountryCode);

	if (pu1PwrLmtCountryCode == NULL) {
		DBGLOG(INIT, INFO,
			"Can not find according CC[0x%04x] in pwr limit default table\n",
			prAdapter->rWifiVar.u2CountryCode);
		return 0;
	}

	rlmDomainSetPwrLimitCountryCode(
		prAdapter,
		pu1PwrLmtCountryCode
	);

	/* Step 3  -  Fill pwr limit header */
	rlmDomainSetPwrLimitHeader(prAdapter);

	/* Step 4  -  Build pwr limit payload */
	rlmDomainSetPwrLimitPayload(prAdapter);

	/* Step 5  -  Show power limit after loading default and config table*/
	rlmDomainDumpAllPwrLmtData("Old", prAdapter);

	/* Step 6  -  Compare with dynamic power setting */
	rlmDomainApplyDynPwrSetting(prAdapter);

	/* Step 7  -  Show power limit after campare dynamic power setting*/
	rlmDomainDumpAllPwrLmtData("Final", prAdapter);

	/* Step 8  -  Write EMI raw data & Send EMI CMD info*/
	if (rlmDomainIsFWInReadEmiDataProcess(prAdapter) == FALSE) {
		rlmDomainWritePwrLimitToEmi(prAdapter);
	} else {
		/*  If driver has new data, and fw is reading
		 *  driver writing at this time may cause race condition
		 *  So we cache new data, and we will trigger again
		 *  when fw event coming.
		 */
		rlmDoaminSetPwrLmtNewDataFlag(prAdapter, TRUE);
		DBGLOG(INIT, INFO,
			"FW is reading EMI, Cache and wait FW response\n");
	}
	return 0;
}
#endif

#if CFG_SUPPORT_WED_PROXY
uint32_t
wlanoidWedAttachWarp(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct net_device *prDev = NULL;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(struct net_device *))
		return WLAN_STATUS_INVALID_LENGTH;

	prDev = (struct net_device *)pvSetBuffer;

	DBGLOG(HAL, STATE, "WED_ATTACH_IFON\n");
	wedAttachDetach(prAdapter, prDev, TRUE);

	return rStatus;
}

uint32_t
wlanoidWedDetachWarp(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct net_device *prDev = NULL;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(struct net_device *))
		return WLAN_STATUS_INVALID_LENGTH;

	prDev = (struct net_device *)pvSetBuffer;

	DBGLOG(HAL, STATE, "WED_DETACH_IFDOWN\n");
	wedAttachDetach(prAdapter, prDev, FALSE);

	return rStatus;
}

uint32_t
wlanoidWedSuspend(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);

	DBGLOG(HAL, STATE, "WED_DETACH_SUSPEND\n");
	wedSuspendResume(TRUE);

	return rStatus;
}

uint32_t
wlanoidWedResume(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);

	DBGLOG(HAL, STATE, "WED_ATTACH_RESUME\n");
	wedSuspendResume(FALSE);

	return rStatus;
}

uint32_t
wlanoidWedRecoveryStatus(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t ser_status;

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(uint32_t))
		return WLAN_STATUS_INVALID_LENGTH;

	ser_status = *(uint32_t *)pvSetBuffer;

	wedHwRecoveryFromError(prAdapter, ser_status);

	return rStatus;
}
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidAddDelMldLink(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct MSG_ADD_DEL_MLD_LINK *prMsg =
		(struct MSG_ADD_DEL_MLD_LINK *)pvSetBuffer;
	struct WIFI_VAR *prWifiVar;
	struct GL_P2P_INFO *prP2pInfo;
	struct MLD_BSS_INFO *prMldBss;
	uint8_t ucRoleIdx, ucBssIdx;
	uint32_t r4Status = WLAN_STATUS_SUCCESS;

	if (!prMsg) {
		DBGLOG(OID, ERROR, "Null msg.\n");
		r4Status = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	ucRoleIdx = prMsg->ucRoleIdx;
	prWifiVar = &prAdapter->rWifiVar;
	prP2pInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx];
	if (!prP2pInfo) {
		DBGLOG(OID, ERROR, "Null prP2pInfo by role %u.\n", ucRoleIdx);
		r4Status = WLAN_STATUS_INVALID_DATA;
		goto exit;
	}

	DBGLOG(OID, INFO,
		"action=%u mld_idx=%u role=%u link=%u type=%d mld_addr="MACSTR
		" link_addr="MACSTR" netdev=0x%p\n",
		prMsg->ucAction,
		prMsg->ucMldBssIdx,
		prMsg->ucRoleIdx,
		prMsg->u4LinkId,
		prMsg->eIftype,
		MAC2STR(prMsg->aucMldAddr),
		MAC2STR(prMsg->aucLinkAddr),
		prMsg->prNetDevice);

	if (prMsg->ucAction == 1) {
		struct MSG_P2P_ADD_MLD_LINK *prMsgMldLinkAdd;

		if (prMsg->u4LinkId > 0 &&
		    prWifiVar->aprP2pRoleFsmInfo[ucRoleIdx] == NULL) {
			struct MSG_P2P_SWITCH_OP_MODE *prSwitchModeMsg;
			uint8_t ucLinkMax = 0;

			if (prMsg->eIftype == IFTYPE_AP)
				ucLinkMax = prWifiVar->ucApMldLinkMax;
			else
				ucLinkMax = prWifiVar->ucP2pMldLinkMax;

			p2pFuncInitConnectionSettings(prAdapter,
				prWifiVar->prP2PConnSettings[ucRoleIdx],
				prMsg->eIftype == IFTYPE_AP);

			prMldBss = mldBssGetByIdx(prAdapter,
						  prMsg->ucMldBssIdx);
			if (!prMldBss) {
				DBGLOG(OID, ERROR,
					"Null prMldBss by idx(%u)\n",
					prMsg->ucMldBssIdx);
				r4Status = WLAN_STATUS_INVALID_DATA;
				goto exit;
			} else if (prMldBss->rBssList.u4NumElem >= ucLinkMax) {
				DBGLOG(OID, ERROR,
					"Exceeds max link num(%u)\n",
					ucLinkMax);
				r4Status = WLAN_STATUS_RESOURCES;
				goto exit;
			}

			ucBssIdx = p2pRoleFsmInit(prAdapter, ucRoleIdx,
						  prMldBss->ucGroupMldId,
						  prMldBss->aucOwnMldAddr);
			if (ucBssIdx == MAX_BSSID_NUM) {
				DBGLOG(OID, ERROR,
					"p2pRoleFsmInit failed, role=%u, group=%u\n",
					ucRoleIdx,
					prMldBss->ucGroupMldId);
				r4Status = WLAN_STATUS_RESOURCES;
				goto exit;
			}

			prP2pInfo->aprRoleHandler = prMsg->prNetDevice;
			prP2pInfo->u4LinkId = prMsg->u4LinkId;

			/* Switch OP MOde. */
			prSwitchModeMsg = (struct MSG_P2P_SWITCH_OP_MODE *)
				cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_SWITCH_OP_MODE));
			if (!prSwitchModeMsg) {
				DBGLOG(OID, ERROR,
					"Alloc msg prSwitchModeMsg failed\n");
				r4Status = WLAN_STATUS_RESOURCES;
				goto exit;
			}
			kalMemZero(prSwitchModeMsg, sizeof(*prSwitchModeMsg));
			prSwitchModeMsg->rMsgHdr.eMsgId =
				MID_MNY_P2P_FUN_SWITCH;
			prSwitchModeMsg->ucRoleIdx = ucRoleIdx;
			switch (prMsg->eIftype) {
			case IFTYPE_AP:
				prSwitchModeMsg->eIftype = IFTYPE_AP;
				prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
				kalP2PSetRole(prAdapter->prGlueInfo, 2,
					      ucRoleIdx);
				break;
			case IFTYPE_P2P_GO:
				prSwitchModeMsg->eIftype = IFTYPE_P2P_GO;
				prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
				kalP2PSetRole(prAdapter->prGlueInfo, 2,
					      ucRoleIdx);
				break;
			default:
				DBGLOG(OID, ERROR, "Unsupported type: %d.\n",
					prMsg->eIftype);
				r4Status = WLAN_STATUS_INVALID_DATA;
				goto exit;
			}
			mboxSendMsg(prAdapter, MBOX_ID_0,
				    (struct MSG_HDR *)prSwitchModeMsg,
				    MSG_SEND_METHOD_UNBUF);
		}

		prMsgMldLinkAdd = (struct MSG_P2P_ADD_MLD_LINK *) cnmMemAlloc(
			prAdapter, RAM_TYPE_MSG,
			sizeof(*prMsgMldLinkAdd));
		if (!prMsgMldLinkAdd) {
			DBGLOG(OID, ERROR,
					"Alloc msg prMsgMldLinkAdd failed\n");
			r4Status = WLAN_STATUS_RESOURCES;
			goto exit;
		}
		kalMemZero(prMsgMldLinkAdd, sizeof(*prMsgMldLinkAdd));
		prMsgMldLinkAdd->rMsgHdr.eMsgId = MID_MNY_P2P_ADD_MLD_LINK;
		prMsgMldLinkAdd->ucRoleIdx = ucRoleIdx;
		prMsgMldLinkAdd->ucLinkIdx = prMsg->u4LinkId;
		COPY_MAC_ADDR(prMsgMldLinkAdd->aucMldAddr, prMsg->aucMldAddr);
		COPY_MAC_ADDR(prMsgMldLinkAdd->aucLinkAddr, prMsg->aucLinkAddr);
		mboxSendMsg(prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *)prMsgMldLinkAdd,
			    MSG_SEND_METHOD_UNBUF);
	} else {
		struct MSG_P2P_DEL_MLD_LINK *prMsgMldLinkDel;

		prMsgMldLinkDel = (struct MSG_P2P_DEL_MLD_LINK *) cnmMemAlloc(
			prAdapter, RAM_TYPE_MSG,
			sizeof(*prMsgMldLinkDel));
		if (prMsgMldLinkDel) {
			kalMemZero(prMsgMldLinkDel, sizeof(*prMsgMldLinkDel));
			prMsgMldLinkDel->rMsgHdr.eMsgId =
				MID_MNY_P2P_DEL_MLD_LINK;
			prMsgMldLinkDel->ucRoleIdx = ucRoleIdx;
			prMsgMldLinkDel->ucLinkIdx = prMsg->u4LinkId;
			mboxSendMsg(prAdapter, MBOX_ID_0,
				    (struct MSG_HDR *)prMsgMldLinkDel,
				    MSG_SEND_METHOD_UNBUF);
		}

		if (prMsg->u4LinkId > 0) {
			p2pRoleFsmUninit(prAdapter, ucRoleIdx);

			prP2pInfo->aprRoleHandler = NULL;
		}

		p2pFuncInitConnectionSettings(prAdapter,
			prWifiVar->prP2PConnSettings[ucRoleIdx],
			FALSE);
	}

exit:
	return r4Status;
}
#endif

#if (CFG_PCIE_GEN_SWITCH == 1)
uint32_t
wlandioStopPcieStatus(struct ADAPTER *prAdapter,
			uint8_t ucPcieStatus
)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct CMD_UPDATA_LP_PARAM *prCmdPcieStatus;

	prCmdPcieStatus =
		(struct CMD_UPDATA_LP_PARAM *)cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct CMD_UPDATA_LP_PARAM));

	if (!prCmdPcieStatus) {
		DBGLOG(OID, ERROR,
			"[Gen_Switch] prCmdPcieStatus fail!\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	prCmdPcieStatus->ucPcieTransitionStatus = ucPcieStatus;

	DBGLOG(OID, INFO,
		"[Gen_Switch] cmd to fw ucPcieStatus = %d\n", ucPcieStatus);

	wlanSendSetQueryCmd(prAdapter,
			CMD_ID_UPDATE_LP,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(*prCmdPcieStatus),
			(uint8_t *) prCmdPcieStatus, NULL, 0);

	cnmMemFree(prAdapter, prCmdPcieStatus);
	return rWlanStatus;

}
#endif //CFG_PCIE_GEN_SWITCH

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to query LTE safe channels.
 *
 * \param[in]  pvAdapter        Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer    A pointer to the buffer that holds the result of
 *                              the query.
 * \param[in]  u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen  If the call is successful, returns the number of
 *                              bytes written into the query buffer. If the call
 *                              failed due to invalid length of the query
 *                              buffer, returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_PENDING
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidQueryLteSafeChannel(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen)
{
#if CFG_SUPPORT_GET_LTE_SAFE_CHANNEL
	struct CMD_GET_LTE_SAFE_CHN rQuery_LTE_SAFE_CHN = { 0 };

	DBGLOG(P2P, TRACE, "query Lte safe channel bitmap");

	if (!prAdapter) {
		DBGLOG(P2P, ERROR, "no adapter found");
		return WLAN_STATUS_FAILURE;
	}

	if (!pu4QueryInfoLen) {
		DBGLOG(P2P, ERROR, "zero query info len");
		return WLAN_STATUS_FAILURE;
	}

	if (u4QueryBufferLen && !pvQueryBuffer) {
		DBGLOG(P2P, ERROR, "null query buffer with buffer len");
		return WLAN_STATUS_FAILURE;
	}

	*pu4QueryInfoLen = sizeof(struct PARAM_GET_CHN_INFO);

	if (u4QueryBufferLen < sizeof(struct PARAM_GET_CHN_INFO))
		return WLAN_STATUS_BUFFER_TOO_SHORT;

	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_GET_LTE_CHN,
				FALSE,
				TRUE,
				TRUE,
				nicCmdEventQueryLteSafeChn,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_GET_LTE_SAFE_CHN),
				(uint8_t *)&rQuery_LTE_SAFE_CHN,
				(struct PARAM_GET_CHN_INFO *)pvQueryBuffer,
				u4QueryBufferLen);

#else
	DBGLOG(P2P, TRACE, "[ACS] Not Support Get safe LTE Channels\n");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif /* CFG_SUPPORT_GET_LTE_SAFE_CHANNEL */
}
#endif /* CFG_ENABLE_WIFI_DIRECT */


#if CFG_SUPPORT_CCM
/*----------------------------------------------------------------------------*/
/*!
 * \brief Re-trigger CCM when CSA finished.
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidCcmRetrigger(struct ADAPTER *prAdapter, void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *)pvQueryBuffer;

	if (!prAdapter) {
		DBGLOG(P2P, ERROR, "no adapter found");
		return WLAN_STATUS_FAILURE;
	}

	if (!prBssInfo) {
		DBGLOG(P2P, ERROR, "no BssInfo found");
		return WLAN_STATUS_FAILURE;
	}

	/* do not support CSA by upper layer within CCM */
	if (LINK_IS_EMPTY(&prAdapter->rCcmCheckCsList))
		ccmChannelSwitchProducer(prAdapter, prBssInfo, __func__);
	else
		ccmChannelSwitchConsumer(prAdapter);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_CCM */
