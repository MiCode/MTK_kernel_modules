
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Ming-Hsuan.Chiang <Ming-Hsuan.Chiang@mediatek.com>
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#ifdef AIE_TF_DUMP_7SP
#include <dt-bindings/memory/mt6897-larb-port.h>
#endif
#include "mtk_aie.h"
#include "iommu_debug.h"

#ifdef AIE_TF_DUMP_7SP
static void aie_tf_dump(struct mtk_aie_dev *fd)
{
    if (fd == NULL) {
        pr_info("AIE TF Dump Data pointer is NULL\n");
        return;
    }

    if (fd->dev == NULL || fd->base_para == NULL || fd->aie_cfg == NULL) {
        pr_info("AIE TF Dump param is NULL\n");
        return;
    }

    /* init param */
    dev_info(fd->dev, "rpn_anchor_thrd(%d), max_img_height(%d), max_img_width(%d)\n",
        fd->base_para->rpn_anchor_thrd, fd->base_para->max_img_height,
        fd->base_para->max_img_width);
    dev_info(fd->dev, "max_pyramid_width(%d), max_pyramid_height(%d)\n",
        fd->base_para->max_pyramid_width, fd->base_para->max_pyramid_height);

    /* perframe param */
    dev_info(fd->dev, "sel_mode(%d), src_img_fmt(%d)\n",
        fd->aie_cfg->sel_mode, fd->aie_cfg->src_img_fmt);
    dev_info(fd->dev, "src_img_width(%d), src_img_height(%d), src_img_stride(%d)\n",
        fd->aie_cfg->src_img_width, fd->aie_cfg->src_img_height, fd->aie_cfg->src_img_stride);
    dev_info(fd->dev, "pyramid_base_width(%d), pyramid_base_height(%d), number_of_pyramid(%d)\n",
        fd->aie_cfg->pyramid_base_width, fd->aie_cfg->pyramid_base_height,
        fd->aie_cfg->number_of_pyramid);
    dev_info(fd->dev, "fd->aie_cfg->rotate_degree(%d), fd->aie_cfg->en_roi(%d),(%d,%d,%d,%d)\n",
        fd->aie_cfg->rotate_degree, fd->aie_cfg->en_roi, fd->aie_cfg->src_roi.x1,
        fd->aie_cfg->src_roi.y1, fd->aie_cfg->src_roi.x2, fd->aie_cfg->src_roi.y2);
    dev_info(fd->dev, "fd->aie_cfg->en_padding(%d),(%d,%d,%d,%d),(%d)\n",
        fd->aie_cfg->en_padding, fd->aie_cfg->src_padding.left, fd->aie_cfg->src_padding.right,
        fd->aie_cfg->src_padding.down, fd->aie_cfg->src_padding.up, fd->aie_cfg->freq_level);
}

static int FDVT_M4U_TranslationFault_callback(int port,
							   dma_addr_t mva,
							   void *data)
{
    pr_info("[FDVT_M4U]fault call port=%d, mva=0x%llx\n", port, mva);

    if (data == NULL)
        pr_info("FDVT TF CB Data is NULL\n");
    else
	    aie_tf_dump(data);

	return 1;
}
#endif

void mtk_aie_register_tf_cb_7sp(void *data)
{
#ifdef AIE_TF_DUMP_7SP
    mtk_iommu_register_fault_callback(M4U_L12_P0_FDVT_RDA_0,
		(mtk_iommu_fault_callback_t)FDVT_M4U_TranslationFault_callback,
		data, false);
	mtk_iommu_register_fault_callback(M4U_L12_P1_FDVT_WRA_0,
		(mtk_iommu_fault_callback_t)FDVT_M4U_TranslationFault_callback,
		data, false);
#endif
}
EXPORT_SYMBOL(mtk_aie_register_tf_cb_7sp);

int mtk_aie_debug_7sp_probe(struct platform_device *pdev)
{
    dev_info(&pdev->dev ,"%s+", __func__);
    register_mtk_aie_reg_tf_cb(mtk_aie_register_tf_cb_7sp);
    dev_info(&pdev->dev ,"%s-", __func__);

    return 0;
}

int mtk_aie_debug_7sp_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev ,"[aie_dev] %s+-", __func__);

	return 0;
}

static const struct of_device_id of_match_mtk_aie_debug_7sp_drv[] = {
	{
		.compatible = "mediatek,mtk-aie-debug-7sp",
	}, {
		/* sentinel */
	}
};

static struct platform_driver mtk_aie_debug_7sp_drv = {
	.probe = mtk_aie_debug_7sp_probe,
    .remove = mtk_aie_debug_7sp_remove,
	.driver = {
		.name = "mtk-aie-debug-7sp",
        .of_match_table = of_match_mtk_aie_debug_7sp_drv,
	},
};

module_platform_driver(mtk_aie_debug_7sp_drv);
MODULE_AUTHOR("Ming-Hsuan Chaing <ming-hsuan.chiang@mediatek.com>");
MODULE_LICENSE("GPL v2");
