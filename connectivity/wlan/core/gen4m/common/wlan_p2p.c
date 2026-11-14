// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file wlan_bow.c
 *    \brief This file contains the Wi-Fi Direct commands processing routines
 *             for MediaTek Inc. 802.11 Wireless LAN Adapters.
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
#if CFG_ENABLE_WIFI_DIRECT
/******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

/*---------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set a key to Wi-Fi Direct driver
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
 */
/*---------------------------------------------------------------------------*/
#if 0
uint32_t
wlanoidSetAddP2PKey(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct CMD_802_11_KEY rCmdKey;
	struct PARAM_KEY *prNewKey;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;

	DBGLOG(REQ, DEBUG, "\n");

	ASSERT(prAdapter);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	prNewKey = (struct PARAM_KEY *) pvSetBuffer;

	/* Verify the key structure length. */
	if (prNewKey->u4Length > u4SetBufferLen) {
		log_dbg(REQ, WARN,
		       "Invalid key structure length (%d) greater than total buffer length (%d)\n",
		       (uint8_t) prNewKey->u4Length, (uint8_t) u4SetBufferLen);

		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_LENGTH;
	}
	/* Verify the key material length for key material buffer */
	else if (prNewKey->u4KeyLength >
		prNewKey->u4Length -
		OFFSET_OF(struct PARAM_KEY, aucKeyMaterial)) {
		log_dbg(REQ, WARN,
				"Invalid key material length (%d)\n",
				(uint8_t) prNewKey->u4KeyLength);
		*pu4SetInfoLen = u4SetBufferLen;
		return WLAN_STATUS_INVALID_DATA;
	}
	/* Exception check */
	else if (prNewKey->u4KeyIndex & 0x0fffff00)
		return WLAN_STATUS_INVALID_DATA;
	/* Exception check, pairwise key must with transmit bit enabled */
	else if ((prNewKey->u4KeyIndex & BITS(30, 31)) == IS_UNICAST_KEY) {
		return WLAN_STATUS_INVALID_DATA;
	} else if (!(prNewKey->u4KeyLength == CCMP_KEY_LEN)
		   && !(prNewKey->u4KeyLength == TKIP_KEY_LEN)) {
		return WLAN_STATUS_INVALID_DATA;
	}
	/* Exception check, pairwise key must with transmit bit enabled */
	else if ((prNewKey->u4KeyIndex & BITS(30, 31)) == BITS(30, 31)) {
		if (((prNewKey->u4KeyIndex & 0xff) != 0) ||
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

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prNewKey->ucBssIdx);
	ASSERT(prBssInfo);
#if 0
	if (prBssInfo->ucBMCWlanIndex >= WTBL_SIZE) {
		prBssInfo->ucBMCWlanIndex =
		    secPrivacySeekForBcEntry(prAdapter,
				prBssInfo->ucBssIndex, prBssInfo->aucBSSID,
					0xff, CIPHER_SUITE_NONE, 0xff);
	}
#endif
	/* fill CMD_802_11_KEY */
	kalMemZero(&rCmdKey, sizeof(struct CMD_802_11_KEY));
	rCmdKey.ucAddRemove = 1;	/* add */
	rCmdKey.ucTxKey =
		((prNewKey->u4KeyIndex & IS_TRANSMIT_KEY) == IS_TRANSMIT_KEY)
		? 1 : 0;
	rCmdKey.ucKeyType =
		((prNewKey->u4KeyIndex & IS_UNICAST_KEY) == IS_UNICAST_KEY)
		? 1 : 0;
#if 0
	/* group client */
	if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
#else
	/* group client */
	if (kalP2PGetRole(prAdapter->prGlueInfo) == 1) {
#endif

		rCmdKey.ucIsAuthenticator = 0;
	} else {		/* group owner */
		rCmdKey.ucIsAuthenticator = 1;
		/* Force to set GO/AP Tx */
		rCmdKey.ucTxKey = 1;
	}

	COPY_MAC_ADDR(rCmdKey.aucPeerAddr, prNewKey->arBSSID);
	rCmdKey.ucBssIdx = prNewKey->ucBssIdx;
	if (prNewKey->u4KeyLength == CCMP_KEY_LEN)
		rCmdKey.ucAlgorithmId = CIPHER_SUITE_CCMP;	/* AES */
	else if (prNewKey->u4KeyLength == TKIP_KEY_LEN)
		rCmdKey.ucAlgorithmId = CIPHER_SUITE_TKIP;	/* TKIP */
	else if (prNewKey->u4KeyLength == WEP_40_LEN)
		rCmdKey.ucAlgorithmId = CIPHER_SUITE_WEP40;	/* WEP 40 */
	else if (prNewKey->u4KeyLength == WEP_104_LEN)
		rCmdKey.ucAlgorithmId = CIPHER_SUITE_WEP104;	/* WEP 104 */
	else
		ASSERT(FALSE);
	rCmdKey.ucKeyId = (uint8_t) (prNewKey->u4KeyIndex & 0xff);
	rCmdKey.ucKeyLen = (uint8_t) prNewKey->u4KeyLength;
	kalMemCopy(rCmdKey.aucKeyMaterial,
		(uint8_t *) prNewKey->aucKeyMaterial, rCmdKey.ucKeyLen);

	if ((rCmdKey.aucPeerAddr[0] &
		rCmdKey.aucPeerAddr[1] & rCmdKey.aucPeerAddr[2] &
	    rCmdKey.aucPeerAddr[3] & rCmdKey.aucPeerAddr[4] &
	    rCmdKey.aucPeerAddr[5]) == 0xFF) {
		kalMemCopy(rCmdKey.aucPeerAddr,
			prBssInfo->aucBSSID, MAC_ADDR_LEN);
		if (!rCmdKey.ucIsAuthenticator) {
			prStaRec = cnmGetStaRecByAddress(prAdapter,
					rCmdKey.ucBssIdx, rCmdKey.aucPeerAddr);
			if (!prStaRec)
				ASSERT(FALSE);
		}
	} else {
		prStaRec = cnmGetStaRecByAddress(prAdapter,
					rCmdKey.ucBssIdx, rCmdKey.aucPeerAddr);
	}

	if (rCmdKey.ucTxKey) {
		if (prStaRec) {
			if (rCmdKey.ucKeyType) {	/* RSN STA */
				ASSERT(prStaRec->ucWlanIndex < WTBL_SIZE);
				rCmdKey.ucWlanIndex = prStaRec->ucWlanIndex;
				/* wait for CMD Done ? */
				prStaRec->fgTransmitKeyExist = TRUE;
			} else {
				ASSERT(FALSE);
			}
		} else {
			if (prBssInfo) {	/* GO/AP Tx BC */
				ASSERT(prBssInfo->ucBMCWlanIndex < WTBL_SIZE);
				rCmdKey.ucWlanIndex = prBssInfo->ucBMCWlanIndex;
				prBssInfo->fgBcDefaultKeyExist = TRUE;
				prBssInfo->ucTxDefaultKeyID = rCmdKey.ucKeyId;
			} else {
				/* GC WEP Tx key ? */
				rCmdKey.ucWlanIndex = 255;
				ASSERT(FALSE);
			}
		}
	} else {
		if (((rCmdKey.aucPeerAddr[0] & rCmdKey.aucPeerAddr[1] &
			rCmdKey.aucPeerAddr[2] & rCmdKey.aucPeerAddr[3] &
			rCmdKey.aucPeerAddr[4] &
			rCmdKey.aucPeerAddr[5]) == 0xFF) ||
		    ((rCmdKey.aucPeerAddr[0] | rCmdKey.aucPeerAddr[1] |
		    rCmdKey.aucPeerAddr[2] | rCmdKey.aucPeerAddr[3] |
		    rCmdKey.aucPeerAddr[4] | rCmdKey.aucPeerAddr[5]) == 0x00)) {
			rCmdKey.ucWlanIndex = 255;	/* GC WEP ? */
			ASSERT(FALSE);
		} else {
			if (prStaRec) {	/* GC Rx RSN Group key */
				rCmdKey.ucWlanIndex =
					secPrivacySeekForBcEntry(prAdapter,
						prStaRec->ucBssIndex,
						prStaRec->aucMacAddr,
						prStaRec->ucIndex,
						rCmdKey.ucAlgorithmId,
						rCmdKey.ucKeyId);
				prStaRec->ucBMCWlanIndex = rCmdKey.ucWlanIndex;
				ASSERT(prStaRec->ucBMCWlanIndex < WTBL_SIZE);
			} else {	/* Exist this case ? */
				ASSERT(FALSE);
			}
		}
	}

	return wlanoidSendSetQueryP2PCmd(prAdapter,
					 CMD_ID_ADD_REMOVE_KEY,
					 prNewKey->ucBssIdx,
					 TRUE,
					 FALSE,
					 TRUE,
					 nicCmdEventSetCommon,
					 NULL,
					 sizeof(struct CMD_802_11_KEY),
					 (uint8_t *)&rCmdKey,
					 pvSetBuffer,
					 u4SetBufferLen);
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to request Wi-Fi Direct driver to remove keys
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
 * \retval WLAN_STATUS_INVALID_DATA
 */
/*---------------------------------------------------------------------------*/
uint32_t
wlanoidSetRemoveP2PKey(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct CMD_802_11_KEY rCmdKey;
	struct PARAM_REMOVE_KEY *prRemovedKey;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;

	ASSERT(prAdapter);

	if (u4SetBufferLen < sizeof(struct PARAM_REMOVE_KEY))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	prRemovedKey = (struct PARAM_REMOVE_KEY *) pvSetBuffer;

	/* Check bit 31: this bit should always 0 */
	if (prRemovedKey->u4KeyIndex & IS_TRANSMIT_KEY) {
		/* Bit 31 should not be set */
		DBGLOG(REQ, ERROR, "invalid key index: 0x%08lx\n",
				prRemovedKey->u4KeyIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Check bits 8 ~ 29 should always be 0 */
	if (prRemovedKey->u4KeyIndex & BITS(8, 29)) {
		/* Bit 31 should not be set */
		DBGLOG(REQ, ERROR, "invalid key index: 0x%08lx\n",
				prRemovedKey->u4KeyIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prRemovedKey->ucBssIdx);

	kalMemZero((uint8_t *)&rCmdKey, sizeof(struct CMD_802_11_KEY));

	rCmdKey.ucAddRemove = 0;	/* remove */
	/* group client */
	if (prBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
		rCmdKey.ucIsAuthenticator = 0;
	} else {		/* group owner */
		rCmdKey.ucIsAuthenticator = 1;
	}
	kalMemCopy(rCmdKey.aucPeerAddr,
		(uint8_t *) prRemovedKey->arBSSID, MAC_ADDR_LEN);
	rCmdKey.ucBssIdx = prRemovedKey->ucBssIdx;
	rCmdKey.ucKeyId = (uint8_t) (prRemovedKey->u4KeyIndex & 0x000000ff);

	/* Clean up the Tx key flag */
	prStaRec = cnmGetStaRecByAddress(prAdapter,
			prRemovedKey->ucBssIdx, prRemovedKey->arBSSID);

	/* mark for MR1 to avoid remove-key,
	 * but remove the wlan_tbl0 at the same time
	 */
	if (1 /*prRemovedKey->u4KeyIndex & IS_UNICAST_KEY */) {
		if (prStaRec) {
			rCmdKey.ucKeyType = 1;
			rCmdKey.ucWlanIndex = prStaRec->ucWlanIndex;
			prStaRec->fgTransmitKeyExist = FALSE;
		} else if (rCmdKey.ucIsAuthenticator)
			prBssInfo->fgBcDefaultKeyExist = FALSE;
	} else {
		if (rCmdKey.ucIsAuthenticator)
			prBssInfo->fgBcDefaultKeyExist = FALSE;
	}

	if (!prStaRec) {
		if (prAdapter->rWifiVar.rConnSettings.eAuthMode < AUTH_MODE_WPA
		    && prAdapter->rWifiVar.rConnSettings.eEncStatus
		    != ENUM_ENCRYPTION_DISABLED) {
			rCmdKey.ucWlanIndex = prBssInfo->ucBMCWlanIndex;
		} else {
			rCmdKey.ucWlanIndex = WTBL_RESERVED_ENTRY;
			return WLAN_STATUS_SUCCESS;
		}
	}

	/* mark for MR1 to avoid remove-key,
	 * but remove the wlan_tbl0 at the same time
	 */
	/* secPrivacyFreeForEntry(prAdapter, rCmdKey.ucWlanIndex); */

	return wlanoidSendSetQueryP2PCmd(prAdapter,
					 CMD_ID_ADD_REMOVE_KEY,
					 prRemovedKey->ucBssIdx,
					 TRUE,
					 FALSE,
					 TRUE,
					 nicCmdEventSetCommon,
					 NULL,
					 sizeof(struct CMD_802_11_KEY),
					 (uint8_t *)&rCmdKey,
					 pvSetBuffer,
					 u4SetBufferLen);
}
#endif

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
wlanoidSetP2pNetworkAddress(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t i, j;
	struct CMD_SET_NETWORK_ADDRESS_LIST *prCmdNWAddrList;
	struct PARAM_NETWORK_ADDRESS_LIST *prNWAddrList = pvSetBuffer;
	struct PARAM_NETWORK_ADDRESS *prNWAddress;
	struct PARAM_NETWORK_ADDRESS_IP *prNetAddrIp;
	uint32_t u4IpAddressCount, u4CmdSize;

	DBGLOG(INIT, TRACE, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = 4;

	if (u4SetBufferLen < sizeof(struct PARAM_NETWORK_ADDRESS_LIST))
		return WLAN_STATUS_INVALID_DATA;

	*pu4SetInfoLen = 0;
	u4IpAddressCount = 0;

	prNWAddress = (struct PARAM_NETWORK_ADDRESS *)(prNWAddrList + 1);
	for (i = 0; i < prNWAddrList->u4AddressCount; i++) {
		if (prNWAddress->u2AddressType
				== PARAM_PROTOCOL_ID_TCP_IP &&
			prNWAddress->u2AddressLength
				== sizeof(struct PARAM_NETWORK_ADDRESS_IP)) {
			u4IpAddressCount++;
		}

		prNWAddress = (struct PARAM_NETWORK_ADDRESS *)
			((uintptr_t) prNWAddress +
			(uintptr_t) (prNWAddress->u2AddressLength +
			OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress)));
	}

	/* construct payload of command packet */
	u4CmdSize =
		OFFSET_OF(struct CMD_SET_NETWORK_ADDRESS_LIST, arNetAddress) +
		sizeof(struct CMD_IPV4_NETWORK_ADDRESS) * u4IpAddressCount;

	prCmdNWAddrList = (struct CMD_SET_NETWORK_ADDRESS_LIST *)
		kalMemAlloc(u4CmdSize, VIR_MEM_TYPE);

	if (prCmdNWAddrList == NULL)
		return WLAN_STATUS_FAILURE;

	/* fill P_CMD_SET_NETWORK_ADDRESS_LIST */
	prCmdNWAddrList->ucBssIndex = prNWAddrList->ucBssIdx;
	prCmdNWAddrList->ucAddressCount = (uint8_t) u4IpAddressCount;
	prNWAddress = (struct PARAM_NETWORK_ADDRESS *)(prNWAddrList + 1);
	for (i = 0, j = 0; i < prNWAddrList->u4AddressCount; i++) {
		if (prNWAddress->u2AddressType
				== PARAM_PROTOCOL_ID_TCP_IP &&
			prNWAddress->u2AddressLength
				== sizeof(struct PARAM_NETWORK_ADDRESS_IP)) {
			prNetAddrIp = (struct PARAM_NETWORK_ADDRESS_IP *)
				prNWAddress->aucAddress;

			kalMemCopy(
				prCmdNWAddrList->arNetAddress[j].aucIpAddr,
				&(prNetAddrIp->in_addr), sizeof(uint32_t));

			j++;
		}

		prNWAddress = (struct PARAM_NETWORK_ADDRESS *)
			((uintptr_t) prNWAddress +
			(uintptr_t) (prNWAddress->u2AddressLength +
			OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress)));
	}

	rStatus = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_SET_IP_ADDRESS,
				      TRUE,
				      FALSE,
				      TRUE,
				      nicCmdEventSetIpAddress,
				      nicOidCmdTimeoutCommon,
				      u4CmdSize,
				      (uint8_t *) prCmdNWAddrList,
				      pvSetBuffer,
				      u4SetBufferLen);

	kalMemFree(prCmdNWAddrList, VIR_MEM_TYPE, u4CmdSize);
	return rStatus;
}

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
wlanoidQueryP2pPowerSaveProfile(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen != 0) {
		ASSERT(pvQueryBuffer);
		/* TODO: FIXME */
		/* *(enum PARAM_POWER_MODE *) pvQueryBuffer =
		 * (enum PARAM_POWER_MODE)
		 *(prWlanInfo->
		 *	arPowerSaveMode[prAdapter->ucP2PDevBssIdx].ucPsProfile);
		 */
		/* *pu4QueryInfoLen = sizeof(PARAM_POWER_MODE); */
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
wlanoidSetP2pPowerSaveProfile(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t status;
	enum PARAM_POWER_MODE ePowerMode;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(enum PARAM_POWER_MODE);
	if (u4SetBufferLen < sizeof(enum PARAM_POWER_MODE)) {
		DBGLOG(REQ, WARN, "Invalid length %u\n", u4SetBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	} else if (*(enum PARAM_POWER_MODE *)
			pvSetBuffer >= Param_PowerModeMax) {
		DBGLOG(REQ, WARN, "Invalid power mode %d\n",
			*(enum PARAM_POWER_MODE *) pvSetBuffer);
		return WLAN_STATUS_INVALID_DATA;
	}

	ePowerMode = *(enum PARAM_POWER_MODE *) pvSetBuffer;

	if (prAdapter->fgEnCtiaPowerMode) {
		if (ePowerMode == Param_PowerModeCAM) {
			/*Todo::  Nothing */
			/*Todo::  Nothing */
		} else {
			/* User setting to PS mode
			 *(Param_PowerModeMAX_PSP or Param_PowerModeFast_PSP)
			 */

			if (prAdapter->u4CtiaPowerMode == 0) {
				/* force to keep in CAM mode */
				ePowerMode = Param_PowerModeCAM;
			} else if (prAdapter->u4CtiaPowerMode == 1) {
				ePowerMode = Param_PowerModeMAX_PSP;
			} else if (prAdapter->u4CtiaPowerMode == 2) {
				ePowerMode = Param_PowerModeFast_PSP;
			}
		}
	}

	/* TODO: FIXME */
	status = nicConfigPowerSaveProfile(prAdapter, prAdapter->ucP2PDevBssIdx,
					   ePowerMode, TRUE, PS_CALLER_P2P);
	return status;
}				/* end of wlanoidSetP2pPowerSaveProfile() */

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
wlanoidSetP2pSetNetworkAddress(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t i, j;
	struct CMD_SET_NETWORK_ADDRESS_LIST *prCmdNWAddrList;
	struct PARAM_NETWORK_ADDRESS_LIST *prNWAddrList = pvSetBuffer;
	struct PARAM_NETWORK_ADDRESS *prNWAddress;
	struct PARAM_NETWORK_ADDRESS_IP *prNetAddrIp;
	uint32_t u4IpAddressCount, u4CmdSize;
	uint8_t *pucBuf = NULL;

	DBGLOG(INIT, TRACE, "\n");
	DBGLOG(INIT, DEBUG, "%s (%d)\n", __func__,
		(int16_t) u4SetBufferLen);

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = 4;

	if (u4SetBufferLen < sizeof(struct PARAM_NETWORK_ADDRESS_LIST))
		return WLAN_STATUS_INVALID_DATA;

	*pu4SetInfoLen = 0;
	u4IpAddressCount = 0;

	prNWAddress = (struct PARAM_NETWORK_ADDRESS *)(prNWAddrList + 1);
	for (i = 0; i < prNWAddrList->u4AddressCount; i++) {
		if (prNWAddress->u2AddressType
				== PARAM_PROTOCOL_ID_TCP_IP &&
		    prNWAddress->u2AddressLength
			== sizeof(struct PARAM_NETWORK_ADDRESS_IP)) {
			u4IpAddressCount++;
		}

		prNWAddress = (struct PARAM_NETWORK_ADDRESS *)
			((uintptr_t) prNWAddress +
			(uintptr_t) (prNWAddress->u2AddressLength +
			OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress)));
	}

	/* construct payload of command packet */
	u4CmdSize =
		OFFSET_OF(struct CMD_SET_NETWORK_ADDRESS_LIST, arNetAddress) +
	    sizeof(struct CMD_IPV4_NETWORK_ADDRESS) * u4IpAddressCount;

	if (u4IpAddressCount == 0)
		u4CmdSize = sizeof(struct CMD_SET_NETWORK_ADDRESS_LIST);

	prCmdNWAddrList = (struct CMD_SET_NETWORK_ADDRESS_LIST *)
		kalMemAlloc(u4CmdSize, VIR_MEM_TYPE);

	if (prCmdNWAddrList == NULL)
		return WLAN_STATUS_FAILURE;

	/* fill P_CMD_SET_NETWORK_ADDRESS_LIST */
	prCmdNWAddrList->ucBssIndex = prNWAddrList->ucBssIdx;

	/* only to set IP address to FW once ARP filter is enabled */
	if (prAdapter->fgEnArpFilter) {
		prCmdNWAddrList->ucAddressCount =
			(uint8_t) u4IpAddressCount;
		prNWAddress =
			(struct PARAM_NETWORK_ADDRESS *)(prNWAddrList + 1);

		DBGLOG(INIT, DEBUG, "u4IpAddressCount (%u)\n",
			(int32_t) u4IpAddressCount);

		for (i = 0, j = 0; i < prNWAddrList->u4AddressCount; i++) {
			if (prNWAddress->u2AddressType
					== PARAM_PROTOCOL_ID_TCP_IP &&
			    prNWAddress->u2AddressLength
				== sizeof(struct PARAM_NETWORK_ADDRESS_IP)) {

				prNetAddrIp =
					(struct PARAM_NETWORK_ADDRESS_IP *)
					prNWAddress->aucAddress;

				kalMemCopy(
					prCmdNWAddrList->arNetAddress[j]
						.aucIpAddr,
					&(prNetAddrIp->in_addr),
					sizeof(uint32_t));

				j++;

				pucBuf = (uint8_t *) &prNetAddrIp->in_addr;
				DBGLOG(INIT, DEBUG,
						"prNetAddrIp->in_addr:%d:%d:%d:%d\n",
						(uint8_t) pucBuf[0],
						(uint8_t) pucBuf[1],
						(uint8_t) pucBuf[2],
						(uint8_t) pucBuf[3]);
			}

			prNWAddress = (struct PARAM_NETWORK_ADDRESS *)
				((uintptr_t) prNWAddress +
				(uintptr_t) (prNWAddress->u2AddressLength +
				OFFSET_OF(struct PARAM_NETWORK_ADDRESS,
					aucAddress)));
		}

	} else {
		prCmdNWAddrList->ucAddressCount = 0;
	}

	rStatus = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_SET_IP_ADDRESS,
				      TRUE,
				      FALSE,
				      TRUE,
				      nicCmdEventSetIpAddress,
				      nicOidCmdTimeoutCommon,
				      u4CmdSize,
				      (uint8_t *) prCmdNWAddrList,
				      pvSetBuffer,
				      u4SetBufferLen);

	kalMemFree(prCmdNWAddrList, VIR_MEM_TYPE, u4CmdSize);
	return rStatus;
}				/* end of wlanoidSetP2pSetNetworkAddress() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to send GAS frame
 *          for P2P Service Discovery Request
 *
 * \param[in] prAdapter  Pointer to the Adapter structure.
 * \param[in] pvSetBuffer  Pointer to the buffer that holds the data to be set.
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
wlanoidSendP2PSDRequest(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(struct PARAM_P2P_SEND_SD_REQUEST)) {
		*pu4SetInfoLen = sizeof(struct PARAM_P2P_SEND_SD_REQUEST);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}
/* rWlanStatus = p2pFsmRunEventSDRequest(prAdapter
 * , (P_PARAM_P2P_SEND_SD_REQUEST)pvSetBuffer);
 */

	return rWlanStatus;
}				/* end of wlanoidSendP2PSDRequest() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to send GAS frame
 *          for P2P Service Discovery Response
 *
 * \param[in] prAdapter  Pointer to the Adapter structure.
 * \param[in] pvSetBuffer  Pointer to the buffer that holds the data to be set.
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
wlanoidSendP2PSDResponse(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

	if (u4SetBufferLen < sizeof(struct PARAM_P2P_SEND_SD_RESPONSE)) {
		*pu4SetInfoLen = sizeof(struct PARAM_P2P_SEND_SD_RESPONSE);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}
/* rWlanStatus = p2pFsmRunEventSDResponse(prAdapter
 * , (P_PARAM_P2P_SEND_SD_RESPONSE)pvSetBuffer);
 */

	return rWlanStatus;
}				/* end of wlanoidGetP2PSDRequest() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get GAS frame
 *          for P2P Service Discovery Request
 *
 * \param[in]  prAdapter  Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer  A pointer to the buffer that holds the result of
 *                          the query.
 * \param[in]  u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen  If the call is successful,
 *                          returns the number of
 *                          bytes written into the query buffer. If the call
 *                          failed due to invalid length of the query buffer,
 *                          returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidGetP2PSDRequest(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
/* PUINT_8 pucChannelNum = NULL; */
/* UINT_8 ucChannelNum = 0, ucSeqNum = 0; */

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < sizeof(struct PARAM_P2P_GET_SD_REQUEST)) {
		*pu4QueryInfoLen = sizeof(struct PARAM_P2P_GET_SD_REQUEST);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	DBGLOG(P2P, TRACE, "Get Service Discovery Request\n");

	*pu4QueryInfoLen = 0;
	return rWlanStatus;
}				/* end of wlanoidGetP2PSDRequest() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get GAS frame
 *          for P2P Service Discovery Response
 *
 * \param[in]  prAdapter        Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer    A pointer to the buffer that holds the result of
 *                          the query.
 * \param[in]  u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen  If the call is successful, returns the number of
 *                          bytes written into the query buffer. If the call
 *                          failed due to invalid length of the query buffer,
 *                          returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidGetP2PSDResponse(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	/* UINT_8 ucSeqNum = 0, */

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen < sizeof(struct PARAM_P2P_GET_SD_RESPONSE)) {
		*pu4QueryInfoLen = sizeof(struct PARAM_P2P_GET_SD_RESPONSE);
		return WLAN_STATUS_BUFFER_TOO_SHORT;
	}

	DBGLOG(P2P, TRACE, "Get Service Discovery Response\n");

	*pu4QueryInfoLen = 0;
	return rWlanStatus;
}				/* end of wlanoidGetP2PSDResponse() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to terminate P2P Service Discovery Phase
 *
 * \param[in] prAdapter  Pointer to the Adapter structure.
 * \param[in] pvSetBuffer  Pointer to the buffer that holds the data to be set.
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
wlanoidSetP2PTerminateSDPhase(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_P2P_TERMINATE_SD_PHASE *prP2pTerminateSD =
		(struct PARAM_P2P_TERMINATE_SD_PHASE *) NULL;
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;

	do {
		if ((prAdapter == NULL) || (pu4SetInfoLen == NULL))
			break;

		if ((u4SetBufferLen) && (pvSetBuffer == NULL))
			break;

		if (u4SetBufferLen
			< sizeof(struct PARAM_P2P_TERMINATE_SD_PHASE)) {

			*pu4SetInfoLen =
				sizeof(struct PARAM_P2P_TERMINATE_SD_PHASE);
			rWlanStatus = WLAN_STATUS_BUFFER_TOO_SHORT;
			break;
		}

		prP2pTerminateSD =
			(struct PARAM_P2P_TERMINATE_SD_PHASE *) pvSetBuffer;

		if (EQUAL_MAC_ADDR(prP2pTerminateSD->rPeerAddr, aucNullAddr)) {
			DBGLOG(P2P, TRACE, "Service Discovery Version 2.0\n");
/* p2pFuncSetVersionNumOfSD(prAdapter, 2); */
		}
		/* rWlanStatus = p2pFsmRunEventSDAbort(prAdapter); */

	} while (FALSE);

	return rWlanStatus;
}				/* end of wlanoidSetP2PTerminateSDPhase() */

