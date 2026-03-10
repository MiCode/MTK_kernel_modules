/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

/*! \file   dvt_phtput.c
 *  \brief  Functions that provide operation in NIC's (Network Interface Card)
 *          point of view.
 *    This file includes functions which unite multiple hal(Hardware) operations
 *    and also take the responsibility of Software Resource Management in order
 *    to keep the synchronization with Hardware Manipulation.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
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
struct PhTputSetting g_rPhTputMap[] = {
	/*STA*/
	{ENUM_PHTPUT_LEGACY_OPEN_STA, FALSE},
	{ENUM_PHTPUT_LEGACY_SEC_GCMP256_STA, TRUE},
	{ENUM_PHTPUT_LEGACY_SEC_WEP128_STA, TRUE},
	{ENUM_PHTPUT_LEGACY_SEC_TKIP_STA, TRUE},
	{ENUM_PHTPUT_LEGACY_SEC_AES_STA, TRUE},
	{ENUM_PHTPUT_MLO_OPEN_BN0_BN2_STA, FALSE},
	{ENUM_PHTPUT_MLO_SEC_BN0_BN2_STA, TRUE},
	{ENUM_PHTPUT_MLO_OPEN_BN0_BN1_STA, FALSE},
	{ENUM_PHTPUT_MLO_SEC_BN0_BN1_STA, TRUE},
	{ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_STA, FALSE},
	{ENUM_PHTPUT_DBDC_SEC_BN0_BN2_STA, TRUE},
	{ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_STA, FALSE},
	{ENUM_PHTPUT_DBDC_SEC_BN0_BN1_STA, TRUE},
	/*AP*/
	{ENUM_PHTPUT_LEGACY_OPEN_AP, FALSE},
	{ENUM_PHTPUT_LEGACY_SEC_GCMP256_AP, TRUE},
	{ENUM_PHTPUT_LEGACY_SEC_WEP128_AP, TRUE},
	{ENUM_PHTPUT_LEGACY_SEC_TKIP_AP, TRUE},
	{ENUM_PHTPUT_LEGACY_SEC_AES_AP, TRUE},
	{ENUM_PHTPUT_MLO_OPEN_BN0_BN2_AP, FALSE},
	{ENUM_PHTPUT_MLO_SEC_BN0_BN2_AP, TRUE},
	{ENUM_PHTPUT_MLO_OPEN_BN0_BN1_AP, FALSE},
	{ENUM_PHTPUT_MLO_SEC_BN0_BN1_AP, TRUE},
	{ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_AP, FALSE},
	{ENUM_PHTPUT_DBDC_SEC_BN0_BN2_AP, TRUE},
	{ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_AP, FALSE},
	{ENUM_PHTPUT_DBDC_SEC_BN0_BN1_AP, TRUE},
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/* NETWORK (WIFISYS) */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to setup WIFISYS for specified
 *        network for PH-Tput based on case_id
 *
 * @param prNetDev           Pointer of net_device
 *        u4CaseIndex        Index of PH-Tput test case
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t dvtSetupPhTput(struct net_device *prNetDev,
			uint32_t u4CaseIndex)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct BSS_INFO *prBssInfo;
	uint16_t ucMapIdx, ucPhTputMapSize;
	struct PhTputSetting *prPhtputSetting = NULL;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, 0);

	ucPhTputMapSize = sizeof(g_rPhTputMap) / sizeof(*prPhtputSetting);

	for (ucMapIdx = 0; ucMapIdx < ucPhTputMapSize; ucMapIdx++) {
		if (u4CaseIndex == g_rPhTputMap[ucMapIdx].u2CmdId)
			prPhtputSetting = &g_rPhTputMap[ucMapIdx];
	}

	if (prPhtputSetting == NULL) {
		DBGLOG(CNM, WARN, "No Match Case ID[%d]\n", u4CaseIndex);
		return 0;
	}

	switch (prPhtputSetting->u2CmdId) {
	case ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_STA:
	case ENUM_PHTPUT_DBDC_SEC_BN0_BN2_STA:
	case ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_STA:
	case ENUM_PHTPUT_DBDC_SEC_BN0_BN1_STA:
	case ENUM_PHTPUT_DBDC_OPEN_BN0_BN2_AP:
	case ENUM_PHTPUT_DBDC_SEC_BN0_BN2_AP:
	case ENUM_PHTPUT_DBDC_OPEN_BN0_BN1_AP:
	case ENUM_PHTPUT_DBDC_SEC_BN0_BN1_AP:
		if (prBssInfo->fgIsNetActive == FALSE) {
			dvtActivateNetworkPhTput(prNetDev, 0, prPhtputSetting);
			dvtActivateNetworkPhTput(prNetDev, 2, prPhtputSetting);
		} else {
			dvtDeactivateNetworkPhTput(prNetDev, 0);
			dvtDeactivateNetworkPhTput(prNetDev, 2);
		}
		break;
	default:
		if (prBssInfo->fgIsNetActive == FALSE) {
			dvtActivateNetworkPhTput(prNetDev, 0, prPhtputSetting);
		} else {
			dvtDeactivateNetworkPhTput(prNetDev, 0);
		}
		break;
	}
	return 0;
}

