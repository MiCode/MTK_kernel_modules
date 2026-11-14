/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __BTMTK_CHIP_IF_H__
#define __BTMTK_CHIP_IF_H__

#ifdef CHIP_IF_USB
#include "btmtk_usb.h"
#elif defined(CHIP_IF_SDIO)
#include "btmtk_sdio.h"
#elif defined(CHIP_IF_UART_TTY)
#include "btmtk_uart_tty.h"
#elif defined(CHIP_IF_UART_SERDEV)
#include "btmtk_uart_serdev.h"
#elif defined(CHIP_IF_BTIF)
#include "btmtk_btif.h"
#endif

int btmtk_cif_register(void);
int btmtk_cif_deregister(void);

#endif /* __BTMTK_CHIP_IF_H__ */
