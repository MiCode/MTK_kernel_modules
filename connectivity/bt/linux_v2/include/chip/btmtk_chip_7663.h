/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_CHIP_7663_H_
#define _BTMTK_CHIP_7663_H_

#include "btmtk_main.h"
#include "btmtk_define.h"

#define PINMUX_REG_NUM 2
#define AUDIO_PINMUX_SETTING_OFFSET 4

#define PATCH_ERR_7663 -1
#define PATCH_IS_DOWNLOAD_BY_OTHER_7663 0
#define PATCH_READY_7663 1
#define PATCH_NEED_DOWNLOAD_7663 2
#define USB_IO_BUF_SIZE (HCI_MAX_EVENT_SIZE > 256 ? HCI_MAX_EVENT_SIZE : 256)


int btmtk_cif_chip_7663_register(struct btmtk_dev *bdev);

#endif /* __BTMTK_CHIP_7663_H__ */