#if CFG_SUPPORT_ANTI_PIRACY
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to
 *
 * \param[in] prAdapter  Pointer to the Adapter structure.
 * \param[in] pvSetBuffer  Pointer to the buffer that holds the data to be set.
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
wlanoidSetSecCheckRequest(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (u4SetBufferLen)
		ASSERT(pvSetBuffer);

#if 0  /* Comment it because CMD_ID_SEC_CHECK is not defined */
	return wlanoidSendSetQueryP2PCmd(prAdapter,
					 CMD_ID_SEC_CHECK,
					 prAdapter->ucP2PDevBssIdx,
					 FALSE,
					 TRUE,
					 TRUE,
					 NULL,
					 nicOidCmdTimeoutCommon,
					 u4SetBufferLen,
					 (uint8_t *) pvSetBuffer,
					 pvSetBuffer,
					 u4SetBufferLen);
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}				/* end of wlanoidSetSecCheckRequest() */

#if 0
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to
 *
 * \param[in]  prAdapter        Pointer to the Adapter structure.
 * \param[out] pvQueryBuffer    A pointer to the buffer that holds the result of
 *                              the query.
 * \param[in]  u4QueryBufferLen The length of the query buffer.
 * \param[out] pu4QueryInfoLen  If the call is successful, returns the number of
 *                              bytes written into the query buffer.
 *                              If the call failed due to invalid length
 *                              of the query buffer,
 *                              returns the amount of storage needed.
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 * \retval WLAN_STATUS_ADAPTER_NOT_READY
 * \retval WLAN_STATUS_MULTICAST_FULL
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanoidGetSecCheckResponse(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	/* P_WLAN_MAC_HEADER_T prWlanHdr = (P_WLAN_MAC_HEADER_T)NULL; */
	struct GLUE_INFO *prGlueInfo;

	prGlueInfo = prAdapter->prGlueInfo;

	ASSERT(prAdapter);
	ASSERT(pu4QueryInfoLen);

	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);

	if (u4QueryBufferLen > 256)
		u4QueryBufferLen = 256;

	*pu4QueryInfoLen = u4QueryBufferLen;

