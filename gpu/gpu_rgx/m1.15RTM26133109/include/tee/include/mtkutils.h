/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 */
#if !defined(MTKUTILS_H)
#define MTKUTILS_H

#include <FreeRTOS.h>
#include <tinysys_reg.h>

enum SECGPU_FW_PROC_STA
{
    SECGPU_FW_PROC_STA_FW_INVALID_SIZE        = 0,
    SECGPU_FW_PROC_STA_FW_INVALID_VER         = 1,
    SECGPU_FW_PROC_STA_FW_INVALID_HEADER      = 2,
    SECGPU_FW_PROC_STA_FW_INVALID_LAYOUT_SIZE = 3,
    SECGPU_FW_PROC_STA_FW_INVALID_LAYOUT_NUM  = 4,
    SECGPU_FW_PROC_STA_FW_INVALID_SEG         = 5,
	SECGPU_FW_PROC_STA_NONE                   = 0xf
};

enum SECGPU_POW_STA
{
    SECGPU_POW_STA_START_FAIL = 0,
    SECGPU_POW_STA_STOP_FAIL  = 1,
	SECGPU_POW_STA_NONE       = 0xf
};

void secgpu_footprint_fw_proc_step(enum SECGPU_FW_PROC_STA step);
void secgpu_footprint_pow_step(enum SECGPU_POW_STA step);

/*
* GPUEB_SRAM_GPR15
* 0x0000000F => SECGPU_FW_PROC_STA
* 0x000000F0 => SECGPU_POW_STA
*/

#endif /* !defined(MTKUTILS_H) */