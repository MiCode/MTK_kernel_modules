/*
* Copyright Statement:
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*
* Copyright  (c) 2023 MediaTek Inc. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSESss
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
* The following software/firmware and/or related documentation ("MediaTek Software")
* have been modified by MediaTek Inc. All revisions are subject to any receiver's
* applicable license agreements with MediaTek Inc.
*/

#include <linux/module.h>
#include "mtk_em_ko.h"

#line __LINE__ "vendor/mediatek/kernel_modules/sched_int/src/mtk_em/mtk_em_main.c"

static void __exit mtk_em_exit(void) {}

static int __init mtk_em_init(void)
{
	int dpt_init_done = 0;
	long *data_malloc;
	struct em_base_info *mtk_em_base_info = NULL;
	mtk_em_base_info = mtk_get_em_base_info();

	if (!mtk_em_base_info)
		pr_info("%s mtk_get_em_base_info not init!\n", __func__);
	else {
		pr_info("%s usram_base: %p, csram_base: %p, wl_base: %p, eemsn_log: %p\n", __func__,
				mtk_em_base_info->usram_base, mtk_em_base_info->csram_base,
				mtk_em_base_info->wl_base, mtk_em_base_info->eemsn_log);
		pr_info("%s curve_adj_support: %d, wl_support: %d\n", __func__,
				mtk_em_base_info->curve_adj_support, mtk_em_base_info->wl_support);
		mtk_static_power_int_init(mtk_em_base_info);
	}

	if (mtk_em_base_info) {
		pr_info("%s %d: init_em_api_1st_stage", __func__, __LINE__);
		init_em_api_1st_stage(get_mtk_em_api_data());
		pr_info("%s %d: data_malloc_request", __func__, __LINE__);
		data_malloc = data_malloc_request();
		pr_info("%s %d: mtk_em_malloc", __func__, __LINE__);
		mtk_em_malloc(data_malloc);
		pr_info("%s %d: data_malloc_assign", __func__, __LINE__);
		data_malloc_assign();
		pr_info("%s %d: init_em_api_2nd_stage", __func__, __LINE__);
		init_em_api_2nd_stage();
		pr_info("%s %d: init_mtk_em_hook", __func__, __LINE__);
		init_dpt(get_dpt_sram_base(), &dpt_init_done);
		pr_info("%s %d: dpt done", __func__, __LINE__);
		init_mtk_em_hook(dpt_init_done);
		pr_info("%s %d: em hook done", __func__, __LINE__);
	}

	return 0;
}

module_init(mtk_em_init);
module_exit(mtk_em_exit);

MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek mtk_em");
MODULE_AUTHOR("MediaTek Inc.");