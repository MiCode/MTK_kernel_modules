/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/module.h>
#include "mtk_disp_msync.h"


static void __exit msync2_exit(void) {}

static int __init msync2_init(void)
{
	mtk_drm_get_target_fps_fp = mtk_drm_get_target_fps;
	fpsgo2msync_hint_frameinfo_fp = fpsgo2msync_hint_frameinfo;
	mtk_sync_te_level_decision_fp = mtk_sync_te_level_decision;
	mtk_sync_slow_descent_fp = mtk_sync_slow_descent;

	pr_debug("%s %d: finish", __func__, __LINE__);

	return 0;
}

module_init(msync2_init);
module_exit(msync2_exit);

MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek MSYNC2");
MODULE_AUTHOR("MediaTek Inc.");
