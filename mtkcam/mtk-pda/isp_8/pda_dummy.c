// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/jiffies.h>

#include <linux/time.h>		//do_gettimeofday()

#include "mtk-interconnect.h"

#include "camera_pda.h"

// --------- define region --------
#define PDA_MMQOS
// --------------------------------

#define PDA_DEV_NAME "camera-pda"

// debug command
static int32_t pda_log_dbg_en;

/*******************************************************************************
 *                               Internal function
 ******************************************************************************/
void pda_debug_log(int32_t debug_log_en)
{
	pda_log_dbg_en = debug_log_en;
}

struct device *init_larb(struct platform_device *pdev, int idx)
{
	return NULL;
}

/*******************************************************************************
 *                                     API
 ******************************************************************************/
#ifdef PDA_MMQOS
void pda_mmqos_init(struct device *pdev)
{
}

void pda_mmqos_bw_set(struct PDA_Data_t *pda_Pdadata)
{
}

void pda_mmqos_bw_reset(void)
{
}
#endif

void pda_init_larb(struct platform_device *pdev)
{
}

int pda_devm_clk_get(struct platform_device *pdev)
{
	return 0;
}

void pda_clk_prepare_enable(void)
{
}

void pda_clk_disable_unprepare(void)
{
}

void __iomem *pda_get_camsys_address(void)
{
	return NULL;
}

unsigned int GetResetBitMask(int PDA_Index)
{
	return 0;
}
