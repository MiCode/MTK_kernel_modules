/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef __IMGSENSOR_PKVM_H__
#define __IMGSENSOR_PKVM_H__

#include <linux/types.h>

#include "mtk_cam-seninf-common.h"

extern bool is_pkvm_enabled(void);
extern uint64_t get_chk_pa(void);

enum SENINF_PKVM_RETURN {
	SENINF_PKVM_RETURN_ERROR,
	SENINF_PKVM_RETURN_SUCCESS
};

int seninf_pkvm_open_session(void);
int seninf_pkvm_close_session(void);
int seninf_pkvm_checkpipe(u64 SecInfo_addr);
int seninf_pkvm_free(void);

#endif /* __IMGSENSOR_PKVM_H__ */
