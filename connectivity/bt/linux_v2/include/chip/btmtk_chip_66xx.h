/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_CHIP_66xx_H_
#define _BTMTK_CHIP_66xx_H_

#include "btmtk_define.h"
#include "btmtk_main.h"
#if (USE_DEVICE_NODE == 1)
#include "btmtk_queue.h"
#include "connv3.h"
//#include "btmtk_proj_sp.h"
#endif

int btmtk_cif_chip_66xx_register(void);

#endif /* __BTMTK_CHIP_66xx_H__ */
