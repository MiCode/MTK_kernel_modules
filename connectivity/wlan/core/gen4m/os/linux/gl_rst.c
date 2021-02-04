/*******************************************************************************
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
 ******************************************************************************/
/*
 ** Id: @(#) gl_rst.c@@
 */

/*! \file   gl_rst.c
 *    \brief  Main routines for supporintg MT6620 whole-chip reset mechanism
 *
 *    This file contains the support routines of Linux driver for MediaTek Inc.
 *    802.11 Wireless LAN Adapters.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include <linux/kernel.h>
#include <linux/workqueue.h>

#include "precomp.h"
#include "gl_rst.h"

#if CFG_CHIP_RESET_SUPPORT

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
static u_int8_t fgResetTriggered = FALSE;
u_int8_t fgIsResetting = FALSE;
u_int8_t fgSimplifyResetFlow = FALSE;
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static struct RESET_STRUCT wifi_rst;

static void mtk_wifi_reset(struct work_struct *work);
static void mtk_wifi_trigger_reset(struct work_struct *work);

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void *glResetCallback(enum ENUM_WMTDRV_TYPE eSrcType,
			     enum ENUM_WMTDRV_TYPE eDstType,
			     enum ENUM_WMTMSG_TYPE eMsgType, void *prMsgBody,
			     unsigned int u4MsgLength);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/* Weak reference for those platform doesn't support wmt functions */
u_int8_t __weak mtk_wcn_stp_coredump_start_get(void)
{
	return FALSE;
}


