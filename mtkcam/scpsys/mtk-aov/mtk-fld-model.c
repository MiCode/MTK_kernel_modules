// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include "mtk-fld-model.h"

#if FLD_SMALL_MODEL
#include "./fldmodel_small/all_header.h"
#if FLD_UT
#include "./fld_pattern_small/fld_pattern_header.h"
#endif	/* #endif FLD_UT */
#else
#include "./fldmodel/all_header.h"
#if FLD_UT
#include "./fld_pattern/fld_pattern_header.h"
#endif	/* #endif FLD_UT */
#endif	/* #endif FLD_SMALL_MODEL */

void mtk_fld_aov_memcpy(char *buffer)
{
	char *tmp = buffer;

	memcpy(tmp, &fdvt_fld_blink_weight_forest14[0],
		fdvt_fld_blink_weight_forest14_size);
	tmp += fdvt_fld_blink_weight_forest14_size;
	memcpy(tmp, &fdvt_fld_tree_forest00_cv_weight[0],
		fdvt_fld_tree_forest00_cv_weight_size);
	tmp += fdvt_fld_tree_forest00_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest01_cv_weight[0],
		fdvt_fld_tree_forest01_cv_weight_size);
	tmp += fdvt_fld_tree_forest01_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest02_cv_weight[0],
		fdvt_fld_tree_forest02_cv_weight_size);
	tmp += fdvt_fld_tree_forest02_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest03_cv_weight[0],
		fdvt_fld_tree_forest03_cv_weight_size);
	tmp += fdvt_fld_tree_forest03_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest04_cv_weight[0],
		fdvt_fld_tree_forest04_cv_weight_size);
	tmp += fdvt_fld_tree_forest04_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest05_cv_weight[0],
		fdvt_fld_tree_forest05_cv_weight_size);
	tmp += fdvt_fld_tree_forest05_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest06_cv_weight[0],
		fdvt_fld_tree_forest06_cv_weight_size);
	tmp += fdvt_fld_tree_forest06_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest07_cv_weight[0],
		fdvt_fld_tree_forest07_cv_weight_size);
	tmp += fdvt_fld_tree_forest07_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest08_cv_weight[0],
		fdvt_fld_tree_forest08_cv_weight_size);
	tmp += fdvt_fld_tree_forest08_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest09_cv_weight[0],
		fdvt_fld_tree_forest09_cv_weight_size);
	tmp += fdvt_fld_tree_forest09_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest10_cv_weight[0],
		fdvt_fld_tree_forest10_cv_weight_size);
	tmp += fdvt_fld_tree_forest10_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest11_cv_weight[0],
		fdvt_fld_tree_forest11_cv_weight_size);
	tmp += fdvt_fld_tree_forest11_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest12_cv_weight[0],
		fdvt_fld_tree_forest12_cv_weight_size);
	tmp += fdvt_fld_tree_forest12_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest13_cv_weight[0],
		fdvt_fld_tree_forest13_cv_weight_size);
	tmp += fdvt_fld_tree_forest13_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest14_cv_weight[0],
		fdvt_fld_tree_forest14_cv_weight_size);
	tmp += fdvt_fld_tree_forest14_cv_weight_size;

	memcpy(tmp, &fdvt_fld_fp_forest00_om45[0],
		fdvt_fld_fp_forest00_om45_size);
	tmp += fdvt_fld_fp_forest00_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest01_om45[0],
		fdvt_fld_fp_forest01_om45_size);
	tmp += fdvt_fld_fp_forest01_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest02_om45[0],
		fdvt_fld_fp_forest02_om45_size);
	tmp += fdvt_fld_fp_forest02_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest03_om45[0],
		fdvt_fld_fp_forest03_om45_size);
	tmp += fdvt_fld_fp_forest03_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest04_om45[0],
		fdvt_fld_fp_forest04_om45_size);
	tmp += fdvt_fld_fp_forest04_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest05_om45[0],
		fdvt_fld_fp_forest05_om45_size);
	tmp += fdvt_fld_fp_forest05_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest06_om45[0],
		fdvt_fld_fp_forest06_om45_size);
	tmp += fdvt_fld_fp_forest06_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest07_om45[0],
		fdvt_fld_fp_forest07_om45_size);
	tmp += fdvt_fld_fp_forest07_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest08_om45[0],
		fdvt_fld_fp_forest08_om45_size);
	tmp += fdvt_fld_fp_forest08_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest09_om45[0],
		fdvt_fld_fp_forest09_om45_size);
	tmp += fdvt_fld_fp_forest09_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest10_om45[0],
		fdvt_fld_fp_forest10_om45_size);
	tmp += fdvt_fld_fp_forest10_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest11_om45[0],
		fdvt_fld_fp_forest11_om45_size);
	tmp += fdvt_fld_fp_forest11_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest12_om45[0],
		fdvt_fld_fp_forest12_om45_size);
	tmp += fdvt_fld_fp_forest12_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest13_om45[0],
		fdvt_fld_fp_forest13_om45_size);
	tmp += fdvt_fld_fp_forest13_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest14_om45[0],
		fdvt_fld_fp_forest14_om45_size);
	tmp += fdvt_fld_fp_forest14_om45_size;

	memcpy(tmp, &fdvt_fld_leafnode_forest00[0],
		fdvt_fld_leafnode_forest00_size);
	tmp += fdvt_fld_leafnode_forest00_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest01[0],
		fdvt_fld_leafnode_forest01_size);
	tmp += fdvt_fld_leafnode_forest01_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest02[0],
		fdvt_fld_leafnode_forest02_size);
	tmp += fdvt_fld_leafnode_forest02_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest03[0],
		fdvt_fld_leafnode_forest03_size);
	tmp += fdvt_fld_leafnode_forest03_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest04[0],
		fdvt_fld_leafnode_forest04_size);
	tmp += fdvt_fld_leafnode_forest04_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest05[0],
		fdvt_fld_leafnode_forest05_size);
	tmp += fdvt_fld_leafnode_forest05_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest06[0],
		fdvt_fld_leafnode_forest06_size);
	tmp += fdvt_fld_leafnode_forest06_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest07[0],
		fdvt_fld_leafnode_forest07_size);
	tmp += fdvt_fld_leafnode_forest07_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest08[0],
		fdvt_fld_leafnode_forest08_size);
	tmp += fdvt_fld_leafnode_forest08_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest09[0],
		fdvt_fld_leafnode_forest09_size);
	tmp += fdvt_fld_leafnode_forest09_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest10[0],
		fdvt_fld_leafnode_forest10_size);
	tmp += fdvt_fld_leafnode_forest10_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest11[0],
		fdvt_fld_leafnode_forest11_size);
	tmp += fdvt_fld_leafnode_forest11_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest12[0],
		fdvt_fld_leafnode_forest12_size);
	tmp += fdvt_fld_leafnode_forest12_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest13[0],
		fdvt_fld_leafnode_forest13_size);
	tmp += fdvt_fld_leafnode_forest13_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest14[0],
		fdvt_fld_leafnode_forest14_size);
	tmp += fdvt_fld_leafnode_forest14_size;

	memcpy(tmp, &fdvt_fld_tree_forest00_init_shape[0],
		fdvt_fld_tree_forest00_init_shape_size);
	tmp += fdvt_fld_tree_forest00_init_shape_size;

	memcpy(tmp, &fdvt_fld_tree_forest00_tree_node[0],
		fdvt_fld_tree_forest00_tree_node_size);
	tmp += fdvt_fld_tree_forest00_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest01_tree_node[0],
		fdvt_fld_tree_forest01_tree_node_size);
	tmp += fdvt_fld_tree_forest01_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest02_tree_node[0],
		fdvt_fld_tree_forest02_tree_node_size);
	tmp += fdvt_fld_tree_forest02_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest03_tree_node[0],
		fdvt_fld_tree_forest03_tree_node_size);
	tmp += fdvt_fld_tree_forest03_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest04_tree_node[0],
		fdvt_fld_tree_forest04_tree_node_size);
	tmp += fdvt_fld_tree_forest04_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest05_tree_node[0],
		fdvt_fld_tree_forest05_tree_node_size);
	tmp += fdvt_fld_tree_forest05_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest06_tree_node[0],
		fdvt_fld_tree_forest06_tree_node_size);
	tmp += fdvt_fld_tree_forest06_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest07_tree_node[0],
		fdvt_fld_tree_forest07_tree_node_size);
	tmp += fdvt_fld_tree_forest07_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest08_tree_node[0],
		fdvt_fld_tree_forest08_tree_node_size);
	tmp += fdvt_fld_tree_forest08_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest09_tree_node[0],
		fdvt_fld_tree_forest09_tree_node_size);
	tmp += fdvt_fld_tree_forest09_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest10_tree_node[0],
		fdvt_fld_tree_forest10_tree_node_size);
	tmp += fdvt_fld_tree_forest10_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest11_tree_node[0],
		fdvt_fld_tree_forest11_tree_node_size);
	tmp += fdvt_fld_tree_forest11_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest12_tree_node[0],
		fdvt_fld_tree_forest12_tree_node_size);
	tmp += fdvt_fld_tree_forest12_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest13_tree_node[0],
		fdvt_fld_tree_forest13_tree_node_size);
	tmp += fdvt_fld_tree_forest13_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest14_tree_node[0],
		fdvt_fld_tree_forest14_tree_node_size);

