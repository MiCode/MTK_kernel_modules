// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_p2p_kal.c
 *    \brief
 *
 */

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "net/cfg80211.h"
#include "precomp.h"
#include "gl_wext.h"

#if CFG_ENABLE_WIFI_DIRECT
#if CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
#include <net/cnss_utils.h>
#include <linux/dev_ril_bridge.h>
#endif

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
struct ieee80211_channel *kalP2pFuncGetChannelEntry(
		struct GLUE_INFO *prGlueInfo,
		struct GL_P2P_INFO *prP2pInfo,
		struct RF_CHANNEL_INFO *prChannelInfo);

static inline enum nl80211_chan_width
__kalP2pGetNl80211ChnlBw(struct RF_CHANNEL_INFO *prRfChnlInfo);

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

/*---------------------------------------------------------------------------*/
/*!
 * \brief to update the assoc req to p2p
 *
 * \param[in]
 *           prGlueInfo
 *           pucFrameBody
 *           u4FrameBodyLen
 *           fgReassocRequest
 * \return
 *           none
 */
/*---------------------------------------------------------------------------*/
void
kalP2PUpdateAssocInfo(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucFrameBody,
		uint32_t u4FrameBodyLen,
		u_int8_t fgReassocRequest,
		uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	union iwreq_data wrqu;
	unsigned char *pucExtraInfo = NULL;
	unsigned char *pucDesiredIE = NULL;
/* unsigned char aucExtraInfoBuf[200]; */
	uint8_t *cp;
	struct net_device *prNetdevice = (struct net_device *)NULL;

	memset(&wrqu, 0, sizeof(wrqu));

	if (fgReassocRequest) {
		if (u4FrameBodyLen < 15) {
			return;
		}
	} else {
		if (u4FrameBodyLen < 9) {
			return;
		}
	}

	cp = pucFrameBody;

	if (fgReassocRequest) {
		/* Capability information field 2 */
		/* Listen interval field 2 */
		/* Current AP address 6 */
		cp += 10;
		u4FrameBodyLen -= 10;
	} else {
		/* Capability information field 2 */
		/* Listen interval field 2 */
		cp += 4;
		u4FrameBodyLen -= 4;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
	if (!prBssInfo)
		return;
	if (u4FrameBodyLen <= CFG_CFG80211_IE_BUF_LEN &&
		u4FrameBodyLen <= MAX_IE_LENGTH &&
		!IS_BSS_APGO(prBssInfo)) {
		struct P2P_ROLE_FSM_INFO *fsm =
			P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
				prGlueInfo->prAdapter,
				prBssInfo->u4PrivateData);

		if (!fsm)
			return;

		DBGLOG(P2P, LOUD,
			"[%d] Copy assoc req info\n", ucBssIndex);

		fsm->rConnReqInfo.u4BufLength = u4FrameBodyLen;
		kalMemCopy(fsm->rConnReqInfo.aucIEBuf, cp, u4FrameBodyLen);
	}
#if CFG_SUPPORT_WPS
	/* do supplicant a favor, parse to the start of WPA/RSN IE */
	if (wextSrchDesiredWPSIE(cp, u4FrameBodyLen, 0xDD, &pucDesiredIE)) {
		/* WPS IE found */
	} else {
#endif
		if (wextSrchDesiredWPAIE(cp,
				u4FrameBodyLen, 0x30, &pucDesiredIE)) {
			/* RSN IE found */
		} else if (wextSrchDesiredWPAIE(cp,
				u4FrameBodyLen, 0xDD, &pucDesiredIE)) {
			/* WPA IE found */
		} else {
			/* no WPA/RSN IE found, skip this event */
			return;
		}
#if CFG_SUPPORT_WPS
	}
#endif

	/* IWEVASSOCREQIE, indicate binary string */
	pucExtraInfo = pucDesiredIE;
	wrqu.data.length = pucDesiredIE[1] + 2;


	if (ucBssIndex == prGlueInfo->prAdapter->ucP2PDevBssIdx)
		prNetdevice = prGlueInfo->prP2PInfo
			[prBssInfo->u4PrivateData]->prDevHandler;
	else
		prNetdevice = prGlueInfo->prP2PInfo
			[prBssInfo->u4PrivateData]->aprRoleHandler;
	if (!prNetdevice)
		return;
	/* Send event to user space */
	wireless_send_event(prNetdevice, IWEVASSOCREQIE, &wrqu, pucExtraInfo);
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to set Wi-Fi Direct state in glue layer
 *
 * \param[in]
 *           prGlueInfo
 *           eBowState
 *           rPeerAddr
 * \return
 *           none
 */
/*---------------------------------------------------------------------------*/
#if 0
void
kalP2PSetState(struct GLUE_INFO *prGlueInfo,
		enum ENUM_PARAM_MEDIA_STATE eState,
		uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN],
		uint8_t ucRole)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];

	ASSERT(prGlueInfo);

	memset(&evt, 0, sizeof(evt));

	if (eState == MEDIA_STATE_CONNECTED) {
		prGlueInfo->prP2PInfo[0]->eState = MEDIA_STATE_CONNECTED;

		snprintf(aucBuffer, IW_CUSTOM_MAX - 1,
			"P2P_STA_CONNECT=" MACSTR, MAC2STR(rPeerAddr));
		evt.data.length = strlen(aucBuffer);

		/* indicate in IWECUSTOM event */
		wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
			IWEVCUSTOM, &evt, aucBuffer);

	} else if (eState == MEDIA_STATE_DISCONNECTED) {
		snprintf(aucBuffer, IW_CUSTOM_MAX - 1,
			"P2P_STA_DISCONNECT=" MACSTR, MAC2STR(rPeerAddr));
		evt.data.length = strlen(aucBuffer);

		/* indicate in IWECUSTOM event */
		wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
			IWEVCUSTOM, &evt, aucBuffer);
	} else {
		ASSERT(0);
	}

}				/* end of kalP2PSetState() */
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief to retrieve Wi-Fi Direct operating frequency
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           in unit of KHz
 */
/*---------------------------------------------------------------------------*/
#if 0
uint32_t kalP2PGetFreqInKHz(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);

	return prGlueInfo->prP2PInfo[0]->u4FreqInKHz;
}				/* end of kalP2PGetFreqInKHz() */
#endif

/*---------------------------------------------------------------------------*/
/*!
 * \brief to retrieve Bluetooth-over-Wi-Fi role
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           0: P2P Device
 *           1: Group Client
 *           2: Group Owner
 */
/*----------------------------------------------------------------------------*/
uint8_t kalP2PGetRole(struct GLUE_INFO *prGlueInfo, uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);

	if (!prGlueInfo->prP2PInfo[ucRoleIdx])
		return 0;

	return prGlueInfo->prP2PInfo[ucRoleIdx]->ucRole;
}				/* end of kalP2PGetRole() */

/*---------------------------------------------------------------------------*/
/*!
 * \brief to set Wi-Fi Direct role
 *
 * \param[in]
 *           prGlueInfo
 *           ucResult
 *                   0: successful
 *                   1: error
 *           ucRole
 *                   0: P2P Device
 *                   1: Group Client
 *                   2: Group Owner
 *
 * \return
 *           none
 */
/*---------------------------------------------------------------------------*/
#if 1
void kalP2PSetRole(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRole, uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);
	if (ucRole > OP_MODE_ACCESS_POINT) {
		DBGLOG(P2P, ERROR, "Invalid RoleIdx %d\n", ucRoleIdx);
		return;
	}
	prGlueInfo->prP2PInfo[ucRoleIdx]->ucRole = ucRole;
	/* Remove non-used code */
}				/* end of kalP2PSetRole() */

#else
void
kalP2PSetRole(struct GLUE_INFO *prGlueInfo,
		uint8_t ucResult, uint8_t *pucSSID,
		uint8_t ucSSIDLen, uint8_t ucRole)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];

	ASSERT(prGlueInfo);
	ASSERT(ucRole <= 2);

	memset(&evt, 0, sizeof(evt));

	if (ucResult == 0)
		prGlueInfo->prP2PInfo[0]->ucRole = ucRole;

	if (pucSSID)
		snprintf(aucBuffer, IW_CUSTOM_MAX - 1,
			"P2P_FORMATION_RST=%d%d%d%c%c", ucResult,
			ucRole, 1 /* persistence or not */,
			pucSSID[7], pucSSID[8]);
	else
		snprintf(aucBuffer, IW_CUSTOM_MAX - 1,
			"P2P_FORMATION_RST=%d%d%d%c%c", ucResult,
			ucRole, 1 /* persistence or not */, '0', '0');

	evt.data.length = strlen(aucBuffer);

	/* indicate in IWECUSTOM event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);

}				/* end of kalP2PSetRole() */

#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief to set the cipher for p2p
 *
 * \param[in]
 *           prGlueInfo
 *           u4Cipher
 *
 * \return
 *           none
 */
/*---------------------------------------------------------------------------*/
void kalP2PSetCipher(struct GLUE_INFO *prGlueInfo,
		uint32_t u4Cipher, uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

	/* It can be WEP40 (used to identify cipher is WEP), TKIP and CCMP */
	prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise = u4Cipher;

}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to get the cipher, return false for security is none
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           TRUE: cipher is ccmp
 *           FALSE: cipher is none
 */
/*---------------------------------------------------------------------------*/
u_int8_t kalP2PGetCipher(struct GLUE_INFO *prGlueInfo, uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_CCMP)
		return TRUE;

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_TKIP)
		return TRUE;

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_WEP40)
		return TRUE;

	return FALSE;
}

u_int8_t kalP2PGetWepCipher(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_WEP40)
		return TRUE;

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_WEP104)
		return TRUE;

	return FALSE;
}

u_int8_t kalP2PGetCcmpCipher(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

	DBGLOG(P2P, TRACE,
		"P2P get ccmp cipher: %d\n",
		prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise);

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_CCMP)
		return TRUE;

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_TKIP)
		return FALSE;

	return FALSE;
}

u_int8_t kalP2PGetTkipCipher(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PInfo[ucRoleIdx]);

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_CCMP)
		return FALSE;

	if (prGlueInfo->prP2PInfo[ucRoleIdx]->u4CipherPairwise
		== IW_AUTH_CIPHER_TKIP)
		return TRUE;

	return FALSE;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to set the status of WSC
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*---------------------------------------------------------------------------*/
void kalP2PSetWscMode(struct GLUE_INFO *prGlueInfo, uint8_t ucWscMode)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PDevInfo);

	prGlueInfo->prP2PDevInfo->ucWSCRunning = ucWscMode;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to get the status of WSC
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*---------------------------------------------------------------------------*/
uint8_t kalP2PGetWscMode(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PDevInfo);

	return prGlueInfo->prP2PDevInfo->ucWSCRunning;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to get the wsc ie length
 *
 * \param[in]
 *           prGlueInfo
 *           ucType : 0 for beacon, 1 for probe req, 2 for probe resp
 *
 * \return
 *           The WSC IE length
 */
/*---------------------------------------------------------------------------*/
uint16_t kalP2PCalWSC_IELen(struct GLUE_INFO *prGlueInfo,
		uint8_t ucType, uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);

	if (ucRoleIdx >= KAL_P2P_NUM) {
		DBGLOG(P2P, ERROR, "Invalid RoleIdx %d\n", ucRoleIdx);
		return 0;
	}
	if (ucType >= 4) {
		DBGLOG(P2P, ERROR, "Invalid Type %d\n", ucType);
		return 0;
	}

	return prGlueInfo->prP2PInfo[ucRoleIdx]->u2WSCIELen[ucType];
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to copy the wsc ie setting from p2p supplicant
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 *           The WPS IE length
 */
/*---------------------------------------------------------------------------*/
void kalP2PGenWSC_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucType, uint8_t *pucBuffer, uint8_t ucRoleIdx)
{
	struct GL_P2P_INFO *prGlP2pInfo = (struct GL_P2P_INFO *) NULL;

	do {
		if ((prGlueInfo == NULL)
			|| (ucType >= 4) || (pucBuffer == NULL))
			break;

		prGlP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

		kalMemCopy(pucBuffer,
			prGlP2pInfo->aucWSCIE[ucType],
			prGlP2pInfo->u2WSCIELen[ucType]);

	} while (FALSE);

}

void kalP2PUpdateWSC_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucType, uint8_t *pucBuffer,
		uint16_t u2BufferLength, uint8_t ucRoleIdx)
{
	struct GL_P2P_INFO *prGlP2pInfo = (struct GL_P2P_INFO *) NULL;

	do {
		if ((prGlueInfo == NULL) || (ucType >= 4)
			|| ((u2BufferLength > 0) && (pucBuffer == NULL)))
			break;

		if (u2BufferLength > 400) {
			log_dbg(P2P, ERROR,
				"Buffer length is not enough, GLUE only 400 bytes but %d received\n",
				u2BufferLength);
			ASSERT(FALSE);
			break;
		}

		prGlP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

		kalMemCopy(prGlP2pInfo->aucWSCIE[ucType],
			pucBuffer, u2BufferLength);

		prGlP2pInfo->u2WSCIELen[ucType] = u2BufferLength;

	} while (FALSE);

}				/* kalP2PUpdateWSC_IE */

uint16_t kalP2PCalP2P_IELen(struct GLUE_INFO *prGlueInfo,
		uint8_t ucIndex, uint8_t ucRoleIdx)
{
	ASSERT(prGlueInfo);
	if (ucRoleIdx >= KAL_P2P_NUM) {
		DBGLOG(P2P, ERROR, "Invalid RoleIdx %d\n", ucRoleIdx);
		return 0;
	}
	if (ucIndex >= MAX_P2P_IE_SIZE) {
		DBGLOG(P2P, ERROR, "Invalid IE len Idx %d\n", ucIndex);
		return 0;
	}

	return prGlueInfo->prP2PInfo[ucRoleIdx]->u2P2PIELen[ucIndex];
}

