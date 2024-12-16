/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 * Id: //Department/DaVinci/BRANCHES/
 *     MT6620_WIFI_DRIVER_V2_3/nic/nic_cmd_event.c#3
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
#include "gl_vendor.h"

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
#if (CFG_SUPPORT_WFDMA_REALLOC == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_WFDMA_REALLOC,
				nicCmdEventQueryNicWfdmaRealloc),
#endif /* CFG_SUPPORT_WFDMA_REALLOC */
#if (CFG_SUPPORT_WIFI_6G == 1)
	NIC_FILL_CAP_V2_REF_TBL(TAG_CAP_6G_CAP,
				nicCfgChipCap6GCap),
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
#if CFG_SUPPORT_NAN
struct _TXM_CMD_EVENT_TEST_T grCmdInfoQueryTestBuffer;
#endif

/*******************************************************************************
 *                            F U N C T I O N   D A T A
 *******************************************************************************
 */
void nicCmdEventQueryMcrRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	return;

}

#if CFG_SUPPORT_QA_TOOL
void nicCmdEventQueryRxStatistics(IN struct ADAPTER
				  *prAdapter, IN struct CMD_INFO *prCmdInfo,
				  IN uint8_t *pucEventBuf)
{
	struct PARAM_CUSTOM_ACCESS_RX_STAT *prRxStatistics;
	struct EVENT_ACCESS_RX_STAT *prEventAccessRxStat;
	uint32_t u4QueryInfoLen, i;
	struct GLUE_INFO *prGlueInfo;
	uint32_t *prElement;
	uint32_t u4Temp;
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

		if (prRxStatistics->u4SeqNum == u4RxStatSeqNum) {
			prElement = &g_HqaRxStat.MAC_FCS_Err;
			for (i = 0; i < HQA_RX_STATISTIC_NUM; i++) {
				u4Temp = ntohl(
					prEventAccessRxStat->au4Buffer[i]);
				kalMemCopy(prElement, &u4Temp, 4);

				if (i < (HQA_RX_STATISTIC_NUM - 1))
					prElement++;
			}

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
		}

		DBGLOG(INIT, ERROR,
		       "MT6632 : RX Statistics Test SeqNum = %d, TotalNum = %d\n",
		       (unsigned int)prEventAccessRxStat->u4SeqNum,
		       (unsigned int)prEventAccessRxStat->u4TotalNum);

		DBGLOG(INIT, ERROR,
		       "MAC_FCS_ERR = %d, MAC_MDRDY = %d, MU_RX_CNT = %d, RX_FIFO_FULL = %d\n",
		       (unsigned int)prEventAccessRxStat->au4Buffer[0],
		       (unsigned int)prEventAccessRxStat->au4Buffer[1],
		       (unsigned int)prEventAccessRxStat->au4Buffer[65],
		       (unsigned int)prEventAccessRxStat->au4Buffer[22]);

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

#if CFG_SUPPORT_TX_BF
void nicCmdEventPfmuDataRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

		g_rPfmuData = *prEventPfmuDataRead;
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

void nicCmdEventPfmuTagRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

	g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1;
	g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2;

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

#if CFG_SUPPORT_MU_MIMO
void nicCmdEventGetQd(IN struct ADAPTER *prAdapter,
		      IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

	kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
		       u4QueryInfoLen, WLAN_STATUS_SUCCESS);

	DBGLOG(INIT, INFO, " event id : %x\n", prGetQd->u4EventId);
	for (i = 0; i < 14; i++)
		DBGLOG(INIT, INFO, "au4RawData[%d]: %x\n", i,
		       prGetQd->au4RawData[i]);

}

void nicCmdEventGetCalcLq(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

	kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
		       u4QueryInfoLen, WLAN_STATUS_SUCCESS);


	DBGLOG(INIT, INFO, " event id : %x\n",
	       prGetMuCalcLq->u4EventId);
	for (i = 0; i < NUM_OF_USER; i++)
		for (j = 0; j < NUM_OF_MODUL; j++)
			DBGLOG(INIT, INFO, " lq_report[%d][%d]: %x\n", i, j,
			       prGetMuCalcLq->rEntry.lq_report[i][j]);

}

void nicCmdEventGetCalcInitMcs(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

	kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
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
#endif /* CFG_SUPPORT_TX_BF */
#endif /* CFG_SUPPORT_QA_TOOL */

void nicCmdEventQuerySwCtrlRead(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventQueryChipConfig(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf)
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
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
		   u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventSetCommon(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
	    prCmdInfo->u4InformationBufferLength, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventSetIpAddress(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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
			prCmdInfo->fgSetQuery,
			OFFSET_OF(struct PARAM_NETWORK_ADDRESS_LIST,
			arAddress) + u4Count *
			(OFFSET_OF(struct PARAM_NETWORK_ADDRESS, aucAddress) +
				sizeof(struct PARAM_NETWORK_ADDRESS_IP)),
			WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventQueryRfTestATInfo(IN struct ADAPTER
				  *prAdapter, IN struct CMD_INFO *prCmdInfo,
				  IN uint8_t *pucEventBuf)
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
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventQueryLinkQuality(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf)
{
	int32_t rRssi, *prRssi;
	struct LINK_QUALITY *prLinkQuality;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prLinkQuality = (struct LINK_QUALITY *) pucEventBuf;

	/* ranged from (-128 ~ 30) in unit of dBm */
	rRssi = (int32_t)	prLinkQuality->cRssi;

	if (aisGetAisBssInfo(prAdapter, AIS_DEFAULT_INDEX)
		->eConnectionState == MEDIA_STATE_CONNECTED) {
		if (rRssi > PARAM_WHQL_RSSI_MAX_DBM)
			rRssi = PARAM_WHQL_RSSI_MAX_DBM;
		else if (rRssi < PARAM_WHQL_RSSI_MIN_DBM)
			rRssi = PARAM_WHQL_RSSI_MIN_DBM;
	} else {
		rRssi = PARAM_WHQL_RSSI_MIN_DBM;
	}

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prRssi = (int32_t *) prCmdInfo->pvInformationBuffer;

		kalMemCopy(prRssi, &rRssi, sizeof(int32_t));
		u4QueryInfoLen = sizeof(int32_t);

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
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
void nicCmdEventQueryLinkSpeed(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	struct LINK_QUALITY *prLinkQuality;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;
	uint32_t *pu4LinkSpeed;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prLinkQuality = (struct LINK_QUALITY *) pucEventBuf;

	prGlueInfo = prAdapter->prGlueInfo;
	pu4LinkSpeed = (uint32_t *) (prCmdInfo->pvInformationBuffer);
	*pu4LinkSpeed = prLinkQuality->u2LinkSpeed * 5000;

	u4QueryInfoLen = sizeof(uint32_t);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}

void nicCmdEventQueryLinkSpeedEx(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	struct EVENT_LINK_QUALITY *prLinkQuality;
	struct PARAM_LINK_SPEED_EX *pu4LinkSpeed;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;
	uint32_t i;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prLinkQuality = (struct EVENT_LINK_QUALITY *) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		pu4LinkSpeed = (struct PARAM_LINK_SPEED_EX *) (
					   prCmdInfo->pvInformationBuffer);

		for (i = 0; i < BSSID_NUM; i++) {
			pu4LinkSpeed->rLq[i].u2LinkSpeed
				= prLinkQuality->rLq[i].u2LinkSpeed * 5000;

			/* ranged from (-128 ~ 30) in unit of dBm */
			pu4LinkSpeed->rLq[i].cRssi
				= prLinkQuality->rLq[i].cRssi;

			DBGLOG(P2P, TRACE,
				"ucBssIdx = %d, rate = %u, signal = %d\n",
				i,
				pu4LinkSpeed->rLq[i].u2LinkSpeed,
				pu4LinkSpeed->rLq[i].cRssi);
		}

		u4QueryInfoLen = sizeof(struct PARAM_LINK_SPEED_EX);

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicCmdEventQueryStatistics(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf)
{
	struct PARAM_802_11_STATISTICS_STRUCT *prStatistics;
	struct EVENT_STATISTICS *prEventStatistics;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo;
#endif

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (struct EVENT_STATISTICS *) pucEventBuf;

	prGlueInfo = prAdapter->prGlueInfo;

	u4QueryInfoLen = sizeof(struct
				PARAM_802_11_STATISTICS_STRUCT);
	prStatistics = (struct PARAM_802_11_STATISTICS_STRUCT *)
		       prCmdInfo->pvInformationBuffer;

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
	prStatistics->rMdrdyCnt = prEventStatistics->rMdrdyCnt;
	prStatistics->rChnlIdleCnt = prEventStatistics->rChnlIdleCnt;

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
	prLinkQualityInfo->u8RxErrCount +=
		prStatistics->rFCSErrorCount.QuadPart;
	prLinkQualityInfo->u8MdrdyCount =
		prStatistics->rMdrdyCnt.QuadPart;
	prLinkQualityInfo->u8IdleSlotCount =
		prStatistics->rChnlIdleCnt.QuadPart;

	wlanFinishCollectingLinkQuality(prGlueInfo);

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

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}

void nicCmdEventQueryBugReport(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicCmdEventEnterRfTest(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	uint32_t u4Idx = 0;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	/* [driver-land] */
	 prAdapter->fgTestMode = TRUE;

	/* 0. always indicate disconnection */
	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		if (kalGetMediaStateIndicated(
			prAdapter->prGlueInfo,
			u4Idx) !=
		    MEDIA_STATE_DISCONNECTED)
			kalIndicateStatusAndComplete(
				prAdapter->prGlueInfo,
				WLAN_STATUS_MEDIA_DISCONNECT,
				NULL, 0, u4Idx);
	}
	/* 1. Remove pending TX */
	nicTxRelease(prAdapter, TRUE);

	/* 1.1 clear pending Security / Management Frames */
	kalClearSecurityFrames(prAdapter->prGlueInfo);
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
					prCmdInfo->fgSetQuery,
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
			       prCmdInfo->fgSetQuery, prCmdInfo->u4SetInfoLen,
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

void nicCmdEventLeaveRfTest(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	uint32_t u4Idx = 0;

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
					prCmdInfo->fgSetQuery,
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

	/* 7. completion indication */
	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery, prCmdInfo->u4SetInfoLen,
			       WLAN_STATUS_SUCCESS);
	}

	/* 8. Indicate as disconnected */
	for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
		if (kalGetMediaStateIndicated(
			prAdapter->prGlueInfo,
			u4Idx) !=
		    MEDIA_STATE_DISCONNECTED) {

			kalIndicateStatusAndComplete(
				prAdapter->prGlueInfo,
				WLAN_STATUS_MEDIA_DISCONNECT,
				NULL, 0, u4Idx);

			prAdapter->rWlanInfo.u4SysTime =
				kalGetTimeTick();
		}
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

	/* 10. Override network address */
	wlanUpdateNetworkAddress(prAdapter);

}

void nicCmdEventQueryMcastAddr(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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
			kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
				u4QueryInfoLen, WLAN_STATUS_BUFFER_TOO_SHORT);
		} else {
			kalMemCopy(prCmdInfo->pvInformationBuffer,
				prEventMacMcastAddr->arAddress,
				prEventMacMcastAddr->u4NumOfGroupAddr *
					MAC_ADDR_LEN);

			kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
				       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
		}
	}
}

void nicCmdEventQueryEepromRead(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

void nicCmdEventSetMediaStreamMode(IN struct ADAPTER
				   *prAdapter, IN struct CMD_INFO *prCmdInfo,
				   IN uint8_t *pucEventBuf)
{
	struct PARAM_MEDIA_STREAMING_INDICATION
		rParamMediaStreamIndication;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery, prCmdInfo->u4SetInfoLen,
			       WLAN_STATUS_SUCCESS);
	}

	rParamMediaStreamIndication.rStatus.eStatusType =
		ENUM_STATUS_TYPE_MEDIA_STREAM_MODE;
	rParamMediaStreamIndication.eMediaStreamMode =
		prAdapter->rWlanInfo.eLinkAttr.ucMediaStreamMode == 0 ?
		ENUM_MEDIA_STREAM_OFF : ENUM_MEDIA_STREAM_ON;

	kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
		WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
		(void *)&rParamMediaStreamIndication,
		sizeof(struct PARAM_MEDIA_STREAMING_INDICATION),
		AIS_DEFAULT_INDEX);
}

void nicCmdEventSetStopSchedScan(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf)
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
			prCmdInfo->fgSetQuery,
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
void nicOidCmdTimeoutCommon(IN struct ADAPTER *prAdapter,
			    IN struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
			       0, WLAN_STATUS_FAILURE);
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
void nicCmdTimeoutCommon(IN struct ADAPTER *prAdapter,
			 IN struct CMD_INFO *prCmdInfo)
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
void nicOidCmdEnterRFTestTimeout(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	/* 1. Remove pending TX frames */
	nicTxRelease(prAdapter, TRUE);

	/* 1.1 clear pending Security / Management Frames */
	kalClearSecurityFrames(prAdapter->prGlueInfo);
	kalClearMgmtFrames(prAdapter->prGlueInfo);

	/* 1.2 clear pending TX packet queued in glue layer */
	kalFlushPendingTxPackets(prAdapter->prGlueInfo);

	/* 2. indicate for OID failure */
	kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
		       0, WLAN_STATUS_FAILURE);
}

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when received dump memory event packet.
 *        transfer the memory data to the IQ format data and write into file
 *
 * @param prIQAry     Pointer to the array store I or Q data.
 *		 prDataLen   The return data length - bytes
 *        u4IQ        0: get I data
 *                    1 : get Q data
 *
 * @return -1: open file error
 *
 */