/*0= f/w assert flag is not set, others=f/w assert flag is set */
u_int8_t glIsWmtCodeDump(void)
{
	return mtk_wcn_stp_coredump_start_get();
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for
 *        1. register wifi reset callback
 *        2. initialize wifi reset work
 *
 * @param none
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void glResetInit(void)
{
	/* 1. Register reset callback */
	mtk_wcn_wmt_msgcb_reg(WMTDRV_TYPE_WIFI,
			      (PF_WMT_CB) glResetCallback);

	/* 2. Initialize reset work */
	INIT_WORK(&(wifi_rst.rst_work), mtk_wifi_reset);
	INIT_WORK(&(wifi_rst.rst_trigger_work),
		  mtk_wifi_trigger_reset);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for
 *        1. deregister wifi reset callback
 *
 * @param none
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void glResetUninit(void)
{
	/* 1. Deregister reset callback */
	mtk_wcn_wmt_msgcb_unreg(WMTDRV_TYPE_WIFI);

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is invoked when there is reset messages indicated
 *
 * @param   eSrcType
 *          eDstType
 *          eMsgType
 *          prMsgBody
 *          u4MsgLength
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
static void *glResetCallback(enum ENUM_WMTDRV_TYPE eSrcType,
			     enum ENUM_WMTDRV_TYPE eDstType,
			     enum ENUM_WMTMSG_TYPE eMsgType, void *prMsgBody,
			     unsigned int u4MsgLength)
{
	switch (eMsgType) {
	case WMTMSG_TYPE_RESET:
		if (u4MsgLength == sizeof(enum ENUM_WMTRSTMSG_TYPE)) {
			enum ENUM_WMTRSTMSG_TYPE *prRstMsg =
					(enum ENUM_WMTRSTMSG_TYPE *) prMsgBody;

			switch (*prRstMsg) {
			case WMTRSTMSG_RESET_START:
				DBGLOG(INIT, WARN, "Whole chip reset start!\n");
				wifi_reset_start();
				fgIsResetting = TRUE;
				fgResetTriggered = FALSE;
				fgSimplifyResetFlow = TRUE;
				break;

			case WMTRSTMSG_RESET_END:
				DBGLOG(INIT, WARN, "Whole chip reset end!\n");
				wifi_rst.rst_data = RESET_SUCCESS;
				fgIsResetting = FALSE;
				schedule_work(&(wifi_rst.rst_work));
				break;

			case WMTRSTMSG_RESET_END_FAIL:
				DBGLOG(INIT, WARN, "Whole chip reset fail!\n");
				fgIsResetting = FALSE;
				wifi_rst.rst_data = RESET_FAIL;
				schedule_work(&(wifi_rst.rst_work));
				break;

			default:
				break;
			}
		}
		break;

	default:
		break;
	}

	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called for wifi reset
 *
 * @param   skb
 *          info
 *
 * @retval  0
 *          nonzero
 */
/*----------------------------------------------------------------------------*/
static void mtk_wifi_reset(struct work_struct *work)
{
	struct RESET_STRUCT *rst = container_of(work,
						struct RESET_STRUCT, rst_work);

	/* wlanOnAtReset(); */
	wifi_reset_end(rst->rst_data);
	fgSimplifyResetFlow = FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called for generating reset request to WMT
 *
 * @param   None
 *
 * @retval  None
 */
/*----------------------------------------------------------------------------*/
void glSendResetRequest(void)
{
	/* WMT thread would trigger whole chip reset itself */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called for checking if connectivity chip is resetting
 *
 * @param   None
 *
 * @retval  TRUE
 *          FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsResetting(void)
{
	return fgIsResetting || fgResetTriggered;
}

static void mtk_wifi_trigger_reset(struct work_struct *work)
{
	u_int8_t fgResult = FALSE;
	struct RESET_STRUCT *rst = container_of(work,
					struct RESET_STRUCT, rst_trigger_work);

	/* Set the power off flag to FALSE in WMT to prevent chip power off
	 * after wlanProbe return failure, because we need to do core dump
	 * afterward.
	 */
	if (rst->rst_trigger_flag & RST_FLAG_PREVENT_POWER_OFF)
		mtk_wcn_set_connsys_power_off_flag(FALSE);

	if (rst->rst_keyword == NULL) {
		DBGLOG(INIT, INFO, "keyword is null\n");
		fgResult = mtk_wcn_wmt_assert_timeout(WMTDRV_TYPE_WIFI, 40, 0);
	} else {
		DBGLOG(INIT, INFO, "reset keyword is %s\n",
			rst->rst_keyword);
		fgResult = mtk_wcn_wmt_assert_keyword(
			WMTDRV_TYPE_WIFI, rst->rst_keyword);
	}
	DBGLOG(INIT, INFO, "reset result %d, trigger flag 0x%x\n",
				fgResult, rst->rst_trigger_flag);
}

u_int8_t glResetTrigger(struct ADAPTER *prAdapter,
		uint32_t u4RstFlag, const uint8_t *pucFile, uint32_t u4Line,
		unsigned char *keyword)
{
	u_int8_t fgResult = TRUE;
	uint16_t u2FwOwnVersion;
	uint16_t u2FwPeerVersion;

	ASSERT(prAdapter);
	u2FwOwnVersion = prAdapter->rVerInfo.u2FwOwnVersion;
	u2FwPeerVersion = prAdapter->rVerInfo.u2FwPeerVersion;
	if (!kalIsResetting()) {
		DBGLOG(INIT, ERROR,
		       "Trigger chip reset in %s line %u! Chip[%04X E%u] FW Ver DEC[%u.%u] HEX[%x.%x], Driver Ver[%u.%u]\n",
		       pucFile, u4Line,
		       MTK_CHIP_REV,
		       wlanGetEcoVersion(prAdapter),
		       (uint16_t)(u2FwOwnVersion >> 8),
		       (uint16_t)(u2FwOwnVersion & BITS(0, 7)),
		       (uint16_t)(u2FwOwnVersion >> 8),
		       (uint16_t)(u2FwOwnVersion & BITS(0, 7)),
		       (uint16_t)(u2FwPeerVersion >> 8),
		       (uint16_t)(u2FwPeerVersion & BITS(0, 7)));

		fgResetTriggered = TRUE;
		prAdapter->u4HifDbgFlag |= DEG_HIF_ALL;
		halPrintHifDbgInfo(prAdapter);

		wifi_rst.rst_trigger_flag = u4RstFlag;
		wifi_rst.rst_keyword = keyword;
		schedule_work(&(wifi_rst.rst_trigger_work));
	}

	return fgResult;
}

#else

u_int8_t kalIsResetting(void)
{
	return FALSE;
}

#endif

enum _ENUM_CHIP_RESET_REASON_TYPE_T eResetReason;
uint64_t u8ResetTime;
void glGetRstReason(enum _ENUM_CHIP_RESET_REASON_TYPE_T
		    eReason)
{
	u8ResetTime = sched_clock();
	eResetReason = eReason;
}
