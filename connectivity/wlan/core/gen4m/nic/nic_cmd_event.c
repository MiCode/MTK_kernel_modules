// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   nic_cmd_event.c
 *    \brief  Callback functions for Command packets.
 *
 *	Various Event packet handlers which will be setup in the callback
 *  function of a command packet.
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
#include "gl_ate_agent.h"
#include "bss.h"
#if (CFG_SUPPORT_FW_IDX_LOG_SAVE == 1)
#include "gl_fw_dev.h"
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
#include "fw_log_parser.h"
#endif

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
#include "gl_ics.h"
#endif

#if CFG_SUPPORT_MBRAIN
#include "gl_mbrain.h"
#endif

#if (CFG_HW_DETECT_REPORT == 1)
#include "conn_dbg.h"
#endif
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define NIC_FILL_CAP_V2_REF_TBL(_tag_type, _hdlr)        \
{                                                        \
	.tag_type = _tag_type,                           \
	.hdlr = _hdlr,				         \
}

const struct NIC_CAPABILITY_V2_REF_TABLE
	gNicCapabilityV2InfoTable[] = {
#if defined(_HIF_SDIO)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_TX_RESOURCE,
				nicEventQueryTxResourceEntry),
#endif
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_TX_EFUSEADDRESS,
				nicCmdEventQueryNicEfuseAddr),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_COEX_FEATURE,
				nicCmdEventQueryNicCoexFeature),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_SINGLE_SKU,
				rlmDomainExtractSingleSkuInfoFromFirmware),
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_CSUM_OFFLOAD,
				nicCmdEventQueryNicCsumOffload),
#endif
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_HW_VERSION,
				nicCfgChipCapHwVersion),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_SW_VERSION,
				nicCfgChipCapSwVersion),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_MAC_ADDR,
				nicCfgChipCapMacAddr),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_PHY_CAP,
				nicCfgChipCapPhyCap),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_LIMITED,
				nicCfgChipCapLimited),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_MAC_CAP,
				nicCfgChipCapMacCap),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_FRAME_BUF_CAP,
				nicCfgChipCapFrameBufCap),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_BEAMFORM_CAP,
				nicCfgChipCapBeamformCap),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_LOCATION_CAP,
				nicCfgChipCapLocationCap),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_MUMIMO_CAP,
				nicCfgChipCapMuMimoCap),
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_HW_ADIE_VERSION,
				nicCfgChipAdieHwVersion),
#if CFG_SUPPORT_ANT_SWAP
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_ANTSWP,
				nicCfgChipCapAntSwpCap),
#endif
#if (CFG_SUPPORT_P2PGO_ACS == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_P2P,
				nicCfgChipP2PCap),
#endif
#if (CFG_SUPPORT_RX_QUOTA_INFO == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_PSE_RX_QUOTA,
				nicCfgChipPseRxQuota),
#endif
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_HOST_STATUS_EMI_OFFSET,
				nicCmdEventHostStatusEmiOffset),
#if (CFG_SUPPORT_WIFI_6G == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_6G_CAP,
				nicCfgChipCap6GCap),
#endif
#if CFG_SUPPORT_LLS
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_LLS_DATA_EMI_OFFSET,
				nicCmdEventLinkStatsEmiOffset),
#endif
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_CASAN_LOAD_TYPE,
				nicCmdEventCasanLoadType),
#if IS_ENABLED(CFG_MTK_WIFI_SUPPORT_UDS_FWDL)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_REDL_INFO,
				nicCfgChipCapRedlInfo),
#endif
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_HOST_SUSPEND_INFO,
				nicCmdEventHostSuspendInfo),
#if CFG_FAST_PATH_SUPPORT
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_FAST_PATH, nicCfgChipCapFastPath),
#endif
#if CFG_SUPPORT_MLR
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_MLR_CAP,
				nicCfgChipCapMlr),
#endif
#if (CFG_SUPPORT_CONNAC3X == 1)
#if (CFG_SUPPORT_QA_TOOL == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_RF_TEST_CAP,
				nicCmdEventTestmodeCap),
#endif
#endif
#if CFG_SUPPORT_802_11BE_MLO
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_MLO_CAP,
				nicCfgChipCapMLO),
#endif
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_STATS_REG_MONTR_EMI_OFFSET,
				nicCfgChipCapStatsRegMontrEmiOffset),
#endif
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_SW_SYNC_BY_EMI,
				nicCfgGetSwSyncEMIOffset),
#endif
#if CFG_SUPPORT_MBRAIN
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_MBRAIN_EMI_INFO,
				nicCfgChipMbrEmiInfo),
#endif
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
struct MIB_INFO_STAT g_arMibInfo[ENUM_BAND_NUM];
uint8_t fgEfuseCtrlAxOn = 1; /* run time control if support AX by efuse */
#if CFG_SUPPORT_NAN
struct _TXM_CMD_EVENT_TEST_T grCmdInfoQueryTestBuffer;
#endif

/*******************************************************************************
 *                            F U N C T I O N   D A T A
 *******************************************************************************
 */
void nicCmdEventQueryMcrRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct PARAM_CUSTOM_MCR_RW_STRUCT *prMcrRdInfo;
	struct GLUE_INFO *prGlueInfo;
	struct CMD_ACCESS_REG *prCmdAccessReg;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCmdAccessReg = (struct CMD_ACCESS_REG *) (pucEventBuf);

		u4QueryInfoLen = sizeof(struct PARAM_CUSTOM_MCR_RW_STRUCT);

		prMcrRdInfo = (struct PARAM_CUSTOM_MCR_RW_STRUCT *)
			      prCmdInfo->pvInformationBuffer;
		prMcrRdInfo->u4McrOffset = prCmdAccessReg->u4Address;
		prMcrRdInfo->u4McrData = prCmdAccessReg->u4Data;

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	return;

}

void nicCmdEventQueryCfgRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct CMD_HEADER *prInCfgHeader;
	struct GLUE_INFO *prGlueInfo;
	struct CMD_HEADER *prOutCfgHeader;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prInCfgHeader = (struct CMD_HEADER *) (pucEventBuf);
		u4QueryInfoLen = sizeof(struct CMD_HEADER);
		prOutCfgHeader = (struct CMD_HEADER *)
			(prCmdInfo->pvInformationBuffer);

		kalMemCopy(prOutCfgHeader, prInCfgHeader,
			sizeof(struct CMD_HEADER));

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

#if CFG_SUPPORT_TX_BF
void nicCmdEventPfmuDataRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	union PFMU_DATA *prEventPfmuDataRead = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventPfmuDataRead = (union PFMU_DATA *) (pucEventBuf);

		u4QueryInfoLen = sizeof(union PFMU_DATA);
#if CFG_SUPPORT_QA_TOOL
		g_rPfmuData = *prEventPfmuDataRead;
#endif
	}

	if (prEventPfmuDataRead == NULL)
		return;

	DBGLOG(INIT, INFO, "=========== Low Seg Angles ===========\n");
	DBGLOG(INIT, INFO, "Psi21 = 0x%02x, Phi11 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegAng.ucPsi21,
		prEventPfmuDataRead->rField.rLowSegAng.u2Phi11);
	DBGLOG(INIT, INFO, "Psi31 = 0x%02x, Phi21 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegAng.ucPsi31,
		prEventPfmuDataRead->rField.rLowSegAng.u2Phi21);
	DBGLOG(INIT, INFO, "Psi41 = 0x%02x, Phi31 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegAng.ucPsi41,
		prEventPfmuDataRead->rField.rLowSegAng.u2Phi31);
	DBGLOG(INIT, INFO, "Psi32 = 0x%02x, Phi22 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegAng.ucPsi32,
		prEventPfmuDataRead->rField.rLowSegAng.u2Phi22);
	DBGLOG(INIT, INFO, "Psi42 = 0x%02x, Phi32 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegAng.ucPsi42,
		prEventPfmuDataRead->rField.rLowSegAng.u2Phi32);
	DBGLOG(INIT, INFO, "Psi43 = 0x%02x, Phi33 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegAng.ucPsi43,
		prEventPfmuDataRead->rField.rLowSegAng.u2Phi33);

	DBGLOG(INIT, INFO, "============ Low Seg SNRs ============\n");
	DBGLOG(INIT, INFO, "SNR00 = 0x%03x, SNR01 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegSnr.u2dSNR00,
		prEventPfmuDataRead->rField.rLowSegSnr.u2dSNR01);
	DBGLOG(INIT, INFO, "SNR02 = 0x%03x, SNR03 = 0x%03x\n",
		prEventPfmuDataRead->rField.rLowSegSnr.u2dSNR02,
		prEventPfmuDataRead->rField.rLowSegSnr.u2dSNR03_MSB << 2 |
		prEventPfmuDataRead->rField.rLowSegSnr.u2dSNR03);

	DBGLOG(INIT, INFO, "======================================\n");
}

void nicCmdEventPfmuTagRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct EVENT_PFMU_TAG_READ *prEventPfmuTagRead = NULL;
	struct PARAM_CUSTOM_PFMU_TAG_READ_STRUCT *prPfumTagRead =
			NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	if (!pucEventBuf) {
		DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
		return;
	}
	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(INIT, ERROR,
		       "prCmdInfo->pvInformationBuffer is NULL.\n");
		return;
	}
	/* 4 <2> Update information of OID */
	if (!prCmdInfo->fgIsOid) {
		DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!",
		       prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		return;
	}
	prGlueInfo = prAdapter->prGlueInfo;
	prEventPfmuTagRead = (struct EVENT_PFMU_TAG_READ *) (
				     pucEventBuf);

	prPfumTagRead = (struct PARAM_CUSTOM_PFMU_TAG_READ_STRUCT *)
			prCmdInfo->pvInformationBuffer;

	kalMemCopy(prPfumTagRead, prEventPfmuTagRead,
		   sizeof(struct EVENT_PFMU_TAG_READ));

	u4QueryInfoLen = sizeof(union CMD_TXBF_ACTION);

#if CFG_SUPPORT_QA_TOOL
	g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1;
	g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2;
#endif
	DBGLOG(INIT, INFO,
	       "========================== (R)Tag1 info ==========================\n");

	DBGLOG(INIT, INFO,
	       " Row data0 : %x, Row data1 : %x, Row data2 : %x, Row data3 : %x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[0],
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[1],
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[2],
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[3]);
	DBGLOG(INIT, INFO,
	       " Row data4 : %x, Row data5 : %x, Row data6 : %x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[4],
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[5],
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[6]);
	DBGLOG(INIT, INFO, "ProfileID = %d Invalid status = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucProfileID,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucInvalidProf);
	DBGLOG(INIT, INFO, "0:iBF / 1:eBF = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucTxBf);
	DBGLOG(INIT, INFO, "DBW(0/1/2/3 BW20/40/80/160NC) = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucDBW);
	DBGLOG(INIT, INFO, "0:SU / 1:MU = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSU_MU);
	DBGLOG(INIT, INFO,
	       "Nrow = %d, Ncol = %d, Ng = %d, LM = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNrow,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNcol,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNgroup,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucLM);
	DBGLOG(INIT, INFO, "  ucCodeBook  = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucCodeBook);
	DBGLOG(INIT, INFO, "  ucRMSD = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucRMSD);
	DBGLOG(INIT, INFO,
	       "Mem1(%d, %d), Mem2(%d, %d), Mem3(%d, %d), Mem4(%d, %d)\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr1ColIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr1RowIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr2ColIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr2RowIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr3ColIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr3RowIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr4ColIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr4RowIdx);
	DBGLOG(INIT, INFO, "  ucRuStartIdx = 0x%x ucRuEndIdx = 0x%x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucRuStartIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucRuEndIdx);
	DBGLOG(INIT, INFO,
	       "SNR STS0=0x%x, SNR STS1=0x%x, SNR STS2=0x%x, SNR STS3=0x%x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS0,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS1,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS2,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS3);
	DBGLOG(INIT, INFO,
	       "SNR STS4=0x%x, SNR STS5=0x%x, SNR STS6=0x%x, SNR STS7=0x%x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS4,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS5,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS6,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS7);
	DBGLOG(INIT, INFO,
	       "===============================================================\n");

	DBGLOG(INIT, INFO,
	       "========================== (R)Tag2 info ==========================\n");
	DBGLOG(INIT, INFO,
	       " Row data0 : %x, Row data1 : %x, Row data2 : %x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[0],
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[1],
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[2]);
	DBGLOG(INIT, INFO,
	       " Raw data3 : %x, Raw data4 : %x, Raw data5 : %x, Raw data6 : %x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[3],
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[4],
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[5],
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[6]);
	DBGLOG(INIT, INFO, "Smart Ant Cfg = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.u2SmartAnt);
	DBGLOG(INIT, INFO, "SE index = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucSEIdx);
	DBGLOG(INIT, INFO, "iBF lifetime limit(unit:4ms) = 0x%x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfTimeOut);
	DBGLOG(INIT, INFO,
	       "iBF desired DBW = %d\n  0/1/2/3 : BW20/40/80/160NC\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfDBW);
	DBGLOG(INIT, INFO,
	       "iBF desired Ncol = %d\n  0/1/2 : Ncol = 1 ~ 3\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfNcol);
	DBGLOG(INIT, INFO,
	       "iBF desired Nrow = %d\n  0/1/2/3 : Nrow = 1 ~ 4\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfNrow);
	DBGLOG(INIT, INFO,
	       "iBf Ru = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfRu);
	DBGLOG(INIT, INFO,
	       "===============================================================\n");

}
#endif /* CFG_SUPPORT_TX_BF */

#if CFG_SUPPORT_QA_TOOL
void nicCmdEventQueryRxStatistics(struct ADAPTER
				  *prAdapter, struct CMD_INFO *prCmdInfo,
				  uint8_t *pucEventBuf)
{
#if (CFG_SUPPORT_CONNAC3X == 0)
	struct PARAM_CUSTOM_ACCESS_RX_STAT *prRxStatistics;
	struct EVENT_ACCESS_RX_STAT *prEventAccessRxStat;
	uint32_t u4QueryInfoLen, i;
	struct GLUE_INFO *prGlueInfo;
	uint32_t *prElement;
	uint32_t u4Temp;
	uint32_t u4LoopCount;
	/* P_CMD_ACCESS_RX_STAT                  prCmdRxStat, prRxStat; */

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventAccessRxStat = (struct EVENT_ACCESS_RX_STAT *) (
					      pucEventBuf);

		prRxStatistics = (struct PARAM_CUSTOM_ACCESS_RX_STAT *)
				 prCmdInfo->pvInformationBuffer;
		prRxStatistics->u4SeqNum = prEventAccessRxStat->u4SeqNum;
		prRxStatistics->u4TotalNum =
			prEventAccessRxStat->u4TotalNum;

		u4QueryInfoLen = sizeof(struct CMD_ACCESS_RX_STAT);

		if (prEventAccessRxStat->u4TotalNum < HQA_RX_STATISTIC_NUM)
			DBGLOG(INIT, WARN, "u4TotalNum=%u too small",
			       prEventAccessRxStat->u4TotalNum);

		u4LoopCount = kal_min_t(uint32_t,
					prEventAccessRxStat->u4TotalNum,
					HQA_RX_STATISTIC_NUM);
		if (prRxStatistics->u4SeqNum == u4RxStatSeqNum) {
			prElement = &g_HqaRxStat.MAC_FCS_Err;
			for (i = 0; i < u4LoopCount; i++) {
				u4Temp = NTOHL(
					prEventAccessRxStat->au4Buffer[i]);
				kalMemCopy(prElement, &u4Temp, 4);

				prElement++;
			}

#if 0	/* copy in for-loop */
			g_HqaRxStat.AllMacMdrdy0 = ntohl(
				prEventAccessRxStat->au4Buffer[i]);
			i++;
			g_HqaRxStat.AllMacMdrdy1 = ntohl(
				prEventAccessRxStat->au4Buffer[i]);
			/* i++; */
			/* g_HqaRxStat.AllFCSErr0 =
			 * ntohl(prEventAccessRxStat->au4Buffer[i]);
			 */
			/* i++; */
			/* g_HqaRxStat.AllFCSErr1 =
			 * ntohl(prEventAccessRxStat->au4Buffer[i]);
			 */
#endif
		}

		DBGLOG(INIT, ERROR,
		       "MT6632 : RX Statistics Test SeqNum = %u, TotalNum = %u\n",
		       prEventAccessRxStat->u4SeqNum,
		       prEventAccessRxStat->u4TotalNum);

		DBGLOG(INIT, ERROR,
		       "MAC_FCS_ERR = %u, MAC_MDRDY = %u, MU_RX_CNT = %u, RX_FIFO_FULL = %u\n",
		       g_HqaRxStat.MAC_FCS_Err,
		       g_HqaRxStat.MAC_Mdrdy,
		       g_HqaRxStat.MRURxCount,
		       g_HqaRxStat.OutOfResource);

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
#endif
}

#if CFG_SUPPORT_MU_MIMO
void nicCmdEventGetQd(struct ADAPTER *prAdapter,
		      struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct EVENT_HQA_GET_QD *prEventHqaGetQd;
	uint32_t i;

	struct PARAM_CUSTOM_GET_QD_STRUCT *prGetQd = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	if (!pucEventBuf) {
		DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
		return;
	}
	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(INIT, ERROR,
		       "prCmdInfo->pvInformationBuffer is NULL.\n");
		return;
	}
	/* 4 <2> Update information of OID */
	if (!prCmdInfo->fgIsOid) {
		DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!\n",
		       prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		return;
	}
	prGlueInfo = prAdapter->prGlueInfo;
	prEventHqaGetQd = (struct EVENT_HQA_GET_QD *) (pucEventBuf);

	prGetQd = (struct PARAM_CUSTOM_GET_QD_STRUCT *)
		  prCmdInfo->pvInformationBuffer;

	kalMemCopy(prGetQd, prEventHqaGetQd,
		   sizeof(struct EVENT_HQA_GET_QD));

	u4QueryInfoLen = sizeof(union CMD_MUMIMO_ACTION);

	/* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
	/* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

	kalOidComplete(prGlueInfo, prCmdInfo,
		       u4QueryInfoLen, WLAN_STATUS_SUCCESS);

	DBGLOG(INIT, INFO, " event id : %x\n", prGetQd->u4EventId);
	for (i = 0; i < 14; i++)
		DBGLOG(INIT, INFO, "au4RawData[%d]: %x\n", i,
		       prGetQd->au4RawData[i]);

}

void nicCmdEventGetCalcLq(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct EVENT_HQA_GET_MU_CALC_LQ *prEventHqaGetMuCalcLq;
	uint32_t i, j;

	struct PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT *prGetMuCalcLq =
			NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	if (!pucEventBuf) {
		DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
		return;
	}
	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(INIT, ERROR,
		       "prCmdInfo->pvInformationBuffer is NULL.\n");
		return;
	}
	/* 4 <2> Update information of OID */
	if (!prCmdInfo->fgIsOid) {
		DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!\n",
		       prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		return;
	}
	prGlueInfo = prAdapter->prGlueInfo;
	prEventHqaGetMuCalcLq = (struct EVENT_HQA_GET_MU_CALC_LQ *)
				(pucEventBuf);

	prGetMuCalcLq = (struct PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT
			 *) prCmdInfo->pvInformationBuffer;

	kalMemCopy(prGetMuCalcLq, prEventHqaGetMuCalcLq,
		   sizeof(struct EVENT_HQA_GET_MU_CALC_LQ));

	u4QueryInfoLen = sizeof(union CMD_MUMIMO_ACTION);

	/* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
	/* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

	kalOidComplete(prGlueInfo, prCmdInfo,
		       u4QueryInfoLen, WLAN_STATUS_SUCCESS);


	DBGLOG(INIT, INFO, " event id : %x\n",
	       prGetMuCalcLq->u4EventId);
	for (i = 0; i < NUM_OF_USER; i++)
		for (j = 0; j < NUM_OF_MODUL; j++)
			DBGLOG(INIT, INFO, " lq_report[%d][%d]: %x\n", i, j,
			       prGetMuCalcLq->rEntry.lq_report[i][j]);

}

void nicCmdEventGetCalcInitMcs(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct EVENT_SHOW_GROUP_TBL_ENTRY *prEventShowGroupTblEntry
			= NULL;

	struct PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT
		*prShowGroupTbl;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	if (!pucEventBuf) {
		DBGLOG(INIT, ERROR, "pucEventBuf is NULL.\n");
		return;
	}
	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(INIT, ERROR,
		       "prCmdInfo->pvInformationBuffer is NULL.\n");
		return;
	}
	/* 4 <2> Update information of OID */
	if (!prCmdInfo->fgIsOid) {
		DBGLOG(INIT, ERROR, "cmd %u seq #%u not oid!\n",
		       prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		return;
	}
	prGlueInfo = prAdapter->prGlueInfo;
	prEventShowGroupTblEntry = (struct
				    EVENT_SHOW_GROUP_TBL_ENTRY *) (pucEventBuf);

	prShowGroupTbl = (struct
			  PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT *)
			 prCmdInfo->pvInformationBuffer;

	kalMemCopy(prShowGroupTbl, prEventShowGroupTblEntry,
		   sizeof(struct EVENT_SHOW_GROUP_TBL_ENTRY));

	u4QueryInfoLen = sizeof(union CMD_MUMIMO_ACTION);

	/* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
	/* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

	kalOidComplete(prGlueInfo, prCmdInfo,
		       u4QueryInfoLen, WLAN_STATUS_SUCCESS);


	DBGLOG(INIT, INFO,
	       "========================== (R)Group table info ==========================\n");
	DBGLOG(INIT, INFO, " event id : %x\n",
	       prEventShowGroupTblEntry->u4EventId);
	DBGLOG(INIT, INFO, "index = %x numUser = %x\n",
	       prEventShowGroupTblEntry->index,
	       prEventShowGroupTblEntry->numUser);
	DBGLOG(INIT, INFO, "BW = %x NS0/1/ = %x/%x\n",
	       prEventShowGroupTblEntry->BW, prEventShowGroupTblEntry->NS0,
	       prEventShowGroupTblEntry->NS1);
	DBGLOG(INIT, INFO, "PFIDUser0/1 = %x/%x\n",
	       prEventShowGroupTblEntry->PFIDUser0,
	       prEventShowGroupTblEntry->PFIDUser1);
	DBGLOG(INIT, INFO,
	       "fgIsShortGI = %x, fgIsUsed = %x, fgIsDisable = %x\n",
	       prEventShowGroupTblEntry->fgIsShortGI,
	       prEventShowGroupTblEntry->fgIsUsed,
	       prEventShowGroupTblEntry->fgIsDisable);
	DBGLOG(INIT, INFO, "initMcsUser0/1 = %x/%x\n",
	       prEventShowGroupTblEntry->initMcsUser0,
	       prEventShowGroupTblEntry->initMcsUser1);
	DBGLOG(INIT, INFO, "dMcsUser0: 0/1/ = %x/%x\n",
	       prEventShowGroupTblEntry->dMcsUser0,
	       prEventShowGroupTblEntry->dMcsUser1);

}
#endif /* CFG_SUPPORT_MU_MIMO */
#endif /* CFG_SUPPORT_QA_TOOL */

void nicCmdEventQuerySwCtrlRead(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT *prSwCtrlInfo;
	struct GLUE_INFO *prGlueInfo;
	struct CMD_SW_DBG_CTRL *prCmdSwCtrl;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCmdSwCtrl = (struct CMD_SW_DBG_CTRL *) (pucEventBuf);

		u4QueryInfoLen = sizeof(struct PARAM_CUSTOM_SW_CTRL_STRUCT);

		prSwCtrlInfo = (struct PARAM_CUSTOM_SW_CTRL_STRUCT *)
			       prCmdInfo->pvInformationBuffer;
		prSwCtrlInfo->u4Id = prCmdSwCtrl->u4Id;
		prSwCtrlInfo->u4Data = prCmdSwCtrl->u4Data;

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventQueryChipConfig(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT *prChipConfigInfo;
	struct GLUE_INFO *prGlueInfo;
	struct CMD_CHIP_CONFIG *prCmdChipConfig;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCmdChipConfig = (struct CMD_CHIP_CONFIG *) (pucEventBuf);

		u4QueryInfoLen = sizeof(struct
					PARAM_CUSTOM_CHIP_CONFIG_STRUCT);

		if (prCmdInfo->u4InformationBufferLength < sizeof(
			    struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT)) {
			DBGLOG(REQ, INFO,
			       "Chip config u4InformationBufferLength %u is not valid (event)\n",
			       prCmdInfo->u4InformationBufferLength);
		}
		prChipConfigInfo = (struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT
				    *) prCmdInfo->pvInformationBuffer;
		prChipConfigInfo->ucRespType = prCmdChipConfig->ucRespType;
		prChipConfigInfo->u2MsgSize = prCmdChipConfig->u2MsgSize;
		DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__,
		       prChipConfigInfo->ucRespType);
		DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__,
		       prChipConfigInfo->u2MsgSize);

		if (prChipConfigInfo->u2MsgSize > CHIP_CONFIG_RESP_SIZE) {
			DBGLOG(REQ, WARN,
			       "Chip config Msg Size %u is not valid (event)\n",
			       prChipConfigInfo->u2MsgSize);
			prChipConfigInfo->u2MsgSize = CHIP_CONFIG_RESP_SIZE;
		}

		kalMemCopy(prChipConfigInfo->aucCmd,
		   prCmdChipConfig->aucCmd, prChipConfigInfo->u2MsgSize);
		kalOidComplete(prGlueInfo, prCmdInfo,
		   u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventSetCommon(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
	    prCmdInfo->u4InformationBufferLength, WLAN_STATUS_SUCCESS);
	}
}

void nicCmdEventSetIpAddress(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4Count;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	u4Count = (prCmdInfo->u4SetInfoLen - OFFSET_OF(
			   struct CMD_SET_NETWORK_ADDRESS_LIST, arNetAddress))
		  / sizeof(struct CMD_IPV4_NETWORK_ADDRESS);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			prCmdInfo,
			sizeof(struct PARAM_NETWORK_ADDRESS_LIST) + u4Count *
			(OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress) +
				sizeof(struct PARAM_NETWORK_ADDRESS_IP)),
			WLAN_STATUS_SUCCESS);
	}

}
/* fos_change begin */
#if CFG_SUPPORT_SET_IPV6_NETWORK
void nicCmdEventSetIpv6Address(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4Count;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	u4Count = (prCmdInfo->u4SetInfoLen - OFFSET_OF(
		struct CMD_IPV6_NETWORK_ADDRESS_LIST, arNetAddress))
		/ sizeof(struct CMD_IPV6_NETWORK_ADDRESS);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			prCmdInfo,
			sizeof(struct PARAM_NETWORK_ADDRESS_LIST) + u4Count *
			(OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress) +
				sizeof(struct PARAM_NETWORK_ADDRESS_IPV6)),
			WLAN_STATUS_SUCCESS);
	}
}
#endif /* fos_change end */


void nicCmdEventQueryRfTestATInfo(struct ADAPTER
				  *prAdapter, struct CMD_INFO *prCmdInfo,
				  uint8_t *pucEventBuf)
{
	union EVENT_TEST_STATUS *prTestStatus, *prQueryBuffer;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prTestStatus = (union EVENT_TEST_STATUS *) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prQueryBuffer = (union EVENT_TEST_STATUS *)
				prCmdInfo->pvInformationBuffer;

		/* Memory copy length is depended on upper-layer */
		kalMemCopy(prQueryBuffer, prTestStatus,
			   prCmdInfo->u4InformationBufferLength);

		u4QueryInfoLen = sizeof(union EVENT_TEST_STATUS);

		/* Update Query Information Length */
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is in response of OID_GEN_LINK_SPEED query request
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param prCmdInfo      Pointer to the pending command info
 * @param pucEventBuf
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryLinkQuality(struct ADAPTER *prAdapter,
				struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	struct EVENT_LINK_QUALITY *prLinkQuality;
	struct PARAM_LINK_SPEED_EX *prLinkSpeed;
	struct LINK_QUALITY *prBaseLq;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;
	uint32_t i;

	prLinkQuality = (struct EVENT_LINK_QUALITY *) pucEventBuf;

	DBGLOG(NIC, TRACE, "prCmdInfo->fgIsOid=%u", prCmdInfo->fgIsOid);

	if (!prCmdInfo->fgIsOid)
		return;

	prGlueInfo = prAdapter->prGlueInfo;
	prLinkSpeed = (struct PARAM_LINK_SPEED_EX *) (
				   prCmdInfo->pvInformationBuffer);

	DBGLOG(NIC, TRACE, "Cmd=%p, pvInformationBuffer=%p",
		prCmdInfo, prCmdInfo->pvInformationBuffer);

	for (i = 0; i < 4; i++) {
		struct LINK_SPEED_EX_ *prLq;

		if (!prLinkQuality->rLq[i].ucIsLQ0Rdy)
			continue;

		prBaseLq = &prLinkQuality->rLq[i];
		prLq = &prAdapter->rLinkQuality.rLq[i];

		nicUpdateLinkQuality(prAdapter, i, prBaseLq->cRssi,
				prBaseLq->cLinkQuality, prBaseLq->u2LinkSpeed,
				prBaseLq->ucMediumBusyPercentage,
				prBaseLq->ucIsLQ0Rdy);

		prLinkSpeed->rLq[i].u2TxLinkSpeed = prLq->u2TxLinkSpeed;
		prLinkSpeed->rLq[i].u2RxLinkSpeed = prLq->u2RxLinkSpeed;
		prLinkSpeed->rLq[i].cRssi = prLq->cRssi;
		prLinkSpeed->rLq[i].fgIsLinkRateValid = TRUE;

		DBGLOG(NIC, TRACE,
			"ucBssIdx=%d, TxRate=%u, RxRate=%u signal=%d\n",
			i,
			prLinkSpeed->rLq[i].u2TxLinkSpeed,
			prLinkSpeed->rLq[i].u2RxLinkSpeed,
			prLinkSpeed->rLq[i].cRssi);
	}

	u4QueryInfoLen = sizeof(struct PARAM_LINK_SPEED_EX);

	kalOidComplete(prGlueInfo, prCmdInfo,
		u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}

void nicUpdateStatistics(struct ADAPTER *prAdapter,
	struct PARAM_802_11_STATISTICS_STRUCT *prStatistics,
	struct EVENT_STATISTICS *prEventStatistics
)
{
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo;
	struct SCAN_INFO *prScanInfo;
#endif
	prStatistics->rTransmittedFragmentCount =
		prEventStatistics->rTransmittedFragmentCount;
	prStatistics->rMulticastTransmittedFrameCount =
		prEventStatistics->rMulticastTransmittedFrameCount;
	prStatistics->rFailedCount =
		prEventStatistics->rFailedCount;
	prStatistics->rRetryCount = prEventStatistics->rRetryCount;
	prStatistics->rMultipleRetryCount =
		prEventStatistics->rMultipleRetryCount;
	prStatistics->rRTSSuccessCount =
		prEventStatistics->rRTSSuccessCount;
	prStatistics->rRTSFailureCount =
		prEventStatistics->rRTSFailureCount;
	prStatistics->rACKFailureCount =
		prEventStatistics->rACKFailureCount;
	prStatistics->rFrameDuplicateCount =
		prEventStatistics->rFrameDuplicateCount;
	prStatistics->rReceivedFragmentCount =
		prEventStatistics->rReceivedFragmentCount;
	prStatistics->rMulticastReceivedFrameCount =
		prEventStatistics->rMulticastReceivedFrameCount;
	prStatistics->rFCSErrorCount =
		prEventStatistics->rFCSErrorCount;
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prStatistics->rMdrdyCnt = prEventStatistics->rMdrdyCnt;
	prStatistics->rChnlIdleCnt = prEventStatistics->rChnlIdleCnt;
	prStatistics->u4HwAwakeDuration =
		prEventStatistics->u4HwMacAwakeDuration;

	prLinkQualityInfo =
		&(prAdapter->rLinkQualityInfo);

	prLinkQualityInfo->u8TxRetryCount =
		prStatistics->rRetryCount.QuadPart;

	prLinkQualityInfo->u8TxRtsFailCount =
		prStatistics->rRTSFailureCount.QuadPart;
	prLinkQualityInfo->u8TxAckFailCount =
		prStatistics->rACKFailureCount.QuadPart;
	prLinkQualityInfo->u8TxFailCount =
		prLinkQualityInfo->u8TxRtsFailCount +
		prLinkQualityInfo->u8TxAckFailCount;
	prLinkQualityInfo->u8TxTotalCount =
		prStatistics->rTransmittedFragmentCount.QuadPart;

	prLinkQualityInfo->u8RxTotalCount =
		prStatistics->rReceivedFragmentCount.QuadPart;
	/* FW report is diff, driver count total */
	prLinkQualityInfo->u8RxErrCount =
		prStatistics->rFCSErrorCount.QuadPart;
	prLinkQualityInfo->u8MdrdyCount =
		prStatistics->rMdrdyCnt.QuadPart;
	prLinkQualityInfo->u8IdleSlotCount =
		prStatistics->rChnlIdleCnt.QuadPart;
	prLinkQualityInfo->u4HwMacAwakeDuration =
		prStatistics->u4HwAwakeDuration;
	if (prScanInfo->eCurrentState == SCAN_STATE_SCANNING)
		prLinkQualityInfo->u2FlagScanning = 1;
	else
		prLinkQualityInfo->u2FlagScanning = 0;

	DBGLOG(SW4, TRACE,
		   "EVENT_STATISTICS: rTransmittedFragmentCount.QuadPart:%lld, rRetryCount.QuadPart:%lld, rRTSFailureCount.QuadPart:%lld, rACKFailureCount.QuadPart:%lld, rReceivedFragmentCount.QuadPart:%lld, rFCSErrorCount.QuadPart:%lld, rChnlIdleCnt.QuadPart:%lld\n",
		   prEventStatistics->rTransmittedFragmentCount.QuadPart,
		   prEventStatistics->rRetryCount.QuadPart,
		   prEventStatistics->rRTSFailureCount.QuadPart,
		   prEventStatistics->rACKFailureCount.QuadPart,
		   prEventStatistics->rReceivedFragmentCount.QuadPart,
		   prEventStatistics->rFCSErrorCount.QuadPart,
		   prEventStatistics->rChnlIdleCnt.QuadPart
	);
#endif
}

void nicCmdEventQueryStatistics(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	struct PARAM_802_11_STATISTICS_STRUCT *prStatistics;
	struct EVENT_STATISTICS *prEventStatistics;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;


	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (struct EVENT_STATISTICS *) pucEventBuf;

	prGlueInfo = prAdapter->prGlueInfo;

	u4QueryInfoLen = sizeof(struct
				PARAM_802_11_STATISTICS_STRUCT);
	prStatistics = (struct PARAM_802_11_STATISTICS_STRUCT *)
		       prCmdInfo->pvInformationBuffer;

	nicUpdateStatistics(prAdapter, prStatistics,
		prEventStatistics);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}

#if CFG_SUPPORT_LLS
void nicCmdEventQueryLinkStats(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct GLUE_INFO *prGlueInfo;
	uint16_t len = prCmdInfo->u4InformationBufferLength;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prGlueInfo = prAdapter->prGlueInfo;

	DBGLOG(NIC, TRACE, "Pend=%p, Cmd=%p, oid=%u, Buf=%p, len=%u",
			&prGlueInfo->rPendComp, prCmdInfo,
			prCmdInfo->fgIsOid, prCmdInfo->pvInformationBuffer,
			len);

	memcpy((uint8_t *)prCmdInfo->pvInformationBuffer, pucEventBuf, len);

	DBGLOG(RX, TRACE, "kalOidComplete: infoLen=%u", len);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo, len, WLAN_STATUS_SUCCESS);
}

/**
 * Check buffer size against reported event buffer in this function,
 * since the event buffer size, prEvent->u2PacketLength, is only available
 * in this function.
 *
 * If the size is not enough, OID complete with FAILURE.
 * If the size is enough, clear the destination buffer, and set the data length,
 * call the command done handler, it will copy the data to returning buffer,
 * and OID complete the caller if needed.
 */
void nicEventStatsLinkStats(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (!prCmdInfo)
		return;

	if (unlikely(prEvent->u2PacketLength - sizeof(struct WIFI_EVENT) >
					prCmdInfo->u4InformationBufferLength)) {
		DBGLOG(RX, WARN, "prEventLen=%u-%zu, BufLen=%u",
				prEvent->u2PacketLength,
				sizeof(struct WIFI_EVENT),
				prCmdInfo->u4InformationBufferLength);
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo, 0,
				WLAN_STATUS_FAILURE);
	} else if (prCmdInfo->pfCmdDoneHandler) {
		/* The destination buffer length has been checked sufficient */
		kalMemZero(prCmdInfo->pvInformationBuffer,
				prCmdInfo->u4InformationBufferLength);
		prCmdInfo->u4InformationBufferLength =
			prEvent->u2PacketLength - sizeof(struct WIFI_EVENT);
		DBGLOG(RX, TRACE, "Calling prCmdInfo->pfCmdDoneHandler=%ps",
				prCmdInfo->pfCmdDoneHandler);
		prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
					    prEvent->aucBuffer);
	} else if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo,
			prCmdInfo,
			prEvent->u2PacketLength - sizeof(struct WIFI_EVENT),
			WLAN_STATUS_SUCCESS);

	/* return prCmdInfo */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
}
#endif


void nicCmdEventQueryBugReport(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
#define BUG_REPORT_VERSION 1

	struct EVENT_BUG_REPORT *prStatistics;
	struct EVENT_BUG_REPORT *prEventStatistics;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (struct EVENT_BUG_REPORT *)
			    pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		u4QueryInfoLen = sizeof(struct EVENT_BUG_REPORT);
		if (prEventStatistics->u4BugReportVersion ==
		    BUG_REPORT_VERSION) {
			prStatistics = (struct EVENT_BUG_REPORT *)
				       prCmdInfo->pvInformationBuffer;
			kalMemCopy(prStatistics,
				prEventStatistics, u4QueryInfoLen);
		}

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicCmdEventEnterRfTest(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4Idx = 0;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	/* [driver-land] */
	 prAdapter->fgTestMode = TRUE;

	/* 0. always indicate disconnection */
	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		if (!wlanGetAisNetDev(prAdapter->prGlueInfo, u4Idx))
			continue;

		if (kalGetMediaStateIndicated(
			prAdapter->prGlueInfo,
			AIS_MAIN_BSS_INDEX(prAdapter, u4Idx)) !=
		    MEDIA_STATE_DISCONNECTED)
			kalIndicateStatusAndComplete(
				prAdapter->prGlueInfo,
				WLAN_STATUS_MEDIA_DISCONNECT,
				NULL, 0, AIS_MAIN_BSS_INDEX(prAdapter, u4Idx));
	}
	/* 1. Remove pending TX */
	nicTxRelease(prAdapter, TRUE);

	/* 1.1 clear pending Management Frames */
	kalClearMgmtFrames(prAdapter->prGlueInfo);

	/* 1.2 clear pending TX packet queued in glue layer */
	kalFlushPendingTxPackets(prAdapter->prGlueInfo);

	/* 2. Reset driver-domain FSMs */
	nicUninitMGMT(prAdapter);

	nicResetSystemService(prAdapter);
	nicInitMGMT(prAdapter, NULL);

	/* Block til firmware completed entering into RF test mode */
	kalMsleep(500);

#if defined(_HIF_SDIO) && 0
	/* 3. Disable Interrupt */
	HAL_INTR_DISABLE(prAdapter);

	/* 4. Block til firmware completed entering into RF test mode */
	kalMsleep(500);
	while (1) {
		uint32_t u4Value;

		HAL_MCR_RD(prAdapter, MCR_WCIR, &u4Value);

		if (u4Value & WCIR_WLAN_READY) {
			break;
		} else if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
			   || fgIsBusAccessFailed == TRUE) {
			if (prCmdInfo->fgIsOid) {
				/* Update Set Information Length */
				kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo,
					prCmdInfo->u4SetInfoLen,
					WLAN_STATUS_NOT_SUPPORTED);

			}
			return;
		}
		kalMsleep(10);
	}

	/* 5. Clear Interrupt Status */
	{
		uint32_t u4WHISR = 0;
		uint16_t au2TxCount[SDIO_TX_RESOURCE_NUM];

		HAL_READ_INTR_STATUS(prAdapter, 4, (uint8_t *)&u4WHISR);
		if (HAL_IS_TX_DONE_INTR(u4WHISR))
			HAL_READ_TX_RELEASED_COUNT(prAdapter, au2TxCount);
	}
	/* 6. Reset TX Counter */
	nicTxResetResource(prAdapter);

	/* 7. Re-enable Interrupt */
	HAL_INTR_ENABLE(prAdapter);
#endif

	/* 8. completion indication */
	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo, prCmdInfo->u4SetInfoLen,
			       WLAN_STATUS_SUCCESS);
	}
#if CFG_SUPPORT_NVRAM
	/* 9. load manufacture data */
	if (kalIsConfigurationExist(prAdapter->prGlueInfo) == TRUE)
		wlanLoadManufactureData(prAdapter,
			kalGetConfiguration(prAdapter->prGlueInfo));
	else
		DBGLOG(REQ, WARN, "%s: load manufacture data fail\n",
		       __func__);
#endif

}

void nicCmdEventLeaveRfTest(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4Idx = 0;
	struct WLAN_INFO *prWlanInfo;

	/* Block until firmware completed leaving from RF test mode */
	kalMsleep(500);

#if defined(_HIF_SDIO) && 0
	uint32_t u4WHISR = 0;
	uint16_t au2TxCount[SDIO_TX_RESOURCE_NUM];
	uint32_t u4Value;

	/* 1. Disable Interrupt */
	HAL_INTR_DISABLE(prAdapter);

	/* 2. Block until firmware completed leaving from RF test mode */
	kalMsleep(500);
	while (1) {
		HAL_MCR_RD(prAdapter, MCR_WCIR, &u4Value);

		if (u4Value & WCIR_WLAN_READY) {
			break;
		} else if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
			   || fgIsBusAccessFailed == TRUE) {
			if (prCmdInfo->fgIsOid) {
				/* Update Set Information Length */
				kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo,
					prCmdInfo->u4SetInfoLen,
					WLAN_STATUS_NOT_SUPPORTED);

			}
			return;
		}
		kalMsleep(10);
	}
	/* 3. Clear Interrupt Status */
	HAL_READ_INTR_STATUS(prAdapter, 4, (uint8_t *)&u4WHISR);
	if (HAL_IS_TX_DONE_INTR(u4WHISR))
		HAL_READ_TX_RELEASED_COUNT(prAdapter, au2TxCount);
	/* 4. Reset TX Counter */
	nicTxResetResource(prAdapter);

	/* 5. Re-enable Interrupt */
	HAL_INTR_ENABLE(prAdapter);
#endif

	/* 6. set driver-land variable */
	prAdapter->fgTestMode = FALSE;

	prWlanInfo = &prAdapter->rWlanInfo;
	/* 7. Indicate as disconnected */
	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		if (!wlanGetAisNetDev(prAdapter->prGlueInfo, u4Idx))
			continue;

		if (kalGetMediaStateIndicated(
			prAdapter->prGlueInfo,
			AIS_MAIN_BSS_INDEX(prAdapter, u4Idx)) !=
		    MEDIA_STATE_DISCONNECTED) {

			kalIndicateStatusAndComplete(
				prAdapter->prGlueInfo,
				WLAN_STATUS_MEDIA_DISCONNECT,
				NULL, 0, AIS_MAIN_BSS_INDEX(prAdapter, u4Idx));

			prWlanInfo->u4SysTime = kalGetTimeTick();
		}
	}
#if CFG_SUPPORT_NVRAM
	/* 8. load manufacture data */
	if (kalIsConfigurationExist(prAdapter->prGlueInfo) == TRUE)
		wlanLoadManufactureData(prAdapter,
			kalGetConfiguration(prAdapter->prGlueInfo));
	else
		DBGLOG(REQ, WARN, "%s: load manufacture data fail\n",
		       __func__);
#endif

	/* 9. Override network address */
	wlanUpdateNetworkAddress(prAdapter);

	/* 10. completion indication */
	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo, prCmdInfo->u4SetInfoLen,
			       WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventQueryMcastAddr(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct CMD_MAC_MCAST_ADDR *prEventMacMcastAddr;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventMacMcastAddr = (struct CMD_MAC_MCAST_ADDR *) (
					      pucEventBuf);

		u4QueryInfoLen = prEventMacMcastAddr->u4NumOfGroupAddr *
				 MAC_ADDR_LEN;

		/* buffer length check */
		if (prCmdInfo->u4InformationBufferLength < u4QueryInfoLen) {
			kalOidComplete(prGlueInfo, prCmdInfo,
				u4QueryInfoLen, WLAN_STATUS_BUFFER_TOO_SHORT);
		} else {
			kalMemCopy(prCmdInfo->pvInformationBuffer,
				prEventMacMcastAddr->arAddress,
				prEventMacMcastAddr->u4NumOfGroupAddr *
					MAC_ADDR_LEN);

			kalOidComplete(prGlueInfo, prCmdInfo,
				       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
		}
	}
}

void nicCmdEventQueryEepromRead(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct PARAM_CUSTOM_EEPROM_RW_STRUCT *prEepromRdInfo;
	struct GLUE_INFO *prGlueInfo;
	struct CMD_ACCESS_EEPROM *prEventAccessEeprom;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventAccessEeprom = (struct CMD_ACCESS_EEPROM *) (
					      pucEventBuf);

		u4QueryInfoLen = sizeof(struct
					PARAM_CUSTOM_EEPROM_RW_STRUCT);

		prEepromRdInfo = (struct PARAM_CUSTOM_EEPROM_RW_STRUCT *)
			prCmdInfo->pvInformationBuffer;
		prEepromRdInfo->info.rEeprom.ucEepromIndex = (uint8_t) (
			prEventAccessEeprom->u2Offset);
		prEepromRdInfo->info.rEeprom.u2EepromData =
			prEventAccessEeprom->u2Data;

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventSetStopSchedScan(struct ADAPTER
				 *prAdapter, struct CMD_INFO *prCmdInfo,
				 uint8_t *pucEventBuf)
{
	/*
	 *  DBGLOG(SCN, INFO, "--->nicCmdEventSetStopSchedScan\n" ));
	 */
	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	/*
	 *  DBGLOG(SCN, INFO, "<--kalSchedScanStopped\n" );
	 */
	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			prCmdInfo,
			prCmdInfo->u4InformationBufferLength,
			WLAN_STATUS_SUCCESS);
	}

	DBGLOG(SCN, INFO,
	       "nicCmdEventSetStopSchedScan OID done, release lock and send event to uplayer\n");
	/* Due to dead lock issue, need to release the IO
	 * control before calling kernel APIs
	 */
	kalSchedScanStopped(prAdapter->prGlueInfo,
			    !prCmdInfo->fgIsOid);

}

#if (CFG_SUPPORT_PKT_OFLD == 1)
void nicCmdEventQueryOfldInfo(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	uint32_t rOidStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_OFLD_INFO *prParamOfldInfo;
	struct CMD_OFLD_INFO *prCmdOfldInfo;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCmdOfldInfo = (struct CMD_OFLD_INFO *) (pucEventBuf);

		u4QueryInfoLen = sizeof(struct
					PARAM_OFLD_INFO);

		if (prCmdInfo->u4InformationBufferLength < sizeof(
			    struct PARAM_OFLD_INFO)) {
			DBGLOG(REQ, INFO,
			       "Ofld info query length %u is not valid.\n",
			       prCmdInfo->u4InformationBufferLength);
			rOidStatus = WLAN_STATUS_FAILURE;
		}
		prParamOfldInfo = (struct PARAM_OFLD_INFO
				    *) prCmdInfo->pvInformationBuffer;
		prParamOfldInfo->ucFragNum = prCmdOfldInfo->ucFragNum;
		prParamOfldInfo->ucFragSeq = prCmdOfldInfo->ucFragSeq;
		prParamOfldInfo->u4TotalLen = prCmdOfldInfo->u4TotalLen;
		prParamOfldInfo->u4BufLen = prCmdOfldInfo->u4BufLen;

		if (prCmdOfldInfo->u4TotalLen > 0 &&
				prCmdOfldInfo->u4BufLen > 0 &&
				prCmdOfldInfo->u4BufLen <= PKT_OFLD_BUF_SIZE) {
			kalMemCopy(prParamOfldInfo->aucBuf,
				prCmdOfldInfo->aucBuf,
				prCmdOfldInfo->u4BufLen);
		} else {
			DBGLOG(REQ, INFO,
			       "Invalid query result, length: %d Buf size: %d.\n",
				prCmdOfldInfo->u4TotalLen,
				prCmdOfldInfo->u4BufLen);
			rOidStatus = WLAN_STATUS_FAILURE;
		}
		kalOidComplete(prGlueInfo, prCmdInfo,
		   u4QueryInfoLen, rOidStatus);
	}

}
#endif /* CFG_SUPPORT_PKT_OFLD */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when command by OID/ioctl has been timeout
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
void nicOidCmdTimeoutCommon(struct ADAPTER *prAdapter,
			    struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       0, WLAN_STATUS_FAILURE);

	if (prAdapter->fgIsPostponeTxEAPOLM3)
		prAdapter->fgIsPostponeTxEAPOLM3 = FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is a generic command timeout handler
 *
 * @param pfnOidHandler      Pointer to the OID handler
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicCmdTimeoutCommon(struct ADAPTER *prAdapter,
			 struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when command for entering RF test has
 *        failed sending due to timeout (highly possibly by firmware crash)
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicOidCmdEnterRFTestTimeout(struct ADAPTER
				 *prAdapter, struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	/* 1. Remove pending TX frames */
	nicTxRelease(prAdapter, TRUE);

	/* 1.1 clear pending Management Frames */
	kalClearMgmtFrames(prAdapter->prGlueInfo);

	/* 1.2 clear pending TX packet queued in glue layer */
	kalFlushPendingTxPackets(prAdapter->prGlueInfo);

	/* 2. indicate for OID failure */
	kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
		       0, WLAN_STATUS_FAILURE);
}
#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
void nicCmdEventQueryMdnsStats(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct GLUE_INFO *prGlueInfo;
	uint16_t len;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	len = prCmdInfo->u4InformationBufferLength;
	prGlueInfo = prAdapter->prGlueInfo;

	DBGLOG(NIC, TRACE, "Glue=%p, Pend=%p, Cmd=%p, oid=%u, Buf=%p, len=%u",
			prGlueInfo, &prGlueInfo->rPendComp, prCmdInfo,
			prCmdInfo->fgIsOid, prCmdInfo->pvInformationBuffer,
			len);

	memcpy((uint8_t *)prCmdInfo->pvInformationBuffer, pucEventBuf, len);

	DBGLOG(RX, TRACE, "kalOidComplete: infoLen=%u", len);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo,
						len, WLAN_STATUS_SUCCESS);
}