/*----------------------------------------------------------------------------*/
int32_t GetIQData(struct ADAPTER *prAdapter,
		  int32_t **prIQAry, uint32_t *prDataLen, uint32_t u4IQ,
		  uint32_t u4GetWf1)
{
	uint8_t aucPath[50];	/* the path for iq data dump out */
	uint8_t aucData[50];	/* iq data in string format */
	uint32_t i = 0, j = 0, count = 0;
	int32_t ret = -1;
	int32_t rv;
	struct file *file = NULL;

	*prIQAry = prAdapter->rIcapInfo.au4IQData;

	/* sprintf(aucPath, "/pattern.txt");             // CSD's Pattern */
	kalSprintf(aucPath, "/tmp/dump_out_%05hu_WF%u.txt",
		prAdapter->rIcapInfo.u2DumpIndex - 1, u4GetWf1);
	if (kalCheckPath(aucPath) == -1) {
		kalSnprintf(aucPath, sizeof(aucPath),
			"/data/dump_out_%05hu_WF%u.txt",
			prAdapter->rIcapInfo.u2DumpIndex - 1, u4GetWf1);
	}

	DBGLOG(INIT, INFO,
	       "iCap Read Dump File dump_out_%05hu_WF%u.txt\n",
	       prAdapter->rIcapInfo.u2DumpIndex - 1, u4GetWf1);

	file = kalFileOpen(aucPath, O_RDONLY, 0);

	if ((file != NULL) && !IS_ERR(file)) {
		/* read 1K data per time */
		for (i = 0; i < RTN_IQ_DATA_LEN / sizeof(uint32_t);
		     i++, prAdapter->rIcapInfo.au4Offset[u4GetWf1][u4IQ] +=
			     IQ_FILE_LINE_OFFSET) {
			if (kalFileRead(file,
				prAdapter->rIcapInfo.au4Offset[u4GetWf1][u4IQ],
				aucData, IQ_FILE_IQ_STR_LEN) == 0)
				break;

			count = 0;

			for (j = 0; j < 8; j++) {
				if (aucData[j] != ' ')
					aucData[count++] = aucData[j];
			}

			aucData[count] = '\0';

			/* transfer data format (string to int) */
			rv = kalStrtoint(aucData, 0,
				       &prAdapter->rIcapInfo.au4IQData[i]);
		}
		*prDataLen = i * sizeof(uint32_t);
		kalFileClose(file);
		ret = 0;
	}

	DBGLOG(INIT, INFO,
	       "MT6632 : QA_AGENT GetIQData prDataLen = %d\n", *prDataLen);
	DBGLOG(INIT, INFO, "MT6632 : QA_AGENT GetIQData i = %d\n",
	       i);

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when received dump memory event packet.
 *        transfer the memory data to the IQ format data and write into file
 *
 * @param prEventDumpMem     Pointer to the event dump memory structure.
 *
 * @return 0: SUCCESS, -1: FAIL
 *
 */
/*----------------------------------------------------------------------------*/

uint32_t nicTsfRawData2IqFmt(struct EVENT_DUMP_MEM
			     *prEventDumpMem, struct ICAP_INFO_T *prIcap)
{
	static uint8_t
	aucPathWF0[40];	/* the path for iq data dump out */
	static uint8_t
	aucPathWF1[40];	/* the path for iq data dump out */
	static uint8_t
	aucPathRAWWF0[40];	/* the path for iq data dump out */
	static uint8_t
	aucPathRAWWF1[40];	/* the path for iq data dump out */
	uint8_t *pucDataWF0 = NULL;	/* the data write into file */
	uint8_t *pucDataWF1 = NULL;	/* the data write into file */
	uint8_t *pucDataRAWWF0 =
		NULL;	/* the data write into file */
	uint8_t *pucDataRAWWF1 =
		NULL;	/* the data write into file */
	uint32_t u4SrcOffset;	/* record the buffer offset */
	uint32_t u4FmtLen = 0;	/* bus format length */
	uint32_t u4CpyLen = 0;
	uint32_t u4RemainByte;
	uint32_t u4DataWBufSize = 150;
	uint32_t u4DataRAWWBufSize = 150;
	uint32_t u4DataWLenF0 = 0;
	uint32_t u4DataWLenF1 = 0;
	uint32_t u4DataRAWWLenF0 = 0;
	uint32_t u4DataRAWWLenF1 = 0;

	u_int8_t fgAppend;
	int32_t u4Iqc160WF0Q0, u4Iqc160WF1I1;

	static uint8_t
	ucDstOffset;	/* for alignment. bcs we send 2KB data per packet,*/
	/*the data will not align in 12 bytes case. */
	static uint32_t u4CurTimeTick;

	static union ICAP_BUS_FMT icapBusData;
	uint32_t *ptr;

	pucDataWF0 = kmalloc(u4DataWBufSize, GFP_KERNEL);
	pucDataWF1 = kmalloc(u4DataWBufSize, GFP_KERNEL);
	pucDataRAWWF0 = kmalloc(u4DataRAWWBufSize, GFP_KERNEL);
	pucDataRAWWF1 = kmalloc(u4DataRAWWBufSize, GFP_KERNEL);


	if ((!pucDataWF0) || (!pucDataWF1) || (!pucDataRAWWF0)
	    || (!pucDataRAWWF1)) {
		DBGLOG(INIT, ERROR, "kmalloc failed.\n");
		kfree(pucDataWF0);
		kfree(pucDataWF1);
		kfree(pucDataRAWWF0);
		kfree(pucDataRAWWF1);
		ASSERT(-1);
		return -1;
	}

	fgAppend = TRUE;
	if (prEventDumpMem->ucFragNum == 1) {

		u4CurTimeTick = kalGetTimeTick();
		/* Store memory dump into sdcard,
		 * path /sdcard/dump_<current  system tick>_
		 * <memory address>_<memory length>.hex
		 */
#if defined(LINUX)

		/* if blbist mkdir undre /data/blbist,
		 * the dump files wouls put on it
		 */
		kalScnprintf(aucPathWF0, sizeof(aucPathWF0),
			  "/tmp/dump_out_%05hu_WF0.txt", prIcap->u2DumpIndex);
		kalScnprintf(aucPathWF1, sizeof(aucPathWF1),
			  "/tmp/dump_out_%05hu_WF1.txt", prIcap->u2DumpIndex);
		if (kalCheckPath(aucPathWF0) == -1) {
			kalMemSet(aucPathWF0, 0x00, sizeof(aucPathWF0));
			kalScnprintf(aucPathWF0, sizeof(aucPathWF0),
				"/data/dump_out_%05hu_WF0.txt",
				prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathWF0);

		if (kalCheckPath(aucPathWF1) == -1) {
			kalMemSet(aucPathWF1, 0x00, sizeof(aucPathWF1));
			kalScnprintf(aucPathWF1, sizeof(aucPathWF1),
				"/data/dump_out_%05hu_WF1.txt",
				prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathWF1);

		kalScnprintf(aucPathRAWWF0, sizeof(aucPathRAWWF0),
			  "/dump_RAW_%hu_WF0.txt", prIcap->u2DumpIndex);
		kalScnprintf(aucPathRAWWF1, sizeof(aucPathRAWWF1),
			  "/dump_RAW_%hu_WF1.txt", prIcap->u2DumpIndex);
		if (kalCheckPath(aucPathRAWWF0) == -1) {
			kalMemSet(aucPathRAWWF0, 0x00, sizeof(aucPathRAWWF0));
			kalScnprintf(aucPathRAWWF0, sizeof(aucPathRAWWF0),
				"/data/dump_RAW_%05hu_WF0.txt",
				prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathRAWWF0);

		if (kalCheckPath(aucPathRAWWF1) == -1) {
			kalMemSet(aucPathRAWWF1, 0x00, sizeof(aucPathRAWWF1));
			kalScnprintf(aucPathRAWWF1, sizeof(aucPathRAWWF1),
				"/data/dump_RAW_%05hu_WF1.txt",
				prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathRAWWF1);

#else
		kal_sprintf_ddk(aucPathWF0, sizeof(aucPathWF0),
				u4CurTimeTick, prEventDumpMem->u4Address,
				prEventDumpMem->u4Length +
				prEventDumpMem->u4RemainLength);
		kal_sprintf_ddk(aucPathWF1, sizeof(aucPathWF1),
				u4CurTimeTick, prEventDumpMem->u4Address,
				prEventDumpMem->u4Length +
				prEventDumpMem->u4RemainLength);
#endif
		/* fgAppend = FALSE; */
	}

	ptr = (uint32_t *)(&prEventDumpMem->aucBuffer[0]);

	for (u4SrcOffset = 0,
	     u4RemainByte = prEventDumpMem->u4Length; u4RemainByte > 0;
	    ) {
		u4FmtLen =
			(prEventDumpMem->eIcapContent
				== ICAP_CONTENT_SPECTRUM) ?
			sizeof(struct SPECTRUM_BUS_FMT) : sizeof(union
					ICAP_BUS_FMT);
		/* 4 bytes : 12 bytes */
		u4CpyLen = (u4RemainByte >= u4FmtLen) ? u4FmtLen :
			   u4RemainByte;

		if ((ucDstOffset + u4CpyLen) > sizeof(icapBusData)) {
			DBGLOG(INIT, ERROR,
			       "ucDstOffset(%u) + u4CpyLen(%u) exceed bound of icapBusData\n",
			       ucDstOffset, u4CpyLen);
			kfree(pucDataWF0);
			kfree(pucDataWF1);
			kfree(pucDataRAWWF0);
			kfree(pucDataRAWWF1);
			ASSERT(-1);
			return -1;
		}
		memcpy((uint8_t *)&icapBusData + ucDstOffset,
		       &prEventDumpMem->aucBuffer[0] + u4SrcOffset, u4CpyLen);
#if 0
		if (prEventDumpMem->eIcapContent == ICAP_CONTENT_ADC) {
			sprintf(aucDataWF0, "%8d,%8d\n",
				icapBusData.rAdcBusData.u4Dcoc0I,
				icapBusData.rAdcBusData.u4Dcoc0Q);
			sprintf(aucDataWF1, "%8d,%8d\n",
				icapBusData.rAdcBusData.u4Dcoc1I,
				icapBusData.rAdcBusData.u4Dcoc1Q);
		}
#endif
		if (prEventDumpMem->eIcapContent == ICAP_CONTENT_FIIQ ||
		    prEventDumpMem->eIcapContent == ICAP_CONTENT_FDIQ) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rIqcBusData.u4Iqc0I,
				icapBusData.rIqcBusData.u4Iqc0Q);
			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rIqcBusData.u4Iqc1I,
				icapBusData.rIqcBusData.u4Iqc1Q);
		} else if (prEventDumpMem->eIcapContent - 1000 ==
			   ICAP_CONTENT_FIIQ
			   || prEventDumpMem->eIcapContent - 1000 ==
			   ICAP_CONTENT_FDIQ) {
			u4Iqc160WF0Q0 =
				icapBusData.rIqc160BusData.u4Iqc0Q0P1 |
				(icapBusData.rIqc160BusData.u4Iqc0Q0P2 << 8);
			u4Iqc160WF1I1 =
				icapBusData.rIqc160BusData.u4Iqc1I1P1 |
				(icapBusData.rIqc160BusData.u4Iqc1I1P2 << 4);

			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n",
				icapBusData.rIqc160BusData.u4Iqc0I0,
				u4Iqc160WF0Q0,
				icapBusData.rIqc160BusData.u4Iqc0I1,
				icapBusData.rIqc160BusData.u4Iqc0Q1);

			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n",
				icapBusData.rIqc160BusData.u4Iqc1I0,
				icapBusData.rIqc160BusData.u4Iqc1Q0,
				u4Iqc160WF1I1,
				icapBusData.rIqc160BusData.u4Iqc1Q1);

		} else if (prEventDumpMem->eIcapContent ==
			   ICAP_CONTENT_SPECTRUM) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rSpectrumBusData.u4DcocI,
				icapBusData.rSpectrumBusData.u4DcocQ);
		} else if (prEventDumpMem->eIcapContent ==
			   ICAP_CONTENT_ADC) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI0T0,
				icapBusData.rPackedAdcBusData.u4AdcQ0T0,
				icapBusData.rPackedAdcBusData.u4AdcI0T1,
				icapBusData.rPackedAdcBusData.u4AdcQ0T1,
				icapBusData.rPackedAdcBusData.u4AdcI0T2,
				icapBusData.rPackedAdcBusData.u4AdcQ0T2,
				icapBusData.rPackedAdcBusData.u4AdcI0T3,
				icapBusData.rPackedAdcBusData.u4AdcQ0T3,
				icapBusData.rPackedAdcBusData.u4AdcI0T4,
				icapBusData.rPackedAdcBusData.u4AdcQ0T4,
				icapBusData.rPackedAdcBusData.u4AdcI0T5,
				icapBusData.rPackedAdcBusData.u4AdcQ0T5);

			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI1T0,
				icapBusData.rPackedAdcBusData.u4AdcQ1T0,
				icapBusData.rPackedAdcBusData.u4AdcI1T1,
				icapBusData.rPackedAdcBusData.u4AdcQ1T1,
				icapBusData.rPackedAdcBusData.u4AdcI1T2,
				icapBusData.rPackedAdcBusData.u4AdcQ1T2,
				icapBusData.rPackedAdcBusData.u4AdcI1T3,
				icapBusData.rPackedAdcBusData.u4AdcQ1T3,
				icapBusData.rPackedAdcBusData.u4AdcI1T4,
				icapBusData.rPackedAdcBusData.u4AdcQ1T4,
				icapBusData.rPackedAdcBusData.u4AdcI1T5,
				icapBusData.rPackedAdcBusData.u4AdcQ1T5);
		} else if (prEventDumpMem->eIcapContent - 2000 ==
			   ICAP_CONTENT_ADC) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI0T0,
				icapBusData.rPackedAdcBusData.u4AdcQ0T0,
				icapBusData.rPackedAdcBusData.u4AdcI0T1,
				icapBusData.rPackedAdcBusData.u4AdcQ0T1,
				icapBusData.rPackedAdcBusData.u4AdcI0T2,
				icapBusData.rPackedAdcBusData.u4AdcQ0T2);

			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI1T0,
				icapBusData.rPackedAdcBusData.u4AdcQ1T0,
				icapBusData.rPackedAdcBusData.u4AdcI1T1,
				icapBusData.rPackedAdcBusData.u4AdcQ1T1,
				icapBusData.rPackedAdcBusData.u4AdcI1T2,
				icapBusData.rPackedAdcBusData.u4AdcQ1T2);
		} else if (prEventDumpMem->eIcapContent ==
			   ICAP_CONTENT_TOAE) {
			/* actually, this is DCOC. we take TOAE as DCOC */
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rAdcBusData.u4Dcoc0I,
				icapBusData.rAdcBusData.u4Dcoc0Q);
			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rAdcBusData.u4Dcoc1I,
				icapBusData.rAdcBusData.u4Dcoc1Q);
		}
		if (u4CpyLen ==
		    u4FmtLen) {	/* the data format is complete */
			kalWriteToFile(aucPathWF0, fgAppend, pucDataWF0,
				       u4DataWLenF0);
			kalWriteToFile(aucPathWF1, fgAppend, pucDataWF1,
				       u4DataWLenF1);
		}
		ptr = (uint32_t *)(&prEventDumpMem->aucBuffer[0] +
				   u4SrcOffset);
		u4DataRAWWLenF0 = kalScnprintf(pucDataRAWWF0, u4DataWBufSize,
					    "%08x%08x%08x\n",
					    *(ptr + 2), *(ptr + 1), *ptr);
		kalWriteToFile(aucPathRAWWF0, fgAppend, pucDataRAWWF0,
			       u4DataRAWWLenF0);
		kalWriteToFile(aucPathRAWWF1, fgAppend, pucDataRAWWF1,
			       u4DataRAWWLenF1);

		u4RemainByte -= u4CpyLen;
		u4SrcOffset += u4CpyLen;	/* shift offset */
		/* only use ucDstOffset at first packet for align 2KB */
		ucDstOffset =	0;
	}
	/* if this is a last packet, we can't transfer the remain data.
	 *  bcs we can't guarantee the data is complete align data format
	 */
	/* the data format is complete */
	if (u4CpyLen != u4FmtLen) {
		/* not align 2KB, keep the data
		 * and next packet data will append it
		 */
		ucDstOffset =	u4CpyLen;
	}

	kfree(pucDataWF0);
	kfree(pucDataWF1);
	kfree(pucDataRAWWF0);
	kfree(pucDataRAWWF1);

	return 0;
}
#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
void nicCmdEventQueryCalBackupV2(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf)
{
	struct PARAM_CAL_BACKUP_STRUCT_V2 *prCalBackupDataV2Info;
	struct CMD_CAL_BACKUP_STRUCT_V2 *prEventCalBackupDataV2;
	uint32_t u4QueryInfoLen, u4QueryInfo, u4TempAddress;
	struct GLUE_INFO *prGlueInfo;

	DBGLOG(RFTEST, INFO, "%s\n", __func__);

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);


	prGlueInfo = prAdapter->prGlueInfo;
	prEventCalBackupDataV2 = (struct CMD_CAL_BACKUP_STRUCT_V2 *)
				 (pucEventBuf);

	u4QueryInfoLen = sizeof(struct CMD_CAL_BACKUP_STRUCT_V2);

	prCalBackupDataV2Info = (struct PARAM_CAL_BACKUP_STRUCT_V2
				 *) prCmdInfo->pvInformationBuffer;
#if 0
	DBGLOG(RFTEST, INFO,
	       "============ Receive a Cal Data EVENT (Info) ============\n");
	DBGLOG(RFTEST, INFO, "Reason = %d\n",
	       prEventCalBackupDataV2->ucReason);
	DBGLOG(RFTEST, INFO, "Action = %d\n",
	       prEventCalBackupDataV2->ucAction);
	DBGLOG(RFTEST, INFO, "NeedResp = %d\n",
	       prEventCalBackupDataV2->ucNeedResp);
	DBGLOG(RFTEST, INFO, "FragNum = %d\n",
	       prEventCalBackupDataV2->ucFragNum);
	DBGLOG(RFTEST, INFO, "RomRam = %d\n",
	       prEventCalBackupDataV2->ucRomRam);
	DBGLOG(RFTEST, INFO, "ThermalValue = %d\n",
	       prEventCalBackupDataV2->u4ThermalValue);
	DBGLOG(RFTEST, INFO, "Address = 0x%08x\n",
	       prEventCalBackupDataV2->u4Address);
	DBGLOG(RFTEST, INFO, "Length = %d\n",
	       prEventCalBackupDataV2->u4Length);
	DBGLOG(RFTEST, INFO, "RemainLength = %d\n",
	       prEventCalBackupDataV2->u4RemainLength);
	DBGLOG(RFTEST, INFO,
	       "=========================================================\n");
#endif

	if (prEventCalBackupDataV2->ucReason == 0
	    && prEventCalBackupDataV2->ucAction == 0) {
		DBGLOG(RFTEST, INFO,
		       "Received an EVENT for Query Thermal Temp.\n");
		prCalBackupDataV2Info->u4ThermalValue =
			prEventCalBackupDataV2->u4ThermalValue;
		g_rBackupCalDataAllV2.u4ThermalInfo =
			prEventCalBackupDataV2->u4ThermalValue;
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	} else if (prEventCalBackupDataV2->ucReason == 0
		   && prEventCalBackupDataV2->ucAction == 1) {
		DBGLOG(RFTEST, INFO,
		       "Received an EVENT for Query Total Cal Data Length.\n");
		prCalBackupDataV2Info->u4Length =
			prEventCalBackupDataV2->u4Length;

		if (prEventCalBackupDataV2->ucRomRam == 0)
			g_rBackupCalDataAllV2.u4ValidRomCalDataLength =
				prEventCalBackupDataV2->u4Length;
		else if (prEventCalBackupDataV2->ucRomRam == 1)
			g_rBackupCalDataAllV2.u4ValidRamCalDataLength =
				prEventCalBackupDataV2->u4Length;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	} else if (prEventCalBackupDataV2->ucReason == 2
		   && prEventCalBackupDataV2->ucAction == 4) {
		DBGLOG(RFTEST, INFO,
		       "Received an EVENT for Query All Cal (%s) Data. FragNum = %d\n",
		       prCalBackupDataV2Info->ucRomRam == 0 ? "ROM" : "RAM",
		       prEventCalBackupDataV2->ucFragNum);
		prCalBackupDataV2Info->u4Address =
			prEventCalBackupDataV2->u4Address;
		prCalBackupDataV2Info->u4Length =
			prEventCalBackupDataV2->u4Length;
		prCalBackupDataV2Info->u4RemainLength =
			prEventCalBackupDataV2->u4RemainLength;
		prCalBackupDataV2Info->ucFragNum =
			prEventCalBackupDataV2->ucFragNum;

		/* Copy Cal Data From FW to Driver Array */
		if (prEventCalBackupDataV2->ucRomRam == 0) {
			u4TempAddress = prEventCalBackupDataV2->u4Address;
			kalMemCopy(
				(uint8_t *)(g_rBackupCalDataAllV2.au4RomCalData)
				+ u4TempAddress,
				(uint8_t *)(prEventCalBackupDataV2->au4Buffer),
				prEventCalBackupDataV2->u4Length);
		} else if (prEventCalBackupDataV2->ucRomRam == 1) {
			u4TempAddress = prEventCalBackupDataV2->u4Address;
			kalMemCopy(
				(uint8_t *)(g_rBackupCalDataAllV2.au4RamCalData)
				+ u4TempAddress,
				(uint8_t *)(prEventCalBackupDataV2->au4Buffer),
				prEventCalBackupDataV2->u4Length);
		}

		if (prEventCalBackupDataV2->u4Address == 0xFFFFFFFF) {
			DBGLOG(RFTEST, INFO,
			  "RLM CMD : Address Error!!!!!!!!!!!\n");
		} else if (prEventCalBackupDataV2->u4RemainLength == 0
			   && prEventCalBackupDataV2->ucRomRam == 1) {
			DBGLOG(RFTEST, INFO,
				"RLM CMD : Get Cal Data from FW (%s). Finish!!!!!!!!!!!\n",
			  prCalBackupDataV2Info->ucRomRam == 0 ? "ROM" : "RAM");
		} else if (prEventCalBackupDataV2->u4RemainLength == 0
			   && prEventCalBackupDataV2->ucRomRam == 0) {
			DBGLOG(RFTEST, INFO,
				"RLM CMD : Get Cal Data from FW (%s). Finish!!!!!!!!!!!\n",
			  prCalBackupDataV2Info->ucRomRam == 0 ? "ROM" : "RAM");
			prCalBackupDataV2Info->ucFragNum = 0;
			prCalBackupDataV2Info->ucRomRam = 1;
			prCalBackupDataV2Info->u4ThermalValue = 0;
			prCalBackupDataV2Info->u4Address = 0;
			prCalBackupDataV2Info->u4Length = 0;
			prCalBackupDataV2Info->u4RemainLength = 0;
			DBGLOG(RFTEST, INFO,
				"RLM CMD : Get Cal Data from FW (%s). Start!!!!!!!!!!!!!!!!\n",
			  prCalBackupDataV2Info->ucRomRam == 0 ? "ROM" : "RAM");
			DBGLOG(RFTEST, INFO, "Thermal Temp = %d\n",
			       g_rBackupCalDataAllV2.u4ThermalInfo);
			wlanoidQueryCalBackupV2(prAdapter,
				prCalBackupDataV2Info,
				sizeof(struct PARAM_CAL_BACKUP_STRUCT_V2),
				&u4QueryInfo);
		} else {
			wlanoidSendCalBackupV2Cmd(prAdapter,
				prCmdInfo->pvInformationBuffer,
				prCmdInfo->u4InformationBufferLength);
		}
	} else if (prEventCalBackupDataV2->ucReason == 3
		   && prEventCalBackupDataV2->ucAction == 5) {
		DBGLOG(RFTEST, INFO,
		       "Received an EVENT for Send All Cal Data. FragNum = %d\n",
		       prEventCalBackupDataV2->ucFragNum);
		prCalBackupDataV2Info->u4Address =
			prEventCalBackupDataV2->u4Address;
		prCalBackupDataV2Info->u4Length =
			prEventCalBackupDataV2->u4Length;
		prCalBackupDataV2Info->u4RemainLength =
			prEventCalBackupDataV2->u4RemainLength;
		prCalBackupDataV2Info->ucFragNum =
			prEventCalBackupDataV2->ucFragNum;

		if (prEventCalBackupDataV2->u4RemainLength == 0
		    || prEventCalBackupDataV2->u4Address == 0xFFFFFFFF) {
			kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
				       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
		} else {
			wlanoidSendCalBackupV2Cmd(prAdapter,
				prCmdInfo->pvInformationBuffer,
				prCmdInfo->u4InformationBufferLength);
		}
	} else {
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to handle dump burst event
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo         Pointer to the command information
 * @param pucEventBuf       Pointer to event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicEventQueryMemDump(IN struct ADAPTER *prAdapter,
			  IN uint8_t *pucEventBuf)
{
	struct EVENT_DUMP_MEM *prEventDumpMem;
	static uint8_t aucPath[256];
	static uint8_t aucPath_done[300];
	static uint32_t u4CurTimeTick;
	int ret;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	ret = kalSnprintf(aucPath, sizeof(aucPath), "/dump_%05hu.hex",
		prAdapter->rIcapInfo.u2DumpIndex);
	if (ret == 0 || ret >= sizeof(aucPath))
		DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);

	prEventDumpMem = (struct EVENT_DUMP_MEM *) (pucEventBuf);

	if (kalCheckPath(aucPath) == -1) {
		kalMemSet(aucPath, 0x00, 256);
		ret = kalSnprintf(aucPath, sizeof(aucPath),
			"/data/dump_%05hu.hex",
			prAdapter->rIcapInfo.u2DumpIndex);
		if (ret == 0 || ret >= sizeof(aucPath))
			DBGLOG(INIT, ERROR,
				"[%u] kalSnprintf failed, ret: %d\n",
				__LINE__, ret);
	}

	if (prEventDumpMem->ucFragNum == 1) {
		/* Store memory dump into sdcard,
		 * path /sdcard/dump_<current  system tick>_
		 * <memory address>_<memory length>.hex
		 */
		u4CurTimeTick = kalGetTimeTick();
#if defined(LINUX)

		/* if blbist mkdir undre /data/blbist,
		 * the dump files wouls put on it
		 */
		ret = kalSnprintf(aucPath, sizeof(aucPath),
			"/dump_%05hu.hex",
			prAdapter->rIcapInfo.u2DumpIndex);
		if (ret == 0 || ret >= sizeof(aucPath))
			DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);

		if (kalCheckPath(aucPath) == -1) {
			kalMemSet(aucPath, 0x00, 256);
			ret = kalSnprintf(aucPath, sizeof(aucPath),
				"/data/dump_%05hu.hex",
				prAdapter->rIcapInfo.u2DumpIndex);
				if (ret == 0 || ret >= sizeof(aucPath))
				DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);
		}
