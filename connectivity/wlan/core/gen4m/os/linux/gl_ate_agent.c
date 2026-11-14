// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
	Module Name:
	gl_ate_agent.c
*/
/*******************************************************************************
 *					C O M P I L E R	 F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *					E X T E R N A L	R E F E R E N C E S
 *******************************************************************************
 */

#include "precomp.h"
#if CFG_SUPPORT_QA_TOOL
#include "gl_wext.h"
#include "gl_cfg80211.h"
#include "gl_ate_agent.h"
#include "gl_hook_api.h"
#include "gl_qa_agent.h"
#if KERNEL_VERSION(3, 8, 0) <= CFG80211_VERSION_CODE
#include <uapi/linux/nl80211.h>
#endif

/*******************************************************************************
 *					C O N S T A N T S
 *******************************************************************************
 */

#if CFG_SUPPORT_TX_BF
union PFMU_PROFILE_TAG1 g_rPfmuTag1;
union PFMU_PROFILE_TAG2 g_rPfmuTag2;
union PFMU_DATA g_rPfmuData;
struct PFMU_HE_INFO g_rPfmuHeInfo;
uint8_t g_rBandIdx;
#endif

struct ATE_PRIV_CMD {
	uint8_t *name;
	int (*set_proc)(struct net_device *prNetDev,
			uint8_t *prInBuf);
};

struct ATE_PRIV_CMD rAtePrivCmdTable[] = {
	{"ResetCounter", Set_ResetStatCounter_Proc},
	{"ATE", SetATE},
#if 0
	{"ADCDump", SetADCDump},
	{"ATEBSSID", SetATEBssid},
#endif
	{"ATEDA", SetATEDa},
	{"ATESA", SetATESa},
	{"ATECHANNEL", SetATEChannel},
	{"ATETXPOW0", SetATETxPower0},
	{"ATETXGI", SetATETxGi},
	{"ATETXBW", SetATETxBw},
	{"ATETXLEN", SetATETxLength},
	{"ATETXCNT", SetATETxCount},
	{"ATETXMCS", SetATETxMcs},
	{"ATETXMODE", SetATETxMode},
	{"ATEIPG", SetATEIpg},
	{"ATEVHTNSS", SetATETxVhtNss},
	{"ATETXANT", SetATETxPath},
	{"ATERXANT", SetATERxPath},
#if CFG_SUPPORT_ANT_SWAP
	{"ATEANTSWP", SetATEAntSwp},
#endif
#if CFG_SUPPORT_TX_BF
	{"TxBfProfileTagHelp", Set_TxBfProfileTag_Help},
	{"TxBfProfileTagInValid", Set_TxBfProfileTag_InValid},
	{"TxBfProfileTagPfmuIdx", Set_TxBfProfileTag_PfmuIdx},
	{"TxBfProfileTagBfType", Set_TxBfProfileTag_BfType},
	{"TxBfProfileTagBw", Set_TxBfProfileTag_DBW},
	{"TxBfProfileTagSuMu", Set_TxBfProfileTag_SuMu},
	{"TxBfProfileTagMemAlloc", Set_TxBfProfileTag_Mem},
	{"TxBfProfileTagMatrix", Set_TxBfProfileTag_Matrix},
	{"TxBfProfileTagSnr", Set_TxBfProfileTag_SNR},
	{"TxBfProfileTagSmtAnt", Set_TxBfProfileTag_SmartAnt},
	{"TxBfProfileTagSeIdx", Set_TxBfProfileTag_SeIdx},
	{"TxBfProfileTagRmsdThrd", Set_TxBfProfileTag_RmsdThrd},
	{"TxBfProfileTagMcsThrd", Set_TxBfProfileTag_McsThrd},
	{"TxBfProfileTagTimeOut", Set_TxBfProfileTag_TimeOut},
	{"TxBfProfileTagDesiredBw", Set_TxBfProfileTag_DesiredBW},
	{"TxBfProfileTagDesiredNc", Set_TxBfProfileTag_DesiredNc},
	{"TxBfProfileTagDesiredNr", Set_TxBfProfileTag_DesiredNr},
	{"TxBfProfileTagBandIdx", Set_TxBfProfileTag_BandIdx},
	{"TxBfProfileTagRead", Set_TxBfProfileTagRead},
	{"TxBfProfileTagWrite", Set_TxBfProfileTagWrite},
	{"TxBfProfileDataRead", Set_TxBfProfileDataRead},
	{"TxBfProfileDataWrite", Set_TxBfProfileDataWrite},
	{"TxBfProfilePnRead", Set_TxBfProfilePnRead},
	{"TxBfProfilePnWrite", Set_TxBfProfilePnWrite},
	{"TriggerSounding", Set_Trigger_Sounding_Proc},
	{"TxBfSoundingStop", Set_Stop_Sounding_Proc},
	{"TxBfTxApply", Set_TxBfTxApply},
	{"TxBfManualAssoc", Set_TxBfManualAssoc},
	{"TxBfPfmuMemAlloc", Set_TxBfPfmuMemAlloc},
	{"TxBfPfmuMemRelease", Set_TxBfPfmuMemRelease},
	{"StaRecCmmUpdate", Set_StaRecCmmUpdate},
	{"StaRecBfUpdate", Set_StaRecBfUpdate},
	{"StaRecBfRead", Set_StaRecBfRead},
	{"StaRecBfHeUpdate", Set_StaRecBfHeUpdate},
	{"DevInfoUpdate", Set_DevInfoUpdate},
	{"BssInfoUpdate", Set_BssInfoUpdate},
	{"TxBfProfileTagPartialBw", Set_TxBfProfileTagPartialBw},
#if CFG_SUPPORT_MU_MIMO
	{"MUGetInitMCS", Set_MUGetInitMCS},
	{"MUCalInitMCS", Set_MUCalInitMCS},
	{"MUCalLQ", Set_MUCalLQ},
	{"MUGetLQ", Set_MUGetLQ},
	{"MUSetSNROffset", Set_MUSetSNROffset},
	{"MUSetZeroNss", Set_MUSetZeroNss},
	{"MUSetSpeedUpLQ", Set_MUSetSpeedUpLQ},
	{"MUSetMUTable", Set_MUSetMUTable},
	{"MUSetGroup", Set_MUSetGroup},
	{"MUGetQD", Set_MUGetQD},
	{"MUSetEnable", Set_MUSetEnable},
	{"MUSetGID_UP", Set_MUSetGID_UP},
	{"MUTriggerTx", Set_MUTriggerTx},
#endif

#if CFG_SUPPORT_TX_BF_FPGA
	{"TxBfProfileSwTagWrite", Set_TxBfProfileSwTagWrite},
#endif

#endif

	{"WriteEfuse", WriteEfuse},
	{"TxPower", SetTxTargetPower},
#if (CFG_SUPPORT_DFS_MASTER == 1)
	{"RDDReport", SetRddReport},
	{"ByPassCac", SetByPassCac},
	{"RadarDetectMode", SetRadarDetectMode},
#endif

	{NULL,}
};

/*******************************************************************************
 *				F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Reset RX Statistic Counters.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 */
