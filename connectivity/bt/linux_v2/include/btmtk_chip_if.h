/* SPDX-License-Identifier: GPL-2.0 */  
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#ifndef __BTMTK_CHIP_IF_H__
#define __BTMTK_CHIP_IF_H__

#ifdef CHIP_IF_USB
#include "btmtk_usb.h"
#elif defined(CHIP_IF_SDIO)
#include "btmtk_sdio.h"
#elif defined(CHIP_IF_UART)
#include "btmtk_uart.h"
#elif defined(CHIP_IF_BTIF)
#include "btmtk_btif.h"
#endif

int btmtk_cif_register(void);
int btmtk_cif_deregister(void);

#endif /* __BTMTK_CHIP_IF_H__ */
