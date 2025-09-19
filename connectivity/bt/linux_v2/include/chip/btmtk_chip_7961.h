/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_CHIP_7961_H_
#define _BTMTK_CHIP_7961_H_

#include "btmtk_main.h"
#include "btmtk_define.h"

#define BT_BACKDOOR_RET_7961 0x74013EC8

#define BT_RADIO_OFF_DONE_7961 0X7C053C00

#define PINMUX_REG_NUM 2
#define AUDIO_PINMUX_SETTING_OFFSET 4

int btmtk_cif_chip_7961_register(void);

#endif /* __BTMTK_CHIP_7961_H__ */