/*----------------------------------------------------------------------------*/
int Set_ResetStatCounter_Proc(struct net_device *prNetDev,
			      uint8_t *prInBuf)
{
	int32_t i4Status;

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set_ResetStatCounter_Proc\n");

	i4Status = MT_ATEResetTXRXCounter(prNetDev);

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Start Test Mode / Stop Test Mode
 *         / Start TX / Stop TX / Start RX / Stop RX.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATE(struct net_device *prNetDev, uint8_t *prInBuf)
{
	int32_t i4Status;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s\n", __func__);

	if (!strcmp(prInBuf, "ATESTART")) {
		DBGLOG(REQ, DEBUG,
		       "ATE_AGENT iwpriv %s - ATESTART\n", __func__);
		i4Status = MT_ATEStart(prNetDev, prInBuf);
	} else if (!strcmp(prInBuf, "ICAPSTART")) {
		DBGLOG(REQ, DEBUG,
		       "ATE_AGENT iwpriv %s - ICAPSTART\n", __func__);
		i4Status = MT_ICAPStart(prNetDev, prInBuf);
	} else if (prInBuf[0] == '1' || prInBuf[0] == '2'
		   || prInBuf[0] == '3' || prInBuf[0] == '4'
		   || prInBuf[0] == '5' || prInBuf[0] == '6') {
		DBGLOG(REQ, DEBUG,
		       "ATE_AGENT iwpriv SetATE - ICAP COMMAND\n");
		i4Status = MT_ICAPCommand(prNetDev, prInBuf);
	} else if (!strcmp(prInBuf, "ATESTOP")) {
		DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s - ATESTOP\n", __func__);
		i4Status = MT_ATEStop(prNetDev, prInBuf);
	} else if (!strcmp(prInBuf, "TXFRAME")) {
		DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s - TXFRAME\n", __func__);
		i4Status = MT_ATEStartTX(prNetDev, prInBuf);
	} else if (!strcmp(prInBuf, "TXSTOP")) {
		DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s - TXSTOP\n", __func__);
		i4Status = MT_ATEStopTX(prNetDev, prInBuf);
	} else if (!strcmp(prInBuf, "RXFRAME")) {
		DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s - RXFRAME\n", __func__);
		i4Status = MT_ATEStartRX(prNetDev, prInBuf);
	} else if (!strcmp(prInBuf, "RXSTOP")) {
		DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s - RXSTOP\n", __func__);
		i4Status = MT_ATEStopRX(prNetDev, prInBuf);
	} else {
		return -EINVAL;
	}

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX Destination Address.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATEDa(struct net_device *prNetDev, uint8_t *prInBuf)
{
	int32_t i4Status = 0, i = 0;
	uint32_t addr[MAC_ADDR_LEN];
	uint8_t addr2[MAC_ADDR_LEN];
	uint32_t *params[] = {
		&addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]
	};
	uint8_t *endptr = prInBuf;
	uint8_t *token;

	if (prInBuf == NULL)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	addr2[0] = (uint8_t) addr[0];
	addr2[1] = (uint8_t) addr[1];
	addr2[2] = (uint8_t) addr[2];
	addr2[3] = (uint8_t) addr[3];
	addr2[4] = (uint8_t) addr[4];
	addr2[5] = (uint8_t) addr[5];

	i4Status = MT_ATESetMACAddress(prNetDev,
		RF_AT_FUNCID_SET_MAC_ADDRESS, addr2);

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX Source Address.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATESa(struct net_device *prNetDev, uint8_t *prInBuf)
{
	int32_t i4Status = 0, i = 0;
	uint32_t addr[MAC_ADDR_LEN];
	uint8_t addr2[MAC_ADDR_LEN];
	uint8_t *endptr;
	uint8_t *token;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "ATE_AGENT iwpriv SetSa\n");

	endptr = prInBuf;

	for (i = 0; i < MAC_ADDR_LEN; i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, &addr[i]))
			return -EINVAL;
	}

	DBGLOG(RFTEST, ERROR,
		"SetATESa Sa:%02x:%02x:%02x:%02x:%02x:%02x\n",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	for (i = 0; i < MAC_ADDR_LEN; i++)
		addr2[i] = (uint8_t) addr[i];

	i4Status = MT_ATESetMACAddress(prNetDev,
		RF_AT_FUNCID_SET_TA, addr2);

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Channel Frequency.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATEChannel(struct net_device *prNetDev,
		  uint8_t *prInBuf)
{
	uint32_t i4SetFreq = 0;
	int32_t i4Status, i4SetChan = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetChannel\n");

	rv = kstrtoint(prInBuf, 0, &i4SetChan);
	if (rv == 0) {
		i4SetFreq = nicChannelNum2Freq(i4SetChan, BAND_NULL);
		i4Status = MT_ATESetChannel(prNetDev, 0, i4SetFreq);
	} else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX WF0 Power.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxPower0(struct net_device *prNetDev,
		   uint8_t *prInBuf)
{
	uint32_t i4SetTxPower0 = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetTxPower0\n");

	rv = kstrtoint(prInBuf, 0, &i4SetTxPower0);
	if (rv == 0)
		i4Status = MT_ATESetTxPower0(prNetDev, i4SetTxPower0);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX Guard Interval.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxGi(struct net_device *prNetDev,
	       uint8_t *prInBuf)
{
	uint32_t i4SetTxGi = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetTxGi\n");

	rv = kstrtoint(prInBuf, 0, &i4SetTxGi);
	if (rv == 0)
		i4Status = MT_ATESetTxGi(prNetDev, i4SetTxGi);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX System Bandwidth.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxBw(struct net_device *prNetDev,
	       uint8_t *prInBuf)
{
	uint32_t i4SetSystemBW = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetSystemBW\n");

	rv = kstrtoint(prInBuf, 0, &i4SetSystemBW);
	if (rv == 0)
		i4Status = MT_ATESetSystemBW(prNetDev, i4SetSystemBW);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX Mode (Preamble).
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxMode(struct net_device *prNetDev,
		 uint8_t *prInBuf)
{
	uint32_t i4SetTxMode = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetTxMode\n");

	rv = kstrtoint(prInBuf, 0, &i4SetTxMode);
	if (rv == 0)
		i4Status = MT_ATESetPreamble(prNetDev, i4SetTxMode);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX Length.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxLength(struct net_device *prNetDev,
		   uint8_t *prInBuf)
{
	uint32_t i4SetTxLength = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetTxLength\n");

	rv = kstrtoint(prInBuf, 0, &i4SetTxLength);
	if (rv == 0)
		i4Status = MT_ATESetTxLength(prNetDev, i4SetTxLength);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX Count.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxCount(struct net_device *prNetDev,
		  uint8_t *prInBuf)
{
	uint32_t i4SetTxCount = 0;
	int32_t i4Status;
	int32_t rv;
	uint8_t addr[MAC_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetTxCount\n");

	rv = kstrtoint(prInBuf, 0, &i4SetTxCount);
	if (rv == 0)
		MT_ATESetTxCount(prNetDev, i4SetTxCount);
	else
		return -EINVAL;

	i4Status = MT_ATESetMACAddress(prNetDev,
				       RF_AT_FUNCID_SET_MAC_ADDRESS, addr);

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set TX Rate.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxMcs(struct net_device *prNetDev,
		uint8_t *prInBuf)
{
	uint32_t i4SetTxMcs = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetTxMcs\n");

	rv = kstrtoint(prInBuf, 0, &i4SetTxMcs);
	if (rv == 0)
		i4Status = MT_ATESetRate(prNetDev, i4SetTxMcs);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Inter-Packet Guard Interval.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATEIpg(struct net_device *prNetDev, uint8_t *prInBuf)
{
	uint32_t i4SetTxIPG = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetIpg\n");

	rv = kstrtoint(prInBuf, 0, &i4SetTxIPG);
	if (rv == 0)
		i4Status = MT_ATESetTxIPG(prNetDev, i4SetTxIPG);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Inter-Packet Guard Interval.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxVhtNss(struct net_device *prNetDev, uint8_t *prInBuf)
{
	uint32_t i4SetTVhtNSS = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s\n", __func__);

	rv = kstrtoint(prInBuf, 0, &i4SetTVhtNSS);
	if (rv == 0)
		i4Status = MT_ATESetTxVhtNss(prNetDev, i4SetTVhtNSS);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Inter-Packet Guard Interval.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATETxPath(struct net_device *prNetDev, uint8_t *prInBuf)
{
	uint32_t i4TxPath = 0;
	int32_t i4Status;
	int32_t rv;


	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s\n", __func__);

	rv = kstrtoint(prInBuf, 0, &i4TxPath);
	if (rv == 0)
		i4Status = MT_ATESetTxPath(prNetDev, i4TxPath);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Inter-Packet Guard Interval.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATERxPath(struct net_device *prNetDev, uint8_t *prInBuf)
{
	uint32_t i4RxPath = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s\n", __func__);

	rv = kstrtoint(prInBuf, 0, &i4RxPath);
	if (rv == 0)
		i4Status = MT_ATESetRxPath(prNetDev, i4RxPath);
	else
		return -EINVAL;

	return i4Status;
}

#if CFG_SUPPORT_ANT_SWAP
/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Antenna Swap
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetATEAntSwp(struct net_device *prNetDev, uint8_t *prInBuf)
{
	uint32_t i4SetAntSwp = 0;
	int32_t i4Status;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv SetAntSwp\n");

	rv = kstrtoint(prInBuf, 0, &i4SetAntSwp);
	if (rv == 0) {
		DBGLOG(REQ, DEBUG, "i4SetAntSwp = %d\n", i4SetAntSwp);
		i4Status = MT_ATESetAntSwap(prNetDev, i4SetAntSwp);
	} else
		return -EINVAL;

	return i4Status;
}
#endif

#if CFG_SUPPORT_TX_BF
int Set_TxBfProfileTag_Help(struct net_device *prNetDev,
			    uint8_t *prInBuf)
{
	DBGLOG(RFTEST, ERROR,
	       "========================================================================================================================\n"
	       "TxBfProfile Tag1 setting example :\n"
	       "iwpriv ra0 set TxBfProfileTagPfmuIdx  =xx\n"
	       "iwpriv ra0 set TxBfProfileTagBfType   =xx (0: iBF; 1: eBF)\n"
	       "iwpriv ra0 set TxBfProfileTagBw       =xx (0/1/2/3 : BW20/40/80/160NC)\n"
	       "iwpriv ra0 set TxBfProfileTagSuMu     =xx (0:SU, 1:MU)\n"
	       "iwpriv ra0 set TxBfProfileTagInvalid  =xx (0: valid, 1: invalid)\n"
	       "iwpriv ra0 set TxBfProfileTagMemAlloc =xx:xx:xx:xx:xx:xx:xx:xx (mem_row, mem_col), ..\n"
	       "iwpriv ra0 set TxBfProfileTagMatrix   =nrow:nol:ng:LM\n"
	       "iwpriv ra0 set TxBfProfileTagSnr      =SNR_STS0:SNR_STS1:SNR_STS2:SNR_STS3\n"
	       "\n\n"
	       "TxBfProfile Tag2 setting example :\n"
	       "iwpriv ra0 set TxBfProfileTagSmtAnt   =xx (11:0)\n"
	       "iwpriv ra0 set TxBfProfileTagSeIdx    =xx\n"
	       "iwpriv ra0 set TxBfProfileTagRmsdThrd =xx\n"
	       "iwpriv ra0 set TxBfProfileTagMcsThrd  =xx:xx:xx:xx:xx:xx (MCS TH L1SS:S1SS:L2SS:....)\n"
	       "iwpriv ra0 set TxBfProfileTagTimeOut  =xx\n"
	       "iwpriv ra0 set TxBfProfileTagDesiredBw=xx (0/1/2/3 : BW20/40/80/160NC)\n"
	       "iwpriv ra0 set TxBfProfileTagDesiredNc=xx\n"
	       "iwpriv ra0 set TxBfProfileTagDesiredNr=xx\n"
	       "\n\n"
	       "Read TxBf profile Tag :\n"
	       "iwpriv ra0 set TxBfProfileTagRead     =xx (PFMU ID)\n"
	       "\n"
	       "Write TxBf profile Tag :\n"
	       "iwpriv ra0 set TxBfProfileTagWrite    =xx (PFMU ID)\n"
	       "When you use one of relative CMD to update one of tag parameters, you should call TxBfProfileTagWrite to update Tag\n"
	       "\n\n"
	       "Read TxBf profile Data	:\n"
	       "iwpriv ra0 set TxBfProfileDataRead    =xx (PFMU ID)\n"
	       "\n"
	       "Write TxBf profile Data :\n"
	       "iwpriv ra0 set TxBfProfileDataWrite   =BW :subcarrier:phi11:psi2l:Phi21:Psi31:Phi31:Psi41:Phi22:Psi32:Phi32:Psi42:Phi33:Psi43\n"
	       "iwpriv ra0 set TxBfProfileDataWriteAll=Profile ID : BW (BW       : 0x00 (20M) , 0x01 (40M), 0x02 (80M), 0x3 (160M)\n"
	       "When you use CMD TxBfProfileDataWrite to update profile data per subcarrier, you should call TxBfProfileDataWriteAll to update all of\n"
	       "subcarrier's profile data.\n\n"
	       "Read TxBf profile PN	:\n"
	       "iwpriv ra0 set TxBfProfilePnRead      =xx (PFMU ID)\n"
	       "\n"
	       "Write TxBf profile PN :\n"
	       "iwpriv ra0 set TxBfProfilePnWrite     =Profile ID:BW:1STS_Tx0:1STS_Tx1:1STS_Tx2:1STS_Tx3:2STS_Tx0:2STS_Tx1:2STS_Tx2:2STS_Tx3:3STS_Tx1:3STS_Tx2:3STS_Tx3\n"
	       "========================================================================================================================\n");
	return 0;
}

int Set_TxBfProfileTag_InValid(struct net_device *prNetDev,
			       uint8_t *prInBuf)
{
	uint32_t ucInValid;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_InValid\n");

	rv = kstrtoint(prInBuf, 0, &ucInValid);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_InValid prInBuf = %s, ucInValid = %d\n",
		       prInBuf,
		       ucInValid);
		i4Status = TxBfProfileTag_InValid(prNetDev, &g_rPfmuTag1,
						  ucInValid);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_PfmuIdx(struct net_device *prNetDev,
			       uint8_t *prInBuf)
{
	uint32_t ucProfileIdx;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_PfmuIdx\n");

	rv = kstrtoint(prInBuf, 0, &ucProfileIdx);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_PfmuIdx prInBuf = %s, ucProfileIdx = %d\n",
		       prInBuf,
		       ucProfileIdx);
		i4Status = TxBfProfileTag_PfmuIdx(prNetDev, &g_rPfmuTag1,
						  ucProfileIdx);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_BfType(struct net_device *prNetDev,
			      uint8_t *prInBuf)
{
	uint32_t ucBFType;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_BfType\n");

	rv = kstrtoint(prInBuf, 0, &ucBFType);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_BfType prInBuf = %s, ucBFType = %d\n",
		       prInBuf, ucBFType);
		i4Status = TxBfProfileTag_TxBfType(prNetDev, &g_rPfmuTag1,
						   ucBFType);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_DBW(struct net_device *prNetDev,
			   uint8_t *prInBuf)
{
	uint32_t ucBW;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_DBW\n");

	rv = kstrtoint(prInBuf, 0, &ucBW);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_DBW prInBuf = %s, ucBW = %d\n",
		       prInBuf, ucBW);
		i4Status = TxBfProfileTag_DBW(prNetDev, &g_rPfmuTag1, ucBW);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_SuMu(struct net_device *prNetDev,
			    uint8_t *prInBuf)
{
	uint32_t ucSuMu;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_SuMu\n");

	rv = kstrtoint(prInBuf, 0, &ucSuMu);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_SuMu prInBuf = %s, ucSuMu = %d\n",
		       prInBuf, ucSuMu);
		i4Status = TxBfProfileTag_SuMu(prNetDev, &g_rPfmuTag1,
					       ucSuMu);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_Mem(struct net_device *prNetDev,
			   uint8_t *prInBuf)
{
	uint32_t aucInput[8];
	int32_t i4Status = 0;
	uint8_t aucMemAddrColIdx[4], aucMemAddrRowIdx[4];
	int32_t i;
	uint8_t *endptr;
	uint8_t *token;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_Mem\n");

	endptr = prInBuf;

	for (i = 0; i < 8; i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, &aucInput[i]))
			return -EINVAL;
	}

	/* mem col0:row0:col1:row1:col2:row2:col3:row3 */
	aucMemAddrColIdx[0] = (uint8_t) aucInput[0];
	aucMemAddrRowIdx[0] = (uint8_t) aucInput[1];
	aucMemAddrColIdx[1] = (uint8_t) aucInput[2];
	aucMemAddrRowIdx[1] = (uint8_t) aucInput[3];
	aucMemAddrColIdx[2] = (uint8_t) aucInput[4];
	aucMemAddrRowIdx[2] = (uint8_t) aucInput[5];
	aucMemAddrColIdx[3] = (uint8_t) aucInput[6];
	aucMemAddrRowIdx[3] = (uint8_t) aucInput[7];

	i4Status = TxBfProfileTag_Mem(prNetDev, &g_rPfmuTag1,
		aucMemAddrColIdx, aucMemAddrRowIdx);

	return i4Status;
}

int Set_TxBfProfileTag_Matrix(struct net_device *prNetDev,
	uint8_t *prInBuf)
{
	uint32_t aucInput[6];
	uint8_t ucNrow, ucNcol, ucNgroup, ucLM, ucCodeBook,
		ucHtcExist;
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr;
	uint8_t *token;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_Matrix\n");

	endptr = prInBuf;

	for (i = 0; i < 6; i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, &aucInput[i]))
			return -EINVAL;
	}

	ucNrow = (uint8_t) aucInput[0];
	ucNcol = (uint8_t) aucInput[1];
	ucNgroup = (uint8_t) aucInput[2];
	ucLM = (uint8_t) aucInput[3];
	ucCodeBook = (uint8_t) aucInput[4];
	ucHtcExist = (uint8_t) aucInput[5];

	i4Status = TxBfProfileTag_Matrix(prNetDev, &g_rPfmuTag1, ucNrow,
		ucNcol, ucNgroup, ucLM, ucCodeBook, ucHtcExist);

	return i4Status;
}

int Set_TxBfProfileTag_SNR(struct net_device *prNetDev,
			   uint8_t *prInBuf)
{
	uint32_t aucInput[4];
	uint8_t ucSNR_STS0, ucSNR_STS1, ucSNR_STS2, ucSNR_STS3;
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr = prInBuf;
	uint8_t *token;
	uint32_t *params[] = {
		&aucInput[0], &aucInput[1], &aucInput[2], &aucInput[3]
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_SNR\n");

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	ucSNR_STS0 = (uint8_t) aucInput[0];
	ucSNR_STS1 = (uint8_t) aucInput[1];
	ucSNR_STS2 = (uint8_t) aucInput[2];
	ucSNR_STS3 = (uint8_t) aucInput[3];

	i4Status = TxBfProfileTag_SNR(prNetDev, &g_rPfmuTag1,
		ucSNR_STS0, ucSNR_STS1, ucSNR_STS2, ucSNR_STS3);

	return i4Status;
}

int Set_TxBfProfileTag_SmartAnt(struct net_device *prNetDev,
				uint8_t *prInBuf)
{
	int32_t i4Status = 0;
	uint32_t ucSmartAnt;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_SmartAnt\n");

	rv = kstrtoint(prInBuf, 0, &ucSmartAnt);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_SmartAnt prInBuf = %s, ucSmartAnt = %d\n",
		       prInBuf,
		       ucSmartAnt);
		i4Status = TxBfProfileTag_SmtAnt(prNetDev, &g_rPfmuTag2,
						 ucSmartAnt);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_SeIdx(struct net_device *prNetDev,
			     uint8_t *prInBuf)
{
	int32_t i4Status = 0;
	uint32_t ucSeIdx;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_SeIdx\n");

	rv = kstrtoint(prInBuf, 0, &ucSeIdx);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "TxBfProfileTag_SeIdx prInBuf = %s, ucSeIdx = %d\n",
		       prInBuf, ucSeIdx);
		i4Status = TxBfProfileTag_SeIdx(prNetDev, &g_rPfmuTag2,
						ucSeIdx);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_RmsdThrd(struct net_device *prNetDev,
				uint8_t *prInBuf)
{
	int32_t i4Status = 0;
	uint32_t ucRmsdThrd;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_RmsdThrd\n");

	rv = kstrtoint(prInBuf, 0, &ucRmsdThrd);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_RmsdThrd prInBuf = %s, ucRmsdThrd = %d\n",
		       prInBuf,
		       ucRmsdThrd);
		i4Status = TxBfProfileTag_RmsdThd(prNetDev, &g_rPfmuTag2,
						  ucRmsdThrd);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_McsThrd(struct net_device *prNetDev,
			       uint8_t *prInBuf)
{
	uint32_t aucInput[6];
	uint8_t ucMcsLss[3], ucMcsSss[3];
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr = prInBuf;
	uint8_t *token;

	uint32_t *params[] = {
		&aucInput[0], &aucInput[1], &aucInput[2], &aucInput[3],
		&aucInput[4], &aucInput[5]
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_McsThrd\n");

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	ucMcsLss[0] = (uint8_t) aucInput[0];
	ucMcsSss[0] = (uint8_t) aucInput[1];
	ucMcsLss[1] = (uint8_t) aucInput[2];
	ucMcsSss[1] = (uint8_t) aucInput[3];
	ucMcsLss[2] = (uint8_t) aucInput[4];
	ucMcsSss[2] = (uint8_t) aucInput[5];

	i4Status = TxBfProfileTag_McsThd(prNetDev, &g_rPfmuTag2,
		ucMcsLss, ucMcsSss);

	return i4Status;
}

int Set_TxBfProfileTag_TimeOut(struct net_device *prNetDev,
			       uint8_t *prInBuf)
{
	uint32_t ucTimeOut;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_TimeOut\n");

	rv = kstrtouint(prInBuf, 0, &ucTimeOut);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_TimeOut prInBuf = %s, ucTimeOut = %d\n",
		       prInBuf,
		       ucTimeOut);
		i4Status = TxBfProfileTag_TimeOut(prNetDev, &g_rPfmuTag2,
						  ucTimeOut);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_DesiredBW(struct net_device
				 *prNetDev, uint8_t *prInBuf)
{
	uint32_t ucDesiredBW;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_DesiredBW\n");

	rv = kstrtoint(prInBuf, 0, &ucDesiredBW);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_DesiredBW prInBuf = %s, ucDesiredBW = %d\n",
		       prInBuf,
		       ucDesiredBW);
		i4Status = TxBfProfileTag_DesiredBW(prNetDev, &g_rPfmuTag2,
						    ucDesiredBW);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_DesiredNc(struct net_device
				 *prNetDev, uint8_t *prInBuf)
{
	uint32_t ucDesiredNc;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_DesiredNc\n");

	rv = kstrtoint(prInBuf, 0, &ucDesiredNc);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_DesiredNc prInBuf = %s, ucDesiredNc = %d\n",
		       prInBuf,
		       ucDesiredNc);
		i4Status = TxBfProfileTag_DesiredNc(prNetDev, &g_rPfmuTag2,
						    ucDesiredNc);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTag_DesiredNr(struct net_device
				 *prNetDev, uint8_t *prInBuf)
{
	uint32_t ucDesiredNr;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTag_DesiredNr\n");

	rv = kstrtoint(prInBuf, 0, &ucDesiredNr);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTag_DesiredNr prInBuf = %s, ucDesiredNr = %d\n",
		       prInBuf,
		       ucDesiredNr);
		i4Status = TxBfProfileTag_DesiredNr(prNetDev, &g_rPfmuTag2,
						    ucDesiredNr);
	} else
		return -EINVAL;

	return i4Status;
}


int Set_TxBfProfileTagPartialBw(
	struct net_device *prNetDev,
	uint8_t *prInBuf
)
{
	uint32_t u4Bitmap, u4Resolution;
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t *params[] = {
		&u4Bitmap, &u4Resolution
	};

	TRACE_FUNC(RFTEST, DEBUG, "%s\n");

	if (prInBuf == NULL)
		return -EINVAL;

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	DBGLOG(RFTEST, DEBUG,
		"%d/%d\n",
		u4Bitmap, u4Bitmap);

	i4Status = TxBfProfileTagPartialBw(prNetDev, &g_rPfmuTag1,
		(uint8_t)u4Bitmap, (uint8_t)u4Resolution);

	return i4Status;
}

int Set_TxBfProfileTag_BandIdx(struct net_device *prNetDev,
			    uint8_t *prInBuf)
{
	uint32_t uBandIdx;
	int32_t i4Status = 0;
	int32_t rv;

	TRACE_FUNC(RFTEST, DEBUG, "%s\n");

	rv = kstrtoint(prInBuf, 0, &uBandIdx);
	if (rv == 0) {
		DBGLOG(RFTEST, DEBUG,
		       "Set_TxBfProfileTag_BandIdx prInBuf = %s, uBandIdx = %d\n",
		       prInBuf, uBandIdx);
		i4Status = TxBfProfileTag_BandIdx(prNetDev, uBandIdx);
	} else {
		return -EINVAL;
	}

	return i4Status;
}

int Set_TxBfProfileTagWrite(struct net_device *prNetDev,
			    uint8_t *prInBuf)
{
	uint32_t profileIdx;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTagWrite\n");

	rv = kstrtoint(prInBuf, 0, &profileIdx);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfileTagWrite prInBuf = %s, profileIdx = %d\n",
		       prInBuf,
		       profileIdx);
		i4Status = TxBfProfileTagWrite(prNetDev, &g_rPfmuTag1,
					       &g_rPfmuTag2, profileIdx);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfileTagRead(struct net_device *prNetDev,
	uint8_t *prInBuf)
{
	uint32_t profileIdx, fgBFer;
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t *params[] = {&profileIdx, &fgBFer};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileTagRead\n");

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	i4Status = TxBfProfileTagRead(prNetDev, profileIdx, fgBFer);

	return i4Status;
}

int Set_TxBfProfileDataRead(struct net_device *prNetDev,
			    uint8_t *prInBuf)
{
	uint32_t profileIdx, fgBFer, subcarrierIdxMsb,
		 subcarrierIdxLsb;
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr = prInBuf;
	uint8_t *token;
	uint32_t *params[] = {
		&profileIdx, &fgBFer, &subcarrierIdxMsb, &subcarrierIdxLsb
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileDataRead\n");

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	i4Status = TxBfProfileDataRead(prNetDev, profileIdx, fgBFer,
		subcarrierIdxMsb, subcarrierIdxLsb);

	return i4Status;
}

int Set_TxBfProfileDataWrite(struct net_device *prNetDev,
			     uint8_t *prInBuf)
{

	uint32_t u4ProfileIdx;
	uint32_t u4SubcarrierIdx;
	uint32_t au4Phi[6];
	uint32_t au4Psi[6];
	uint32_t au4DSnr[4];
	uint16_t au2Phi[6];
	uint8_t aucPsi[6];
	uint8_t aucDSnr[4];
	uint32_t i;
	uint8_t *endptr = prInBuf;
	uint8_t *token;
	uint32_t *params[] = {
		&u4ProfileIdx, &u4SubcarrierIdx};
	uint32_t *pu4PhiParams[] = {
		&au4Phi[0], &au4Psi[0], &au4Phi[1], &au4Psi[1],
		&au4Phi[2], &au4Psi[2], &au4Phi[3], &au4Psi[3],
		&au4Phi[4], &au4Psi[4], &au4Phi[5], &au4Psi[5]};
	uint32_t *pu4DsnrParams[] = {
		&au4DSnr[0], &au4DSnr[1], &au4DSnr[2], &au4DSnr[3]};
	int32_t i4Status = 0;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "TxBfProfileDataWrite\n");

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(pu4PhiParams); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, pu4PhiParams[i]))
			return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(pu4DsnrParams); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, pu4DsnrParams[i]))
			return -EINVAL;
	}

	for (i = 0; i < 6; i++) {
		au2Phi[i] = (uint16_t)au4Phi[i];
		aucPsi[i] = (uint8_t)au4Psi[i];
	}
	for (i = 0; i < 4; i++)
		aucDSnr[i] = (uint8_t)au4DSnr[i];

	i4Status = TxBfProfileDataWrite(prNetDev, u4ProfileIdx,
		u4SubcarrierIdx, au2Phi, aucPsi, aucDSnr);

	return i4Status;
}

int Set_TxBfProfilePnRead(struct net_device *prNetDev,
			  uint8_t *prInBuf)
{
	uint32_t profileIdx;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfilePnRead\n");

	rv = kstrtoint(prInBuf, 0, &profileIdx);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_TxBfProfilePnRead prInBuf = %s, profileIdx = %d\n",
		       prInBuf, profileIdx);
		i4Status = TxBfProfilePnRead(prNetDev, profileIdx);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_TxBfProfilePnWrite(struct net_device *prNetDev,
			   uint8_t *prInBuf)
{
	uint32_t u4ProfileIdx;
	uint16_t u2bw;
	uint16_t au2XSTS[12];
	uint8_t *endptr = prInBuf;
	uint8_t *token;
	void *params[] = {
		&u4ProfileIdx, &u2bw, &au2XSTS[0], &au2XSTS[1],
		&au2XSTS[2], &au2XSTS[3], &au2XSTS[4], &au2XSTS[5],
		&au2XSTS[6], &au2XSTS[7], &au2XSTS[8],
		&au2XSTS[9], &au2XSTS[10], &au2XSTS[11]
	};
	int32_t i4Status = 0, i = 0;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "TxBfProfilePnWrite\n");

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (i == 0) {
			if (!token || kalkStrtou32(token, 16,
				(uint32_t *)params[i]))
				return -EINVAL;
		} else {
			if (!token || kalkStrtou16(token, 16,
				(uint16_t *)params[i]))
				return -EINVAL;
		}
	}

	i4Status = TxBfProfilePnWrite(prNetDev, u4ProfileIdx, u2bw,
		au2XSTS);

	return i4Status;
}

/* Su_Mu:NumSta:SndInterval:WLan0:WLan1:WLan2:WLan3 */
int Set_Trigger_Sounding_Proc(struct net_device *prNetDev,
	uint8_t *prInBuf)
{
	uint32_t ucSuMu, ucNumSta, ucSndInterval, ucWLan0, ucWLan1,
		 ucWLan2, ucWLan3;
	uint32_t *params[] = {
		&ucSuMu,
		&ucNumSta,
		&ucSndInterval,
		&ucWLan0,
		&ucWLan1,
		&ucWLan2,
		&ucWLan3};
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr;
	uint8_t *token;
	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_Trigger_Sounding_Proc\n");

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	DBGLOG(RFTEST, ERROR,
		"%d/%d/%d/%d/%d/%d/%d\n",
		ucSuMu, ucNumSta, ucSndInterval, ucWLan0,
		ucWLan1, ucWLan2, ucWLan3);

	i4Status = TxBfSounding(prNetDev, ucSuMu, ucNumSta,
		(ucSndInterval << 2), ucWLan0, ucWLan1, ucWLan2, ucWLan3);

	return i4Status;
}

int Set_Stop_Sounding_Proc(struct net_device *prNetDev,
			   uint8_t *prInBuf)
{
	int32_t i4Status = 0;

	DBGLOG(RFTEST, ERROR, "Set_Stop_Sounding_Proc\n");

	i4Status = TxBfSoundingStop(prNetDev);

	return i4Status;
}

int Set_TxBfTxApply(
	struct net_device *prNetDev,
	uint8_t *prInBuf
)
{
	uint32_t u4WlanId, u4ETxBf, u4ITxBf, u4MuTxBf, u4PhaseCali = 0;
	uint32_t *params[] = {
		&u4WlanId,
		&u4ETxBf,
		&u4ITxBf,
		&u4MuTxBf,
		&u4PhaseCali
	};
	uint8_t *endptr;
	int32_t i4Status = 0, i = 0;
	uint8_t *token;
	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "TxBfTxApply\n");

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	DBGLOG(RFTEST, ERROR,
		"%d/%d/%d/%d\n",
		u4WlanId, u4ETxBf, u4ITxBf, u4MuTxBf);

	i4Status = TxBfTxApply(prNetDev, u4WlanId, u4ETxBf, u4ITxBf,
		u4MuTxBf, u4PhaseCali);

	return i4Status;
}


int Set_TxBfManualAssoc(
	struct net_device *prNetDev,
	uint8_t *prInBuf
)
{
	int8_t aucMac[MAC_ADDR_LEN];
	uint32_t u4Type, u4Wtbl, u4Ownmac, u4Bw, u4Nss,
		u4PfmuId, u4Mode, u4Marate, u4SpeIdx, ucaid;
	int32_t i4Status = 0, i = 0;
	uint32_t *params[] = {
		&u4Type, &u4Wtbl, &u4Ownmac, &u4Bw,
		&u4Nss, &u4PfmuId, &u4Mode, &u4Marate, &u4SpeIdx,
		&ucaid
	};
	uint8_t *endptr;
	uint8_t *token;
	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "TxBfManualAssoc\n");

	endptr = prInBuf;

	for (i = 0; i < MAC_ADDR_LEN; i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, (uint32_t *)&aucMac[i]))
			return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	i4Status =
		TxBfManualAssoc(prNetDev, aucMac, u4Type,
			u4Wtbl,	u4Ownmac, u4Mode, u4Bw, u4Nss,
			u4PfmuId, u4Marate, u4SpeIdx, ucaid);

	return i4Status;
}

