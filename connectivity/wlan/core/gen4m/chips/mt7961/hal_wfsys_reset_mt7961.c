/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2019 MediaTek Inc.
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
 * Copyright(C) 2019 MediaTek Inc. All rights reserved.
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
/*! \file   hal_wfsys_reset_mt7961.c
*    \brief  WFSYS reset HAL API for MT7961
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT7961

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"
#include "mt7961.h"
#include "coda/mt7961/cbtop_rgu.h"
#include "coda/mt7961/ssusb_epctl_csr.h"

#if CFG_CHIP_RESET_SUPPORT
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#if defined(_HIF_PCIE)

/*----------------------------------------------------------------------------*/
/*!
* @brief assert or de-assert WF subsys reset on CBTOP RGU
*
* @param prAdapter pointer to the Adapter handler
* @param fgAssertRst TRUE means assert reset and FALSE means de-assert reset
*
* @return TRUE means operation successfully and FALSE means fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt7961HalCbtopRguWfRst(struct ADAPTER *prAdapter,
				u_int8_t fgAssertRst)
{
	/* TODO */

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief polling if WF subsys is SW_INIT_DONE
*
* @param prAdapter pointer to the Adapter handler
*
* @return TRUE means WF subsys is SW_INIT_DONE and ready for driver probe
*         procedure; otherwise, return FALSE
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt7961HalPollWfsysSwInitDone(struct ADAPTER *prAdapter)
{
	/* TODO */

	return TRUE;
}

#endif /* defined(_HIF_PCIE) */

#if defined(_HIF_USB)

/*----------------------------------------------------------------------------*/
/*!
* @brief assert or de-assert WF subsys reset on CBTOP RGU
*
* @param prAdapter pointer to the Adapter handler
* @param fgAssertRst TRUE means assert reset and FALSE means de-assert reset
*
* @return TRUE means operation successfully and FALSE means fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt7961HalCbtopRguWfRst(struct ADAPTER *prAdapter,
				u_int8_t fgAssertRst)
{
	uint32_t u4CrVal;
	u_int8_t fgStatus;

	HAL_UHW_RD(prAdapter, CBTOP_RGU_WF_SUBSYS_RST_ADDR, &u4CrVal,
		   &fgStatus);
	if (!fgStatus) {
		DBGLOG(HAL, ERROR, "UHW read CBTOP RGU CR fail\n");

		goto end;
	}

	if (fgAssertRst) {
		/* CBTOP_RGU_WF_SUBSYS_RST_BYPASS_WFDMA_SLP_PROT_MASK is defined
		 * since MT7922. For prior project like MT7961, it's undefined
		 * and doesn't have any effect if this bit is set.
		 */
		u4CrVal |= CBTOP_RGU_WF_SUBSYS_RST_BYPASS_WFDMA_SLP_PROT_MASK;
		u4CrVal |= CBTOP_RGU_WF_SUBSYS_RST_WF_WHOLE_PATH_RST_MASK;
		HAL_UHW_WR(prAdapter, CBTOP_RGU_WF_SUBSYS_RST_ADDR, u4CrVal,
			   &fgStatus);
	} else {
		u4CrVal &= ~CBTOP_RGU_WF_SUBSYS_RST_WF_WHOLE_PATH_RST_MASK;
		HAL_UHW_WR(prAdapter, CBTOP_RGU_WF_SUBSYS_RST_ADDR, u4CrVal,
			   &fgStatus);
	}

	if (!fgStatus) {
		DBGLOG(HAL, ERROR, "UHW write CBTOP RGU CR fail\n");

		goto end;
	}

end:
	return fgStatus;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief set USB endpoint reset scope