void nicEventMdnsStats(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	if (unlikely(prEvent->u2PacketLength - sizeof(struct WIFI_EVENT) >
					prCmdInfo->u4InformationBufferLength)) {
		DBGLOG(RX, WARN, "prEventLen=%u-%u, BufLen=%u",
				prEvent->u2PacketLength,
				sizeof(struct WIFI_EVENT),
				prCmdInfo->u4InformationBufferLength);
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo, 0,
				WLAN_STATUS_FAILURE);
	} else if (prCmdInfo->pfCmdDoneHandler) {
		/* The destination buffer length has been checked sufficient */
		kalMemZero(prCmdInfo->pvInformationBuffer,
				prCmdInfo->u4InformationBufferLength);
		prCmdInfo->u4InformationBufferLength =
			prEvent->u2PacketLength - sizeof(struct WIFI_EVENT);
		DBGLOG(RX, TRACE, "Calling prCmdInfo->pfCmdDoneHandler=%ps",
				prCmdInfo->pfCmdDoneHandler);
		prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
					    prEvent->aucBuffer);
	} else if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo,
			prCmdInfo,
			prEvent->u2PacketLength - sizeof(struct WIFI_EVENT),
			WLAN_STATUS_SUCCESS);

	/* return prCmdInfo */
	cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
}
#endif
#endif

#if CFG_SUPPORT_BATCH_SCAN
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for SUPPORT_BATCH_SCAN
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventBatchScanResult(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct EVENT_BATCH_RESULT *prEventBatchResult;
	struct GLUE_INFO *prGlueInfo;

	DBGLOG(SCN, TRACE, "nicCmdEventBatchScanResult");

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventBatchResult = (struct EVENT_BATCH_RESULT *)
				     pucEventBuf;

		u4QueryInfoLen = sizeof(struct EVENT_BATCH_RESULT);
		kalMemCopy(prCmdInfo->pvInformationBuffer,
			prEventBatchResult, sizeof(struct EVENT_BATCH_RESULT));

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

void nicEventHifCtrlv2(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
#if defined(_HIF_USB)
	struct EVENT_HIF_CTRL *prEventHifCtrl;
	struct GL_HIF_INFO *prHifInfo;

	prEventHifCtrl = (struct EVENT_HIF_CTRL *) (
				 prEvent->aucBuffer);

	if (prEventHifCtrl->ucHifSuspend) {
		/* suspend event, change state to PRE_SUSPEND_DONE */
		if (prEventHifCtrl->ucHifTxTrafficStatus ==
		    ENUM_HIF_TRAFFIC_IDLE &&
		    prEventHifCtrl->ucHifRxTrafficStatus ==
		    ENUM_HIF_TRAFFIC_IDLE) {	/* success */
			halUSBPreSuspendDone(prAdapter,
				NULL, prEvent->aucBuffer);
		} else {
			/* busy */
			/* invalid */
			halUSBPreSuspendTimeout(prAdapter, NULL);
		}
	} else {
		/* if USB get resume event, change to LINK_UP */
		prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
		glUsbSetState(prHifInfo, USB_STATE_LINK_UP);
	}
#endif
}

void nicEventHifCtrl(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct EVENT_HIF_CTRL *prEventHifCtrl;
	struct BUS_INFO *prBusInfo;
#if defined(_HIF_SDIO)
	struct GL_HIF_INFO *prHifInfo;
#endif

	prBusInfo = prAdapter->chip_info->bus_info;
	prEventHifCtrl = (struct EVENT_HIF_CTRL *) (
				 prEvent->aucBuffer);

	DBGLOG(HAL, INFO, "%s: EVENT_ID_HIF_CTRL\n", __func__);
	DBGLOG(HAL, INFO, "prEventHifCtrl->ucHifType = %hhu suspend %d\n",
	       prEventHifCtrl->ucHifType, prEventHifCtrl->ucHifSuspend);
	DBGLOG(HAL, INFO,
	       "prEventHifCtrl->ucHifTxTrafficStatus, prEventHifCtrl->ucHifRxTrafficStatus = %hhu, %hhu\n",
	       prEventHifCtrl->ucHifTxTrafficStatus,
	       prEventHifCtrl->ucHifRxTrafficStatus);

#if defined(_HIF_USB)
	if (prBusInfo->u4SuspendVer == SUSPEND_V2) {
		/* Suspend V2, control state of HIF here */
		nicEventHifCtrlv2(prAdapter, prEvent);
	} else {
		if (prEventHifCtrl->ucHifTxTrafficStatus ==
		    ENUM_HIF_TRAFFIC_IDLE &&
		    prEventHifCtrl->ucHifRxTrafficStatus ==
		    ENUM_HIF_TRAFFIC_IDLE) {	/* success */
			halUSBPreSuspendDone(prAdapter,
				NULL, prEvent->aucBuffer);
		} else {
			/* busy */
			/* invalid */
			halUSBPreSuspendTimeout(prAdapter, NULL);
		}
	}
#endif

#if defined(_HIF_SDIO)
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	if (prEventHifCtrl->ucHifSuspend) {
		/* if SDIO get suspend event, change to PRE_SUSPEND_DONE */
		glSdioSetState(prHifInfo, SDIO_STATE_PRE_SUSPEND_DONE);
	} else {
		/* if SDIO get resume event, change to LINK_UP */
		glSdioSetState(prHifInfo, SDIO_STATE_LINK_UP);
	}
#endif

#if defined(_HIF_PCIE)
	/* if PCIE suspend, polling sequence */
	if (prEventHifCtrl->ucHifType == ENUM_HIF_TYPE_PCIE) {
		if (prEventHifCtrl->ucHifSuspend) {
			if (prEventHifCtrl->ucHifTxTrafficStatus ==
			  ENUM_HIF_TRAFFIC_IDLE &&
			  prEventHifCtrl->ucHifRxTrafficStatus ==
			  ENUM_HIF_TRAFFIC_IDLE) {
				/* success */
				halPciePreSuspendDone(
				  prAdapter, NULL,
				  prEvent->aucBuffer);
			} else {
				/* busy */
				/* invalid */
				halPciePreSuspendTimeout(prAdapter, NULL);
			}
		}  else {
			prAdapter->prGlueInfo->rHifInfo.eSuspendtate =
				PCIE_STATE_PRE_RESUME_DONE;
		}
	}
#endif

}

#if CFG_SUPPORT_BUILD_DATE_CODE
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for build date code information
 *        has been retrieved
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventBuildDateCode(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct EVENT_BUILD_DATE_CODE *prEvent;
	struct GLUE_INFO *prGlueInfo;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEvent = (struct EVENT_BUILD_DATE_CODE *) pucEventBuf;

		u4QueryInfoLen = sizeof(uint8_t) * 16;
		kalMemCopy(prCmdInfo->pvInformationBuffer,
			   prEvent->aucDateCode, sizeof(uint8_t) * 16);

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

void nicUpdateStaStats(struct ADAPTER *prAdapter,
	struct EVENT_STA_STATISTICS *prEvent,
	struct PARAM_GET_STA_STATISTICS *prStaStatistics,
	uint8_t ucStaRecIdx, bool fgIsMibDiff)
{
	enum ENUM_WMM_ACI eAci;
	struct STA_RECORD *prStaRec;
	uint8_t ucDbdcIdx, ucIdx;
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo;
#endif
	struct MIB_INFO_STAT *prEvtMib;
	struct MIB_INFO_STAT *prMibInfo;

	/* Statistics from FW is valid */
	if (prEvent->u4Flags & BIT(0)) {
		prStaStatistics->ucPer = prEvent->ucPer;
		prStaStatistics->ucRcpi = prEvent->ucRcpi;
		prStaStatistics->u4PhyMode = prEvent->u4PhyMode;
		prStaStatistics->u2LinkSpeed = prEvent->u2LinkSpeed;

		prStaStatistics->u4TxFailCount = prEvent->u4TxFailCount;
		prStaStatistics->u4TxLifeTimeoutCount =
			prEvent->u4TxLifeTimeoutCount;
		prStaStatistics->u4TransmitCount =
			prEvent->u4TransmitCount;
		prStaStatistics->u4TransmitFailCount =
			prEvent->u4TransmitFailCount;
		prStaStatistics->u4Rate1TxCnt = prEvent->u4Rate1TxCnt;
		prStaStatistics->u4Rate1FailCnt =
			prEvent->u4Rate1FailCnt;

		prStaStatistics->ucTemperature = prEvent->ucTemperature;
		prStaStatistics->ucSkipAr = prEvent->ucSkipAr;
		prStaStatistics->ucArTableIdx = prEvent->ucArTableIdx;
		prStaStatistics->ucRateEntryIdx =
			prEvent->ucRateEntryIdx;
		prStaStatistics->ucRateEntryIdxPrev =
			prEvent->ucRateEntryIdxPrev;
		prStaStatistics->ucTxSgiDetectPassCnt =
			prEvent->ucTxSgiDetectPassCnt;
		prStaStatistics->ucAvePer = prEvent->ucAvePer;
#if (CFG_SUPPORT_RA_GEN == 0)
		kalMemCopy(prStaStatistics->aucArRatePer,
			   prEvent->aucArRatePer,
			   sizeof(prEvent->aucArRatePer));
		kalMemCopy(prStaStatistics->aucRateEntryIndex,
			   prEvent->aucRateEntryIndex,
			   sizeof(prEvent->aucRateEntryIndex));
#else
		prStaStatistics->u4AggRangeCtrl_0 =
			prEvent->u4AggRangeCtrl_0;
		prStaStatistics->u4AggRangeCtrl_1 =
			prEvent->u4AggRangeCtrl_1;
		prStaStatistics->ucRangeType = prEvent->ucRangeType;
#if ((CFG_SUPPORT_CONNAC2X == 1) || (CFG_SUPPORT_CONNAC3X == 1))
		prStaStatistics->u4AggRangeCtrl_2 =
			prEvent->u4AggRangeCtrl_2;
		prStaStatistics->u4AggRangeCtrl_3 =
			prEvent->u4AggRangeCtrl_3;
#endif
#if (CFG_SUPPORT_CONNAC3X == 1)
		prStaStatistics->u4AggRangeCtrl_4 =
			prEvent->u4AggRangeCtrl_4;
		prStaStatistics->u4AggRangeCtrl_5 =
			prEvent->u4AggRangeCtrl_5;
		prStaStatistics->u4AggRangeCtrl_6 =
			prEvent->u4AggRangeCtrl_6;
		prStaStatistics->u4AggRangeCtrl_7 =
			prEvent->u4AggRangeCtrl_7;
#else
		kalMemCopy(prStaStatistics->aucReserved5,
			   prEvent->aucReserved5,
			   sizeof(prEvent->aucReserved5));
#endif
#endif
		prStaStatistics->ucArStateCurr = prEvent->ucArStateCurr;
		prStaStatistics->ucArStatePrev = prEvent->ucArStatePrev;
		prStaStatistics->ucArActionType =
			prEvent->ucArActionType;
		prStaStatistics->ucHighestRateCnt =
			prEvent->ucHighestRateCnt;
		prStaStatistics->ucLowestRateCnt =
			prEvent->ucLowestRateCnt;
		prStaStatistics->u2TrainUp = prEvent->u2TrainUp;
		prStaStatistics->u2TrainDown = prEvent->u2TrainDown;
		kalMemCopy(&prStaStatistics->rTxVector,
			&prEvent->rTxVector,
			sizeof(prEvent->rTxVector));
		kalMemCopy(&prStaStatistics->rMibInfo,
			&prEvent->rMibInfo,
			sizeof(prEvent->rMibInfo));
		for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
			prMibInfo = &g_arMibInfo[ucDbdcIdx];
			prEvtMib = &prStaStatistics->rMibInfo[ucDbdcIdx];
			if (fgIsMibDiff) {
				prMibInfo->u4RxMpduCnt += prEvtMib->u4RxMpduCnt;
				prMibInfo->u4FcsError += prEvtMib->u4FcsError;
				prMibInfo->u4RxFifoFull +=
					prEvtMib->u4RxFifoFull;
				prMibInfo->u4AmpduTxSfCnt +=
					prEvtMib->u4AmpduTxSfCnt;
				prMibInfo->u4AmpduTxAckSfCnt +=
					prEvtMib->u4AmpduTxAckSfCnt;
#if (CFG_SUPPORT_CONNAC3X == 1)
				for (ucIdx = 0; ucIdx <= AGG_RANGE_SEL_NUM;
					ucIdx++)
					prMibInfo->au4TxRangeAmpduCnt[ucIdx] +=
						prEvtMib->au4TxRangeAmpduCnt[
								ucIdx];
#else
				for (ucIdx = 0; ucIdx <= AGG_RANGE_SEL_NUM;
					ucIdx++)
					prMibInfo->au2TxRangeAmpduCnt[ucIdx] +=
						prEvtMib->au2TxRangeAmpduCnt[
								ucIdx];
#endif
			} else {
				prMibInfo->u4RxMpduCnt = prEvtMib->u4RxMpduCnt;
				prMibInfo->u4FcsError =
					prEvtMib->u4FcsError;
				prMibInfo->u4RxFifoFull =
					prEvtMib->u4RxFifoFull;
				prMibInfo->u4AmpduTxSfCnt =
					prEvtMib->u4AmpduTxSfCnt;
				prMibInfo->u4AmpduTxAckSfCnt =
					prEvtMib->u4AmpduTxAckSfCnt;
#if (CFG_SUPPORT_CONNAC3X == 1)
				for (ucIdx = 0; ucIdx <= AGG_RANGE_SEL_NUM;
					ucIdx++)
					prMibInfo->au4TxRangeAmpduCnt[ucIdx] =
						prEvtMib->au4TxRangeAmpduCnt[
								ucIdx];
#else
				for (ucIdx = 0; ucIdx <= AGG_RANGE_SEL_NUM;
					ucIdx++)
					prMibInfo->au2TxRangeAmpduCnt[ucIdx] =
						prEvtMib->au2TxRangeAmpduCnt[
								ucIdx];
#endif
			}
			DBGLOG_LIMITED(NIC, TRACE,
				"updateSta B%u diff:%u mpdu:%u/%u fcsE:%u/%u fifo:%u/%u\n",
				ucDbdcIdx,
				fgIsMibDiff,
				prMibInfo->u4RxMpduCnt,
				prEvtMib->u4RxMpduCnt,
				prMibInfo->u4FcsError,
				prEvtMib->u4FcsError,
				prMibInfo->u4RxFifoFull,
				prEvtMib->u4RxFifoFull);
		}

		prStaStatistics->fgIsForceTxStream =
			prEvent->fgIsForceTxStream;
		prStaStatistics->fgIsForceSeOff =
			prEvent->fgIsForceSeOff;
#if (CFG_SUPPORT_RA_GEN == 0)
		kalMemCopy(prStaStatistics->aucReserved6,
			   prEvent->aucReserved6,
			   sizeof(prEvent->aucReserved6));
#else
		prStaStatistics->u2RaRunningCnt =
			prEvent->u2RaRunningCnt;
		prStaStatistics->ucRaStatus = prEvent->ucRaStatus;
		prStaStatistics->ucFlag = prEvent->ucFlag;
		kalMemCopy(&prStaStatistics->aucTxQuality,
			   &prEvent->aucTxQuality,
			   sizeof(prEvent->aucTxQuality));
		prStaStatistics->ucTxRateUpPenalty =
			prEvent->ucTxRateUpPenalty;
		prStaStatistics->ucLowTrafficMode =
			prEvent->ucLowTrafficMode;
		prStaStatistics->ucLowTrafficCount =
			prEvent->ucLowTrafficCount;
		prStaStatistics->ucLowTrafficDashBoard =
			prEvent->ucLowTrafficDashBoard;
		prStaStatistics->ucDynamicSGIState =
			prEvent->ucDynamicSGIState;
		prStaStatistics->ucDynamicSGIScore =
			prEvent->ucDynamicSGIScore;
		prStaStatistics->ucDynamicBWState =
			prEvent->ucDynamicBWState;
		prStaStatistics->ucDynamicGband256QAMState =
			prEvent->ucDynamicGband256QAMState;
		prStaStatistics->ucVhtNonSpRateState =
			prEvent->ucVhtNonSpRateState;
#endif
		prStaRec = cnmGetStaRecByIndex(prAdapter,
					       ucStaRecIdx);

		if (prStaRec) {
			/*link layer statistics */
			for (eAci = 0; eAci < WMM_AC_INDEX_NUM;
				eAci++) {
				prStaStatistics->arLinkStatistics[eAci].
				u4TxFailMsdu =
					prEvent->arLinkStatistics[eAci].
					u4TxFailMsdu;
				prStaStatistics->arLinkStatistics[eAci].
				u4TxRetryMsdu =
					prEvent->arLinkStatistics[eAci].
					u4TxRetryMsdu;

				/*for dump bss statistics */
				prStaRec->arLinkStatistics[eAci].
				u4TxFailMsdu =
					prEvent->arLinkStatistics[eAci].
					u4TxFailMsdu;
				prStaRec->arLinkStatistics[eAci].
				u4TxRetryMsdu =
					prEvent->arLinkStatistics[eAci].
					u4TxRetryMsdu;
			}
#if CFG_SUPPORT_STA_INFO
			prStaStatistics->u4RxBmcCnt = prEvent->u4RxBmcCnt;
			prStaRec->u4RxBmcCnt = prEvent->u4RxBmcCnt;
#endif
		}
		if (prEvent->u4TxCount) {
			uint32_t u4TxDoneAirTimeMs =
				USEC_TO_MSEC(prEvent->u4TxDoneAirTime *
				32);

			prStaStatistics->u4TxAverageAirTime =
				(u4TxDoneAirTimeMs /
				prEvent->u4TxCount);
		} else {
			prStaStatistics->u4TxAverageAirTime = 0;
		}

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
#if CFG_SUPPORT_WFD
		/* dump statistics for WFD */
		if (prAdapter->rWifiVar
			.rWfdConfigureSettings.ucWfdEnable == 1) {
			uint32_t u4LinkScore = 0;
			uint32_t u4TotalError =
				prStaStatistics->u4TxFailCount +
				prStaStatistics->u4TxLifeTimeoutCount;
			uint32_t u4TxExceedThresholdCount =
				prStaStatistics->u4TxExceedThresholdCount;
			uint32_t u4TxTotalCount =
				prStaStatistics->u4TxTotalCount;

			/* Calcute Link Score
			 * u4LinkScore 10~100 ,
			 * ExceedThreshold ratio 0~90 only
			 * u4LinkScore 0~9
			 * Drop packet ratio 0~9 and
			 * all packets exceed threshold
			 */
			if (u4TxTotalCount) {
				if (u4TxExceedThresholdCount <= u4TxTotalCount)
					u4LinkScore = (90 -
						((u4TxExceedThresholdCount * 90)
						/ u4TxTotalCount));
				else
					u4LinkScore = 0;
			} else
				u4LinkScore = 90;

			u4LinkScore += 10;
			if (u4LinkScore == 10) {
				if (u4TotalError <= u4TxTotalCount)
					u4LinkScore = (10 -
						((u4TotalError * 10) /
						u4TxTotalCount));
				else
					u4LinkScore = 0;
			}

			if (u4LinkScore > 100)
				u4LinkScore = 100;

			prAdapter->rWifiVar.rWfdConfigureSettings.u4LinkScore
				= u4LinkScore;

			log_dbg(P2P, INFO,
				"[%u][%u] link_score=%u, rssi=%u, rate=%u, threshold_cnt=%u, fail_cnt=%u\n",
				prEvent->ucNetworkTypeIndex,
				ucStaRecIdx,
				u4LinkScore,
				prStaStatistics->ucRcpi,
				prStaStatistics->u2LinkSpeed,
				prStaStatistics->u4TxExceedThresholdCount,
				prStaStatistics->u4TxFailCount);
			log_dbg(P2P, INFO, "timeout_cnt=%u, apt=%u, aat=%u, total_cnt=%u\n",
				prStaStatistics->u4TxLifeTimeoutCount,
				prStaStatistics->u4TxAverageProcessTime,
				prStaStatistics->u4TxAverageAirTime,
				prStaStatistics->u4TxTotalCount);
		}
#endif
#endif
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
		if (prStaRec &&
			prStaRec->ucBssIndex == aisGetDefaultLinkBssIndex(
				prAdapter)) {
			/* only update linkQuality for default link bss */
			prLinkQualityInfo = &(prAdapter->rLinkQualityInfo);
			prLinkQualityInfo->u4CurTxRate = (
				prEvent->u2LinkSpeed * 5);
			DBGLOG(REQ, TRACE, "ucBssIndex=%u txRate:%u\n",
				prStaRec->ucBssIndex,
				prLinkQualityInfo->u4CurTxRate);
		}
#endif
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for query STA link status
 *        has been retrieved
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryStaStatistics(struct ADAPTER
				   *prAdapter, struct CMD_INFO *prCmdInfo,
				   uint8_t *pucEventBuf)
{
	uint8_t ucStaRecIdx;
	uint32_t u4QueryInfoLen;
	struct EVENT_STA_STATISTICS *prEvent;
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_GET_STA_STATISTICS *prStaStatistics;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	ASSERT(prCmdInfo->pvInformationBuffer);

	prGlueInfo = prAdapter->prGlueInfo;
	prEvent = (struct EVENT_STA_STATISTICS *) pucEventBuf;
	prStaStatistics = (struct PARAM_GET_STA_STATISTICS *)
			  prCmdInfo->pvInformationBuffer;

	u4QueryInfoLen = sizeof(struct PARAM_GET_STA_STATISTICS);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	ucStaRecIdx = secGetStaIdxByWlanIdx(prAdapter, prEvent->ucStaRecIdx);
#else
	ucStaRecIdx = prEvent->ucStaRecIdx;
#endif
	if (ucStaRecIdx < CFG_STA_REC_NUM)
		nicUpdateStaStats(prAdapter, prEvent, prStaStatistics,
			ucStaRecIdx, TRUE);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo,
			prCmdInfo,
			u4QueryInfoLen,
			WLAN_STATUS_SUCCESS);

}

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when event for query LTE safe channels
 *        has been retrieved
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo          Pointer to the command information
 * @param pucEventBuf        Pointer to the event buffer
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryLteSafeChn(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf)
{
	struct EVENT_LTE_SAFE_CHN *prEvent;
	struct PARAM_GET_CHN_INFO *prLteSafeChnInfo;
	uint8_t ucIdx = 0;

	if ((prAdapter == NULL) || (prCmdInfo == NULL) || (pucEventBuf == NULL)
			|| (prCmdInfo->pvInformationBuffer == NULL)) {
		ASSERT(FALSE);
		return;
	}

	prEvent = (struct EVENT_LTE_SAFE_CHN *) pucEventBuf;

	prLteSafeChnInfo = (struct PARAM_GET_CHN_INFO *)
			prCmdInfo->pvInformationBuffer;

	/* Statistics from FW is valid */
	if (prEvent->u4Flags & BIT(0)) {
		struct LTE_SAFE_CHN_INFO *prLteSafeChnList;

		prLteSafeChnList = &prLteSafeChnInfo->rLteSafeChnList;
		for (ucIdx = 0; ucIdx <
				ENUM_SAFE_CH_MASK_MAX_NUM;
				ucIdx++) {
			prLteSafeChnList->au4SafeChannelBitmask[ucIdx]
				= prEvent->rLteSafeChn.
					au4SafeChannelBitmask[ucIdx];

			DBGLOG(NIC, INFO,
				"[ACS]LTE safe channels[%d]=0x%08x\n",
				ucIdx,
				prLteSafeChnList->au4SafeChannelBitmask[ucIdx]);
		}
	} else {
		DBGLOG(NIC, ERROR, "FW's event is NOT valid.\n");
	}

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			sizeof(struct PARAM_GET_CHN_INFO), WLAN_STATUS_SUCCESS);
}
#endif

void nicEventRddPulseDump(struct ADAPTER *prAdapter,
			  uint8_t *pucEventBuf)
{
	uint16_t u2Idx, u2PulseCnt = 0;
	struct EVENT_WIFI_RDD_TEST *prRddPulseEvent;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	prRddPulseEvent = (struct EVENT_WIFI_RDD_TEST *) (
				  pucEventBuf);

	if (prRddPulseEvent->u4FuncLength >
		(RX_GET_PACKET_MAX_SIZE(prAdapter)
			- OFFSET_OF(struct WIFI_EVENT, aucBuffer))) {
		DBGLOG(INIT, ERROR,
			"u4FuncLength %u out of valid event length!\n",
			prRddPulseEvent->u4FuncLength);
		return;
	}

	/* underflow check */
	if (prRddPulseEvent->u4FuncLength >= RDD_EVENT_HDR_SIZE) {
		u2PulseCnt = (prRddPulseEvent->u4FuncLength -
			RDD_EVENT_HDR_SIZE) / RDD_ONEPLUSE_SIZE;
	}

	DBGLOG(INIT, INFO, "[RDD]0x%08x %08d[RDD%d]\n",
	       prRddPulseEvent->u4Prefix
	       , prRddPulseEvent->u4Count, prRddPulseEvent->ucRddIdx);

	for (u2Idx = 0; u2Idx < u2PulseCnt; u2Idx++) {
		DBGLOG(INIT, INFO,
			"[RDD]0x%02x%02x%02x%02x %02x%02x%02x%02x[RDD%d]\n"
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET3]
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET2]
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET1]
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET0]
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET7]
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET6]
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET5]
		  , prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE * u2Idx +
			  RDD_PULSE_OFFSET4]
		  , prRddPulseEvent->ucRddIdx
		  );
	}

}