int Set_TxBfPfmuMemAlloc(struct net_device *prNetDev,
			 uint8_t *prInBuf)
{
	uint32_t u4SuMuMode, u4WlanIdx;
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t *params[] = {
		&u4SuMuMode, &u4WlanIdx
	};
	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "TxBfPfmuMemAlloc\n");

	endptr = prInBuf;
	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	DBGLOG(RFTEST, ERROR,
		"TxBfPfmuMemAlloc u4SuMuMode = %d, u4WlanIdx = %d",
		u4SuMuMode, u4WlanIdx);
	i4Status = TxBfPfmuMemAlloc(prNetDev, u4SuMuMode,
		u4WlanIdx);

	return i4Status;
}

int Set_TxBfPfmuMemRelease(struct net_device *prNetDev,
			   uint8_t *prInBuf)
{
	uint32_t ucWlanId;
	int32_t i4Status = 0;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "TxBfPfmuMemRelease\n");

	rv = kstrtoint(prInBuf, 0, &ucWlanId);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR, "TxBfPfmuMemRelease ucWlanId = %d",
		       ucWlanId);
		i4Status = TxBfPfmuMemRelease(prNetDev, ucWlanId);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_DevInfoUpdate(struct net_device *prNetDev,
	uint8_t *prInBuf)
{
	uint32_t u4OwnMacIdx, fgBand;
	uint32_t OwnMacAddr[MAC_ADDR_LEN];
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	int32_t i4Status = 0;
	uint32_t i;
	uint8_t *endptr;
	uint8_t *token;
	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "DevInfoUpdate\n");

	endptr = prInBuf;

	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &u4OwnMacIdx))
		return -EINVAL;

	for (i = 0; i < MAC_ADDR_LEN; i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, &OwnMacAddr[i]))
			return -EINVAL;
	}

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &fgBand))
		return -EINVAL;

	for (i = 0; i < MAC_ADDR_LEN; i++)
		aucMacAddr[i] = (uint8_t)OwnMacAddr[i];

	g_rBandIdx = fgBand;

	i4Status = DevInfoUpdate(prNetDev, u4OwnMacIdx, fgBand, aucMacAddr);

	return i4Status;
}


