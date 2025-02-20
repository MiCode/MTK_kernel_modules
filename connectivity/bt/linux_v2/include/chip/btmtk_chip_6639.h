/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_CHIP_COMMON_6639_H_
#define _BTMTK_CHIP_COMMON_6639_H_

#include "btmtk_main.h"
#include "btmtk_define.h"

#define BT_RADIO_OFF_DONE_6639 0x7C05B100

#define BACK_UP_CMD_LEN 10

/* calibration backup & restore */
#define CAL_DATA_EVNET_LEN 9
#define CAL_DATA_CMD_LEN 10


int btmtk_cif_chip_6639_register(void);

#endif /* __BTMTK_CHIP_COMMON_6639_H__ */
