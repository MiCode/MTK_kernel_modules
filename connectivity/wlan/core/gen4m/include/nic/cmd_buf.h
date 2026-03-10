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
/*
 * Id:
 */

/*! \file   "cmd_buf.h"
 *   \brief  In this file we define the structure for Command Packet.
 *
 *		In this file we define the structure for Command Packet and the
 *    control unit of MGMT Memory Pool.
 */


#ifndef _CMD_BUF_H
#define _CMD_BUF_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum EUNM_CMD_SEND_METHOD {
	CMD_SEND_METHOD_ENQUEUE = 0,
	CMD_SEND_METHOD_REQ_RESOURCE,
	CMD_SEND_METHOD_TX
};

enum COMMAND_TYPE {
	COMMAND_TYPE_NETWORK_IOCTL,
	COMMAND_TYPE_MANAGEMENT_FRAME,
	COMMAND_TYPE_DATA_FRAME,
	COMMAND_TYPE_NUM
};

typedef void(*PFN_CMD_DONE_HANDLER) (struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);

typedef void(*PFN_CMD_TIMEOUT_HANDLER) (struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo);

typedef void(*PFN_HIF_TX_CMD_DONE_CB) (struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo);

struct CMD_INFO {
	struct QUE_ENTRY rQueEntry;

	enum COMMAND_TYPE eCmdType;

	uint16_t u2InfoBufLen;	/* This is actual CMD buffer length */
	uint8_t *pucInfoBuffer;	/* May pointer to structure in prAdapter */
	struct MSDU_INFO *prMsduInfo;
				/* only valid when it's a CmdData/MGMT frame */
	void *prPacket;	/* only valid when it's a CmdData frame */

	PFN_CMD_DONE_HANDLER pfCmdDoneHandler;
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler;
	PFN_HIF_TX_CMD_DONE_CB pfHifTxCmdDoneCb;

	u_int8_t fgIsOid;	/* Used to check if we need indicate */

	uint8_t ucCID;
	u_int8_t fgSetQuery;
	u_int8_t fgNeedResp;
	uint8_t ucCmdSeqNum;
	uint32_t u4SetInfoLen;	/* Indicate how many byte we read for Set OID */
	uint8_t *pucSetInfoBuffer; /* ptr to cmd content without cmd header */

	/* information indicating by OID/ioctl */
	void *pvInformationBuffer;
	uint32_t u4InformationBufferLength;

	/* private data */
	//uint32_t u4PrivateData;

	/* TXD/TXP pointer/len for hif tx copy */
	uint32_t u4TxdLen;
	uint32_t u4TxpLen;
	uint8_t *pucTxd;
	uint8_t *pucTxp;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#if CFG_DBG_MGT_BUF
#define cmdBufAllocateCmdInfo(_prAdapter, u4Length) \
	cmdBufAllocateCmdInfoX(_prAdapter, u4Length, \
		__FILE__ ":" STRLINE(__LINE__))
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void cmdBufInitialize(struct ADAPTER *prAdapter);

#if CFG_DBG_MGT_BUF
struct CMD_INFO *cmdBufAllocateCmdInfoX(struct ADAPTER
					   *prAdapter, uint32_t u4Length,
					   uint8_t *fileAndLine);
#else
struct CMD_INFO *cmdBufAllocateCmdInfo(struct ADAPTER
				       *prAdapter, uint32_t u4Length);
#endif

void cmdBufFreeCmdInfo(struct ADAPTER *prAdapter,
		       struct CMD_INFO *prCmdInfo);

/*----------------------------------------------------------------------------*/
/* Routines for CMDs                                                          */
/*----------------------------------------------------------------------------*/

uint32_t
wlanSendSetQueryCmd(struct ADAPTER *prAdapter,
		    uint8_t ucCID,
		    u_int8_t fgSetQuery,
		    u_int8_t fgNeedResp,
		    u_int8_t fgIsOid,
		    PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
		    PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
		    uint32_t u4SetQueryInfoLen,
		    uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
		    uint32_t u4SetQueryBufferLen);

uint32_t
wlanSendSetQueryCmdAdv(struct ADAPTER *prAdapter,
		    uint8_t ucCID,
		    uint8_t ucExtCID,
		    u_int8_t fgSetQuery,
		    u_int8_t fgNeedResp,
		    u_int8_t fgIsOid,
		    PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
		    PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
		    uint32_t u4SetQueryInfoLen,
		    uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
		    uint32_t u4SetQueryBufferLen,
		    enum EUNM_CMD_SEND_METHOD eMethod);

uint32_t
wlanSendSetQueryExtCmd(struct ADAPTER *prAdapter,
		       uint8_t ucCID,
		       uint8_t ucExtCID,
		       u_int8_t fgSetQuery,
		       u_int8_t fgNeedResp,
		       u_int8_t fgIsOid,
		       PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
		       PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
		       uint32_t u4SetQueryInfoLen,
		       uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
		       uint32_t u4SetQueryBufferLen);

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
	uint32_t u4EvtWaitTimeout);

uint32_t wlanSendInitSetQueryCmd(struct ADAPTER *prAdapter,
	uint8_t ucCmdId,
	void *pucCmdBuf,
	uint32_t u4CmdSz,
	u_int8_t fgWaitResp,
	u_int8_t fgSkipCheckSeq,
	uint8_t ucEvtId,
	void *pucEvtBuf,
	uint32_t u4EvtSz);

void cmdBufDumpCmdQueue(struct QUE *prQueue,
			int8_t *queName);
#if (CFG_SUPPORT_TRACE_TC4 == 1)
void wlanDebugTC4Init(void);
void wlanDebugTC4Uninit(void);
void wlanTraceReleaseTcRes(struct ADAPTER *prAdapter,
			   uint32_t u4TxRlsCnt, uint32_t u4Available);
void wlanTraceTxCmd(struct CMD_INFO *prCmd);
void wlanDumpTcResAndTxedCmd(uint8_t *pucBuf,
			     uint32_t maxLen);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _CMD_BUF_H */