void kalP2PTxCarrierOn(struct GLUE_INFO *prGlueInfo,
		struct BSS_INFO *prBssInfo)
{
	struct net_device *prDevHandler = NULL;
	uint8_t ucBssIndex = (uint8_t)prBssInfo->ucBssIndex;

	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
	if (prDevHandler == NULL)
		return;

	if (!netif_carrier_ok(prDevHandler)) {
		netif_carrier_on(prDevHandler);
		netif_tx_start_all_queues(prDevHandler);
	}
}

uint8_t kalP2PIsTxCarrierOn(struct GLUE_INFO *prGlueInfo,
		struct BSS_INFO *prBssInfo)
{
	struct net_device *prDevHandler = NULL;
	uint8_t ucBssIndex = (uint8_t)prBssInfo->ucBssIndex;

	prDevHandler = wlanGetNetDev(prGlueInfo, ucBssIndex);
	if (prDevHandler == NULL)
		return FALSE;

	return netif_carrier_ok(prDevHandler);
}

void kalP2PGenP2P_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucIndex, uint8_t *pucBuffer, uint8_t ucRoleIdx)
{
	struct GL_P2P_INFO *prGlP2pInfo = (struct GL_P2P_INFO *) NULL;

	do {
		if ((prGlueInfo == NULL) || (ucIndex >= MAX_P2P_IE_SIZE)
			|| (pucBuffer == NULL))
			break;

		prGlP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

		kalMemCopy(pucBuffer,
			prGlP2pInfo->aucP2PIE[ucIndex],
			prGlP2pInfo->u2P2PIELen[ucIndex]);

	} while (FALSE);
}

void kalP2PUpdateP2P_IE(struct GLUE_INFO *prGlueInfo,
		uint8_t ucIndex, uint8_t *pucBuffer,
		uint16_t u2BufferLength, uint8_t ucRoleIdx)
{
	struct GL_P2P_INFO *prGlP2pInfo = (struct GL_P2P_INFO *) NULL;

	do {
		if ((prGlueInfo == NULL) ||
			(ucIndex >= MAX_P2P_IE_SIZE) ||
			((u2BufferLength > 0) && (pucBuffer == NULL)))
			break;

		if (u2BufferLength > 400) {
			log_dbg(P2P, ERROR,
			       "kalP2PUpdateP2P_IE > Buffer length is not enough, GLUE only 400 bytes but %d received\n",
					u2BufferLength);
			ASSERT(FALSE);
			break;
		}

		prGlP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

		kalMemCopy(prGlP2pInfo->aucP2PIE[ucIndex],
			pucBuffer, u2BufferLength);

		prGlP2pInfo->u2P2PIELen[ucIndex] = u2BufferLength;

	} while (FALSE);

}

#if 0
/*---------------------------------------------------------------------------*/
/*!
 * \brief indicate an event to supplicant for device connection request
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 *
 * \retval none
 */
/*---------------------------------------------------------------------------*/
void kalP2PIndicateConnReq(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucDevName, int32_t u4NameLength,
		uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN],
		uint8_t ucDevType,/* 0: P2P Device / 1: GC / 2: GO */
		int32_t i4ConfigMethod, int32_t i4ActiveConfigMethod
)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];

	ASSERT(prGlueInfo);

	/* buffer peer information
	 * for later IOC_P2P_GET_REQ_DEVICE_INFO access
	 */
	prGlueInfo->prP2PInfo[0]->u4ConnReqNameLength =
		u4NameLength > 32 ? 32 : u4NameLength;
	kalMemCopy(prGlueInfo->prP2PInfo[0]->aucConnReqDevName,
		pucDevName,
		prGlueInfo->prP2PInfo[0]->u4ConnReqNameLength);
	COPY_MAC_ADDR(prGlueInfo->prP2PInfo[0]->rConnReqPeerAddr, rPeerAddr);
	prGlueInfo->prP2PInfo[0]->ucConnReqDevType = ucDevType;
	prGlueInfo->prP2PInfo[0]->i4ConnReqConfigMethod = i4ConfigMethod;
	prGlueInfo->prP2PInfo[0]->i4ConnReqActiveConfigMethod =
		i4ActiveConfigMethod;

	/* prepare event structure */
	memset(&evt, 0, sizeof(evt));

	snprintf(aucBuffer, IW_CUSTOM_MAX - 1, "P2P_DVC_REQ");
	evt.data.length = strlen(aucBuffer);

	/* indicate in IWEVCUSTOM event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);

}				/* end of kalP2PIndicateConnReq() */

/*---------------------------------------------------------------------------*/
/*!
 * \brief Indicate an event to supplicant
 *        for device connection request from other device.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 * \param[in] pucGroupBssid  Only valid when invitation Type equals to 0.
 *
 * \retval none
 */
/*---------------------------------------------------------------------------*/
void
kalP2PInvitationIndication(struct GLUE_INFO *prGlueInfo,
		struct P2P_DEVICE_DESC *prP2pDevDesc,
		uint8_t *pucSsid,
		uint8_t ucSsidLen,
		uint8_t ucOperatingChnl,
		uint8_t ucInvitationType,
		uint8_t *pucGroupBssid)
{
#if 1
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];

	ASSERT(prGlueInfo);

	/* buffer peer information for later IOC_P2P_GET_STRUCT access */
	prGlueInfo->prP2PInfo[0]->u4ConnReqNameLength =
	    (uint32_t) ((prP2pDevDesc->u2NameLength > 32)
	    ? 32 : prP2pDevDesc->u2NameLength);
	kalMemCopy(prGlueInfo->prP2PInfo[0]->aucConnReqDevName,
		prP2pDevDesc->aucName,
		prGlueInfo->prP2PInfo[0]->u4ConnReqNameLength);
	COPY_MAC_ADDR(prGlueInfo->prP2PInfo[0]->rConnReqPeerAddr,
		prP2pDevDesc->aucDeviceAddr);
	COPY_MAC_ADDR(prGlueInfo->prP2PInfo[0]->rConnReqGroupAddr,
		pucGroupBssid);
	prGlueInfo->prP2PInfo[0]->i4ConnReqConfigMethod = (int32_t)
		(prP2pDevDesc->u2ConfigMethod);
	prGlueInfo->prP2PInfo[0]->ucOperatingChnl = ucOperatingChnl;
	prGlueInfo->prP2PInfo[0]->ucInvitationType = ucInvitationType;

	/* prepare event structure */
	memset(&evt, 0, sizeof(evt));

	snprintf(aucBuffer, IW_CUSTOM_MAX - 1, "P2P_INV_INDICATE");
	evt.data.length = strlen(aucBuffer);

	/* indicate in IWEVCUSTOM event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);

#else
	struct MSG_P2P_CONNECTION_REQUEST *prP2pConnReq =
		(struct MSG_P2P_CONNECTION_REQUEST *) NULL;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	struct P2P_CONNECTION_SETTINGS *prP2pConnSettings =
		(struct P2P_CONNECTION_SETTINGS *) NULL;

	do {
		ASSERT_BREAK((prGlueInfo != NULL) && (prP2pDevDesc != NULL));

		/* Not a real solution */

		prP2pSpecificBssInfo =
			prGlueInfo->prAdapter->rWifiVar.prP2pSpecificBssInfo;
		prP2pConnSettings =
			prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings;

		prP2pConnReq = (struct MSG_P2P_CONNECTION_REQUEST *)
			cnmMemAlloc(prGlueInfo->prAdapter,
			    RAM_TYPE_MSG,
			    sizeof(struct MSG_P2P_CONNECTION_REQUEST));

		if (prP2pConnReq == NULL)
			break;

		kalMemZero(prP2pConnReq,
			sizeof(struct MSG_P2P_CONNECTION_REQUEST));

		prP2pConnReq->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_REQ;

		prP2pConnReq->eFormationPolicy = ENUM_P2P_FORMATION_POLICY_AUTO;

		COPY_MAC_ADDR(prP2pConnReq->aucDeviceID,
			prP2pDevDesc->aucDeviceAddr);

		prP2pConnReq->u2ConfigMethod = prP2pDevDesc->u2ConfigMethod;

		if (ucInvitationType == P2P_INVITATION_TYPE_INVITATION) {
			prP2pConnReq->fgIsPersistentGroup = FALSE;
			prP2pConnReq->fgIsTobeGO = FALSE;

		}

		else if (ucInvitationType == P2P_INVITATION_TYPE_REINVOKE) {
			DBGLOG(P2P, TRACE, "Re-invoke Persistent Group\n");
			prP2pConnReq->fgIsPersistentGroup = TRUE;
			prP2pConnReq->fgIsTobeGO =
				(prGlueInfo->prP2PInfo[0]->ucRole == 2)
				? TRUE : FALSE;

		}

		p2pFsmRunEventDeviceDiscoveryAbort(prGlueInfo->prAdapter, NULL);

		if (ucOperatingChnl != 0)
			prP2pSpecificBssInfo->ucPreferredChannel =
				ucOperatingChnl;

		if ((ucSsidLen < 32) && (pucSsid != NULL))
			COPY_SSID(prP2pConnSettings->aucSSID,
				prP2pConnSettings->ucSSIDLen,
				pucSsid, ucSsidLen);

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pConnReq,
			MSG_SEND_METHOD_BUF);

	} while (FALSE);

	/* frog add. */
	/* TODO: Invitation Indication */

#endif

}				/* kalP2PInvitationIndication */
#endif

#if 0
/*---------------------------------------------------------------------------*/
/*!
 * \brief Indicate an status to supplicant for device invitation status.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 *
 * \retval none
 */
/*---------------------------------------------------------------------------*/
void kalP2PInvitationStatus(struct GLUE_INFO *prGlueInfo,
		uint32_t u4InvStatus)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];

	ASSERT(prGlueInfo);

	/* buffer peer information for later IOC_P2P_GET_STRUCT access */
	prGlueInfo->prP2PInfo[0]->u4InvStatus = u4InvStatus;

	/* prepare event structure */
	memset(&evt, 0, sizeof(evt));

	snprintf(aucBuffer, IW_CUSTOM_MAX - 1, "P2P_INV_STATUS");
	evt.data.length = strlen(aucBuffer);

	/* indicate in IWEVCUSTOM event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);

}				/* kalP2PInvitationStatus */
#endif

/*---------------------------------------------------------------------------*/
/*!
 * \brief Indicate an event to supplicant
 *         for Service Discovery request from other device.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 *
 * \retval none
 */
/*---------------------------------------------------------------------------*/
void kalP2PIndicateSDRequest(struct GLUE_INFO *prGlueInfo,
		uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN], uint8_t ucSeqNum)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];
	int32_t i4Ret = 0;

	ASSERT(prGlueInfo);

	memset(&evt, 0, sizeof(evt));

	i4Ret =
		snprintf(aucBuffer, IW_CUSTOM_MAX - 1,
		"P2P_SD_REQ %d", ucSeqNum);
	if (i4Ret < 0) {
		DBGLOG(INIT, WARN, "sprintf failed:%d\n", i4Ret);
		return;
	}

	evt.data.length = strlen(aucBuffer);

	/* indicate IWEVP2PSDREQ event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);
}				/* end of kalP2PIndicateSDRequest() */

/*---------------------------------------------------------------------------*/
/*!
 * \brief Indicate an event to supplicant for Service Discovery response
 *         from other device.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 *
 * \retval none
 */
/*---------------------------------------------------------------------------*/
void kalP2PIndicateSDResponse(struct GLUE_INFO *prGlueInfo,
		uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN], uint8_t ucSeqNum)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];
	int32_t i4Ret = 0;

	ASSERT(prGlueInfo);

	memset(&evt, 0, sizeof(evt));

	i4Ret =
		snprintf(aucBuffer, IW_CUSTOM_MAX - 1,
		"P2P_SD_RESP %d", ucSeqNum);
	if (i4Ret < 0) {
		DBGLOG(INIT, WARN, "sprintf failed:%d\n", i4Ret);
		return;
	}
	evt.data.length = strlen(aucBuffer);

	/* indicate IWEVP2PSDREQ event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);
}				/* end of kalP2PIndicateSDResponse() */

/*---------------------------------------------------------------------------*/
/*!
 * \brief Indicate an event to supplicant for Service Discovery TX Done
 *         from other device.
 *
 * \param[in] prGlueInfo Pointer of GLUE_INFO_T
 * \param[in] ucSeqNum   Sequence number of the frame
 * \param[in] ucStatus   Status code for TX
 *
 * \retval none
 */
/*---------------------------------------------------------------------------*/
void kalP2PIndicateTXDone(struct GLUE_INFO *prGlueInfo,
		uint8_t ucSeqNum, uint8_t ucStatus)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];
	int32_t i4Ret = 0;

	ASSERT(prGlueInfo);

	memset(&evt, 0, sizeof(evt));

	i4Ret =
		snprintf(aucBuffer, IW_CUSTOM_MAX - 1,
		"P2P_SD_XMITTED: %d %d", ucSeqNum, ucStatus);
	if (i4Ret < 0) {
		DBGLOG(INIT, WARN, "sprintf failed:%d\n", i4Ret);
		return;
	}
	evt.data.length = strlen(aucBuffer);

	/* indicate IWEVP2PSDREQ event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);
}				/* end of kalP2PIndicateSDResponse() */

struct net_device *kalP2PGetDevHdlr(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PInfo[0]);
	return prGlueInfo->prP2PInfo[0]->prDevHandler;
}