int Set_BssInfoUpdate(struct net_device *prNetDev,
		      uint8_t *prInBuf)
{
	uint32_t u4OwnMacIdx, u4BssIdx;
	uint32_t au4BssId[MAC_ADDR_LEN];
	uint8_t aucBssId[MAC_ADDR_LEN];
	int32_t i4Status = 0;
	uint32_t i;
	uint32_t *params[] = {
		&u4OwnMacIdx, &u4BssIdx, &au4BssId[0], &au4BssId[1],
		&au4BssId[2], &au4BssId[3], &au4BssId[4], &au4BssId[5]
	};
	uint8_t *endptr = prInBuf;
	uint8_t *token;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "BssInfoUpdate\n");

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (i < 2) {
			if (!token || kalkStrtou32(token, 10, params[i]))
				return -EINVAL;
		} else {
			if (!token || kalkStrtou32(token, 16, params[i]))
				return -EINVAL;
		}
	}

	for (i = 0; i < MAC_ADDR_LEN; i++)
		aucBssId[i] = au4BssId[i];

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	i4Status = BssInfoUpdateUnify(prNetDev, u4OwnMacIdx, u4BssIdx,
		g_rBandIdx, aucBssId);
#else
	i4Status = BssInfoConnectOwnDev(prNetDev, u4OwnMacIdx, u4BssIdx,
		g_rBandIdx);

	i4Status = BssInfoUpdate(prNetDev, u4OwnMacIdx, u4BssIdx,
		aucBssId);