#if DBG
	DBGLOG_MEM8(SEC, LOUD,
		prGlueInfo->prP2PInfo[0]->aucSecCheckRsp,
		u4QueryBufferLen);
#endif
	kalMemCopy((uint8_t *)
		(pvQueryBuffer +
		OFFSET_OF(struct iw_p2p_transport_struct, aucBuffer)),
		prGlueInfo->prP2PInfo[0]->aucSecCheckRsp,
		u4QueryBufferLen);

	return rWlanStatus;
}				/* end of wlanoidGetSecCheckResponse() */
#endif
#endif

uint32_t
wlanoidSetNoaParam(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct PARAM_CUSTOM_NOA_PARAM_STRUCT *prNoaParam;
	struct CMD_CUSTOM_NOA_PARAM_STRUCT rCmdNoaParam;

	DBGLOG(INIT, TRACE, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_NOA_PARAM_STRUCT);

	if (u4SetBufferLen < sizeof(struct PARAM_CUSTOM_NOA_PARAM_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prNoaParam = (struct PARAM_CUSTOM_NOA_PARAM_STRUCT *) pvSetBuffer;

	kalMemZero(&rCmdNoaParam, sizeof(struct CMD_CUSTOM_NOA_PARAM_STRUCT));
	rCmdNoaParam.u4NoaDurationMs = prNoaParam->u4NoaDurationMs;
	rCmdNoaParam.u4NoaIntervalMs = prNoaParam->u4NoaIntervalMs;
	rCmdNoaParam.u4NoaCount = prNoaParam->u4NoaCount;
	rCmdNoaParam.ucBssIdx = prNoaParam->ucBssIdx;

#if 0
	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_NOA_PARAM,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_CUSTOM_NOA_PARAM_STRUCT),
				(uint8_t *) &rCmdNoaParam,
				pvSetBuffer,
				u4SetBufferLen);
#else
	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_NOA_PARAM,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_CUSTOM_NOA_PARAM_STRUCT),
				(uint8_t *) &rCmdNoaParam,
				pvSetBuffer,
				u4SetBufferLen);