#if CFG_SUPPORT_ANTI_PIRACY
#if 0
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*---------------------------------------------------------------------------*/
void kalP2PIndicateSecCheckRsp(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucRsp, uint16_t u2RspLen)
{
	union iwreq_data evt;
	uint8_t aucBuffer[IW_CUSTOM_MAX];

	ASSERT(prGlueInfo);

	memset(&evt, 0, sizeof(evt));
	snprintf(aucBuffer, IW_CUSTOM_MAX - 1, "P2P_SEC_CHECK_RSP=");

	kalMemCopy(prGlueInfo->prP2PInfo[0]->aucSecCheckRsp, pucRsp, u2RspLen);
	evt.data.length = strlen(aucBuffer);

#if DBG
	DBGLOG_MEM8(SEC, LOUD,
		prGlueInfo->prP2PInfo[0]->aucSecCheckRsp, u2RspLen);
#endif
	/* indicate in IWECUSTOM event */
	wireless_send_event(prGlueInfo->prP2PInfo[0]->prDevHandler,
		IWEVCUSTOM, &evt, aucBuffer);
}				/* p2pFsmRunEventRxDisassociation */
#endif
#endif

/*---------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prAdapter  Pointer of ADAPTER_T
 *
 * \return none
 */
/*---------------------------------------------------------------------------*/
void
kalGetChnlList(struct GLUE_INFO *prGlueInfo,
		enum ENUM_BAND eSpecificBand,
		uint8_t ucMaxChannelNum,
		uint8_t *pucNumOfChannel,
		struct RF_CHANNEL_INFO *paucChannelList)
{
	rlmDomainGetChnlList(prGlueInfo->prAdapter, eSpecificBand,
		FALSE, ucMaxChannelNum, pucNumOfChannel, paucChannelList);
}				/* kalGetChnlList */

/* ////////////////////////////ICS SUPPORT////////////////////////////// */

void
kalP2PIndicateChannelReady(struct GLUE_INFO *prGlueInfo,
		uint64_t u8SeqNum,
		uint32_t u4ChannelNum,
		enum ENUM_BAND eBand,
		enum ENUM_CHNL_EXT eSco,
		uint32_t u4Duration)
{
	struct ieee80211_channel *prIEEE80211ChnlStruct =
		(struct ieee80211_channel *)NULL;
	struct RF_CHANNEL_INFO rChannelInfo;
	enum nl80211_channel_type eChnlType = NL80211_CHAN_NO_HT;

	do {
		if (prGlueInfo == NULL || !prGlueInfo->prP2PInfo[0] ||
		    !prGlueInfo->prP2PInfo[0]->prDevHandler)
			break;

		kalMemZero(&rChannelInfo, sizeof(struct RF_CHANNEL_INFO));

		rChannelInfo.ucChannelNum = u4ChannelNum;
		rChannelInfo.eBand = eBand;

		prIEEE80211ChnlStruct =
			kalP2pFuncGetChannelEntry(prGlueInfo,
				prGlueInfo->prP2PInfo[0],
				&rChannelInfo);

		kalP2pFuncGetChannelType(eSco, &eChnlType);

		if (!prIEEE80211ChnlStruct) {
			DBGLOG(P2P, WARN, "prIEEE80211ChnlStruct is NULL\n");
			break;
		}

		cfg80211_ready_on_channel(
			/* struct wireless_dev, */
			prGlueInfo->prP2PInfo[0]->prDevHandler->ieee80211_ptr,
			/* u64 cookie, */
			u8SeqNum,
			/* struct ieee80211_channel * chan, */
			prIEEE80211ChnlStruct,
			/* unsigned int duration, */
			u4Duration,
			/* gfp_t gfp *//* allocation flags */
			GFP_KERNEL);
	} while (FALSE);

}				/* kalP2PIndicateChannelReady */

void
kalP2PIndicateChannelExpired(struct GLUE_INFO *prGlueInfo,
		uint64_t u8SeqNum,
		uint32_t u4ChannelNum,
		enum ENUM_BAND eBand,
		enum ENUM_CHNL_EXT eSco)
{

	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct ieee80211_channel *prIEEE80211ChnlStruct =
		(struct ieee80211_channel *)NULL;
	enum nl80211_channel_type eChnlType = NL80211_CHAN_NO_HT;
	struct RF_CHANNEL_INFO rRfChannelInfo;
	struct GL_P2P_DEV_INFO *prGlueP2pDevInfo =
		(struct GL_P2P_DEV_INFO *) NULL;

	do {
		if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL) {
			DBGLOG(P2P, ERROR,
				"prGlueInfo=0x%p prAdapter=0x%p\n",
				prGlueInfo,
				prGlueInfo != NULL ?
					prGlueInfo->prAdapter : NULL);
			break;
		}

		prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

		if (prGlueP2pInfo == NULL ||
		    prGlueP2pInfo->prDevHandler == NULL ||
		    prGlueInfo->prAdapter->rP2PNetRegState !=
				ENUM_NET_REG_STATE_REGISTERED) {
			DBGLOG(P2P, ERROR,
				"prGlueP2pInfo=0x%p prDevHandler=0x%p rP2PNetRegState=%d\n",
				prGlueP2pInfo,
				prGlueP2pInfo != NULL ?
					prGlueP2pInfo->prDevHandler : NULL,
				prGlueInfo->prAdapter->rP2PNetRegState);
			break;
		}

		DBGLOG(P2P, TRACE, "kalP2PIndicateChannelExpired\n");

		rRfChannelInfo.eBand = eBand;
		rRfChannelInfo.ucChannelNum = u4ChannelNum;

		prIEEE80211ChnlStruct =
			kalP2pFuncGetChannelEntry(prGlueInfo,
				prGlueP2pInfo,
				&rRfChannelInfo);

		kalP2pFuncGetChannelType(eSco, &eChnlType);

		if (!prIEEE80211ChnlStruct) {
			DBGLOG(P2P, WARN, "prIEEE80211ChnlStruct is NULL\n");
			break;
		}

		prGlueP2pDevInfo = prGlueInfo->prP2PDevInfo;
		if (prGlueP2pDevInfo &&
		    prGlueP2pDevInfo->rP2pRocRequest.wdev &&
		    prGlueP2pDevInfo->rP2pRocRequest.u8Cookie ==
		    u8SeqNum)
			prGlueP2pDevInfo->rP2pRocRequest.wdev = NULL;

		/* struct wireless_dev, */
		cfg80211_remain_on_channel_expired(
			prGlueP2pInfo->prDevHandler->ieee80211_ptr,
			u8SeqNum, prIEEE80211ChnlStruct, GFP_KERNEL);
	} while (FALSE);

}				/* kalP2PIndicateChannelExpired */

void kalP2PIndicateScanDone(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex, u_int8_t fgIsAbort)
{
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GL_P2P_DEV_INFO *prP2pGlueDevInfo =
		(struct GL_P2P_DEV_INFO *) NULL;
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct cfg80211_scan_request *prScanRequest = NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	do {
		if (prGlueInfo == NULL) {

			ASSERT(FALSE);
			break;
		}

		prGlueP2pInfo = prGlueInfo->prP2PInfo[0];
		prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;

		if ((prGlueP2pInfo == NULL) || (prP2pGlueDevInfo == NULL)) {
			ASSERT(FALSE);
			break;
		}

		DBGLOG(INIT, TRACE,
			"[p2p] scan complete %p abort=%d\n",
			prP2pGlueDevInfo->prScanRequest, fgIsAbort);

		KAL_ACQUIRE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_INF);
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

		if ((prP2pGlueDevInfo->prScanRequest != NULL)
			&& (prGlueInfo->prAdapter->fgIsP2PRegistered == TRUE)) {
			prScanRequest = prP2pGlueDevInfo->prScanRequest;
			kalCfg80211ScanDone(prScanRequest, fgIsAbort);
			prP2pGlueDevInfo->prScanRequest = NULL;
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

		if ((prScanRequest != NULL)
			&& (prGlueInfo->prAdapter->fgIsP2PRegistered == TRUE)) {

			/* report all queued beacon/probe response frames
			 * to upper layer
			 */
			scanReportBss2Cfg80211(prGlueInfo->prAdapter,
				BSS_TYPE_P2P_DEVICE, NULL);

			DBGLOG(INIT, TRACE, "DBG:p2p_cfg_scan_done\n");
		}
		KAL_RELEASE_MUTEX(prGlueInfo->prAdapter, MUTEX_DEL_INF);

	} while (FALSE);
#endif
}				/* kalP2PIndicateScanDone */

void
kalP2PIndicateBssInfo(struct GLUE_INFO *prGlueInfo,
		uint8_t *pucFrameBuf,
		uint32_t u4BufLen,
		struct RF_CHANNEL_INFO *prChannelInfo,
		int32_t i4SignalStrength)
{
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct ieee80211_channel *prChannelEntry =
		(struct ieee80211_channel *)NULL;
	struct ieee80211_mgmt *prBcnProbeRspFrame =
		(struct ieee80211_mgmt *)pucFrameBuf;
	struct cfg80211_bss *prCfg80211Bss = (struct cfg80211_bss *)NULL;

	do {
		if ((prGlueInfo == NULL) || (pucFrameBuf == NULL)
			|| (prChannelInfo == NULL)) {
			ASSERT(FALSE);
			break;
		}

		prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

		if (prGlueP2pInfo == NULL) {
			ASSERT(FALSE);
			break;
		}

		prChannelEntry =
			kalP2pFuncGetChannelEntry(prGlueInfo,
				prGlueP2pInfo,
				prChannelInfo);

		if (prChannelEntry == NULL) {
			DBGLOG(P2P, TRACE, "Unknown channel info\n");
			break;
		}

		/* rChannelInfo.center_freq =
		 * nicChannelNum2Freq((UINT_32)prChannelInfo->ucChannelNum)
		 * / 1000;
		 */

		if (u4BufLen > 0) {
#if CFG_SUPPORT_TSF_USING_BOOTTIME
			prBcnProbeRspFrame->u.beacon.timestamp =
				kalGetBootTime();
#endif
			prCfg80211Bss = cfg80211_inform_bss_frame(
				/* struct wiphy * wiphy, */
				prGlueP2pInfo->prWdev->wiphy,
				prChannelEntry,
				prBcnProbeRspFrame,
				u4BufLen, i4SignalStrength, GFP_KERNEL);
		}

		/* Return this structure. */
		if (prCfg80211Bss)
			cfg80211_put_bss(prGlueP2pInfo->prWdev->wiphy,
				prCfg80211Bss);
		else
			DBGLOG(P2P, WARN,
				"indicate BSS to cfg80211 failed [" MACSTR
				"]: bss channel %d, rcpi %d, frame_len=%d\n",
				MAC2STR(prBcnProbeRspFrame->bssid),
				prChannelInfo->ucChannelNum,
				i4SignalStrength, u4BufLen);

	} while (FALSE);
#endif
	return;

}				/* kalP2PIndicateBssInfo */

void kalP2PIndicateMgmtTxStatus(struct GLUE_INFO *prGlueInfo,
		struct MSDU_INFO *prMsduInfo, u_int8_t fgIsAck)
{
	struct wireless_dev *prWdev;
	struct WLAN_MAC_HEADER *prWlanHdr;
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	uint64_t *pu8GlCookie = (uint64_t *) NULL;
	struct net_device *prNetdevice = (struct net_device *)NULL;

	do {
		if ((prGlueInfo == NULL) || (prMsduInfo == NULL)) {
			DBGLOG(P2P, WARN,
				"Unexpected NULL pointer prGlueInfo/prMsduInfo.\n");
			ASSERT(FALSE);
			break;
		}

		pu8GlCookie =
		    (uint64_t *) ((unsigned long) prMsduInfo->prPacket +
				(unsigned long) prMsduInfo->u2FrameLength +
				MAC_TX_RESERVED_FIELD);

		if (prMsduInfo->ucBssIndex
			== prGlueInfo->prAdapter->ucP2PDevBssIdx) {

			prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

			if (prGlueP2pInfo == NULL)
				return;

			prNetdevice = prGlueP2pInfo->prDevHandler;

		} else {
			struct BSS_INFO *prP2pBssInfo =
				GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter,
				prMsduInfo->ucBssIndex);
			if (prP2pBssInfo == NULL)
				return;

			prGlueP2pInfo =
				prGlueInfo->prP2PInfo
					[prP2pBssInfo->u4PrivateData];

			if (prGlueP2pInfo == NULL)
				return;

			prNetdevice = prGlueP2pInfo->aprRoleHandler;

			if (!prNetdevice) {
				DBGLOG(P2P, WARN,
					"prMsduInfo->ucBssIndex %d, ucP2PDevBssIdx %d\n",
					prMsduInfo->ucBssIndex,
					prGlueInfo->prAdapter->ucP2PDevBssIdx);

				prNetdevice = prGlueP2pInfo->prDevHandler;
			}
		}

		if (!prGlueInfo->fgIsRegistered ||
			test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag) ||
			!prGlueInfo->prAdapter->fgIsP2PRegistered ||
			(prGlueInfo->prAdapter->rP2PNetRegState !=
				ENUM_NET_REG_STATE_REGISTERED) ||
			(prNetdevice == NULL) ||
			(prNetdevice->reg_state != NETREG_REGISTERED) ||
			(prNetdevice->ieee80211_ptr == NULL)) {
			DBGLOG(P2P, WARN,
				"prNetdevice is not ready or NULL!\n");
			break;
		}

		prWdev = prNetdevice->ieee80211_ptr;
		prWlanHdr = (struct WLAN_MAC_HEADER *)
			((unsigned long) prMsduInfo->prPacket +
					 MAC_TX_RESERVED_FIELD);

		p2pFuncRemovePendingMgmtLinkEntry(prGlueInfo->prAdapter,
			prMsduInfo->ucBssIndex, *pu8GlCookie);

#if (CFG_SUPPORT_802_11BE_MLO == 1) && \
	(KERNEL_VERSION(5, 19, 2) <= CFG80211_VERSION_CODE)
		if (prWdev->valid_links &&
		    prWdev->iftype == NL80211_IFTYPE_AP &&
		    (ieee80211_is_auth(prWlanHdr->u2FrameCtrl) ||
		     ieee80211_is_assoc_resp(prWlanHdr->u2FrameCtrl) ||
		     ieee80211_is_reassoc_resp(prWlanHdr->u2FrameCtrl)))
			nicMgmtMAT_Tx_L2M(prGlueInfo->prAdapter, prMsduInfo);