#else
		kal_sprintf_ddk(aucPath, sizeof(aucPath),
				u4CurTimeTick,
				prEventDumpMem->u4Address,
				prEventDumpMem->u4Length +
				prEventDumpMem->u4RemainLength);
#endif
		kalWriteToFile(aucPath, FALSE,
			&prEventDumpMem->aucBuffer[0],
			prEventDumpMem->u4Length);
	} else {
		/* Append current memory dump to the hex file */
		kalWriteToFile(aucPath, TRUE, &prEventDumpMem->aucBuffer[0],
			       prEventDumpMem->u4Length);
	}
#if CFG_SUPPORT_QA_TOOL
	nicTsfRawData2IqFmt(prEventDumpMem, &prAdapter->rIcapInfo);
#endif /* CFG_SUPPORT_QA_TOOL */
	DBGLOG(INIT, INFO,
	       "iCap : ==> (u4RemainLength = %x, u4Address=%x )\n",
	       prEventDumpMem->u4RemainLength,
	       prEventDumpMem->u4Address);

	if (prEventDumpMem->u4RemainLength == 0
	    || prEventDumpMem->u4Address == 0xFFFFFFFF) {

		/* The request is finished or firmware response a error */
		/* Reply time tick to iwpriv */

		prAdapter->rIcapInfo.eIcapState = ICAP_STATE_FW_DUMP_DONE;

		kalSprintf(aucPath_done, "%s", "/file_dump_done.txt");
		if (kalCheckPath(aucPath_done) == -1) {
			kalMemSet(aucPath_done, 0x00, 256);
			kalSprintf(aucPath_done, "%s", "/data/file_dump_done.txt");
		}
		DBGLOG(INIT, INFO, ": ==> gen done_file\n");
		kalWriteToFile(aucPath_done, FALSE, aucPath_done,
			       sizeof(aucPath_done));
#if CFG_SUPPORT_QA_TOOL
		prAdapter->rIcapInfo.au4Offset[0][0] = 0;
		prAdapter->rIcapInfo.au4Offset[0][1] = 9;
		prAdapter->rIcapInfo.au4Offset[1][0] = 0;
		prAdapter->rIcapInfo.au4Offset[1][1] = 9;
#endif /* CFG_SUPPORT_QA_TOOL */

		prAdapter->rIcapInfo.u2DumpIndex++;

	}

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when command for memory dump has
 *        replied a event.
 *
 * @param prAdapter          Pointer to the Adapter structure.
 * @param prCmdInfo         Pointer to the command information
 * @param pucEventBuf       Pointer to event buffer
 *
 * @return none
 *
 */
/*----------------------------------------------------------------------------*/
void nicCmdEventQueryMemDump(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct GLUE_INFO *prGlueInfo;
	struct EVENT_DUMP_MEM *prEventDumpMem;
	static uint8_t aucPath[256];
	/*	static UINT_8 aucPath_done[300]; */
	static uint32_t u4CurTimeTick;
	int iRet = 0;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (1) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventDumpMem = (struct EVENT_DUMP_MEM *) (pucEventBuf);

		u4QueryInfoLen = sizeof(struct PARAM_CUSTOM_MEM_DUMP_STRUCT
					*);

		if (prEventDumpMem->ucFragNum == 1) {
			/* Store memory dump into sdcard,
			 * path /sdcard/dump_<current  system tick>_
			 * <memory address>_<memory length>.hex
			 */
			u4CurTimeTick = kalGetTimeTick();
#if defined(LINUX)

			/* PeiHsuan add for avoiding out of memory 20160801 */
			if (prAdapter->rIcapInfo.u2DumpIndex >= 20)
				prAdapter->rIcapInfo.u2DumpIndex = 0;

			/* if blbist mkdir undre /data/blbist,
			 * the dump files wouls put on it
			 */
			iRet += sprintf(aucPath, "/dump_%05hu.hex",
				prAdapter->rIcapInfo.u2DumpIndex);
			if (kalCheckPath(aucPath) == -1) {
				kalMemSet(aucPath, 0x00, 256);
				iRet += sprintf(aucPath, "/data/dump_%05hu.hex",
					prAdapter->rIcapInfo.u2DumpIndex);
			} else
				kalTrunkPath(aucPath);

			DBGLOG(INIT, INFO,
			       "iCap Create New Dump File dump_%05hu.hex,iRet[%d]\n",
			       prAdapter->rIcapInfo.u2DumpIndex, iRet);
#else
			kal_sprintf_ddk(aucPath, sizeof(aucPath),
				u4CurTimeTick,
				prEventDumpMem->u4Address,
				prEventDumpMem->u4Length +
				prEventDumpMem->u4RemainLength);
			/* strcpy(aucPath, "dump.hex"); */
#endif
			kalWriteToFile(aucPath, FALSE,
				&prEventDumpMem->aucBuffer[0],
				prEventDumpMem->u4Length);
		} else {
			/* Append current memory dump to the hex file */
			kalWriteToFile(aucPath, TRUE,
				&prEventDumpMem->aucBuffer[0],
				prEventDumpMem->u4Length);
		}
#if CFG_SUPPORT_QA_TOOL
		nicTsfRawData2IqFmt(prEventDumpMem, &prAdapter->rIcapInfo);
#endif /* CFG_SUPPORT_QA_TOOL */
		if (prEventDumpMem->u4RemainLength == 0
		    || prEventDumpMem->u4Address == 0xFFFFFFFF) {
			/* The request is finished or firmware response
			 * a error
			 */
			/* Reply time tick to iwpriv */
			if (prCmdInfo->fgIsOid) {

				/* the oid would be complete only in oid-trigger
				 * mode, that is no need to if the event-trigger
				 */
				if (prAdapter->rIcapInfo.eIcapState
						== ICAP_STATE_FW_DUMPING) {
					*((uint32_t *)
					  prCmdInfo->pvInformationBuffer)
						= u4CurTimeTick;
					kalOidComplete(prGlueInfo,
						prCmdInfo->fgSetQuery,
						u4QueryInfoLen,
						WLAN_STATUS_SUCCESS);
				}
			}

			prAdapter->rIcapInfo.eIcapState
				= ICAP_STATE_FW_DUMP_DONE;

#if defined(LINUX)

			prAdapter->rIcapInfo.u2DumpIndex++;

#else
			kal_sprintf_done_ddk(aucPath_done,
				sizeof(aucPath_done));
			kalWriteToFile(aucPath_done, FALSE, aucPath_done,
				       sizeof(aucPath_done));
#endif
		} else {
#if defined(LINUX)

#else /* 2013/05/26 fw would try to send the buffer successfully */
			/* The memory dump request is not finished,
			 * Send next command
			 */
			wlanSendMemDumpCmd(prAdapter,
				prCmdInfo->pvInformationBuffer,
				prCmdInfo->u4InformationBufferLength);
#endif
		}
	}

	return;

}

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
void nicCmdEventBatchScanResult(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

void nicEventHifCtrlv2(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
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

void nicEventHifCtrl(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
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
void nicCmdEventBuildDateCode(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

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
void nicCmdEventQueryStaStatistics(IN struct ADAPTER
				   *prAdapter, IN struct CMD_INFO *prCmdInfo,
				   IN uint8_t *pucEventBuf)
{
	uint32_t u4QueryInfoLen;
	struct EVENT_STA_STATISTICS *prEvent;
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_GET_STA_STATISTICS *prStaStatistics;
	enum ENUM_WMM_ACI eAci;
	struct STA_RECORD *prStaRec;
	uint8_t ucDbdcIdx, ucIdx;
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo;
#endif

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	ASSERT(prCmdInfo->pvInformationBuffer);

	prGlueInfo = prAdapter->prGlueInfo;
	prEvent = (struct EVENT_STA_STATISTICS *) pucEventBuf;
	prStaStatistics = (struct PARAM_GET_STA_STATISTICS *)
			  prCmdInfo->pvInformationBuffer;

	u4QueryInfoLen = sizeof(struct PARAM_GET_STA_STATISTICS);

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
#if (CFG_SUPPORT_CONNAC2X == 1)
		prStaStatistics->u4AggRangeCtrl_2 =
			prEvent->u4AggRangeCtrl_2;
		prStaStatistics->u4AggRangeCtrl_3 =
			prEvent->u4AggRangeCtrl_3;
#endif
		kalMemCopy(prStaStatistics->aucReserved5,
			   prEvent->aucReserved5,
			   sizeof(prEvent->aucReserved5));
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
			g_arMibInfo[ucDbdcIdx].u4RxMpduCnt +=
				prStaStatistics->rMibInfo[ucDbdcIdx].
				u4RxMpduCnt;
			g_arMibInfo[ucDbdcIdx].u4FcsError +=
				prStaStatistics->rMibInfo[ucDbdcIdx].
				u4FcsError;
			g_arMibInfo[ucDbdcIdx].u4RxFifoFull +=
				prStaStatistics->rMibInfo[ucDbdcIdx].
				u4RxFifoFull;
			g_arMibInfo[ucDbdcIdx].u4AmpduTxSfCnt +=
				prStaStatistics->rMibInfo[ucDbdcIdx].
				u4AmpduTxSfCnt;
			g_arMibInfo[ucDbdcIdx].u4AmpduTxAckSfCnt +=
				prStaStatistics->rMibInfo[ucDbdcIdx].
				u4AmpduTxAckSfCnt;

			for (ucIdx = 0; ucIdx <= AGG_RANGE_SEL_NUM; ucIdx++)
				g_arMibInfo[ucDbdcIdx].au2TxRangeAmpduCnt[ucIdx]
					+= prStaStatistics->rMibInfo[ucDbdcIdx].
					au2TxRangeAmpduCnt[ucIdx];
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
					       prEvent->ucStaRecIdx);

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

			log_dbg(P2P, INFO,
				"[%u][%u] link_score=%u, rssi=%u, rate=%u, threshold_cnt=%u, fail_cnt=%u\n",
				prEvent->ucNetworkTypeIndex,
				prEvent->ucStaRecIdx,
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
		prLinkQualityInfo = &(prAdapter->rLinkQualityInfo);
		prLinkQualityInfo->u4CurTxRate = prEvent->u2LinkSpeed * 5;
#endif
	}

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo,
			prCmdInfo->fgSetQuery,
			u4QueryInfoLen,
			WLAN_STATUS_SUCCESS);

}

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
void nicCmdEventQueryLteSafeChn(IN struct ADAPTER *prAdapter,
		IN struct CMD_INFO *prCmdInfo,
		IN uint8_t *pucEventBuf)
{
	struct EVENT_LTE_SAFE_CHN *prEvent;
	struct PARAM_GET_CHN_INFO *prLteSafeChnInfo;
	uint8_t ucIdx = 0;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo;

	if ((prAdapter == NULL) || (prCmdInfo == NULL) || (pucEventBuf == NULL)
			|| (prCmdInfo->pvInformationBuffer == NULL)) {
		ASSERT(FALSE);
		return;
	}

	prEvent = (struct EVENT_LTE_SAFE_CHN *) pucEventBuf;

	prLteSafeChnInfo = (struct PARAM_GET_CHN_INFO *)
			prCmdInfo->pvInformationBuffer;

	if (prLteSafeChnInfo->ucRoleIndex >= BSS_P2P_NUM) {
		ASSERT(FALSE);
		kalMemFree(prLteSafeChnInfo, VIR_MEM_TYPE,
				sizeof(struct PARAM_GET_CHN_INFO));
		return;
	}
	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter,
			prLteSafeChnInfo->ucRoleIndex);
	if (prP2pRoleFsmInfo == NULL) {
		DBGLOG(P2P, ERROR,
			"Corresponding P2P Role FSM empty: %d.\n",
			prLteSafeChnInfo->ucRoleIndex);
		kalMemFree(prLteSafeChnInfo, VIR_MEM_TYPE,
				sizeof(struct PARAM_GET_CHN_INFO));
		return;
	}

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
	p2pFunProcessAcsReport(prAdapter,
			prLteSafeChnInfo->ucRoleIndex,
			prLteSafeChnInfo,
			&(prP2pRoleFsmInfo->rAcsReqInfo));
	kalMemFree(prLteSafeChnInfo, VIR_MEM_TYPE,
			sizeof(struct PARAM_GET_CHN_INFO));
}
#if CFG_SUPPORT_ADVANCE_CONTROL
void nicCmdEventQueryAdvCtrl(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf)
{
	uint8_t *query;
	struct GLUE_INFO *prGlueInfo;
	uint32_t query_len;
	struct CMD_ADV_CONFIG_HEADER *hdr;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is null.\n");
		return;
	}
	if (!prCmdInfo) {
		DBGLOG(REQ, ERROR, "prCmdInfo is null.\n");
		return;
	}
	if (!pucEventBuf) {
		DBGLOG(REQ, ERROR, "pucEventBuf is null.\n");
		return;
	}
	hdr = (struct CMD_ADV_CONFIG_HEADER *)pucEventBuf;
	DBGLOG(REQ, LOUD, "%s type %x len %d>\n", __func__
				, hdr->u2Type, hdr->u2Len);
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		query_len = hdr->u2Len;
		query = prCmdInfo->pvInformationBuffer;
		if (query &&
			(query_len == prCmdInfo->u4InformationBufferLength))
			kalMemCopy(query, hdr, query_len);
		else
			DBGLOG(REQ, LOUD, "%s type %x, len %d != buflen %d>\n"
				, __func__, hdr->u2Type, hdr->u2Len,
				prCmdInfo->u4InformationBufferLength);
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
					query_len, WLAN_STATUS_SUCCESS);
	}
}
#endif
void nicEventRddPulseDump(IN struct ADAPTER *prAdapter,
			  IN uint8_t *pucEventBuf)
{
	uint16_t u2Idx, u2PulseCnt;
	struct EVENT_WIFI_RDD_TEST *prRddPulseEvent;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	prRddPulseEvent = (struct EVENT_WIFI_RDD_TEST *) (
				  pucEventBuf);

	u2PulseCnt = (prRddPulseEvent->u4FuncLength -
		      RDD_EVENT_HDR_SIZE) / RDD_ONEPLUSE_SIZE;

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
void nicCmdEventQueryWlanInfo(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}


void nicCmdEventQueryMibInfo(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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
		kalMemCopy(&prMibInfo->rHwMib3Cnt,
			   &prEventMibInfo->rHwMib3Cnt,
			   sizeof(struct HW_MIB3_COUNTER));
		kalMemCopy(&prMibInfo->rHwTxAmpduMts,
			   &prEventMibInfo->rHwTxAmpduMts,
			   sizeof(struct HW_TX_AMPDU_METRICS));
	}

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}
#endif

uint32_t nicEventQueryTxResourceEntry(IN struct ADAPTER *prAdapter,
				      IN uint8_t *pucEventBuf)
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

uint32_t nicCmdEventQueryNicEfuseAddr(IN struct ADAPTER *prAdapter,
				      IN uint8_t *pucEventBuf)
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

uint32_t nicCmdEventQueryNicCoexFeature(IN struct ADAPTER *prAdapter,
					IN uint8_t *pucEventBuf)
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
uint32_t nicCmdEventQueryNicCsumOffload(IN struct ADAPTER *prAdapter,
					IN uint8_t *pucEventBuf)
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

