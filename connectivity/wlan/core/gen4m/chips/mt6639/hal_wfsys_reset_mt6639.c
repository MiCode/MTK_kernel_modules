// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hal_wfsys_reset_mt6639.c
*    \brief  WFSYS reset HAL API for MT6639
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifdef MT6639

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"
#include "mt6639.h"
#include "coda/mt6639/ssusb_epctl_csr.h"
#include "hal_wfsys_reset_mt6639.h"
#include "coda/mt6639/cb_infra_rgu.h"
#include "coda/mt6639/wf_top_cfg_on.h"
#include "coda/mt6639/wf_wfdma_host_dma0.h"

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
* @brief assert or de-assert WF subsys reset on CB_INFRA RGU
*
* @param prAdapter pointer to the Adapter handler
* @param fgAssertRst TRUE means assert reset and FALSE means de-assert reset
*
* @return TRUE means operation successfully and FALSE means fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt6639HalCbInfraRguWfRst(struct ADAPTER *prAdapter,
				u_int8_t fgAssertRst)
{
	uint32_t u4AddrVal;
	uint32_t u4CrVal = 0;

	if (fgAssertRst) {
		mt6639GetSemaphore(prAdapter);
		/*A.1*/
		u4AddrVal = CBTOP_RGU_BASE;
		/* Falcon can't reset with WF_WHOLE_PATH_RST[0],
		 * reset with WF_SUBSYS_RST[4].
		 */
		HAL_MCR_RD(prAdapter, u4AddrVal, &u4CrVal);
		DBGLOG(HAL, DEBUG, "Read cr_bus_rst 0x%x: 0x%x\n",
			u4AddrVal, u4CrVal);
		u4CrVal |= BIT(4);
		HAL_MCR_WR(prAdapter, u4AddrVal, u4CrVal);
		DBGLOG(HAL, DEBUG,
			"A.1 - Write assert cr_bus_rst 0x%x[4]: 0x%x\n",
			u4AddrVal, u4CrVal);
	} else {
		/*e.1*/
		u4AddrVal = CBTOP_RGU_BASE;
		HAL_MCR_RD(prAdapter, u4AddrVal, &u4CrVal);
		DBGLOG(HAL, DEBUG, "E. Read 0x%x: 0x%x\n", u4AddrVal, u4CrVal);

		u4CrVal &= ~BIT(4);

		HAL_MCR_WR(prAdapter, u4AddrVal, u4CrVal);
	}

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
u_int8_t mt6639HalPollWfsysSwInitDone(struct ADAPTER *prAdapter)
{
	uint32_t u4CrValue = 0;
	uint32_t u4ResetTimeCnt = 0, u4ResetTimeTmout = 2;
	u_int8_t fgSwInitDone = TRUE;

#define MMIO_READ_FAIL	0xFFFFFFFF
#define MCU_IDLE	0x1D1E

	/* Polling until WF WM is done */
	while (TRUE) {
		HAL_MCR_RD(prAdapter, WF_TOP_CFG_ON_ROMCODE_INDEX_REMAP_ADDR,
			&u4CrValue);
		DBGLOG(HAL, DEBUG, "Poll ROM_INDEX, 0X81021604: 0x%x\n",
			u4CrValue);
		if (u4CrValue == MMIO_READ_FAIL)
			DBGLOG(HAL, ERROR, "[SER][L0.5] MMIO read CR fail\n");
		else if (u4CrValue == MCU_IDLE)
			break;

		if (u4ResetTimeCnt >= u4ResetTimeTmout) {
			DBGLOG(INIT, ERROR,
				"[SER][L0.5] Poll Sw Init Done FAIL\n");
			fgSwInitDone = FALSE;
			break;
		}

		kalMsleep(100);
		u4ResetTimeCnt++;
	}

	mt6639GetSemaReport(prAdapter);
	return fgSwInitDone;
}

void mt6639GetSemaphore(struct ADAPTER *prAdapter)
{
	uint32_t u4RemapVal = 0;
	uint32_t u4Val;

	HAL_MCR_RD(prAdapter, CONN_INFRA_BUS_CR_PCIE2AP_REMAP_WF_0_54_ADDR,
		&u4RemapVal);
	/* Set PCIE2AP public mapping CR4 */
	u4Val = (u4RemapVal & ~BITS(0, 15)) | (0x00001807);
	HAL_MCR_WR(prAdapter, CONN_INFRA_BUS_CR_PCIE2AP_REMAP_WF_0_54_ADDR,
		u4Val);
	DBGLOG(HAL, DEBUG, "Write Remap val: 0x%x\n", u4Val);
	kalUdelay(10);

	HAL_MCR_RD(prAdapter, 0x40000, &u4Val);
	DBGLOG(HAL, DEBUG, "Read CONN_SEMA00_M0_OWN_STA: 0x%x\n",
		u4Val);

	HAL_MCR_WR(prAdapter, CONN_INFRA_BUS_CR_PCIE2AP_REMAP_WF_0_54_ADDR,
		u4RemapVal);
	DBGLOG(HAL, DEBUG, "Write back default Remap val: 0x%x\n", u4RemapVal);
	kalUdelay(10);
}

void mt6639GetSemaReport(struct ADAPTER *prAdapter)
{
	uint32_t u4RemapVal = 0;
	uint32_t u4Val;

	/* 0x18070400 */
	HAL_MCR_RD(prAdapter, CONN_INFRA_BUS_CR_PCIE2AP_REMAP_WF_0_54_ADDR,
		&u4RemapVal);

	/* Set PCIE2AP public mapping CR4 */
	u4Val = (u4RemapVal & ~BITS(0, 15)) | (0x00001807);

	DBGLOG(HAL, DEBUG, "Write Remap val: 0x%x\n", u4Val);

	kalUdelay(10);

	HAL_MCR_RD(prAdapter, 0x40400, &u4Val);
	DBGLOG(HAL, DEBUG, "Read CONN_SEMA_OWN_BY_M0_STA_REP_1: 0x%x\n",
		u4Val);

	HAL_MCR_WR(prAdapter, CONN_INFRA_BUS_CR_PCIE2AP_REMAP_WF_0_54_ADDR,
		u4RemapVal);
	DBGLOG(HAL, DEBUG, "Write back default Remap val: 0x%x\n", u4RemapVal);
	kalUdelay(10);
}
#endif /* defined(_HIF_PCIE) */

#if defined(_HIF_USB)
/*----------------------------------------------------------------------------*/
/*!
* @brief assert or de-assert WF subsys reset on CB_INFRA RGU
*
* @param prAdapter pointer to the Adapter handler
* @param fgAssertRst TRUE means assert reset and FALSE means de-assert reset
*
* @return TRUE means operation successfully and FALSE means fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt6639HalCbInfraRguWfRst(struct ADAPTER *prAdapter,
				u_int8_t fgAssertRst)
{
	uint32_t u4CrVal;
	u_int8_t fgStatus;

	HAL_UHW_RD(prAdapter, CB_INFRA_RGU_WF_SUBSYS_RST_ADDR, &u4CrVal,
		&fgStatus);
	if (!fgStatus) {
		DBGLOG(HAL, ERROR, "UHW read CBTOP RGU CR fail\n");

		goto end;
	}
	DBGLOG(HAL, DEBUG, "UHW read SUBSYS_RST_CR: 0x%x\n", u4CrVal);

	if (fgAssertRst) {
		/* CBTOP_RGU_WF_SUBSYS_RST_BYPASS_WFDMA_SLP_PROT_MASK is defined
		 * since MT7922. For prior project like MT7961, it's undefined
		 * and doesn't have any effect if this bit is set.
		 */

		/* Check connac3 is need? */
#if 0
		u4CrVal |=
		CB_INFRA_RGU_WF_SUBSYS_RST_BYPASS_WFDMA_2_SLP_PROT_MASK;
		u4CrVal |=
		CB_INFRA_RGU_WF_SUBSYS_RST_BYPASS_WFDMA_SLP_PROT_MASK;
		HAL_UHW_WR(prAdapter,
			CB_INFRA_RGU_WF_SUBSYS_RST_ADDR, u4CrVal,
			&fgStatus);
#endif

		u4CrVal |= CB_INFRA_RGU_WF_SUBSYS_RST_WF_WHOLE_PATH_RST_MASK;
		HAL_UHW_WR(prAdapter, CB_INFRA_RGU_WF_SUBSYS_RST_ADDR, u4CrVal,
			&fgStatus);
	} else {
		u4CrVal &= ~CB_INFRA_RGU_WF_SUBSYS_RST_WF_WHOLE_PATH_RST_MASK;
		HAL_UHW_WR(prAdapter, CB_INFRA_RGU_WF_SUBSYS_RST_ADDR, u4CrVal,
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
u_int8_t mt6639HalUsbEpctlRstOpt(struct ADAPTER *prAdapter,
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
u_int8_t mt6639HalPollWfsysSwInitDone(struct ADAPTER *prAdapter)
{
	uint32_t u4CrValue = 0;
	uint32_t u4ResetTimeCnt = 0, u4ResetTimeTmout = 2;
	u_int8_t fgStatus;
	u_int8_t fgSwInitDone = TRUE;

#define MCU_IDLE		0x1D1E

	/* polling until WF WM is ready */
	while (TRUE) {
		HAL_UHW_RD(prAdapter, WF_TOP_CFG_ON_ROMCODE_INDEX_REMAP_ADDR,
			&u4CrValue, &fgStatus);

		if (!fgStatus)
			DBGLOG(HAL, ERROR, "UHW read WF_TOP_CFG_ON CR fail\n");
		else if (u4CrValue == MCU_IDLE) {
			DBGLOG(HAL, DEBUG,
				"UHW read WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR: 0x%x\n",
				u4CrValue);
			break;
		}

		DBGLOG(HAL, DEBUG,
			"UHW read WF_TOP_CFG_ON_ROMCODE_INDEX_ADDR: 0x%x\n",
			u4CrValue);
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
* @brief assert or de-assert WF subsys reset on CB_INFRA RGU
*
* @param prAdapter pointer to the Adapter handler
* @param fgAssertRst TRUE means assert reset and FALSE means de-assert reset
*
* @return TRUE means operation successfully and FALSE means fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t mt6639HalCbInfraRguWfRst(struct ADAPTER *prAdapter,
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
u_int8_t mt6639HalPollWfsysSwInitDone(struct ADAPTER *prAdapter)
{
	/* TODO */

	return TRUE;
}

#endif /* defined(_HIF_SDIO) */

#endif /* CFG_CHIP_RESET_SUPPORT */
#endif /* MT6639 */
