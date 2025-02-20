/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_ate_agent.h
 *    \brief  This file includes private ioctl support.
 */

#ifndef _GL_ATE_AGENT_H
#define _GL_ATE_AGENT_H
#if CFG_SUPPORT_QA_TOOL
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#if (CFG_SUPPORT_CONNAC3X == 0)
extern uint32_t u4RxStatSeqNum;
#else
extern uint16_t u2RxStatSeqNum;
#endif

#if CFG_SUPPORT_TX_BF
extern union PFMU_PROFILE_TAG1 g_rPfmuTag1;
extern union PFMU_PROFILE_TAG2 g_rPfmuTag2;
extern union PFMU_DATA g_rPfmuData;
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct STA_REC_BF_UPD_ARGUMENT {
	uint32_t u4WlanId;
	uint32_t u4BssId;
	uint32_t u4PfmuId;
	uint32_t u4SuMu;
	uint32_t u4eTxBfCap;
	uint32_t u4NdpaRate;
	uint32_t u4NdpRate;
	uint32_t u4ReptPollRate;
	uint32_t u4TxMode;
	uint32_t u4Nc;
	uint32_t u4Nr;
	uint32_t u4Bw;
	uint32_t u4SpeIdx;
	uint32_t u4TotalMemReq;
	uint32_t u4MemReq20M;
	uint32_t au4MemRow[4];
	uint32_t au4MemCol[4];
};

struct ATE_OPS_T {
	int32_t (*setICapStart)(struct GLUE_INFO *prGlueInfo,
				uint32_t fgTrigger,
				uint32_t fgRingCapEn,
				uint32_t u4Event,
				uint32_t u4Node,
				uint32_t u4Len,
				uint32_t u4StopCycle,
				uint32_t u4BW,
				uint32_t u4MACTriggerEvent,
				uint32_t u4SourceAddrLSB,
				uint32_t u4SourceAddrMSB,
				uint32_t u4Band);
	int32_t (*getICapStatus)(struct GLUE_INFO *prGlueInfo);
	int32_t (*getICapIQData)(struct GLUE_INFO *prGlueInfo,
				 uint8_t *pData,
				 uint32_t u4IQType,
				 uint32_t u4WFNum);
	void (*getRbistDataDumpEvent)(struct ADAPTER *prAdapter,
				      uint8_t *pucEventBuf);

#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
	void (*getICapDataDumpCmdEvent)(struct ADAPTER *prAdapter,
					struct CMD_INFO *prCmdInfo,
					uint8_t *pucEventBuf);
#endif
	void (*icapRiseVcoreClockRate)(void);
	void (*icapDownVcoreClockRate)(void);
};


/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

int Set_ResetStatCounter_Proc(void *prNetDev,
			      uint8_t *prInBuf);
int SetATE(void *prNetDev, uint8_t *prInBuf);
int SetATEDa(void *prNetDev, uint8_t *prInBuf);
int SetATESa(void *prNetDev, uint8_t *prInBuf);
int SetATEChannel(void *prNetDev,
		  uint8_t *prInBuf);
int SetATETxPower0(void *prNetDev,
		   uint8_t *prInBuf);
int SetATETxGi(void *prNetDev,
	       uint8_t *prInBuf);
int SetATETxBw(void *prNetDev,
	       uint8_t *prInBuf);
int SetATETxMode(void *prNetDev,
		 uint8_t *prInBuf);
int SetATETxLength(void *prNetDev,
		   uint8_t *prInBuf);
int SetATETxCount(void *prNetDev,
		  uint8_t *prInBuf);
int SetATETxMcs(void *prNetDev,
		uint8_t *prInBuf);
int SetATEIpg(void *prNetDev,
	      uint8_t *prInBuf);

#if CFG_SUPPORT_TX_BF
int Set_TxBfProfileTag_Help(void *prNetDev,
			    uint8_t *prInBuf);
int Set_TxBfProfileTag_InValid(void *prNetDev,
			       uint8_t *prInBuf);
int Set_TxBfProfileTag_PfmuIdx(void *prNetDev,
			       uint8_t *prInBuf);
int Set_TxBfProfileTag_BfType(void *prNetDev,
			      uint8_t *prInBuf);
int Set_TxBfProfileTag_DBW(void *prNetDev,
			   uint8_t *prInBuf);
int Set_TxBfProfileTag_SuMu(void *prNetDev,
			    uint8_t *prInBuf);
int Set_TxBfProfileTag_Mem(void *prNetDev,
			   uint8_t *prInBuf);
int Set_TxBfProfileTag_Matrix(void *prNetDev,
			      uint8_t *prInBuf);
int Set_TxBfProfileTag_SNR(void *prNetDev,
			   uint8_t *prInBuf);