uint32_t nicCfgChipCapHwVersion(IN struct ADAPTER *prAdapter,
				IN uint8_t *pucEventBuf)
{
	struct CAP_HW_VERSION *prHwVer =
		(struct CAP_HW_VERSION *)pucEventBuf;

	prAdapter->rVerInfo.u2FwProductID = prHwVer->u2ProductID;

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipAdieHwVersion(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucEventBuf)
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


uint32_t nicCfgChipCapSwVersion(IN struct ADAPTER *prAdapter,
				IN uint8_t *pucEventBuf)
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

uint32_t nicCfgChipCapMacAddr(IN struct ADAPTER *prAdapter,
			      IN uint8_t *pucEventBuf)
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

uint32_t nicCfgChipCapPhyCap(IN struct ADAPTER *prAdapter,
			     IN uint8_t *pucEventBuf)
{
	struct CAP_PHY_CAP *prPhyCap = (struct CAP_PHY_CAP *)pucEventBuf;

	prAdapter->rWifiVar.ucStaVht &= prPhyCap->ucVht;
	prAdapter->rWifiVar.ucApVht &= prPhyCap->ucVht;
	prAdapter->rWifiVar.ucP2pGoVht &= prPhyCap->ucVht;
	prAdapter->rWifiVar.ucP2pGcVht &= prPhyCap->ucVht;
	prAdapter->fgIsHw5GBandDisabled = !prPhyCap->uc5gBand;
	prAdapter->rWifiVar.ucNSS = (prPhyCap->ucNss >
		prAdapter->rWifiVar.ucNSS) ?
		(prAdapter->rWifiVar.ucNSS):(prPhyCap->ucNss);
 #if CFG_SUPPORT_DBDC
	if (!prPhyCap->ucDbdc) {
		prAdapter->rWifiVar.eDbdcMode = ENUM_DBDC_MODE_DISABLED;
#if CFG_SUPPORT_CFG_FILE
		wlanCfgSetUint32(prAdapter, "DbdcMode",
					prAdapter->rWifiVar.eDbdcMode);
#endif
	}
#endif
	prAdapter->rWifiVar.ucTxLdpc &= prPhyCap->ucTxLdpc;
	prAdapter->rWifiVar.ucRxLdpc &= prPhyCap->ucRxLdpc;
	prAdapter->rWifiVar.ucTxStbc &= prPhyCap->ucTxStbc;
	prAdapter->rWifiVar.ucRxStbc &= prPhyCap->ucRxStbc;
#if CFG_SUPPORT_CFG_FILE
	wlanCfgSetUint32(prAdapter, "StaVHT", prAdapter->rWifiVar.ucStaVht);
	wlanCfgSetUint32(prAdapter, "ApVHT", prAdapter->rWifiVar.ucApVht);
	wlanCfgSetUint32(prAdapter, "P2pGoVHT", prAdapter->rWifiVar.ucP2pGoVht);
	wlanCfgSetUint32(prAdapter, "P2pGcVHT", prAdapter->rWifiVar.ucP2pGcVht);
	wlanCfgSetUint32(prAdapter, "Nss", prAdapter->rWifiVar.ucNSS);
	wlanCfgSetUint32(prAdapter, "LdpcTx", prAdapter->rWifiVar.ucTxLdpc);
	wlanCfgSetUint32(prAdapter, "LdpcRx", prAdapter->rWifiVar.ucRxLdpc);
	wlanCfgSetUint32(prAdapter, "StbcTx", prAdapter->rWifiVar.ucTxStbc);
	wlanCfgSetUint32(prAdapter, "StbcRx", prAdapter->rWifiVar.ucRxStbc);
#endif

	DBGLOG(INIT, TRACE,
		"Vht [%u], 5gBand [%d], Nss [%d], Dbdc [%d]\n",
			prPhyCap->ucVht,
			prPhyCap->uc5gBand,
			prPhyCap->ucNss,
			prPhyCap->ucDbdc);

	DBGLOG(INIT, TRACE,
		"TxLdpc [%u], RxLdpc [%u], StbcTx [%u], StbcRx [%u]\n",
			prPhyCap->ucTxLdpc,
			prPhyCap->ucRxLdpc,
			prPhyCap->ucTxStbc,
			prPhyCap->ucRxStbc);

	prAdapter->rWifiVar.ucHwNotSupportAC = (prPhyCap->ucVht) ? FALSE:TRUE;
	prAdapter->rWifiFemCfg.u2WifiPath = (uint16_t)(prPhyCap->ucHwWifiPath);
	prAdapter->rWifiVar.ucHwNotSupportAX = (prPhyCap->ucHe) ? FALSE:TRUE;

	prAdapter->rWifiVar.ucStaBandwidth =
		(prAdapter->rWifiVar.ucStaBandwidth >
		prPhyCap->ucMaxBandwidth) ? (prPhyCap->ucMaxBandwidth) :
		(prAdapter->rWifiVar.ucStaBandwidth);
	prAdapter->rWifiVar.ucApBandwidth =
		(prAdapter->rWifiVar.ucApBandwidth >
		prPhyCap->ucMaxBandwidth) ? (prPhyCap->ucMaxBandwidth) :
		(prAdapter->rWifiVar.ucApBandwidth);

	if (!prPhyCap->ucVht) {
#if CFG_SUPPORT_MTK_SYNERGY
		prAdapter->rWifiVar.ucGbandProbe256QAM = FEATURE_DISABLED;
#endif
#if CFG_SUPPORT_VHT_IE_IN_2G
		prAdapter->rWifiVar.ucVhtIeIn2g = FEATURE_DISABLED;
#endif
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapMacCap(IN struct ADAPTER *prAdapter,
			     IN uint8_t *pucEventBuf)
{
	struct CAP_MAC_CAP *prMacCap = (struct CAP_MAC_CAP *)pucEventBuf;

	if (prMacCap->ucHwBssIdNum > 0
	    && prMacCap->ucHwBssIdNum <= MAX_BSSID_NUM) {
		prAdapter->ucHwBssIdNum = prMacCap->ucHwBssIdNum;
		prAdapter->ucP2PDevBssIdx = prAdapter->ucHwBssIdNum;
		prAdapter->aprBssInfo[prAdapter->ucP2PDevBssIdx] =
			&prAdapter->rWifiVar.rP2pDevInfo;
	}
	DBGLOG(INIT, INFO, "ucHwBssIdNum: %d.\n",
	       prMacCap->ucHwBssIdNum);

	if (prMacCap->ucWtblEntryNum > 0
	    && prMacCap->ucWtblEntryNum <= WTBL_SIZE) {
		prAdapter->ucWtblEntryNum = prMacCap->ucWtblEntryNum;
		prAdapter->ucTxDefaultWlanIndex = prAdapter->ucWtblEntryNum
						  - 1;
	}
	DBGLOG(INIT, INFO, "ucWtblEntryNum: %d.\n",
	       prMacCap->ucWtblEntryNum);

	prAdapter->ucWmmSetNum = prMacCap->ucWmmSet > 0 ?
		prMacCap->ucWmmSet : 1;
	DBGLOG(INIT, INFO, "ucWmmSetNum: %d.\n",
	       prMacCap->ucWmmSet);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapFrameBufCap(IN struct ADAPTER *prAdapter,
				  IN uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapBeamformCap(IN struct ADAPTER *prAdapter,
				  IN uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapLocationCap(IN struct ADAPTER *prAdapter,
				  IN uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicCfgChipCapMuMimoCap(IN struct ADAPTER *prAdapter,
				IN uint8_t *pucEventBuf)
{
	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_ANT_SWAP
uint32_t nicCfgChipCapAntSwpCap(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucEventBuf)
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

#if (CFG_SUPPORT_WFDMA_REALLOC == 1)
uint32_t nicCmdEventQueryNicWfdmaRealloc(IN struct ADAPTER *prAdapter,
					 IN uint8_t *pucEventBuf)
{
	struct CAP_WFDMA_REALLOC_T *prWfdmaRealloc =
		(struct CAP_WFDMA_REALLOC_T *)pucEventBuf;

	DBGLOG(INIT, INFO, "version=%d IsSupport=%d ReallocCmdRsc=%d\n",
	       prWfdmaRealloc->ucVersion,
	       prWfdmaRealloc->ucIsSupportWfdmaRealloc,
	       prWfdmaRealloc->u4ReallocCmdTotalResource);

	prAdapter->fgIsSupportWfdmaRealloc =
		prWfdmaRealloc->ucIsSupportWfdmaRealloc;

	prAdapter->u4ReallocCmdTotalResource =
		prWfdmaRealloc->u4ReallocCmdTotalResource;

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_WFDMA_REALLOC */

#if (CFG_SUPPORT_WIFI_6G == 1)
uint32_t nicCfgChipCap6GCap(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucEventBuf)
{
	struct CAP_6G_CAP *pr6gCap = (struct CAP_6G_CAP *)pucEventBuf;

	ASSERT(prAdapter);

	prAdapter->fgIsHwSupport6G = pr6gCap->ucIsSupport6G;
	prAdapter->rWifiFemCfg.u2WifiPath6G =
		(uint16_t)(pr6gCap->ucHwWifiPath);

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

uint32_t nicEventQueryTxResource(IN struct ADAPTER
				 *prAdapter, IN uint8_t *pucEventBuf)
{
	uint32_t i, i_max;
	uint32_t version = *((uint32_t *)(pucEventBuf));

	i_max = sizeof(nicTxRsrcEvtHdlrTbl) / sizeof(
			struct nicTxRsrcEvtHdlr);
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

uint32_t nicEventQueryTxResource_v1(IN struct ADAPTER
				    *prAdapter, IN uint8_t *pucEventBuf)
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

uint32_t nicEventQueryTxResource_v2(IN struct ADAPTER
				    *prAdapter, IN uint8_t *pucEventBuf)
{
	/* Follow V1 as default */
	return nicEventQueryTxResource_v1(prAdapter, pucEventBuf);
}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
void nicCmdEventQueryCfgRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicParsingNicCapV2(IN struct ADAPTER *prAdapter,
	IN uint32_t u4Type, IN uint8_t *pucEventBuf)
{
	uint32_t table_idx;

	for (table_idx = 0;
	     table_idx < (sizeof(gNicCapabilityV2InfoTable) / sizeof(
				  struct NIC_CAPABILITY_V2_REF_TABLE));
	     table_idx++) {
		/* find the corresponding tag's handler */
		if (gNicCapabilityV2InfoTable[table_idx].tag_type == u4Type) {
			gNicCapabilityV2InfoTable[table_idx].hdlr(
				prAdapter, pucEventBuf);
			break;
		}
	}
}
#endif /* #ifdef CFG_SUPPORT_UNIFIED_COMMAND */

void nicCmdEventQueryNicCapabilityV2(IN struct ADAPTER *prAdapter,
				     IN uint8_t *pucEventBuf)
{
	struct EVENT_NIC_CAPABILITY_V2 *prEventNicV2 =
		(struct EVENT_NIC_CAPABILITY_V2 *)pucEventBuf;
	struct NIC_CAPABILITY_V2_ELEMENT *prElement;
	uint32_t tag_idx, table_idx, offset;

	offset = 0;

	/* process each element */
	for (tag_idx = 0; tag_idx < prEventNicV2->u2TotalElementNum;
	     tag_idx++) {

		prElement = (struct NIC_CAPABILITY_V2_ELEMENT *)(
				    prEventNicV2->aucBuffer + offset);

		for (table_idx = 0;
		     table_idx < (sizeof(gNicCapabilityV2InfoTable) / sizeof(
					  struct NIC_CAPABILITY_V2_REF_TABLE));
		     table_idx++) {

			/* find the corresponding tag's handler */
			if (gNicCapabilityV2InfoTable[table_idx].tag_type ==
			    prElement->tag_type) {
				gNicCapabilityV2InfoTable[table_idx].hdlr(
					prAdapter, prElement->aucbody);
				break;
			}
		}

		/* move to the next tag */
		offset += prElement->body_len + (uint16_t) OFFSET_OF(
				  struct NIC_CAPABILITY_V2_ELEMENT, aucbody);
	}

}

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
void nicCmdEventQueryTxPowerInfo(IN struct ADAPTER *prAdapter,
				 IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#endif

void nicEventLinkQuality(IN struct ADAPTER *prAdapter,
			 IN struct WIFI_EVENT *prEvent)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CMD_INFO *prCmdInfo;
	uint8_t ucBssIndex = AIS_DEFAULT_INDEX;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

#if CFG_ENABLE_WIFI_DIRECT && CFG_SUPPORT_P2P_RSSI_QUERY
	if (prEvent->u2PacketLen == prChipInfo->event_hdr_size +
	    sizeof(struct EVENT_LINK_QUALITY_EX)) {
		struct EVENT_LINK_QUALITY_EX *prLqEx =
			(struct EVENT_LINK_QUALITY_EX *) (prEvent->aucBuffer);

		if (prLqEx->ucIsLQ0Rdy)
			nicUpdateLinkQuality(prAdapter, 0,
				(struct LINK_QUALITY *) prLqEx);
		if (prLqEx->ucIsLQ1Rdy)
			nicUpdateLinkQuality(prAdapter, 1,
				(struct LINK_QUALITY *) prLqEx);
	} else {
		/* For old FW, P2P may invoke link quality query,
		 * and make driver flag becone TRUE.
		 */
		DBGLOG(P2P, WARN,
		       "Old FW version, not support P2P RSSI query.\n");

		/* Must not use NETWORK_TYPE_P2P_INDEX,
		 * cause the structure is mismatch.
		 */
		nicUpdateLinkQuality(prAdapter, 0,
			(struct LINK_QUALITY *) (prEvent->aucBuffer));
	}
#else
	/*only support ais query */
	{
		struct BSS_INFO *prBssInfo;

		for (ucBssIndex = 0; ucBssIndex < prAdapter->ucHwBssIdNum;
		     ucBssIndex++) {
			prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

			if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS
			    && prBssInfo->fgIsInUse)
				break;
		}

		if (ucBssIndex >= prAdapter->ucHwBssIdNum)
			ucBssIndex = 1;
			/* No hit(bss1 for default ais network) */
		nicUpdateLinkQuality(prAdapter, ucBssIndex,
			(struct EVENT_LINK_QUALITY *) (prEvent->aucBuffer));
	}

#endif

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

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
#ifndef LINUX
	if (prAdapter->rWlanInfo.eRssiTriggerType ==
	    ENUM_RSSI_TRIGGER_GREATER &&
	    prAdapter->rWlanInfo.rRssiTriggerValue >= (int32_t) (
		    prAdapter->rLinkQuality.cRssi)) {

		prAdapter->rWlanInfo.eRssiTriggerType =
			ENUM_RSSI_TRIGGER_TRIGGERED;

		kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
			WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
			(void *) &(prAdapter->rWlanInfo.rRssiTriggerValue),
			sizeof(int32_t),
			ucBssIndex);
	} else if (prAdapter->rWlanInfo.eRssiTriggerType ==
		   ENUM_RSSI_TRIGGER_LESS &&
		   prAdapter->rWlanInfo.rRssiTriggerValue <= (int32_t) (
			   prAdapter->rLinkQuality.cRssi)) {

		prAdapter->rWlanInfo.eRssiTriggerType =
			ENUM_RSSI_TRIGGER_TRIGGERED;

		kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
			WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
			(void *) &(prAdapter->rWlanInfo.rRssiTriggerValue),
			sizeof(int32_t),
			ucBssIndex);
	}
#endif
}

uint32_t nicExtTsfRawData2IqFmt(
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prEventDumpMem,
	struct ICAP_INFO_T *prIcap)
{
	static uint8_t
	aucPathWF0[40];	/* the path for iq data dump out */
	static uint8_t
	aucPathWF1[40];	/* the path for iq data dump out */
	static uint8_t
	aucPathRAWWF0[40];	/* the path for iq data dump out */
	static uint8_t
	aucPathRAWWF1[40];	/* the path for iq data dump out */
	uint8_t *pucDataWF0 = NULL;	/* the data write into file */
	uint8_t *pucDataWF1 = NULL;	/* the data write into file */
	uint8_t *pucDataRAWWF0 =
		NULL;	/* the data write into file */
	uint8_t *pucDataRAWWF1 =
		NULL;	/* the data write into file */
	uint32_t u4SrcOffset;	/* record the buffer offset */
	uint32_t u4FmtLen = 0;	/* bus format length */
	uint32_t u4CpyLen = 0;
	uint32_t u4RemainByte;
	uint32_t u4DataWBufSize = 150;
	uint32_t u4DataRAWWBufSize = 150;
	uint32_t u4DataWLenF0 = 0;
	uint32_t u4DataWLenF1 = 0;
	uint32_t u4DataRAWWLenF0 = 0;
	uint32_t u4DataRAWWLenF1 = 0;
	u_int8_t fgAppend;
	int32_t u4Iqc160WF0Q0, u4Iqc160WF1I1;
	/* for alignment. bcs we send 2KB data per packet,*/
	static uint8_t	ucDstOffset;
	/* the data will not align in 12 bytes case. */
	static uint32_t u4CurTimeTick;

	static union ICAP_BUS_FMT icapBusData;
	uint32_t *ptr;

	pucDataWF0 = kmalloc(u4DataWBufSize, GFP_KERNEL);
	pucDataWF1 = kmalloc(u4DataWBufSize, GFP_KERNEL);
	pucDataRAWWF0 = kmalloc(u4DataRAWWBufSize, GFP_KERNEL);
	pucDataRAWWF1 = kmalloc(u4DataRAWWBufSize, GFP_KERNEL);


	if ((!pucDataWF0) || (!pucDataWF1) || (!pucDataRAWWF0)
	    || (!pucDataRAWWF1)) {
		DBGLOG(INIT, ERROR, "kmalloc failed.\n");
		kfree(pucDataWF0);
		kfree(pucDataWF1);
		kfree(pucDataRAWWF0);
		kfree(pucDataRAWWF1);
		ASSERT(-1);
		return -1;
	}

	fgAppend = TRUE;
	if (prEventDumpMem->u4PktNum == 0) {
		u4CurTimeTick = kalGetTimeTick();
		/* Store memory dump into sdcard,
		 * path /sdcard/dump_<current system tick>_
		 * <memory address>_<memory length>.hex
		 */
#if defined(LINUX)

		/* if blbist mkdir undre /data/blbist,
		 * the dump files wouls put on it
		 */
		kalScnprintf(aucPathWF0, sizeof(aucPathWF0),
			  "/dump_out_%05hu_WF0.txt", prIcap->u2DumpIndex);
		kalScnprintf(aucPathWF1, sizeof(aucPathWF1),
			  "/dump_out_%05hu_WF1.txt", prIcap->u2DumpIndex);
		if (kalCheckPath(aucPathWF0) == -1) {
			kalMemSet(aucPathWF0, 0x00, sizeof(aucPathWF0));
			kalScnprintf(aucPathWF0, sizeof(aucPathWF0),
				  "/data/dump_out_%05hu_WF0.txt",
				  prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathWF0);

		if (kalCheckPath(aucPathWF1) == -1) {
			kalMemSet(aucPathWF1, 0x00, sizeof(aucPathWF1));
			kalScnprintf(aucPathWF1, sizeof(aucPathWF1),
				  "/data/dump_out_%05hu_WF1.txt",
				  prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathWF1);

		kalScnprintf(aucPathRAWWF0, sizeof(aucPathRAWWF0),
			  "/dump_RAW_%05hu_WF0.txt", prIcap->u2DumpIndex);
		kalScnprintf(aucPathRAWWF1, sizeof(aucPathRAWWF1),
			  "/dump_RAW_%05hu_WF1.txt", prIcap->u2DumpIndex);
		if (kalCheckPath(aucPathRAWWF0) == -1) {
			kalMemSet(aucPathRAWWF0, 0x00, sizeof(aucPathRAWWF0));
			kalScnprintf(aucPathRAWWF0, sizeof(aucPathRAWWF0),
				  "/data/dump_RAW_%05hu_WF0.txt",
				  prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathRAWWF0);

		if (kalCheckPath(aucPathRAWWF1) == -1) {
			kalMemSet(aucPathRAWWF1, 0x00, sizeof(aucPathRAWWF1));
			kalScnprintf(aucPathRAWWF1, sizeof(aucPathRAWWF1),
				  "/data/dump_RAW_%hu_WF1.txt",
				  prIcap->u2DumpIndex);
		} else
			kalTrunkPath(aucPathRAWWF1);

#else
		/* TODO: check Address */
		kal_sprintf_ddk(aucPathWF0, sizeof(aucPathWF0),
				u4CurTimeTick, prEventDumpMem->u4Address,
				prEventDumpMem->u4Length +
				prEventDumpMem->u4RemainLength);
		kal_sprintf_ddk(aucPathWF1, sizeof(aucPathWF1),
				u4CurTimeTick, prEventDumpMem->u4Address,
				prEventDumpMem->u4Length +
				prEventDumpMem->u4RemainLength);
#endif
		/* fgAppend = FALSE; */
	}

	ptr = (uint32_t *)(&prEventDumpMem->u4Data);

	for (u4SrcOffset = 0,
	     u4RemainByte = prEventDumpMem->u4DataLength;
	     u4RemainByte > 0;) {
		u4FmtLen = (prIcap->u4CapNode == ICAP_CONTENT_SPECTRUM) ?
			   sizeof(struct SPECTRUM_BUS_FMT) : sizeof(union
					   ICAP_BUS_FMT);
		/* 4 bytes : 12 bytes */
		u4CpyLen = (u4RemainByte >= u4FmtLen) ? u4FmtLen :
			   u4RemainByte;
		if ((ucDstOffset + u4CpyLen) > sizeof(icapBusData)) {
			DBGLOG(INIT, ERROR,
				"ucDstOffset(%u) + u4CpyLen(%u) exceed bound of icapBusData\n",
			  ucDstOffset, u4CpyLen);
			kfree(pucDataWF0);
			kfree(pucDataWF1);
			kfree(pucDataRAWWF0);
			kfree(pucDataRAWWF1);
			ASSERT(-1);
			return -1;
		}

		memcpy((uint8_t *)&icapBusData + ucDstOffset,
		       &prEventDumpMem->u4Data + u4SrcOffset, u4CpyLen);
		if (prIcap->u4CapNode == ICAP_CONTENT_FIIQ
		    || prIcap->u4CapNode == ICAP_CONTENT_FDIQ) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rIqcBusData.u4Iqc0I,
				icapBusData.rIqcBusData.u4Iqc0Q);
			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rIqcBusData.u4Iqc1I,
				icapBusData.rIqcBusData.u4Iqc1Q);
		} else if (prIcap->u4CapNode - 1000 == ICAP_CONTENT_FIIQ ||
			   prIcap->u4CapNode - 1000 == ICAP_CONTENT_FDIQ) {
			u4Iqc160WF0Q0 = icapBusData.rIqc160BusData.u4Iqc0Q0P1 |
				(icapBusData.rIqc160BusData.u4Iqc0Q0P2 << 8);
			u4Iqc160WF1I1 = icapBusData.rIqc160BusData.u4Iqc1I1P1 |
				(icapBusData.rIqc160BusData.u4Iqc1I1P2 << 4);

			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n",
				icapBusData.rIqc160BusData.u4Iqc0I0,
				u4Iqc160WF0Q0,
				icapBusData.rIqc160BusData.u4Iqc0I1,
				icapBusData.rIqc160BusData.u4Iqc0Q1);

			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n",
				icapBusData.rIqc160BusData.u4Iqc1I0,
				icapBusData.rIqc160BusData.u4Iqc1Q0,
				u4Iqc160WF1I1,
				icapBusData.rIqc160BusData.u4Iqc1Q1);

		} else if (prIcap->u4CapNode == ICAP_CONTENT_SPECTRUM) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n",
				icapBusData.rSpectrumBusData.u4DcocI,
				icapBusData.rSpectrumBusData.u4DcocQ);
		} else if (prIcap->u4CapNode == ICAP_CONTENT_ADC) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI0T0,
				icapBusData.rPackedAdcBusData.u4AdcQ0T0,
				icapBusData.rPackedAdcBusData.u4AdcI0T1,
				icapBusData.rPackedAdcBusData.u4AdcQ0T1,
				icapBusData.rPackedAdcBusData.u4AdcI0T2,
				icapBusData.rPackedAdcBusData.u4AdcQ0T2,
				icapBusData.rPackedAdcBusData.u4AdcI0T3,
				icapBusData.rPackedAdcBusData.u4AdcQ0T3,
				icapBusData.rPackedAdcBusData.u4AdcI0T4,
				icapBusData.rPackedAdcBusData.u4AdcQ0T4,
				icapBusData.rPackedAdcBusData.u4AdcI0T5,
				icapBusData.rPackedAdcBusData.u4AdcQ0T5);

			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI1T0,
				icapBusData.rPackedAdcBusData.u4AdcQ1T0,
				icapBusData.rPackedAdcBusData.u4AdcI1T1,
				icapBusData.rPackedAdcBusData.u4AdcQ1T1,
				icapBusData.rPackedAdcBusData.u4AdcI1T2,
				icapBusData.rPackedAdcBusData.u4AdcQ1T2,
				icapBusData.rPackedAdcBusData.u4AdcI1T3,
				icapBusData.rPackedAdcBusData.u4AdcQ1T3,
				icapBusData.rPackedAdcBusData.u4AdcI1T4,
				icapBusData.rPackedAdcBusData.u4AdcQ1T4,
				icapBusData.rPackedAdcBusData.u4AdcI1T5,
				icapBusData.rPackedAdcBusData.u4AdcQ1T5);
		} else if (prIcap->u4CapNode - 2000 == ICAP_CONTENT_ADC) {
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI0T0,
				icapBusData.rPackedAdcBusData.u4AdcQ0T0,
				icapBusData.rPackedAdcBusData.u4AdcI0T1,
				icapBusData.rPackedAdcBusData.u4AdcQ0T1,
				icapBusData.rPackedAdcBusData.u4AdcI0T2,
				icapBusData.rPackedAdcBusData.u4AdcQ0T2);

			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
				"%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI1T0,
				icapBusData.rPackedAdcBusData.u4AdcQ1T0,
				icapBusData.rPackedAdcBusData.u4AdcI1T1,
				icapBusData.rPackedAdcBusData.u4AdcQ1T1,
				icapBusData.rPackedAdcBusData.u4AdcI1T2,
				icapBusData.rPackedAdcBusData.u4AdcQ1T2);
		} else if (prIcap->u4CapNode == ICAP_CONTENT_TOAE) {
			/* actually, this is DCOC. we take TOAE as DCOC */
			u4DataWLenF0 = kalScnprintf(pucDataWF0, u4DataWBufSize,
					"%8d,%8d\n",
					icapBusData.rAdcBusData.u4Dcoc0I,
					icapBusData.rAdcBusData.u4Dcoc0Q);
			u4DataWLenF1 = kalScnprintf(pucDataWF1, u4DataWBufSize,
					"%8d,%8d\n",
					icapBusData.rAdcBusData.u4Dcoc1I,
					icapBusData.rAdcBusData.u4Dcoc1Q);
		}
		if (u4CpyLen ==
		    u4FmtLen) {	/* the data format is complete */
			kalWriteToFile(aucPathWF0, fgAppend, pucDataWF0,
				       u4DataWLenF0);
			kalWriteToFile(aucPathWF1, fgAppend, pucDataWF1,
				       u4DataWLenF1);
		}
		ptr = (uint32_t *)(&prEventDumpMem->u4Data + u4SrcOffset);
		u4DataRAWWLenF0 = kalScnprintf(pucDataRAWWF0, u4DataWBufSize,
					    "%08x%08x%08x\n",
					    *(ptr + 2), *(ptr + 1), *ptr);
		kalWriteToFile(aucPathRAWWF0, fgAppend, pucDataRAWWF0,
			       u4DataRAWWLenF0);
		kalWriteToFile(aucPathRAWWF1, fgAppend, pucDataRAWWF1,
			       u4DataRAWWLenF1);

		u4RemainByte -= u4CpyLen;
		u4SrcOffset += u4CpyLen;	/* shift offset */
		/* only use ucDstOffset at first packet for align 2KB */
		ucDstOffset = 0;
	}
	/* if this is a last packet, we can't transfer the remain data.
	 *  bcs we can't guarantee the data is complete align data format
	 */
	if (u4CpyLen !=
	    u4FmtLen) {	/* the data format is complete */
		/* not align 2KB, keep the data and next packet data
		 * will append it
		 */
		ucDstOffset =	u4CpyLen;
	}

	kfree(pucDataWF0);
	kfree(pucDataWF1);
	kfree(pucDataRAWWF0);
	kfree(pucDataRAWWF1);

	return 0;
}