#if CFG_SUPPORT_MSP
void nicCmdEventQueryWlanInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct PARAM_HW_WLAN_INFO *prWlanInfo;
	struct EVENT_WLAN_INFO *prEventWlanInfo;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;
#if CFG_AP_80211KVR_INTERFACE
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	struct STA_RECORD *prStaRec;
#endif

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventWlanInfo = (struct EVENT_WLAN_INFO *) pucEventBuf;

	DBGLOG(RSN, INFO, "MT6632 : nicCmdEventQueryWlanInfo\n");

	prGlueInfo = prAdapter->prGlueInfo;

	u4QueryInfoLen = sizeof(struct PARAM_HW_WLAN_INFO);
	prWlanInfo = (struct PARAM_HW_WLAN_INFO *)
			     prCmdInfo->pvInformationBuffer;

	/* prWlanInfo->u4Length = sizeof(PARAM_HW_WLAN_INFO_T); */
	if (prEventWlanInfo && prWlanInfo) {
		kalMemCopy(&prWlanInfo->rWtblTxConfig,
				   &prEventWlanInfo->rWtblTxConfig,
				   sizeof(struct PARAM_TX_CONFIG));
		kalMemCopy(&prWlanInfo->rWtblSecConfig,
				   &prEventWlanInfo->rWtblSecConfig,
				   sizeof(struct PARAM_SEC_CONFIG));
		kalMemCopy(&prWlanInfo->rWtblKeyConfig,
				   &prEventWlanInfo->rWtblKeyConfig,
				   sizeof(struct PARAM_KEY_CONFIG));
		kalMemCopy(&prWlanInfo->rWtblRateInfo,
				   &prEventWlanInfo->rWtblRateInfo,
				   sizeof(struct PARAM_PEER_RATE_INFO));
		kalMemCopy(&prWlanInfo->rWtblBaConfig,
				   &prEventWlanInfo->rWtblBaConfig,
				   sizeof(struct PARAM_PEER_BA_CONFIG));
		kalMemCopy(&prWlanInfo->rWtblPeerCap,
				   &prEventWlanInfo->rWtblPeerCap,
				   sizeof(struct PARAM_PEER_CAP));
		kalMemCopy(&prWlanInfo->rWtblRxCounter,
				   &prEventWlanInfo->rWtblRxCounter,
				   sizeof(struct PARAM_PEER_RX_COUNTER_ALL));
		kalMemCopy(&prWlanInfo->rWtblTxCounter,
				   &prEventWlanInfo->rWtblTxCounter,
				   sizeof(struct PARAM_PEER_TX_COUNTER_ALL));
	}

	if (prCmdInfo->fgIsOid) {
#if CFG_AP_80211KVR_INTERFACE
		if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			prGlueInfo->prP2PInfo[1]->prDevHandler,
			&ucRoleIdx) == 0) {
			if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
				ucRoleIdx,
				&ucBssIdx) == WLAN_STATUS_SUCCESS) {
				prStaRec = cnmGetStaRecByAddress(prAdapter,
					ucBssIdx,
					prWlanInfo->rWtblTxConfig.aucPA);
				if (prStaRec)
					prStaRec->u8GetDataRateTime =
					kalGetTimeTick();
			} else
				DBGLOG(INIT, ERROR, "get ucBssIdx fail\n");
		} else
			DBGLOG(INIT, ERROR, "get ucRoleIdx fail\n");
#endif
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
void nicCmdEventQueryMibInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct PARAM_HW_MIB_INFO *prParamMibInfo;
	struct EVENT_MIB_INFO *prEventMibInfo;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	prEventMibInfo = (struct EVENT_MIB_INFO *) pucEventBuf;

	DBGLOG(RSN, INFO, "unified cmd: %s\n", __func__);

	prGlueInfo = prAdapter->prGlueInfo;

	u4QueryInfoLen = sizeof(struct PARAM_HW_MIB_INFO);
	prParamMibInfo = (struct PARAM_HW_MIB_INFO *)
			    prCmdInfo->pvInformationBuffer;
	if (prEventMibInfo && prParamMibInfo) {
		kalMemCopy(&prParamMibInfo->arMibInfo,
			   &prEventMibInfo->arMibInfo,
			   sizeof(struct MIB_INFO) * MAX_MIB_TAG_CNT);
	}

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}
#else
void nicCmdEventQueryMibInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct PARAM_HW_MIB_INFO *prMibInfo;
	struct EVENT_MIB_INFO *prEventMibInfo;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventMibInfo = (struct EVENT_MIB_INFO *) pucEventBuf;

	DBGLOG(RSN, INFO, "MT6632 : nicCmdEventQueryMibInfo\n");

	prGlueInfo = prAdapter->prGlueInfo;

	u4QueryInfoLen = sizeof(struct PARAM_HW_MIB_INFO);
	prMibInfo = (struct PARAM_HW_MIB_INFO *)
			    prCmdInfo->pvInformationBuffer;
	if (prEventMibInfo && prMibInfo) {
		kalMemCopy(&prMibInfo->rHwMibCnt,
			   &prEventMibInfo->rHwMibCnt,
			   sizeof(struct HW_MIB_COUNTER));
		kalMemCopy(&prMibInfo->rHwMib2Cnt,
			   &prEventMibInfo->rHwMib2Cnt,
			   sizeof(struct HW_MIB2_COUNTER));
		kalMemCopy(&prMibInfo->rHwTxAmpduMts,
			   &prEventMibInfo->rHwTxAmpduMts,
			   sizeof(struct HW_TX_AMPDU_METRICS));
	}

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}
#endif
#endif

uint32_t nicEventQueryTxResourceEntry(struct ADAPTER *prAdapter,
				      uint8_t *pucEventBuf)
{
	struct NIC_TX_RESOURCE *prTxResource;
	uint32_t version = *((uint32_t *)pucEventBuf);


	prAdapter->fgIsNicTxReousrceValid = TRUE;

	if (version & NIC_TX_RESOURCE_REPORT_VERSION_PREFIX)
		return nicEventQueryTxResource(prAdapter, pucEventBuf);

	/* 6632, 7668 ways */
	prAdapter->nicTxReousrce.txResourceInit = NULL;

	prTxResource = (struct NIC_TX_RESOURCE *)pucEventBuf;
	prAdapter->nicTxReousrce.u4CmdTotalResource =
		prTxResource->u4CmdTotalResource;
	prAdapter->nicTxReousrce.u4CmdResourceUnit =
		prTxResource->u4CmdResourceUnit;
	prAdapter->nicTxReousrce.u4DataTotalResource =
		prTxResource->u4DataTotalResource;
	prAdapter->nicTxReousrce.u4DataResourceUnit =
		prTxResource->u4DataResourceUnit;

	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicTxResource: u4CmdTotalResource = %x\n",
	       prAdapter->nicTxReousrce.u4CmdTotalResource);
	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicTxResource: u4CmdResourceUnit = %x\n",
	       prAdapter->nicTxReousrce.u4CmdResourceUnit);
	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicTxResource: u4DataTotalResource = %x\n",
	       prAdapter->nicTxReousrce.u4DataTotalResource);
	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicTxResource: u4DataResourceUnit = %x\n",
	       prAdapter->nicTxReousrce.u4DataResourceUnit);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCmdEventQueryNicEfuseAddr(struct ADAPTER *prAdapter,
				      uint8_t *pucEventBuf)
{
	struct NIC_EFUSE_ADDRESS *prTxResource =
		(struct NIC_EFUSE_ADDRESS *)pucEventBuf;

	prAdapter->u4EfuseStartAddress =
		prTxResource->u4EfuseStartAddress;
	prAdapter->u4EfuseEndAddress =
		prTxResource->u4EfuseEndAddress;

	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicEfuseAddr: u4EfuseStartAddress = %x\n",
	       prAdapter->u4EfuseStartAddress);
	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicEfuseAddr: u4EfuseEndAddress = %x\n",
	       prAdapter->u4EfuseEndAddress);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCmdEventQueryNicCoexFeature(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
	struct NIC_COEX_FEATURE *prCoexFeature =
		(struct NIC_COEX_FEATURE *)pucEventBuf;

	prAdapter->u4FddMode = prCoexFeature->u4FddMode;

	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicCoexFeature: u4FddMode = %x\n",
	       prAdapter->u4FddMode);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_TCP_IP_CHKSUM_OFFLOAD
uint32_t nicCmdEventQueryNicCsumOffload(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
	struct NIC_CSUM_OFFLOAD *prChecksumOffload =
		(struct NIC_CSUM_OFFLOAD *)pucEventBuf;

	prAdapter->fgIsSupportCsumOffload =
		prChecksumOffload->ucIsSupportCsumOffload;

	DBGLOG(INIT, INFO,
	       "nicCmdEventQueryNicCsumOffload: ucIsSupportCsumOffload = %x\n",
	       prAdapter->fgIsSupportCsumOffload);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicCfgChipCapHwVersion(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CAP_HW_VERSION *prHwVer =
		(struct CAP_HW_VERSION *)pucEventBuf;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);

	prAdapter->rVerInfo.u2FwProductID = prHwVer->u2ProductID;
	prChipInfo->u4ChipIpConfig = prHwVer->u4ConfigId;
	prChipInfo->u4ChipIpVersion = prHwVer->u4TopIpID;

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipAdieHwVersion(struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CAP_HW_ADIE_VERSION *prAdieHwVer =
		(struct CAP_HW_ADIE_VERSION *)pucEventBuf;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);

	prChipInfo->u2ADieChipVersion = prAdieHwVer->u2ProductID;
	DBGLOG(INIT, INFO, "A DieID = 0x%x\n", prAdieHwVer->u2ProductID);
	return WLAN_STATUS_SUCCESS;
}


uint32_t nicCfgChipCapSwVersion(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf)
{
	struct CAP_SW_VERSION *prSwVer =
		(struct CAP_SW_VERSION *)pucEventBuf;

	prAdapter->rVerInfo.u2FwOwnVersion = prSwVer->u2FwVersion;
	prAdapter->rVerInfo.ucFwBuildNumber =
		prSwVer->u2FwBuildNumber;
	kalMemCopy(prAdapter->rVerInfo.aucFwBranchInfo,
		   prSwVer->aucBranchInfo, 4);
	kalMemCopy(prAdapter->rVerInfo.aucFwDateCode,
		   prSwVer->aucDateCode, 16);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapMacAddr(struct ADAPTER *prAdapter,
			      uint8_t *pucEventBuf)
{
	struct CAP_MAC_ADDR *prMacAddr = (struct CAP_MAC_ADDR *)pucEventBuf;
	uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;

	COPY_MAC_ADDR(prAdapter->rWifiVar.aucPermanentAddress,
		      prMacAddr->aucMacAddr);
	COPY_MAC_ADDR(prAdapter->rWifiVar.aucMacAddress,
		      prMacAddr->aucMacAddr);
	prAdapter->fgIsEmbbededMacAddrValid = (u_int8_t) (
			!IS_BMCAST_MAC_ADDR(prMacAddr->aucMacAddr) &&
			!EQUAL_MAC_ADDR(aucZeroMacAddr, prMacAddr->aucMacAddr));

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapPhyCap(struct ADAPTER *prAdapter,
			     uint8_t *pucEventBuf)
{
	struct CAP_PHY_CAP *prPhyCap = (struct CAP_PHY_CAP *)pucEventBuf;
	uint8_t aucCapMaxBw[CAP_PHY_MAX_BW_NUM] = {
		MAX_BW_20MHZ,
		MAX_BW_40MHZ,
		MAX_BW_80MHZ,
		MAX_BW_160MHZ,
		MAX_BW_80_80_MHZ,
		MAX_BW_320_2MHZ,
	};
	uint8_t ucMaxBandwidth = MAX_BW_UNKNOWN;

	prAdapter->rWifiVar.ucStaVht &= prPhyCap->ucVht;
	wlanCfgSetUint32(prAdapter, "StaVHT", prAdapter->rWifiVar.ucStaVht);
	if (prAdapter->rWifiVar.ucApVht != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucApVht &= prPhyCap->ucVht;
		wlanCfgSetUint32(prAdapter, "ApVHT",
					prAdapter->rWifiVar.ucApVht);
	}
	prAdapter->rWifiVar.ucP2pGoVht &= prPhyCap->ucVht;
	wlanCfgSetUint32(prAdapter, "P2pGoVHT", prAdapter->rWifiVar.ucP2pGoVht);
	prAdapter->rWifiVar.ucP2pGcVht &= prPhyCap->ucVht;
	wlanCfgSetUint32(prAdapter, "P2pGcVHT", prAdapter->rWifiVar.ucP2pGcVht);
	prAdapter->fgIsHw5GBandDisabled = !prPhyCap->uc5gBand;
	prAdapter->rWifiVar.ucNSS = (prPhyCap->ucNss >
		prAdapter->rWifiVar.ucNSS) ?
		(prAdapter->rWifiVar.ucNSS):(prPhyCap->ucNss);
	wlanCfgSetUint32(prAdapter, "Nss", prAdapter->rWifiVar.ucNSS);
#if CFG_SUPPORT_DBDC
	if (!prPhyCap->ucDbdc) {
		prAdapter->rWifiVar.eDbdcMode = ENUM_DBDC_MODE_DISABLED;
		wlanCfgSetUint32(prAdapter, "DbdcMode",
					prAdapter->rWifiVar.eDbdcMode);
	} else if (prPhyCap->ucWifiPath
		== (WLAN_FLAG_2G4_WF0 | WLAN_FLAG_5G_WF1)) {
		prAdapter->rWifiVar.eDbdcMode = ENUM_DBDC_MODE_STATIC;
		wlanCfgSetUint32(prAdapter, "DbdcMode",
					prAdapter->rWifiVar.eDbdcMode);
	}
#endif
	prAdapter->rWifiVar.ucTxLdpc &= prPhyCap->ucTxLdpc;
	wlanCfgSetUint32(prAdapter, "LdpcTx", prAdapter->rWifiVar.ucTxLdpc);
	prAdapter->rWifiVar.ucRxLdpc &= prPhyCap->ucRxLdpc;
	wlanCfgSetUint32(prAdapter, "LdpcRx", prAdapter->rWifiVar.ucRxLdpc);
	prAdapter->rWifiVar.ucTxStbc &= prPhyCap->ucTxStbc;
	wlanCfgSetUint32(prAdapter, "StbcTx", prAdapter->rWifiVar.ucTxStbc);
	prAdapter->rWifiVar.ucRxStbc &= prPhyCap->ucRxStbc;
	wlanCfgSetUint32(prAdapter, "StbcRx", prAdapter->rWifiVar.ucRxStbc);
	prAdapter->rWifiVar.u4PhyMaxBandwidth = prPhyCap->ucMaxBandwidth;

	if ((prPhyCap->ucWifiPath != 0xF) && (prPhyCap->ucWifiPath != 0x3)) {
		/* May be legal settings in the future */
		DBGLOG(INIT, ERROR, "illegle ucWifiPath %x\n",
			prPhyCap->ucWifiPath);
	} else { /* 0xF = 2*2, 0x3 = 1*1 */
		 DBGLOG(INIT, TRACE, "ucWifiPath %x\n",
			prPhyCap->ucWifiPath);
	}
	/* Overwirte driver default setting here */
	prAdapter->rWifiFemCfg.u2WifiPath = prPhyCap->ucWifiPath;
	prAdapter->rWifiVar.ucHwNotSupportAC = (prPhyCap->ucVht) ? FALSE:TRUE;

	if (!prPhyCap->ucVht) {
#if CFG_SUPPORT_MTK_SYNERGY
		prAdapter->rWifiVar.ucGbandProbe256QAM = FEATURE_DISABLED;
		wlanCfgSetUint32(prAdapter, "Probe256QAM",
			prAdapter->rWifiVar.ucGbandProbe256QAM);
#endif
#if CFG_SUPPORT_VHT_IE_IN_2G
		prAdapter->rWifiVar.ucVhtIeIn2g = FEATURE_DISABLED;
		wlanCfgSetUint32(prAdapter, "VhtIeIn2G",
			prAdapter->rWifiVar.ucVhtIeIn2g);
#endif
	}

#if (CFG_SUPPORT_802_11AX == 1)
	if (prAdapter->rWifiVar.ucStaHe != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucStaHe &= prPhyCap->ucHe;
		wlanCfgSetUint32(prAdapter, "StaHE",
			prAdapter->rWifiVar.ucStaHe);
	}
	if (prAdapter->rWifiVar.ucApHe != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucApHe &= prPhyCap->ucHe;
		wlanCfgSetUint32(prAdapter, "ApHE",
			prAdapter->rWifiVar.ucApHe);
	}
	if (prAdapter->rWifiVar.ucP2pGoHe != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucP2pGoHe &= prPhyCap->ucHe;
		wlanCfgSetUint32(prAdapter, "P2pGoHE",
			prAdapter->rWifiVar.ucP2pGoHe);
	}
	if (prAdapter->rWifiVar.ucP2pGcHe != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucP2pGcHe &= prPhyCap->ucHe;
		wlanCfgSetUint32(prAdapter, "P2pGcHE",
			prAdapter->rWifiVar.ucP2pGcHe);
	}

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucStaHe)) {
		fgEfuseCtrlAxOn = 1; /* default is 1 */
	} else if (prAdapter->rWifiVar.u4SwTestMode !=
		ENUM_SW_TEST_MODE_SIGMA_AX_AP) {
		fgEfuseCtrlAxOn = 0;
	}
	DBGLOG(INIT, TRACE, "fgEfuseCtrlAxOn = %u\n", fgEfuseCtrlAxOn);

#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (prAdapter->rWifiVar.ucStaEht != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucStaEht &= prPhyCap->ucEht;
		wlanCfgSetUint32(prAdapter, "StaEHT",
			prAdapter->rWifiVar.ucStaEht);
	}
	if (prAdapter->rWifiVar.ucApEht != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucApEht &= prPhyCap->ucEht;
		wlanCfgSetUint32(prAdapter, "ApEHT",
			prAdapter->rWifiVar.ucApEht);
	}
	if (prAdapter->rWifiVar.ucP2pGoEht != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucP2pGoEht &= prPhyCap->ucEht;
		wlanCfgSetUint32(prAdapter, "P2pGoEHT",
			prAdapter->rWifiVar.ucP2pGoEht);
	}
	if (prAdapter->rWifiVar.ucP2pGcEht != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucP2pGcEht &= prPhyCap->ucEht;
		wlanCfgSetUint32(prAdapter, "P2pGcEHT",
			prAdapter->rWifiVar.ucP2pGcEht);
	}
	if (prAdapter->rWifiVar.ucEnableMlo != FEATURE_FORCE_ENABLED) {
		prAdapter->rWifiVar.ucEnableMlo &= prPhyCap->ucEht;
		wlanCfgSetUint32(prAdapter, "EnableMlo",
			prAdapter->rWifiVar.ucEnableMlo);
	}
#endif
	/* Overwrite bandwidth settings by phy capability */
	if (prPhyCap->ucMaxBandwidth < CAP_PHY_MAX_BW_NUM)
		ucMaxBandwidth = aucCapMaxBw[prPhyCap->ucMaxBandwidth];

	if (prAdapter->rWifiVar.ucStaBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucStaBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "StaBw",
			prAdapter->rWifiVar.ucStaBandwidth);
	}

	if (prAdapter->rWifiVar.ucSta5gBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucSta5gBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "Sta5gBw",
			prAdapter->rWifiVar.ucSta5gBandwidth);
	}

	if (prAdapter->rWifiVar.ucApBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucApBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "ApBw",
			prAdapter->rWifiVar.ucApBandwidth);
	}

	if (prAdapter->rWifiVar.ucAp5gBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucAp5gBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "Ap5gBw",
			prAdapter->rWifiVar.ucAp5gBandwidth);
	}

	if (prAdapter->rWifiVar.ucP2p5gBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucP2p5gBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "P2p5gBw",
			prAdapter->rWifiVar.ucP2p5gBandwidth);
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prAdapter->rWifiVar.ucSta6gBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucSta6gBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "Sta6gBw",
			prAdapter->rWifiVar.ucSta6gBandwidth);
	}

	if (prAdapter->rWifiVar.ucAp6gBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucAp6gBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "Ap6gBw",
			prAdapter->rWifiVar.ucAp6gBandwidth);
	}

	if (prAdapter->rWifiVar.ucP2p6gBandwidth > ucMaxBandwidth) {
		prAdapter->rWifiVar.ucP2p6gBandwidth = ucMaxBandwidth;
		wlanCfgSetUint32(prAdapter, "P2p6gBw",
			prAdapter->rWifiVar.ucP2p6gBandwidth);
	}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
	DBGLOG(INIT, INFO,
		"Vht [%u] He[%u] Eht[%u] 5gBand [%d], Nss [%d], Dbdc [%d], bw [%d=>%d]\n",
			prPhyCap->ucVht,
			prPhyCap->ucHe,
			prPhyCap->ucEht,
			prPhyCap->uc5gBand,
			prPhyCap->ucNss,
			prPhyCap->ucDbdc,
			prPhyCap->ucMaxBandwidth,
			ucMaxBandwidth);
#else
	DBGLOG(INIT, INFO,
		"Vht [%u] He[%u] Eht[%u] 5gBand [%d], Nss [%d], Dbdc [%d], bw [%d=>%d]\n",
			prPhyCap->ucVht,
			prPhyCap->ucHe,
			0,
			prPhyCap->uc5gBand,
			prPhyCap->ucNss,
			prPhyCap->ucDbdc,
			prPhyCap->ucMaxBandwidth,
			ucMaxBandwidth);
#endif

	DBGLOG(INIT, INFO,
		"TxLdpc [%u], RxLdpc [%u], StbcTx [%u], StbcRx [%u], WifiPath [%x]\n",
			prPhyCap->ucTxLdpc,
			prPhyCap->ucRxLdpc,
			prPhyCap->ucTxStbc,
			prPhyCap->ucRxStbc,
			prPhyCap->ucWifiPath);


	return WLAN_STATUS_SUCCESS;
}

#if (CFG_SUPPORT_802_11AX == 1)
uint8_t heMcsToMcsMap(uint8_t ucMcs)
{
	switch (ucMcs) {
	case 7:
		return HE_CAP_INFO_MCS_MAP_MCS7;
	case 9:
		return HE_CAP_INFO_MCS_MAP_MCS9;
	case 11:
		return HE_CAP_INFO_MCS_MAP_MCS11;
	default:
		return HE_CAP_INFO_MCS_NOT_SUPPORTED;
	}
}
#endif

uint32_t nicCfgChipCapLimited(struct ADAPTER *prAdapter,
				 uint8_t *pucEventBuf)
{
	struct CAP_LIMITED *prCapLimited;

	prCapLimited = (struct CAP_LIMITED *)pucEventBuf;

	DBGLOG(INIT, INFO,
		"Limited max MCS map from FW: [2G][%u],[5G][%u],[6G][%u]\n",
		prCapLimited->ucLimitedMaxMcsMap2g,
		prCapLimited->ucLimitedMaxMcsMap5g,
		prCapLimited->ucLimitedMaxMcsMap6g);

#if (CFG_SUPPORT_802_11AX == 1)
	prAdapter->rWifiVar.ucHeMaxMcsMap2g = kal_min_t(uint8_t,
			prCapLimited->ucLimitedMaxMcsMap2g,
			prAdapter->rWifiVar.ucHeMaxMcsMap2g);

	prAdapter->rWifiVar.ucHeMaxMcsMap5g = kal_min_t(uint8_t,
			prCapLimited->ucLimitedMaxMcsMap5g,
			prAdapter->rWifiVar.ucHeMaxMcsMap5g);

	prAdapter->rWifiVar.ucHeMaxMcsMap6g = kal_min_t(uint8_t,
			prCapLimited->ucLimitedMaxMcsMap6g,
			prAdapter->rWifiVar.ucHeMaxMcsMap6g);

	DBGLOG(INIT, INFO,
		"Limited max MCS map: [2G][%u],[5G][%u],[6G][%u]\n",
		prAdapter->rWifiVar.ucHeMaxMcsMap2g,
		prAdapter->rWifiVar.ucHeMaxMcsMap5g,
		prAdapter->rWifiVar.ucHeMaxMcsMap6g);
#endif
	return WLAN_STATUS_SUCCESS;
}

#if (CFG_SUPPORT_P2PGO_ACS == 1)
uint32_t nicCfgChipP2PCap(struct ADAPTER *prAdapter,
					 uint8_t *pucEventBuf)
{
#if 0
	struct CAP_PHY_CAP *prPhyCap =
	(struct CAP_PHY_CAP *)pucEventBuf;
	prAdapter->rWifiVar.ucStaVht &= prPhyCap->ucVht;
#endif
	wlanCfgSetUint32(prAdapter, "P2pGoACSEnable",
		FEATURE_ENABLED);
	DBGLOG(INIT, INFO, "P2pGoACSEnable:ACS Enable[%d]\n",
		FEATURE_ENABLED);
	return WLAN_STATUS_SUCCESS;
	}
#endif