#endif

	return i4Status;
}

int Set_StaRecCmmUpdate(struct net_device *prNetDev,
			uint8_t *prInBuf)
{
	uint32_t u4WlanId, u4BssId, u4Aid;
	uint32_t au4MacAddr[MAC_ADDR_LEN];
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	int32_t i4Status = 0;
	uint32_t i;
	uint8_t *endptr = prInBuf;
	uint8_t *token;
	uint32_t *params[] = {
		&u4WlanId, &u4BssId, &u4Aid,
		&au4MacAddr[0], &au4MacAddr[1], &au4MacAddr[2],
		&au4MacAddr[3], &au4MacAddr[4], &au4MacAddr[5]
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_StaRecCmmUpdate\n");

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	for (i = 0; i < MAC_ADDR_LEN; i++)
		aucMacAddr[i] = (uint8_t) au4MacAddr[i];

	i4Status = StaRecCmmUpdate(prNetDev, u4WlanId, u4BssId,
		u4Aid, aucMacAddr);

	return i4Status;
}

int Set_StaRecBfUpdate(struct net_device *prNetDev,
		       uint8_t *prInBuf)
{
	struct STA_REC_BF_UPD_ARGUMENT rStaRecBfUpdArg;
	uint8_t aucMemRow[4], aucMemCol[4];
	int32_t i4Status = 0;
	uint32_t i;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t *params[] = {
		&rStaRecBfUpdArg.u4WlanId,
		&rStaRecBfUpdArg.u4BssId,
		&rStaRecBfUpdArg.u4PfmuId,
		&rStaRecBfUpdArg.u4SuMu,
		&rStaRecBfUpdArg.u4eTxBfCap,
		&rStaRecBfUpdArg.u4NdpaRate,
		&rStaRecBfUpdArg.u4NdpRate,
		&rStaRecBfUpdArg.u4ReptPollRate,
		&rStaRecBfUpdArg.u4TxMode,
		&rStaRecBfUpdArg.u4Nc,
		&rStaRecBfUpdArg.u4Nr,
		&rStaRecBfUpdArg.u4Bw,
		&rStaRecBfUpdArg.u4SpeIdx,
		&rStaRecBfUpdArg.u4TotalMemReq,
		&rStaRecBfUpdArg.u4MemReq20M,
		&rStaRecBfUpdArg.au4MemRow[0],
		&rStaRecBfUpdArg.au4MemCol[0],
		&rStaRecBfUpdArg.au4MemRow[1],
		&rStaRecBfUpdArg.au4MemCol[1],
		&rStaRecBfUpdArg.au4MemRow[2],
		&rStaRecBfUpdArg.au4MemCol[2],
		&rStaRecBfUpdArg.au4MemRow[3],
		&rStaRecBfUpdArg.au4MemCol[3]
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_StaRecBfUpdate\n");

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	for (i = 0; i < 4; i++) {
		aucMemRow[i] = rStaRecBfUpdArg.au4MemRow[i];
		aucMemCol[i] = rStaRecBfUpdArg.au4MemCol[i];
	}

	/* Default setting */
	rStaRecBfUpdArg.u4SmartAnt     = 0;
	/* 0: legacy, 1: OFDM, 2: HT, 4: VHT */
	rStaRecBfUpdArg.u4SoundingPhy  = 1;
	rStaRecBfUpdArg.u4iBfTimeOut   = 0xFF;
	rStaRecBfUpdArg.u4iBfDBW       = 0;
	rStaRecBfUpdArg.u4iBfNcol      = 0;
	rStaRecBfUpdArg.u4iBfNrow      = 0;
	rStaRecBfUpdArg.u4RuStartIdx   = 0;
	rStaRecBfUpdArg.u4RuEndIdx     = 0;
	rStaRecBfUpdArg.u4TriggerSu    = 0;
	rStaRecBfUpdArg.u4TriggerMu    = 0;
	rStaRecBfUpdArg.u4Ng16Su       = 0;
	rStaRecBfUpdArg.u4Ng16Mu       = 0;
	rStaRecBfUpdArg.u4Codebook42Su = 0;
	rStaRecBfUpdArg.u4Codebook75Mu = 0;
	rStaRecBfUpdArg.u4HeLtf        = 0;
	rStaRecBfUpdArg.u4NrBw160      = 0;
	rStaRecBfUpdArg.u4NcBw160      = 0;

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_SU_MU))
		rStaRecBfUpdArg.u4SuMu = g_rPfmuHeInfo.fgSU_MU;

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_RU_RANGE)) {
		rStaRecBfUpdArg.u4RuStartIdx =
				g_rPfmuHeInfo.u1RuStartIdx;
		rStaRecBfUpdArg.u4RuEndIdx = g_rPfmuHeInfo.u1RuEndIdx;
	}

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_TRIGGER)) {
		rStaRecBfUpdArg.u4TriggerSu = g_rPfmuHeInfo.fgTriggerSu;
		rStaRecBfUpdArg.u4TriggerMu = g_rPfmuHeInfo.fgTriggerMu;
	}

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_NG16)) {
		rStaRecBfUpdArg.u4Ng16Su = g_rPfmuHeInfo.fgNg16Su;
		rStaRecBfUpdArg.u4Ng16Mu = g_rPfmuHeInfo.fgNg16Mu;
	}

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_CODEBOOK)) {
		rStaRecBfUpdArg.u4Codebook42Su =
				g_rPfmuHeInfo.fgCodebook42Su;
		rStaRecBfUpdArg.u4Codebook75Mu =
				g_rPfmuHeInfo.fgCodebook75Mu;
	}

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_LTF))
		rStaRecBfUpdArg.u4HeLtf = g_rPfmuHeInfo.u1HeLtf;

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_IBF)) {
		rStaRecBfUpdArg.u4iBfNcol = g_rPfmuHeInfo.uciBfNcol;
		rStaRecBfUpdArg.u4iBfNrow = g_rPfmuHeInfo.uciBfNrow;
	}

	if (g_rPfmuHeInfo.u4Config & BIT(MANUAL_HE_BW160)) {
		rStaRecBfUpdArg.u4NrBw160 = g_rPfmuHeInfo.ucNrBw160;
		rStaRecBfUpdArg.u4NcBw160 = g_rPfmuHeInfo.ucNcBw160;
	}

	i4Status = StaRecBfUpdate(prNetDev, &rStaRecBfUpdArg,
		aucMemRow, aucMemCol);

	return i4Status;
}

