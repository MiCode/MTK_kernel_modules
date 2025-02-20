/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2023 MediaTek Inc. */

#ifndef __ADAPTOR_TSREC_CB_CTRL_H__
#define __ADAPTOR_TSREC_CB_CTRL_H__


#include <linux/mutex.h>

#include "mtk_camera-v4l2-controls.h"
#include "adaptor.h"


/******************************************************************************/
// adaptor tsrec call back ctrl --- define/struct
/******************************************************************************/
struct adaptor_tsrec_cb_ctrl {
	struct mutex op_lock;

	struct mtk_cam_seninf_tsrec_cb_info cb_info;
};

#endif
