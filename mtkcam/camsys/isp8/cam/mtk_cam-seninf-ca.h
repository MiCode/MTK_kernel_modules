/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */



#ifndef __IMGSENSOR_CA_H__
#define __IMGSENSOR_CA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/limits.h>
#include <kree/system.h>
#include <kree/mem.h>

#include "mtk_cam-seninf-common.h"

enum SENINF_CA_RETURN {

	SENINF_CA_RETURN_ERROR,
	SENINF_CA_RETURN_SUCCESS

};

struct SENINF_CA {

	KREE_SESSION_HANDLE session;

};

int seninf_ca_open_session(void);
int seninf_ca_close_session(void);
int seninf_ca_checkpipe(u64 SecInfo_addr);
int seninf_ca_free(void);

#endif

