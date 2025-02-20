/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2023 MediaTek Inc. */

#ifndef __ADAPTOR_SENTEST_H__
#define __ADAPTOR_SENTEST_H__

#include <linux/mutex.h>

/******************************************************************************/
// adaptor sentest  --- structure define
/******************************************************************************/

struct mtk_cam_sentest_cfg_info {
	bool lbmf_delay_do_ae_en;
	bool power_on_profile_en;
	u32  listen_tsrec_frame_id;

	struct mutex sentest_update_tsrec_mutex;
};

#endif