/* NETWORK (WIFISYS) */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to activate WIFISYS for specified
 *        network for PH-Tput
 *
 * @param prNetDev	     Pointer of net_device
 *        ucBssIndex         Index of Bss to activate
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t dvtActivateNetworkPhTput(struct net_device *prNetDev,
			uint8_t ucBssIndex,
			struct PhTputSetting *prPhtputSetting)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct net_device *prDevHandler;
	struct WIFI_VAR *prWifiVar = NULL;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct WLAN_TABLE *prWtbl;
	uint16_t i;
	uint8_t aucBSSID[6];
	uint8_t aucStaRecMacAddr[6];
	uint8_t ucStaRecIdx = ucBssIndex;
	uint8_t ucWlanIdx = ucBssIndex == 0 ? 1 : 2;
	uint8_t ucTid;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
        prAdapter = prGlueInfo->prAdapter;

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucHwBssIdNum);

	/* setup BssInfo */
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prWifiVar = &prAdapter->rWifiVar;

	kalMemCopy(aucStaRecMacAddr, prWifiVar->aucMacAddress, 6);
	kalMemCopy(aucBSSID, prWifiVar->aucMacAddress, 6);
	aucStaRecMacAddr[2] = aucStaRecMacAddr[2] == 'S' ? 'A' : 'S';
	aucStaRecMacAddr[3] = ucBssIndex == 0 ? 0x33 : 0x31; //DBDC
	aucBSSID[2] = 'A';
	aucBSSID[3] = ucBssIndex == 0 ? 0x33 : 0x31; //DBDC

	kalMemCopy(prBssInfo->aucOwnMacAddr, prWifiVar->aucMacAddress, 6);
	kalMemCopy(prBssInfo->aucBSSID, aucBSSID, 6);
	prBssInfo->aucOwnMacAddr[3] = ucBssIndex == 0 ? 0x33 : 0x31;//DBDC
	prBssInfo->eNetworkType = NETWORK_TYPE_AIS;
	prBssInfo->eConnectionState = MEDIA_STATE_CONNECTED;
	prBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;
	prBssInfo->ucBssIndex = ucBssIndex;
	prBssInfo->ucWmmQueSet = ucBssIndex == 0 ? 0 : 1; //DBDC
	prBssInfo->ucOwnMacIndex = 0;
	prBssInfo->fgIsNetActive = TRUE;
	prBssInfo->fgIsQBSS = TRUE;
	prBssInfo->eBand = ucBssIndex == 0 ? BAND_5G : BAND_2G4; //DBDC
	prBssInfo->ucPrimaryChannel = ucBssIndex == 0 ? 36 : 6; //DBDC

	/* setup StaRec */
	prStaRec = &prAdapter->arStaRec[ucStaRecIdx];
	if (!prStaRec->fgIsInUse) {
		kalMemZero(prStaRec, sizeof(struct STA_RECORD));
	}
	kalMemCopy(prStaRec->aucMacAddr, aucStaRecMacAddr, 6);
	prStaRec->ucRCPI = 0;
	prStaRec->ucIndex = ucBssIndex;
	prStaRec->ucWlanIndex = ucWlanIdx;
	prStaRec->ucBssIndex = ucBssIndex; //DBDC
	prStaRec->ucStaState = STA_STATE_3;
	prStaRec->u2AssocId = ucBssIndex;
	prStaRec->fgIsInUse = TRUE;
	prStaRec->fgIsValid = TRUE;
	prStaRec->fgIsQoS = TRUE;
	qmSetStaPS(prAdapter, prStaRec, FALSE);
	prStaRec->fgIsTxAllowed = TRUE;

	if (prPhtputSetting->fgIsSec) {
		prStaRec->fgTransmitKeyExist = TRUE;
		prAdapter->rWifiVar.rAisFsmInfo[prBssInfo->u4PrivateData]
			.rConnSettings.eEncStatus = ENUM_ENCRYPTION4_ENABLED;
	} else {
		prStaRec->fgTransmitKeyExist = FALSE;
		prAdapter->rWifiVar.rAisFsmInfo[prBssInfo->u4PrivateData]
			.rConnSettings.eEncStatus = ENUM_ENCRYPTION_DISABLED;
	}

	for (i = 0; i < NUM_OF_PER_STA_TX_QUEUES; i++) {
		QUEUE_INITIALIZE(&prStaRec->arTxQueue[i]);
		QUEUE_INITIALIZE(&prStaRec->arPendingTxQueue[i]);
		prStaRec->aprTargetQueue[i] = &prStaRec->arTxQueue[i];
	}

	/* setup WTBL */
	prWtbl = prAdapter->rWifiVar.arWtbl;
	prWtbl[ucWlanIdx].ucUsed = TRUE;
	prWtbl[ucWlanIdx].ucStaIndex = ucStaRecIdx;
	prWtbl[ucWlanIdx].ucBssIndex = ucBssIndex;
	kalMemCopy(prWtbl[ucWlanIdx].aucMacAddr, aucStaRecMacAddr, 6);

	/* netif_carrier_on */
	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex); //DBDC
	netif_carrier_on(prDevHandler);

	/* setup TEMP TXD */
	nicTxGenerateDescTemplate(prAdapter, prStaRec);

	for (ucTid = 0; ucTid < TX_DESC_TID_NUM; ucTid++) {
		nicTxSetHwAmsduDescTemplate(prAdapter, prStaRec, ucTid,
					    TRUE);
	}

	return 0;
}

/* NETWORK (WIFISYS) */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This utility function is used to deactivate WIFISYS for specified
 *        network for PH-Tput
 *
 * @param prNetDev	     Pointer of net_device
 *        ucBssIndex         Index of Bss to deactivate
 *
 * @retval -
 */
/*----------------------------------------------------------------------------*/
uint32_t dvtDeactivateNetworkPhTput(struct net_device *prNetDev,
		uint8_t ucBssIndex)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct net_device *prDevHandler;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	struct WLAN_TABLE *prWtbl;
	uint8_t ucStaRecIdx = ucBssIndex;
	uint8_t ucWlanIdx = ucBssIndex + 1;

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucHwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prBssInfo->fgIsNetActive = FALSE;

	prStaRec = &prAdapter->arStaRec[ucStaRecIdx];
	prStaRec->fgIsInUse = FALSE;

	prWtbl = prAdapter->rWifiVar.arWtbl;
	prWtbl[ucWlanIdx].ucUsed = FALSE;

	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
	netif_carrier_off(prDevHandler);

	return 0;
}