void nicExtEventReCalData(IN struct ADAPTER *prAdapter, IN uint8_t *pucEventBuf)
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
				CAL_ARRAY_SIZE * sizeof(struct RECAL_DATA_T),
				VIR_MEM_TYPE);

		if (prCalArray == NULL) {
			DBGLOG(RFTEST, ERROR,
				"Unable to alloc memory for recal data\n");
			return;
		}
		prReCalInfo->prCalArray = prCalArray;
	}

	if (prReCalInfo->u4Count >= CAL_ARRAY_SIZE) {
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

#if (CFG_SUPPORT_PHY_ICS == 1)
void nicExtEventPhyIcsRawData(IN struct ADAPTER *prAdapter,
			   IN uint8_t *pucEventBuf)
{
	struct EXT_EVENT_PHY_ICS_DUMP_DATA_T *prPhyIcsEvent;
	struct ICS_BIN_LOG_HDR *prIcsBinLogHeader;
	void *pvPacket = NULL;
	uint32_t u4Size = 0, Idxi = 0;
	uint8_t *pucRecvBuff;

	ASSERT(prAdapter);

	if (pucEventBuf == NULL) {
		DBGLOG(RFTEST, ERROR, "pucEventBuf is null\n");
		return;
	}

	prPhyIcsEvent = (struct EXT_EVENT_PHY_ICS_DUMP_DATA_T *)
		      pucEventBuf;

	DBGLOG(RFTEST, INFO,
	       "u4PktNum = [%d], u4PhyTimestamp = [0x%08x], u4DataLen = [%d]\n",
	       prPhyIcsEvent->u4PktNum,
	       prPhyIcsEvent->u4PhyTimestamp,
	       prPhyIcsEvent->u4DataLen);

	/* 1KB phy ics packet + fw parser header */
	u4Size = prPhyIcsEvent->u4DataLen * sizeof(uint32_t) +
			sizeof(struct ICS_BIN_LOG_HDR);
	pvPacket = kalPacketAlloc(prAdapter->prGlueInfo, u4Size, &pucRecvBuff);

#if 0
	/* Print ICap data to console for debugging purpose */
	for (Idxi = 0; Idxi < 256; Idxi++)
		DBGLOG(RFTEST, ERROR, "Data[%d] : %08x\n", Idxi,
			prPhyIcsEvent->u4Data[Idxi]);
#endif

    /* endian swap */
	for (Idxi = 0; Idxi < 256; Idxi++) {
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

		/* prepare ICS frame */
		/* pucRecvBuff = ICS Header + AGG Header + AGG payload length */
		/* skip ICS header of pucRecvBuff, start to next address copy */
		kalMemCopy(pucRecvBuff + sizeof(struct ICS_BIN_LOG_HDR),
				prPhyIcsEvent->u4Data,
				prPhyIcsEvent->u4DataLen * sizeof(uint32_t));

		if (!prAdapter->fgIcsDumpOngoing) {
			if (kalOpenFwDumpFile(DUMP_FILE_ICS)
				!= WLAN_STATUS_SUCCESS)
				DBGLOG(NIC, ERROR,
					"open PHY ICS dump file fail\n");
			else
				prAdapter->fgIcsDumpFileOpend = TRUE;
			prAdapter->fgIcsDumpOngoing = TRUE;
		}

		if (prAdapter->fgIcsDumpOngoing) {
			if (prAdapter->fgIcsDumpFileOpend) {
				if (kalWriteFwDumpFile(
						pucRecvBuff,
						u4Size) != WLAN_STATUS_SUCCESS)
					DBGLOG(NIC, ERROR,
						"write PHY ICS log into file fail\n");
			}
		}

#if CFG_ASSERT_DUMP
		if (kalEnqFwDumpLog(
				prAdapter,
				pucRecvBuff,
				u4Size,
				&prAdapter->prGlueInfo->rFwDumpSkbQueue)
				!= WLAN_STATUS_SUCCESS) {
			DBGLOG(NIC, ERROR,
				"Enqueue PHY ICS log into queue fail\n");
		}
#endif
		kalPacketFree(prAdapter->prGlueInfo, pvPacket);
	}


}
#endif /* #if (CFG_SUPPORT_PHY_ICS == 1) */

void nicExtEventICapIQData(IN struct ADAPTER *prAdapter,
			   IN uint8_t *pucEventBuf)
{
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prICapEvent;
	uint32_t Idxi = 0, Idxj = 0, Idxk = 0;
	struct _RBIST_IQ_DATA_T *prIQArray = NULL;
	struct ICAP_INFO_T *prIcapInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	prICapEvent = (struct EXT_EVENT_RBIST_DUMP_DATA_T *)
		      pucEventBuf;
	prIcapInfo = &prAdapter->rIcapInfo;
	prIQArray = prIcapInfo->prIQArray;
	ASSERT(prIQArray);

	/* If we receive the packet which is delivered from
	 * last time data-capure, we need to drop it.
	 */
	DBGLOG(RFTEST, INFO, "prICapEvent->u4PktNum = %d\n",
	       prICapEvent->u4PktNum);
	if (prICapEvent->u4PktNum > prIcapInfo->u4ICapEventCnt) {
		if (prICapEvent->u4DataLength == 0)
			prIcapInfo->eIcapState = ICAP_STATE_FW_DUMP_DONE;

		DBGLOG(RFTEST, ERROR,
		       "Packet out of order: Pkt num %d, EventCnt %d\n",
		       prICapEvent->u4PktNum, prIcapInfo->u4ICapEventCnt);
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

	if (prIcapInfo->u4IQArrayIndex + prICapEvent->u4SmplCnt >=
	    MAX_ICAP_IQ_DATA_CNT) {
		DBGLOG(RFTEST, ERROR,
		       "Too many packets from FW, skip rest of them\n");
		return;
	}

	for (Idxi = 0; Idxi < prICapEvent->u4SmplCnt; Idxi++) {
		for (Idxj = 0; Idxj < prICapEvent->u4WFCnt; Idxj++) {
			prIQArray[prIcapInfo->u4IQArrayIndex].
				i4IQArray[Idxj][CAP_I_TYPE] =
				prICapEvent->u4Data[Idxk++];
			/*DBGLOG(RFTEST, WARN, "prIQArray[%d].i4IQArray[%d][CAP_I_TYPE]: %08x\n",
				prIcapInfo->u4IQArrayIndex,
				Idxj,
				prIQArray[prIcapInfo->u4IQArrayIndex].i4IQArray[Idxj][CAP_I_TYPE]);*/
			prIQArray[prIcapInfo->u4IQArrayIndex].
				i4IQArray[Idxj][CAP_Q_TYPE] =
				prICapEvent->u4Data[Idxk++];
			/*DBGLOG(RFTEST, WARN, "prIQArray[%d].i4IQArray[%d][CAP_Q_TYPE]: %08x\n",
				prIcapInfo->u4IQArrayIndex,
				Idxj,
				prIQArray[prIcapInfo->u4IQArrayIndex].i4IQArray[Idxj][CAP_Q_TYPE]);*/
		}
		prIcapInfo->u4IQArrayIndex++;
	}


	/* Print ICap data to console for debugging purpose */
	for (Idxi = 0; Idxi < prICapEvent->u4SmplCnt; Idxi++)
		if (prICapEvent->u4Data[Idxi] == 0)
			DBGLOG(RFTEST, WARN, "Data[%d] : %08x\n", Idxi,
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

void nicExtEventQueryMemDump(IN struct ADAPTER *prAdapter,
			     IN uint8_t *pucEventBuf)
{
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prEventDumpMem;
	static uint8_t aucPath[256];
	static uint8_t aucPath_done[300];
	static uint32_t u4CurTimeTick;
	int ret;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	kalSprintf(aucPath, "/dump_%05hu.hex",
		prAdapter->rIcapInfo.u2DumpIndex);

	prEventDumpMem = (struct EXT_EVENT_RBIST_DUMP_DATA_T *)
			 pucEventBuf;

	if (kalCheckPath(aucPath) == -1) {
		kalMemSet(aucPath, 0x00, 256);
		kalSprintf(aucPath, "/data/dump_%05hu.hex",
			prAdapter->rIcapInfo.u2DumpIndex);
	}

	if (prEventDumpMem->u4PktNum == 0) {
		/* Store memory dump into sdcard,
		 * path /sdcard/dump_<current  system tick>_<memory address>
		 * _<memory length>.hex
		 */
		u4CurTimeTick = kalGetTimeTick();
#if defined(LINUX)

		/* if blbist mkdir undre /data/blbist,
		 * the dump files wouls put on it
		 */
		kalSprintf(aucPath, "/dump_%05hu.hex",
			prAdapter->rIcapInfo.u2DumpIndex);
		if (kalCheckPath(aucPath) == -1) {
			kalMemSet(aucPath, 0x00, 256);
			kalSprintf(aucPath, "/data/dump_%05hu.hex",
				prAdapter->rIcapInfo.u2DumpIndex);
		}
#else
		/* TODO: check Address */
		kal_sprintf_ddk(aucPath, sizeof(aucPath),
			u4CurTimeTick, prEventDumpMem->u4Address,
			prEventDumpMem->u4Length +
			prEventDumpMem->u4RemainLength);
#endif
		kalWriteToFile(aucPath, FALSE,
			       (uint8_t *)prEventDumpMem->u4Data,
			       prEventDumpMem->u4DataLength);
	} else {
		/* Append current memory dump to the hex file */
		kalWriteToFile(aucPath, TRUE,
			       (uint8_t *)prEventDumpMem->u4Data,
			       prEventDumpMem->u4DataLength);
	}
#if CFG_SUPPORT_QA_TOOL
	nicExtTsfRawData2IqFmt(prEventDumpMem,
			       &prAdapter->rIcapInfo);
#endif /* CFG_SUPPORT_QA_TOOL */

	/* TODO: check Address */

	if (prEventDumpMem->u4DataLength == 0) {
		/* The request is finished or firmware response a error */
		/* Reply time tick to iwpriv */

		prAdapter->rIcapInfo.eIcapState = ICAP_STATE_FW_DUMP_DONE;

		ret = kalSnprintf(aucPath_done,
			sizeof(aucPath_done), "/file_dump_done.txt");
		if (ret == 0 || ret >= sizeof(aucPath_done))
			DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);
		if (kalCheckPath(aucPath_done) == -1) {
			kalMemSet(aucPath_done, 0x00, 256);
			ret = kalSnprintf(aucPath_done,
				sizeof(aucPath_done),
				"/data/file_dump_done.txt");
			if (ret == 0 || ret >= sizeof(aucPath_done))
				DBGLOG(INIT, ERROR,
					"[%u] kalSnprintf failed, ret: %d\n",
					__LINE__, ret);
		}
		DBGLOG(INIT, INFO, ": ==> gen done_file\n");
		kalWriteToFile(aucPath_done, FALSE, aucPath_done,
			       sizeof(aucPath_done));
#if CFG_SUPPORT_QA_TOOL
		prAdapter->rIcapInfo.au4Offset[0][0] = 0;
		prAdapter->rIcapInfo.au4Offset[0][1] = 9;
		prAdapter->rIcapInfo.au4Offset[1][0] = 0;
		prAdapter->rIcapInfo.au4Offset[1][1] = 9;
#endif /* CFG_SUPPORT_QA_TOOL */
		prAdapter->rIcapInfo.u2DumpIndex++;

	}
}


uint32_t nicRfTestEventHandler(IN struct ADAPTER *prAdapter,
			       IN struct WIFI_EVENT *prEvent)
{
	uint32_t u4QueryInfoLen = 0;
	struct EXT_EVENT_RF_TEST_RESULT_T *prResult;
	struct EXT_EVENT_RBIST_CAP_STATUS_T *prCapStatus;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct ATE_OPS_T *prAteOps = NULL;
	struct ICAP_INFO_T *prIcapInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);
	prAteOps = prChipInfo->prAteOps;
	ASSERT(prAteOps);
	prIcapInfo = &prAdapter->rIcapInfo;

	prResult = (struct EXT_EVENT_RF_TEST_RESULT_T *)
		   prEvent->aucBuffer;
	DBGLOG(RFTEST, INFO, "%s funcID = %d\n",
			__func__,
	       prResult->u4FuncIndex);
	switch (prResult->u4FuncIndex) {
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

void nicEventLayer0ExtMagic(IN struct ADAPTER *prAdapter,
			    IN struct WIFI_EVENT *prEvent)
{
	uint32_t u4QueryInfoLen = 0;
	struct CMD_INFO *prCmdInfo = NULL;

	log_dbg(NIC, TRACE, "prEvent->ucExtenEID = %x\n", prEvent->ucExtenEID);

	switch (prEvent->ucExtenEID) {
	case EXT_EVENT_ID_CMD_RESULT:
		u4QueryInfoLen = sizeof(struct
					PARAM_CUSTOM_EFUSE_BUFFER_MODE);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);
		break;

	case EXT_EVENT_ID_EFUSE_ACCESS:
	{
		struct EVENT_ACCESS_EFUSE *prEventEfuseAccess;

		u4QueryInfoLen = sizeof(struct PARAM_CUSTOM_ACCESS_EFUSE);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);
		prEventEfuseAccess = (struct EVENT_ACCESS_EFUSE *) (
					     prEvent->aucBuffer);

		/* Efuse block size 16 */
		kalMemCopy(prAdapter->aucEepromVaule,
			   prEventEfuseAccess->aucData, 16);
		break;
	}

	case EXT_EVENT_ID_RF_TEST:
		u4QueryInfoLen = nicRfTestEventHandler(prAdapter, prEvent);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);
		break;

	case EXT_EVENT_ID_GET_TX_POWER:
	{
		struct EXT_EVENT_GET_TX_POWER *prEventGetTXPower;

		u4QueryInfoLen = sizeof(struct PARAM_CUSTOM_GET_TX_POWER);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);
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
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);
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

		prAdapter->u4TotalBlockNum =
			prEventGetFreeBlock->ucTotalBlockNum;

		break;
	}

#if CFG_SUPPORT_TX_BF
	case EXT_EVENT_ID_BF_STATUS_READ:
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);
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
#if CFG_SUPPORT_WIFI_SYSDVT
	case EXT_EVENT_ID_SYSDVT_TEST:
	{
		u4QueryInfoLen = sizeof(struct SYSDVT_CTRL_EXT_T);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
			prEvent->ucSeqNum);
		break;
	}
