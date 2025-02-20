/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *         Holmes Chiou <holmes.chiou@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_FRM_SYNC_EVENT_H_
#define _MTK_IMGSYS_FRM_SYNC_EVENT_H_

enum mtk_imgsys_frm_sync_event_group {
	mtk_imgsys_frm_sync_event_group_mcnr = 0,
	mtk_imgsys_frm_sync_event_group_vsdof = 1,
	mtk_imgsys_frm_sync_event_group_aiseg = 2,
};

#define TOKEN_GROUP(event)  (((event) >>16) & 0xFFFF)

#endif /* _MTK_IMGSYS_FRM_SYNC_EVENT_H_ */
