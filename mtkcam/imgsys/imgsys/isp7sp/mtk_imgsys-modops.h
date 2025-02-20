/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *         Holmes Chiou <holmes.chiou@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_MODOPS_H_
#define _MTK_IMGSYS_MODOPS_H_

#include "mtk_imgsys-module.h"
#include "imgsys_modules/mtk_imgsys-dip.h"
#include "imgsys_modules/mtk_imgsys-traw.h"
#include "imgsys_modules/mtk_imgsys-pqdip.h"
#include "imgsys_modules/mtk_imgsys-wpe.h"
#include "imgsys_modules/mtk_imgsys-me.h"
#include "imgsys_modules/mtk_imgsys-adl.h"
//#include "mtk-ipesys-me.h"
#include "mtk_imgsys-debug.h"

const struct module_ops imgsys_isp7_modules[] = {
	[IMGSYS_MOD_TRAW] = {
		.module_id = IMGSYS_MOD_TRAW,
		.init = imgsys_traw_set_initial_value,
		.set = imgsys_traw_set_initial_value_hw,
		.updatecq = imgsys_traw_updatecq,
		.cmdq_set = imgsys_traw_cmdq_set_initial_value_hw,
		.dump = imgsys_traw_debug_dump,
		.done_chk = imgsys_traw_done_chk,
		.uninit = imgsys_traw_uninit,
	},
	[IMGSYS_MOD_LTRAW] = {
		.module_id = IMGSYS_MOD_LTRAW,
		.init = imgsys_traw_set_initial_value,
		.set = imgsys_ltraw_set_initial_value_hw,
		.updatecq = NULL,
		.cmdq_set = imgsys_ltraw_cmdq_set_initial_value_hw,
		.dump = imgsys_traw_debug_dump,
		.done_chk = imgsys_traw_done_chk,
		.uninit = imgsys_traw_uninit,
	},
	[IMGSYS_MOD_DIP] = {
		.module_id = IMGSYS_MOD_DIP,
		.init = imgsys_dip_set_initial_value,
		.set = imgsys_dip_set_hw_initial_value,
		.updatecq = imgsys_dip_updatecq,
		.cmdq_set = imgsys_dip_cmdq_set_hw_initial_value,
		.dump = imgsys_dip_debug_dump,
		.done_chk = imgsys_dip_done_chk,
		.uninit = imgsys_dip_uninit,
	},
	[IMGSYS_MOD_PQDIP] = {
		.module_id = IMGSYS_MOD_PQDIP,
		.init = imgsys_pqdip_set_initial_value,
		.set = imgsys_pqdip_set_hw_initial_value,
		.updatecq = imgsys_pqdip_updatecq,
		.cmdq_set = imgsys_pqdip_cmdq_set_hw_initial_value,
		.dump = imgsys_pqdip_debug_dump,
		.done_chk = imgsys_pqdip_done_chk,
		.uninit = imgsys_pqdip_uninit,
	},
	[IMGSYS_MOD_ME] = {
		.module_id = IMGSYS_MOD_ME,
		.init = imgsys_me_set_initial_value,
		.set = ME_mode3_reset,
		.updatecq = NULL,
		.cmdq_set = NULL,
		.dump = imgsys_me_debug_dump,
		.done_chk = imgsys_me_done_chk,
		.uninit = imgsys_me_uninit,
	},
	[IMGSYS_MOD_WPE] = {
		.module_id = IMGSYS_MOD_WPE,
		.init = imgsys_wpe_set_initial_value,
		.set = imgsys_wpe_set_hw_initial_value,
		.updatecq = imgsys_wpe_updatecq,
		.cmdq_set = imgsys_wpe_cmdq_set_hw_initial_value,
		.dump = imgsys_wpe_debug_dump,
		.done_chk = imgsys_wpe_done_chk,
		.uninit = imgsys_wpe_uninit,
	},
	[IMGSYS_MOD_ADL] = {
		.module_id = IMGSYS_MOD_ADL,
		.init = imgsys_adl_init,
		.set = imgsys_adl_set,
		.updatecq = NULL,
		.cmdq_set = NULL,
		.dump = imgsys_adl_debug_dump,
		.done_chk = NULL,
		.uninit = imgsys_adl_uninit,
	},
	/*pure sw usage for timeout debug dump*/
	[IMGSYS_MOD_IMGMAIN] = {
		.module_id = IMGSYS_MOD_IMGMAIN,
		.init = imgsys_main_init,
		.set = imgsys_main_set_init,
		.updatecq = NULL,
		.cmdq_set = imgsys_main_cmdq_set_init,
		.dump = NULL,
		.done_chk = NULL,
		.uninit = imgsys_main_uninit,
	},
};
#define MTK_IMGSYS_MODULE_NUM	ARRAY_SIZE(imgsys_isp7_modules)


#endif /* _MTK_IMGSYS_MODOPS_H_ */

