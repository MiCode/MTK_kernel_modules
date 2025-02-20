/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   hal_wfsys_reset_mt6639.h
*    \brief  WFSYS reset HAL API for MT6639
*
*    This file contains all routines which are exported
     from MediaTek 802.11 Wireless LAN driver stack to GLUE Layer.
*/

#ifndef _HAL_WFSYS_RESET_MT6639_H
#define _HAL_WFSYS_RESET_MT6639_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

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
#if defined(_HIF_USB)
/* Accessing conn_sys CR by UHW needs remap CR address to conn_infra view */
#define WF_TOP_CFG_ON_ROMCODE_INDEX_REMAP_ADDR 0x184C1604
#endif

#if defined(_HIF_PCIE)
#define CONN_INFRA_BUS_CR_PCIE2AP_REMAP_WF_0_54_DEFAULT 0x18451844
#define CONN_INFRA_BUS_CR_PCIE2AP_REMAP_WF_0_54_ADDR 0x7c021008
#define CBTOP_RGU_BASE 0x70028600
#define R_PCIE2AP_PUBLIC_REMAPPING_4_BUS_ADDR 0x40000

#define WF_TOP_CFG_ON_ROMCODE_INDEX_REMAP_ADDR 0x81021604
#endif

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

u_int8_t mt6639HalCbInfraRguWfRst(struct ADAPTER *prAdapter,
				u_int8_t fgAssertRst);

u_int8_t mt6639HalPollWfsysSwInitDone(struct ADAPTER *prAdapter);

#if defined(_HIF_PCIE)
void mt6639GetSemaphore(struct ADAPTER *prAdapter);
void mt6639GetSemaReport(struct ADAPTER *prAdapter);
#endif /* defined(_HIF_PCIE) */

#if defined(_HIF_USB)

u_int8_t mt6639HalUsbEpctlRstOpt(struct ADAPTER *prAdapter,
				 u_int8_t fgIsRstScopeIncludeToggleBit);

#endif /* defined(_HIF_USB) */

#if defined(_HIF_SDIO)

#endif

#endif /* _HAL_WFSYS_RESET_MT6639_H */