#endif /* CFG_SUPPORT_802_11BE_MLO */


		cfg80211_mgmt_tx_status(
			prWdev,
			*pu8GlCookie,
			(uint8_t *) ((unsigned long) prMsduInfo->prPacket +
			MAC_TX_RESERVED_FIELD),
			prMsduInfo->u2FrameLength, fgIsAck, GFP_KERNEL);

	} while (FALSE);

}				/* kalP2PIndicateMgmtTxStatus */

void
kalP2PIndicateRxMgmtFrame(struct ADAPTER *prAdapter,
		struct GLUE_INFO *prGlueInfo,
		struct SW_RFB *prSwRfb,
		u_int8_t fgIsDevInterface,
		uint8_t ucRoleIdx,
		uint32_t u4LinkId)
{
#define DBG_P2P_MGMT_FRAME_INDICATION 1

	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	int32_t i4Freq = 0;
	uint8_t ucChnlNum = 0;
#if DBG_P2P_MGMT_FRAME_INDICATION
	struct WLAN_MAC_HEADER *prWlanHeader = (struct WLAN_MAC_HEADER *) NULL;
#endif
	struct net_device *prNetdevice = (struct net_device *)NULL;
#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)
	struct cfg80211_rx_info rRxInfo;
#endif
	struct RX_DESC_OPS_T *prRxDescOps;
	enum ENUM_BAND eBand;

	do {
		char acRxPktInfo[64];

		if ((prGlueInfo == NULL) || (prSwRfb == NULL)) {
			ASSERT(FALSE);
			break;
		}

		prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

		/* ToDo[6630]: Get the following by channel freq */
		/* HAL_RX_STATUS_GET_CHAN_FREQ( prSwRfb->prRxStatus) */
		/* ucChnlNum = prSwRfb->prHifRxHdr->ucHwChannelNum; */
		ucChnlNum = prSwRfb->ucChnlNum;

		prRxDescOps = prAdapter->chip_info->prRxDescOps;

		eBand = prSwRfb->eRfBand;

#if DBG_P2P_MGMT_FRAME_INDICATION

		prWlanHeader = (struct WLAN_MAC_HEADER *) prSwRfb->pvHeader;

		switch (prWlanHeader->u2FrameCtrl) {
		case MAC_FRAME_PROBE_REQ:
			kalSnprintf(acRxPktInfo, sizeof(acRxPktInfo),
				"RX Probe Req at channel %d",
				ucChnlNum);
			break;
		case MAC_FRAME_PROBE_RSP:
			kalSnprintf(acRxPktInfo, sizeof(acRxPktInfo),
				"RX Probe Rsp at channel %d",
				ucChnlNum);
			break;
		case MAC_FRAME_ACTION:
			kalSnprintf(acRxPktInfo, sizeof(acRxPktInfo),
				"RX Action frame at channel %d",
				ucChnlNum);
			break;
		default:
			kalSnprintf(acRxPktInfo, sizeof(acRxPktInfo),
				"RX Packet:%d at channel %d",
				prWlanHeader->u2FrameCtrl, ucChnlNum);
			break;
		}
#endif
		i4Freq = nicChannelNum2Freq(ucChnlNum, eBand) / 1000;

		if (fgIsDevInterface)
			prNetdevice = prGlueP2pInfo->prDevHandler;
		else
			prNetdevice = prGlueP2pInfo->aprRoleHandler;

		DBGLOG(P2P, TRACE, "%s from: " MACSTR ", netdev: %p\n",
				acRxPktInfo, MAC2STR(prWlanHeader->aucAddr2),
				prNetdevice);

		if (!prGlueInfo->fgIsRegistered ||
			test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag) ||
			!prGlueInfo->prAdapter->fgIsP2PRegistered ||
			(prGlueInfo->prAdapter->rP2PNetRegState !=
				ENUM_NET_REG_STATE_REGISTERED) ||
			(prNetdevice == NULL) ||
			(prNetdevice->reg_state != NETREG_REGISTERED) ||
			(prNetdevice->ieee80211_ptr == NULL)) {
			DBGLOG(P2P, WARN,
				"prNetdevice is not ready or NULL!\n");
			break;
		}

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)
		kalMemZero(&rRxInfo, sizeof(rRxInfo));
		rRxInfo.freq = MHZ_TO_KHZ(i4Freq);
		rRxInfo.sig_dbm = RCPI_TO_dBm(
			nicRxGetRcpiValueFromRxv(prAdapter,
				RCPI_MODE_WF0, prSwRfb));
		rRxInfo.buf = prSwRfb->pvHeader;
		rRxInfo.len = prSwRfb->u2PacketLen;
		rRxInfo.flags = GFP_ATOMIC;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (u4LinkId != MLD_LINK_ID_NONE) {
			nicMgmtMAT_Rx_L2M(prAdapter, prSwRfb);
			rRxInfo.have_link_id = true;
			rRxInfo.link_id = u4LinkId;
		}
#endif

		cfg80211_rx_mgmt_ext(prNetdevice->ieee80211_ptr, &rRxInfo);
#elif (KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE)
		cfg80211_rx_mgmt(
			/* struct net_device * dev, */
			prNetdevice->ieee80211_ptr,
			i4Freq,
			RCPI_TO_dBm(
				nicRxGetRcpiValueFromRxv(prGlueInfo->prAdapter,
				RCPI_MODE_MAX,
				prSwRfb)),
			prSwRfb->pvHeader,
			prSwRfb->u2PacketLen,
			NL80211_RXMGMT_FLAG_ANSWERED);
#elif (KERNEL_VERSION(3, 12, 0) <= CFG80211_VERSION_CODE)
		cfg80211_rx_mgmt(
			/* struct net_device * dev, */
			prNetdevice->ieee80211_ptr,
			i4Freq,
			RCPI_TO_dBm(
				nicRxGetRcpiValueFromRxv(prGlueInfo->prAdapter,
				RCPI_MODE_WF0,
				prSwRfb)),
			prSwRfb->pvHeader,
			prSwRfb->u2PacketLen,
			NL80211_RXMGMT_FLAG_ANSWERED,
			GFP_ATOMIC);
#else
		cfg80211_rx_mgmt(
			/* struct net_device * dev, */
			prNetdevice->ieee80211_ptr,
			i4Freq,
			RCPI_TO_dBm(
				nicRxGetRcpiValueFromRxv(prGlueInfo->prAdapter,
				RCPI_MODE_WF0,
				prSwRfb)),
			prSwRfb->pvHeader,
			prSwRfb->u2PacketLen,
			GFP_ATOMIC);
#endif


	} while (FALSE);

}				/* kalP2PIndicateRxMgmtFrame */

void
kalP2PGCIndicateConnectionStatus(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		struct P2P_CONNECTION_REQ_INFO *prP2pConnInfo,
		uint8_t *pucRxIEBuf,
		uint16_t u2RxIELen,
		uint16_t u2StatusReason,
		uint32_t eStatus)
{
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct ADAPTER *prAdapter = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct STA_RECORD *prStaRec;
#if ((CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_STA_RECORD *prMldStaRec = NULL;
#endif

	if (prGlueInfo == NULL) {
		ASSERT(FALSE);
		return;
	}

	prAdapter = prGlueInfo->prAdapter;
	prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIndex];
	prP2pRoleFsmInfo =
		P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIndex);
	prStaRec = p2pGetLinkStaRec(prP2pRoleFsmInfo, P2P_MAIN_LINK_INDEX);

	/* FIXME: This exception occurs at wlanRemove. */
	if ((prGlueP2pInfo == NULL) ||
	    (prAdapter->rP2PNetRegState != ENUM_NET_REG_STATE_REGISTERED) ||
	    (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag) == 1) ||
	    (prGlueP2pInfo->aprRoleHandler == NULL) ||
	    (prGlueP2pInfo->aprRoleHandler->reg_state != NETREG_REGISTERED))
		return;

	if (prP2pConnInfo) {
#if ((CFG_ADVANCED_80211_MLO == 1) || \
(KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE)) && \
(CFG_SUPPORT_802_11BE_MLO == 1)
		prMldStaRec = mldStarecGetByStarec(prAdapter, prStaRec);
		if (prMldStaRec) {
			struct cfg80211_connect_resp_params params;
			uint8_t i;

			kalMemSet(&params, 0, sizeof(params));
			params.status = u2StatusReason;
			params.req_ie = prP2pConnInfo->aucIEBuf;
			params.req_ie_len = prP2pConnInfo->u4BufLength;
			params.resp_ie = pucRxIEBuf;
			params.resp_ie_len = u2RxIELen;
			params.timeout_reason = NL80211_TIMEOUT_UNSPECIFIED;
			params.ap_mld_addr = prMldStaRec->aucPeerMldAddr;

			for (i = 0; i < MLD_LINK_MAX; i++) {
				struct BSS_INFO *prP2pLinkBssInfo =
					p2pGetLinkBssInfo(prP2pRoleFsmInfo, i);
				struct STA_RECORD *prStaRec =
					p2pGetLinkStaRec(prP2pRoleFsmInfo, i);
				uint8_t id;

				if (!prP2pLinkBssInfo || !prStaRec)
					continue;

				id = prStaRec->ucLinkId;
				params.valid_links |= BIT(id);
				params.links[id].addr =
					prP2pLinkBssInfo->aucOwnMacAddr;
				params.links[id].bssid =
					prStaRec->aucMacAddr;
			}

			cfg80211_connect_done(
				prGlueP2pInfo->aprRoleHandler,
				&params, GFP_KERNEL);
		} else
#endif
		{
			uint8_t aucBssid[MAC_ADDR_LEN];

			COPY_MAC_ADDR(aucBssid, prP2pConnInfo->aucBssid);

			cfg80211_connect_result(
				prGlueP2pInfo->aprRoleHandler,
				/* struct net_device * dev, */
				aucBssid,
				prP2pConnInfo->aucIEBuf,
				prP2pConnInfo->u4BufLength,
				pucRxIEBuf, u2RxIELen,
				u2StatusReason,
				/* gfp_t gfp *//* allocation flags */
				GFP_KERNEL);
		}
		prP2pConnInfo->eConnRequest = P2P_CONNECTION_TYPE_IDLE;

		if (u2StatusReason == STATUS_CODE_SUCCESSFUL) {
			/* switch netif on */
			netif_carrier_on(prGlueP2pInfo->aprRoleHandler);
			netif_tx_start_all_queues(
				prGlueP2pInfo->aprRoleHandler);

#if (CFG_SUPPORT_NAN == 1)
			/* TODO: GC connected */
			DBGLOG(INIT, DEBUG, "GC connected\n");
			nanConcurrencyHandler(prAdapter);
#endif
		}
	} else {
		DBGLOG(INIT, DEBUG,
			"indicate disconnection event to kernel, reason=%d, locally_generated=%d\n",
			u2StatusReason,
			eStatus == WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY);
		/* Disconnect, what if u2StatusReason == 0? */
		cfg80211_disconnected(prGlueP2pInfo->aprRoleHandler,
			/* struct net_device * dev, */
			u2StatusReason,
			pucRxIEBuf, u2RxIELen,
#if CFG_WPS_DISCONNECT || (KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE)
			eStatus == WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY,
#endif
			GFP_KERNEL);
	}
}				/* kalP2PGCIndicateConnectionStatus */

void
kalP2PGOStationUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		struct STA_RECORD *prCliStaRec,
		u_int8_t fgIsNew)
{
	struct GL_P2P_INFO *prP2pGlueInfo = (struct GL_P2P_INFO *) NULL;
	uint8_t aucBssid[MAC_ADDR_LEN];
	struct BSS_INFO *prBssInfo = NULL;
	struct station_info rStationInfo = {0};
#if (KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE) || \
	(CFG_ADVANCED_80211_MLO == 1)
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss;
	struct MLD_STA_RECORD *prMldSta;
#endif
#endif

	if ((prGlueInfo == NULL) || (prCliStaRec == NULL) ||
	    (ucRoleIndex >= KAL_P2P_NUM))
		return;

	prP2pGlueInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

	if ((prP2pGlueInfo == NULL) ||
	    (prP2pGlueInfo->aprRoleHandler == NULL)) {
		/* This case may occur when the usb is unplugged */
		return;
	}

	COPY_MAC_ADDR(aucBssid, prCliStaRec->aucMacAddr);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter,
					  prCliStaRec->ucBssIndex);
	if (!prBssInfo)
		return;

#if (KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE) || \
		(CFG_ADVANCED_80211_MLO == 1)
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = mldBssGetByBss(prGlueInfo->prAdapter,
				  prBssInfo);
	prMldSta = mldStarecGetByStarec(prGlueInfo->prAdapter,
					prCliStaRec);
