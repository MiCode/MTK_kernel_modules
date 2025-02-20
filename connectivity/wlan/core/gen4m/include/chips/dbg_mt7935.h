/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _DBG_MT7935_H
#define _DBG_MT7935_H

#include "coda/mt7935/cb_infra_misc0.h"
#include "coda/mt7935/cb_infra_slp_ctrl.h"
#include "coda/mt7935/cb_infra_rgu.h"
#include "coda/mt7935/conn_bus_cr_on.h"
#include "coda/mt7935/conn_cfg.h"
#include "coda/mt7935/conn_dbg_ctl.h"
#include "coda/mt7935/conn_host_csr_top.h"
#include "coda/mt7935/conn_mcu_bus_cr.h"
#include "coda/mt7935/conn_mcu_confg_ls.h"
#include "coda/mt7935/top_misc.h"
#include "coda/mt7935/wf_mcusys_vdnr_gen_bus_u_debug_ctrl_ao.h"
#include "coda/mt7935/wf_top_cfg_on.h"
#include "coda/mt7935/conn_bus_cr.h"
#include "coda/mt7935/conn_cfg_on.h"
#include "coda/mt7935/conn_clkgen_top.h"
#include "coda/mt7935/conn_gpio.h"
#include "coda/mt7935/conn_rf_spi_mst_reg.h"
#include "coda/mt7935/conn_rgu_on.h"
#include "coda/mt7935/conn_wt_slp_ctl_reg.h"

struct dump_cr_set {
	u_int8_t read;
	uint32_t addr;
	uint32_t mask;
	uint32_t shift;
	uint32_t value;
};

#if defined(_HIF_PCIE)
u_int8_t mt7935_pcie_show_mcu_debug_info(struct ADAPTER *ad,
	uint8_t *pucBuf, uint32_t u4Max, uint8_t ucFlag,
	uint32_t *pu4Length);
#elif defined(_HIF_USB)
u_int8_t mt7935_usb_show_mcu_debug_info(struct ADAPTER *ad,
	uint8_t *pucBuf, uint32_t u4Max, uint8_t ucFlag,
	uint32_t *pu4Length);
#endif

#endif
