/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuhsuan.chang <yuhsuan.chang@mediatek.com>
 *
 */

#ifndef IMGSYS_PLATFORMS_ISP8_MTK_IMGSYS_CMDQ_HWQOS_H_
#define IMGSYS_PLATFORMS_ISP8_MTK_IMGSYS_CMDQ_HWQOS_H_

#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include "mtk_imgsys-dev.h"

void mtk_imgsys_cmdq_hwqos_init(struct mtk_imgsys_dev *imgsys_dev);
void mtk_imgsys_cmdq_hwqos_release(void);
void mtk_imgsys_cmdq_hwqos_streamon(const struct mtk_imgsys_hwqos *hwqos_info);
void mtk_imgsys_cmdq_hwqos_streamoff(void);
void mtk_imgsys_cmdq_hwqos_report(
	struct cmdq_pkt *pkt,
	const struct mtk_imgsys_hwqos *hwqos_info,
	const int *fps);

#endif  // IMGSYS_PLATFORMS_ISP8_MTK_IMGSYS_CMDQ_HWQOS_H_
