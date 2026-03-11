/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_CHIP_7902_H_
#define _BTMTK_CHIP_7902_H_

#include "btmtk_define.h"
#include "btmtk_main.h"

#define ZB_ENABLE	0x7C00114C

#define BT_BACKDOOR_RET_7902 0x74013EC8

int btmtk_cif_chip_7902_register(void);

#endif /* __BTMTK_CHIP_7902_H__ */
