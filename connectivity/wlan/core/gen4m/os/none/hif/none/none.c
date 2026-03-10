/****************************************************************************
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
 ***************************************************************************/
/****************************************************************************
 *[File]             axi.c
 *[Version]          v1.0
 *[Revision Date]    2010-03-01
 *[Author]
 *[Description]
 *    The program provides AXI HIF driver
 *[Copyright]
 *    Copyright (C) 2010 MediaTek Incorporation. All Rights Reserved.
 ****************************************************************************/


/*****************************************************************************
 *                         C O M P I L E R   F L A G S
 *****************************************************************************
 */

/*****************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *****************************************************************************
 */

#include "gl_os.h"

#include "precomp.h"

#include "hif.h"

/*****************************************************************************
 *                              C O N S T A N T S
 *****************************************************************************
 */

/*****************************************************************************
 *                             D A T A   T Y P E S
 *****************************************************************************
 */

/*****************************************************************************
 *                            P U B L I C   D A T A
 *****************************************************************************
 */

/*****************************************************************************
 *                           P R I V A T E   D A T A
 *****************************************************************************
 */


/*****************************************************************************
 *                                 M A C R O S
 *****************************************************************************
 */

/*****************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *****************************************************************************
 */


/*****************************************************************************
 *                              F U N C T I O N S
 *****************************************************************************
 */

void kal_virt_write_tx_port(struct ADAPTER *ad,
	uint16_t pid, uint32_t len, uint8_t *buf, uint32_t buf_size)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_get_wifi_func_stat(struct ADAPTER *ad, uint32_t *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_chk_wifi_func_off(struct ADAPTER *ad, uint32_t ready_bits,
	uint8_t *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_chk_wifi_func_ready(struct ADAPTER *ad, uint32_t ready_bits,
	uint8_t *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_set_mailbox_readclear(struct ADAPTER *ad, bool enable)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_set_int_stat_readclear(struct ADAPTER *ad)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_init_hif(struct ADAPTER *ad)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_enable_fwdl(struct ADAPTER *ad, bool enable)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_get_int_status(struct ADAPTER *ad, uint32_t *status)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_virt_uhw_rd(struct ADAPTER *ad, uint32_t u4Offset, uint32_t *pu4Value,
		     u_int8_t *pfgSts)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, ad, u4Offset, pu4Value,
			   pfgSts);
}

void kal_virt_uhw_wr(struct ADAPTER *ad, uint32_t u4Offset, uint32_t u4Value,
		     u_int8_t *pfgSts)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, ad, u4Offset, u4Value,
			   pfgSts);
}

void kal_virt_cancel_tx_rx(struct ADAPTER *ad)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, ad);
}

void kal_virt_resume_tx_rx(struct ADAPTER *ad)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, ad);
}

uint32_t kal_virt_toggle_wfsys_rst(struct ADAPTER *ad)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, ad);
}

/* the following functions are defined in include/nic/hal.h
 * need to be implemented directly in os/hif
 */
bool halHifSwInfoInit(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halHifSwInfoUnInit(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void halPrintHifDbgInfo(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halHifPowerOffWifi(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halTxResourceResetHwTQCounter(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halDumpHifStatus(struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint32_t u4Max)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halGetValidCoalescingBufSize(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halAllocateIOBuffer(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halReleaseIOBuffer(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halDisableInterrupt(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halEnableInterrupt(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

u_int8_t halVerifyChipID(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halProcessAbnormalInterrupt(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halProcessSoftwareInterrupt(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halProcessTxInterrupt(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halSerHifReset(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

u_int8_t halIsTxResourceControlEn(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halTxPollingResource(struct ADAPTER *prAdapter, uint8_t ucTC)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

static uint32_t halGetHifTxPageSize(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halGetHifTxDataPageSize(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halGetHifTxCmdPageSize(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

u_int8_t halTxIsDataBufEnough(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

u_int8_t halTxIsBssCntFull(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

static uint32_t halTxGetPageCount(struct ADAPTER *prAdapter,
	uint32_t u4FrameLength, u_int8_t fgIncludeDesc)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halTxGetDataPageCount(struct ADAPTER *prAdapter,
	uint32_t u4FrameLength, u_int8_t fgIncludeDesc)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halTxGetCmdPageCount(struct ADAPTER *prAdapter,
	uint32_t u4FrameLength, u_int8_t fgIncludeDesc)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halTxCancelSendingCmd(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halRxProcessMsduReport(struct ADAPTER *prAdapter,
			    struct SW_RFB *prSwRfb)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

u_int8_t halIsPendingRx(struct ADAPTER *prAdapter)
{
	/* TODO: check pending Rx
	 * if previous Rx handling is break due to lack of SwRfb
	 */
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t
halRxWaitResponse(struct ADAPTER *prAdapter, uint8_t ucPortIdx,
	uint8_t *pucRspBuffer, uint32_t u4MaxRespBufferLen, uint32_t *pu4Length,
	uint32_t u4WaitingInterval, uint32_t u4TimeoutValue)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halSetFWOwn(struct ADAPTER *prAdapter, u_int8_t fgEnableGlobalInt)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

u_int8_t halSetDriverOwn(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint8_t halTxRingDataSelect(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Check if HIF state is during supend process
*
* @param prAdapter      Pointer to the Adapter structure.
*
* @return (TRUE: suspend, reject the caller action. FALSE: not suspend)
*/
/*----------------------------------------------------------------------------*/
bool halIsHifStateSuspend(struct ADAPTER *prAdapter)
{
	/* HIF owner should implement this function */
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
	return FALSE;
}

void halUpdateHifConfig(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halDumpHifStats(struct ADAPTER *prAdapter)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

void halUpdateBssTokenCnt(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

#if (CFG_TX_HIF_CREDIT_FEATURE == 1)
void halAdjustBssTxCredit(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint32_t halGetBssTxCredit(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
};

u_int8_t halTxIsBssCreditCntFull(uint32_t u4TxCredit)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#endif

u_int8_t halIsHifStateReady(struct GLUE_INFO *prGlueInfo,
	uint8_t *pucState)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

u_int8_t halProcessToken(struct ADAPTER *prAdapter,
	uint32_t u4Token,
	struct QUE *prFreeQueue)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void halMsduReportStats(struct ADAPTER *prAdapter, uint32_t u4Token,
	uint32_t u4MacLatency, uint32_t u4Stat)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, prAdapter);
}

uint8_t halSetRxRingHwAddr(
	struct RTMP_RX_RING *prRxRing,
	struct BUS_INFO *prBusInfo,
	uint32_t u4SwRingIdx)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

bool halWpdmaAllocRxRing(struct GLUE_INFO *prGlueInfo, uint32_t u4Num,
			 uint32_t u4Size, uint32_t u4DescSize,
			 uint32_t u4BufSize, bool fgAllocMem)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void halWpdmaProcessCmdDmaDone(struct GLUE_INFO *prGlueInfo,
	uint16_t u2Port)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void halWpdmaProcessDataDmaDone(struct GLUE_INFO *prGlueInfo,
	uint16_t u2Port)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void halRxReceiveRFBs(struct ADAPTER *prAdapter, uint32_t u4Port,
	uint8_t fgRxData)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

u_int8_t halTxIsCmdBufEnough(struct ADAPTER *prAdapter)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