int Set_StaRecBfRead(struct net_device *prNetDev,
		       uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint16_t u2WlanId;
	uint32_t u4Temp;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;
	uint8_t *endptr;

	TRACE_FUNC(RFTEST, DEBUG, "%s\n");
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	if (prInBuf == NULL || prGlueInfo == NULL)
		return -EINVAL;

	endptr = prInBuf;

	if (kalkStrtou32(endptr, 16, &u4Temp))
		return -EINVAL;
	u2WlanId = (uint16_t)u4Temp;

	i4Status = kalIoctl(prGlueInfo,
		wlanoidStaRecBFRead,
		&u2WlanId,
		sizeof(u2WlanId),
		&u4BufLen);

	return i4Status;
}

int Set_StaRecBfHeUpdate(struct net_device *prNetDev,
		       uint8_t *prInBuf)
{
	uint32_t au4Input[15];
	uint32_t u4Config;
	uint8_t ucSuMu, ucRuStartIdx, ucRuEndIdx, ucTriggerSu, ucTriggerMu,
		ucNg16Su, ucNg16Mu, ucCodebook42Su, ucCodebook75Mu, ucHeLtf,
		uciBfNcol, uciBfNrow, ucNrBw160, ucNcBw160;
	int32_t i4Status = 0, i = 0;
	uint8_t *endptr;
	uint32_t *params[] = {
		&au4Input[0], &au4Input[1], &au4Input[2], &au4Input[3],
		&au4Input[4], &au4Input[5], &au4Input[6], &au4Input[7],
		&au4Input[8], &au4Input[9], &au4Input[10],
		&au4Input[11], &au4Input[12], &au4Input[13], &au4Input[14]
	};
	uint8_t *token;

	DBGLOG(RFTEST, ERROR, "Set_StaRecBfHeUpdate\n");

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	u4Config = au4Input[0];
	ucSuMu = (uint8_t) au4Input[1];
	ucRuStartIdx = (uint8_t) au4Input[2];
	ucRuEndIdx = (uint8_t) au4Input[3];
	ucTriggerSu = (uint8_t) au4Input[4];
	ucTriggerMu = (uint8_t) au4Input[5];
	ucNg16Su = (uint8_t) au4Input[6];
	ucNg16Mu = (uint8_t) au4Input[7];
	ucCodebook42Su = (uint8_t) au4Input[8];
	ucCodebook75Mu = (uint8_t) au4Input[9];
	ucHeLtf = (uint8_t) au4Input[10];
	uciBfNcol = (uint8_t) au4Input[11];
	uciBfNrow = (uint8_t) au4Input[12];
	ucNrBw160 = (uint8_t) au4Input[13];
	ucNcBw160 = (uint8_t) au4Input[14];

	i4Status = StaRecBfHeUpdate(prNetDev, &g_rPfmuHeInfo, u4Config,
		ucSuMu,	ucRuStartIdx, ucRuEndIdx, ucTriggerSu,
		ucTriggerMu, ucNg16Su, ucNg16Mu,
		ucCodebook42Su, ucCodebook75Mu,
		ucHeLtf, uciBfNcol, uciBfNrow,
		ucNrBw160, ucNcBw160);

	return i4Status;
}


#if CFG_SUPPORT_MU_MIMO
int Set_MUGetInitMCS(struct net_device *prNetDev,
		     uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;

	uint32_t u4groupIdx;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUGetInitMCS\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	rv = kstrtouint(prInBuf, 0, &u4groupIdx);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR, "Test\n");
		DBGLOG(RFTEST, ERROR,
		       "Set_MUGetInitMCS prInBuf = %s, u4groupIdx = %x",
		       prInBuf, u4groupIdx);

		rMuMimoActionInfo.ucMuMimoCategory = MU_GET_CALC_INIT_MCS;
		rMuMimoActionInfo.unMuMimoParam.rMuGetCalcInitMcs.ucgroupIdx
			= u4groupIdx;

		i4Status = kalIoctl(prGlueInfo,
				    wlanoidMuMimoAction,
				    &rMuMimoActionInfo,
				    sizeof(rMuMimoActionInfo),
				    &u4BufLen);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_MUCalInitMCS(struct net_device *prNetDev,
		     uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0, i = 0;
	uint32_t u4BufLen = 0;
	uint8_t *token;
	uint32_t u4NumOfUser, u4Bandwidth, u4NssOfUser0,
		 u4NssOfUser1, u4PfMuIdOfUser0, u4PfMuIdOfUser1, u4NumOfTxer,
		 u4SpeIndex, u4GroupIndex;
	uint32_t *params[] = {
		&u4NumOfUser, &u4Bandwidth, &u4NssOfUser0, &u4NssOfUser1,
		&u4PfMuIdOfUser0, &u4PfMuIdOfUser1, &u4NumOfTxer,
		&u4SpeIndex, &u4GroupIndex
	};
	uint8_t *endptr = prInBuf;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUCalInitMCS\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}


	rMuMimoActionInfo.ucMuMimoCategory = MU_SET_CALC_INIT_MCS;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucNumOfUser =
		u4NumOfUser;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucBandwidth =
		u4Bandwidth;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucNssOfUser0 =
		u4NssOfUser0;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucNssOfUser1 =
		u4NssOfUser1;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucPfMuIdOfUser0
		= u4PfMuIdOfUser0;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucPfMuIdOfUser1
		= u4PfMuIdOfUser1;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucNumOfTxer =
		u4NumOfTxer;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.ucSpeIndex =
		u4SpeIndex;
	rMuMimoActionInfo.unMuMimoParam.rMuSetInitMcs.u4GroupIndex =
		u4GroupIndex;

	i4Status = kalIoctl(prGlueInfo,
		wlanoidMuMimoAction,
		&rMuMimoActionInfo,
		sizeof(rMuMimoActionInfo),
		&u4BufLen);

	return i4Status;
}

int Set_MUCalLQ(struct net_device *prNetDev,
		uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0, i = 0;
	uint32_t u4BufLen = 0;
	uint32_t u4NumOfUser, u4Bandwidth, u4NssOfUser0,
		 u4NssOfUser1, u4PfMuIdOfUser0, u4PfMuIdOfUser1,
		 u4NumOfTxer, u4SpeIndex, u4GroupIndex;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t *params[] = {
		&u4NumOfUser, &u4Bandwidth, &u4NssOfUser0,
		&u4NssOfUser1, &u4PfMuIdOfUser0, &u4PfMuIdOfUser1,
		&u4NumOfTxer, &u4SpeIndex, &u4GroupIndex};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUCalLQ\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_CALC_LQ;
	/* rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucType =
	 *							u4Type;
	 */
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucNumOfUser =
		u4NumOfUser;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucBandwidth =
		u4Bandwidth;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucNssOfUser0 =
		u4NssOfUser0;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucNssOfUser1 =
		u4NssOfUser1;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucPfMuIdOfUser0
		= u4PfMuIdOfUser0;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucPfMuIdOfUser1
		= u4PfMuIdOfUser1;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucNumOfTxer =
		u4NumOfTxer;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.ucSpeIndex =
		u4SpeIndex;
	rMuMimoActionInfo.unMuMimoParam.rMuSetCalcLq.u4GroupIndex =
		u4GroupIndex;

	i4Status = kalIoctl(prGlueInfo,
		wlanoidMuMimoAction,
		&rMuMimoActionInfo,
		sizeof(rMuMimoActionInfo),
		&u4BufLen);

	return i4Status;
}