#endif
#endif

	DBGLOG(P2P, INFO, "role=%u mac="MACSTR" new=%d\n",
		ucRoleIndex,
		MAC2STR(prCliStaRec->aucMacAddr),
		fgIsNew);

	if (fgIsNew) {
		if (prCliStaRec->fgIsConnected == TRUE) {
			DBGLOG(P2P, WARN,
				"Skip duplicate notify " MACSTR "\n",
				MAC2STR(prCliStaRec->aucMacAddr));
			return;
		}

		prCliStaRec->fgIsConnected = TRUE;

#if KERNEL_VERSION(4, 0, 0) > CFG80211_VERSION_CODE
		rStationInfo.filled = STATION_INFO_ASSOC_REQ_IES;
#endif
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
		rStationInfo.generation = ++prP2pGlueInfo->i4Generation;
#endif
		rStationInfo.assoc_req_ies = prCliStaRec->pucAssocReqIe;
		rStationInfo.assoc_req_ies_len =
			prCliStaRec->u2AssocReqIeLen;

#if (KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE) || \
	(CFG_ADVANCED_80211_MLO == 1 && CFG_MTK_ANDROID_WMT == 1)
		if (prCliStaRec->pucAssocRespIe &&
		    prCliStaRec->u2AssocRespIeLen) {
			rStationInfo.assoc_resp_ies =
				prCliStaRec->pucAssocRespIe;
			rStationInfo.assoc_resp_ies_len =
				prCliStaRec->u2AssocRespIeLen;
		}
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (IS_MLD_BSSINFO_MULTI(prMldBss) || prMldSta) {
			rStationInfo.mlo_params_valid = true;
			rStationInfo.assoc_link_id =
				prBssInfo->ucLinkId;
			if (prMldSta)
				COPY_MAC_ADDR(rStationInfo.mld_addr,
					      prMldSta->aucPeerMldAddr);
		}
#endif
#endif

		cfg80211_new_sta(prP2pGlueInfo->aprRoleHandler,
			aucBssid,
			&rStationInfo, GFP_KERNEL);
	} else {
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
		++prP2pGlueInfo->i4Generation;
#endif
		/* FIXME: The exception occurs at wlanRemove, and
		 *    check GLUE_FLAG_HALT is the temporarily solution.
		 */
		if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag) == 0) {
			/* sae hostapd new_sta, when auth fail,
			 * driver need del_sta
			 */
			if (prCliStaRec->fgIsConnected == FALSE &&
			    !rsnKeyMgmtSae(prBssInfo->u4RsnSelectedAKMSuite) &&
			    prBssInfo->u4RsnSelectedAKMSuite !=
					RSN_AKM_SUITE_OWE)
				return;
			prCliStaRec->fgIsConnected = FALSE;

#if (KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE)
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			if (prMldSta)
				COPY_MAC_ADDR(aucBssid,
					      prMldSta->aucPeerMldAddr);
#endif
#endif

			cfg80211_del_sta(prP2pGlueInfo->aprRoleHandler,
					 aucBssid, GFP_KERNEL);
		}
#if CFG_STAINFO_FEATURE
		kalMemCopy(&prGlueInfo->prAdapter->rSapLastStaRec,
			prCliStaRec, sizeof(struct STA_RECORD));
		prGlueInfo->prAdapter->fgSapLastStaRecSet = 1;
#endif
	}
}				/* kalP2PGOStationUpdate */

#if (CFG_SUPPORT_DFS_MASTER == 1)
void kalP2PRddDetectUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex)
{
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct net_device *prNetdevice = (struct net_device *) NULL;

	DBGLOG(INIT, DEBUG, "Radar Detection event\n");

	do {

		if (prGlueInfo == NULL)
			break;

		prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

		if ((prGlueP2pInfo->aprRoleHandler != NULL) &&
			(prGlueP2pInfo->aprRoleHandler !=
				prGlueP2pInfo->prDevHandler))
			prNetdevice = prGlueP2pInfo->aprRoleHandler;
		else
			prNetdevice = prGlueP2pInfo->prDevHandler;

#ifdef CFG_REPORT_TO_OS
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
		/* cac start disable for next cac slot
		 * if enable in dfs channel
		 */
		prGlueP2pInfo->prWdev->cac_started = FALSE;
		DBGLOG(INIT, DEBUG,
			"Update to OS\n");
		if (prGlueP2pInfo->chandefCsa.chan) {
			cfg80211_radar_event(
				prGlueP2pInfo->prWdev->wiphy,
				&prGlueP2pInfo->chandefCsa,
				GFP_KERNEL);
			DBGLOG(INIT, DEBUG,
				"Update to OS Done\n");
		}
#endif
#endif

		if (prGlueP2pInfo->chandefCsa.chan)
			kalP2pIndicateRadarEvent(prGlueInfo,
				ucRoleIndex,
				WIFI_EVENT_DFS_OFFLOAD_RADAR_DETECTED,
			prGlueP2pInfo->chandefCsa.chan->center_freq);

		netif_carrier_off(prNetdevice);
		netif_tx_stop_all_queues(prNetdevice);
	} while (FALSE);

}				/* kalP2PRddDetectUpdate */

void kalP2PCacStartedUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex)
{
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct net_device *prNetdevice = (struct net_device *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;

	do {
		if (prGlueInfo == NULL)
			break;

		prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
			prGlueInfo->prAdapter, ucRoleIndex);
		prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
		DBGLOG(INIT, DEBUG, "CAC Started event\n");
		prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

		if ((prGlueP2pInfo->aprRoleHandler != NULL) &&
			(prGlueP2pInfo->aprRoleHandler !=
				prGlueP2pInfo->prDevHandler))
			prNetdevice = prGlueP2pInfo->aprRoleHandler;
		else
			prNetdevice = prGlueP2pInfo->prDevHandler;

#ifdef CFG_REPORT_TO_OS
		DBGLOG(INIT, DEBUG, "CacStarted: Update to OS\n");
#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
		cfg80211_cac_event(
			prNetdevice,
			&prGlueP2pInfo->chandefCsa,
			NL80211_RADAR_CAC_STARTED, GFP_KERNEL);
#else
		cfg80211_cac_event(
			prNetdevice,
			NL80211_RADAR_CAC_STARTED, GFP_KERNEL);
#endif
		DBGLOG(INIT, DEBUG, "CacStarted: Update to OS Done\n");
#endif

		if (prGlueP2pInfo->chandefCsa.chan)
			kalP2pIndicateRadarEvent(prGlueInfo,
				ucRoleIndex,
				WIFI_EVENT_DFS_OFFLOAD_CAC_STARTED,
				prGlueP2pInfo->chandefCsa.chan->center_freq);
		else {
			kalP2pIndicateRadarEvent(prGlueInfo,
				ucRoleIndex,
				WIFI_EVENT_DFS_OFFLOAD_CAC_STARTED,
				prP2pConnReqInfo->u2PriChnlFreq);
		}
	} while (FALSE);

}

void kalP2PCacFinishedUpdate(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex)
{
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct net_device *prNetdevice = (struct net_device *) NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prP2pConnReqInfo;

	do {
		if (prGlueInfo == NULL)
			break;
		prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
			prGlueInfo->prAdapter, ucRoleIndex);
		prP2pConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);

		DBGLOG(INIT, DEBUG, "CAC Finished event\n");
		prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

		if ((prGlueP2pInfo->aprRoleHandler != NULL) &&
			(prGlueP2pInfo->aprRoleHandler !=
				prGlueP2pInfo->prDevHandler))
			prNetdevice = prGlueP2pInfo->aprRoleHandler;
		else
			prNetdevice = prGlueP2pInfo->prDevHandler;

#ifdef CFG_REPORT_TO_OS
		DBGLOG(INIT, DEBUG, "%s: Update to OS\n", __func__);
#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
		cfg80211_cac_event(
			prNetdevice,
			&prGlueP2pInfo->chandefCsa,
			NL80211_RADAR_CAC_FINISHED, GFP_KERNEL);
#else
		cfg80211_cac_event(
			prNetdevice,
			NL80211_RADAR_CAC_FINISHED, GFP_KERNEL);
#endif
		DBGLOG(INIT, DEBUG,
			"kalP2PCacFinishedUpdate: Update to OS Done\n");
#endif

		if (prGlueP2pInfo->chandefCsa.chan)
			kalP2pIndicateRadarEvent(prGlueInfo,
				ucRoleIndex,
				WIFI_EVENT_DFS_OFFLOAD_CAC_FINISHED,
				prGlueP2pInfo->chandefCsa.chan->center_freq);
		else {
			kalP2pIndicateRadarEvent(prGlueInfo,
				ucRoleIndex,
				WIFI_EVENT_DFS_OFFLOAD_CAC_FINISHED,
				prP2pConnReqInfo->u2PriChnlFreq);
		}
	} while (FALSE);

}				/* kalP2PRddDetectUpdate */
#endif

u_int8_t kalP2pFuncGetChannelType(enum ENUM_CHNL_EXT rChnlSco,
		enum nl80211_channel_type *channel_type)
{
	u_int8_t fgIsValid = FALSE;

	do {
		if (channel_type) {

			switch (rChnlSco) {
			case CHNL_EXT_SCN:
				*channel_type = NL80211_CHAN_NO_HT;
				break;
			case CHNL_EXT_SCA:
				*channel_type = NL80211_CHAN_HT40MINUS;
				break;
			case CHNL_EXT_SCB:
				*channel_type = NL80211_CHAN_HT40PLUS;
				break;
			default:
				ASSERT(FALSE);
				*channel_type = NL80211_CHAN_NO_HT;
				break;
			}

		}

		fgIsValid = TRUE;
	} while (FALSE);

	return fgIsValid;
}				/* kalP2pFuncGetChannelType */

struct ieee80211_channel *kalP2pFuncGetChannelEntry(
		struct GLUE_INFO *prGlueInfo,
		struct GL_P2P_INFO *prP2pInfo,
		struct RF_CHANNEL_INFO *prChannelInfo)
{
	struct ieee80211_channel *prTargetChannelEntry =
		(struct ieee80211_channel *)NULL;
	uint32_t u4TblSize = 0, u4Idx = 0;
	struct wiphy *wiphy = GLUE_GET_WIPHY(prGlueInfo);

	if ((prP2pInfo == NULL) ||
		(prChannelInfo == NULL) ||
		(wiphy == NULL))
		return NULL;

	do {

		switch (prChannelInfo->eBand) {
		case BAND_2G4:
			if (wiphy->bands[KAL_BAND_2GHZ] == NULL)
				DBGLOG(P2P, ERROR,
					"kalP2pFuncGetChannelEntry 2.4G NULL Bands!!\n");
			else {
				prTargetChannelEntry =
					wiphy->bands[KAL_BAND_2GHZ]->channels;
				u4TblSize =
					wiphy->bands[KAL_BAND_2GHZ]->n_channels;
			}
			break;
		case BAND_5G:
			if (wiphy->bands[KAL_BAND_5GHZ] == NULL)
				DBGLOG(P2P, ERROR,
					"kalP2pFuncGetChannelEntry 5G NULL Bands!!\n");
			else {
				prTargetChannelEntry =
					wiphy->bands[KAL_BAND_5GHZ]->channels;
				u4TblSize =
					wiphy->bands[KAL_BAND_5GHZ]->n_channels;
			}
			break;
#if (CFG_SUPPORT_WIFI_6G == 1)
		case BAND_6G:
			if (wiphy->bands[KAL_BAND_6GHZ] == NULL)
				DBGLOG(P2P, ERROR,
					"kalP2pFuncGetChannelEntry 6G NULL Bands!!\n");
			else {
				prTargetChannelEntry =
					wiphy->bands[KAL_BAND_6GHZ]->channels;
				u4TblSize =
					wiphy->bands[KAL_BAND_6GHZ]->n_channels;
			}
			break;
#endif
		default:
			break;
		}

		if (prTargetChannelEntry == NULL)
			break;

		for (u4Idx = 0; u4Idx < u4TblSize
			; u4Idx++, prTargetChannelEntry++) {
			if (prTargetChannelEntry->hw_value
				== prChannelInfo->ucChannelNum)
				break;

		}

		if (u4Idx == u4TblSize) {
			prTargetChannelEntry = NULL;
			break;
		}

	} while (FALSE);

	return prTargetChannelEntry;
}				/* kalP2pFuncGetChannelEntry */

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

/*---------------------------------------------------------------------------*/
/*!
 * \brief to set the block list of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*---------------------------------------------------------------------------*/
u_int8_t kalP2PSetBlockList(struct GLUE_INFO *prGlueInfo,
		uint8_t rbssid[PARAM_MAC_ADDR_LEN],
		u_int8_t fgIsblock,
		uint8_t ucRoleIndex)
{
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;
	uint32_t i;
	uint8_t ucBssIdx = 0;

	ASSERT(prGlueInfo);

	/*if only one ap mode register, prGlueInfo->prP2PInfo[1] would be null*/
	if (!prGlueInfo->prP2PInfo[ucRoleIndex])
		return FALSE;

	if (EQUAL_MAC_ADDR(rbssid, aucNullAddr))
		return FALSE;

#if CFG_AP_80211KVR_INTERFACE
	kalP2PCatBlockList(prGlueInfo, 1);
#endif

	if (fgIsblock) {
		for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
			if (EQUAL_MAC_ADDR(
				&(prGlueInfo->prP2PInfo[ucRoleIndex]
				->aucBlockMACList[i]), rbssid)) {
				DBGLOG(P2P, WARN, MACSTR
					" already in block list\n",
					MAC2STR(rbssid));
				return FALSE;
			}
		}
		for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
			if (EQUAL_MAC_ADDR(
				&(prGlueInfo->prP2PInfo
				[ucRoleIndex]
				->aucBlockMACList[i]),
				aucNullAddr)) {
				COPY_MAC_ADDR(
					&(prGlueInfo->prP2PInfo
					[ucRoleIndex]
					->aucBlockMACList[i]),
					rbssid);
				if (p2pFuncRoleToBssIdx(
					prGlueInfo->prAdapter,
					ucRoleIndex,
					&ucBssIdx) ==
					WLAN_STATUS_SUCCESS) {
					p2pFuncSetAclPolicy(
						prGlueInfo->prAdapter,
						ucBssIdx,
						PARAM_CUSTOM_ACL_POLICY_ADD,
						rbssid);
				}
#if CFG_AP_80211KVR_INTERFACE
				kalP2PCatBlockList(
					prGlueInfo,
					0);
#endif
				return FALSE;
			}
		}
	} else {
		for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
			if (EQUAL_MAC_ADDR(
					&(prGlueInfo->prP2PInfo[ucRoleIndex]
					->aucBlockMACList[i]), rbssid)) {
				COPY_MAC_ADDR(
					&(prGlueInfo->prP2PInfo[ucRoleIndex]
					->aucBlockMACList[i]), aucNullAddr);
				if (p2pFuncRoleToBssIdx(
					prGlueInfo->prAdapter,
					ucRoleIndex,
					&ucBssIdx) ==
					WLAN_STATUS_SUCCESS) {
					p2pFuncSetAclPolicy(
						prGlueInfo->prAdapter,
						ucBssIdx,
						PARAM_CUSTOM_ACL_POLICY_REMOVE,
						rbssid);
				}
#if CFG_AP_80211KVR_INTERFACE
				kalP2PCatBlockList(prGlueInfo, 0);
#endif
				return FALSE;
			}
		}
	}
