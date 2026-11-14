/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_fw_dev.h
 *    \brief  This file includes private ioctl support.
 */
#ifndef _GL_FW_DEV_H_
#define _GL_FW_DEV_H_

enum ENUM_FW_IDX_LOG_SAVE_CFG {
	FW_IDX_LOG_SAVE_DISABLE = 0,
	FW_IDX_LOG_SAVE_ONLY,
	FW_IDX_LOG_SAVE_WITH_PRINT
};

void FwLogWrite(char *buf, size_t count);
int FwLogDevInit(void);
int FwLogDevUninit(void);

#endif