#if FLD_UT
#if FLD_SMALL_MODEL
	tmp += fdvt_fld_tree_forest14_tree_node_size;
	memcpy(tmp, &fdvt_fld_img_frame_00[0], fdvt_fld_img_frame_00_size);
	tmp += fdvt_fld_img_frame_00_size;
	memcpy(tmp, &fdvt_fld_result_frame00[0], fdvt_fld_result_frame00_size);
	tmp += fdvt_fld_result_frame00_size;
	memcpy(tmp, &fdvt_fld_img_frame_01[0], fdvt_fld_img_frame_01_size);
	tmp += fdvt_fld_img_frame_01_size;
	memcpy(tmp, &fdvt_fld_result_frame01[0], fdvt_fld_result_frame01_size);
#else
	tmp += fdvt_fld_tree_forest14_tree_node_size;
	memcpy(tmp, &fdvt_fld_img_frame_02[0], fdvt_fld_img_frame_02_size);
	tmp += fdvt_fld_img_frame_02_size;
	memcpy(tmp, &fdvt_fld_result_frame02[0], fdvt_fld_result_frame02_size);
	tmp += fdvt_fld_result_frame02_size;
	memcpy(tmp, &fdvt_fld_img_frame_07[0], fdvt_fld_img_frame_07_size);
	tmp += fdvt_fld_img_frame_07_size;
	memcpy(tmp, &fdvt_fld_result_frame07[0], fdvt_fld_result_frame07_size);
#endif /* #endif FLD_SMALL_MODEL */
#endif /* #endif FLD_UT */
}