#endif	/* CFG_SUPPORT_WIFI_SYSDVT */

#if (CFG_SUPPORT_802_11AX == 1)
	case EXT_EVENT_ID_SR_INFO:
	{
		struct _SR_EVENT_T *prEventSr;

		prEventSr = (struct _SR_EVENT_T *) (
						prEvent->aucBuffer);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);

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
				prCmdInfo->fgSetQuery,
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
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);

		if (prCmdInfo != NULL) {
			if (prCmdInfo->pfCmdDoneHandler)
				prCmdInfo->pfCmdDoneHandler(prAdapter,
					prCmdInfo, prEvent->aucBuffer);
			else if (prCmdInfo->fgIsOid)
				kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo->fgSetQuery,
				  u4QueryInfoLen, WLAN_STATUS_SUCCESS);

			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	}
#endif
	else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_MAC_INFO) {
		u4QueryInfoLen = sizeof(struct EXT_EVENT_MAC_INFO_T);
		prCmdInfo =
			nicGetPendingCmdInfo(prAdapter,
				prEvent->ucSeqNum);

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
					prCmdInfo->fgSetQuery,
					u4QueryInfoLen,
					WLAN_STATUS_SUCCESS);
			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	} else if (prEvent->ucExtenEID == EXT_EVENT_ID_DUMP_MEM) {
		u4QueryInfoLen = sizeof(struct EXT_CMD_EVENT_DUMP_MEM_T);
		prCmdInfo = nicGetPendingCmdInfo(
			prAdapter,
			prEvent->ucSeqNum);

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
					prCmdInfo->fgSetQuery,
					u4QueryInfoLen,
					WLAN_STATUS_SUCCESS);
			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	} else if (prEvent->ucExtenEID == EXT_EVENT_ID_SER) {
		u4QueryInfoLen = sizeof(struct PARAM_SER_INFO_T);

		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						 prEvent->ucSeqNum);

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
					prCmdInfo->fgSetQuery,
					u4QueryInfoLen,
					WLAN_STATUS_SUCCESS);

			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		}
	}
}

void nicEventMicErrorInfo(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent)
{
	uint8_t ucBssIndex = AIS_DEFAULT_INDEX;
	struct EVENT_MIC_ERR_INFO *prMicError;
	/* P_PARAM_AUTH_EVENT_T prAuthEvent; */
	struct STA_RECORD *prStaRec;
	struct PARAM_BSSID_EX *prCurrBssid =
		aisGetCurrBssId(prAdapter,
		ucBssIndex);

	DBGLOG(RSN, EVENT, "EVENT_ID_MIC_ERR_INFO\n");

	prMicError = (struct EVENT_MIC_ERR_INFO *) (
			     prEvent->aucBuffer);
	prStaRec = cnmGetStaRecByAddress(prAdapter,
			 ucBssIndex,
			 prCurrBssid->arMacAddress);
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
		   (void *) prAdapter->rWlanInfo.rCurrBssId.arMacAddress,
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

void nicEventScanDone(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent)
{
	scnEventScanDone(prAdapter,
			 (struct EVENT_SCAN_DONE *) (prEvent->aucBuffer), TRUE);
}

void nicEventSchedScanDone(IN struct ADAPTER *prAdapter,
		IN struct WIFI_EVENT *prEvent)
{
	DBGLOG(INIT, INFO, "EVENT_ID_SCHED_SCAN_DONE\n");
	scnEventSchedScanDone(prAdapter,
		(struct EVENT_SCHED_SCAN_DONE *) (prEvent->aucBuffer));
}

void nicEventSleepyNotify(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent)
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

void nicEventBtOverWifi(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent)
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

void nicEventStatistics(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();
	kalMemCopy(&prAdapter->rStatStruct, prEvent->aucBuffer,
		   sizeof(struct EVENT_STATISTICS));

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

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
void nicEventWlanInfo(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();
	kalMemCopy(&prAdapter->rEventWlanInfo, prEvent->aucBuffer,
		   sizeof(struct EVENT_WLAN_INFO));

	DBGLOG(RSN, INFO, "EVENT_ID_WTBL_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

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

void nicEventMibInfo(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;


	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();

	DBGLOG(RSN, INFO, "EVENT_ID_MIB_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

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
bool nicBeaconTimeoutFilterPolicy(IN struct ADAPTER *prAdapter,
	uint8_t ucReason, uint8_t ucBssIdx)
{
	struct RX_CTRL	*prRxCtrl;
	struct TX_CTRL	*prTxCtrl;
	OS_SYSTIME	u4CurrentTime;
	bool		bValid = true;
	uint32_t	u4MonitorWindow;

	ASSERT(prAdapter);
	u4MonitorWindow = CFG_BEACON_TIMEOUT_FILTER_DURATION_DEFAULT_VALUE;

	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	prTxCtrl = &prAdapter->rTxCtrl;
	ASSERT(prTxCtrl);

	GET_CURRENT_SYSTIME(&u4CurrentTime);

	DBGLOG(NIC, INFO,
			"u4MonitorWindow: %d, u4CurrentTime: %d, u4LastRxTime: %d, u4LastTxTime: %d",
			u4MonitorWindow, u4CurrentTime,
			prRxCtrl->u4LastRxTime[ucBssIdx],
			prTxCtrl->u4LastTxTime[ucBssIdx]);

	/* Policy 1, if RX in the past duration (in ms)
	 */
	if (ucReason == BEACON_TIMEOUT_REASON_HIGH_PER) {
		bValid = true;
	} else if (!CHECK_FOR_TIMEOUT(u4CurrentTime,
		prRxCtrl->u4LastRxTime[ucBssIdx],
		SEC_TO_SYSTIME(MSEC_TO_SEC(u4MonitorWindow))) &&
	    !scanBeaconTimeoutFilterPolicyForAis(prAdapter, ucBssIdx)) {
		DBGLOG(NIC, INFO, "Policy 1 hit, RX in the past duration");
		bValid = false;
	}

	DBGLOG(NIC, INFO, "valid beacon time out event?: %d", bValid);

	return bValid;
}

void nicEventBeaconTimeout(IN struct ADAPTER *prAdapter,
			   IN struct WIFI_EVENT *prEvent)
{
	DBGLOG(NIC, INFO,
		"EVENT_ID_BSS_BEACON_TIMEOUT Ignore:%d\n",
		prAdapter->fgDisBcnLostDetection);

	if (prAdapter->fgDisBcnLostDetection == FALSE) {
		struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
		struct EVENT_BSS_BEACON_TIMEOUT *prEventBssBeaconTimeout;
		uint16_t DeauthReasonCode;

		prEventBssBeaconTimeout = (struct EVENT_BSS_BEACON_TIMEOUT
					   *) (prEvent->aucBuffer);

		if (prEventBssBeaconTimeout->ucBssIndex >=
		    prAdapter->ucHwBssIdNum)
			return;

		DBGLOG(NIC, INFO, "Reason code: %d\n",
		       prEventBssBeaconTimeout->ucReasonCode);
		DBGLOG(NIC, INFO, "Deauth Reason code: %d\n",
		       prEventBssBeaconTimeout->u2RxDeauthReason);
		DeauthReasonCode =
			prEventBssBeaconTimeout->u2RxDeauthReason;

/* fos_change begin */
#if CFG_SUPPORT_EXCEPTION_STATISTICS
		prAdapter->total_beacon_timeout_count++;
		if (prEventBssBeaconTimeout->ucReasonCode >=
			BEACON_TIMEOUT_DUE_2_NUM) {
			DBGLOG(RX, WARN, "Invaild Beacon Timeout Reason: %d\n",
				prEventBssBeaconTimeout->ucReasonCode);
		} else {
			prAdapter->beacon_timeout_count
				[prEventBssBeaconTimeout->ucReasonCode]++;
		}
#endif /* fos_change end */

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
			prEventBssBeaconTimeout->ucBssIndex);

		if (IS_BSS_AIS(prBssInfo)) {
			if (nicBeaconTimeoutFilterPolicy(prAdapter,
				prEventBssBeaconTimeout->ucReasonCode,
				prBssInfo->ucBssIndex)) {

				prBssInfo->u2DeauthReason = DeauthReasonCode;
				aisBssBeaconTimeout_impl(prAdapter,
					prEventBssBeaconTimeout->ucReasonCode,
					prBssInfo->ucBssIndex);
			}
		}
#if CFG_ENABLE_WIFI_DIRECT
		else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
			if (nicBeaconTimeoutFilterPolicy(prAdapter,
					prEventBssBeaconTimeout->ucReasonCode,
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

void nicEventUpdateNoaParams(IN struct ADAPTER *prAdapter,
			     IN struct WIFI_EVENT *prEvent)
{
#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->fgIsP2PRegistered) {
		struct EVENT_UPDATE_NOA_PARAMS *prEventUpdateNoaParam;

		prEventUpdateNoaParam = (struct EVENT_UPDATE_NOA_PARAMS *) (
						prEvent->aucBuffer);

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
#else
	ASSERT(0);
#endif
}

void nicEventStaAgingTimeout(IN struct ADAPTER *prAdapter,
			     IN struct WIFI_EVENT *prEvent)
{
	if (prAdapter->fgDisStaAgingTimeoutDetection == FALSE) {
		struct EVENT_STA_AGING_TIMEOUT *prEventStaAgingTimeout;
		struct STA_RECORD *prStaRec;
		struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

		prEventStaAgingTimeout = (struct EVENT_STA_AGING_TIMEOUT *)
					 (prEvent->aucBuffer);
		prStaRec = cnmGetStaRecByIndex(prAdapter,
			prEventStaAgingTimeout->ucStaRecIdx);
		if (prStaRec == NULL)
			return;

		DBGLOG(NIC, INFO, "EVENT_ID_STA_AGING_TIMEOUT: STA[%u] "
		       MACSTR "\n",
		       prEventStaAgingTimeout->ucStaRecIdx,
		       MAC2STR(prStaRec->aucMacAddr));

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  prStaRec->ucBssIndex);

		bssRemoveClient(prAdapter, prBssInfo, prStaRec);

		if (prAdapter->fgIsP2PRegistered) {
			p2pFuncDisconnect(prAdapter, prBssInfo, prStaRec, FALSE,
					  REASON_CODE_DISASSOC_INACTIVITY);
		}

	}
	/* gDisStaAgingTimeoutDetection */
}

void nicEventApObssStatus(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent)
{
#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->fgIsP2PRegistered)
		rlmHandleObssStatusEventPkt(prAdapter,
			(struct EVENT_AP_OBSS_STATUS *) prEvent->aucBuffer);
#endif
}

void nicEventRoamingStatus(IN struct ADAPTER *prAdapter,
			   IN struct WIFI_EVENT *prEvent)
{
#if CFG_SUPPORT_ROAMING
	struct CMD_ROAMING_TRANSIT *prTransit;

	prTransit = (struct CMD_ROAMING_TRANSIT *) (
			    prEvent->aucBuffer);

	/* Default path */
	if (!IS_BSS_INDEX_AIS(prAdapter, prTransit->ucBssidx)) {
		DBGLOG(NIC, LOUD,
			"Use default, invalid index = %d\n",
			prTransit->ucBssidx);
		prTransit->ucBssidx = AIS_DEFAULT_INDEX;
	}

	roamingFsmProcessEvent(prAdapter, prTransit);
#endif /* CFG_SUPPORT_ROAMING */
}

void nicEventSendDeauth(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent)
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

void nicEventUpdateRddStatus(IN struct ADAPTER *prAdapter,
			     IN struct WIFI_EVENT *prEvent)
{
#if CFG_SUPPORT_RDD_TEST_MODE
	struct EVENT_RDD_STATUS *prEventRddStatus;

	prEventRddStatus = (struct EVENT_RDD_STATUS *) (
				   prEvent->aucBuffer);

	prAdapter->ucRddStatus = prEventRddStatus->ucRddStatus;
#endif
}

void nicEventUpdateBwcsStatus(IN struct ADAPTER *prAdapter,
			      IN struct WIFI_EVENT *prEvent)
{
	struct PTA_IPC *prEventBwcsStatus;

	prEventBwcsStatus = (struct PTA_IPC *) (prEvent->aucBuffer);

#if CFG_SUPPORT_BCM_BWCS_DEBUG
	DBGLOG(RSN, EVENT, "BCM BWCS Event: %02x%02x%02x%02x\n",
	       prEventBwcsStatus->u.aucBTPParams[0],
	       prEventBwcsStatus->u.aucBTPParams[1],
	       prEventBwcsStatus->u.aucBTPParams[2],
	       prEventBwcsStatus->u.aucBTPParams[3]);
#endif

	kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
		WLAN_STATUS_BWCS_UPDATE,
		(void *) prEventBwcsStatus, sizeof(struct PTA_IPC),
		AIS_DEFAULT_INDEX);
}

void nicEventUpdateBcmDebug(IN struct ADAPTER *prAdapter,
			    IN struct WIFI_EVENT *prEvent)
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

void nicEventAddPkeyDone(IN struct ADAPTER *prAdapter,
			 IN struct WIFI_EVENT *prEvent)
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
}

void nicEventIcapDone(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent)
{
	struct EVENT_ICAP_STATUS *prEventIcapStatus;
	struct PARAM_CUSTOM_MEM_DUMP_STRUCT rMemDumpInfo;
	uint32_t u4QueryInfo;

	prEventIcapStatus = (struct EVENT_ICAP_STATUS *) (
				    prEvent->aucBuffer);

	rMemDumpInfo.u4Address = prEventIcapStatus->u4StartAddress;
	rMemDumpInfo.u4Length = prEventIcapStatus->u4IcapSieze;
#if CFG_SUPPORT_QA_TOOL
	rMemDumpInfo.u4IcapContent =
		prEventIcapStatus->u4IcapContent;
#endif

	wlanoidQueryMemDump(prAdapter, &rMemDumpInfo,
			    sizeof(rMemDumpInfo), &u4QueryInfo);
}

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
struct PARAM_CAL_BACKUP_STRUCT_V2	g_rCalBackupDataV2;

void nicEventCalAllDone(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent)
{
	struct CMD_CAL_BACKUP_STRUCT_V2 *prEventCalBackupDataV2;
	uint32_t u4QueryInfo;

	DBGLOG(RFTEST, INFO, "%s\n", __func__);

	memset(&g_rCalBackupDataV2, 0,
	       sizeof(struct PARAM_CAL_BACKUP_STRUCT_V2));

	prEventCalBackupDataV2 = (struct CMD_CAL_BACKUP_STRUCT_V2 *)
				 (prEvent->aucBuffer);

	if (prEventCalBackupDataV2->ucReason == 1
	    && prEventCalBackupDataV2->ucAction == 2) {
		DBGLOG(RFTEST, INFO,
		       "Received an EVENT for Trigger Do All Cal Function.\n");

		g_rCalBackupDataV2.ucReason = 2;
		g_rCalBackupDataV2.ucAction = 4;
		g_rCalBackupDataV2.ucNeedResp = 1;
		g_rCalBackupDataV2.ucFragNum = 0;
		g_rCalBackupDataV2.ucRomRam = 0;
		g_rCalBackupDataV2.u4ThermalValue = 0;
		g_rCalBackupDataV2.u4Address = 0;
		g_rCalBackupDataV2.u4Length = 0;
		g_rCalBackupDataV2.u4RemainLength = 0;

		DBGLOG(RFTEST, INFO,
		       "RLM CMD : Get Cal Data from FW (%s). Start!!!!!!!!!!!!!!!!\n",
		       g_rCalBackupDataV2.ucRomRam == 0 ? "ROM" : "RAM");
		DBGLOG(RFTEST, INFO, "Thermal Temp = %d\n",
		       g_rBackupCalDataAllV2.u4ThermalInfo);
		wlanoidQueryCalBackupV2(prAdapter,
				&g_rCalBackupDataV2,
				sizeof(struct PARAM_CAL_BACKUP_STRUCT_V2),
				&u4QueryInfo);
	}

}
#endif

void nicEventDebugMsg(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent)
{
	struct EVENT_DEBUG_MSG *prEventDebugMsg;
	uint8_t ucMsgType;
	uint16_t u2MsgSize;
	uint8_t *pucMsg;

	prEventDebugMsg = (struct EVENT_DEBUG_MSG *)(
				  prEvent->aucBuffer);
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
			prCalData->u4FuncIndex = RE_CALIBRATION;
			prCalData->u4Type = 0;
			/* format: [XXXXXXXX][YYYYYYYY]ZZZZZZZZ */
			kalMemCopy(prCalData->u.ucData, pucMsg + 7, 28);
			nicRfTestEventHandler(prAdapter, prTmpEvent);
			kalMemFree(prTmpEvent, VIR_MEM_TYPE, u4Size);
		}
	}
#endif

	wlanPrintFwLog(pucMsg, u2MsgSize, ucMsgType, NULL);
}

#if CFG_SUPPORT_TDLS
void nicEventTdls(IN struct ADAPTER *prAdapter,
		  IN struct WIFI_EVENT *prEvent)
{
	TdlsexEventHandle(prAdapter->prGlueInfo,
			  (uint8_t *)prEvent->aucBuffer,
			  (uint32_t)(prEvent->u2PacketLength - 8));
}
#endif

void nicEventRssiMonitor(IN struct ADAPTER *prAdapter,
	IN struct WIFI_EVENT *prEvent)
{
	int32_t rssi = 0;
	struct GLUE_INFO *prGlueInfo;
	struct wiphy *wiphy;
	struct net_device *prNetDev;

	prGlueInfo = prAdapter->prGlueInfo;
	wiphy = priv_to_wiphy(prGlueInfo);

	kalMemCopy(&rssi, prEvent->aucBuffer, sizeof(int32_t));
	DBGLOG(RX, TRACE, "EVENT_ID_RSSI_MONITOR value=%d\n", rssi);
#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
	prNetDev = wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX);
	if (prNetDev)
		mtk_cfg80211_vendor_event_rssi_beyond_range(wiphy,
			prNetDev->ieee80211_ptr, rssi);
#endif
}

void nicEventDumpMem(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	DBGLOG(SW4, INFO, "%s: EVENT_ID_DUMP_MEM\n", __func__);

	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		DBGLOG(NIC, INFO, ": ==> 1\n");
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				prCmdInfo->fgSetQuery, 0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	} else {
		/* Burst mode */
		DBGLOG(NIC, INFO, ": ==> 2\n");
#if 0
		nicEventQueryMemDump(prAdapter, prEvent->aucBuffer);
#endif
	}
}

#if CFG_SUPPORT_ONE_TIME_CAL
void nicEventGetOneTimeCalData(
	IN struct ADAPTER *prAdapter,
	IN struct WIFI_EVENT *prEvent
)
{
	struct ONE_TIME_CAL_DATA_BLK *prEvTBlk;
	struct ONE_TIME_CAL_BLK_INFO *prEvTBlkInfo;
	uint8_t ucBlkSeqNumCur = 0, ucBlkSeqNum = 0;
	uint32_t u4CalParam = 0;
	enum ONE_TIME_CAL_TYPE eCalType;

	DBGLOG(NIC, STATE, "EVENT_ID_ONE_TIME_CAL\n");

	prEvTBlk = (struct ONE_TIME_CAL_DATA_BLK *)(&prEvent->aucBuffer[0]);

	if (!prEvTBlk)
		return;

	prEvTBlkInfo = &prEvTBlk->rOneTimeCalBlkInfo;

	eCalType = prEvTBlkInfo->ucCalType;
	ucBlkSeqNumCur = prEvTBlkInfo->ucBlkSeqNum & BITS(0, 3);
	ucBlkSeqNum = (prEvTBlkInfo->ucBlkSeqNum & BITS(4, 7)) >> 4;
	u4CalParam = prEvTBlkInfo->u4CalParam;

	DBGLOG(NIC, INFO, "CalType=0x%X, BlkNum=0x%X, BlkNumCur=0x%X\n",
		eCalType, prEvTBlkInfo->ucBlkSeqNum, ucBlkSeqNumCur);


	/* Stop if receiving null cal data */
	if (ucBlkSeqNum == 0) {

		DBGLOG(NIC, INFO, "Get null blk to stop\n");

		/* Write the file if ucBlkSeqNumCur == 1 */
		if (ucBlkSeqNumCur == 1)
			rlmOneTimeCalWriteToFile(prAdapter);

		rlmOneTimeCalStop(prAdapter, 0);

		return;
	}

	if (rlmOneTimeCalUpdateStruct(prAdapter, prEvTBlk, TRUE)
			!= WLAN_STATUS_SUCCESS)
		return;

}
#endif

void nicEventAssertDump(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	char aucVersionBuf[128];
	char aucLineEnd[] = {'\n', '\0'};
	char *pucLineEndHead;
	uint16_t u2VerBufLen = 0;
	uint16_t u2VerStrLen = 0;
	uint16_t u2BufSize = 0;

	ASSERT(prAdapter);
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

			if (kalOpenFwDumpFile(DUMP_FILE_N9)
				!= WLAN_STATUS_SUCCESS)
				DBGLOG(NIC, ERROR, "kalOpenFwDumpFile fail\n");
			else
				prAdapter->fgN9CorDumpFileOpend = TRUE;

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

			if (kalEnqFwDumpLog(prAdapter,
					aucVersionBuf, kalStrLen(aucVersionBuf),
					&prAdapter->prGlueInfo
					->rFwDumpSkbQueue)
					!= WLAN_STATUS_SUCCESS) {
				DBGLOG(NIC, ERROR,
						"Add FW version in core dump header fail\n");
			}

			if (kalWriteFwDumpFile(
					aucVersionBuf, kalStrLen(aucVersionBuf))
					!= WLAN_STATUS_SUCCESS) {
				DBGLOG(NIC, ERROR,
					"Add FW version in core dump header fail\n");
			}

			wlanCorDumpTimerInit(prAdapter, TRUE);
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

			if (kalEnqFwDumpLog(prAdapter,
					prEvent->aucBuffer, u2BufSize,
					&prAdapter->prGlueInfo
					->rFwDumpSkbQueue)
					!= WLAN_STATUS_SUCCESS) {
				DBGLOG(NIC, ERROR,
						"kalEnqFwDumpLog fail\n");
			}

			if (prAdapter->fgN9CorDumpFileOpend) {
				if (kalWriteFwDumpFile(
						prEvent->aucBuffer,
						u2BufSize)
						!= WLAN_STATUS_SUCCESS) {
					DBGLOG(NIC, INFO,
						"kalWriteN9CorDumpFile fail\n");
				}
			}

			if (kalStrStr(prEvent->aucBuffer,
					";coredump end")) {
				DBGLOG(NIC, ERROR,
					"core dump end, trigger whole chip reset\n");
				prAdapter->fgN9AssertDumpOngoing = FALSE;
				cnmTimerStopTimer(prAdapter,
						  &prAdapter->rN9CorDumpTimer);
				GL_DEFAULT_RESET_TRIGGER(prAdapter,
					RST_FW_ASSERT_DONE);
			}

			wlanCorDumpTimerReset(prAdapter, TRUE);
		}
	} else {
		/* prEvent->ucS2DIndex == S2D_INDEX_EVENT_C2H */
		if (!prAdapter->fgCr4AssertDumpOngoing) {
			DBGLOG(NIC, ERROR,
					"%s: EVENT_ID_ASSERT_DUMP\n", __func__);
			DBGLOG(NIC, ERROR,
							"\n[DUMP_Cr4]====CR4 ASSERT_DUMPSTART====\n");
			prAdapter->fgKeepPrintCoreDump = TRUE;
			if (kalOpenFwDumpFile(DUMP_FILE_CR4)
				!= WLAN_STATUS_SUCCESS)
				DBGLOG(NIC, ERROR, "kalOpenFwDumpFile fail\n");
			else
				prAdapter->fgCr4CorDumpFileOpend = TRUE;

			prAdapter->fgCr4AssertDumpOngoing = TRUE;
			wlanCorDumpTimerInit(prAdapter, FALSE);
		}
		if (prAdapter->fgCr4AssertDumpOngoing) {
			if (prAdapter->fgKeepPrintCoreDump)
				DBGLOG(NIC, ERROR, "[DUMP_CR4]%s:\n",
					prEvent->aucBuffer);
			if (!kalStrnCmp(prEvent->aucBuffer,
					";more log added here", 5))
				prAdapter->fgKeepPrintCoreDump = FALSE;

			if (prAdapter->fgCr4CorDumpFileOpend) {
				if (kalWriteFwDumpFile(
					    prEvent->aucBuffer,
					    prEvent->u2PacketLength -
					    prChipInfo->event_hdr_size)
					    != WLAN_STATUS_SUCCESS) {
					DBGLOG(NIC, ERROR,
						"kalWriteN9CorDumpFile fail\n");
				}
			}
			wlanCorDumpTimerReset(prAdapter, FALSE);
		}
	}
}