int Set_MUGetLQ(struct net_device *prNetDev,
		uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	/* UINT_32      u4Type; */
	uint32_t u4BufLen = 0;

	DBGLOG(RFTEST, ERROR, "Set_MUGetLQ\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	/* if (sscanf(prInBuf, "%x", &u4Type) == 1)
	 * {
	 * DBGLOG(RFTEST, ERROR, "Set_MUGetLQ prInBuf = %s, u4Type = %x",
	 *        prInBuf, u4Type);
	 */

	rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_GET_CALC_LQ;
	/* rMuMimoActionInfo.unMuMimoParam.rMuGetLq.ucType = u4Type; */

	i4Status = kalIoctl(prGlueInfo,
			    wlanoidMuMimoAction,
			    &rMuMimoActionInfo,
			    sizeof(rMuMimoActionInfo),
			    &u4BufLen);
	/* }
	 * else
	 * {
	 * return -EINVAL;
	 * }
	 */

	return i4Status;
}

int Set_MUSetSNROffset(struct net_device *prNetDev,
		       uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;

	uint32_t u4Val;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUSetSNROffset\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	rv = kstrtoint(prInBuf, 0, &u4Val);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_MUSetSNROffset prInBuf = %s, u4Val = %x", prInBuf,
		       u4Val);

		rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_SNR_OFFSET;
		rMuMimoActionInfo.unMuMimoParam.rMuSetSnrOffset.ucVal =
			u4Val;

		i4Status = kalIoctl(prGlueInfo,
				    wlanoidMuMimoAction,
				    &rMuMimoActionInfo,
				    sizeof(rMuMimoActionInfo),
				    &u4BufLen);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_MUSetZeroNss(struct net_device *prNetDev,
		     uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;

	uint32_t u4Val;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUSetZeroNss\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	rv = kstrtouint(prInBuf, 0, &u4Val);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_MUSetZeroNss prInBuf = %s, u4Val = %x", prInBuf,
		       u4Val);

		rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_ZERO_NSS;
		rMuMimoActionInfo.unMuMimoParam.rMuSetZeroNss.ucVal = u4Val;

		i4Status = kalIoctl(prGlueInfo,
				    wlanoidMuMimoAction,
				    &rMuMimoActionInfo,
				    sizeof(rMuMimoActionInfo),
				    &u4BufLen);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_MUSetSpeedUpLQ(struct net_device *prNetDev,
		       uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;

	uint32_t u4Val;
	int32_t rv;
	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUSetSpeedUpLQ\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	rv = kstrtouint(prInBuf, 0, &u4Val);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_MUSetSpeedUpLQ prInBuf = %s, u4Val = %x", prInBuf,
		       u4Val);

		rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_SPEED_UP_LQ;
		rMuMimoActionInfo.unMuMimoParam.rMuSpeedUpLq.u4Val = u4Val;

		i4Status = kalIoctl(prGlueInfo,
				    wlanoidMuMimoAction,
				    &rMuMimoActionInfo,
				    sizeof(rMuMimoActionInfo),
				    &u4BufLen);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_MUSetMUTable(struct net_device *prNetDev,
		     uint8_t *prTable)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;
	/*uint32_t i;
	 *uint32_t u4Type, u4Length;
	 */

	if (prTable == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUSetMUTable\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	/* if (sscanf(prInBuf, "%x:%x", &u4Type, &u4Length) == 2) */
	/* { */
	/* DBGLOG(RFTEST, ERROR, "Set_MUSetMUTable prInBuf = %s, */
	/* u4Type = %x, u4Length = %x", prInBuf, u4Type, u4Length); */

	rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_MU_TABLE;
	/* rMuMimoActionInfo.unMuMimoParam.rMuSetMuTable.u2Type = u4Type; */
	/* rMuMimoActionInfo.unMuMimoParam.rMuSetMuTable.u4Length = u4Length; */

	/* for ( i = 0 ; i < NUM_MUT_NR_NUM * NUM_MUT_FEC * NUM_MUT_MCS
	 *      * NUM_MUT_INDEX ; i++)
	 * {
	 */
	memcpy(rMuMimoActionInfo.unMuMimoParam.rMuSetMuTable.aucMetricTable,
	       prTable,
	       NUM_MUT_NR_NUM * NUM_MUT_FEC * NUM_MUT_MCS * NUM_MUT_INDEX);
	/* } */

	i4Status = kalIoctl(prGlueInfo,
			    wlanoidMuMimoAction,
			    &rMuMimoActionInfo,
			    sizeof(rMuMimoActionInfo),
			    &u4BufLen);
	/* } */
	/* else */
	/* { */
	/* return -EINVAL; */
	/* } */

	return i4Status;
}

int Set_MUSetGroup(struct net_device *prNetDev,
		   uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	struct MU_SET_GROUP *prMuSetGroup;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;
	uint32_t i = 0;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t aucUser0MacAddr[PARAM_MAC_ADDR_LEN],
		 aucUser1MacAddr[PARAM_MAC_ADDR_LEN];

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUSetGroup\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	prMuSetGroup = &rMuMimoActionInfo.unMuMimoParam.rMuSetGroup;

	endptr = prInBuf;

	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4GroupIndex))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4NumOfUser))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User0Ldpc))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User1Ldpc))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4ShortGI))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4Bw))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User0Nss))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User1Nss))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4GroupId))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User0UP))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User1UP))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User0MuPfId))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User1MuPfId))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User0InitMCS))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &prMuSetGroup->u4User1InitMCS))
		return -EINVAL;

	for (i = 0; i < PARAM_MAC_ADDR_LEN; i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, &aucUser0MacAddr[i]))
			return -EINVAL;
	}

	for (i = 0; i < PARAM_MAC_ADDR_LEN; i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, &aucUser1MacAddr[i]))
			return -EINVAL;
	}

	rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_GROUP;
	for (i = 0; i < PARAM_MAC_ADDR_LEN; i++) {
		rMuMimoActionInfo.unMuMimoParam.rMuSetGroup.aucUser0MacAddr[i]
							= aucUser0MacAddr[i];
		rMuMimoActionInfo.unMuMimoParam.rMuSetGroup.aucUser1MacAddr[i]
							= aucUser1MacAddr[i];
	}

	i4Status = kalIoctl(prGlueInfo, wlanoidMuMimoAction, &rMuMimoActionInfo,
			    sizeof(rMuMimoActionInfo), &u4BufLen);

	return i4Status;
}

int Set_MUGetQD(struct net_device *prNetDev,
	uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;
	uint8_t *token;
	uint32_t u4SubcarrierIndex, u4Length;
	uint8_t *endptr;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUGetQD\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	endptr = prInBuf;

	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &u4SubcarrierIndex))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &u4Length))
		return -EINVAL;

	DBGLOG(RFTEST, ERROR,
		"%s/%x/%x",
		prInBuf, u4SubcarrierIndex, u4Length);

	rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_GET_QD;
	rMuMimoActionInfo.unMuMimoParam.rMuGetQd.ucSubcarrierIndex =
		u4SubcarrierIndex;
	/*
	 * rMuMimoActionInfo.unMuMimoParam.rMuGetQd.u4Length =
	 *						u4Length;
	 */
	/*
	 * rMuMimoActionInfo.unMuMimoParam.rMuGetQd.ucGroupIdx =
	 *						ucGroupIdx;
	 */

	i4Status = kalIoctl(prGlueInfo,
		wlanoidMuMimoAction,
		&rMuMimoActionInfo,
		sizeof(rMuMimoActionInfo),
		&u4BufLen);

	return i4Status;
}

int Set_MUSetEnable(struct net_device *prNetDev,
		    uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;

	uint32_t u4Val;
	int32_t rv;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUSetEnable\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	rv = kstrtouint(prInBuf, 0, &u4Val);
	if (rv == 0) {
		DBGLOG(RFTEST, ERROR,
		       "Set_MUSetEnable prInBuf = %s, u4Val = %x",
		       prInBuf, u4Val);

		rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_ENABLE;
		rMuMimoActionInfo.unMuMimoParam.rMuSetEnable.ucVal = u4Val;

		i4Status = kalIoctl(prGlueInfo,
				    wlanoidMuMimoAction,
				    &rMuMimoActionInfo,
				    sizeof(rMuMimoActionInfo),
				    &u4BufLen);
	} else
		return -EINVAL;

	return i4Status;
}

int Set_MUSetGID_UP(struct net_device *prNetDev,
		    uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0, i = 0;
	uint32_t u4BufLen = 0;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t *params[] = {
		&rMuMimoActionInfo.unMuMimoParam.rMuSetGidUp.au4Gid[0],
		&rMuMimoActionInfo.unMuMimoParam.rMuSetGidUp.au4Gid[1],
		&rMuMimoActionInfo.unMuMimoParam.rMuSetGidUp.au4Up[0],
		&rMuMimoActionInfo.unMuMimoParam.rMuSetGidUp.au4Up[1],
		&rMuMimoActionInfo.unMuMimoParam.rMuSetGidUp.au4Up[2],
		&rMuMimoActionInfo.unMuMimoParam.rMuSetGidUp.au4Up[3]
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUSetGID_UP\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	rMuMimoActionInfo.ucMuMimoCategory = MU_HQA_SET_STA_PARAM;

	i4Status = kalIoctl(prGlueInfo,
		wlanoidMuMimoAction,
		&rMuMimoActionInfo,
		sizeof(rMuMimoActionInfo),
		&u4BufLen);

	return i4Status;
}

int Set_MUTriggerTx(struct net_device *prNetDev,
		    uint8_t *prInBuf)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT rMuMimoActionInfo;
	int32_t i4Status = 0;
	uint32_t u4BufLen = 0;
	uint32_t i, j;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t u4IsRandomPattern, u4MsduPayloadLength0,
		 u4MsduPayloadLength1, u4MuPacketCount, u4NumOfSTAs;
	uint32_t au4MacAddrs[2][6];
	uint32_t *params[] = {
		&u4IsRandomPattern,
		&u4MsduPayloadLength0,
		&u4MsduPayloadLength1,
		&u4MuPacketCount,
		&u4MuPacketCount,
		&u4NumOfSTAs
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_MUTriggerTx\n");

	kalMemZero(&rMuMimoActionInfo, sizeof(rMuMimoActionInfo));

	ASSERT(prNetDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prNetDev));

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 6; j++) {
			if (endptr == NULL)
				return -EINVAL;
			token = kalStrSep((char **)&endptr, ":");
			if (!token || kalkStrtou32(token, 16,
				&au4MacAddrs[i][j]))
				return -EINVAL;
		}
	}

	DBGLOG(RFTEST, ERROR,
		"%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x/%x",
		u4IsRandomPattern, u4MsduPayloadLength0,
		u4MsduPayloadLength1, u4MuPacketCount,
		u4NumOfSTAs, au4MacAddrs[0][0], au4MacAddrs[0][1],
		au4MacAddrs[0][2], au4MacAddrs[0][3],
		au4MacAddrs[0][4], au4MacAddrs[0][5], au4MacAddrs[1][0],
		au4MacAddrs[1][1], au4MacAddrs[1][2],
		au4MacAddrs[1][3], au4MacAddrs[1][4], au4MacAddrs[1][5]);

	rMuMimoActionInfo.ucMuMimoCategory = MU_SET_TRIGGER_MU_TX;
	rMuMimoActionInfo.unMuMimoParam.rMuTriggerMuTx
		.fgIsRandomPattern
			= u4IsRandomPattern;
	rMuMimoActionInfo.unMuMimoParam.rMuTriggerMuTx
		.u4MsduPayloadLength0
			= u4MsduPayloadLength0;
	rMuMimoActionInfo.unMuMimoParam.rMuTriggerMuTx
		.u4MsduPayloadLength1
			= u4MsduPayloadLength1;
	rMuMimoActionInfo.unMuMimoParam.rMuTriggerMuTx.u4MuPacketCount
		= u4MuPacketCount;
	rMuMimoActionInfo.unMuMimoParam.rMuTriggerMuTx.u4NumOfSTAs
		= u4NumOfSTAs;

	for (i = 0 ; i < 2 ; i++) {
		for (j = 0 ; j < PARAM_MAC_ADDR_LEN ; j++)
			rMuMimoActionInfo.unMuMimoParam.rMuTriggerMuTx
			.aucMacAddrs[i][j]
				= au4MacAddrs[i][j];
	}
	i4Status = kalIoctl(prGlueInfo,
		wlanoidMuMimoAction,
		&rMuMimoActionInfo,
		sizeof(rMuMimoActionInfo),
		&u4BufLen);

	return i4Status;
}
#endif

