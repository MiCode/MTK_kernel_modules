// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include "./platforms/isp7sp/mtk_imgsys-data-7sp.h"
#include "mtk_imgsys-probe.h"

int imgsys_dip_dbg_en;
module_param(imgsys_dip_dbg_en, int, 0644);

int imgsys_traw_dbg_en;
module_param(imgsys_traw_dbg_en, int, 0644);

int imgsys_pqdip_dbg_en;
module_param(imgsys_pqdip_dbg_en, int, 0644);

int imgsys_wpe_dbg_en;
module_param(imgsys_wpe_dbg_en, int, 0644);

int imgsys_me_dbg_en;
module_param(imgsys_me_dbg_en, int, 0644);

static const struct dev_pm_ops mtk_imgsys_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_imgsys_runtime_suspend,
						mtk_imgsys_runtime_resume, NULL)
};

static const struct of_device_id mtk_imgsys_of_match[] = {
	{ .compatible = "mediatek,imgsys-isp7sp-mt6897", .data = (void *)&imgsys_data_mt6897},
	{ .compatible = "mediatek,imgsys-isp7sp-mt6989", .data = (void *)&imgsys_data_mt6989},
	{ .compatible = "mediatek,imgsys-isp7sp-mt6878", .data = (void *)&imgsys_data_mt6878},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_imgsys_of_match);

static struct platform_driver mtk_imgsys_driver = {
	.probe   = mtk_imgsys_probe,
	.remove  = mtk_imgsys_remove,
	.shutdown = mtk_imgsys_shutdown,
	.driver  = {
		.name = "imgisp7sp",
		.owner	= THIS_MODULE,
		.pm = &mtk_imgsys_pm_ops,
		.of_match_table = of_match_ptr(mtk_imgsys_of_match),
	}
};

module_platform_driver(mtk_imgsys_driver);

MODULE_AUTHOR("Johnson-CH chiu <johnson-ch.chiu@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek imgsys driver");