void nicEventRddSendPulse(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent)
{
	DBGLOG(RLM, INFO, "%s: EVENT_ID_RDD_SEND_PULSE\n",
	       __func__);

	nicEventRddPulseDump(prAdapter, prEvent->aucBuffer);
}

void nicEventUpdateCoexPhyrate(IN struct ADAPTER *prAdapter,
			       IN struct WIFI_EVENT *prEvent)
{
	uint8_t i;
	struct EVENT_UPDATE_COEX_PHYRATE *prEventUpdateCoexPhyrate;

	ASSERT(prAdapter);

	DBGLOG(NIC, LOUD, "%s\n", __func__);

	prEventUpdateCoexPhyrate = (struct EVENT_UPDATE_COEX_PHYRATE
				    *)(prEvent->aucBuffer);

	for (i = 0; i < (prAdapter->ucHwBssIdNum + 1); i++) {
		prAdapter->aprBssInfo[i]->u4CoexPhyRateLimit =
			prEventUpdateCoexPhyrate->au4PhyRateLimit[i];
		DBGLOG_LIMITED(NIC, TRACE, "Coex:BSS[%d]R:%d\n", i,
		       prAdapter->aprBssInfo[i]->u4CoexPhyRateLimit);
	}

	prAdapter->ucSmarGearSupportSisoOnly =
		prEventUpdateCoexPhyrate->ucSupportSisoOnly;
	prAdapter->ucSmartGearWfPathSupport =
		prEventUpdateCoexPhyrate->ucWfPathSupport;

	DBGLOG_LIMITED(NIC, INFO, "Smart Gear SISO:%d, WF:%d\n",
	       prAdapter->ucSmarGearSupportSisoOnly,
	       prAdapter->ucSmartGearWfPathSupport);
}

void nicCmdEventQueryCnmInfo(IN struct ADAPTER *prAdapter,
		IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicEventCoexCtrl(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
					prCmdInfo->fgSetQuery,
					0,
					WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

void nicEventCnmInfo(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

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

#if CFG_SUPPORT_REPLAY_DETECTION
void nicCmdEventSetAddKey(IN struct ADAPTER *prAdapter,
		IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	struct WIFI_CMD *prWifiCmd = NULL;
	struct CMD_802_11_KEY *prCmdKey = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery,
			       prCmdInfo->u4InformationBufferLength,
			       WLAN_STATUS_SUCCESS);
	}

	prGlueInfo = prAdapter->prGlueInfo;

	if (pucEventBuf) {
		prWifiCmd = (struct WIFI_CMD *) (pucEventBuf);
		prCmdKey = (struct CMD_802_11_KEY *) (prWifiCmd->aucBuffer);

		if (!IS_BSS_INDEX_AIS(prAdapter,
			prCmdKey->ucBssIdx))
			return;

		/* AIS only */
		if (!prCmdKey->ucKeyType &&
			prCmdKey->ucKeyId < 4) {
			/* Only save data broadcast key info.
			*  ucKeyType == 1 means unicast key
			*  ucKeyId == 4 or ucKeyId == 5 means it is a PMF key
			*/
			prDetRplyInfo = aisGetDetRplyInfo(prAdapter,
				prCmdKey->ucBssIdx);

			prDetRplyInfo->ucCurKeyId = prCmdKey->ucKeyId;
			prDetRplyInfo->ucKeyType = prCmdKey->ucKeyType;
			prDetRplyInfo->arReplayPNInfo[
				prCmdKey->ucKeyId].fgRekey = TRUE;
			prDetRplyInfo->arReplayPNInfo[
				prCmdKey->ucKeyId].fgFirstPkt = TRUE;
			DBGLOG(NIC, TRACE,
				"[%d] Keyid is %d, ucKeyType is %d\n",
				prCmdKey->ucBssIdx,
				prCmdKey->ucKeyId, prCmdKey->ucKeyType);
		}
	}
}
void nicOidCmdTimeoutSetAddKey(IN struct ADAPTER *prAdapter,
			       IN struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	DBGLOG(NIC, WARN, "Wlan setaddkey timeout.\n");
	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
			       0, WLAN_STATUS_FAILURE);
}
#endif
#if (CFG_WOW_SUPPORT == 1)
void nicEventWowWakeUpReason(IN struct ADAPTER *prAdapter,
		IN struct WIFI_EVENT *prEvent)
{
	struct EVENT_WOW_WAKEUP_REASON_INFO *prWakeUpReason;
	struct GLUE_INFO *prGlueInfo;
#if CFG_SUPPORT_MAGIC_PKT_VENDOR_EVENT
	struct wiphy *wiphy;
#endif

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

#if CFG_SUPPORT_MAGIC_PKT_VENDOR_EVENT
	/* for yocto branch, wifi needs to upload wake up event to
	 * upper layer to hold wake lock to prevent system going to
	 * suspend again, the reason is not only magic packet,
	 * but also UDP/TCP and beacon timeout event and so on,
	 * those reasons also need to be upload to upper layer
	 */
	if (prWakeUpReason->reason != ENUM_PF_CMD_TYPE_UNDEFINED) {
		wiphy = priv_to_wiphy(prGlueInfo);
		DBGLOG(RX, INFO,
			"upload EVENT ID[0x%02X] to upper layer!!\n",
			prEvent->ucEID);
		mtk_cfg80211_vendor_event_wowlan_magic_pkt(wiphy,
				prGlueInfo->prDevHandler->ieee80211_ptr, 0);
	}
#endif
}
#endif