#if CFG_AP_80211KVR_INTERFACE
	kalP2PCatBlockList(prGlueInfo, 0);
#endif

	return FALSE;

}

u_int8_t kalP2PResetBlockList(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex)
{
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;
	uint32_t i;
	uint8_t ucBssIdx = 0;

	if (!prGlueInfo || !prGlueInfo->prP2PInfo[ucRoleIndex])
		return FALSE;

	for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
		COPY_MAC_ADDR(
			&(prGlueInfo->prP2PInfo[ucRoleIndex]
			->aucBlockMACList[i]), aucNullAddr);
	}

	if (p2pFuncRoleToBssIdx(
		prGlueInfo->prAdapter,
		ucRoleIndex,
		&ucBssIdx) == WLAN_STATUS_SUCCESS) {
		p2pFuncSetAclPolicy(
			prGlueInfo->prAdapter,
			ucBssIdx,
			PARAM_CUSTOM_ACL_POLICY_CLEAR,
			NULL);
		p2pFuncSetAclPolicy(
			prGlueInfo->prAdapter,
			ucBssIdx,
			PARAM_CUSTOM_ACL_POLICY_DENY,
			NULL);
	}

	return TRUE;
}

#if CFG_AP_80211KVR_INTERFACE
void kalP2PCatBlockList(struct GLUE_INFO *prGlueInfo, bool flag)
{
	uint32_t i;
	uint8_t ucRoleIndex;

	if (flag)
		DBGLOG(INIT, DEBUG, "Before Set BlockLis\n");
	else
		DBGLOG(INIT, DEBUG, "After Set BlockLis\n");

	for (ucRoleIndex = 0; ucRoleIndex < KAL_P2P_NUM; ucRoleIndex++) {
		for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
			DBGLOG(INIT, DEBUG,
				"ucRoleIndex[%d]-BlockList[%d] MA="MACSTR"\n",
				ucRoleIndex, i,
				MAC2STR(&(prGlueInfo->prP2PInfo[ucRoleIndex]
				->aucBlockMACList[i])));
		}
	}
}
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief to compare the block list of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*---------------------------------------------------------------------------*/
u_int8_t kalP2PCmpBlockList(struct GLUE_INFO *prGlueInfo,
		uint8_t rbssid[PARAM_MAC_ADDR_LEN],
		uint8_t ucRoleIndex)
{
	uint8_t aucNullAddr[] = NULL_MAC_ADDR;
	u_int8_t fgIsExsit = FALSE;
	uint32_t i;

	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prP2PInfo[ucRoleIndex]);

	for (i = 0; i < P2P_MAXIMUM_CLIENT_COUNT; i++) {
		if (UNEQUAL_MAC_ADDR(rbssid, aucNullAddr)) {
			if (EQUAL_MAC_ADDR(
				&(prGlueInfo->prP2PInfo
				[ucRoleIndex]->aucBlockMACList[i]),
				rbssid)) {
				fgIsExsit = TRUE;
				return fgIsExsit;
			}
		}
	}

	return fgIsExsit;

}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to return the max clients of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*---------------------------------------------------------------------------*/
void kalP2PSetMaxClients(struct GLUE_INFO *prGlueInfo,
		uint32_t u4MaxClient,
		uint8_t ucRoleIndex)
{
	ASSERT(prGlueInfo);

	if (prGlueInfo->prP2PInfo[ucRoleIndex] == NULL)
		return;

	if (u4MaxClient == 0 ||
		u4MaxClient >= P2P_MAXIMUM_CLIENT_COUNT)
		prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients =
			P2P_MAXIMUM_CLIENT_COUNT;
	else
		prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients = u4MaxClient;

	DBGLOG(P2P, TRACE,
		"Role(%d) max client count = %u\n",
		ucRoleIndex,
		prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients);
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief to return the max clients of Hotspot
 *
 * \param[in]
 *           prGlueInfo
 *
 * \return
 */
/*---------------------------------------------------------------------------*/
u_int8_t kalP2PMaxClients(struct GLUE_INFO *prGlueInfo,
		uint32_t u4NumClient, uint8_t ucRoleIndex)
{
	ASSERT(prGlueInfo);

	if (prGlueInfo->prP2PInfo[ucRoleIndex] &&
		prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients) {
		if ((uint8_t) u4NumClient
			>= prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients)
			return TRUE;
		else
			return FALSE;
	}

	return FALSE;
}

uint8_t kalP2PGetMaxClients(struct GLUE_INFO *prGlueInfo, uint8_t ucRoleIndex)
{
	uint8_t ucCount = 0;

	if (prGlueInfo->prP2PInfo[ucRoleIndex])
		ucCount = prGlueInfo->prP2PInfo[ucRoleIndex]->ucMaxClients;

	return ucCount;
}
#endif

void kalP2pUnlinkBss(struct GLUE_INFO *prGlueInfo, uint8_t aucBSSID[])
{
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;

	ASSERT(prGlueInfo);
	ASSERT(aucBSSID);

	prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

	if (prGlueP2pInfo == NULL) {
		DBGLOG(P2P, ERROR, "NULL prP2PInfo[0]");
		return;
	}

	DBGLOG(P2P, INFO, "bssid: " MACSTR "\n", MAC2STR(aucBSSID));

	scanRemoveBssDescByBssid(prGlueInfo->prAdapter, aucBSSID);
}

void kalP2pIndicateQueuedMgmtFrame(struct GLUE_INFO *prGlueInfo,
		struct P2P_QUEUED_ACTION_FRAME *prFrame)
{
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct net_device *prNetdevice = (struct net_device *) NULL;

	if ((prGlueInfo == NULL) || (prFrame == NULL)) {
		ASSERT(FALSE);
		return;
	}

	DBGLOG(P2P, INFO, "Indicate queued p2p action frame.\n");

	if (prFrame->prHeader == NULL || prFrame->u2Length == 0) {
		DBGLOG(P2P, WARN, "Frame pointer is null or length is 0.\n");
		return;
	}

	prGlueP2pInfo = prGlueInfo->prP2PInfo[prFrame->ucRoleIdx];

	if ((prGlueP2pInfo->aprRoleHandler != NULL) &&
		(prGlueP2pInfo->aprRoleHandler != prGlueP2pInfo->prDevHandler))
		prNetdevice = prGlueP2pInfo->aprRoleHandler;
	else
		prNetdevice = prGlueP2pInfo->prDevHandler;

#if (KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE)
	cfg80211_rx_mgmt(
		/* struct net_device * dev, */
		prNetdevice->ieee80211_ptr,
		prFrame->u4Freq,
		0,
		prFrame->prHeader,
		prFrame->u2Length,
		NL80211_RXMGMT_FLAG_ANSWERED);
#elif (KERNEL_VERSION(3, 12, 0) <= CFG80211_VERSION_CODE)
	cfg80211_rx_mgmt(
		/* struct net_device * dev, */
		prNetdevice->ieee80211_ptr,
		prFrame->u4Freq,
		0,
		prFrame->prHeader,
		prFrame->u2Length,
		NL80211_RXMGMT_FLAG_ANSWERED,
		GFP_ATOMIC);
#else
	cfg80211_rx_mgmt(
		/* struct net_device * dev, */
		prNetdevice->ieee80211_ptr,
		prFrame->u4Freq,
		0,
		prFrame->prHeader,
		prFrame->u2Length,
		GFP_ATOMIC);
#endif
}

#if (CFG_SUPPORT_DFS_MASTER == 1)

void kalP2pPreStartRdd(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucRoleIdx,
	uint32_t ucPrimaryCh,
	enum ENUM_BAND eBand)
{
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211 && CFG_SUPPORT_DFS_MASTER
	uint32_t freq =
		nicChannelNum2Freq(ucPrimaryCh, eBand) / 1000;
	struct cfg80211_chan_def chandef;
	struct ieee80211_channel *chan;
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;

	prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

	if (!prGlueP2pInfo) {
		DBGLOG(P2P, ERROR, "p2p glue info null.\n");
		return;
	}
	kalMemZero(
		&chandef,
		sizeof(struct cfg80211_chan_def));
	chan = ieee80211_get_channel(
		prGlueP2pInfo->prWdev->wiphy,
		freq);
	cfg80211_chandef_create(&chandef,
		chan, NL80211_CHAN_NO_HT);

	kalP2pFuncPreStartRdd(
		prGlueInfo,
		ucRoleIdx,
		&chandef,
		P2P_AP_CAC_MIN_CAC_TIME_MS);
#endif
}


void kalP2pIndicateRadarEvent(struct GLUE_INFO *prGlueInfo,
	uint8_t ucRoleIndex,
	uint32_t event,
	uint32_t freq)
{
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211 && CFG_SUPPORT_DFS_MASTER
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct sk_buff *vendor_event = NULL;

	prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

	if (!prGlueP2pInfo) {
		DBGLOG(P2P, ERROR, "p2p glue info null.\n");
		return;
	}

	DBGLOG(P2P, INFO,
		"r=%d, event=%d, f=%d\n",
		ucRoleIndex,
		event,
		freq);

	vendor_event = cfg80211_vendor_event_alloc(
		prGlueP2pInfo->prWdev->wiphy,
		prGlueP2pInfo->prWdev,
		sizeof(uint32_t) + NLMSG_HDRLEN,
		event,
		GFP_KERNEL);

	if (!vendor_event) {
		DBGLOG(P2P, ERROR, "allocate vendor event fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u32(vendor_event,
			NL80211_ATTR_WIPHY_FREQ,
			freq) < 0)) {
		DBGLOG(P2P, ERROR, "put freq fail.\n");
		goto nla_put_failure;
	}

	cfg80211_vendor_event(vendor_event, GFP_KERNEL);

	return;

nla_put_failure:
	if (vendor_event)
		kfree_skb(vendor_event);
#endif
}
#endif /* CFG_SUPPORT_DFS_MASTER */

void kalP2pIndicateAcsResult(struct GLUE_INFO *prGlueInfo,
		uint8_t ucRoleIndex,
		int8_t icLinkId,
		enum ENUM_BAND eBand,
		uint8_t ucPrimaryCh,
		uint8_t ucSecondCh,
		uint8_t ucSeg0Ch,
		uint8_t ucSeg1Ch,
		enum ENUM_MAX_BANDWIDTH_SETTING eChnlBw,
		enum P2P_VENDOR_ACS_HW_MODE eHwMode,
		uint16_t u2PunctBitmap,
		enum ENUM_CHNL_EXT eSco)
{
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct sk_buff *vendor_event = NULL;
	uint16_t ch_width = MAX_BW_20MHZ;

	prGlueP2pInfo = prGlueInfo->prP2PInfo[ucRoleIndex];

	if (!prGlueP2pInfo || !prGlueP2pInfo->prWdev) {
		DBGLOG(P2P, ERROR, "p2p glue info null.\n");
		return;
	}

	switch (eChnlBw) {
	case MAX_BW_20MHZ:
		ch_width = 20;
		break;
	case MAX_BW_40MHZ:
		ch_width = 40;
		break;
	case MAX_BW_80MHZ:
		ch_width = 80;
		break;
	case MAX_BW_160MHZ:
		ch_width = 160;
		break;
	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		ch_width = 320;
		break;
	default:
		DBGLOG(P2P, ERROR, "unsupport width: %d.\n", ch_width);
		break;
	}

#if CFG_SUPPORT_SAP_DFS_CHANNEL
	/* Indicatre CAC */
	if ((eBand == BAND_5G) &&
	    (rlmDomainIsLegalDfsChannel(prGlueInfo->prAdapter,
					eBand,
					ucPrimaryCh) ||
	     (eChnlBw >= MAX_BW_160MHZ))) {
		DBGLOG(P2P, INFO, "Do pre CAC.\n");
		wlanDfsChannelsReqAdd(prGlueInfo->prAdapter,
			DFS_CHANNEL_CTRL_SOURCE_SAP,
			ucPrimaryCh,
			rlmGetVhtOpBwByBssOpBw(eChnlBw),
			0,
			nicChannelNum2Freq(ucSeg0Ch, eBand) / 1000,
			eBand);
	}
#endif