#if (CFG_SUPPORT_RX_QUOTA_INFO == 1)
uint32_t nicCfgChipPseRxQuota(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf)
{
	struct CAP_PSE_RX_QUOTA *prPseCap =
	(struct CAP_PSE_RX_QUOTA *)pucEventBuf;
	uint8_t ucMaxBand = prAdapter->rWifiVar.ucNSS;
	uint32_t u4MaxQuotaBytes = 0;
	uint32_t u4MaxPktSize = 0;

	if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.ucRxQuotaInfoEn)) {
		DBGLOG(INIT, INFO, "RxQuotaInfoEn disabled.");
		return WLAN_STATUS_SUCCESS;
	}

	if (prPseCap->u4MaxQuotaBytes < RX_QUOTA_MAGIC_NUM) {
		DBGLOG(INIT, ERROR, "invalid u4MaxQuotaBytes:%d\n",
			prPseCap->u4MaxQuotaBytes);
		return WLAN_STATUS_SUCCESS;
	}

	u4MaxQuotaBytes = prPseCap->u4MaxQuotaBytes - RX_QUOTA_MAGIC_NUM;
	u4MaxPktSize = u4MaxQuotaBytes/ucMaxBand;
	if (u4MaxPktSize < 3000) {
		/* disable AMSDU */
		prAdapter->rWifiVar.ucAmsduInAmpduRx = FEATURE_DISABLED;
		DBGLOG(INIT, INFO, "Disable AMSDU\n");
	} else if (u4MaxPktSize < 7000) {
		/* MAX RX MPDU len = 3K */
		prAdapter->rWifiVar.ucRxMaxMpduLen = 0;
	} else if (u4MaxPktSize < 11000) {
		/* MAX RX MPDU len = 7K */
		prAdapter->rWifiVar.ucRxMaxMpduLen = 1;
	} else {
		/* MAX RX MPDU len = 11K */
		prAdapter->rWifiVar.ucRxMaxMpduLen = 2;
	}
	DBGLOG(INIT, INFO,
		"u4MaxQuotaBytes:%d u4MaxPktSize:%d ucRxMaxMpduLen:%d\n",
		prPseCap->u4MaxQuotaBytes, u4MaxPktSize,
		prAdapter->rWifiVar.ucRxMaxMpduLen);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicCmdEventHostStatusEmiOffset(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
	struct NIC_HOST_STATUS_EMI_OFFSET *prOffset =
		(struct NIC_HOST_STATUS_EMI_OFFSET *)pucEventBuf;
	struct HOST_SUSPEND_NOTIFY_INFO *prNotifyInfo =
		&prAdapter->rHostSuspendInfo;

	prNotifyInfo->eType = ENUM_HOST_SUSPEND_ADDR_TYPE_EMI;
	prNotifyInfo->u4SetAddr = prOffset->u4EmiOffset;
	prNotifyInfo->u4ClrAddr = prOffset->u4EmiOffset;
	prNotifyInfo->u4Mask = 0xFFFFFFFF;
	prNotifyInfo->u4Shift = 0;

	DBGLOG(INIT, INFO, "EMI offset= 0x%x\n",
		prNotifyInfo->u4SetAddr);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_LLS
uint32_t nicCmdEventLinkStatsEmiOffset(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
#if CFG_MTK_ANDROID_EMI
	struct CAP_LLS_DATA_EMI_OFFSET *prOffset =
		(struct CAP_LLS_DATA_EMI_OFFSET *)pucEventBuf;
	uint32_t offset = prOffset->u4DataEmiOffset;
	uint32_t size2 = prOffset->u4NumTxPowerLevels * ENUM_BAND_NUM *
			sizeof(uint32_t);
	uint32_t offset2 = prOffset->u4TxTimePerLevelEmiOffset;

	uint32_t u4HostOffsetInfo =
		OFFSET_OF(struct STATS_LLS_WIFI_IFACE_STAT, info);
	uint32_t u4HostOffsetAc =
		OFFSET_OF(struct STATS_LLS_WIFI_IFACE_STAT, ac);
	uint32_t u4HostOffsetChannel =
		OFFSET_OF(struct WIFI_RADIO_CHANNEL_STAT, channel);
	uint32_t u4HostOffsetTxTimePerLevels =
		OFFSET_OF(struct STATS_LLS_WIFI_RADIO_STAT, tx_time_per_levels);
	uint32_t u4HostOffsetRxTime =
		OFFSET_OF(struct STATS_LLS_WIFI_RADIO_STAT, rx_time);
	uint8_t ucLinkStatsBssNum = 1;

	DBGLOG(INIT, INFO, "Offset(Host): %u/%u/%u/%u/%u power=%u,%u",
			u4HostOffsetInfo, u4HostOffsetAc,
			u4HostOffsetTxTimePerLevels, u4HostOffsetRxTime,
			u4HostOffsetChannel,
			prOffset->u4NumTxPowerLevels, ENUM_BAND_NUM);

	if (!offset) {
		DBGLOG(INIT, WARN, "NULL offset: offset=0x%08x, offset2=0x%08x",
				offset, offset2);
		return WLAN_STATUS_FAILURE;
	}
	if (!offset2) {
		DBGLOG(INIT, TRACE,
				"NULL offset: offset=0x%08x, offset2=0x%08x",
				offset, offset2);
	}

	/* Sanity check calculate number of interfaces */
	if (prOffset->u4OffsetPeerInfo %
			sizeof(struct STATS_LLS_WIFI_IFACE_STAT)) {
		DBGLOG(INIT, WARN,
		       "u4OffsetPeerInfo not multiple of iface, FW: %u, sz=%zu, return fail",
			prOffset->u4OffsetPeerInfo,
			sizeof(struct STATS_LLS_WIFI_IFACE_STAT));
	} else {
		ucLinkStatsBssNum = prOffset->u4OffsetPeerInfo /
				sizeof(struct STATS_LLS_WIFI_IFACE_STAT);
	}

	/* Check sizeof peer info by calculation */
	if (prOffset->u4OffsetRadioStat - prOffset->u4OffsetPeerInfo <
	    sizeof(struct PEER_INFO_RATE_STAT[CFG_STA_REC_NUM])) {
		DBGLOG(INIT, WARN,
		       "sizeof ratio stats FW: %u-%u < %u, return fail",
		       prOffset->u4OffsetRadioStat, prOffset->u4OffsetPeerInfo,
		       sizeof(struct PEER_INFO_RATE_STAT[CFG_STA_REC_NUM]));
	}

	if (prOffset->u4OffsetInfo != u4HostOffsetInfo ||
	    prOffset->u4OffsetAc != u4HostOffsetAc ||
	    prOffset->u4OffsetPeerInfo %
		sizeof(struct STATS_LLS_WIFI_IFACE_STAT) != 0 ||
	    prOffset->u4OffsetRadioStat - prOffset->u4OffsetPeerInfo <
		sizeof(struct PEER_INFO_RATE_STAT[CFG_STA_REC_NUM]) ||
	    prOffset->u4OffsetTxTimerPerLevels != u4HostOffsetTxTimePerLevels ||
	    prOffset->u4OffsetRxTime != u4HostOffsetRxTime ||
	    prOffset->u4OffsetChannel != u4HostOffsetChannel) {
		DBGLOG(INIT, WARN,
		       "Offset not match(FW): %u/%u/%u(:%u)/%u:(%u)/%u/%u/%u",
			prOffset->u4OffsetInfo,
			prOffset->u4OffsetAc,
			prOffset->u4OffsetPeerInfo,
			sizeof(struct STATS_LLS_WIFI_IFACE_STAT),
			prOffset->u4OffsetRadioStat -
			prOffset->u4OffsetPeerInfo,
			sizeof(struct PEER_INFO_RATE_STAT[CFG_STA_REC_NUM]),
			prOffset->u4OffsetTxTimerPerLevels,
			prOffset->u4OffsetRxTime,
			prOffset->u4OffsetChannel);
		return WLAN_STATUS_FAILURE;
	}
	if (offset2 && size2 > LLS_RADIO_STAT_MAX_TX_LEVELS *
			ENUM_BAND_NUM * sizeof(uint32_t)) {
		DBGLOG(INIT, WARN, "Size 2 too large: %u", size2);
		return WLAN_STATUS_FAILURE;
	}

	prAdapter->ucLinkStatsBssNum = ucLinkStatsBssNum;
	if (prAdapter->pucLinkStatsSrcBufAddr) {
		DBGLOG(INIT, WARN, "LLS EMI stats set, update it.");
		prAdapter->pucLinkStatsSrcBufAddr = NULL;
	}

	if (prAdapter->pu4TxTimePerLevels) {
		DBGLOG(INIT, WARN, "LLS EMI PowerLevel set, update it.");
		prAdapter->pu4TxTimePerLevels = NULL;
	}

	/* Update offset */
	prAdapter->pucLinkStatsSrcBufAddr =
		emi_mem_get_vir_base(prAdapter->chip_info) +
		emi_mem_offset_convert(offset);
	prAdapter->pu4TxTimePerLevels =
		emi_mem_get_vir_base(prAdapter->chip_info) +
		emi_mem_offset_convert(offset2);
	prAdapter->u4TxTimePerLevelsSize = size2;

	prAdapter->prLinkStatsIface = (struct STATS_LLS_WIFI_IFACE_STAT *)
		prAdapter->pucLinkStatsSrcBufAddr;
	prAdapter->prLinkStatsPeerInfo = (struct PEER_INFO_RATE_STAT *)
		&prAdapter->pucLinkStatsSrcBufAddr[prOffset->u4OffsetPeerInfo];
	prAdapter->prLinkStatsRadioInfo = (struct WIFI_RADIO_CHANNEL_STAT *)
		&prAdapter->pucLinkStatsSrcBufAddr[prOffset->u4OffsetRadioStat];

	DBGLOG(INIT, INFO, "EMI offset=%x, offset2=%x (%u), BssNum=%u",
			offset, offset2, size2, prAdapter->ucLinkStatsBssNum);
#endif
	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
uint32_t nicCfgChipCapStatsRegMontrEmiOffset(
	struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf)
{
	struct CAP_STATS_REG_MONTR_EMI_OFFSET *prCap =
		(struct CAP_STATS_REG_MONTR_EMI_OFFSET *)pucEventBuf;
	uint32_t offset = prCap->u4EmiOffset;
	uint32_t u4HostOffsetBasic =
		OFFSET_OF(struct STATS_REG_STAT_FW_REPORT, rBasicStatistics);
	uint32_t u4HostOffsetLq =
		OFFSET_OF(struct STATS_REG_STAT_FW_REPORT, rLq);
	uint32_t u4HostOffsetStaStats =
		OFFSET_OF(struct STATS_REG_STAT_FW_REPORT, rStaStats);
	uint32_t u4HostOffsetLlsStatus =
		OFFSET_OF(struct STATS_REG_STAT_FW_REPORT, llsUpdateStatus);
#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
	uint32_t u4HostOffsetLastTxRateInfo =
		OFFSET_OF(struct STATS_REG_STAT_FW_REPORT, rLlsRateInfo);
#endif

	if (!offset) {
		DBGLOG(INIT, WARN, "NULL offset: offset=%p", offset);
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(INIT, INFO,
			"Offset FW:%u/%u/%u/%u host:%u/%u/%u/%u",
			prCap->u4OffsetOfBasic,
			prCap->u4OffsetOfLq,
			prCap->u4OffsetOfStaStats,
			prCap->u4OffsetOfLlsStatus,
			u4HostOffsetBasic,
			u4HostOffsetLq,
			u4HostOffsetStaStats,
			u4HostOffsetLlsStatus);

	if (prCap->u4OffsetOfBasic != u4HostOffsetBasic ||
	    prCap->u4OffsetOfLq != u4HostOffsetLq ||
	    prCap->u4OffsetOfStaStats != u4HostOffsetStaStats ||
	    prCap->u4OffsetOfLlsStatus != u4HostOffsetLlsStatus) {
		DBGLOG(INIT, WARN,
			"Offset not match(FW):%u/%u/%u/%u host:%u/%u/%u/%u",
			prCap->u4OffsetOfBasic,
			prCap->u4OffsetOfLq,
			prCap->u4OffsetOfStaStats,
			prCap->u4OffsetOfLlsStatus,
			u4HostOffsetBasic,
			u4HostOffsetLq,
			u4HostOffsetStaStats,
			u4HostOffsetLlsStatus);
		return WLAN_STATUS_FAILURE;
	}
#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
	DBGLOG(INIT, INFO,
			"Tx Rate Offset FW:%u host:%u",
			prCap->u4OffsetOfLastTxRateInfo,
			u4HostOffsetLastTxRateInfo);
	if (prCap->u4OffsetOfLastTxRateInfo == u4HostOffsetLastTxRateInfo)
		prAdapter->fgTxRateOffsetMapped = TRUE;
#endif

	prAdapter->prStatsAllRegStat =
		emi_mem_get_vir_base(prAdapter->chip_info) +
		emi_mem_offset_convert(offset);

	DBGLOG(INIT, INFO, "offset:%x addr:%p\n",
	       offset,
	       prAdapter->prStatsAllRegStat);

	return WLAN_STATUS_SUCCESS;
}
#endif

/* This function is used to get the EMI offset sent by FW.
 * Driver will handle the returned EMI offset only if it is valid.
 * The format of the returned event buffer is shown as follows.
 * struct buffer {
 *     uint32_t num_of_tables;
 *     struct sw_sync_emi_info tables[num_of_tables] = {
 *         // {tag, isValid, EMI offset}
 *         {0, 1, 0x20000},
 *         {2, 0, 0x0},
 *         ...
 *     };
 * };
 */
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
uint32_t nicCfgGetSwSyncEMIOffset(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	uint32_t u4Idx, u4NumOfTables = 0, *pu4NumOfTables = NULL;
	struct sw_sync_emi_info *prInfo = NULL, *prOnOffInfo = NULL;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);

	/* Get the number of struct sw_sync_emi_info */
	pu4NumOfTables = (uint32_t *)pucEventBuf;
	if (pu4NumOfTables == NULL) {
		DBGLOG(INIT, ERROR, "NULL EVT buffer.\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}
	u4NumOfTables = *pu4NumOfTables;
	if (u4NumOfTables == 0) {
		DBGLOG(INIT, WARN,
		"None of Host/FW EMI sync info is available.\n");
		return WLAN_STATUS_NOT_SUPPORTED;
	}
	++pu4NumOfTables;
	/* Start to parsing sw_sync_emi_info table */
	prInfo = (struct sw_sync_emi_info *)pu4NumOfTables;
	for (u4Idx = 0; u4Idx < u4NumOfTables; ++u4Idx) {
		switch (prInfo[u4Idx].tag) {
		case SW_SYNC_ON_OFF_TAG: {
			DBGLOG(INIT, INFO,
			"WiFi On/Off EMI valid flag:[%s], Offset:[0x%08x]\n",
			prInfo[u4Idx].isValid ? "valid" : "invalid",
			prInfo[u4Idx].offset);
			prOnOffInfo =
			  &prChipInfo->sw_sync_emi_info[SW_SYNC_ON_OFF_TAG];
			if (prInfo[u4Idx].isValid)
				kalMemCopy(prOnOffInfo,
					   &prInfo[u4Idx],
					   sizeof(prInfo[u4Idx]));
			break;
		}
		default:
			DBGLOG(INIT, WARN, "Get Invalid TAG:%d\n",
				prInfo[u4Idx].tag);
			return WLAN_STATUS_NOT_ACCEPTED;
		}
	}
	return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_SUPPORT_MBRAIN
uint32_t checkMbrOffset(uint32_t num,
	struct MBRAIN_OFFSET_INFO *prOffsetInfo)
{
	uint32_t i;
	uint32_t *pu4OffsetMap = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (!prOffsetInfo || MBRAIN_EMI_OFFSET_NUM == 0)
		return WLAN_STATUS_FAILURE;

	pu4OffsetMap = kalMemAlloc(
		MBRAIN_EMI_OFFSET_NUM * sizeof(uint32_t),
		VIR_MEM_TYPE);

	if (!pu4OffsetMap)
		return WLAN_STATUS_FAILURE;

	kalMemZero(pu4OffsetMap, MBRAIN_EMI_OFFSET_NUM * sizeof(uint32_t));

	/* init offset mapping here */
	/* example:
	 * pu4OffsetMap[MBRAIN_EMI_OFFSET_TEST] =
	 *	OFFSET_OF(struct mbrain_emi_data, u4Mbr_test),
	 * pu4OffsetMap[MBRAIN_EMI_OFFSET_TEST] =
	 *	OFFSET_OF(struct mbrain_emi_data, u4Mbr_test2),
	 */

#if CFG_SUPPORT_WIFI_ICCM
	pu4OffsetMap[MBRAIN_EMI_OFFSET_ICCM] =
		OFFSET_OF(struct mbrain_emi_data, rMbrIccmData);
#endif

	for (i = 0; i < num; i++, prOffsetInfo++) {
		if (prOffsetInfo->u4Tag >= MBRAIN_EMI_OFFSET_NUM) {
			DBGLOG(INIT, WARN, "invalid tag:%u offset:%u\n",
				prOffsetInfo->u4Tag,
				prOffsetInfo->u4EmiOffset);
			status = WLAN_STATUS_FAILURE;
			goto end;
		}
		if (prOffsetInfo->u4EmiOffset !=
				*(pu4OffsetMap + prOffsetInfo->u4Tag)) {
			DBGLOG(INIT, WARN, "Offset mismatch [%u]=%u/%u\n",
				prOffsetInfo->u4Tag,
				*(pu4OffsetMap + prOffsetInfo->u4Tag),
				prOffsetInfo->u4EmiOffset);
			status = WLAN_STATUS_FAILURE;
			goto end;
		}
	}

end:
	kalMemFree(pu4OffsetMap, VIR_MEM_TYPE,
		MBRAIN_EMI_OFFSET_NUM * sizeof(uint32_t))
	return status;
}

uint32_t nicCfgChipMbrEmiInfo(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
#if CFG_MTK_ANDROID_EMI
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct HIF_MEM_OPS *prMemOps;
	struct HIF_MEM *prMem;
	struct CAP_MBRAIN_EMI_INFO *prInfo =
		(struct CAP_MBRAIN_EMI_INFO *)pucEventBuf;
	uint32_t mbrOffset = prInfo->u4PcieGenSwRsvd;
	uint32_t num = prInfo->u4OffsetNum;

	struct MBRAIN_OFFSET_INFO *prOffsetInfo;

	++prInfo;
	prOffsetInfo = (struct MBRAIN_OFFSET_INFO *)(prInfo);

	if (checkMbrOffset(num, prOffsetInfo) != WLAN_STATUS_SUCCESS)
		return WLAN_STATUS_FAILURE;

	/* Update offset */
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(
			prChipInfo, WIFI_MISC_MEM_BLOCK_WF_M_BRAIN);
		if (!prMem || !prMem->va)
			return WLAN_STATUS_FAILURE;

		prAdapter->prMbrEmiData = (struct mbrain_emi_data *)(
			(uint32_t *)prMem->va + mbrOffset);
		DBGLOG(INIT, INFO, "addr=%p offsetNum=%u",
			prAdapter->prMbrEmiData, num);
	}

#endif
	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicCmdEventCasanLoadType(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{

	struct CAP_CASAN_LOAD_TYPE_T *prLoadType =
		(struct CAP_CASAN_LOAD_TYPE_T *)pucEventBuf;

	prAdapter->u4CasanLoadType = prLoadType->u4CasanLoadType;

	DBGLOG(INIT, INFO,
	       "Casan load type = %x\n",
	       prAdapter->u4CasanLoadType);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_ENABLE_FW_DOWNLOAD
#if IS_ENABLED(CFG_MTK_WIFI_SUPPORT_UDS_FWDL)
uint32_t nicCfgChipCapRedlInfo(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
	struct CAP_REDL_INFO *prRedlInfo = (struct CAP_REDL_INFO *)pucEventBuf;

	fwDlSetupReDl(prAdapter, prRedlInfo->u4RedlOffset,
		prRedlInfo->u4RedlLen);

	return WLAN_STATUS_SUCCESS;
}
#endif
#endif

#if CFG_SUPPORT_MLR
uint32_t nicCfgChipCapMlr(struct ADAPTER *prAdapter,
			       uint8_t *pucEventBuf)
{
	struct CAP_MLR_CAP *prMLRCap =
		(struct CAP_MLR_CAP *)pucEventBuf;

	prAdapter->u4MlrCapSupportBitmap = prMLRCap->u4MlrSupportBitmap;
	prAdapter->u4MlrSupportBitmap = prMLRCap->u4MlrSupportBitmap &
		prAdapter->rWifiVar.u4MlrCfg;
	prAdapter->ucMlrVersion = prMLRCap->ucVersion;

	DBGLOG(INIT, INFO,
		"MLR cap - MlrCfg=0x%02x MlrSB=0x%02x, cMlrVer=%d, cMlrSB=0x%02x\n",
		prAdapter->rWifiVar.u4MlrCfg,
		prAdapter->u4MlrSupportBitmap,
		prMLRCap->ucVersion,
		prMLRCap->u4MlrSupportBitmap);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_CONNAC3X == 1)
#if (CFG_SUPPORT_QA_TOOL == 1)
uint32_t nicCmdEventTestmodeCap(struct ADAPTER
	  *prAdapter, uint8_t *pucEventBuf)
{
	memcpy((uint8_t *)&g_HqaCap, pucEventBuf, sizeof(struct TESTMODE_CAP));
	return WLAN_STATUS_SUCCESS;
}
#endif
#endif
uint32_t nicCmdEventHostSuspendInfo(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
	struct CAP_HOST_SUSPEND_INFO_T *prEvent =
		(struct CAP_HOST_SUSPEND_INFO_T *)pucEventBuf;
	struct HOST_SUSPEND_NOTIFY_INFO *prNotifyInfo =
		&prAdapter->rHostSuspendInfo;

	prNotifyInfo->eType = prEvent->eType;
	prNotifyInfo->u4SetAddr = prEvent->u4SetAddr;
	prNotifyInfo->u4ClrAddr = prEvent->u4ClrAddr;
	prNotifyInfo->u4Mask = prEvent->u4Mask;
	prNotifyInfo->u4Shift = prEvent->u4Shift;

	DBGLOG(INIT, INFO, "type: %d, addr: 0x%x 0x%x, mask: 0x%x, shift: %d\n",
		prNotifyInfo->eType,
		prNotifyInfo->u4SetAddr,
		prNotifyInfo->u4ClrAddr,
		prNotifyInfo->u4Mask,
		prNotifyInfo->u4Shift);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_FAST_PATH_SUPPORT
uint32_t nicCfgChipCapFastPath(struct ADAPTER *prAdapter,
			       uint8_t *pucEventBuf)
{
	struct MSCS_CAP_FAST_PATH *prFastPathCap =
		(struct MSCS_CAP_FAST_PATH *)pucEventBuf;

	kalMemCopy(&prAdapter->rFastPathCap, prFastPathCap,
			sizeof(struct MSCS_CAP_FAST_PATH));

	DBGLOG(INIT, INFO,
	       "Fast path version(%d) support(%d) vendor key(0x%x) group key(0x%x)\n",
	       prFastPathCap->ucVersion, prFastPathCap->fgSupportFastPath,
	       prFastPathCap->u4KeyBitmap[0], prFastPathCap->u4KeyBitmap[2]);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicCfgChipCapMacCap(struct ADAPTER *prAdapter,
			     uint8_t *pucEventBuf)
{
	struct CAP_MAC_CAP *prMacCap = (struct CAP_MAC_CAP *)pucEventBuf;

	if (prMacCap->ucHwBssIdNum > 0 &&
	    prMacCap->ucHwBssIdNum <= HW_BSSID_NUM) {
		prAdapter->ucHwBssIdNum = prMacCap->ucHwBssIdNum;
		prAdapter->ucP2PDevBssIdx = prAdapter->ucHwBssIdNum;
	}

	if (prMacCap->ucSwBssIdNum > 0) {
		if (prMacCap->ucSwBssIdNum <= MAX_BSSID_NUM) {
			prAdapter->ucSwBssIdNum = prMacCap->ucSwBssIdNum;
			prAdapter->ucP2PDevBssIdx = prAdapter->ucSwBssIdNum;
		} else {
			/* p2p dev init faii if over MAX_BSSID_NUM */
			prAdapter->ucP2PDevBssIdx = MAX_BSSID_NUM + 1;
		}
	}

	if (prMacCap->ucWtblEntryNum > 0
	    && prMacCap->ucWtblEntryNum <= WTBL_SIZE) {
		prAdapter->ucWtblEntryNum = prMacCap->ucWtblEntryNum;
		prAdapter->ucTxDefaultWlanIndex = prAdapter->ucWtblEntryNum - 1;
	}

	prAdapter->ucWmmSetNum = prMacCap->ucWmmSet > 0 ?
		prMacCap->ucWmmSet : 1;

	DBGLOG(INIT, INFO,
		"ucHwBssIdNum: %d, ucSwBssIdNum: %d(MAX=%d), ucP2PDevBssIdx: %d, ucWtblEntryNum: %d, ucWmmSetNum: %d.\n",
			prMacCap->ucHwBssIdNum,
			prMacCap->ucSwBssIdNum,
			MAX_BSSID_NUM,
			prAdapter->ucP2PDevBssIdx,
			prMacCap->ucWtblEntryNum,
			prMacCap->ucWmmSet);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapFrameBufCap(struct ADAPTER *prAdapter,
				  uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapBeamformCap(struct ADAPTER *prAdapter,
				  uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapLocationCap(struct ADAPTER *prAdapter,
				  uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapMuMimoCap(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_ANT_SWAP
uint32_t nicCfgChipCapAntSwpCap(struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf)
{
	struct CAP_ANTSWP *prAntSwpCap = (struct CAP_ANTSWP *)pucEventBuf;

	/* FW's value combines both platform and FW capablity */
	prAdapter->fgIsSupportAntSwp = prAntSwpCap->ucIsSupported;
	DBGLOG(INIT, INFO,
		"fgIsSupportAntSwp = %d\n",
		prAdapter->fgIsSupportAntSwp);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_ANT_SWAP */

#if (CFG_SUPPORT_WIFI_6G == 1)
uint32_t nicCfgChipCap6GCap(struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf)
{
	struct CAP_6G_CAP *pr6gCap = (struct CAP_6G_CAP *)pucEventBuf;

	ASSERT(prAdapter);

	if (prAdapter->rWifiVar.ucDisallowBand6G)
		prAdapter->fgIsHwSupport6G = FALSE;
	else
		prAdapter->fgIsHwSupport6G = pr6gCap->ucIsSupport6G;

	prAdapter->rWifiFemCfg.u2WifiPath6G = (uint16_t)(pr6gCap->ucHwWifiPath);

	prAdapter->rWifiFemCfg.u2WifiDBDCAwithA =
		(uint16_t)(pr6gCap->ucWifiDBDCAwithA);

	/* Frequency unit is 5 (due to char8), need to x5 to original value */
	prAdapter->rWifiFemCfg.u2WifiDBDCAwithAMinimumFrqInterval =
		(uint16_t)((pr6gCap->ucWifiDBDCAwithAMinimumFrqInterval)*5);

	/* Force set to support DBDC A+A and interval for TEST
	 * prAdapter->rWifiFemCfg.u2WifiDBDCAwithA = 1;
	 * prAdapter->rWifiFemCfg.u2WifiDBDCAwithAMinimumFrqInterval = 360;
	 */
	DBGLOG(INIT, INFO, "fgIsHwSupport6G = %d, u2WifiPath6G=%d\n",
		prAdapter->fgIsHwSupport6G,
		prAdapter->rWifiFemCfg.u2WifiPath6G);
	DBGLOG(INIT, INFO, "u2WifiDBDCAwithA = %d minimumFre=%d\n",
		prAdapter->rWifiFemCfg.u2WifiDBDCAwithA,
		prAdapter->rWifiFemCfg.u2WifiDBDCAwithAMinimumFrqInterval);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_WIFI_6G */

struct nicTxRsrcEvtHdlr nicTxRsrcEvtHdlrTbl[] = {
	{NIC_TX_RESOURCE_REPORT_VERSION_1,
	nicEventQueryTxResource_v1,
	nicTxResourceUpdate_v1},
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	{NIC_TX_RESOURCE_REPORT_VERSION_2,
	nicEventQueryTxResource_v2,
	nicTxResourceUpdate_v2},
#endif
};

#if CFG_SUPPORT_802_11BE_MLO
uint32_t nicCfgChipCapMLO(struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf)
{
	struct CAP_MLO_CAP *cap = (struct CAP_MLO_CAP *)pucEventBuf;

#if (CFG_SUPPORT_MLO_HYBRID == 1)
	prAdapter->rWifiVar.ucNonApHyMloSupportCap =
			cap->ucNonApHyMloSupport;
	prAdapter->rWifiVar.ucLink3BandLimitBitmap =
			cap->ucLink3BandLimitBitmap;
#endif

	prAdapter->rWifiVar.ucMaxSimuLinksCap = cap->ucMaxSimuLinks;
	if (cap->ucNonApMldEMLSupport)
		prAdapter->rWifiVar.u2NonApMldEMLCap = cap->u2NonApMldEMLCap;
	if (cap->ucApMldEMLSupport)
		prAdapter->rWifiVar.u2ApMldEMLCap = cap->u2ApMldEMLCap;

	DBGLOG(INIT, INFO,
		"EML cap - Non-AP=(%d,0x%x,%d), AP=(%d, 0x%x), MaxSimuLinks=%d\n",
		prAdapter->rWifiVar.ucNonApMldEMLSupport,
		prAdapter->rWifiVar.u2NonApMldEMLCap,
		prAdapter->rWifiVar.ucNonApHyMloSupportCap,
		prAdapter->rWifiVar.ucApMldEMLSupport,
		prAdapter->rWifiVar.u2ApMldEMLCap,
		prAdapter->rWifiVar.ucMaxSimuLinksCap);
	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicEventQueryTxResource(struct ADAPTER
				 *prAdapter, uint8_t *pucEventBuf)
{
	uint32_t i, i_max;
	uint32_t version = *((uint32_t *)(pucEventBuf));

	i_max = ARRAY_SIZE(nicTxRsrcEvtHdlrTbl);
	for (i = 0; i < i_max; i++) {
		if (version == nicTxRsrcEvtHdlrTbl[i].u4Version) {
			/* assign callback to do the resource init. */
			prAdapter->nicTxReousrce.txResourceInit =
				nicTxRsrcEvtHdlrTbl[i].nicTxResourceInit;

			return nicTxRsrcEvtHdlrTbl[i].nicEventTxResource(
				prAdapter, pucEventBuf);
		}
	}

	/* invalid version, cannot find the handler */
	DBGLOG(INIT, ERROR,
	       "Invalid version. (0x%X)\n", version);
	prAdapter->nicTxReousrce.txResourceInit = NULL;

	return WLAN_STATUS_NOT_SUPPORTED;
}

uint32_t nicEventQueryTxResource_v1(struct ADAPTER
				    *prAdapter, uint8_t *pucEventBuf)
{
	struct tx_resource_report_v1 *pV1 =
		(struct tx_resource_report_v1 *)pucEventBuf;
	uint16_t page_size;

	/* PSE */
	page_size = pV1->u4PlePsePageSize & 0xFFFF;
	prAdapter->nicTxReousrce.u4CmdTotalResource =
		pV1->u4HifCmdPsePageQuota;
	prAdapter->nicTxReousrce.u4CmdResourceUnit = page_size;
	prAdapter->nicTxReousrce.u4DataTotalResource =
		pV1->u4HifDataPsePageQuota;
	prAdapter->nicTxReousrce.u4DataResourceUnit = page_size;

	/* PLE */
	page_size = (pV1->u4PlePsePageSize >> 16) & 0xFFFF;
	prAdapter->nicTxReousrce.u4CmdTotalResourcePle =
		pV1->u4HifCmdPlePageQuota;
	prAdapter->nicTxReousrce.u4CmdResourceUnitPle = page_size;
	prAdapter->nicTxReousrce.u4DataTotalResourcePle =
		pV1->u4HifDataPlePageQuota;
	prAdapter->nicTxReousrce.u4DataResourceUnitPle = page_size;

	/* PpTxAddCnt */
	prAdapter->nicTxReousrce.ucPpTxAddCnt = pV1->ucPpTxAddCnt;

	/* enable PLE resource control flag */
	if (!prAdapter->nicTxReousrce.u4DataTotalResourcePle)
		prAdapter->rTxCtrl.rTc.fgNeedPleCtrl = FALSE;
	else
		prAdapter->rTxCtrl.rTc.fgNeedPleCtrl =
			NIC_TX_RESOURCE_CTRL_PLE;
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicEventQueryTxResource_v2(struct ADAPTER
				    *prAdapter, uint8_t *pucEventBuf)
{
	/* Follow V1 as default */
	return nicEventQueryTxResource_v1(prAdapter, pucEventBuf);
}

uint32_t nicParsingNicCapV2(struct ADAPTER *prAdapter,
	uint32_t u4Type, uint8_t *pucEventBuf)
{
	uint32_t u4RetStatus = WLAN_STATUS_FAILURE;
	uint32_t table_idx;
	u_int8_t fgTagFound = FALSE;

	for (table_idx = 0; table_idx < ARRAY_SIZE(gNicCapabilityV2InfoTable);
	     table_idx++) {

		/* find the corresponding tag's handler */
		if (gNicCapabilityV2InfoTable[table_idx].tag_type == u4Type &&
		    gNicCapabilityV2InfoTable[table_idx].hdlr != NULL) {
			u4RetStatus = gNicCapabilityV2InfoTable[table_idx].hdlr(
				prAdapter, pucEventBuf);
			fgTagFound = TRUE;
			break;
		}
	}

	if (!fgTagFound)
		DBGLOG(INIT, ERROR, "Find TAG: %d failed\n", u4Type);

	return u4RetStatus;
}

void nicCmdEventQueryNicCapabilityV2(struct ADAPTER *prAdapter,
				     uint8_t *pucEventBuf)
{
	struct EVENT_NIC_CAPABILITY_V2 *prEventNicV2 =
		(struct EVENT_NIC_CAPABILITY_V2 *)pucEventBuf;
	struct NIC_CAPABILITY_V2_ELEMENT *prElement;
	uint32_t tag_idx, offset;
	uint16_t u2TotalElementNum;

	offset = 0;
	u2TotalElementNum = ARRAY_SIZE(gNicCapabilityV2InfoTable);

	/* process each element */
	for (tag_idx = 0; tag_idx < prEventNicV2->u2TotalElementNum;
	     tag_idx++) {
		if (tag_idx > u2TotalElementNum) {
			DBGLOG(INIT, ERROR,
				"tag idx too long: %d > %d\n",
				tag_idx, u2TotalElementNum);
			break;
		}

		prElement = (struct NIC_CAPABILITY_V2_ELEMENT *)(
				    prEventNicV2->aucBuffer + offset);

		nicParsingNicCapV2(prAdapter, prElement->tag_type,
			prElement->aucbody);

		/* move to the next tag */
		offset += prElement->body_len + (uint16_t) OFFSET_OF(
				  struct NIC_CAPABILITY_V2_ELEMENT, aucbody);
	}
}

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
void nicCmdEventQueryTxPowerInfo(struct ADAPTER *prAdapter,
				 struct CMD_INFO *prCmdInfo,
				 uint8_t *pucEventBuf)
{
	struct EXT_EVENT_TXPOWER_ALL_RATE_POWER_INFO_T *prEvent =
			NULL;
	struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T *prTxPowerInfo =
			NULL;
	uint32_t u4QueryInfoLen = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (!prAdapter)
		return;

	prGlueInfo = prAdapter->prGlueInfo;
	if (!prCmdInfo)
		return;
	if (!pucEventBuf)
		return;
	if (!(prCmdInfo->pvInformationBuffer))
		return;

	if (prCmdInfo->fgIsOid) {
		prEvent = (struct EXT_EVENT_TXPOWER_ALL_RATE_POWER_INFO_T *)
			  pucEventBuf;

		if (prEvent->ucTxPowerCategory ==
		    TXPOWER_EVENT_SHOW_ALL_RATE_TXPOWER_INFO) {
			prEvent =
				(struct
				EXT_EVENT_TXPOWER_ALL_RATE_POWER_INFO_T *)
				 pucEventBuf;
			prTxPowerInfo =
				(struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T *)
				prCmdInfo->pvInformationBuffer;
			u4QueryInfoLen =
				sizeof(
				struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T);

			kalMemCopy(prTxPowerInfo, prEvent,
			sizeof(
			struct EXT_EVENT_TXPOWER_ALL_RATE_POWER_INFO_T));
		}

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#endif

void nicEventLinkQuality(struct ADAPTER *prAdapter,
			 struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	ASSERT(prAdapter);

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	DBGLOG(RX, TRACE, "prCmdInfo=%p", prCmdInfo);
	if (prCmdInfo != NULL) {
		DBGLOG(RX, TRACE, "Calling prCmdInfo->pfCmdDoneHandler=%ps",
				prCmdInfo->pfCmdDoneHandler);
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				prCmdInfo,
				0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

void nicExtEventReCalData(struct ADAPTER *prAdapter, uint8_t *pucEventBuf)
{
	struct EXT_EVENT_RECAL_DATA_T *prReCalData = NULL;
	struct RECAL_INFO_T *prReCalInfo = NULL;
	struct RECAL_DATA_T *prCalArray = NULL;
	uint32_t u4Idx = 0;

	ASSERT(pucEventBuf);
	ASSERT(prAdapter);
	prReCalInfo = &prAdapter->rReCalInfo;
	if (prReCalInfo->prCalArray == NULL) {
		prCalArray = (struct RECAL_DATA_T *)kalMemAlloc(
			  2048 * sizeof(struct RECAL_DATA_T), VIR_MEM_TYPE);

		if (prCalArray == NULL) {
			DBGLOG(RFTEST, ERROR,
				"Unable to alloc memory for recal data\n");
			return;
		}
		prReCalInfo->prCalArray = prCalArray;
	}

	if (prReCalInfo->u4Count >= 2048) {
		DBGLOG(RFTEST, ERROR,
			"Too many Recal packet, maximum packets will be 2048, ignore\n");
		return;
	}

	prCalArray = prReCalInfo->prCalArray;
	DBGLOG(RFTEST, INFO, "prCalArray[%d] address [%p]\n",
			     prReCalInfo->u4Count,
			     &prCalArray[prReCalInfo->u4Count]);

	prReCalData = (struct EXT_EVENT_RECAL_DATA_T *)pucEventBuf;
	switch (prReCalData->u4Type) {
	case 0: {
		unsigned long ulTmpData;

		prReCalData->u.ucData[9] = '\0';
		prReCalData->u.ucData[19] = '\0';
		u4Idx = prReCalInfo->u4Count;

		if (kalStrtoul(&prReCalData->u.ucData[1], 16, &ulTmpData))
			DBGLOG(RFTEST, ERROR, "convert fail: ucData[1]\n");
		else
			prCalArray[u4Idx].u4CalId = (unsigned int)ulTmpData;
		if (kalStrtoul(&prReCalData->u.ucData[11], 16, &ulTmpData))
			DBGLOG(RFTEST, ERROR, "convert fail: ucData[11]\n");
		else
			prCalArray[u4Idx].u4CalAddr = (unsigned int)ulTmpData;
		if (kalStrtoul(&prReCalData->u.ucData[20], 16, &ulTmpData))
			DBGLOG(RFTEST, ERROR, "convert fail: ucData[20] %s\n",
					       &prReCalData->u.ucData[20]);
		else
			prCalArray[u4Idx].u4CalValue = (unsigned int)ulTmpData;

		DBGLOG(RFTEST, TRACE, "[0x%08x][0x%08x][0x%08x]\n",
					prCalArray[u4Idx].u4CalId,
					prCalArray[u4Idx].u4CalAddr,
					prCalArray[u4Idx].u4CalValue);
		prReCalInfo->u4Count++;
		break;
	}
	case 1:
		/* Todo: for extension to handle int */
		/*       data directly come from FW */
		break;
	}
}

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))

#if ((CFG_SUPPORT_PHY_ICS_V3 == 1) || (CFG_SUPPORT_PHY_ICS_V4 == 1))
void nicExtEventPhyIcsDumpEmiRawData(struct ADAPTER *prAdapter,
			   uint8_t *pucEventBuf)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct ICS_BIN_LOG_HDR *prIcsBinLogHeader;

	uint32_t u4EmiBaseAddr = 0;
	uint32_t u4PhyIcsTotalCnt = 0;
	uint32_t u4PhyIcsBufSize = 0;
	uint32_t *u4PhyIcsEventBuf = NULL;
	uint32_t u4TmpRawData[4];
	uint32_t u4Size = 0, u4EmiAddr = 0;
	uint32_t u4TotalCnt = 0;
	uint16_t u2DataOffset = 0, u2Idxi = 0;
	uint8_t  ucRawDataIdx = 0;
	uint32_t u4MemoryPart = 0;

	uint8_t *pucBuf = NULL;
	uint32_t *pu4Data = NULL;
	ssize_t ret;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	struct UNI_EVENT_PHY_ICS_DUMP_RAW_DATA *prPhyIcsEvent;
#else
	struct EXT_EVENT_PHY_ICS_DUMP_DATA_T *prPhyIcsEvent;
#endif

	if (!prAdapter) {
		DBGLOG(RFTEST, ERROR, "prAdapter is null\n");
		return;
	}

	prChipInfo = prAdapter->chip_info;

	if (pucEventBuf == NULL) {
		DBGLOG(RFTEST, ERROR, "pucEventBuf is null\n");
		return;
	}

	if (!prChipInfo->u4PhyIcsEmiBaseAddr) {
		DBGLOG(RFTEST, ERROR, "u4PhyIcsEmiBaseAddr is null\n");
		return;
	}

	if (!prChipInfo->u4PhyIcsTotalCnt) {
		DBGLOG(RFTEST, ERROR, "u4PhyIcsTotalCnt is null\n");
		return;
	}

	if (!prChipInfo->u4PhyIcsBufSize) {
		DBGLOG(RFTEST, ERROR, "u4PhyIcsBufSize is null\n");
		return;
	}

	u4EmiBaseAddr = prChipInfo->u4PhyIcsEmiBaseAddr;
	u4PhyIcsTotalCnt = prChipInfo->u4PhyIcsTotalCnt;
	u4PhyIcsBufSize = prChipInfo->u4PhyIcsBufSize;
	u4MemoryPart = prChipInfo->u4MemoryPart;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	prPhyIcsEvent = (struct UNI_EVENT_PHY_ICS_DUMP_RAW_DATA *)
				pucEventBuf;
#else
	prPhyIcsEvent = (struct EXT_EVENT_PHY_ICS_DUMP_DATA_T *)
				pucEventBuf;
#endif

	DBGLOG(RFTEST, TRACE,
		"u4WifiSysTimestamp = [0x%08x]\n",
		prPhyIcsEvent->u4PhyTimestamp);

	u4PhyIcsEventBuf = kalMemAlloc(u4PhyIcsBufSize, VIR_MEM_TYPE);
	if (!u4PhyIcsEventBuf) {
		DBGLOG(RFTEST, ERROR, "u4PhyIcsEventBuf is null\n");
		goto exit;
	}

	kalMemZero(u4PhyIcsEventBuf, u4PhyIcsBufSize);

#if (CFG_SUPPORT_PHY_ICS_V4 == 1)

	DBGLOG(RFTEST, LOUD, "Memory Part = %d\n",
		prPhyIcsEvent->u4Reserved[0]);

	if (prPhyIcsEvent->u4Reserved[0] == u4MemoryPart)
		u4EmiBaseAddr += u4PhyIcsBufSize;
#endif

	if (emi_mem_read(prChipInfo, u4EmiBaseAddr,
			u4PhyIcsEventBuf, u4PhyIcsBufSize)) {
		DBGLOG(REQ, ERROR, "emi_mem_read fail.\n");
		goto exit;
	}

	/* Print EMI data for debugging purpose */
	DBGLOG_MEM32(REQ, LOUD, u4PhyIcsEventBuf, u4PhyIcsBufSize);

	/* band0 phyics bus 128bit only have 64bit data
	 * other 64bit data value is 0, so allocate 8KB
	 */
	if (prAdapter->uPhyICSBandIdx == ENUM_BAND_0)
		u4PhyIcsBufSize = u4PhyIcsBufSize / 2;

	/* phy ics packet + fw parser header */
	u4Size = u4PhyIcsBufSize + sizeof(struct ICS_BIN_LOG_HDR);

	if (prAdapter->uPhyICSBandIdx == ENUM_BAND_0)
		u4TotalCnt = u4PhyIcsTotalCnt / 2;
	else
		u4TotalCnt = u4PhyIcsTotalCnt;

	pucBuf = kalMemAlloc(u4Size, VIR_MEM_TYPE);
	if (!pucBuf) {
		DBGLOG_LIMITED(NIC, INFO, "pucBuf NULL\n");
		RX_INC_CNT(&prAdapter->rRxCtrl, RX_ICS_DROP_COUNT);
		goto exit;
	}

	kalMemZero(pucBuf, u4Size);

	pu4Data = (uint32_t *)(pucBuf + sizeof(struct ICS_BIN_LOG_HDR));

	/* MCU sysram data parsing and reorder */
	for (u2DataOffset = 0; u2DataOffset < (u4TotalCnt - 1);) {

		for (ucRawDataIdx = 0; ucRawDataIdx < 4; ucRawDataIdx++) {

			u4TmpRawData[3 - ucRawDataIdx]
				= *(u4PhyIcsEventBuf +
					u4EmiAddr + ucRawDataIdx);

			if (u4EmiAddr > 4096)
				break;

			/* Print ICap data to console for debugging purpose */
			DBGLOG(RFTEST, LOUD, "DataOfs[%d], rawDataIdx[%d]\t"
				"RawData[%d]: %08x\n",
				u2DataOffset, ucRawDataIdx,
				ucRawDataIdx,
				*(u4PhyIcsEventBuf + u4EmiAddr + ucRawDataIdx));

		}

		if (prAdapter->uPhyICSBandIdx == ENUM_BAND_0) {
			/* TmpRawData[2]=phytimestamp_band
			 *              or payload+checksum
			 * TmpRawData[3]=mac_timestamp or payload
			 */
			pu4Data[u2DataOffset + 0] = u4TmpRawData[2];
			pu4Data[u2DataOffset + 1] = u4TmpRawData[3];

			u2DataOffset += 2; /* 64 bit mode */
		} else {
			/* TmpRawData[0] = payload+checksum
			 * TmpRawData[1] = payload
			 * TmpRawData[2] = phytimestamp_band
			 * TmpRawData[3] = mac_timestamp
			 */
			pu4Data[u2DataOffset + 0] = u4TmpRawData[0];
			pu4Data[u2DataOffset + 1] = u4TmpRawData[1];
			pu4Data[u2DataOffset + 2] = u4TmpRawData[2];
			pu4Data[u2DataOffset + 3] = u4TmpRawData[3];

			u2DataOffset += 4; /* 128 bit mode */
		}
		u4EmiAddr += 4;
	}

	/* endian swap */
	for (u2Idxi = 0; u2Idxi < u4TotalCnt; u2Idxi++) {
		pu4Data[u2Idxi] =
			((pu4Data[u2Idxi] & 0x000000FF) << 24)
			| ((pu4Data[u2Idxi] & 0x0000FF00) << 8)
			| ((pu4Data[u2Idxi] & 0x00FF0000) >> 8)
			| ((pu4Data[u2Idxi] & 0xFF000000) >> 24);
	}

	/* prepare ICS header */
	prIcsBinLogHeader = (struct ICS_BIN_LOG_HDR *)pucBuf;
	prIcsBinLogHeader->u4MagicNum = ICS_BIN_LOG_MAGIC_NUM;
	prIcsBinLogHeader->u4Timestamp = prPhyIcsEvent->u4PhyTimestamp;
	prIcsBinLogHeader->u2MsgID = RX_PKT_TYPE_PHY_ICS;
	prIcsBinLogHeader->u2Length = u4PhyIcsBufSize;

	/* prepare ICS frame
	 * pucBuf = ICS Header + PHY ICS payload length
	 * skip ICS header of pucBuf, start to next address copy
	 */
	kalMemCopy(pucBuf + sizeof(struct ICS_BIN_LOG_HDR),
			pu4Data,
			u4PhyIcsBufSize);

	/* write to ring, ret: written */
	ret = kalIcsWrite(pucBuf, u4Size);
	if (ret != u4Size) {
		DBGLOG_LIMITED(NIC, ERROR,
			"dropped written:%d write\t"
			"PHY ICS log into file fail\n",
			ret);
		goto exit;
	}
exit:

	if (u4PhyIcsEventBuf)
		kalMemFree(u4PhyIcsEventBuf, VIR_MEM_TYPE, u4PhyIcsBufSize);
	if (pucBuf)
		kalMemFree(pucBuf, VIR_MEM_TYPE, u4Size);

	return;

}
#endif/* #if (CFG_SUPPORT_PHY_ICS_SW_FLOW_VER3 == 1) */

void nicExtEventPhyIcsRawData(struct ADAPTER *prAdapter,
			   uint8_t *pucEventBuf)
{

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	struct UNI_EVENT_PHY_ICS_DUMP_RAW_DATA *prPhyIcsEvent;
#else
	struct EXT_EVENT_PHY_ICS_DUMP_DATA_T *prPhyIcsEvent;
#endif

	struct ICS_BIN_LOG_HDR *prIcsBinLogHeader;
	void *pvPacket = NULL;
	uint32_t u4Size = 0, Idxi = 0;
	uint8_t *pucRecvBuff = NULL;
	ssize_t ret;

	if (!prAdapter) {
		DBGLOG(RFTEST, ERROR, "prAdapter is null\n");
		return;
	}

	if (pucEventBuf == NULL) {
		DBGLOG(RFTEST, ERROR, "pucEventBuf is null\n");
		return;
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	prPhyIcsEvent = (struct UNI_EVENT_PHY_ICS_DUMP_RAW_DATA *)
				pucEventBuf;
#else
	prPhyIcsEvent = (struct EXT_EVENT_PHY_ICS_DUMP_DATA_T *)
				pucEventBuf;
#endif

	if (prPhyIcsEvent->u4DataLen > MAX_PHY_ICS_DUMP_DATA_CNT) {
		DBGLOG(RFTEST, ERROR,
			"u4DataLen %d out of valid event length!\n",
			prPhyIcsEvent->u4DataLen);
		return;
	}

	DBGLOG(RFTEST, INFO,
	       "u4FuncIndex = %d, u4PktNum = [%d], u4PhyTimestamp = [0x%08x], u4DataLen = [%d]\n",
	       prPhyIcsEvent->u4FuncIndex,
	       prPhyIcsEvent->u4PktNum,
	       prPhyIcsEvent->u4PhyTimestamp,
	       prPhyIcsEvent->u4DataLen);

	/* check u4Size overflow before using it */
	if (checkMulOverflow(prPhyIcsEvent->u4DataLen, sizeof(uint32_t))) {
		DBGLOG(RFTEST, ERROR,
			"u4DataLen %d mul overflow!\n",
			prPhyIcsEvent->u4DataLen);
		return;
	}
	if (checkAddOverflow(prPhyIcsEvent->u4DataLen * sizeof(uint32_t),
		sizeof(struct ICS_BIN_LOG_HDR))) {
		DBGLOG(RFTEST, ERROR,
			"u4DataLen %d add overflow!\n",
			prPhyIcsEvent->u4DataLen);
		return;
	}
	/* 1KB phy ics packet + fw parser header */
	u4Size = prPhyIcsEvent->u4DataLen * sizeof(uint32_t) +
			sizeof(struct ICS_BIN_LOG_HDR);
	pvPacket = kalPacketAlloc(prAdapter->prGlueInfo, u4Size,
			FALSE, &pucRecvBuff);

#if 0
	/* Print ICap data to console for debugging purpose */
	for (Idxi = 0; Idxi < 256; Idxi++)
		DBGLOG(RFTEST, ERROR, "Data[%d] : %08x\n", Idxi,
			prPhyIcsEvent->u4Data[Idxi]);
#endif

    /* endian swap */
	for (Idxi = 0; Idxi < MAX_PHY_ICS_DUMP_DATA_CNT; Idxi++) {
		prPhyIcsEvent->u4Data[Idxi] =
			((prPhyIcsEvent->u4Data[Idxi] & 0x000000FF) << 24)
			| ((prPhyIcsEvent->u4Data[Idxi] & 0x0000FF00) << 8)
			| ((prPhyIcsEvent->u4Data[Idxi] & 0x00FF0000) >> 8)
			| ((prPhyIcsEvent->u4Data[Idxi] & 0xFF000000) >> 24);
	}


	if (pvPacket) {
		/* prepare ICS header */
		prIcsBinLogHeader = (struct ICS_BIN_LOG_HDR *)pucRecvBuff;
		prIcsBinLogHeader->u4MagicNum = ICS_BIN_LOG_MAGIC_NUM;
		prIcsBinLogHeader->u4Timestamp = prPhyIcsEvent->u4PhyTimestamp;
		prIcsBinLogHeader->u2MsgID = RX_PKT_TYPE_PHY_ICS;
		prIcsBinLogHeader->u2Length =
			prPhyIcsEvent->u4DataLen * sizeof(uint32_t);

		/* prepare ICS frame
		 * pucRecvBuff = ICS Header + AGG Header + AGG payload length
		 * skip ICS header of pucRecvBuff, start to next address copy
		 */
		kalMemCopy(pucRecvBuff + sizeof(struct ICS_BIN_LOG_HDR),
				prPhyIcsEvent->u4Data,
				prPhyIcsEvent->u4DataLen * sizeof(uint32_t));

		/* write to ring, ret: written */
		ret = kalIcsWrite(pucRecvBuff, u4Size);
		if (ret != u4Size)
			DBGLOG_LIMITED(NIC, ERROR,
				"dropped written:%ld write PHY ICS log into file fail\n",
				ret);

		kalPacketFree(prAdapter->prGlueInfo, pvPacket);
	}

}
#endif /* #if (CFG_SUPPORT_PHY_ICS == 1) */

#if CFG_SUPPORT_QA_TOOL

void nicExtEventICapIQData(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf)
{
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prICapEvent;
	uint32_t Idxi = 0, Idxj = 0, Idxk = 0;
	uint32_t u4MaxWFCnt = 0;
	struct _RBIST_IQ_DATA_T *prIQArray = NULL;
	struct ICAP_INFO_T *prIcapInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	prICapEvent = (struct EXT_EVENT_RBIST_DUMP_DATA_T *)
		    pucEventBuf;

	prIcapInfo = &prAdapter->rIcapInfo;
	prIQArray = prIcapInfo->prIQArray;

	if (prIQArray == NULL) {
		DBGLOG(RFTEST, ERROR, "prIQArray is NULL\n");
		return;
	}

	/* If we receive the packet which is delivered from
	 * last time data-capure, we need to drop it.
	 */
	DBGLOG(RFTEST, INFO, "u4PktNum = [%d], u4DataLength = [%d]\n",
						prICapEvent->u4PktNum,
						prICapEvent->u4DataLength);

	if (prICapEvent->u4PktNum > prIcapInfo->u4ICapEventCnt) {
		if (prICapEvent->u4DataLength == 0)
			prIcapInfo->eIcapState = ICAP_STATE_FW_DUMP_DONE;

		DBGLOG(RFTEST, ERROR,
		       "Packet out of order: Pkt num %d, EventCnt %d\n",
		       prICapEvent->u4PktNum, prIcapInfo->u4ICapEventCnt);
		return;
	}

	if (prICapEvent->u4WFCnt > MAX_ANTENNA_NUM
		|| prICapEvent->u4WFCnt > MAX_IQ_ARRAY_WF_CNT) {
		DBGLOG(RFTEST, WARN,
		       "u4WFCnt is larger than Max Ant Num\n");
		return;
	}

	if (prICapEvent->u4SmplCnt >
		(ICAP_EVENT_DATA_SAMPLE / NUM_OF_CAP_TYPE)) {
		DBGLOG(RFTEST, WARN,
		       "u4SmplCnt is larger than buffer size\n");
		return;
	}

	DBGLOG(RFTEST, INFO,
	       "u4SmplCnt = [%d], u4WFCnt = [%d], IQArrayIndex = [%d]\n",
	       prICapEvent->u4SmplCnt,
	       prICapEvent->u4WFCnt,
	       prIcapInfo->u4IQArrayIndex);

	if (prICapEvent->u4DataLength != 0 &&
	    prICapEvent->u4SmplCnt * prICapEvent->u4WFCnt *
	    NUM_OF_CAP_TYPE > ICAP_EVENT_DATA_SAMPLE) {
		/* Max count = Total ICAP_EVENT_DATA_SAMPLE count
		 * and cut into half (I/Q)
		 */
		prICapEvent->u4SmplCnt = ICAP_EVENT_DATA_SAMPLE /
					 NUM_OF_CAP_TYPE;
		DBGLOG(RFTEST, WARN,
		       "u4SmplCnt is larger than buffer size\n");
	}

	/* Check the max count of WFCnt*/
	u4MaxWFCnt = ARRAY_SIZE(prIQArray[0].u4IQArray);
	if (prICapEvent->u4WFCnt > u4MaxWFCnt) {
		DBGLOG(RFTEST, ERROR,
		       "Too many WFCnt[%u > %u] from FW, skip rest of them\n",
		       prICapEvent->u4WFCnt, u4MaxWFCnt);
		return;
	}

	if (prIcapInfo->u4IQArrayIndex + prICapEvent->u4SmplCnt >
	    MAX_ICAP_IQ_DATA_CNT) {
		DBGLOG(RFTEST, ERROR,
		       "Too many packets from FW, skip rest of them\n");
		return;
	}

	for (Idxi = 0; Idxi < prICapEvent->u4SmplCnt; Idxi++) {
		for (Idxj = 0; Idxj < prICapEvent->u4WFCnt; Idxj++) {
			prIQArray[prIcapInfo->u4IQArrayIndex].
				    u4IQArray[Idxj][CAP_I_TYPE] =
					prICapEvent->u4Data[Idxk++];
			prIQArray[prIcapInfo->u4IQArrayIndex].
				    u4IQArray[Idxj][CAP_Q_TYPE] =
					prICapEvent->u4Data[Idxk++];
		}
		prIcapInfo->u4IQArrayIndex++;
	}

	/* Print ICap data to console for debugging purpose */
	for (Idxi = 0; Idxi < prICapEvent->u4SmplCnt; Idxi++)
		if (prICapEvent->u4Data[Idxi] == 0)
			DBGLOG(RFTEST, WARN, "Data[%d] : %x\n", Idxi,
				prICapEvent->u4Data[Idxi]);


	/* Update ICapEventCnt */
	if (prICapEvent->u4DataLength != 0)
		prIcapInfo->u4ICapEventCnt++;

	/* Check whether is the last FW event or not */
	if ((prICapEvent->u4DataLength == 0)
	    && (prICapEvent->u4PktNum == prIcapInfo->u4ICapEventCnt)) {
		/* Reset ICapEventCnt */
		prAdapter->rIcapInfo.eIcapState = ICAP_STATE_FW_DUMP_DONE;
		prIcapInfo->u4ICapEventCnt = 0;
		DBGLOG(INIT, INFO, ": ==> gen done_file\n");
	} else
		prAdapter->rIcapInfo.eIcapState = ICAP_STATE_FW_DUMPING;

}

#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
void nicExtCmdEventSolicitICapIQData(struct ADAPTER *prAdapter,
					struct CMD_INFO *prCmdInfo,
					uint8_t *pucEventBuf)
{
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prICapEvent;
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prTmpICapEvent = NULL;
	struct ICAP_INFO_T *prIcapInfo = NULL;
	struct RBIST_DUMP_IQ_T *prQAICapInfo = NULL;
	uint32_t Idxi = 0;
	uint32_t u4WfNum;
	uint32_t u4IQType;
	int32_t *pData;

	if (!prAdapter) {
		DBGLOG(RFTEST, ERROR, "prAdapter is null\n");
		return;
	}
	if (!pucEventBuf) {
		DBGLOG(RFTEST, ERROR, "pucEventBuf is null\n");
		return;
	}

	/* like jedi: UniEventRfTestSolicitICapIQDataCb */
	prICapEvent = (struct EXT_EVENT_RBIST_DUMP_DATA_T *)pucEventBuf;

	prIcapInfo = &prAdapter->rIcapInfo;
	prQAICapInfo = &prAdapter->QAICapInfo;
	prTmpICapEvent = &prAdapter->IcapDumpEvent;

	/* prRbistDump pointer to QATool's answer */
	u4WfNum = prQAICapInfo->u4WfNum;
	u4IQType = prQAICapInfo->u4IQType;
	pData = prQAICapInfo->pIcapData;

	DBGLOG(RFTEST, INFO, "u4WfNum=%d, u4IQType=%d\n",
						u4WfNum, u4IQType);
	DBGLOG(RFTEST, INFO, "u4PktNum = [%d], u4DataLength = [%d]\n",
				prICapEvent->u4PktNum,
				prICapEvent->u4DataLength);

	prTmpICapEvent->u4DataLength = prICapEvent->u4DataLength;


	/* If we receive the packet which is delivered from
	 * last time data-capure, we need to drop it.
	 */
	if (prICapEvent->u4PktNum > prIcapInfo->u4ICapEventCnt) {
		if (prICapEvent->u4DataLength == 0)
			prIcapInfo->eIcapState = ICAP_STATE_FW_DUMP_DONE;

		DBGLOG(RFTEST, ERROR,
			   "Packet out of order: Pkt num %d, EventCnt %d\n",
			   prICapEvent->u4PktNum, prIcapInfo->u4ICapEventCnt);
		return;
	}


	/* Store I/Q data to data buffer */
	for (Idxi = 0; Idxi < prICapEvent->u4DataLength; Idxi++)
		pData[Idxi] = prICapEvent->u4Data[Idxi];


	/* Update ICapEventCnt */
	if (prICapEvent->u4DataLength != 0)
		prIcapInfo->u4ICapEventCnt++;

	/* Check whether is the last FW event or not */
	if ((prICapEvent->u4DataLength == 0)
	    && (prICapEvent->u4PktNum == prIcapInfo->u4ICapEventCnt)) {

		DBGLOG(INIT, INFO,
			": ==> Dump data done, total pkt cnts=%d!!\n",
			prIcapInfo->u4ICapEventCnt);

		/* Reset ICapEventCnt */
		prIcapInfo->u4ICapEventCnt = 0;
	}

}
#endif /* #if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1) */
#endif

uint32_t nicRfTestEventHandler(struct ADAPTER *prAdapter,
			       struct WIFI_EVENT *prEvent)
{
	uint32_t u4QueryInfoLen = 0;
	struct EXT_EVENT_RF_TEST_RESULT_T *prResult;
	struct mt66xx_chip_info *prChipInfo = NULL;
#if CFG_SUPPORT_QA_TOOL
	struct EXT_EVENT_RBIST_CAP_STATUS_T *prCapStatus;
	struct ATE_OPS_T *prAteOps = NULL;
#endif
	struct ICAP_INFO_T *prIcapInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);
#if CFG_SUPPORT_QA_TOOL
	prAteOps = prChipInfo->prAteOps;
	ASSERT(prAteOps);
#endif
	prIcapInfo = &prAdapter->rIcapInfo;

	prResult = (struct EXT_EVENT_RF_TEST_RESULT_T *)
		   prEvent->aucBuffer;
	DBGLOG(RFTEST, INFO, "%s funcID = %d\n",
			__func__,
	       prResult->u4FuncIndex);
	switch (prResult->u4FuncIndex) {
#if CFG_SUPPORT_QA_TOOL
	case GET_ICAP_CAPTURE_STATUS:
		u4QueryInfoLen = sizeof(struct
					EXT_EVENT_RBIST_CAP_STATUS_T);
		prCapStatus = (struct EXT_EVENT_RBIST_CAP_STATUS_T *)
			      prEvent->aucBuffer;

		DBGLOG(RFTEST, INFO, "%s iCapDone = %d , icap state=%d\n",
				__func__,
		       prCapStatus->u4CapDone,
		       prAdapter->rIcapInfo.eIcapState);
		if (prCapStatus->u4CapDone &&
		    prIcapInfo->eIcapState != ICAP_STATE_FW_DUMP_DONE) {
			wlanoidRfTestICapRawDataProc(prAdapter,
			 0 /*prCapStatus->u4CapStartAddr*/,
			 0 /*prCapStatus->u4TotalBufferSize*/);
			prIcapInfo->eIcapState = ICAP_STATE_FW_DUMPING;
		}
		break;
	case GET_ICAP_RAW_DATA:
		if (prAteOps->getRbistDataDumpEvent) {
			prAteOps->getRbistDataDumpEvent(prAdapter,
						prEvent->aucBuffer);
			if (prIcapInfo->eIcapState != ICAP_STATE_FW_DUMP_DONE)
				wlanoidRfTestICapRawDataProc(prAdapter,
				0 /*prCapStatus->u4CapStartAddr*/,
				0 /*prCapStatus->u4TotalBufferSize*/);
		}
		break;
#endif
#if (CFG_SUPPORT_PHY_ICS == 1)
	case GET_PHY_ICS_RAW_DATA:
		nicExtEventPhyIcsRawData(prAdapter, prEvent->aucBuffer);
		break;
#endif

	case RE_CALIBRATION:
		nicExtEventReCalData(prAdapter, prEvent->aucBuffer);
		break;

	default:
		DBGLOG(RFTEST, WARN, "Unknown rf test event, ignore\n");
		break;
	}

	return u4QueryInfoLen;
}

void nicEventLayer0ExtMagic(struct ADAPTER *prAdapter,
			    struct WIFI_EVENT *prEvent)
{
	uint32_t u4QueryInfoLen = 0;
	struct CMD_INFO *prCmdInfo = NULL;
	uint8_t ucSeqNum = prEvent->ucSeqNum;

	log_dbg(NIC, TRACE, "prEvent->ucExtenEID = %x\n", prEvent->ucExtenEID);

	switch (prEvent->ucExtenEID) {
	case EXT_EVENT_ID_CMD_RESULT:
		u4QueryInfoLen = sizeof(struct
					PARAM_CUSTOM_EFUSE_BUFFER_MODE);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);
		break;

	case EXT_EVENT_ID_EFUSE_ACCESS:
	{
		struct EVENT_ACCESS_EFUSE *prEventEfuseAccess;

		u4QueryInfoLen = sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);
		prEventEfuseAccess = (struct EVENT_ACCESS_EFUSE *) (
					     prEvent->aucBuffer);

		/* Efuse block size 16 */
		kalMemCopy(prAdapter->aucEepromVaule,
			   prEventEfuseAccess->aucData, 16);
		break;
	}

	case EXT_EVENT_ID_RF_TEST:
		u4QueryInfoLen = nicRfTestEventHandler(prAdapter, prEvent);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);
		break;

	case EXT_EVENT_ID_GET_TX_POWER:
	{
		struct EXT_EVENT_GET_TX_POWER *prEventGetTXPower;

		u4QueryInfoLen = sizeof(struct PARAM_CUSTOM_GET_TX_POWER);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);
		prEventGetTXPower = (struct EXT_EVENT_GET_TX_POWER *) (
					    prEvent->aucBuffer);

		prAdapter->u4GetTxPower =
			prEventGetTXPower->ucTx0TargetPower;
		break;
	}

	case EXT_EVENT_ID_EFUSE_FREE_BLOCK:
	{
		struct EXT_EVENT_EFUSE_FREE_BLOCK *prEventGetFreeBlock;
		struct PARAM_CUSTOM_EFUSE_FREE_BLOCK *prFreeBlock;

		u4QueryInfoLen = sizeof(struct
					PARAM_CUSTOM_EFUSE_FREE_BLOCK);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);
		prEventGetFreeBlock = (struct EXT_EVENT_EFUSE_FREE_BLOCK *)
				      (prEvent->aucBuffer);

		if (prCmdInfo != NULL) {
			prFreeBlock = (struct PARAM_CUSTOM_EFUSE_FREE_BLOCK *)
				prCmdInfo->pvInformationBuffer;
			prFreeBlock->ucGetFreeBlock =
				prEventGetFreeBlock->ucFreeBlockNum;
			prFreeBlock->ucGetTotalBlock =
				prEventGetFreeBlock->ucTotalBlockNum;
		}

		prAdapter->u4FreeBlockNum =
			prEventGetFreeBlock->ucFreeBlockNum;
		break;
	}

#if CFG_SUPPORT_TX_BF
	case EXT_EVENT_ID_BF_STATUS_READ:
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);
		if (prCmdInfo != NULL && prCmdInfo->pfCmdDoneHandler) {
			struct EXT_EVENT_BF_STATUS_T *prExtBfStatus =
			(struct EXT_EVENT_BF_STATUS_T *)prEvent->aucBuffer;

			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prExtBfStatus->aucBuf);
		}
		break;
#endif

	case EXT_EVENT_ID_MAX_AMSDU_LENGTH_UPDATE:
	{
		struct EXT_EVENT_MAX_AMSDU_LENGTH_UPDATE *prEventAmsdu;
		struct STA_RECORD *prStaRec;
		uint8_t ucStaRecIndex;

		prEventAmsdu = (struct EXT_EVENT_MAX_AMSDU_LENGTH_UPDATE *)
			(prEvent->aucBuffer);

		ucStaRecIndex = secGetStaIdxByWlanIdx(
			prAdapter, prEventAmsdu->ucWlanIdx);
		if (ucStaRecIndex == STA_REC_INDEX_NOT_FOUND)
			break;

		prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIndex);
		if (!prStaRec)
			break;

		if (prStaRec->ucMaxMpduCount == 0 ||
		    prStaRec->ucMaxMpduCount > prEventAmsdu->ucAmsduLen)
			prStaRec->ucMaxMpduCount = prEventAmsdu->ucAmsduLen;

		DBGLOG(NIC, INFO,
		       "Amsdu update event ucWlanIdx[%u] ucLen[%u] ucMaxMpduCount[%u]\n",
		       prEventAmsdu->ucWlanIdx, prEventAmsdu->ucAmsduLen,
		       prStaRec->ucMaxMpduCount);
		break;
	}
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	case EXT_EVENT_ID_THERMAL_PROTECT:
	{
		thrmProtEventHandler(prAdapter, prEvent->aucBuffer);
		break;
	}
#endif
#if CFG_SUPPORT_WIFI_SYSDVT
	case EXT_EVENT_ID_SYSDVT_TEST:
	{
		u4QueryInfoLen = sizeof(struct SYSDVT_CTRL_EXT_T);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);
		break;
	}
#endif	/* CFG_SUPPORT_WIFI_SYSDVT */

#if (CFG_SUPPORT_802_11AX == 1)
	case EXT_EVENT_ID_SR_INFO:
	{
		struct _SR_EVENT_T *prEventSr;

		prEventSr = (struct _SR_EVENT_T *) (prEvent->aucBuffer);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);

		switch (prEventSr->u1EventSubId) {
		case SR_EVENT_GET_SR_CAP_ALL_INFO:
		{
			struct _SR_EVENT_SR_CAP_T *prEventSrCap;

			u4QueryInfoLen = sizeof(struct _SR_EVENT_SR_CAP_T);
			prEventSrCap = (struct _SR_EVENT_SR_CAP_T *) (
							prEvent->aucBuffer);

			DBGLOG(NIC, STATE, "========== SR CAP ==========\n");
			DBGLOG(NIC, STATE, "fgSrEn = %d \t"
				"fgSrgEn = %d\n",
				prEventSrCap->rSrCap.fgSrEn,
				prEventSrCap->rSrCap.fgSrgEn);

			DBGLOG(NIC, STATE, "fgNonSrgEn = %d \t"
				"fgSingleMdpuRtsctsEn = %d\n",
				prEventSrCap->rSrCap.fgNonSrgEn,
				prEventSrCap->rSrCap.fgSingleMdpuRtsctsEn);

			DBGLOG(NIC, STATE, "fgHdrDurEn = %d \t"
				"fgTxopDurEn = %d\n",
				prEventSrCap->rSrCap.fgHdrDurEn,
				prEventSrCap->rSrCap.fgTxopDurEn);

			DBGLOG(NIC, STATE, "fgNonSrgInterPpduPresv = %d \t"
				"fgSrgInterPpduPresv = %d\n",
				prEventSrCap->rSrCap.fgNonSrgInterPpduPresv,
				prEventSrCap->rSrCap.fgSrgInterPpduPresv);

			DBGLOG(NIC, STATE, "fgSrRemTimeEn = %d \t"
				"fgProtInSrWinDis = %d\n",
				prEventSrCap->rSrCap.fgSrRemTimeEn,
				prEventSrCap->rSrCap.fgProtInSrWinDis);

			DBGLOG(NIC, STATE, "fgTxCmdDlRateSelEn = %d \t"
				"fgAmpduTxCntEn = %d\n",
				prEventSrCap->rSrCap.fgTxCmdDlRateSelEn,
				prEventSrCap->rSrCap.fgAmpduTxCntEn);
			DBGLOG(NIC, STATE, "============================\n");
			break;
		}

		case SR_EVENT_GET_SR_IND_ALL_INFO:
		{
			struct _SR_EVENT_SR_IND_T *prEventSrInd;

			u4QueryInfoLen = sizeof(struct _SR_EVENT_SR_IND_T);
			prEventSrInd = (struct _SR_EVENT_SR_IND_T *) (
							prEvent->aucBuffer);

			DBGLOG(NIC, STATE, "======== SR INDICATOR ========\n");
			DBGLOG(NIC, STATE, "NonSrgInterPpduRcpi = %x \t"
				"SrgInterPpduRcpi = %x\n",
				prEventSrInd->rSrInd.u1NonSrgInterPpduRcpi,
				prEventSrInd->rSrInd.u1SrgInterPpduRcpi);

			DBGLOG(NIC, STATE, "NonSrgVldCnt = %x \t"
				"SrgVldCnt = %x\n",
				prEventSrInd->rSrInd.u2NonSrgVldCnt,
				prEventSrInd->rSrInd.u2SrgVldCnt);

			DBGLOG(NIC, STATE, "IntraBssPpduCnt = %x \t"
				"InterBssPpduCnt = %x\n",
				prEventSrInd->rSrInd.u2IntraBssPpduCnt,
				prEventSrInd->rSrInd.u2InterBssPpduCnt);

			DBGLOG(NIC, STATE, "NonSrgPpduVldCnt = %x \t"
				"SrgPpduVldCnt = %x\n",
				prEventSrInd->rSrInd.u2NonSrgPpduVldCnt,
				prEventSrInd->rSrInd.u2SrgPpduVldCnt);

			DBGLOG(NIC, STATE, "SrAmpduMpduCnt = %x \t"
				"SrAmpduMpduAckedCnt = %x\n",
				prEventSrInd->rSrInd.u4SrAmpduMpduCnt,
				prEventSrInd->rSrInd.u4SrAmpduMpduAckedCnt);
			DBGLOG(NIC, STATE, "==========================\n");
			break;
		}

		default:
			break;
		}
		break;
	}
#endif

	default:
		break;
	}

	if (prCmdInfo != NULL) {
		if ((prCmdInfo->fgIsOid) != 0)
			kalOidComplete(prAdapter->prGlueInfo,
				prCmdInfo,
				u4QueryInfoLen, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
#if (CFG_SUPPORT_TXPOWER_INFO == 1)
	else if ((prEvent->ucExtenEID) ==
		 EXT_EVENT_ID_TX_POWER_FEATURE_CTRL) {
		u4QueryInfoLen = sizeof(struct
					PARAM_TXPOWER_ALL_RATE_POWER_INFO_T);
		/* command response handling */
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);

		if (prCmdInfo != NULL) {
			if (prCmdInfo->pfCmdDoneHandler)
				prCmdInfo->pfCmdDoneHandler(prAdapter,
					prCmdInfo, prEvent->aucBuffer);
			else if (prCmdInfo->fgIsOid)
				kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo,
				  u4QueryInfoLen, WLAN_STATUS_SUCCESS);

			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	}
#endif
	else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_MAC_INFO) {
		u4QueryInfoLen = sizeof(struct EXT_EVENT_MAC_INFO_T);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);

		if (prCmdInfo != NULL) {
			if (prCmdInfo->pfCmdDoneHandler) {
				prCmdInfo->pfCmdDoneHandler(
					prAdapter,
					prCmdInfo,
					prEvent->aucBuffer
				);
			} else if ((prCmdInfo->fgIsOid) != 0)
				kalOidComplete(
					prAdapter->prGlueInfo,
					prCmdInfo,
					u4QueryInfoLen,
					WLAN_STATUS_SUCCESS);
			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	} else if (prEvent->ucExtenEID == EXT_EVENT_ID_DUMP_MEM) {
		u4QueryInfoLen = sizeof(struct EXT_CMD_EVENT_DUMP_MEM_T);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);

		if (prCmdInfo != NULL) {
			if (prCmdInfo->pfCmdDoneHandler) {
				prCmdInfo->pfCmdDoneHandler(
					prAdapter,
					prCmdInfo,
					prEvent->aucBuffer
				);
			} else if ((prCmdInfo->fgIsOid) != 0)
				kalOidComplete(
					prAdapter->prGlueInfo,
					prCmdInfo,
					u4QueryInfoLen,
					WLAN_STATUS_SUCCESS);
			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	} else if (prEvent->ucExtenEID == EXT_EVENT_ID_SER) {
		u4QueryInfoLen = sizeof(struct PARAM_SER_INFO_T);

		prCmdInfo = nicGetPendingCmdInfo(prAdapter, ucSeqNum);

		if (prCmdInfo != NULL) {
			if (prCmdInfo->pfCmdDoneHandler)
				prCmdInfo->pfCmdDoneHandler(
					prAdapter,
					prCmdInfo,
					prEvent->aucBuffer
				);
			else if ((prCmdInfo->fgIsOid) != 0)
				kalOidComplete(
					prAdapter->prGlueInfo,
					prCmdInfo,
					u4QueryInfoLen,
					WLAN_STATUS_SUCCESS);

			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	}
}

void nicEventMicErrorInfo(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent)
{
	struct EVENT_MIC_ERR_INFO *prMicError;
	/* P_PARAM_AUTH_EVENT_T prAuthEvent; */
	struct STA_RECORD *prStaRec;

	DBGLOG(RSN, EVENT, "EVENT_ID_MIC_ERR_INFO\n");

	prMicError = (struct EVENT_MIC_ERR_INFO *) (
			     prEvent->aucBuffer);
	prStaRec = aisGetDefaultStaRecOfAP(prAdapter);
	ASSERT(prStaRec);

	if (prStaRec)
		rsnTkipHandleMICFailure(prAdapter, prStaRec,
					(u_int8_t) prMicError->u4Flags);
	else
		DBGLOG(RSN, INFO, "No STA rec!!\n");
#if 0
	prAuthEvent = (struct PARAM_AUTH_EVENT *)
		      prAdapter->aucIndicationEventBuffer;

	/* Status type: Authentication Event */
	prAuthEvent->rStatus.eStatusType =
		ENUM_STATUS_TYPE_AUTHENTICATION;

	/* Authentication request */
	prAuthEvent->arRequest[0].u4Length = sizeof(
			struct PARAM_AUTH_REQUEST);
	kalMemCopy((void *) prAuthEvent->arRequest[0].arBssid,
		   (void *) prWlanInfo->rCurrBssId.arMacAddress,
		   PARAM_MAC_ADDR_LEN);

	if (prMicError->u4Flags != 0)
		prAuthEvent->arRequest[0].u4Flags =
			PARAM_AUTH_REQUEST_GROUP_ERROR;
	else
		prAuthEvent->arRequest[0].u4Flags =
			PARAM_AUTH_REQUEST_PAIRWISE_ERROR;

	kalIndicateStatusAndComplete(
		prAdapter->prGlueInfo,
		WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
		(void *) prAuthEvent,
		sizeof(struct PARAM_STATUS_INDICATION) + sizeof(
		struct PARAM_AUTH_REQUEST));
#endif
}

void nicEventScanDone(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent)
{
	scnEventScanDone(prAdapter,
			 (struct EVENT_SCAN_DONE *) (prEvent->aucBuffer), TRUE);
}

void nicEventSchedScanDone(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	DBGLOG(INIT, INFO, "EVENT_ID_SCHED_SCAN_DONE\n");
	scnEventSchedScanDone(prAdapter,
		(struct EVENT_SCHED_SCAN_DONE *) (prEvent->aucBuffer));
}

void nicEventSleepyNotify(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent)
{
#if !defined(_HIF_USB)
	struct EVENT_SLEEPY_INFO *prEventSleepyNotify;

	prEventSleepyNotify = (struct EVENT_SLEEPY_INFO *) (
				      prEvent->aucBuffer);

	prAdapter->fgWiFiInSleepyState = (u_int8_t) (
			prEventSleepyNotify->ucSleepyState);

#if CFG_SUPPORT_MULTITHREAD
	if (prEventSleepyNotify->ucSleepyState)
		kalSetFwOwnEvent2Hif(prAdapter->prGlueInfo);
#endif
#endif
}

void nicEventBtOverWifi(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
#if CFG_ENABLE_BT_OVER_WIFI
	uint8_t aucTmp[sizeof(struct BT_OVER_WIFI_EVENT) + sizeof(
					     struct BOW_LINK_DISCONNECTED)];
	struct EVENT_BT_OVER_WIFI *prEventBtOverWifi;
	struct BT_OVER_WIFI_EVENT *prBowEvent;
	struct BOW_LINK_CONNECTED *prBowLinkConnected;
	struct BOW_LINK_DISCONNECTED *prBowLinkDisconnected;

	prEventBtOverWifi = (struct EVENT_BT_OVER_WIFI *) (
				    prEvent->aucBuffer);

	/* construct event header */
	prBowEvent = (struct BT_OVER_WIFI_EVENT *) aucTmp;

	if (prEventBtOverWifi->ucLinkStatus == 0) {
		/* Connection */
		prBowEvent->rHeader.ucEventId = BOW_EVENT_ID_LINK_CONNECTED;
		prBowEvent->rHeader.ucSeqNumber = 0;
		prBowEvent->rHeader.u2PayloadLength = sizeof(
				struct BOW_LINK_CONNECTED);

		/* fill event body */
		prBowLinkConnected = (struct BOW_LINK_CONNECTED *) (
					     prBowEvent->aucPayload);
		prBowLinkConnected->rChannel.ucChannelNum =
			prEventBtOverWifi->ucSelectedChannel;
		kalMemZero(prBowLinkConnected->aucPeerAddress,
			   MAC_ADDR_LEN);	/* @FIXME */

		kalIndicateBOWEvent(prAdapter->prGlueInfo, prBowEvent);
	} else {
		/* Disconnection */
		prBowEvent->rHeader.ucEventId =
			BOW_EVENT_ID_LINK_DISCONNECTED;
		prBowEvent->rHeader.ucSeqNumber = 0;
		prBowEvent->rHeader.u2PayloadLength = sizeof(
				struct BOW_LINK_DISCONNECTED);

		/* fill event body */
		prBowLinkDisconnected = (struct BOW_LINK_DISCONNECTED *) (
						prBowEvent->aucPayload);
		prBowLinkDisconnected->ucReason = 0;	/* @FIXME */
		kalMemZero(prBowLinkDisconnected->aucPeerAddress,
			   MAC_ADDR_LEN);	/* @FIXME */

		kalIndicateBOWEvent(prAdapter->prGlueInfo, prBowEvent);
	}
#endif
}

void nicEventStatistics(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();
	kalMemCopy(&prAdapter->rStatStruct, prEvent->aucBuffer,
		   sizeof(struct EVENT_STATISTICS));

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				prCmdInfo,
				0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

void nicEventTputFactorHandler(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
#define EVENT_HDR_SIZE              OFFSET_OF(struct WIFI_EVENT, aucBuffer)
#define TPUT_FACTOR_DUMP_LINE_SIZE  16

	struct TPUT_FACTOR_LIST_T *pFactorList;
	struct TPUT_SUB_FACTOR_T *pFactor;
	uint8_t *pucCont;
	uint32_t u4EvtLen;
	uint32_t u4FIdx;
	uint32_t u4ContIdx;
	uint8_t ucContTemp[TPUT_FACTOR_DUMP_LINE_SIZE];
	uint32_t *pu4ContCR;

	pFactorList = (struct TPUT_FACTOR_LIST_T *)prEvent->aucBuffer;
	pFactor = (struct TPUT_SUB_FACTOR_T *)&pFactorList->Cont[0];
	pucCont = &pFactorList->Cont[0];

	u4EvtLen = prEvent->u2PacketLength - EVENT_HDR_SIZE;
	if (u4EvtLen <= 8)
		return;

	/* skip u4EvtId & u4FactorType */
	u4EvtLen -= 8;

/* ucExtenEID: TPUT_EVENT_EXTEND_ID_GEN (0)
 *           & TPUT_EVENT_EXTEND_ID_CR (1)
 * u4EvtId:    TPUT_EVENT_START (0),
 *             TPUT_EVENT_ASSOCD (1) ~
 */
	DBGLOG(INIT, WARN, "tputf> |FGROUP|%d|FCASE|%d|VER|%d|ALLLEN|%d|\n",
		   prEvent->ucExtenEID,
		   pFactorList->u4EvtId,
		   pFactorList->u4Ver,
		   u4EvtLen);

	if (prEvent->ucExtenEID == 1) {
		/* skip u4EvtId & u4FactorType in BKRS event, not GEN event */
		pFactor->u2Len -= 8;
	}

	while (u4EvtLen > 0) {
		/* for BKRS event, ucRes = number of report events
		 * for latest one, we will set BIT7 to 1,
		 *                 EX: 9th event, ucRes = 0x89
		 */
		DBGLOG(INIT, WARN, "tputf> |FTAG|%d|FLEN|%d|FRES|%d|\n",
		       pFactor->ucTag, pFactor->u2Len, pFactor->ucRes);

#if 0
		DBGLOG(INIT, WARN,
				"tputf> debug= %d %d 0x%x %x, 0x%x %x %x %x\n",
				pFactorList,
				u4EvtLen,
				&pFactor->ucTag,
				&pFactorList->Cont[0],
				pFactorList->Cont[0], pFactorList->Cont[1],
				pFactorList->Cont[2], pFactorList->Cont[3]);
#endif

	if ((pFactor->u2Len <= 4) || (pFactor->u2Len > u4EvtLen))
		break;

	pFactor->u2Len -= 4; /* 4: tag/len/res */
	u4ContIdx = 0;

	for (u4FIdx = 0;
		 u4FIdx < pFactor->u2Len;
		 u4FIdx += TPUT_FACTOR_DUMP_LINE_SIZE) {
		if ((u4FIdx+TPUT_FACTOR_DUMP_LINE_SIZE)
			> pFactor->u2Len) {
			kalMemZero(ucContTemp, sizeof(ucContTemp));

			if ((pFactor->u2Len > u4FIdx) &&
				((pFactor->u2Len-u4FIdx)
				 <= TPUT_FACTOR_DUMP_LINE_SIZE)) {
#if 0
				DBGLOG(INIT, WARN,
					"tputf> debug: last=%d %d\n",
					u4FIdx, pFactor->u2Len - u4FIdx);
#endif
				kalMemCopy(ucContTemp,
							&pFactor->Cont[u4FIdx],
							pFactor->u2Len-u4FIdx);
			}
		} else {
			kalMemCopy(ucContTemp, &pFactor->Cont[u4FIdx], 16);
		}

		/* dump content byte by byte */
		if (prEvent->ucExtenEID == 0) {
			DBGLOG(INIT, WARN,
				"tputf> FACTOR: %03d %02x %02x %02x %02x %02x %02x %02x %02x\n",
				u4ContIdx++,
				ucContTemp[0], ucContTemp[1],
				ucContTemp[2], ucContTemp[3],
				ucContTemp[4], ucContTemp[5],
				ucContTemp[6], ucContTemp[7]);

			DBGLOG(INIT, WARN,
				"tputf> FACTOR: %03d %02x %02x %02x %02x %02x %02x %02x %02x\n",
				u4ContIdx++,
				ucContTemp[8], ucContTemp[9],
				ucContTemp[10], ucContTemp[11],
				ucContTemp[12], ucContTemp[13],
				ucContTemp[14], ucContTemp[15]);

		} else {
			pu4ContCR = (uint32_t *)ucContTemp;

			DBGLOG(INIT, WARN,
				"tputf> FACTOR: %03d %08x %08x %08x %08x\n",
				u4ContIdx++,
				pu4ContCR[0], pu4ContCR[1],
				pu4ContCR[2], pu4ContCR[3]);
			}
		}

		/* recover real length */
		pFactor->u2Len += 4; /* 4: tag/len/res */

#if 0
		DBGLOG(INIT, WARN,
			"tputf> debug: u4EvtLen %d %d\n",
			u4EvtLen, pFactor->u2Len);
#endif

		if (u4EvtLen > pFactor->u2Len)
			u4EvtLen -= pFactor->u2Len;
		else
			break;

		pucCont += pFactor->u2Len;
		pFactor = (struct TPUT_SUB_FACTOR_T *)pucCont;
	}

	DBGLOG(INIT, WARN, "tputf> FEND\n");
}

void nicEventWlanInfo(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();
	kalMemCopy(&prAdapter->rEventWlanInfo, prEvent->aucBuffer,
		   sizeof(struct EVENT_WLAN_INFO));

	DBGLOG(RSN, INFO, "EVENT_ID_WTBL_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
							prCmdInfo,
							0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

void nicEventMibInfo(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;


	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();

	DBGLOG(RSN, INFO, "EVENT_ID_MIB_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				prCmdInfo,
				0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is to decide if the beacon time out is reasonable
*                by the TRX and some known factors
*
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return true	if the beacon timeout event is valid after policy checking
*         false	if the beacon timeout event needs to be ignored
*/
/*----------------------------------------------------------------------------*/
bool nicBeaconTimeoutFilterPolicy(struct ADAPTER *prAdapter,
	uint8_t ucBcnTimeoutReason, uint8_t *ucDisconnectReason,
	uint8_t ucBssIdx)
{
	struct RX_CTRL	*prRxCtrl;
	struct TX_CTRL	*prTxCtrl;
	OS_SYSTIME	u4CurrentTime;
	bool		bValid = true;
	uint32_t	u4MonitorWindow;
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

	if (ucBssIdx >= MAX_BSSID_NUM) {
		DBGLOG(NIC, ERROR, "ucBssIdx out of range!\n");
		return FALSE;
	}

	ASSERT(prAdapter);
	u4MonitorWindow = CFG_BEACON_TIMEOUT_FILTER_DURATION_DEFAULT_VALUE;

	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	prTxCtrl = &prAdapter->rTxCtrl;
	ASSERT(prTxCtrl);

	GET_BOOT_SYSTIME(&u4CurrentTime);

	DBGLOG(NIC, INFO,
			"u4MonitorWindow: %d, u4CurrentTime: %d, u4LastRxTime: %d, u4LastUnicastRxTime: %d, u4LastTxTime: %d",
			u4MonitorWindow, u4CurrentTime,
			prRxCtrl->u4LastRxTime[ucBssIdx],
			prRxCtrl->u4LastUnicastRxTime[ucBssIdx],
			prTxCtrl->u4LastTxTime[ucBssIdx]);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(RSN, ERROR, "prBssInfo is null\n");
		return WLAN_STATUS_FAILURE;
	}

#if CFG_SUPPORT_DFS
	if (prBssInfo->CSAParams.u4MaxSwitchTime != 0) {
		DBGLOG(RSN, INFO,
			"DFS/Extra CSA is on-going, time:%d, filter out BTO event\n",
			prBssInfo->CSAParams.u4MaxSwitchTime);
		return FALSE;
	}
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1) && defined(CFG_SUPPORT_UNIFIED_COMMAND)
	if (ucBcnTimeoutReason != UNI_ENUM_BCN_MLINK_NULL_FRAME_THRESHOLD) {
		DBGLOG(ML, INFO, "Only single link BTO reason=%d",
			ucBcnTimeoutReason);
		return FALSE;
	}
#endif

	if (IS_BSS_AIS(prBssInfo)) {
		if (!CHECK_FOR_TIMEOUT(u4CurrentTime,
			prRxCtrl->u4LastRxTime[ucBssIdx],
			SEC_TO_SYSTIME(MSEC_TO_SEC(u4MonitorWindow)))) {
			/* Policy 1, if RX in the past duration (in ms) */
			if (aisBeaconTimeoutFilterPolicy(
					prAdapter, ucBssIdx)) {
				DBGLOG(NIC, INFO, "Driver find better TX AP");
#if (CFG_EXT_ROAMING == 1)
				*ucDisconnectReason =
				       DISCONNECT_REASON_CODE_RADIO_LOST;
#else
				*ucDisconnectReason =
				       DISCONNECT_REASON_CODE_RADIO_LOST_TX_ERR;
#endif
			} else {
				DBGLOG(NIC, INFO, "RX in the past duration");
				bValid = false;
			}
		}
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (IS_BSS_P2P(prBssInfo)) {
		if (!CHECK_FOR_TIMEOUT(u4CurrentTime,
			prRxCtrl->u4LastRxTime[ucBssIdx],
			SEC_TO_SYSTIME(MSEC_TO_SEC(u4MonitorWindow)))) {
			DBGLOG(NIC, INFO,
				"Policy 1 hit, RX in the past duration");
			bValid = false;
		}
	}
#endif /* CFG_ENABLE_WIFI_DIRECT */

	DBGLOG(NIC, INFO, "valid beacon time out event?: %d", bValid);

	return bValid;
}

void nicEventBeaconTimeout(struct ADAPTER *prAdapter,
			   struct WIFI_EVENT *prEvent)
{
	DBGLOG(NIC, INFO, "EVENT_ID_BSS_BEACON_TIMEOUT\n");

	if (prAdapter->fgDisBcnLostDetection == FALSE) {
		struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
		struct EVENT_BSS_BEACON_TIMEOUT *prEventBssBeaconTimeout;

		prEventBssBeaconTimeout = (struct EVENT_BSS_BEACON_TIMEOUT
					   *) (prEvent->aucBuffer);

		if (prEventBssBeaconTimeout->ucBssIndex >=
		    prAdapter->ucSwBssIdNum ||
		    prEventBssBeaconTimeout->ucBssIndex >=
		    MAX_BSSID_NUM) {
			DBGLOG(NIC, ERROR, "ucBssIndex out of range!\n");
			return;
		}

		DBGLOG(NIC, INFO, "Reason code: %d\n",
		       prEventBssBeaconTimeout->ucReasonCode);
/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS
		prAdapter->total_beacon_timeout_count++;
		if (prEventBssBeaconTimeout->ucReasonCode >=
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
			UNI_ENUM_BCN_TIMEOUT_REASON_MAX_NUM) {
#else
			BEACON_TIMEOUT_REASON_NUM) {
#endif
			DBGLOG(RX, WARN, "Invaild Beacon Timeout Reason: %d\n",
				prEventBssBeaconTimeout->ucReasonCode);
		} else {
			prAdapter->beacon_timeout_count
				[prEventBssBeaconTimeout->ucReasonCode]++;
		}
#endif /* fos_change end */

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prEventBssBeaconTimeout->ucBssIndex);
		if (!prBssInfo) {
			DBGLOG(RSN, ERROR, "prBssInfo is null\n");
			return;
		}

		if (IS_BSS_AIS(prBssInfo)) {
			uint8_t ucDisconnectReason =
				DISCONNECT_REASON_CODE_RADIO_LOST;

			if (nicBeaconTimeoutFilterPolicy(prAdapter,
				prEventBssBeaconTimeout->ucReasonCode,
				&ucDisconnectReason,
				prBssInfo->ucBssIndex))
				aisBssBeaconTimeout_impl(prAdapter,
					prEventBssBeaconTimeout->ucReasonCode,
					ucDisconnectReason,
					prBssInfo->ucBssIndex);
		}
#if CFG_ENABLE_WIFI_DIRECT
		else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
			uint8_t ucDisconnectReason =
				DISCONNECT_REASON_CODE_RADIO_LOST;

			if (nicBeaconTimeoutFilterPolicy(prAdapter,
					prEventBssBeaconTimeout->ucReasonCode,
					&ucDisconnectReason,
					prEventBssBeaconTimeout->ucBssIndex))
				p2pRoleFsmRunEventBeaconTimeout(prAdapter,
					prBssInfo);
		}
#endif
#if CFG_ENABLE_BT_OVER_WIFI
		else if (GET_BSS_INFO_BY_INDEX(prAdapter,
			prEventBssBeaconTimeout->ucBssIndex)->eNetworkType ==
			NETWORK_TYPE_BOW) {
			/* ToDo:: Nothing */
		}
#endif
		else {
			DBGLOG(RX, ERROR,
			       "EVENT_ID_BSS_BEACON_TIMEOUT: (ucBssIndex = %d)\n",
			       prEventBssBeaconTimeout->ucBssIndex);
		}
	}

}

#if CFG_ENABLE_WIFI_DIRECT
void nicEventUpdateNoaParams(struct ADAPTER *prAdapter,
			     struct WIFI_EVENT *prEvent)
{
	if (prAdapter->fgIsP2PRegistered) {
		struct EVENT_UPDATE_NOA_PARAMS *prEventUpdateNoaParam;

		prEventUpdateNoaParam = (struct EVENT_UPDATE_NOA_PARAMS *) (
						prEvent->aucBuffer);

		if (GET_BSS_INFO_BY_INDEX(prAdapter,
				prEventUpdateNoaParam->ucBssIndex) == NULL) {
			DBGLOG(NIC, ERROR, "prBssInfo is null\n");
			return;
		}
		if (GET_BSS_INFO_BY_INDEX(prAdapter,
				prEventUpdateNoaParam->ucBssIndex)->eNetworkType
					== NETWORK_TYPE_P2P) {

			p2pProcessEvent_UpdateNOAParam(prAdapter,
				prEventUpdateNoaParam->ucBssIndex,
				prEventUpdateNoaParam);
		} else {
			ASSERT(0);
		}
	}
}

void nicEventStaAgingTimeout(struct ADAPTER *prAdapter,
			     struct WIFI_EVENT *prEvent)
{
	struct EVENT_STA_AGING_TIMEOUT *prEventStaAgingTimeout;
	struct STA_RECORD *prStaRec;

	if (prAdapter->fgDisStaAgingTimeoutDetection ||
	    prAdapter->fgIsP2PRegistered == FALSE) {
		DBGLOG(NIC, INFO,
			"fgDisStaAgingTimeoutDetection=%d fgIsP2PRegistered=%d\n",
			prAdapter->fgDisStaAgingTimeoutDetection,
			prAdapter->fgIsP2PRegistered);
		return;
	}

	prEventStaAgingTimeout = (struct EVENT_STA_AGING_TIMEOUT *)
		prEvent->aucBuffer;

	prStaRec = cnmGetStaRecByIndex(prAdapter,
				       prEventStaAgingTimeout->ucStaRecIdx);
	if (prStaRec == NULL) {
		DBGLOG(NIC, ERROR, "prStaRec is null\n");
		return;
	}

	if (!IS_NET_PWR_STATE_ACTIVE(prAdapter,
		prStaRec->ucBssIndex)) {
		DBGLOG(NIC, ERROR, "bss is not active\n");
		return;
	}

	p2pRoleFsmRunEventAgingTimeout(prAdapter, prStaRec);
}

void nicEventApObssStatus(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent)
{
	if (prAdapter->fgIsP2PRegistered)
		rlmHandleObssStatusEventPkt(prAdapter,
			(struct EVENT_AP_OBSS_STATUS *) prEvent->aucBuffer);
}
#endif

void nicEventRoamingStatus(struct ADAPTER *prAdapter,
			   struct WIFI_EVENT *prEvent)
{
#if CFG_SUPPORT_ROAMING
	struct CMD_ROAMING_TRANSIT *prTransit;

	prTransit = (struct CMD_ROAMING_TRANSIT *) (prEvent->aucBuffer);
	roamingFsmProcessEvent(prAdapter, prTransit);
#endif /* CFG_SUPPORT_ROAMING */
}

void nicEventSendDeauth(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
	struct SW_RFB rSwRfb;

	DBGLOG(NIC, INFO, "%s\n", __func__);
#if DBG
	struct WLAN_MAC_HEADER *prWlanMacHeader;

	prWlanMacHeader = (struct WLAN_MAC_HEADER *)prEvent->aucBuffer;
	DBGLOG(RX, TRACE, "nicRx: aucAddr1: " MACSTR "\n",
	       MAC2STR(prWlanMacHeader->aucAddr1));
	DBGLOG(RX, TRACE, "nicRx: aucAddr2: " MACSTR "\n",
	       MAC2STR(prWlanMacHeader->aucAddr2));
#endif

	/* receive packets without StaRec */
	rSwRfb.pvHeader = (struct WLAN_MAC_HEADER *)prEvent->aucBuffer;
	if (authSendDeauthFrame(prAdapter, NULL, NULL, &rSwRfb,
			REASON_CODE_CLASS_3_ERR,
			(PFN_TX_DONE_HANDLER) NULL) == WLAN_STATUS_SUCCESS) {

		DBGLOG(RX, ERROR, "Send Deauth Error\n");
	}
}

void nicEventUpdateRddStatus(struct ADAPTER *prAdapter,
			     struct WIFI_EVENT *prEvent)
{
#if CFG_SUPPORT_RDD_TEST_MODE
	struct EVENT_RDD_STATUS *prEventRddStatus;

	prEventRddStatus = (struct EVENT_RDD_STATUS *) (
				   prEvent->aucBuffer);

	prAdapter->ucRddStatus = prEventRddStatus->ucRddStatus;
#endif
}

void nicEventUpdateBwcsStatus(struct ADAPTER *prAdapter,
			      struct WIFI_EVENT *prEvent)
{
	DBGLOG(RSN, EVENT, "not support");
}

void nicEventUpdateBcmDebug(struct ADAPTER *prAdapter,
			    struct WIFI_EVENT *prEvent)
{
	struct PTA_IPC *prEventBwcsStatus;

	prEventBwcsStatus = (struct PTA_IPC *) (prEvent->aucBuffer);

#if CFG_SUPPORT_BCM_BWCS_DEBUG
	DBGLOG(RSN, EVENT, "BCM FW status: %02x%02x%02x%02x\n",
	       prEventBwcsStatus->u.aucBTPParams[0],
	       prEventBwcsStatus->u.aucBTPParams[1],
	       prEventBwcsStatus->u.aucBTPParams[2],
	       prEventBwcsStatus->u.aucBTPParams[3]);
#endif
}

void nicEventAddPkeyDone(struct ADAPTER *prAdapter,
			 struct WIFI_EVENT *prEvent)
{
	struct EVENT_ADD_KEY_DONE_INFO *prKeyDone;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t ucKeyId;

	prKeyDone = (struct EVENT_ADD_KEY_DONE_INFO *) (
			    prEvent->aucBuffer);

	DBGLOG(RSN, INFO, "EVENT_ID_ADD_PKEY_DONE BSSIDX=%d " MACSTR
	       "\n",
	       prKeyDone->ucBSSIndex, MAC2STR(prKeyDone->aucStaAddr));

	prStaRec = cnmGetStaRecByAddress(prAdapter,
					 prKeyDone->ucBSSIndex,
					 prKeyDone->aucStaAddr);

	if (!prStaRec) {
		ucKeyId =
			aisGetAisSpecBssInfo(prAdapter,
			prKeyDone->ucBSSIndex)->ucKeyAlgorithmId;
		if ((ucKeyId == CIPHER_SUITE_WEP40)
		    || (ucKeyId == CIPHER_SUITE_WEP104)) {
			DBGLOG(RX, INFO, "WEP, ucKeyAlgorithmId= %d\n",
				ucKeyId);
			prStaRec = cnmGetStaRecByAddress(prAdapter,
					prKeyDone->ucBSSIndex,
					prAdapter->rWifiVar.arBssInfoPool[
					prKeyDone->ucBSSIndex].aucBSSID);
			if (!prStaRec) {
				DBGLOG(RX, INFO,
					"WEP, AddPKeyDone, ucBSSIndex %d, Addr "
					MACSTR ", StaRec is NULL\n",
					prKeyDone->ucBSSIndex,
					MAC2STR(prAdapter->rWifiVar
					.arBssInfoPool[prKeyDone->
					ucBSSIndex].aucBSSID));
			}
		} else {
			DBGLOG(RX, INFO,
			       "AddPKeyDone, ucBSSIndex %d, Addr "
			       MACSTR ", StaRec is NULL\n",
			       prKeyDone->ucBSSIndex,
			       MAC2STR(prKeyDone->aucStaAddr));
		}
	}
	if (prStaRec) {
		DBGLOG(RSN, INFO, "STA " MACSTR " Add Key Done!!\n",
		       MAC2STR(prStaRec->aucMacAddr));
		prStaRec->fgIsTxKeyReady = TRUE;
		qmUpdateStaRec(prAdapter, prStaRec);
	}

	if (prAdapter->fgIsPostponeTxEAPOLM3) {
		prAdapter->fgIsPostponeTxEAPOLM3 = FALSE;
		DBGLOG(RX, INFO,
			"[Passpoint] PTK is installed and ready!\n");
	}
}

void nicEventDebugMsg(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent)
{
	struct EVENT_DEBUG_MSG *prEventDebugMsg;
	uint8_t ucMsgType;
	uint16_t u2MsgSize;
	uint8_t *pucMsg;

	prEventDebugMsg = (struct EVENT_DEBUG_MSG *)(
				  prEvent->aucBuffer);
	if (!prEventDebugMsg) {
		DBGLOG(RSN, WARN, "prEventDebugMsg is NULL\n");
		return;
	}

	ucMsgType = prEventDebugMsg->ucMsgType;
	u2MsgSize = prEventDebugMsg->u2MsgSize;
	pucMsg = prEventDebugMsg->aucMsg;

#if CFG_SUPPORT_QA_TOOL
	if (ucMsgType == DEBUG_MSG_TYPE_ASCII) {
		if (kalStrnCmp("[RECAL DUMP START]", pucMsg, 18) == 0) {
			prAdapter->rReCalInfo.fgDumped = TRUE;
			return;
		} else if (kalStrnCmp("[RECAL DUMP END]", pucMsg, 16) == 0) {
			prAdapter->rReCalInfo.fgDumped = TRUE;
			return;
		} else if (prAdapter->rReCalInfo.fgDumped &&
				  kalStrnCmp("[Recal]", pucMsg, 7) == 0) {
			struct WIFI_EVENT *prTmpEvent;
			struct EXT_EVENT_RECAL_DATA_T *prCalData;
			uint32_t u4Size = sizeof(struct WIFI_EVENT) +
					  sizeof(struct EXT_EVENT_RECAL_DATA_T);

			prTmpEvent = (struct WIFI_EVENT *)
				kalMemAlloc(u4Size, VIR_MEM_TYPE);

			if (prTmpEvent == NULL) {
				DBGLOG(RFTEST, ERROR,
					"Unable to alloc memory for prTmpEvent\n");
				return;
			}
			kalMemZero(prTmpEvent, u4Size);

			prCalData = (struct EXT_EVENT_RECAL_DATA_T *)
						    prTmpEvent->aucBuffer;
			if (prCalData) {
				prCalData->u4FuncIndex = RE_CALIBRATION;
				prCalData->u4Type = 0;
				/* format: [XXXXXXXX][YYYYYYYY]ZZZZZZZZ */
				kalMemCopy(prCalData->u.ucData, pucMsg + 7, 28);
				nicRfTestEventHandler(prAdapter, prTmpEvent);
			}
			kalMemFree(prTmpEvent, VIR_MEM_TYPE, u4Size);
		}
	}
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_SAVE == 1)
	if (prAdapter->rWifiVar.fgFwIdxLogSave != FW_IDX_LOG_SAVE_DISABLE)
		kalIndexWrite(pucMsg, u2MsgSize);
	if (prAdapter->rWifiVar.fgFwIdxLogSave == FW_IDX_LOG_SAVE_ONLY)
		return;
#endif

#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgFwIdxLogTrans))
		wlanFwLogIdxToStr(prAdapter, pucMsg, u2MsgSize);
	else
#endif
	{
		wlanPrintFwLog(pucMsg, u2MsgSize, ucMsgType, NULL);
	}
}

#if CFG_SUPPORT_TDLS
void nicEventTdls(struct ADAPTER *prAdapter,
		  struct WIFI_EVENT *prEvent)
{
	TdlsexEventHandle(prAdapter->prGlueInfo,
			  (uint8_t *)prEvent->aucBuffer,
			  (uint32_t)(prEvent->u2PacketLength - 8));
}
#endif

void nicEventRssiMonitor(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	int32_t rssi = 0;

	kalMemCopy(&rssi, prEvent->aucBuffer, sizeof(int32_t));
	DBGLOG(RX, TRACE, "EVENT_ID_RSSI_MONITOR value=%d\n", rssi);
	kalVendorEventRssiBeyondRange(prAdapter->prGlueInfo,
		aisGetDefaultLinkBssIndex(prAdapter), rssi);
}

void nicEventDumpMem(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	DBGLOG(SW4, INFO, "%s: EVENT_ID_DUMP_MEM\n", __func__);

	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		DBGLOG(NIC, INFO, ": ==> 1\n");
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				prCmdInfo, 0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	} else {
		/* Burst mode */
		DBGLOG(NIC, INFO, ": ==> 2\n");
	}
}

#if (CFG_CE_ASSERT_DUMP == 1)
void nicEventAssertDump(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	char aucVersionBuf[128];
	char aucLineEnd[] = {'\n', '\0'};
	char *pucLineEndHead;
	uint16_t u2VerBufLen = 0;
	uint16_t u2VerStrLen = 0;
	uint16_t u2BufSize = 0;

	prChipInfo = prAdapter->chip_info;

	if (wlanIsChipRstRecEnabled(prAdapter))
		wlanChipRstPreAct(prAdapter);

	if (prEvent->ucS2DIndex == S2D_INDEX_EVENT_N2H) {
		if (!prAdapter->fgN9AssertDumpOngoing) {
			DBGLOG(NIC, ERROR,
				"%s: EVENT_ID_ASSERT_DUMP\n", __func__);
			DBGLOG(NIC, ERROR,
			       "\n[DUMP_N9]====N9 ASSERT_DUMPSTART====\n");
			prAdapter->fgKeepPrintCoreDump = TRUE;

			prAdapter->fgN9AssertDumpOngoing = TRUE;
			/* Add FW version in coredump header*/
			kalMemZero(aucVersionBuf, sizeof(aucVersionBuf));
			u2VerBufLen = sizeof(aucVersionBuf) - sizeof('\0');

			/* Concat prefix string */
			kalStrnCat(aucVersionBuf, ";", u2VerBufLen);
			u2VerBufLen -= kalStrLen(";");

			/* Concat Mainfest string */
			kalStrnCat(aucVersionBuf,
				prAdapter->rVerInfo.aucReleaseManifest,
				u2VerBufLen);

			/* Force write '\n\0' as string terminator */
			u2VerStrLen = kalStrLen(aucVersionBuf);
			if (u2VerStrLen <= sizeof(aucVersionBuf) -
				sizeof(aucLineEnd))
				pucLineEndHead = aucVersionBuf + u2VerStrLen;
			else
				pucLineEndHead = aucVersionBuf +
					sizeof(aucVersionBuf) -
					sizeof(aucLineEnd);
			kalMemCopy(pucLineEndHead,
				aucLineEnd, sizeof(aucLineEnd));

			if (kalEnqCoreDumpLog(prAdapter,
					aucVersionBuf, kalStrLen(aucVersionBuf))
					!= WLAN_STATUS_SUCCESS) {
				DBGLOG(NIC, ERROR,
						"Add FW version in core dump header fail\n");
			}

			wlanCorDumpTimerInit(prAdapter);
		}
		if (prAdapter->fgN9AssertDumpOngoing) {
			u2BufSize = prEvent->u2PacketLength
						- prChipInfo->event_hdr_size;
			if (prAdapter->fgKeepPrintCoreDump)
				DBGLOG(NIC, ERROR, "[DUMP_N9]%s:\n",
					prEvent->aucBuffer);
			if (!kalStrnCmp(prEvent->aucBuffer,
					";more log added here", 5)
			    || !kalStrnCmp(prEvent->aucBuffer,
					";;[CONNSYS] coredump start", 26))
				prAdapter->fgKeepPrintCoreDump = FALSE;

			if (kalEnqCoreDumpLog(prAdapter,
				prEvent->aucBuffer, u2BufSize)
				!= WLAN_STATUS_SUCCESS) {
				DBGLOG(NIC, ERROR,
						"kalEnqCoreDumpLog fail\n");
			}

			if (kalStrStr(prEvent->aucBuffer,
					";coredump end")) {
				DBGLOG(NIC, ERROR,
					"core dump end, trigger whole chip reset\n");
				prAdapter->fgN9AssertDumpOngoing = FALSE;
				cnmTimerStopTimer(prAdapter,
						  &prAdapter->rN9CorDumpTimer);
				GL_DEFAULT_RESET_TRIGGER(prAdapter,
						RST_FW_ASSERT);
			}

			wlanCorDumpTimerReset(prAdapter);
		}
	} else {
		/* prEvent->ucS2DIndex == S2D_INDEX_EVENT_C2H */
		DBGLOG(NIC, ERROR,
				"%s: Skip CR4 Dump Handle\n", __func__);
	}
}
#endif

void nicEventRddSendPulse(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent)
{
	DBGLOG(RLM, INFO, "%s: EVENT_ID_RDD_SEND_PULSE\n",
	       __func__);

	nicEventRddPulseDump(prAdapter, prEvent->aucBuffer);
}

void nicEventUpdateCoexPhyrate(struct ADAPTER *prAdapter,
			       struct WIFI_EVENT *prEvent)
{
	uint8_t i, j;
	struct EVENT_UPDATE_COEX_PHYRATE *prEventUpdateCoexPhyrate;

	ASSERT(prAdapter);

	DBGLOG(NIC, LOUD, "%s\n", __func__);

	prEventUpdateCoexPhyrate = (struct EVENT_UPDATE_COEX_PHYRATE
				    *)(prEvent->aucBuffer);

	/* This event indicates HW BSS, need to convert to SW BSS */
	for (i = 0; i < (prAdapter->ucHwBssIdNum + 1); i++) {
		for (j = 0; j < (MAX_BSSID_NUM + 1); j++) {
			if (prAdapter->aprBssInfo[j]->ucOwnMacIndex == i) {
				prAdapter->aprBssInfo[j]->u4CoexPhyRateLimit =
				  prEventUpdateCoexPhyrate->au4PhyRateLimit[i];

				DBGLOG_LIMITED(NIC, INFO,
				  "Coex:BSS[%d]R:%d, OwnMacID:%d\n", j,
				  prAdapter->aprBssInfo[j]->u4CoexPhyRateLimit,
				  prAdapter->aprBssInfo[j]->ucOwnMacIndex);
			}
		}
	}

	prAdapter->ucSmarGearSupportSisoOnly =
		prEventUpdateCoexPhyrate->ucSupportSisoOnly;
	prAdapter->ucSmartGearWfPathSupport =
		prEventUpdateCoexPhyrate->ucWfPathSupport;

	DBGLOG_LIMITED(NIC, INFO, "Smart Gear SISO:%d, WF:%d\n",
	       prAdapter->ucSmarGearSupportSisoOnly,
	       prAdapter->ucSmartGearWfPathSupport);
}

void nicEventUpdateCoexStatus(struct ADAPTER *prAdapter,
			      struct WIFI_EVENT *prEvent)
{
	struct EVENT_COEX_STATUS *prEventCoexStatus;
	struct STA_RECORD *prStaRec;
	struct BSS_DESC *prBssDesc;
	struct BSS_INFO *prBssInfo;
	struct CMD_ADDBA_REJECT rAddBaReject = {0};
	enum ENUM_COEX_MODE eCoexMode = COEX_NONE_BT;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucAisIndex;
	bool fgIsBAND2G4Coex = FALSE;
	bool fgHitBlockList = FALSE;

	ASSERT(prAdapter);

	DBGLOG(NIC, LOUD, "%s\n", __func__);

	prEventCoexStatus = (struct EVENT_COEX_STATUS *)(prEvent->aucBuffer);

	eCoexMode = prEventCoexStatus->ucCoexMode;
	fgIsBAND2G4Coex = prEventCoexStatus->fgIsBAND2G4Coex;

#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	prAdapter->fgIsNeedAvoidDesenseFreq =
		!!(prEventCoexStatus->ucBtOnOff &&
		!prEventCoexStatus->fgIs5GsupportEPA);
	DBGLOG(NIC, TRACE, "Avoid desense frequency[%d]\n",
		prAdapter->fgIsNeedAvoidDesenseFreq);
#endif

	DBGLOG(NIC, TRACE, "[BTon:%d BTPrf:0x%x BTRssi=%d Mode:%d Flag:%d]\n",
	       prEventCoexStatus->ucBtOnOff,
	       prEventCoexStatus->u2BtProfile,
	       prEventCoexStatus->ucBtRssi,
	       prEventCoexStatus->ucCoexMode,
	       prEventCoexStatus->fgIsBAND2G4Coex);
	/*AIS only feature*/
	for (ucAisIndex = 0; ucAisIndex < KAL_AIS_NUM; ucAisIndex++) {
		uint8_t ucBssIndex;

		if (!AIS_MAIN_BSS_INFO(prAdapter, ucAisIndex))
			continue;

		ucBssIndex = AIS_MAIN_BSS_INDEX(prAdapter, ucAisIndex);
		prBssInfo = aisGetAisBssInfo(prAdapter, ucBssIndex);
		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
		prStaRec = aisGetStaRecOfAP(prAdapter, ucBssIndex);
		fgHitBlockList = bssIsIotAp(prAdapter, prBssDesc,
					    WLAN_IOT_AP_COEX_DIS_RX_AMPDU);

#if (CFG_SUPPORT_APS == 1)
		aisGetApsInfo(prAdapter, ucBssIndex)->fgIsGBandCoex =
			!!(prEventCoexStatus->u2BtProfile & 0x0045);
#endif

		if (!prBssInfo || !prStaRec)
			return;
		/**
		 * 1. Previouse mode is not TDD
		 * 2. Coex mode in Event = TDD
		 * 3. CoexFlag in Event is TRUE
		 * 4. Bss Band is 2G
		 * 5. BssDesc hit Blocklist
		 */
		if (prBssInfo->eCoexMode != COEX_TDD_MODE &&
		    eCoexMode == COEX_TDD_MODE &&
		    fgIsBAND2G4Coex == TRUE &&
		    prBssInfo->eBand == BAND_2G4 &&
		    fgHitBlockList == TRUE) {
			/*Set Rx BA size=1*/
			prAdapter->rWifiVar.ucRxHtBaSize = 1;
			prAdapter->rWifiVar.ucRxVhtBaSize = 1;
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
			rAddBaReject.fgEnable = TRUE;
			rAddBaReject.fgApply = TRUE;
			rStatus = wlanSendSetQueryCmd(prAdapter,
				CMD_ID_ADDBA_REJECT,
				TRUE, FALSE, FALSE, NULL, NULL,
				sizeof(struct CMD_ADDBA_REJECT),
				(uint8_t *) &rAddBaReject, NULL, 0);
			DBGLOG(NIC, INFO, "Set Rx BA size=1 [%u]\n", rStatus);
		} else if (prBssInfo->eCoexMode == COEX_TDD_MODE &&
			   eCoexMode != COEX_TDD_MODE &&
			   prBssInfo->eBand == BAND_2G4 &&
			   fgHitBlockList == TRUE) {
			/*restore Tx BA size setting*/
			prAdapter->rWifiVar.ucRxHtBaSize =
				WLAN_LEGACY_MAX_BA_SIZE;
			prAdapter->rWifiVar.ucRxVhtBaSize =
				WLAN_LEGACY_MAX_BA_SIZE;
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
			rAddBaReject.fgEnable = TRUE;
			rAddBaReject.fgApply = TRUE;
			rStatus = wlanSendSetQueryCmd(prAdapter,
				CMD_ID_ADDBA_REJECT,
				TRUE, FALSE, FALSE, NULL, NULL,
				sizeof(struct CMD_ADDBA_REJECT),
				(uint8_t *) &rAddBaReject, NULL, 0);
			DBGLOG(NIC, INFO, "Reset Rx BA size [%u]\n", rStatus);
		}
		/*Record current coex mode to Ais BssInfo*/
		prBssInfo->eCoexMode = eCoexMode;
	}

#if CFG_ENABLE_WIFI_DIRECT && (CFG_SUPPORT_AVOID_DESENSE == 1)
	p2pFuncSwitchSapChannel(prAdapter, P2P_BT_COEX_SCENARIO);
#endif
}


void nicCmdEventQueryCnmInfo(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct PARAM_GET_CNM_T *prCnmInfoQuery = NULL;
	struct PARAM_GET_CNM_T *prCnmInfoEvent = NULL;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	if (prCmdInfo->fgIsOid) {
		prCnmInfoQuery = (struct PARAM_GET_CNM_T *)
				 prCmdInfo->pvInformationBuffer;
		prCnmInfoEvent = (struct PARAM_GET_CNM_T *)pucEventBuf;
		kalMemCopy(prCnmInfoQuery, prCnmInfoEvent,
			   sizeof(struct PARAM_GET_CNM_T));

		prGlueInfo = prAdapter->prGlueInfo;
		u4QueryInfoLen = sizeof(struct PARAM_GET_CNM_T);
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicEventCoexCtrl(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo,
					0,
					WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

void nicEventCnmInfo(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
							prCmdInfo,
							0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

void nicEventReportUEvent(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct EVENT_REPORT_U_EVENT *prEventData;

	prEventData = (struct EVENT_REPORT_U_EVENT *) (prEvent->aucBuffer);

	if (prEventData != NULL) {
		DBGLOG(NIC, TRACE, "UEvent: %s\n",
		prEventData->aucData);
		kalSendUevent(prAdapter, prEventData->aucData);
	}
}

#if CFG_SUPPORT_REPLAY_DETECTION
void nicCmdEventDetectReplayInfo(struct ADAPTER *prAdapter,
		uint8_t ucKeyId, uint8_t ucKeyType, uint8_t ucBssIdx)
{
	struct GL_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIdx))
		return;

	/* AIS only */
	if (!ucKeyType && ucKeyId < 4) {
		/* Only save data broadcast key info.
		*  ucKeyType == 1 means unicast key
		*  ucKeyId == 4 or ucKeyId == 5 means it is a PMF key
		*/
		prDetRplyInfo = aisGetDetRplyInfo(prAdapter, ucBssIdx);

		prDetRplyInfo->ucCurKeyId = ucKeyId;
		prDetRplyInfo->ucKeyType = ucKeyType;
		prDetRplyInfo->arReplayPNInfo[ucKeyId].fgRekey = TRUE;
		prDetRplyInfo->arReplayPNInfo[ucKeyId].fgFirstPkt = TRUE;
		DBGLOG(NIC, TRACE,
			"[%d] Keyid is %d, ucKeyType is %d\n",
			ucBssIdx, ucKeyId, ucKeyType);
	}
}

void nicCmdEventSetAddKey(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_CMD *prWifiCmd = NULL;
	struct CMD_802_11_KEY *prCmdKey = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo,
			       prCmdInfo->u4InformationBufferLength,
			       WLAN_STATUS_SUCCESS);
	}

	prGlueInfo = prAdapter->prGlueInfo;

	if (pucEventBuf) {
		prWifiCmd = (struct WIFI_CMD *) (pucEventBuf);
		prCmdKey = (struct CMD_802_11_KEY *) (pucEventBuf);

		nicCmdEventDetectReplayInfo(prAdapter, prCmdKey->ucKeyId,
			prCmdKey->ucKeyType, prCmdKey->ucBssIdx);
	}
}
void nicOidCmdTimeoutSetAddKey(struct ADAPTER *prAdapter,
			       struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	DBGLOG(NIC, WARN, "Wlan setaddkey timeout.\n");
	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       0, WLAN_STATUS_FAILURE);

	if (prAdapter->fgIsPostponeTxEAPOLM3)
		prAdapter->fgIsPostponeTxEAPOLM3 = FALSE;
}
#endif
#if (CFG_WOW_SUPPORT == 1)
void nicEventWowWakeUpReason(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	struct EVENT_WOW_WAKEUP_REASON_INFO *prWakeUpReason;
	struct GLUE_INFO *prGlueInfo;

	DBGLOG(NIC, INFO, "nicEventWakeUpReason\n");
	prGlueInfo = prAdapter->prGlueInfo;

	/* Driver receives EVENT_ID_WOW_WAKEUP_REASON after fw wake up host
	 * The possible Wakeup Reason define in FW as following:
	 * (Orignal design re-use _ENUM_PF_TYPE_T in firmware
	 *    --> keep the value the same as 7668/7663.)
	 * 0:  MAGIC PACKET
	 * 3:  GTK_REKEY
	 * 8:  DISCONNECT
	 * 9:  IPV4_UDP PACKET
	 * 10: IPV4_TCP PACKET
	 * 11: IPV6_UDP PACKET
	 * 12: IPV6_TCP PACKET
	 * 13: BEACON LOST
	 * 14: IPV6_ICMP PACKET
	 */
	prWakeUpReason =
		(struct EVENT_WOW_WAKEUP_REASON_INFO *) (prEvent->aucBuffer);
	prGlueInfo->prAdapter->rWowCtrl.ucReason = prWakeUpReason->reason;
	DBGLOG(NIC, INFO, "nicEventWakeUpReason:%d\n",
		prGlueInfo->prAdapter->rWowCtrl.ucReason);
}
#endif

#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
void nicEventChannelTime(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_CHANNEL_TIMING_INFO *ChanTime;
	struct GLUE_INFO *prGlueInfo;

	prGlueInfo = prAdapter->prGlueInfo;
	ChanTime = (struct EVENT_CHANNEL_TIMING_INFO *) (prEvent->aucBuffer);

	kalMemCopy(prGlueInfo->rChanTimeRecord,
				   ChanTime,
				   sizeof(struct EVENT_CHANNEL_TIMING_INFO));
}
#endif

#if CFG_SUPPORT_802_PP_DSCB
void nicEventUpdateStaticPPDscb(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_UPDATE_PP_DSCB *prEvtStaticPPDscb;
	struct BSS_INFO *prBssInfo;

	if (!prAdapter)
		return;

	prEvtStaticPPDscb =
			(struct EVENT_UPDATE_PP_DSCB *) (prEvent->aucBuffer);

	if (prEvtStaticPPDscb->ucBssIndex > MAX_BSSID_NUM)
		return;

	prBssInfo = prAdapter->aprBssInfo[prEvtStaticPPDscb->ucBssIndex];

	if ((!prBssInfo) || (!IS_BSS_ACTIVE(prBssInfo)))
		return;

	DBGLOG(NIC, INFO,
		"[STATIC_PP_DSCB][EVENT] ucBssIndex=%d, fgIsDscbEnable=%d, u2DscbBitmap=%d\n",
				prEvtStaticPPDscb->ucBssIndex,
				prEvtStaticPPDscb->fgIsDscbEnable,
				prEvtStaticPPDscb->u2DscbBitmap);

	prBssInfo->fgIsEhtDscbPresent = prEvtStaticPPDscb->fgIsDscbEnable;
	prBssInfo->u2EhtDisSubChanBitmap = prEvtStaticPPDscb->u2DscbBitmap;

#if (CFG_SUPPORT_ADHOC || CFG_ENABLE_WIFI_DIRECT)
	bssUpdateBeaconContentEx(prAdapter,
		prEvtStaticPPDscb->ucBssIndex,
			IE_UPD_METHOD_UPDATE_ALL);
#endif

}
#endif /* #if CFG_SUPPORT_802_PP_DSCB */

#if CFG_SUPPORT_NAN
struct _CMD_EVENT_TLV_ELEMENT_T *nicGetTargetTlvElement(
		   uint16_t u2TargetTlvElement, void *prCmdBuffer)
{
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint16_t u2ElementNum;
	void *pvCurrPtr;

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	/* Check target element is exist or not */
	if (u2TargetTlvElement > prTlvCommon->u2TotalElementNum) {
		/* New element or element is not exist */
		if (u2TargetTlvElement - prTlvCommon->u2TotalElementNum > 1) {
			/* element is not exist */
			DBGLOG(TX, ERROR, "Target element is not exist\n");
			return NULL;
		}
	}

	for (u2ElementNum = 1; u2ElementNum <= u2TargetTlvElement;
	     u2ElementNum++) {
		if (u2ElementNum == 1) {
			pvCurrPtr = prTlvCommon->aucBuffer;
		} else {
			pvCurrPtr = prTlvElement->aucbody;
			pvCurrPtr = (void *)((uint8_t *)pvCurrPtr +
					    prTlvElement->body_len);
		}
		prTlvElement = (struct _CMD_EVENT_TLV_ELEMENT_T *)pvCurrPtr;
	}

	return prTlvElement;
}

uint32_t nicAddNewTlvElement(uint32_t u4Tag, uint32_t u4BodyLen,
		    uint32_t prCmdBufferLen, void *prCmdBuffer)
{
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint32_t u4TotalLen;

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	/* Get new element */
	prTlvElement = nicGetTargetTlvElement(
		prTlvCommon->u2TotalElementNum + 1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get new TLV element fail\n");
		return WLAN_STATUS_FAILURE;
	}

	/* Check tatol len is overflow or not */
	u4TotalLen = ((size_t)prTlvElement->aucbody + u4BodyLen) -
		     (size_t)prCmdBuffer;

	if (u4TotalLen > prCmdBufferLen) {
		/* Length overflow */
		DBGLOG(TX, ERROR,
		       "Length overflow: Total len:%d, CMD buffer len:%d\n",
		       u4TotalLen, prCmdBufferLen);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	/* Update total element count */
	prTlvCommon->u2TotalElementNum++;

	/* Fill TLV constant */
	prTlvElement->tag_type = u4Tag;

	prTlvElement->body_len = u4BodyLen;

	return WLAN_STATUS_SUCCESS;
}

void nicNanEventSTATxCTL(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct EVENT_UPDATE_NAN_TX_STATUS *prUpdateTxStatus;

	prUpdateTxStatus = (struct EVENT_UPDATE_NAN_TX_STATUS *)pcuEvtBuf;
	qmUpdateFreeNANQouta(prAdapter, prUpdateTxStatus);
}

void nicNanVendorEventHandler(struct ADAPTER *prAdapter,
			 struct WIFI_EVENT *prEvent)
{
	ASSERT(prAdapter);
	ASSERT(prEvent);

	DBGLOG(NAN, INFO, "[%s] IN, Guiding to Vendor event handler\n",
	       __func__);

	kalNanHandleVendorEvent(prAdapter, prEvent->aucBuffer);
}

struct NanMatchInd g_rDiscMatchInd;
uint8_t g_u2IndPubId;

void nicNanEventDiscoveryResult(struct ADAPTER *prAdapter,
	    uint8_t *pcuEvtBuf)
{
	struct NAN_DISCOVERY_EVENT *prDiscEvt;

	prDiscEvt = (struct NAN_DISCOVERY_EVENT *)pcuEvtBuf;
	g_u2IndPubId = prDiscEvt->u2PublishID; /* for sigma test */

	DBGLOG(NAN, INFO, "generate discovey event\n");
	dumpMemory8((uint8_t *)prDiscEvt->aucNanAddress, MAC_ADDR_LEN);

	kalMemSet(&g_rDiscMatchInd, 0, sizeof(struct NanMatchInd));
	g_rDiscMatchInd.eventID = ENUM_NAN_SD_RESULT;
	g_rDiscMatchInd.peer_sdea_params.config_nan_data_path =
		prDiscEvt->ucDataPathParm;
	g_rDiscMatchInd.publish_subscribe_id = prDiscEvt->u2SubscribeID;
	g_rDiscMatchInd.requestor_instance_id = prDiscEvt->u2PublishID;
	g_rDiscMatchInd.peer_sdea_params.security_cfg = 0;
	g_rDiscMatchInd.peer_cipher_type = 1;
	g_rDiscMatchInd.peer_sdea_params.ndp_type = NAN_DATA_PATH_UNICAST_MSG;
	g_rDiscMatchInd.service_specific_info_len =
		prDiscEvt->u2Service_info_len;
	kalMemCopy(g_rDiscMatchInd.service_specific_info,
		   prDiscEvt->aucSerive_specificy_info,
		   NAN_MAX_SERVICE_SPECIFIC_INFO_LEN);
	kalMemCopy(g_rDiscMatchInd.addr, prDiscEvt->aucNanAddress,
		   MAC_ADDR_LEN);
	g_rDiscMatchInd.sdf_match_filter_len =
		prDiscEvt->ucSdf_match_filter_len;
	kalMemCopy(g_rDiscMatchInd.sdf_match_filter,
			prDiscEvt->aucSdf_match_filter,
			NAN_FW_MAX_MATCH_FILTER_LEN);

	kalIndicateNetlink2User(prAdapter->prGlueInfo, &g_rDiscMatchInd,
				sizeof(struct NanMatchInd));
}

struct NanFollowupInd rFollowInd;
void nicNanReceiveEvent(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct NAN_FOLLOW_UP_EVENT *prDiscEvt;

	_Static_assert(sizeof(rFollowInd.service_specific_info) ==
		       sizeof(prDiscEvt->service_specific_info),
		       "service_specific_info len not match");
	prDiscEvt = (struct NAN_FOLLOW_UP_EVENT *)pcuEvtBuf;
	dumpMemory8((uint8_t *)pcuEvtBuf, 32);
	DBGLOG(NAN, LOUD, "receive followup event\n");
	kalMemSet(&rFollowInd, 0, sizeof(struct NanFollowupInd));
	rFollowInd.eventID = ENUM_NAN_RECEIVE;
	rFollowInd.publish_subscribe_id = prDiscEvt->publish_subscribe_id;
	rFollowInd.requestor_instance_id = prDiscEvt->requestor_instance_id;
	kalMemCopy(rFollowInd.addr, prDiscEvt->addr, MAC_ADDR_LEN);
	if (unlikely(prDiscEvt->service_specific_info_len >
		sizeof(rFollowInd.service_specific_info))) {
		DBGLOG(NAN, WARN,
			"service_specific_info len too large: %u > %u\n",
			prDiscEvt->service_specific_info_len,
			sizeof(rFollowInd.service_specific_info));
		prDiscEvt->service_specific_info_len =
			sizeof(rFollowInd.service_specific_info);
	}
	rFollowInd.service_specific_info_len =
		prDiscEvt->service_specific_info_len;
	kalMemCopy(rFollowInd.service_specific_info,
		   prDiscEvt->service_specific_info,
		   prDiscEvt->service_specific_info_len);
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rFollowInd,
				sizeof(struct NanFollowupInd));
}

void nicNanRepliedEvnt(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct NanPublishRepliedInd rRepliedInd;
	struct NAN_REPLIED_EVENT *prRepliedEvt;

	prRepliedEvt = (struct NAN_REPLIED_EVENT *)pcuEvtBuf;
	kalMemZero(&rRepliedInd, sizeof(struct NanPublishRepliedInd));
	rRepliedInd.eventID = ENUM_NAN_REPLIED;
	rRepliedInd.pubid = prRepliedEvt->u2Pubid;
	rRepliedInd.subid = prRepliedEvt->u2Subid;
	COPY_MAC_ADDR(rRepliedInd.addr, prRepliedEvt->auAddr);
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rRepliedInd,
				sizeof(struct NanPublishRepliedInd));
}

void nicNanPublishTerminateEvt(struct ADAPTER *prAdapter,
		   uint8_t *pcuEvtBuf)
{
	struct NanPublishTerminatedInd rPubTerminatEvt;
	struct NAN_PUBLISH_TERMINATE_EVENT *prPubTerEvt;

	prPubTerEvt = (struct NAN_PUBLISH_TERMINATE_EVENT *)pcuEvtBuf;
	kalMemZero(&rPubTerminatEvt, sizeof(struct NanPublishTerminatedInd));
	rPubTerminatEvt.eventID = ENUM_NAN_PUB_TERMINATE;
	rPubTerminatEvt.publish_id = prPubTerEvt->u2Pubid;
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rPubTerminatEvt,
				sizeof(struct NanPublishTerminatedInd));
}

void nicNanSubscribeTerminateEvt(struct ADAPTER *prAdapter,
			    uint8_t *pcuEvtBuf)
{
	struct NanSubscribeTerminatedInd rSubTerminatEvt;
	struct NAN_SUBSCRIBE_TERMINATE_EVENT *pSubTerEvt;

	pSubTerEvt = (struct NAN_SUBSCRIBE_TERMINATE_EVENT *)pcuEvtBuf;
	kalMemZero(&rSubTerminatEvt, sizeof(struct NanPublishTerminatedInd));
	rSubTerminatEvt.eventID = ENUM_NAN_SUB_TERMINATE;
	rSubTerminatEvt.subscribe_id = pSubTerEvt->u2Subid;
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rSubTerminatEvt,
				sizeof(struct NanSubscribeTerminatedInd));
}

#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
void nicNanNdlFlowCtrlEvt(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct NAN_EVT_NDL_FLOW_CTRL *prFlowCtrlEvt;
	struct STA_RECORD *prStaRec;
	uint16_t u2SchId = 0;
	uint32_t u4Idx;
	unsigned char fgNeedToSendPkt = FALSE;
	OS_SYSTIME rCurrentTime;
	OS_SYSTIME rExpiryTime;

	prFlowCtrlEvt = (struct NAN_EVT_NDL_FLOW_CTRL *)pcuEvtBuf;
	for (u2SchId = 0; u2SchId < NAN_MAX_CONN_CFG; u2SchId++) {
		uint8_t ucSTAIdx;
		uint16_t u2SlotTime;

		if (nanSchedPeerSchRecordIsValid(prAdapter, u2SchId) == FALSE)
			continue;

		rCurrentTime = kalGetTimeTick();
		u2SlotTime = prFlowCtrlEvt->au2FlowCtrl[u2SchId];
		rExpiryTime =
			rCurrentTime + u2SlotTime * NAN_SEND_PKT_TIME_SLOT;

		DBGLOG(NAN, LOUD,
		       "[NDL flow control] Sch:%u, Expiry:%u, Slot:%u\n",
		       u2SchId, rExpiryTime, u2SlotTime);

		if (u2SlotTime == 0)
			continue;

		rExpiryTime -= NAN_SEND_PKT_TIME_GUARD_TIME;
		for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDP_CXT_NUM; u4Idx++) {
			ucSTAIdx = nanSchedQueryStaRecIdx(prAdapter, u2SchId,
							  u4Idx);
			if (ucSTAIdx == STA_REC_INDEX_NOT_FOUND)
				continue;

			prStaRec = &prAdapter->arStaRec[ucSTAIdx];
			prStaRec->rNanExpiredSendTime = rExpiryTime;

			if (prStaRec->fgNanSendTimeExpired)
				fgNeedToSendPkt = TRUE;
		}
	}

	if (fgNeedToSendPkt == TRUE &&
	    wlanGetTxPendingFrameCount(prAdapter) > 0) {
		DBGLOG(NAN, LOUD, "Trigger NAN tx request\n");
		kalSetEvent(prAdapter->prGlueInfo);
	}
}

void nicNanNdlFlowCtrlEvtV2(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct NAN_EVT_NDL_FLOW_CTRL_V2 *prFlowCtrlEvt;
	struct STA_RECORD *prStaRec;
	uint16_t u2SchId = 0;
	uint32_t u4Idx;
	uint32_t u4NanSendPacketGuardTime;
	struct NAN_FLOW_CTRL *prNanFlowCtrlRecord;
	OS_SYSTIME rCurrentTime;
	OS_SYSTIME rExpiryTime;

	KAL_SPIN_LOCK_DECLARATION();

	u4NanSendPacketGuardTime = prAdapter->rWifiVar.u4NanSendPacketGuardTime;
	prFlowCtrlEvt = (struct NAN_EVT_NDL_FLOW_CTRL_V2 *)pcuEvtBuf;

	for (u2SchId = 0; u2SchId < NAN_MAX_CONN_CFG; u2SchId++) {
		uint8_t ucSTAIdx;
		uint16_t u4RemainingTime;

		if (nanSchedPeerSchRecordIsValid(prAdapter, u2SchId) == FALSE)
			continue;

		prNanFlowCtrlRecord = nanSchedGetPeerSchRecFlowCtrl(prAdapter,
								    u2SchId);
		rCurrentTime = kalGetTimeTick();
		u4RemainingTime = prFlowCtrlEvt->au4RemainingTime[u2SchId];
		rExpiryTime = rCurrentTime + u4RemainingTime;

		DBGLOG(NAN, INFO,
		       "[NDL flow control] Sch:%u, Expiry:%u, Remain:%u, %sstayed %u for %u ms\n",
		       u2SchId, rExpiryTime, u4RemainingTime,
		       prNanFlowCtrlRecord[u2SchId].fgAllow ==
				       !!u4RemainingTime ? "WARN " : "",
		       prNanFlowCtrlRecord[u2SchId].fgAllow,
		       prNanFlowCtrlRecord[u2SchId].u4Time ?
			       rCurrentTime -
				       prNanFlowCtrlRecord[u2SchId].u4Time : 0);
		prNanFlowCtrlRecord[u2SchId].fgAllow = !!u4RemainingTime;
		prNanFlowCtrlRecord[u2SchId].u4Time = rCurrentTime;

		if (u4RemainingTime == 0)
			continue;

		rExpiryTime -= u4NanSendPacketGuardTime;
		for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDP_CXT_NUM; u4Idx++) {
			ucSTAIdx = nanSchedQueryStaRecIdx(prAdapter, u2SchId,
							  u4Idx);
			if (ucSTAIdx == STA_REC_INDEX_NOT_FOUND)
				continue;

			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_NAN_NDL_FLOW_CTRL);

			prStaRec = &prAdapter->arStaRec[ucSTAIdx];
			prStaRec->rNanExpiredSendTime = rExpiryTime;

			if (prStaRec->fgNanSendTimeExpired) {
				prStaRec->fgNanSendTimeExpired = FALSE;

				DBGLOG(NAN, INFO, "Trigger NAN tx request\n");
				/* NAN StaRec Start Tx */
				qmSetStaRecTxAllowed(prAdapter,
					prStaRec, TRUE);
			}
			KAL_RELEASE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_NAN_NDL_FLOW_CTRL);
		}

		kalSetEvent(prAdapter->prGlueInfo); /* Wakeup TX */
	}
}
#endif

void nicNanEventDispatcher(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent)
{
	ASSERT(prAdapter);
	ASSERT(prEvent);

	if (prAdapter->fgIsNANfromHAL == FALSE) {
		DBGLOG(INIT, INFO, "nicNanIOEventHandler\n");
		/* For IOCTL use */
		nicNanIOEventHandler(prAdapter, prEvent);
	} else {
		DBGLOG(INIT, INFO, "nicNanVendorEventHandler\n");
		/* For Vendor command use */
		nicNanVendorEventHandler(prAdapter, prEvent);
	}
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
void nicNanIOEventHandler(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct UNI_CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint32_t u4SubEvent;

	ASSERT(prAdapter);

	prTlvElement =
	(struct UNI_CMD_EVENT_TLV_ELEMENT_T *)prEvent->aucBuffer;

	u4SubEvent = prTlvElement->u2Tag;

	DBGLOG(NAN, INFO, "nicNanIOEventHandler, subEvent:%d\n", u4SubEvent);

	if (prAdapter->fgIsNANRegistered == FALSE) {
		DBGLOG(NAN, ERROR,
			"nicNanIOEventHandler, NAN is unregistered\n");
		return;
	}

	switch (u4SubEvent) {
	case UNI_EVENT_NAN_TAG_DISCOVERY_RESULT:
		nicNanEventDiscoveryResult(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_FOLLOW_EVENT:
		nicNanReceiveEvent(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_REPLIED_EVENT:
		nicNanRepliedEvnt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_PUBLISH_TERMINATE_EVENT:
		nicNanPublishTerminateEvt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_SUBSCRIBE_TERMINATE_EVENT:
		nicNanSubscribeTerminateEvt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_MASTER_IND_ATTR:
		nanDevMasterIndEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_CLUSTER_ID_UPDATE:
		nanDevClusterIdEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_SCHEDULE_CONFIG:
	case UNI_EVENT_NAN_TAG_ID_PEER_AVAILABILITY:
	case UNI_EVENT_NAN_TAG_ID_PEER_CAPABILITY:
	case UNI_EVENT_NAN_TAG_ID_CRB_HANDSHAKE_TOKEN:
	case UNI_EVENT_NAN_TAG_ID_DEVICE_CAPABILITY:
		nanSchedulerUniEventDispatch(prAdapter, u4SubEvent,
					  prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_PEER_SEC_CONTEXT_INFO:
		nanDiscUpdateSecContextInfoAttr(prAdapter,
						prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_PEER_CIPHER_SUITE_INFO:
		nanDiscUpdateCipherSuiteInfoAttr(prAdapter,
						 prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_ID_DATA_NOTIFY:
		nicNanEventSTATxCTL(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_FTM_DONE:
		nanRangingFtmDoneEvt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_RANGING_BY_DISC:
		nanRangingInvokedByDiscEvt(prAdapter, prTlvElement->aucbody);
		break;
#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
	case UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL:
		nicNanNdlFlowCtrlEvt(prAdapter, prTlvElement->aucbody);
		break;
	case UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL_V2:
		nicNanNdlFlowCtrlEvtV2(prAdapter, prTlvElement->aucbody);
		break;
#endif
	case UNI_EVENT_NAN_TAG_NDL_DISCONNECT:
		nanDataEngingDisconnectEvt(prAdapter, prTlvElement->aucbody);
		break;
	}
}

#else
void nicNanIOEventHandler(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint32_t u4SubEvent;

	ASSERT(prAdapter);
	ASSERT(prEvent);

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prEvent->aucBuffer;

	prTlvElement =
		(struct _CMD_EVENT_TLV_ELEMENT_T *)prTlvCommon->aucBuffer;

	u4SubEvent = prTlvElement->tag_type;

	DBGLOG(NAN, INFO, "nicNanIOEventHandler, subEvent:%d\n", u4SubEvent);

	if (prAdapter->fgIsNANRegistered == FALSE) {
		DBGLOG(NAN, ERROR,
			"nicNanIOEventHandler, NAN is unregistered\n");
		return;
	}

	switch (u4SubEvent) {
	case NAN_EVENT_DISCOVERY_RESULT:
		nicNanEventDiscoveryResult(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_FOLLOW_EVENT:
		nicNanReceiveEvent(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_REPLIED_EVENT:
		nicNanRepliedEvnt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_PUBLISH_TERMINATE_EVENT:
		nicNanPublishTerminateEvt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_SUBSCRIBE_TERMINATE_EVENT:
		nicNanSubscribeTerminateEvt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_MASTER_IND_ATTR:
		nanDevMasterIndEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_CLUSTER_ID_UPDATE:
		nanDevClusterIdEvtHandler(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_SCHEDULE_CONFIG:
	case NAN_EVENT_ID_PEER_AVAILABILITY:
	case NAN_EVENT_ID_PEER_CAPABILITY:
	case NAN_EVENT_ID_CRB_HANDSHAKE_TOKEN:
	case NAN_EVENT_ID_DEVICE_CAPABILITY:
		nanSchedulerEventDispatch(prAdapter, u4SubEvent,
					  prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_PEER_SEC_CONTEXT_INFO:
		nanDiscUpdateSecContextInfoAttr(prAdapter,
						prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_PEER_CIPHER_SUITE_INFO:
		nanDiscUpdateCipherSuiteInfoAttr(prAdapter,
						 prTlvElement->aucbody);
		break;
	case NAN_EVENT_ID_DATA_NOTIFY:
		nicNanEventSTATxCTL(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_FTM_DONE:
		nanRangingFtmDoneEvt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_RANGING_BY_DISC:
		nanRangingInvokedByDiscEvt(prAdapter, prTlvElement->aucbody);
		break;
#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
	case NAN_EVENT_NDL_FLOW_CTRL:
		nicNanNdlFlowCtrlEvt(prAdapter, prTlvElement->aucbody);
		break;
	case NAN_EVENT_NDL_FLOW_CTRL_V2:
		nicNanNdlFlowCtrlEvtV2(prAdapter, prTlvElement->aucbody);
		break;
#endif
	case NAN_EVENT_NDL_DISCONNECT:
		nanDataEngingDisconnectEvt(prAdapter, prTlvElement->aucbody);
	}
}
#endif

void nicNanGetCmdInfoQueryTestBuffer(
	struct _TXM_CMD_EVENT_TEST_T **prCmdInfoQueryTestBuffer)
{
	*prCmdInfoQueryTestBuffer =
		(struct _TXM_CMD_EVENT_TEST_T *)&grCmdInfoQueryTestBuffer;
}

#endif

void nicEventHandleAddBa(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec, uint8_t ucTid,
			uint16_t u2WinSize, uint16_t u2WinStart)
{
	if (!prStaRec) {
		/* Invalid STA_REC index, discard the event packet */
		/* ASSERT(0); */
		DBGLOG(QM, WARN,
			"QM: RX ADDBA Event for a NULL STA_REC\n");
		return;
	}
#if 0
	if (!prStaRec->fgIsValid) {
		/* TODO: (Tehuang) Handle the Host-FW synchronization issue */
		DBGLOG(QM, WARN,
			"QM: RX ADDBA Event for an invalid STA_REC\n");
		/* ASSERT(0); */
		/* return; */
	}
#endif
	if (!qmAddRxBaEntry(prAdapter, prStaRec->ucIndex,
				ucTid, u2WinStart, u2WinSize)) {
		/* FW shall ensure the availabiilty of
		 * the free-to-use BA entry
		 */
		DBGLOG(QM, ERROR, "QM: (Error) qmAddRxBaEntry() failure\n");
	}
}

#if (CFG_WIFI_GET_DPD_CACHE == 1)
void nicCmdEventQueryDpdCache(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen = 0;
	struct EVENT_GET_DPD_CACHE *prEventDpdCache;
	struct PARAM_GET_DPD_CACHE *prParaDpdCache;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	if (!pucEventBuf) {
		DBGLOG(NIC, ERROR, "NULL pucEventBuf!\n");
		u4Status = WLAN_STATUS_FAILURE;
	}

	if (!prCmdInfo->pvInformationBuffer) {
		DBGLOG(NIC, ERROR, "NULL pvInformationBuffer!\n");
		u4Status = WLAN_STATUS_FAILURE;
	}

	if (prCmdInfo->fgIsOid) {
		if (u4Status == WLAN_STATUS_SUCCESS) {
			prEventDpdCache =
				(struct EVENT_GET_DPD_CACHE *) pucEventBuf;
			prParaDpdCache = (struct PARAM_GET_DPD_CACHE *)
				prCmdInfo->pvInformationBuffer;
			u4QueryInfoLen = sizeof(struct EVENT_GET_DPD_CACHE);

			kalMemCopy(prParaDpdCache, prEventDpdCache,
				sizeof(struct PARAM_GET_DPD_CACHE));
		}

		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
				u4QueryInfoLen, u4Status);
	}
}
#endif /* CFG_WIFI_GET_DPD_CACHE */

#if CFG_SUPPORT_FW_DROP_SSN
void nicEventHandleFwDropSSN(struct ADAPTER *prAdapter,
	struct EVENT_STORED_FW_DROP_SSN_INFO *prSSN)
{
	struct QUE rQue;
	struct QUE *prQue = &rQue;
	struct SW_RFB *prSwRfb;

	QUEUE_INITIALIZE(prQue);

	NIC_RX_DEQUEUE_FREE_QUE(prAdapter, 1, prQue, RFB_TRACK_FW_DROP_SSN);

	QUEUE_REMOVE_HEAD(prQue, prSwRfb, struct SW_RFB *);
	if (!prSwRfb) {
		DBGLOG_LIMITED(QM, WARN, "No More RFB\n");
		return;
	}

	prSwRfb->ucWlanIdx = prSSN->ucWlanIdx;
	prSwRfb->ucStaRecIdx = secGetStaIdxByWlanIdx(prAdapter,
			prSwRfb->ucWlanIdx);
	prSwRfb->prStaRec = cnmGetStaRecByIndex(prAdapter,
		prSwRfb->ucStaRecIdx);
	prSwRfb->ucTid = prSSN->ucTid;
	prSwRfb->u2SSN = prSSN->u2SSN;
	prSwRfb->ucPayloadFormat = prSSN->ucAmsduFormat;
	prSwRfb->eDst = RX_PKT_DESTINATION_NULL;

	if (!prSwRfb->prStaRec) {
		DBGLOG(NIC, WARN,
			"Invalid STA[%u] WIDX[%u] TID[%u] SSN[%u] AmsduFormat[%u]\n",
			prSwRfb->ucStaRecIdx, prSSN->ucWlanIdx,
			prSSN->ucTid, prSSN->u2SSN,
			prSSN->ucAmsduFormat);
		return;
	}

	DBGLOG(NIC, TRACE,
		"STA[%u] WIDX[%u] TID[%u] SSN[%u] AmsduFormat[%u]\n",
		prSwRfb->ucStaRecIdx, prSSN->ucWlanIdx,
		prSSN->ucTid, prSSN->u2SSN,
		prSSN->ucAmsduFormat);

	/* insert prSwRfb into reordering queue */
	qmProcessPktWithReordering(prAdapter, prSwRfb, prQue);

	/* no prSwRfb is in return queue, so just early return */
	if (QUEUE_IS_EMPTY(prQue))
		return;

	/* enqueue all prSwRfb to NAPI */
	nicRxEnqueueRfbMainToNapi(prAdapter, prQue);
	if (kalScheduleNapiTask(prAdapter) == WLAN_STATUS_NOT_ACCEPTED) {
		/* Handle Non Rx-direct call path */
		nicRxIndicateRfbMainToNapi(prAdapter);
	}
}

void nicEventFwDropSSN(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent)
{
	struct EVENT_FW_DROP_SSN_INFO *info;
	int i = 0;

	info = (struct EVENT_FW_DROP_SSN_INFO *)(
				prEvent->aucBuffer);

	if (unlikely(info == NULL)) {
		DBGLOG(NIC, WARN, "info is NULL\n");
		return;
	}

	if (info->ucDrpPktNum > FW_DROP_SSN_MAX) {
		DBGLOG(NIC, WARN, "skip invalid ucDrpPktNum:%u\n",
			info->ucDrpPktNum);
		return;
	}

	for (i = 0; i < info->ucDrpPktNum; i++)
		nicEventHandleFwDropSSN(prAdapter, &(info->arSSN[i]));
}
#endif /* CFG_SUPPORT_FW_DROP_SSN */

#if CFG_SUPPORT_BAR_DELAY_INDICATION
void nicEventHandleDelayBar(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent)
{
	struct EVENT_BAR_DELAY *prEventStoredBAR;
	struct QUE rReturnedQue;
	struct QUE *prReturnedQue;
	int i = 0;

	prEventStoredBAR = (struct EVENT_BAR_DELAY *)(
				prEvent->aucBuffer);

	if (unlikely(prEventStoredBAR == NULL)) {
		DBGLOG(NIC, WARN, "prEventStoredBAR is NULL\n");
		return;
	}

	if (unlikely(prEventStoredBAR->ucEvtVer != 0)) {
		DBGLOG(NIC, WARN, "not be handle, ucEvtVer:%u\n",
			prEventStoredBAR->ucEvtVer);
		return;
	}

	if (unlikely(prEventStoredBAR->ucBaNum >
		BAR_DELAY_INDICATION_BA_MAX)) {
		DBGLOG(NIC, WARN, "not be handle, ucBaNum:%u\n",
			prEventStoredBAR->ucBaNum);
		return;
	}

	prReturnedQue = &rReturnedQue;
	QUEUE_INITIALIZE(prReturnedQue);
	for (i = 0; i < prEventStoredBAR->ucBaNum; i++) {
		/* always add 1 since cnt=0 for 1st stored in fw */
		prEventStoredBAR->arBAR[i].ucStoredBARCount++;

		DBGLOG(NIC, INFO,
			"[Id:StaId:Tid:SSN:StoredCnt]:[%d:%d:%d:%d:%d]\n",
			i,
			prEventStoredBAR->arBAR[i].ucStaRecIdx,
			prEventStoredBAR->arBAR[i].ucTid,
			prEventStoredBAR->arBAR[i].u2SSN,
			prEventStoredBAR->arBAR[i].ucStoredBARCount);

		qmHandleRxReorderWinShift(prAdapter,
			prEventStoredBAR->arBAR[i].ucStaRecIdx,
			prEventStoredBAR->arBAR[i].ucTid,
			prEventStoredBAR->arBAR[i].u2SSN,
			prReturnedQue);

		RX_ADD_CNT(&prAdapter->rRxCtrl, RX_BAR_DELAY_COUNT,
			prEventStoredBAR->arBAR[i].ucStoredBARCount
			);
	}

	/* no prSwRfb is in return queue, so just early return */
	if (QUEUE_IS_EMPTY(prReturnedQue))
		return;

	/* enqueue all prSwRfb to NAPI */
	nicRxEnqueueRfbMainToNapi(prAdapter, prReturnedQue);
	if (kalScheduleNapiTask(prAdapter) == WLAN_STATUS_NOT_ACCEPTED) {
		/* Handle Non Rx-direct call path */
		nicRxIndicateRfbMainToNapi(prAdapter);
	}
}
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */

#if (CFG_SUPPORT_TSF_SYNC == 1)
void nicCmdEventLatchTSF(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	if (!pucEventBuf) {
		DBGLOG(NIC, ERROR, "NULL pucEventBuf!\n");
		return;
	}

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		kalMemCopy(prCmdInfo->pvInformationBuffer,
			pucEventBuf, sizeof(struct CMD_TSF_SYNC));
		u4QueryInfoLen = sizeof(struct CMD_TSF_SYNC);

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	return;

}
#endif

#if (CFG_COALESCING_INTERRUPT == 1)
void nicEventCoalescingIntDone(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	struct EVENT_PF_CF_COALESCING_INT_DONE *prEventCf;
	struct BUS_INFO *prBusInfo;

	prEventCf = (struct EVENT_PF_CF_COALESCING_INT_DONE *) (
		prEvent->aucBuffer);
	prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->setWfdmaCoalescingInt)
		prBusInfo->setWfdmaCoalescingInt(prAdapter,
						prEventCf->fgEnable);
}
#endif /* CFG_WIFI_TXPWR_TBL_DUMP */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is the handler when driver receives EXT_EVENT_ID_SER.
*
* \param[in] prAdapter
* \param[in] prCmdInfo
* \param[in] pucEventBuf
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void nicCmdEventQuerySerInfo(struct ADAPTER *prAdapter,
			     struct CMD_INFO *prCmdInfo,
			     uint8_t *pucEventBuf)
{
	struct PARAM_SER_INFO_T *prQuerySerInfo = NULL;
	struct EXT_EVENT_SER_T *prEventSer = NULL;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	prGlueInfo = prAdapter->prGlueInfo;

	if (prCmdInfo->fgIsOid) {
		prQuerySerInfo = (struct PARAM_SER_INFO_T *)
				 prCmdInfo->pvInformationBuffer;
		prEventSer = (struct EXT_EVENT_SER_T *) pucEventBuf;

#if (EXT_EVENT_SER_VER > 0)
		if (prEventSer->ucEvtVer < EXT_EVENT_SER_VER)
			u4QueryInfoLen = prEventSer->u2EvtLen;
		else
#endif /* EXT_EVENT_SER_VER */
			u4QueryInfoLen = sizeof(struct PARAM_SER_INFO_T);

		kalMemCopy(prQuerySerInfo, pucEventBuf, u4QueryInfoLen);

#if (EXT_EVENT_SER_VER > 0) /* for coverity */
		if (prEventSer->ucEvtVer >= EXT_EVENT_SER_VER)
#endif
			prQuerySerInfo->ucEvtVer = EXT_EVENT_SER_VER;

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

#if (CFG_WIFI_ISO_DETECT == 1)
void nicCmdEventQueryCoexIso(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;

	struct COEX_CMD_HANDLER *prCoexCmdHandler;
	struct COEX_CMD_ISO_DETECT *prCoexCmdIsoDetect;
	struct PARAM_COEX_HANDLER *prParaCoexHandler;
	struct PARAM_COEX_ISO_DETECT *prCoexIsoDetect;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCoexCmdHandler = (struct COEX_CMD_HANDLER *) (pucEventBuf);
		u4QueryInfoLen  = sizeof(struct PARAM_COEX_HANDLER);
		prCoexCmdIsoDetect =
	(struct COEX_CMD_ISO_DETECT *) &prCoexCmdHandler->aucBuffer[0];

		prParaCoexHandler =
	(struct PARAM_COEX_HANDLER *) prCmdInfo->pvInformationBuffer;
		prCoexIsoDetect  =
	(struct PARAM_COEX_ISO_DETECT *) &prParaCoexHandler->aucBuffer[0];
		prCoexIsoDetect->u4IsoPath = prCoexCmdIsoDetect->u4IsoPath;
		prCoexIsoDetect->u4Channel = prCoexCmdIsoDetect->u4Channel;
		prCoexIsoDetect->u4Isolation = prCoexCmdIsoDetect->u4Isolation;

		kalOidComplete(prGlueInfo,
				prCmdInfo,
				u4QueryInfoLen,
				WLAN_STATUS_SUCCESS);

	}

}
#endif

#if CFG_WIFI_TXPWR_TBL_DUMP
void nicCmdEventGetTxPwrTbl(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct EVENT_GET_TXPWR_TBL *prTxPwrTblEvent = NULL;
	struct PARAM_CMD_GET_TXPWR_TBL *prTxPwrTbl = NULL;
	void *info_buf = NULL;

	DBGLOG(NIC, INFO, "Enter nicCmdEventGetTxPwrTbl\n");

	if (!prAdapter) {
		DBGLOG(NIC, WARN, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, WARN, "NULL prCmdInfo!\n");
		return;
	}

	if (!pucEventBuf || !prCmdInfo->pvInformationBuffer) {
		if (prCmdInfo->fgIsOid) {
			kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo,
					0,
					WLAN_STATUS_FAILURE);
		}

		if (!pucEventBuf)
			DBGLOG(NIC, WARN, "NULL pucEventBuf!\n");

		if (!prCmdInfo->pvInformationBuffer)
			DBGLOG(NIC, WARN, "NULL pvInformationBuffer!\n");

		return;
	}

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		info_buf = prCmdInfo->pvInformationBuffer;

		prTxPwrTblEvent = (struct EVENT_GET_TXPWR_TBL *) pucEventBuf;
		prTxPwrTbl = (struct PARAM_CMD_GET_TXPWR_TBL *) info_buf;
		u4QueryInfoLen = sizeof(struct PARAM_CMD_GET_TXPWR_TBL);

		prTxPwrTbl->ucCenterCh = prTxPwrTblEvent->ucCenterCh;

		kalMemCopy(prTxPwrTbl->tx_pwr_tbl,
				prTxPwrTblEvent->tx_pwr_tbl,
				sizeof(prTxPwrTblEvent->tx_pwr_tbl));

		kalOidComplete(prGlueInfo, prCmdInfo,
				u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#endif /* CFG_WIFI_TXPWR_TBL_DUMP */

#if (CFG_WIFI_GET_MCS_INFO == 1)
void nicCmdEventQueryTxMcsInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct EVENT_TX_MCS_INFO *prTxMcsEvent;
	struct PARAM_TX_MCS_INFO *prTxMcsInfo;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	if (!pucEventBuf || !prCmdInfo->pvInformationBuffer) {
		if (prCmdInfo->fgIsOid) {
			kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo->fgSetQuery,
					0,
					WLAN_STATUS_FAILURE);
		}

		if (!pucEventBuf)
			DBGLOG(NIC, ERROR, "NULL pucEventBuf!\n");

		if (!prCmdInfo->pvInformationBuffer)
			DBGLOG(NIC, ERROR, "NULL pvInformationBuffer!\n");

		return;
	}

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		prTxMcsEvent = (struct EVENT_TX_MCS_INFO *) pucEventBuf;
		prTxMcsInfo = (struct PARAM_TX_MCS_INFO *)
			      prCmdInfo->pvInformationBuffer;

		u4QueryInfoLen = sizeof(struct EVENT_TX_MCS_INFO);

		kalMemCopy(prTxMcsInfo->au2TxRateCode,
			   prTxMcsEvent->au2TxRateCode,
			   sizeof(prTxMcsEvent->au2TxRateCode));
		kalMemCopy(prTxMcsInfo->aucTxBw,
			   prTxMcsEvent->aucTxBw,
			   sizeof(prTxMcsEvent->aucTxBw));
		kalMemCopy(prTxMcsInfo->aucTxSgi,
			   prTxMcsEvent->aucTxSgi,
			   sizeof(prTxMcsEvent->aucTxSgi));
		kalMemCopy(prTxMcsInfo->aucTxLdpc,
			   prTxMcsEvent->aucTxLdpc,
			   sizeof(prTxMcsEvent->aucTxLdpc));
		kalMemCopy(prTxMcsInfo->aucTxRatePer,
			   prTxMcsEvent->aucTxRatePer,
			   sizeof(prTxMcsEvent->aucTxRatePer));

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
				u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}


}

void nicEventTxMcsInfo(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	DBGLOG(RSN, INFO, "EVENT_ID_TX_MCS_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
				prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				prCmdInfo->fgSetQuery,
				0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}
#endif /* CFG_WIFI_GET_MCS_INFO */

#if CFG_SUPPORT_RTT
void nicCmdEventRttCapabilities(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct EVENT_RTT_CAPABILITIES *prRttCapa;
	struct RTT_CAPABILITIES *prCapaBuf =
		 (struct RTT_CAPABILITIES *)prCmdInfo->pvInformationBuffer;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		prRttCapa = (struct EVENT_RTT_CAPABILITIES *) pucEventBuf;
		u4QueryInfoLen = sizeof(struct RTT_CAPABILITIES);

		if (prCmdInfo->u2InfoBufLen >= u4QueryInfoLen) {
			kalMemCopy(prCapaBuf, &prRttCapa->rCapabilities,
				u4QueryInfoLen);

			DBGLOG(RTT, INFO,
				"one_sided=%hhu, ftm=%hhu, lci=%hhu, lcr=%hhu, preamble=%hhu, bw=%hhu, responder=%hhu, ver=%hhu",
				prCapaBuf->fgRttOneSidedSupported,
				prCapaBuf->fgRttFtmSupported,
				prCapaBuf->fgLciSupported,
				prCapaBuf->fgLcrSupported,
				prCapaBuf->ucPreambleSupport,
				prCapaBuf->ucBwSupport,
				prCapaBuf->fgResponderSupported,
				prCapaBuf->fgMcVersion);
		} else {
			DBGLOG(RTT, ERROR,
				"invalid CMD buffer, length=%d",
				prCmdInfo->u2InfoBufLen);
		}

		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicEventRttDone(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent)
{
	rttEventDone(prAdapter,
		 (struct EVENT_RTT_DONE *) (prEvent->aucBuffer));
}

void nicEventRttResult(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent)
{
	rttEventResult(prAdapter,
		 (struct EVENT_RTT_RESULT *) (prEvent->aucBuffer));
}
#endif /* CFG_SUPPORT_RTT */

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
void nicCmdEventListmode(struct ADAPTER
				  *prAdapter, struct CMD_INFO *prCmdInfo,
				  uint8_t *pucEventBuf)
{
	struct list_mode_event *prStatus;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prStatus = (struct list_mode_event *) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		u4QueryInfoLen = sizeof(struct list_mode_event);

		/* Memory copy length is depended on upper-layer */
		kalMemCopy(&g_HqaListModeStatus, prStatus, u4QueryInfoLen);

		/* Update Query Information Length */
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#endif
#endif

#if (CFG_VOLT_INFO == 1)
void nicEventGetVnf(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	struct EVENT_GET_VOLT_INFO_T *prEventVnf;

	prEventVnf = (struct EVENT_GET_VOLT_INFO_T *)(prEvent->aucBuffer);
	DBGLOG(NIC, INFO, "FW current volt[%d], trigger volt info sync",
				prEventVnf->u2Volt);
	kalVnfEventHandler(prAdapter);
}
#endif /* CFG_VOLT_INFO */

#if CFG_SUPPORT_WIFI_POWER_METRICS
void nicEventPowerMetricsStatGetInfo(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	struct EVENT_POWER_METRICS_INFO_T *prEventVnf;

	prEventVnf = (struct EVENT_POWER_METRICS_INFO_T *)(prEvent->aucBuffer);

	DBGLOG(NIC, INFO, "NSS: 1T =%d 2T =%d\n",
				prEventVnf->u4Nss[0],
				prEventVnf->u4Nss[1]);

	DBGLOG(NIC, INFO,
		"Total =%d Band =%d Protocol =%d TX =%d RX =%d Listen =%d Sleep =%d\n",
		prEventVnf->u4TotalTime,
		prEventVnf->u4Band,
		prEventVnf->u4Protocol,
		prEventVnf->u4BandRatio.u4TxTime,
		prEventVnf->u4BandRatio.u4RxTime,
		prEventVnf->u4BandRatio.u4RxListenTime,
		prEventVnf->u4BandRatio.u4SleepTime);

	DBGLOG(NIC, INFO, "CCK: 1M =%d 2M =%d 5.5M =%d 11M =%d\n",
		prEventVnf->arStatsPmCckRateStat[0],
		prEventVnf->arStatsPmCckRateStat[1],
		prEventVnf->arStatsPmCckRateStat[2],
		prEventVnf->arStatsPmCckRateStat[3]);

	DBGLOG(NIC, INFO, "OFDM: 6M =%d 9M =%d 12M =%d 18M =%d\n",
		prEventVnf->arStatsPmOfdmRateStat[0],
		prEventVnf->arStatsPmOfdmRateStat[1],
		prEventVnf->arStatsPmOfdmRateStat[2],
		prEventVnf->arStatsPmOfdmRateStat[3]);

	DBGLOG(NIC, INFO, "OFDM: 24M =%d 36M =%d 48M =%d 54M =%d\n",
		prEventVnf->arStatsPmOfdmRateStat[4],
		prEventVnf->arStatsPmOfdmRateStat[5],
		prEventVnf->arStatsPmOfdmRateStat[6],
		prEventVnf->arStatsPmOfdmRateStat[7]);

	DBGLOG(NIC, INFO, "HT BW20: MCS0~7 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHtRateStat[0],
		prEventVnf->arStatsPmHtRateStat[1],
		prEventVnf->arStatsPmHtRateStat[2],
		prEventVnf->arStatsPmHtRateStat[3],
		prEventVnf->arStatsPmHtRateStat[4],
		prEventVnf->arStatsPmHtRateStat[5],
		prEventVnf->arStatsPmHtRateStat[6],
		prEventVnf->arStatsPmHtRateStat[7]);

	DBGLOG(NIC, INFO, "HT BW20: MCS8~15 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHtRateStat[8],
		prEventVnf->arStatsPmHtRateStat[9],
		prEventVnf->arStatsPmHtRateStat[10],
		prEventVnf->arStatsPmHtRateStat[11],
		prEventVnf->arStatsPmHtRateStat[12],
		prEventVnf->arStatsPmHtRateStat[13],
		prEventVnf->arStatsPmHtRateStat[14],
		prEventVnf->arStatsPmHtRateStat[15]);

	DBGLOG(NIC, INFO, "HT BW40: MCS0~7 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHtRateStat[16],
		prEventVnf->arStatsPmHtRateStat[17],
		prEventVnf->arStatsPmHtRateStat[18],
		prEventVnf->arStatsPmHtRateStat[19],
		prEventVnf->arStatsPmHtRateStat[20],
		prEventVnf->arStatsPmHtRateStat[21],
		prEventVnf->arStatsPmHtRateStat[22],
		prEventVnf->arStatsPmHtRateStat[23]);

	DBGLOG(NIC, INFO, "HT BW40: MCS8~15 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHtRateStat[24],
		prEventVnf->arStatsPmHtRateStat[25],
		prEventVnf->arStatsPmHtRateStat[26],
		prEventVnf->arStatsPmHtRateStat[27],
		prEventVnf->arStatsPmHtRateStat[28],
		prEventVnf->arStatsPmHtRateStat[29],
		prEventVnf->arStatsPmHtRateStat[30],
		prEventVnf->arStatsPmHtRateStat[31]);

	DBGLOG(NIC, INFO,
		"VHT BW20: MCS0~9 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmVhtRateStat[0],
		prEventVnf->arStatsPmVhtRateStat[1],
		prEventVnf->arStatsPmVhtRateStat[2],
		prEventVnf->arStatsPmVhtRateStat[3],
		prEventVnf->arStatsPmVhtRateStat[4],
		prEventVnf->arStatsPmVhtRateStat[5],
		prEventVnf->arStatsPmVhtRateStat[6],
		prEventVnf->arStatsPmVhtRateStat[7],
		prEventVnf->arStatsPmVhtRateStat[8],
		prEventVnf->arStatsPmVhtRateStat[9]);

	DBGLOG(NIC, INFO,
		"VHT BW40: MCS0~9 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmVhtRateStat[10],
		prEventVnf->arStatsPmVhtRateStat[11],
		prEventVnf->arStatsPmVhtRateStat[12],
		prEventVnf->arStatsPmVhtRateStat[13],
		prEventVnf->arStatsPmVhtRateStat[14],
		prEventVnf->arStatsPmVhtRateStat[15],
		prEventVnf->arStatsPmVhtRateStat[16],
		prEventVnf->arStatsPmVhtRateStat[17],
		prEventVnf->arStatsPmVhtRateStat[18],
		prEventVnf->arStatsPmVhtRateStat[19]);

	DBGLOG(NIC, INFO,
		"VHT BW80: MCS0~9 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmVhtRateStat[20],
		prEventVnf->arStatsPmVhtRateStat[21],
		prEventVnf->arStatsPmVhtRateStat[22],
		prEventVnf->arStatsPmVhtRateStat[23],
		prEventVnf->arStatsPmVhtRateStat[24],
		prEventVnf->arStatsPmVhtRateStat[25],
		prEventVnf->arStatsPmVhtRateStat[26],
		prEventVnf->arStatsPmVhtRateStat[27],
		prEventVnf->arStatsPmVhtRateStat[28],
		prEventVnf->arStatsPmVhtRateStat[29]);

	DBGLOG(NIC, INFO,
		"HE BW20: MCS0~11 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHeRateStat[0],
		prEventVnf->arStatsPmHeRateStat[1],
		prEventVnf->arStatsPmHeRateStat[2],
		prEventVnf->arStatsPmHeRateStat[3],
		prEventVnf->arStatsPmHeRateStat[4],
		prEventVnf->arStatsPmHeRateStat[5],
		prEventVnf->arStatsPmHeRateStat[6],
		prEventVnf->arStatsPmHeRateStat[7],
		prEventVnf->arStatsPmHeRateStat[8],
		prEventVnf->arStatsPmHeRateStat[9],
		prEventVnf->arStatsPmHeRateStat[10],
		prEventVnf->arStatsPmHeRateStat[11]);

	DBGLOG(NIC, INFO,
		"HE BW40: MCS0~11 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHeRateStat[12],
		prEventVnf->arStatsPmHeRateStat[13],
		prEventVnf->arStatsPmHeRateStat[14],
		prEventVnf->arStatsPmHeRateStat[15],
		prEventVnf->arStatsPmHeRateStat[16],
		prEventVnf->arStatsPmHeRateStat[17],
		prEventVnf->arStatsPmHeRateStat[18],
		prEventVnf->arStatsPmHeRateStat[19],
		prEventVnf->arStatsPmHeRateStat[20],
		prEventVnf->arStatsPmHeRateStat[21],
		prEventVnf->arStatsPmHeRateStat[22],
		prEventVnf->arStatsPmHeRateStat[23]);

	DBGLOG(NIC, INFO,
		"HE BW80: MCS0~11 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHeRateStat[24],
		prEventVnf->arStatsPmHeRateStat[25],
		prEventVnf->arStatsPmHeRateStat[26],
		prEventVnf->arStatsPmHeRateStat[27],
		prEventVnf->arStatsPmHeRateStat[28],
		prEventVnf->arStatsPmHeRateStat[29],
		prEventVnf->arStatsPmHeRateStat[30],
		prEventVnf->arStatsPmHeRateStat[31],
		prEventVnf->arStatsPmHeRateStat[32],
		prEventVnf->arStatsPmHeRateStat[33],
		prEventVnf->arStatsPmHeRateStat[34],
		prEventVnf->arStatsPmHeRateStat[35]);

	DBGLOG(NIC, INFO,
		"HE BW160: MCS0~11 :%d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d/ %d\n",
		prEventVnf->arStatsPmHeRateStat[36],
		prEventVnf->arStatsPmHeRateStat[37],
		prEventVnf->arStatsPmHeRateStat[38],
		prEventVnf->arStatsPmHeRateStat[39],
		prEventVnf->arStatsPmHeRateStat[40],
		prEventVnf->arStatsPmHeRateStat[41],
		prEventVnf->arStatsPmHeRateStat[42],
		prEventVnf->arStatsPmHeRateStat[43],
		prEventVnf->arStatsPmHeRateStat[44],
		prEventVnf->arStatsPmHeRateStat[45],
		prEventVnf->arStatsPmHeRateStat[46],
		prEventVnf->arStatsPmHeRateStat[47]);
}
#endif

void nicCmdEventGetSlpCntInfo(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct PARAM_SLEEP_CNT_INFO *prSlpCntInfo = NULL;
	struct PARAM_SLEEP_CNT_INFO *prInfoEvent = NULL;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	if (!pucEventBuf) {
		DBGLOG(NIC, ERROR, "NULL pucEventBuf!\n");
		return;
	}

	if (prCmdInfo->fgIsOid) {
		prSlpCntInfo = (struct PARAM_SLEEP_CNT_INFO *)
				prCmdInfo->pvInformationBuffer;
		prInfoEvent = (struct PARAM_SLEEP_CNT_INFO *)pucEventBuf;

		kalMemCopy(prSlpCntInfo, prInfoEvent,
			   sizeof(struct PARAM_SLEEP_CNT_INFO));

		prGlueInfo = prAdapter->prGlueInfo;
		u4QueryInfoLen = sizeof(struct PARAM_SLEEP_CNT_INFO);

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicCmdEventLpKeepPwrCtrl(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct CMD_LP_DBG_CTRL *prCmdLp = NULL;
	struct CMD_LP_DBG_CTRL *prInfoEvent = NULL;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	if (!prAdapter) {
		DBGLOG(NIC, ERROR, "NULL prAdapter!\n");
		return;
	}

	if (!prCmdInfo) {
		DBGLOG(NIC, ERROR, "NULL prCmdInfo!\n");
		return;
	}

	if (!pucEventBuf) {
		DBGLOG(NIC, ERROR, "NULL pucEventBuf!\n");
		return;
	}

	if (prCmdInfo->fgIsOid) {
		prCmdLp = (struct CMD_LP_DBG_CTRL *)
			   prCmdInfo->pvInformationBuffer;
		prInfoEvent = (struct CMD_LP_DBG_CTRL *)pucEventBuf;

		kalMemCopy(prCmdLp, prInfoEvent,
			   sizeof(struct CMD_LP_DBG_CTRL));

		prGlueInfo = prAdapter->prGlueInfo;
		u4QueryInfoLen = sizeof(struct CMD_LP_DBG_CTRL);

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#if (CFG_HW_DETECT_REPORT == 1)
void nicEventHwDetectReport(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent)
{
	struct EVENT_HW_DETECT_REPORT *prEventHwDetectReport;
	uint8_t str_buf[HW_DETECT_REPORT_STR_TO_NODE_MAX_LEN];

	if (!prAdapter->rWifiVar.fgHwDetectReportEn)
		return;

	prEventHwDetectReport =
		(struct EVENT_HW_DETECT_REPORT *)(prEvent->aucBuffer);

	if (snprintf(str_buf, HW_DETECT_REPORT_STR_TO_NODE_MAX_LEN,
		"[wlan]%s\n", prEventHwDetectReport->aucStrBuffer) < 0) {
		DBGLOG(NIC, ERROR,
			"HW Detect Report: %s copy failure\n", str_buf);
		return;
	}

	DBGLOG(NIC, INFO, "HW Detect Report: %s\n", str_buf);

	if (prEventHwDetectReport->fgIsReportNode) {
		/* Report to conninfra node */
		conn_dbg_add_log(CONN_DBG_LOG_TYPE_HW_ERR, str_buf);
	}

	if (prAdapter->rWifiVar.fgHwDetectReportEn == 2) {
		/* Trigger kernel warning */
		kalSendAeeWarning("WLAN", "HW Detect Report: %s\n", str_buf);
	}
}
#endif