*
* @param prAdapter pointer to the Adapter handler
* @param fgIsRstScopeIncludeToggleBit TRUE means reset scope including toggle
*                                     bit, sequence number, etc
*                                     FALSE means exclusion
*
* @return TRUE means operation successfully and FALSE means fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt7961HalUsbEpctlRstOpt(struct ADAPTER *prAdapter,
			     u_int8_t fgIsRstScopeIncludeToggleBit)
{
	uint32_t u4CrVal;
	u_int8_t fgStatus;

	HAL_UHW_RD(prAdapter, SSUSB_EPCTL_CSR_EP_RST_OPT_ADDR, &u4CrVal,
		   &fgStatus);

	if (!fgStatus) {
		DBGLOG(HAL, ERROR, "UHW read USB CR fail\n");

		return fgStatus;
	}

	if (fgIsRstScopeIncludeToggleBit) {
		/* BITS(4, 9) represents BULK OUT EP4 ~ EP9
		 * BITS(20, 22) represents BULK IN EP4 ~ EP5, INT IN EP6
		 */
		u4CrVal |= (BITS(4, 9) | BITS(20, 22));
		HAL_UHW_WR(prAdapter, SSUSB_EPCTL_CSR_EP_RST_OPT_ADDR, u4CrVal,
			   &fgStatus);
	} else {
		/* BITS(4, 9) represents BULK OUT EP4 ~ EP9
		 * BITS(20, 22) represents BULK IN EP4 ~ EP5, INT IN EP6
		 */
		u4CrVal &= ~(BITS(4, 9) | BITS(20, 22));
		HAL_UHW_WR(prAdapter, SSUSB_EPCTL_CSR_EP_RST_OPT_ADDR, u4CrVal,
			   &fgStatus);
	}

	if (!fgStatus)
		DBGLOG(HAL, ERROR, "UHW write USB CR fail\n");

	return fgStatus;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief polling if WF subsys is SW_INIT_DONE
*
* @param prAdapter pointer to the Adapter handler
*
* @return TRUE means WF subsys is SW_INIT_DONE and ready for driver probe
*         procedure; otherwise, return FALSE
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt7961HalPollWfsysSwInitDone(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo;
	uint32_t u4CrValue;
	uint32_t u4ResetTimeCnt = 0, u4ResetTimeTmout = 2;
	u_int8_t fgStatus;
	u_int8_t fgSwInitDone = TRUE;

#define CONN_INFRA_SYSSTRAP_WFSYS_SW_INIT_DONE_MASK        0x00400000

	prBusInfo = prAdapter->chip_info->bus_info;

	/* select conn_infra sysstrap */
	HAL_UHW_WR(prAdapter, prBusInfo->u4UdmaConnInfraStatusSelAddr,
		   0x00000000, &fgStatus);

	if (!fgStatus)
		DBGLOG(HAL, ERROR, "UHW write UDMA CR fail\n");

	/* polling until WF WM is ready */
	u4CrValue = 0;
	while (TRUE) {
		HAL_UHW_RD(prAdapter, prBusInfo->u4UdmaConnInfraStatusAddr,
			   &u4CrValue, &fgStatus);

		if (!fgStatus)
			DBGLOG(HAL, ERROR, "UHW read UDMA CR fail\n");
		else if (u4CrValue &
			 CONN_INFRA_SYSSTRAP_WFSYS_SW_INIT_DONE_MASK)
			break;

		if (u4ResetTimeCnt >= u4ResetTimeTmout) {
			DBGLOG(INIT, ERROR,
			       "L0.5 Reset polling sw init done timeout\n");

			fgSwInitDone = FALSE;

			break;
		}

		kalMsleep(100);
		u4ResetTimeCnt++;
	}

	return fgSwInitDone;
}

#endif /* defined(_HIF_USB) */

#if defined(_HIF_SDIO)

/*----------------------------------------------------------------------------*/
/*!
* @brief assert or de-assert WF subsys reset on CBTOP RGU
*
* @param prAdapter pointer to the Adapter handler
* @param fgAssertRst TRUE means assert reset and FALSE means de-assert reset
*
* @return TRUE means operation successfully and FALSE means fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt7961HalCbtopRguWfRst(struct ADAPTER *prAdapter,
				u_int8_t fgAssertRst)
{
	uint32_t u4Val = 0;

	HAL_MCR_RD(prAdapter, MCR_WHCR, &u4Val);

	if (fgAssertRst) {
		/* trigger rst */
		u4Val &= ~WHCR_WF_WHOLE_PATH_RSTB;
		HAL_MCR_WR(prAdapter, MCR_WHCR, u4Val);

	} else {
		/* clear rst */
		u4Val |= WHCR_WF_WHOLE_PATH_RSTB;
		HAL_MCR_WR(prAdapter, MCR_WHCR, u4Val);
	}

	if (fgIsBusAccessFailed == TRUE) {
		DBGLOG(INIT, ERROR,
			"[SER][L0.5] fgIsBusAccessFailed=%d fgAssertRst=%d\n",
				fgIsBusAccessFailed, fgAssertRst);
		return FALSE;
	} else
		return TRUE;

}

/*----------------------------------------------------------------------------*/
/*!
* @brief polling if WF subsys is SW_INIT_DONE
*
* @param prAdapter pointer to the Adapter handler
*
* @return TRUE means WF subsys is SW_INIT_DONE and ready for driver probe
*         procedure; otherwise, return FALSE
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt7961HalPollWfsysSwInitDone(struct ADAPTER *prAdapter)
{
#define SER_L05_RST_DONE_TIMEOUT        (2000)

	uint32_t u4CrValue1, u4Tick;
	u_int8_t fgSwInitDone = TRUE;


	HAL_MCR_RD(prAdapter, MCR_WHCR, &u4CrValue1);
	/* polling until WF WM is ready */
	u4Tick = kalGetTimeTick();
	while ((u4CrValue1 & WHCR_WF_RST_DONE) == FALSE) {
		kalMsleep(50);
		HAL_MCR_RD(prAdapter, MCR_WHCR, &u4CrValue1);

		/* timeout handle */
		if ((CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Tick,
				MSEC_TO_SYSTIME(SER_L05_RST_DONE_TIMEOUT)))) {
			fgSwInitDone = FALSE;
			DBGLOG(INIT, ERROR,
				"[SER] L05 - polling sw init done timeout\n");
			break;
		}
	}

	return fgSwInitDone;
}

#endif /* defined(_HIF_SDIO) */

#endif /* CFG_CHIP_RESET_SUPPORT */
#endif /* MT7961 */