	DBGLOG(P2P, INFO,
		"%s l=%d r=%d, b=%d, c=%d, s=%d, s0=%d, s1=%d, ch_w=%d, h=%d, b=0x%x\n",
		prGlueP2pInfo->prWdev->netdev->name,
		icLinkId,
		ucRoleIndex,
		eBand,
		ucPrimaryCh,
		ucSecondCh,
		ucSeg0Ch,
		ucSeg1Ch,
		ch_width,
		eHwMode,
		u2PunctBitmap);

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	vendor_event = kalCfg80211VendorEventAlloc(prGlueP2pInfo->prWdev->wiphy,
			prGlueP2pInfo->prWdev,
			4 * sizeof(u8) + 1 * sizeof(u16) + 4 + NLMSG_HDRLEN,
			WIFI_EVENT_ACS,
			GFP_KERNEL);
#endif
	if (!vendor_event) {
		DBGLOG(P2P, ERROR, "allocate vendor event fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u32(vendor_event,
			WIFI_VENDOR_ATTR_ACS_PRIMARY_FREQUENCY,
			nicChannelNum2Freq(ucPrimaryCh, eBand) / 1000) < 0)) {
		DBGLOG(P2P, ERROR, "put primary channel fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u32(vendor_event,
			WIFI_VENDOR_ATTR_ACS_SECONDARY_FREQUENCY,
			nicChannelNum2Freq(ucSecondCh, eBand) / 1000) < 0)) {
		DBGLOG(P2P, ERROR, "put secondary channel fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u8(vendor_event,
			WIFI_VENDOR_ATTR_ACS_VHT_SEG0_CENTER_CHANNEL,
			ucSeg0Ch) < 0)) {
		DBGLOG(P2P, ERROR, "put vht seg0 fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u8(vendor_event,
			WIFI_VENDOR_ATTR_ACS_VHT_SEG1_CENTER_CHANNEL,
			ucSeg1Ch) < 0)) {
		DBGLOG(P2P, ERROR, "put vht seg1 fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u16(vendor_event,
			WIFI_VENDOR_ATTR_ACS_CHWIDTH,
			ch_width) < 0)) {
		DBGLOG(P2P, ERROR, "put ch width fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u8(vendor_event,
			WIFI_VENDOR_ATTR_ACS_HW_MODE,
			eHwMode) < 0)) {
		DBGLOG(P2P, ERROR, "put hw mode fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u16(vendor_event,
			WIFI_VENDOR_ATTR_ACS_PUNCTURE_BITMAP,
			u2PunctBitmap) < 0)) {
		DBGLOG(P2P, ERROR, "put punct bitmap fail.\n");
		goto nla_put_failure;
	}

	if (icLinkId != -1 &&
	    unlikely(nla_put_u8(vendor_event,
			WIFI_VENDOR_ATTR_ACS_LINK_ID,
			icLinkId) < 0)) {
		DBGLOG(P2P, ERROR, "put link id fail.\n");
		goto nla_put_failure;
	}

#if KERNEL_VERSION(3, 14, 0) <= LINUX_VERSION_CODE
	cfg80211_vendor_event(vendor_event, GFP_KERNEL);
#endif
	return;

nla_put_failure:
	if (vendor_event)
		kfree_skb(vendor_event);
}

u_int8_t kalP2pIsStoppingAp(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	struct net_device *prDevHandler = NULL;

	if (!prAdapter)
		return FALSE;

	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo,
		prBssInfo->ucBssIndex);

	if (prDevHandler && !netif_carrier_ok(prDevHandler))
		return TRUE;

	return FALSE;
}

void kalP2pNotifyStopApComplete(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex)
{
	struct GL_P2P_INFO *prP2PInfo;

	if (!prAdapter)
		return;

	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIndex];
	if (prP2PInfo && !completion_done(&prP2PInfo->rStopApComp))
		complete(&prP2PInfo->rStopApComp);
}

void kalP2pNotifyDisconnComplete(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex)
{
	struct GL_P2P_INFO *prP2PInfo;

	if (!prAdapter)
		return;

	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIndex];
	if (prP2PInfo && !completion_done(&prP2PInfo->rDisconnComp))
		complete(&prP2PInfo->rDisconnComp);
}

void kalP2pNotifyDelStaComplete(struct ADAPTER *prAdapter,
		uint8_t ucRoleIndex)
{
	struct GL_P2P_INFO *prP2PInfo;

	if (!prAdapter)
		return;

	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIndex];
	if (prP2PInfo && !completion_done(&prP2PInfo->rDelStaComp))
		complete(&prP2PInfo->rDelStaComp);
}

void __kalP2pIndicateChnlSwitchStarted(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	struct RF_CHANNEL_INFO *prRfChnlInfo,
	uint8_t ucCsaCount,
	u_int8_t fgQuiet,
	u_int8_t fgLockHeld)
{
	struct GL_P2P_INFO *prP2PInfo;
	struct net_device *prNetdevice;
	struct cfg80211_chan_def chandef;
	struct ieee80211_channel *chan;
	enum nl80211_channel_type rChannelType;
	enum ENUM_CHNL_EXT eChnlSco;
	uint8_t ucRoleIdx;
	uint8_t ucLinkIdx = 0;

	if (!prAdapter || !prBssInfo || !prRfChnlInfo)
		return;

#if (CFG_SUPPORT_NAN == 1)
	/* Set complete for nan init */
	if (!kal_completion_done(
		&prAdapter->prGlueInfo->rNanHaltComp) &&
		prAdapter->fgIsNANCSAWaiting) {
		DBGLOG(NAN, DEBUG,
			"Concurrency: Skip lock NAN\n");
		fgLockHeld = TRUE;
	}
#endif /* CFG_SUPPORT_NAN */

	ucRoleIdx = prBssInfo->u4PrivateData;
#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE) || \
	(CFG_ADVANCED_80211_MLO == 1)
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (IS_BSS_GC(prBssInfo) || IS_BSS_AP(prAdapter, prBssInfo))
		ucLinkIdx = prBssInfo->ucLinkId;
#endif
#endif
	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx];

	if (prP2PInfo->aprRoleHandler != NULL &&
	    prP2PInfo->aprRoleHandler != prP2PInfo->prDevHandler)
		prNetdevice = prP2PInfo->aprRoleHandler;
	else
		prNetdevice = prP2PInfo->prDevHandler;

	chan = ieee80211_get_channel(prP2PInfo->prWdev->wiphy,
		nicChannelNum2Freq(prRfChnlInfo->ucChannelNum,
				   prRfChnlInfo->eBand) / 1000);
	if (!chan) {
		DBGLOG(P2P, ERROR,
			"Get channel failed by channel(%d) band(%d)\n",
			prRfChnlInfo->ucChannelNum,
			prRfChnlInfo->eBand);
		return;
	}

	eChnlSco = rlmGetScoByChnInfo(prAdapter, prRfChnlInfo);
	switch (eChnlSco) {
	case CHNL_EXT_SCA:
		rChannelType = NL80211_CHAN_HT40PLUS;
		break;
	case CHNL_EXT_SCB:
		rChannelType = NL80211_CHAN_HT40MINUS;
		break;
	case CHNL_EXT_RES:
		rChannelType = NL80211_CHAN_HT20;
		break;
	case CHNL_EXT_SCN:
	default:
		rChannelType = NL80211_CHAN_NO_HT;
		break;
	}

	cfg80211_chandef_create(&chandef, chan, rChannelType);
	chandef.width = __kalP2pGetNl80211ChnlBw(prRfChnlInfo);
	chandef.center_freq1 =
		nicGetCenterChFreq(prRfChnlInfo->eBand,
				   prRfChnlInfo->ucChannelNum,
				   eChnlSco,
				   prRfChnlInfo->ucChnlBw);
	chandef.center_freq2 = 0;

	DBGLOG(P2P, INFO,
		"name=%s role=%u link=%u b=%d f=%d w=%d s1=%d s2=%d\n",
		prNetdevice->name,
		ucRoleIdx,
		ucLinkIdx,
		chandef.chan->band,
		chandef.chan->center_freq,
		chandef.width,
		chandef.center_freq1,
		chandef.center_freq2);

	/* Notify hostapd channel switch started only for AP mode
	 * to avoid duplicated csa
	 */
	if (prP2PInfo->prWdev->iftype != NL80211_IFTYPE_AP)
		goto queue_ctrl;

	if (!fgLockHeld) {
#if (KERNEL_VERSION(6, 7, 0) <= CFG80211_VERSION_CODE)
		wiphy_lock(prNetdevice->ieee80211_ptr->wiphy);
#else
		mutex_lock(&prNetdevice->ieee80211_ptr->mtx);
#endif
	}

#if (KERNEL_VERSION(6, 9, 0) <= CFG80211_VERSION_CODE)
#if (CFG_SUPPORT_SAP_CSA_PUNCTURE == 1)
	chandef.punctured = prAdapter->rWifiVar.u2NewPunctBitmap;
#endif /* CFG_SUPPORT_SAP_CSA_PUNCTURE */
	cfg80211_ch_switch_started_notify(prNetdevice, &chandef,
					  ucLinkIdx, ucCsaCount, fgQuiet);
#elif (KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE) || \
	(CFG_ADVANCED_80211_MLO == 1)
	cfg80211_ch_switch_started_notify(prNetdevice, &chandef,
					  ucLinkIdx, ucCsaCount, fgQuiet, 0);
#elif KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE
	cfg80211_ch_switch_started_notify(prNetdevice, &chandef,
					  ucLinkIdx, ucCsaCount, fgQuiet);
#elif KERNEL_VERSION(5, 11, 0) <= CFG80211_VERSION_CODE
	cfg80211_ch_switch_started_notify(prNetdevice, &chandef, ucCsaCount,
					  fgQuiet);
#elif KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE
	cfg80211_ch_switch_started_notify(prNetdevice, &chandef, ucCsaCount);
#endif

	if (!fgLockHeld) {
#if (KERNEL_VERSION(6, 7, 0) <= CFG80211_VERSION_CODE)
		wiphy_unlock(prNetdevice->ieee80211_ptr->wiphy);
#else
		mutex_unlock(&prNetdevice->ieee80211_ptr->mtx);
#endif
	}

queue_ctrl:
	if (fgQuiet)
		kalIndicateAllQueueTxAllowed(prAdapter->prGlueInfo,
					     prBssInfo->ucBssIndex, FALSE);
}

void kalP2pCsaNotifyWorkInit(struct BSS_INFO *prBssInfo)
{
	struct work_struct *prNotifyWork;

	if (!prBssInfo)
		return;

	prNotifyWork =
		&(prBssInfo->rGlChSwitchStartWork.rChSwitchStartNotifyWork);

	INIT_WORK(prNotifyWork, kalP2pChnlSwitchStartNotifyWork);
	prBssInfo->rGlChSwitchStartWork.fgWorkInit = TRUE;
}

void kalP2pChnlSwitchStartNotifyWork(struct work_struct *work)
{
	struct GL_CH_SWITCH_START_WORK *prWorkContainer =
		CONTAINER_OF(work, struct GL_CH_SWITCH_START_WORK,
			rChSwitchStartNotifyWork);
	struct ADAPTER *prAdapter;
	struct BSS_INFO *prBssInfo;

	prBssInfo = CONTAINER_OF(prWorkContainer, struct BSS_INFO,
				 rGlChSwitchStartWork);
	prAdapter = (struct ADAPTER *)
		(((int8_t *) (prBssInfo - prBssInfo->ucBssIndex)) -
		OFFSET_OF(struct WIFI_VAR, arBssInfoPool) -
		OFFSET_OF(struct ADAPTER, rWifiVar));

	if (!prAdapter || !prAdapter->prGlueInfo ||
	    prAdapter->prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}

	__kalP2pIndicateChnlSwitchStarted(prAdapter,
			prBssInfo,
			&prBssInfo->rGlChSwitchStartWork.rRfChnlInfo,
			prAdapter->rWifiVar.ucChannelSwitchCount,
			(prAdapter->rWifiVar.ucChannelSwitchMode == 1),
			FALSE);
}

void kalP2pIndicateChnlSwitchStarted(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo,
		struct RF_CHANNEL_INFO *prRfChnlInfo,
		uint8_t ucCsaCount,
		u_int8_t fgQuiet,
		u_int8_t fgLockHeld)
{
	struct work_struct *prNotifyWork;

	kalMemCopy(&prBssInfo->rGlChSwitchStartWork.rRfChnlInfo,
	   prRfChnlInfo,
	   sizeof(struct RF_CHANNEL_INFO));
	prNotifyWork =
		&(prBssInfo->rGlChSwitchStartWork.rChSwitchStartNotifyWork);

	schedule_work(prNotifyWork);
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
int32_t kalP2pFuncPreStartRdd(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucRoleIdx,
	struct cfg80211_chan_def *chandef,
	unsigned int cac_time_ms)
{
	int32_t i4Rslt = -EINVAL;
	u_int8_t fgWidthInvalid = FALSE;
	struct MSG_P2P_DFS_CAC *prP2pDfsCacMsg =
		(struct MSG_P2P_DFS_CAC *) NULL;
	struct RF_CHANNEL_INFO rRfChnlInfo;

	if ((prGlueInfo == NULL) || (chandef == NULL))
		goto out;

	kalMemZero(
		&(prGlueInfo->prP2PInfo[ucRoleIdx]->chandefCsa),
		sizeof(struct cfg80211_chan_def));
	prGlueInfo->prP2PInfo[ucRoleIdx]->chandefCsa.chan
		= (struct ieee80211_channel *)
		&(prGlueInfo->prP2PInfo[ucRoleIdx]->chanCsa);
	kalMemZero(
		prGlueInfo->prP2PInfo[ucRoleIdx]->chandefCsa.chan,
		sizeof(struct ieee80211_channel));
	kalMemZero(&rRfChnlInfo, sizeof(struct RF_CHANNEL_INFO));

	/* Copy chan def to local buffer*/
	prGlueInfo->prP2PInfo[ucRoleIdx]
		->chandefCsa.center_freq1 = chandef->center_freq1;
	prGlueInfo->prP2PInfo[ucRoleIdx]
		->chandefCsa.center_freq2 = chandef->center_freq2;
	prGlueInfo->prP2PInfo[ucRoleIdx]
		->chandefCsa.width = chandef->width;
	kalMemCopy(prGlueInfo->prP2PInfo[ucRoleIdx]->chandefCsa.chan,
		chandef->chan, sizeof(struct ieee80211_channel));
	prGlueInfo->prP2PInfo[ucRoleIdx]->cac_time_ms = cac_time_ms;


	if (chandef) {
		kalChannelFormatSwitch(chandef, chandef->chan,
				&rRfChnlInfo);

		p2pFuncSetChannel(prGlueInfo->prAdapter,
			ucRoleIdx, &rRfChnlInfo);
	}

	DBGLOG(P2P, INFO,
		"mtk_p2p_cfg80211_start_radar_detection.(role %d)\n",
		ucRoleIdx);

	p2pFuncSetDfsState(DFS_STATE_INACTIVE);

	prP2pDfsCacMsg = (struct MSG_P2P_DFS_CAC *)
		cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, sizeof(*prP2pDfsCacMsg));

	if (prP2pDfsCacMsg == NULL) {
		i4Rslt = -ENOMEM;
		goto out;
	}

	prP2pDfsCacMsg->rMsgHdr.eMsgId = MID_MNY_P2P_DFS_CAC;

	switch (chandef->width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
	case NL80211_CHAN_WIDTH_20:
	case NL80211_CHAN_WIDTH_40:
		prP2pDfsCacMsg->eChannelWidth = CW_20_40MHZ;
		break;

	case NL80211_CHAN_WIDTH_80:
		prP2pDfsCacMsg->eChannelWidth = CW_80MHZ;
		break;

	case NL80211_CHAN_WIDTH_160:
		prP2pDfsCacMsg->eChannelWidth = CW_160MHZ;
		break;

	case NL80211_CHAN_WIDTH_80P80:
		prP2pDfsCacMsg->eChannelWidth = CW_80P80MHZ;
		break;

	default:
		DBGLOG(P2P, ERROR,
			"!!!Bandwidth do not support!!!\n");
		fgWidthInvalid = TRUE;
		break;
	}

	if (fgWidthInvalid)
		goto out;

	prP2pDfsCacMsg->ucRoleIdx = ucRoleIdx;

	mboxSendMsg(prGlueInfo->prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prP2pDfsCacMsg,
		MSG_SEND_METHOD_BUF);

	i4Rslt = 0;

out:
	return i4Rslt;
}
#endif