int Set_TxBfProfileTag_SmartAnt(void *prNetDev,
				uint8_t *prInBuf);
int Set_TxBfProfileTag_SeIdx(void *prNetDev,
			     uint8_t *prInBuf);
int Set_TxBfProfileTag_RmsdThrd(void *prNetDev,
				uint8_t *prInBuf);
int Set_TxBfProfileTag_McsThrd(void *prNetDev,
			       uint8_t *prInBuf);
int Set_TxBfProfileTag_TimeOut(void *prNetDev,
			       uint8_t *prInBuf);
int Set_TxBfProfileTag_DesiredBW(struct net_device
				 *prNetDev, uint8_t *prInBuf);
int Set_TxBfProfileTag_DesiredNc(struct net_device
				 *prNetDev, uint8_t *prInBuf);
int Set_TxBfProfileTag_DesiredNr(struct net_device
				 *prNetDev, uint8_t *prInBuf);
int Set_TxBfProfileTagRead(void *prNetDev,
			   uint8_t *prInBuf);
int Set_TxBfProfileTagWrite(void *prNetDev,
			    uint8_t *prInBuf);
int Set_StaRecCmmUpdate(void *prNetDev,
			uint8_t *prInBuf);
int Set_StaRecBfUpdate(void *prNetDev,
		       uint8_t *prInBuf);
int Set_StaRecBfRead(void *prNetDev,
		       uint8_t *prInBuf);

int Set_DevInfoUpdate(void *prNetDev,
		      uint8_t *prInBuf);

int Set_BssInfoUpdate(void *prNetDev,
		      uint8_t *prInBuf);
int Set_TxBfProfileDataRead(void *prNetDev,
			    uint8_t *prInBuf);
int Set_TxBfProfileDataWrite(void *prNetDev,
			     uint8_t *prInBuf);
int Set_Trigger_Sounding_Proc(void *prNetDev,
			      uint8_t *prInBuf);
int Set_Stop_Sounding_Proc(void *prNetDev,
			   uint8_t *prInBuf);
int Set_TxBfTxApply(void *prNetDev,
		    uint8_t *prInBuf);
int Set_TxBfProfilePnRead(void *prNetDev,
			  uint8_t *prInBuf);
int Set_TxBfProfilePnWrite(void *prNetDev,
			   uint8_t *prInBuf);
int Set_TxBfManualAssoc(void *prNetDev,
			uint8_t *prInBuf);
int Set_TxBfPfmuMemAlloc(void *prNetDev,
			 uint8_t *prInBuf);
int Set_TxBfPfmuMemRelease(void *prNetDev,
			   uint8_t *prInBuf);

#if CFG_SUPPORT_MU_MIMO
int Set_MUGetInitMCS(void *prNetDev,
		     uint8_t *prInBuf);
int Set_MUCalInitMCS(void *prNetDev,
		     uint8_t *prInBuf);
int Set_MUCalLQ(void *prNetDev,
		uint8_t *prInBuf);
int Set_MUGetLQ(void *prNetDev,
		uint8_t *prInBuf);
int Set_MUSetSNROffset(void *prNetDev,
		       uint8_t *prInBuf);
int Set_MUSetZeroNss(void *prNetDev,
		     uint8_t *prInBuf);
int Set_MUSetSpeedUpLQ(void *prNetDev,
		       uint8_t *prInBuf);
int Set_MUSetMUTable(void *prNetDev,
		     uint8_t *prInBuf);
int Set_MUSetGroup(void *prNetDev,
		   uint8_t *prInBuf);
int Set_MUGetQD(void *prNetDev,
		uint8_t *prInBuf);
int Set_MUSetEnable(void *prNetDev,
		    uint8_t *prInBuf);
int Set_MUSetGID_UP(void *prNetDev,
		    uint8_t *prInBuf);
int Set_MUTriggerTx(void *prNetDev,
		    uint8_t *prInBuf);
#endif

#if CFG_SUPPORT_TX_BF_FPGA
int Set_TxBfProfileSwTagWrite(void *prNetDev,
			      uint8_t *prInBuf);
#endif
#endif


int WriteEfuse(void *prNetDev,
	       uint8_t *prInBuf);
int SetTxTargetPower(void *prNetDev,
		     uint8_t *prInBuf);

#if (CFG_SUPPORT_DFS_MASTER == 1)
int SetRddReport(void *prNetDev,
		 uint8_t *prInBuf);
int SetByPassCac(void *prNetDev,
		 uint8_t *prInBuf);
int SetRadarDetectMode(void *prNetDev,
		       uint8_t *prInBuf);
#endif

int AteCmdSetHandle(void *prNetDev,
		    uint8_t *prInBuf, uint32_t u4InBufLen);

#endif /*CFG_SUPPORT_QA_TOOL */
#endif /* _GL_ATE_AGENT_H */