#endif

}

uint32_t
wlanoidSetOppPsParam(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT *prOppPsParam;
	struct CMD_CUSTOM_OPPPS_PARAM_STRUCT rCmdOppPsParam;

	DBGLOG(INIT, TRACE, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT);

	if (u4SetBufferLen < sizeof(struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prOppPsParam = (struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT *) pvSetBuffer;

	kalMemZero(&rCmdOppPsParam,
		sizeof(struct CMD_CUSTOM_OPPPS_PARAM_STRUCT));
	rCmdOppPsParam.u4CTwindowMs = prOppPsParam->u4CTwindowMs;
	rCmdOppPsParam.ucBssIdx = prOppPsParam->ucBssIdx;

#if 0
	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_OPPPS_PARAM,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_CUSTOM_OPPPS_PARAM_STRUCT),
				(uint8_t *) &rCmdOppPsParam,
				pvSetBuffer,
				u4SetBufferLen);
#else
	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_OPPPS_PARAM,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_CUSTOM_OPPPS_PARAM_STRUCT),
				(uint8_t *) &rCmdOppPsParam,
				pvSetBuffer,
				u4SetBufferLen);

#endif

}

uint32_t
wlanoidSetUApsdParam(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT *prUapsdParam;
	struct CMD_CUSTOM_UAPSD_PARAM_STRUCT rCmdUapsdParam;
	struct PM_PROFILE_SETUP_INFO *prPmProfSetupInfo;
	struct BSS_INFO *prBssInfo;
	u_int8_t fgIsOid = TRUE;

	DBGLOG(INIT, TRACE, "\n");

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT);

	if (u4SetBufferLen < sizeof(struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);

	prUapsdParam = (struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT *) pvSetBuffer;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prUapsdParam->ucBssIdx);
	if (!prBssInfo)
		return WLAN_STATUS_FAILURE;
	prPmProfSetupInfo = &prBssInfo->rPmProfSetupInfo;

	kalMemZero(&rCmdUapsdParam,
		sizeof(struct CMD_CUSTOM_UAPSD_PARAM_STRUCT));

	rCmdUapsdParam.fgEnAPSD = prUapsdParam->fgEnAPSD;

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