void kalP2pClearCsaChan(struct GL_P2P_INFO *prGlueP2pInfo)
{
	kalMemZero(
		&(prGlueP2pInfo->chandefCsa),
		sizeof(struct cfg80211_chan_def));
	prGlueP2pInfo->chandefCsa.chan
		= (struct ieee80211_channel *)
		&(prGlueP2pInfo->chanCsa);
	kalMemZero(
		prGlueP2pInfo->chandefCsa.chan,
		sizeof(struct ieee80211_channel));
}

void kalSetP2pRoleMac(
		struct ADAPTER *prAdapter,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t ucRoleIdx)
{
	COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
		      prAdapter->rWifiVar.aucP2pInterfaceAddress[ucRoleIdx]);

	DBGLOG(P2P, TRACE, "Set Bss[%d], macAddr: " MACSTR "\n",
		prP2pBssInfo->ucBssIndex, MAC2STR(prP2pBssInfo->aucOwnMacAddr));
}

void kalSetP2pDevMac(
		struct GLUE_INFO *prGlueInfo,
		struct BSS_INFO *prP2pBssInfo,
		uint8_t ucRoleIdx)
{
	struct GL_P2P_INFO *prP2PInfo = NULL;

	prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
	COPY_MAC_ADDR(prP2pBssInfo->aucOwnMacAddr,
		      prP2PInfo->prDevHandler->dev_addr);

	DBGLOG(P2P, TRACE, "Set Bss[%d], macAddr: " MACSTR "\n",
		prP2pBssInfo->ucBssIndex, MAC2STR(prP2pBssInfo->aucOwnMacAddr));
}

void *kalGetP2pNetHdl(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Idx, u_int8_t fgIsRole)
{
	void *pvHandler = NULL;

	if (prGlueInfo != NULL &&
			prGlueInfo->prP2PInfo[u4Idx] != NULL) {
		pvHandler = fgIsRole ?
		(void *)prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler :
		(void *)prGlueInfo->prP2PInfo[u4Idx]->prDevHandler;
	}

	return pvHandler;
}

#if CFG_AP_80211KVR_INTERFACE
int32_t kalGetMulAPIfIdx(struct GLUE_INFO *prGlueInfo,
	uint32_t u4Idx, uint32_t *pu4IfIndex)
{
	int32_t i4Ret = 0;

	if (prGlueInfo != NULL &&
			prGlueInfo->prP2PInfo[u4Idx] != NULL) {
		i4Ret = sscanf(
			prGlueInfo->prP2PInfo[u4Idx]->prDevHandler->name,
			"ap%u", pu4IfIndex);
		if (i4Ret)
			return i4Ret;
	}

	return i4Ret;
}
#endif


void *kalGetP2pDevScanReq(struct GLUE_INFO *prGlueInfo)
{
	void *pvRet = NULL;

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (prGlueInfo && prGlueInfo->prP2PDevInfo)
		pvRet = (void *)(prGlueInfo->prP2PDevInfo->prScanRequest);
#endif

	return pvRet;
}

u_int8_t kalGetP2pDevScanSpecificSSID(struct GLUE_INFO *prGlueInfo)
{
	u_int8_t fgScanSpecificSSID = FALSE;
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (prGlueInfo && prGlueInfo->prP2PDevInfo) {
		fgScanSpecificSSID =
			prGlueInfo->prP2PDevInfo->fgScanSpecificSSID;
	}
#endif
	return fgScanSpecificSSID;
}

void kalP2pIndicateListenOffloadEvent(
	struct GLUE_INFO *prGlueInfo,
	uint32_t event)
{
	struct GL_P2P_INFO *prGlueP2pInfo =
		(struct GL_P2P_INFO *) NULL;
	struct sk_buff *vendor_event = NULL;

	prGlueP2pInfo = prGlueInfo->prP2PInfo[0];
	if (!prGlueP2pInfo) {
		DBGLOG(P2P, ERROR, "p2p glue info null.\n");
		return;
	}

	vendor_event =
		cfg80211_vendor_event_alloc(
		prGlueP2pInfo->prWdev->wiphy,
		prGlueP2pInfo->prWdev,
		sizeof(uint32_t) + NLMSG_HDRLEN,
		WIFI_EVENT_P2P_LISTEN_OFFLOAD,
		GFP_KERNEL);
	if (!vendor_event) {
		DBGLOG(P2P, ERROR, "allocate vendor event fail.\n");
		goto nla_put_failure;
	}

	if (unlikely(nla_put_u32(vendor_event,
		QCA_WLAN_VENDOR_ATTR_P2P_LO_STOP_REASON,
		event) < 0)) {
		DBGLOG(P2P, ERROR, "put freq fail.\n");
		goto nla_put_failure;
	}

	cfg80211_vendor_event(vendor_event, GFP_KERNEL);

	DBGLOG(P2P, INFO,
		"p2p_lo event: %d\n", event);

	return;

nla_put_failure:
	if (vendor_event)
		kfree_skb(vendor_event);
}

#if CFG_SUPPORT_IDC_RIL_BRIDGE
struct CP_NOTI_INFO {
	uint8_t ucRat; /* LTE or NR */
	uint32_t u4Band;
	uint32_t u4Channel;
} __packed;
#if !CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
struct dev_ril_bridge_msg {
	unsigned int dev_id;
	unsigned int data_len;
	void *data;
};
#endif

void kalSetRilBridgeChannelInfo(
	struct ADAPTER *prAdapter,
	uint8_t ucRat,
	uint32_t u4Band,
	uint32_t u4Channel)
{
	struct CMD_SET_IDC_RIL_BRIDGE *prCmd;
	uint16_t u2CmdBufLen = 0;

	do {
		if (!prAdapter)
			break;

		u2CmdBufLen =
			sizeof(struct CMD_SET_IDC_RIL_BRIDGE);

		prCmd = (struct CMD_SET_IDC_RIL_BRIDGE *)
			cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			u2CmdBufLen);
		if (!prCmd) {
			DBGLOG(P2P, ERROR,
				"cnmMemAlloc for prCmd failed!\n");
			break;
		}

		prCmd->ucRat = ucRat;
		prCmd->u4Band = u4Band;
		prCmd->u4Channel = u4Channel;

		DBGLOG(INIT, TRACE,
			"Update CP channel info [%d,%d,%d]\n",
			prCmd->ucRat,
			prCmd->u4Band,
			prCmd->u4Channel);

		wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_IDC_RIL,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			u2CmdBufLen,
			(uint8_t *) prCmd,
			NULL,
			0);

		cnmMemFree(prAdapter, prCmd);
	} while (FALSE);
}

static int kalIdcRilNotifier(
	struct notifier_block *nb,
	unsigned long size,
	void *buf)
{
	struct dev_ril_bridge_msg *msg;
	struct CP_NOTI_INFO *cmd;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (!nb) {
		DBGLOG(INIT, WARN, "notifier_block invalid!!\n");
		return NOTIFY_DONE;
	}

	prGlueInfo = CONTAINER_OF(nb,
				struct GLUE_INFO, ril_notifier_block);

	if (!prGlueInfo ||
		!prGlueInfo->prAdapter) {
		DBGLOG(INIT, WARN,
			   "prGlueInfo invalid!!\n");
		return NOTIFY_DONE;
	}

	if (!prGlueInfo->init_ril_notifier) {
		DBGLOG(INIT, WARN,
			"Not init ril notifier\n");
		return NOTIFY_DONE;
	}

	DBGLOG(INIT, LOUD,
		"ril notification size %lu\n", size);

	msg = (struct dev_ril_bridge_msg *)buf;

	DBGLOG(INIT, LOUD,
		"dev_id : %d, data_len : %d\n",
		msg->dev_id, msg->data_len);

	if (msg->dev_id == IDC_RIL_CHANNEL_INFO
		&& msg->data_len ==
		sizeof(struct CP_NOTI_INFO)) {

		cmd = (struct CP_NOTI_INFO *)msg->data;

		DBGLOG(INIT, TRACE,
			"Update CP channel info [%d,%d,%d]\n",
			cmd->ucRat, cmd->u4Band, cmd->u4Channel);

		/* rat mode : LTE (3), NR5G (7) */
		if ((cmd->ucRat == IDC_RIL_BRIDGE_LTE) ||
			(cmd->ucRat == IDC_RIL_BRIDGE_NR))
			kalSetRilBridgeChannelInfo(
				prGlueInfo->prAdapter,
				cmd->ucRat,
				cmd->u4Band,
				cmd->u4Channel);

		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}
#endif
#if CFG_SUPPORT_IDC_RIL_BRIDGE_NOTIFY
void kalIdcGetRilInfo(void)
{
	int val = 1;

	DBGLOG(INIT, DEBUG, "Get RIL Notifier\n");

	dev_ril_bridge_send_msg(
		IDC_RIL_CHANNEL_INFO,
		sizeof(int), &val);
}

void kalIdcRegisterRilNotifier(struct GLUE_INFO *prGlueInfo)
{
	struct notifier_block *prNotifier = NULL;

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	prNotifier = &prGlueInfo->ril_notifier_block;
	prNotifier->notifier_call = kalIdcRilNotifier;

	if (!prGlueInfo->init_ril_notifier) {
		DBGLOG(INIT, DEBUG, "Register RIL Notifier\n");

		register_dev_ril_bridge_event_notifier(
			prNotifier);

		kalIdcGetRilInfo();

		prGlueInfo->init_ril_notifier = 1;
	}
}

void kalIdcUnregisterRilNotifier(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL\n");
		return;
	}

	if (!prGlueInfo->init_ril_notifier) {
		unregister_dev_ril_bridge_event_notifier(
			&prGlueInfo->ril_notifier_block);
		prGlueInfo->init_ril_notifier = 0;
		DBGLOG(INIT, DEBUG, "Unregister RIL Notifier\n");
	}
}
#endif

static inline enum nl80211_chan_width
__kalP2pGetNl80211ChnlBw(struct RF_CHANNEL_INFO *prRfChnlInfo)
{
	if (!prRfChnlInfo) {
		DBGLOG(P2P, ERROR, "NULL channel info.\n");
		return NL80211_CHAN_WIDTH_20;
	}

	switch (prRfChnlInfo->ucChnlBw) {
#if KERNEL_VERSION(5, 18, 0) <= CFG80211_VERSION_CODE
	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		return NL80211_CHAN_WIDTH_320;
#endif
	case MAX_BW_80_80_MHZ:
		return NL80211_CHAN_WIDTH_80P80;
	case MAX_BW_160MHZ:
		return NL80211_CHAN_WIDTH_160;
	case MAX_BW_80MHZ:
		return NL80211_CHAN_WIDTH_80;
	case MAX_BW_40MHZ:
		return NL80211_CHAN_WIDTH_40;
	case MAX_BW_20MHZ:
	default:
		return NL80211_CHAN_WIDTH_20;
	}
}

void kalP2pStopApInterface(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo)
{
	struct wiphy *wiphy = NULL;
	struct GL_P2P_INFO *prP2PInfo;
	struct net_device *prNetdevice;
	uint8_t ucRoleIdx;

	if (!prAdapter || !prBssInfo)
		return;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	ucRoleIdx = prBssInfo->u4PrivateData;
	prP2PInfo = prAdapter->prGlueInfo->prP2PInfo[ucRoleIdx];

	if (prP2PInfo->aprRoleHandler != NULL &&
	    prP2PInfo->aprRoleHandler != prP2PInfo->prDevHandler)
		prNetdevice = prP2PInfo->aprRoleHandler;
	else
		prNetdevice = prP2PInfo->prDevHandler;

	DBGLOG(P2P, INFO, "AP interface (%s) leaving.\n",
		prNetdevice->name);

	cfg80211_stop_iface(wiphy, prNetdevice->ieee80211_ptr, GFP_KERNEL);
}

void kalP2pCuRptUevent(struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand, uint8_t ucPrimaryCh,
	uint8_t ucChnlLoad)
{
	char uEvent[300];

	kalSnprintf(uEvent, sizeof(uEvent),
		"CuRpt=Band:%u,Ch:%u,Cu:%u",
		eBand, ucPrimaryCh, ucChnlLoad);

	DBGLOG(P2P, LOUD, "CuRpt: %u and %u\n",
		ucPrimaryCh, ucChnlLoad);
	kalSendUevent(prAdapter, uEvent);
}

#endif /* CFG_ENABLE_WIFI_DIRECT */