#if CFG_SUPPORT_CSI
void
nicEventCSIData(IN struct ADAPTER *prAdapter, IN struct WIFI_EVENT *prEvent)
{
	struct CSI_TLV_ELEMENT *prCSITlvData;
	int32_t i4EventLen;
	int16_t i2Idx = 0;
	int8_t *prBuf = NULL;
	uint16_t *pru2Tmp = NULL;
	uint32_t *p32tmp = NULL;
	struct CSI_DATA_T *prCSIData = NULL;
	struct CSI_INFO_T *prCSIInfo = &(prAdapter->rCSIInfo);
	/* u2Offset is 8 bytes currently, tag 4 bytes + length 4 bytes */
	uint16_t u2Offset = OFFSET_OF(struct CSI_TLV_ELEMENT, aucbody);
	uint32_t u4Tmp = 0;
	uint8_t ucLastTagFlg = false;

#if CFG_SUPPORT_CSI_NF
	uint8_t ucRes = 0;
#endif

#define CSI_EVENT_MAX_SIZE 1500

	ASSERT(prAdapter);

	DBGLOG(NIC, INFO, "[CSI] nicEventCSIData\n");

	i4EventLen = prEvent->u2PacketLength -
			prAdapter->chip_info->event_hdr_size;
	if (i4EventLen > CSI_EVENT_MAX_SIZE) {
		DBGLOG(NIC, WARN, "[CSI] Invalid CSI event size %u\n",
			i4EventLen);
		return;
	}
	prCSIData = kalMemAlloc(sizeof(struct CSI_DATA_T), VIR_MEM_TYPE);

	if (!prCSIData) {
		DBGLOG(NIC, WARN, "[CSI] Alloc prCSIData failed!");
		return;
	}

	prCSIData->u8TimeStamp = div_u64(kalGetBootTime(), USEC_PER_MSEC);

	prBuf = (int8_t *) (prEvent->aucBuffer);
#if CFG_CSI_DEBUG
	DBGLOG(NIC, ERROR, "[CSI] debug: i4EventLen=%d\n", i4EventLen);
	DBGLOG_MEM32(NIC, INFO, (uint8_t *) prBuf, i4EventLen);
#endif
	while ((i4EventLen >= u2Offset) && (ucLastTagFlg == false)) {
		prCSITlvData = (struct CSI_TLV_ELEMENT *) prBuf;

		DBGLOG(NIC, LOUD, "[CSI] tag_type=%d\n"
						, prCSITlvData->tag_type);
		if (prCSITlvData->tag_type == (CSI_EVENT_TLV_TAG_NUM - 1))
			ucLastTagFlg = true;

		switch (prCSITlvData->tag_type) {
		case CSI_EVENT_VERSION:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid FwVer len %u",
					prCSITlvData->body_len);
				goto out;
			}
			u4Tmp = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			prCSIData->ucFwVer = (uint8_t)u4Tmp;
			break;
		case CSI_EVENT_CBW:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CBW len %u",
					prCSITlvData->body_len);
				goto out;
			}
			prCSIData->ucBw = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RSSI:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSSI len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->cRssi =
				le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_SNR:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid SNR len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucSNR =
				le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_BAND:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid BAND len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucDbdcIdx =
				le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_CSI_NUM:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI num len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->u2DataCount =
				le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_CSI_I_DATA:
			if (prCSIData->u2DataCount > CSI_MAX_DATA_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI count %u\n",
					prCSIData->u2DataCount);
				goto out;
			}

			if (prCSITlvData->body_len !=
				sizeof(int16_t) * CSI_MAX_DATA_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI num len %u",
					prCSITlvData->body_len);
				goto out;
			}

			kalMemZero(prCSIData->ac2IData,
				sizeof(prCSIData->ac2IData));

			pru2Tmp = (int16_t *)prCSITlvData->aucbody;
			for (i2Idx = 0; i2Idx < prCSIData->u2DataCount; i2Idx++)
				prCSIData->ac2IData[i2Idx] =
					le16_to_cpup(pru2Tmp + i2Idx);
			break;
		case CSI_EVENT_CSI_Q_DATA:
			if (prCSIData->u2DataCount > CSI_MAX_DATA_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI count %u\n",
					prCSIData->u2DataCount);
				goto out;
			}

			if (prCSITlvData->body_len !=
				sizeof(int16_t) * CSI_MAX_DATA_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CSI num len %u",
					prCSITlvData->body_len);
				goto out;
			}

			kalMemZero(prCSIData->ac2QData,
				sizeof(prCSIData->ac2QData));

			pru2Tmp = (int16_t *) prCSITlvData->aucbody;
			for (i2Idx = 0; i2Idx < prCSIData->u2DataCount; i2Idx++)
				prCSIData->ac2QData[i2Idx] =
					le16_to_cpup(pru2Tmp + i2Idx);
			break;
		case CSI_EVENT_DBW:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid DBW len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucDataBw =
				le32_to_cpup((int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_CH_IDX:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid CH IDX len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucPrimaryChIdx =
				le32_to_cpup((int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_TA:
			/*
			 * TA length is 8-byte long (MAC addr 6 bytes +
			 * 2 bytes padding), the 2-byte padding keeps
			 * the next Tag at a 4-byte aligned address.
			 */
			if (prCSITlvData->body_len !=
				ALIGN_4(sizeof(prCSIData->aucTA))) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid TA len %u",
					prCSITlvData->body_len);
				goto out;
			}
			kalMemCopy(prCSIData->aucTA, prCSITlvData->aucbody,
				sizeof(prCSIData->aucTA));
			break;
		case CSI_EVENT_EXTRA_INFO:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Error len %u",
					prCSITlvData->body_len);
				goto out;
			}
			prCSIData->u4ExtraInfo =
				le32_to_cpup((int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RX_MODE:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Rx Mode len %u",
					prCSITlvData->body_len);
				goto out;
			}
			u4Tmp = le32_to_cpup((int32_t *) prCSITlvData->aucbody);

			prCSIData->ucRxMode = (uint8_t)GET_CSI_RX_MODE(u4Tmp);
			if (prCSIData->ucRxMode == 0)
				prCSIData->bIsCck = TRUE;
			else
				prCSIData->bIsCck = FALSE;

			prCSIData->u2RxRate = GET_CSI_RATE(u4Tmp);
			break;
		case CSI_EVENT_RSVD1:
			if (prCSITlvData->body_len >
				sizeof(int32_t) * CSI_MAX_RSVD1_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD1 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			kalMemCopy(prCSIData->ai4Rsvd1,
				prCSITlvData->aucbody,
				prCSITlvData->body_len);

			prCSIData->ucRsvd1Cnt =
				prCSITlvData->body_len / sizeof(int32_t);
			break;
		case CSI_EVENT_RSVD2:
			if (prCSITlvData->body_len >
				sizeof(int32_t) * CSI_MAX_RSVD2_COUNT) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD2 len %u",
					prCSITlvData->body_len);
				goto out;
			}
			prCSIData->ucRsvd2Cnt =
				prCSITlvData->body_len / sizeof(int32_t);
			p32tmp = (int32_t *)(prCSITlvData->aucbody);
			for (i2Idx = 0; i2Idx < prCSIData->ucRsvd2Cnt; i2Idx++)
				prCSIData->au4Rsvd2[i2Idx] =
						le2cpu32(*(p32tmp + i2Idx));
			break;
		case CSI_EVENT_RSVD3:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD3 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->i4Rsvd3 =
				le32_to_cpup((int32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_RSVD4:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid RSVD4 len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->ucRsvd4 =
				le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		case CSI_EVENT_H_IDX:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid Antenna_pattern len %u",
					prCSITlvData->body_len);
				goto out;
			}

			prCSIData->Antenna_pattern =
				le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);

			/*to drop pkt that usr do not need*/
			if ((prCSIInfo->Matrix_Get_Bit) &&
				!(prCSIInfo->Matrix_Get_Bit &
				(1 << (prCSIData->Antenna_pattern & 0xf)))) {
				DBGLOG(NIC, WARN,
					"[CSI] drop Antenna_pattern=%d\n",
					prCSIData->Antenna_pattern);
				goto out;
			}
			break;
		case CSI_EVENT_TX_RX_IDX:
			if (prCSITlvData->body_len != sizeof(uint32_t)) {
				DBGLOG(NIC, WARN,
					"[CSI] Invalid TRxIdx len %u",
							prCSITlvData->body_len);
				goto out;
			}
			prCSIData->u4TRxIdx = le32_to_cpup(
					(uint32_t *) prCSITlvData->aucbody);
			break;
		default:
			DBGLOG(NIC, WARN, "[CSI] Unsupported CSI tag %d\n",
				prCSITlvData->tag_type);
		};

		i4EventLen -= (u2Offset + prCSITlvData->body_len);

		if (i4EventLen >= u2Offset)
			prBuf += (u2Offset + prCSITlvData->body_len);
	}

	/*mask the null tone && pilot tone*/
	if ((prCSIInfo->ucValue1[CSI_CONFIG_OUTPUT_FORMAT] ==
		CSI_OUTPUT_TONE_MASKED ||
		prCSIInfo->ucValue1[CSI_CONFIG_OUTPUT_FORMAT] ==
		CSI_OUTPUT_TONE_MASKED_SHIFTED) &&
		!prCSIData->bIsCck) {
		wlanApplyCSIToneMask(prCSIData->ucRxMode,
			prCSIData->ucBw, prCSIData->ucDataBw,
			prCSIData->ucPrimaryChIdx,
			prCSIData->ac2IData, prCSIData->ac2QData);
	}

	/*reoder the tone */
	if (prCSIInfo->ucValue1[CSI_CONFIG_OUTPUT_FORMAT] ==
		CSI_OUTPUT_TONE_MASKED_SHIFTED &&
		!prCSIData->bIsCck) {
		kalMemCopy(prCSIInfo->ai2TempIData,
			prCSIData->ac2IData,
			sizeof(int16_t) * prCSIData->u2DataCount);
		kalMemCopy(prCSIInfo->ai2TempQData,
			prCSIData->ac2QData,
			sizeof(int16_t) * prCSIData->u2DataCount);
		wlanShiftCSI(prCSIData->ucRxMode,
			prCSIData->ucBw, prCSIData->ucDataBw,
			prCSIData->ucPrimaryChIdx,
			prCSIInfo->ai2TempIData,
			prCSIInfo->ai2TempQData,
			prCSIData->ac2IData,
			prCSIData->ac2QData);

		if (prCSIData->ucDataBw == RX_VT_FR_MODE_20)
			prCSIData->u2DataCount = 64;
		else if (prCSIData->ucDataBw == RX_VT_FR_MODE_40)
			prCSIData->u2DataCount = 128;
		else
			prCSIData->u2DataCount = 256;
		DBGLOG(INIT, INFO, "[CSI] u2DataCount=%d\n",
					prCSIData->u2DataCount);
	}

#if CFG_SUPPORT_CSI_NF
	ucRes = wlanCSINoiseFilter(prAdapter, prCSIData);
	if (ucRes == FALSE) {
		DBGLOG(INIT, STATE, "[CSI] drop noise\n");
		goto out;
	}
#endif

	wlanPushCSIData(prAdapter, prCSIData);
	wake_up_interruptible(&(prAdapter->rCSIInfo.waitq));

out:
	kalMemFree(prCSIData, VIR_MEM_TYPE, sizeof(struct CSI_DATA_T));
}
#endif



#if (CFG_COALESCING_INTERRUPT == 1)
void nicEventCoalescingIntDone(IN struct ADAPTER *prAdapter,
		IN struct WIFI_EVENT *prEvent)
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
#endif

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
void nicCmdEventQuerySerInfo(IN struct ADAPTER *prAdapter,
			     IN struct CMD_INFO *prCmdInfo,
			     IN uint8_t *pucEventBuf)
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

		if (prEventSer->ucEvtVer >= EXT_EVENT_SER_VER)
			u4QueryInfoLen = sizeof(struct PARAM_SER_INFO_T);
		else
			u4QueryInfoLen = prEventSer->u2EvtLen;

		kalMemCopy(prQuerySerInfo, pucEventBuf, u4QueryInfoLen);

		if (prEventSer->ucEvtVer >= EXT_EVENT_SER_VER)
			prQuerySerInfo->ucEvtVer = EXT_EVENT_SER_VER;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

#if CFG_WIFI_TXPWR_TBL_DUMP
void nicCmdEventGetTxPwrTbl(IN struct ADAPTER *prAdapter,
		IN struct CMD_INFO *prCmdInfo,
		IN uint8_t *pucEventBuf)
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
					prCmdInfo->fgSetQuery,
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
				u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#endif /* CFG_WIFI_TXPWR_TBL_DUMP */

#if (CFG_WIFI_ISO_DETECT == 1)
void nicCmdEventQueryCoexIso(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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
				prCmdInfo->fgSetQuery,
				u4QueryInfoLen,
				WLAN_STATUS_SUCCESS);

	}

}
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
void nicCmdEventQueryTxMcsInfo(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

void nicEventTxMcsInfo(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	DBGLOG(RSN, INFO, "EVENT_ID_TX_MCS_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter,
					 prEvent->ucSeqNum);

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

#if CFG_SUPPORT_NAN
uint32_t nicDumpTlv(IN void *prCmdBuffer)
{
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint16_t u2ElementNum = 1;
	uint32_t u4BodyByteCnt;

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	DBGLOG(TX, INFO, "u2TotalElementNum:%d\n",
	       prTlvCommon->u2TotalElementNum);

	for (u2ElementNum = 1; u2ElementNum <= prTlvCommon->u2TotalElementNum;
	     u2ElementNum++) {
		prTlvElement =
			nicGetTargetTlvElement(u2ElementNum, prCmdBuffer);
		DBGLOG(TX, INFO, "TLV(%d) start address:%p\n", u2ElementNum,
		       prTlvElement);
		DBGLOG(TX, INFO, "TLV(%d) tag_type:%d\n", u2ElementNum,
		       (uint32_t)prTlvElement->tag_type);
		DBGLOG(TX, INFO, "TLV(%d) body_len:%d\n", u2ElementNum,
		       (uint32_t)prTlvElement->body_len);

		for (u4BodyByteCnt = 0; u4BodyByteCnt < prTlvElement->body_len;
		     u4BodyByteCnt++) {
			DBGLOG(TX, INFO, "TLV(%d) body[%d]:%x\n", u2ElementNum,
			       u4BodyByteCnt,
			       prTlvElement->aucbody[u4BodyByteCnt]);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

struct _CMD_EVENT_TLV_ELEMENT_T *nicGetTargetTlvElement(
		   IN uint16_t u2TargetTlvElement, IN void *prCmdBuffer)
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

	/* Get target element */
	pvCurrPtr = prCmdBuffer;

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

uint32_t nicAddNewTlvElement(IN uint32_t u4Tag, IN uint32_t u4BodyLen,
		    IN uint32_t prCmdBufferLen, IN void *prCmdBuffer)
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

void nicNanEventTestProcess(IN struct ADAPTER *prAdapter,
		       IN struct WIFI_EVENT *prEvent)
{
	struct CMD_INFO *prCmdInfo;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return;
	}
	if (!prEvent) {
		DBGLOG(NAN, ERROR, "prEvent error!\n");
		return;
	}

	DBGLOG(TX, INFO, "nicNanEventDispatcher\n");

	/* Dump Event content */
	nicDumpTlv((void *)prEvent->aucBuffer);

	/* Process CMD done handler */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				       prCmdInfo->fgSetQuery, 0,
				       WLAN_STATUS_SUCCESS);

		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

void nicNanEventSTATxCTL(IN struct ADAPTER *prAdapter, IN uint8_t *pcuEvtBuf)
{
	struct EVENT_UPDATE_NAN_TX_STATUS *prUpdateTxStatus;

	prUpdateTxStatus = (struct EVENT_UPDATE_NAN_TX_STATUS *)pcuEvtBuf;
	qmUpdateFreeNANQouta(prAdapter, prUpdateTxStatus);
}

void nicNanVendorEventHandler(IN struct ADAPTER *prAdapter,
			 IN struct WIFI_EVENT *prEvent)
{
	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return;
	}
	if (!prEvent) {
		DBGLOG(NAN, ERROR, "prEvent error!\n");
		return;
	}

	DBGLOG(NAN, INFO, "[%s] IN, Guiding to Vendor event handler\n",
	       __func__);

	kalNanHandleVendorEvent(prAdapter, prEvent->aucBuffer);
}

struct NanMatchInd g_rDiscMatchInd;
uint8_t g_u2IndPubId;

void nicNanEventDiscoveryResult(IN struct ADAPTER *prAdapter,
	    IN uint8_t *pcuEvtBuf)
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
void nicNanReceiveEvent(IN struct ADAPTER *prAdapter, IN uint8_t *pcuEvtBuf)
{
	struct NAN_FOLLOW_UP_EVENT *prDiscEvt;

	prDiscEvt = (struct NAN_FOLLOW_UP_EVENT *)pcuEvtBuf;
	dumpMemory8((uint8_t *)pcuEvtBuf, 32);
	DBGLOG(NAN, LOUD, "receive followup event\n");
	kalMemSet(&rFollowInd, 0, sizeof(struct NanFollowupInd));
	rFollowInd.eventID = ENUM_NAN_RECEIVE;
	rFollowInd.publish_subscribe_id = prDiscEvt->publish_subscribe_id;
	rFollowInd.requestor_instance_id = prDiscEvt->requestor_instance_id;
	kalMemCopy(rFollowInd.addr, prDiscEvt->addr, MAC_ADDR_LEN);
	rFollowInd.service_specific_info_len =
		prDiscEvt->service_specific_info_len;
	kalMemCopy(rFollowInd.service_specific_info,
		   prDiscEvt->service_specific_info,
		   prDiscEvt->service_specific_info_len);
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rFollowInd,
				sizeof(struct NanFollowupInd));
}

void nicNanRepliedEvnt(IN struct ADAPTER *prAdapter, IN uint8_t *pcuEvtBuf)
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

void nicNanPublishTerminateEvt(IN struct ADAPTER *prAdapter,
		   IN uint8_t *pcuEvtBuf)
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

void nicNanSubscribeTerminateEvt(IN struct ADAPTER *prAdapter,
			    IN uint8_t *pcuEvtBuf)
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

void nicNanNdlFlowCtrlEvt(IN struct ADAPTER *prAdapter, IN uint8_t *pcuEvtBuf)
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
		for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDP_NUM; u4Idx++) {
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

void nicNanEventDispatcher(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent)
{
	ASSERT(prAdapter);
	ASSERT(prEvent);

	DBGLOG(INIT, WARN, "nicNanEventDispatcher\n");

	if (prAdapter->fgIsNANfromHAL == FALSE) {
		DBGLOG(INIT, WARN, "nicNanIOEventHandler\n");
		/* For IOCTL use */
		nicNanIOEventHandler(prAdapter, prEvent);
	} else {
		DBGLOG(INIT, WARN, "nicNanVendorEventHandler\n");
		/* For Vendor command use */
		nicNanVendorEventHandler(prAdapter, prEvent);
	}
}

void nicNanIOEventHandler(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent)
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

	switch (u4SubEvent) {
	case NAN_EVENT_TEST:
		nicNanEventTestProcess(prAdapter, prEvent);
		break;
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
#endif
	case NAN_EVENT_NDL_DISCONNECT:
		nanDataEngingDisconnectEvt(prAdapter, prTlvElement->aucbody);
	}
}

void nicNanGetCmdInfoQueryTestBuffer(
	struct _TXM_CMD_EVENT_TEST_T **prCmdInfoQueryTestBuffer)
{
	*prCmdInfoQueryTestBuffer =
		(struct _TXM_CMD_EVENT_TEST_T *)&grCmdInfoQueryTestBuffer;
}

void nicNanTestQueryInfoDone(IN struct ADAPTER *prAdapter,
	    IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	struct GLUE_INFO *prGlueInfo;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _TXM_CMD_EVENT_TEST_T *prEventContent = NULL;
	struct _TXM_CMD_EVENT_TEST_T *prQueryInfoContent = NULL;
	uint32_t u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	DBGLOG(TX, INFO, "nicNanTestQueryInfoDone\n");

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)pucEventBuf;
		prTlvElement = nicGetTargetTlvElement(1, prTlvCommon);
		prEventContent =
			(struct _TXM_CMD_EVENT_TEST_T *)prTlvElement->aucbody;
		prQueryInfoContent =
			(struct _TXM_CMD_EVENT_TEST_T *)
				prCmdInfo->pvInformationBuffer;
		prQueryInfoContent->u4TestValue0 = prEventContent->u4TestValue0;
		prQueryInfoContent->u4TestValue1 = prEventContent->u4TestValue1;
		prQueryInfoContent->ucTestValue2 = prEventContent->ucTestValue2;
		u4QueryInfoLen = sizeof(struct _TXM_CMD_EVENT_TEST_T);

		nicDumpTlv((void *)pucEventBuf);

		DBGLOG(TX, INFO, "grCmdInfoQueryTestBuffer.u4TestValue0 = %x\n",
		       grCmdInfoQueryTestBuffer.u4TestValue0);
		DBGLOG(TX, INFO, "grCmdInfoQueryTestBuffer.u4TestValue1 = %x\n",
		       grCmdInfoQueryTestBuffer.u4TestValue1);
		DBGLOG(TX, INFO, "grCmdInfoQueryTestBuffer.ucTestValue2 = %x\n",
		       grCmdInfoQueryTestBuffer.ucTestValue2);

		if ((grCmdInfoQueryTestBuffer.u4TestValue0 == 0x22222222) &&
		    (grCmdInfoQueryTestBuffer.u4TestValue1 == 0x22222222) &&
		    (grCmdInfoQueryTestBuffer.ucTestValue2 == 0x22)) {
			DBGLOG(TX, INFO, ">>CMD done content check pass\n");
		} else {
			DBGLOG(TX, INFO, ">>CMD done content check fail\n");
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#endif

#if (CFG_WIFI_GET_DPD_CACHE == 1)
void nicCmdEventQueryDpdCache(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
				u4QueryInfoLen, u4Status);
	}
}
#endif /* CFG_WIFI_GET_DPD_CACHE */

#if (CFG_SUPPORT_TSF_SYNC == 1)
void nicCmdEventLatchTSF(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
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

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	return;

}
#endif