#if CFG_SUPPORT_MULTITHREAD
#if CFG_MTK_ANDROID_WMT
	if (prAdapter->prGlueInfo)
		fgIsOid = (prAdapter->prGlueInfo->u4TxThreadPid
				!= KAL_GET_CURRENT_THREAD_ID());
#endif
#endif

#if 0
	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_UAPSD_PARAM,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_CUSTOM_UAPSD_PARAM_STRUCT),
				(uint8_t *) &rCmdUapsdParam,
				pvSetBuffer,
				u4SetBufferLen);
#else
	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_UAPSD_PARAM,
				TRUE,
				FALSE,
				fgIsOid,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_CUSTOM_UAPSD_PARAM_STRUCT),
				(uint8_t *) &rCmdUapsdParam,
				pvSetBuffer,
				u4SetBufferLen);

#endif
}

uint32_t
wlanoidQueryP2pVersion(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	uint32_t rResult = WLAN_STATUS_FAILURE;
/* PUINT_8 pucVersionNum = (PUINT_8)pvQueryBuffer; */

	do {
		if ((prAdapter == NULL) || (pu4QueryInfoLen == NULL))
			break;

		if ((u4QueryBufferLen) && (pvQueryBuffer == NULL))
			break;

		if (u4QueryBufferLen < sizeof(uint8_t)) {
			*pu4QueryInfoLen = sizeof(uint8_t);
			rResult = WLAN_STATUS_BUFFER_TOO_SHORT;
			break;
		}

	} while (FALSE);

	return rResult;
}				/* wlanoidQueryP2pVersion */

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to set the WPS mode.
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
wlanoidSetP2pWPSmode(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t status;
	uint32_t u4IsWPSmode = 0;
	int i = 0;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	if (pvSetBuffer)
		u4IsWPSmode = *(uint32_t *) pvSetBuffer;
	else
		u4IsWPSmode = 0;

	/* set all Role to the same value */
	for (i = 0; i < KAL_P2P_NUM; i++)
		if (u4IsWPSmode)
			prAdapter->rWifiVar.prP2PConnSettings[i]->fgIsWPSMode
				= 1;
		else
			prAdapter->rWifiVar.prP2PConnSettings[i]->fgIsWPSMode
				= 0;

	status = nicUpdateBss(prAdapter, prAdapter->ucP2PDevBssIdx);

	return status;
}				/* end of wlanoidSetP2pWPSmode() */