#if CFG_SUPPORT_TX_BF_FPGA
int Set_TxBfProfileSwTagWrite(struct net_device *prNetDev,
	uint8_t *prInBuf)
{
	int32_t i4Status = 0, i = 0;
	uint32_t u4Lm, u4Nc, u4Nr, u4Bw, u4Codebook, u4Group;
	uint8_t *endptr;
	uint8_t *token;
	uint32_t *params[] = {
		&u4Lm,
		&u4Nc,
		&u4Nr,
		&u4Bw,
		&u4Codebook,
		&u4Group
	};

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(RFTEST, ERROR, "Set_TxBfProfileSwTagWrite\n");

	endptr = prInBuf;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		if (endptr == NULL)
			return -EINVAL;
		token = kalStrSep((char **)&endptr, ":");
		if (!token || kalkStrtou32(token, 16, params[i]))
			return -EINVAL;
	}

	if ((u4Lm > 0) && (u4Group < 3) && (u4Nr < 4) && (u4Nc < 4)
		&& (u4Codebook < 4)) {
		DBGLOG(RFTEST, ERROR,
			"%d/%d/%d/%d/%d/%d\n",
			u4Lm, u4Nr, u4Nc, u4Bw, u4Codebook,
			u4Group);

		i4Status = TxBfPseudoTagUpdate(prNetDev, u4Lm, u4Nr,
			u4Nc, u4Bw, u4Codebook, u4Group);
	} else {
		return -EINVAL;
	}

	return i4Status;
}
#endif
#endif


/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Write Efuse.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int WriteEfuse(struct net_device *prNetDev,
	       uint8_t *prInBuf)
{
	int32_t i4Status;
	uint32_t addr[2];
	uint16_t addr2[2];
	uint8_t *endptr;
	uint8_t *token;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv %s, buf: %s\n", __func__,
	       prInBuf);

	endptr = prInBuf;

	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &addr[0]))
		return -EINVAL;

	if (endptr == NULL)
		return -EINVAL;
	token = kalStrSep((char **)&endptr, ":");
	if (!token || kalkStrtou32(token, 16, &addr[1]))
		return -EINVAL;

	addr2[0] = (uint16_t) addr[0];
	addr2[1] = (uint16_t) addr[1];

	i4Status = MT_ATEWriteEfuse(prNetDev, addr2[0], addr2[1]);

	return i4Status;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Tx Power.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetTxTargetPower(struct net_device *prNetDev,
		     uint8_t *prInBuf)
{
	int32_t i4Status;
	int32_t rv;
	int addr;
	uint8_t addr2;

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set Tx Target Power, buf: %s\n", prInBuf);

	if (prInBuf == NULL)
		return -EINVAL;

	/* rv = sscanf(prInBuf, "%u", &addr);*/
	rv = kstrtoint(prInBuf, 0, &addr);

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set Tx Target Power, prInBuf: %s\n",
	       prInBuf);
	DBGLOG(INIT, ERROR,
	       "ATE_AGENT iwpriv Set Tx Target Power :%02x\n", addr);

	addr2 = (uint8_t) addr;

	if (rv == 0)
		i4Status = MT_ATESetTxTargetPower(prNetDev, addr2);
	else
		return -EINVAL;

	return i4Status;
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set RDD Report
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetRddReport(struct net_device *prNetDev,
		 uint8_t *prInBuf)
{
	int32_t i4Status;
	int32_t rv;
	int dbdcIdx;
	uint8_t ucDbdcIdx;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set RDD Report, buf: %s\n", prInBuf);

	/* rv = sscanf(prInBuf, "%u", &addr);*/
	rv = kstrtoint(prInBuf, 0, &dbdcIdx);

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set RDD Report, prInBuf: %s\n", prInBuf);
	DBGLOG(INIT, ERROR,
	       "ATE_AGENT iwpriv Set RDD Report : Band %d\n", dbdcIdx);

	if (p2pFuncGetDfsState() == DFS_STATE_INACTIVE
	    || p2pFuncGetDfsState() == DFS_STATE_DETECTED) {
		DBGLOG(REQ, ERROR,
		       "RDD Report is not supported in this DFS state (inactive or deteted)\n");
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	if (dbdcIdx != 0 && dbdcIdx != 1) {
		DBGLOG(REQ, ERROR,
		       "RDD index is not \"0\" or \"1\", Invalid data\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	ucDbdcIdx = (uint8_t) dbdcIdx;

	if (rv == 0)
		i4Status = MT_ATESetRddReport(prNetDev, ucDbdcIdx);
	else
		return -EINVAL;

	return i4Status;
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set By Pass CAC.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetByPassCac(struct net_device *prNetDev,
		 uint8_t *prInBuf)
{
	int32_t i4Status;
	int32_t rv;
	int32_t i4ByPassCacTime;
	uint32_t u4ByPassCacTime;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set By Pass Cac, buf: %s\n", prInBuf);

	rv = kstrtoint(prInBuf, 0, &i4ByPassCacTime);

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set By Pass Cac, prInBuf: %s\n", prInBuf);
	DBGLOG(INIT, ERROR,
	       "ATE_AGENT iwpriv Set By Pass Cac : %dsec\n",
	       i4ByPassCacTime);

	if (i4ByPassCacTime < 0) {
		DBGLOG(REQ, ERROR, "Cac time < 0, Invalid data\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u4ByPassCacTime = (uint32_t) i4ByPassCacTime;

	p2pFuncEnableManualCac();

	if (rv == 0)
		i4Status = p2pFuncSetDriverCacTime(u4ByPassCacTime);
	else
		return -EINVAL;

	return i4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to Set Radar Detect Mode.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int SetRadarDetectMode(struct net_device *prNetDev,
		       uint8_t *prInBuf)
{
	int32_t i4Status;
	int32_t rv;
	int radarDetectMode;
	uint8_t ucRadarDetectMode;

	if (prInBuf == NULL)
		return -EINVAL;

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set Radar Detect Mode, buf: %s\n",
	       prInBuf);

	rv = kstrtoint(prInBuf, 0, &radarDetectMode);

	DBGLOG(REQ, DEBUG,
	       "ATE_AGENT iwpriv Set Radar Detect Mode, prInBuf: %s\n",
	       prInBuf);
	DBGLOG(INIT, ERROR,
	       "ATE_AGENT iwpriv Set Radar Detect Mode : %d\n",
	       radarDetectMode);

	if (p2pFuncGetDfsState() == DFS_STATE_INACTIVE
	    || p2pFuncGetDfsState() == DFS_STATE_DETECTED) {
		DBGLOG(REQ, ERROR,
		       "RDD Report is not supported in this DFS state (inactive or deteted)\n");
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	if (radarDetectMode != 0 && radarDetectMode != 1) {
		DBGLOG(REQ, ERROR,
		       "Radar Detect Mode is not \"0\" or \"1\", Invalid data\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	ucRadarDetectMode = (uint8_t) radarDetectMode;

	p2pFuncSetRadarDetectMode(ucRadarDetectMode);

	if (rv == 0)
		i4Status = MT_ATESetRadarDetectMode(prNetDev,
						    ucRadarDetectMode);
	else
		return -EINVAL;

	return i4Status;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief  This routine is called to search the corresponding ATE agent
 *         function.
 *
 * \param[in] prNetDev		Pointer to the Net Device
 * \param[in] prInBuf		A pointer to the command string buffer
 * \param[in] u4InBufLen	The length of the buffer
 * \param[out] None
 *
 * \retval 0				On success.
 * \retval -EINVAL			If invalid argument.
 */
/*----------------------------------------------------------------------------*/
int AteCmdSetHandle(struct net_device *prNetDev,
		    uint8_t *prInBuf, uint32_t u4InBufLen)
{
	uint8_t *this_char, *value;
	struct ATE_PRIV_CMD *prAtePrivCmd;
	int32_t i4Status = 0;

	while ((this_char = strsep((char **)&prInBuf,
				   ",")) != NULL) {
		if (!*this_char)
			continue;
		DBGLOG(RFTEST, ERROR, "ATE_AGENT iwpriv this_char = %s\n",
		       this_char);
		DBGLOG(RFTEST, DEBUG, "ATE_AGENT iwpriv this_char = %s\n",
		       this_char);

		value = strchr(this_char, '=');
		if (value != NULL)
			*value++ = 0;

		DBGLOG(REQ, DEBUG, "ATE_AGENT iwpriv cmd = %s, value = %s\n",
		       this_char, value);

		for (prAtePrivCmd = rAtePrivCmdTable; prAtePrivCmd->name;
		     prAtePrivCmd++) {
			if (!strcmp(this_char, prAtePrivCmd->name)) {
				/* FALSE: Set private failed then return Invalid
				 *        argument
				 */
				if (prAtePrivCmd->set_proc(prNetDev, value)
				    != 0)
					i4Status = -EINVAL;
				break;	/*Exit for loop. */
			}
		}

		if (prAtePrivCmd->name == NULL) {	/*Not found argument */
			i4Status = -EINVAL;
			break;
		}
	}
	return i4Status;
}
#endif /*CFG_SUPPORT_QA_TOOL */
