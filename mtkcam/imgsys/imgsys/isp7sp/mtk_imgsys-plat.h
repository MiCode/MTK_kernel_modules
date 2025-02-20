/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Christopher Chen <christopher.chen@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_PLAT_H_
#define _MTK_IMGSYS_PLAT_H_

#include <linux/clk.h>

struct clk_bulk_data imgsys_isp7_clks_mt6897[] = {
	{
		.id = "IMGSYS_CG_IMG_TRAW0",
	},
	{
		.id = "IMGSYS_CG_IMG_TRAW1",
	},
	{
		.id = "IMGSYS_CG_IMG_DIP0",
	},
	{
		.id = "IMGSYS_CG_IMG_WPE0",
	},
	{
		.id = "IMGSYS_CG_IMG_WPE1",
	},
	{
		.id = "IMGSYS_CG_IMG_WPE2",
	},
	{
		.id = "IMGSYS_CG_IMG_AVS",
	},
	{
		.id = "IMGSYS_CG_IMG_IPS",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON0",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON1",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON2",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON3",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON4",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_DIP0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_DIP1",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_TRAW0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_WPE0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_WPE1",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_WPE2",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_IPE0",
	},
	{
		.id = "IMGSYS_CG_GALS_TX_IPE0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_IPE1",
	},
	{
		.id = "IMGSYS_CG_GALS_TX_IPE1",
	},
	{
		.id = "IMGSYS_CG_IMG_GALS",
	},
	{
		.id = "DIP_TOP_DIP_TOP",
	},
	{
		.id = "DIP_TOP_DIP_TOP_GALS0",
	},
	{
		.id = "DIP_TOP_DIP_TOP_GALS1",
	},
	{
		.id = "DIP_TOP_DIP_TOP_GALS2",
	},
	{
		.id = "DIP_TOP_DIP_TOP_GALS3",
	},
	{
		.id = "DIP_NR1_DIP1_LARB",
	},
	{
		.id = "DIP_NR1_DIP_NR1",
	},
	{
		.id = "DIP_NR2_DIP_NR",
	},
	{
		.id = "WPE1_CG_DIP1_WPE",
	},
	{
		.id = "WPE1_CG_DIP1_GALS0",
	},
	{
		.id = "WPE2_CG_DIP1_WPE",
	},
	{
		.id = "WPE2_CG_DIP1_GALS0",
	},
	{
		.id = "WPE3_CG_DIP1_WPE",
	},
	{
		.id = "WPE3_CG_DIP1_GALS0",
	},
	{
		.id = "TRAW_CG_DIP1_TRAW",
	},
	{
		.id = "TRAW_CG_DIP1_GALS",
	},
	{
		.id = "IMGSYS_CG_IMG_IPE"
	},
	{
		.id = "ME_CG"
	},
	{
		.id = "MMG_CG"
	}
};

#define MTK_IMGSYS_CLK_NUM_MT6897	ARRAY_SIZE(imgsys_isp7_clks_mt6897)

struct clk_bulk_data imgsys_isp7_clks_mt6989[] = {
	{
		.id = "IMGSYS_CG_IMG_TRAW0",
	},
	{
		.id = "IMGSYS_CG_IMG_TRAW1",
	},
	{
		.id = "IMGSYS_CG_IMG_DIP0",
	},
	{
		.id = "IMGSYS_CG_IMG_WPE0",
	},
	{
		.id = "IMGSYS_CG_IMG_WPE1",
	},
	{
		.id = "IMGSYS_CG_IMG_WPE2",
	},
	{
		.id = "IMGSYS_CG_IMG_AVS",
	},
	{
		.id = "IMGSYS_CG_IMG_IPS",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON0",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON1",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON2",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON3",
	},
	{
		.id = "IMGSYS_CG_SUB_COMMON4",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_DIP0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_DIP1",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_TRAW0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_WPE0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_WPE1",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_WPE2",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_IPE0",
	},
	{
		.id = "IMGSYS_CG_GALS_TX_IPE0",
	},
	{
		.id = "IMGSYS_CG_GALS_RX_IPE1",
	},
	{
		.id = "IMGSYS_CG_GALS_TX_IPE1",
	},
	{
		.id = "IMGSYS_CG_IMG_GALS",
	},
	{
		.id = "IMGSYS_CG_IMG_IPE"
	},
	{
		.id = "ME_CG"
	},
	{
		.id = "MMG_CG"
	}
};

#define MTK_IMGSYS_CLK_NUM_MT6989	ARRAY_SIZE(imgsys_isp7_clks_mt6989)

#endif /* _MTK_IMGSYS_PLAT_H_ */