#endif

uint32_t
wlanoidSetP2pSupplicantVersion(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen)
{
	uint32_t rResult;
	uint8_t ucVersionNum;

	do {
		if ((prAdapter == NULL) || (pu4SetInfoLen == NULL)) {

			rResult = WLAN_STATUS_INVALID_DATA;
			break;
		}

		if ((u4SetBufferLen) && (pvSetBuffer == NULL)) {
			rResult = WLAN_STATUS_INVALID_DATA;
			break;
		}

		*pu4SetInfoLen = sizeof(uint8_t);

		if (u4SetBufferLen < sizeof(uint8_t)) {
			rResult = WLAN_STATUS_INVALID_LENGTH;
			break;
		}

		ucVersionNum = *((uint8_t *) pvSetBuffer);

		rResult = WLAN_STATUS_SUCCESS;
	} while (FALSE);

	return rResult;
}				/* wlanoidSetP2pSupplicantVersion */

uint32_t
wlanoidRequestP2pScan(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	struct MSG_HDR *prMsgHdr = pvQueryBuffer;

	if (!prMsgHdr)
		return WLAN_STATUS_INVALID_DATA;

	p2pFsmRunEventScanRequest(prAdapter, prMsgHdr);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidAbortP2pScan(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	uint8_t ucBssIdx;

	ASSERT(prAdapter);

	ucBssIdx = *((uint8_t *) pvQueryBuffer);

	if (ucBssIdx == prAdapter->ucP2PDevBssIdx)
		p2pDevFsmRunEventScanAbort(prAdapter, ucBssIdx);
	else
		p2pRoleFsmRunEventScanAbort(prAdapter, ucBssIdx);

	return WLAN_STATUS_SUCCESS;
}
#endif
