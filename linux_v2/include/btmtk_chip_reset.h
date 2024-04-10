/* SPDX-License-Identifier: GPL-2.0 */  
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#ifndef __BTMTK_CHIP_RESET_H__
#define __BTMTK_CHIP_RESET_H__

#include <linux/version.h>
#include <linux/timer.h>

#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_woble.h"

#define CHIP_RESET_TIMEOUT 20

void btmtk_reset_timer_add(struct btmtk_dev *bdev);
void btmtk_reset_timer_update(struct btmtk_dev *bdev);
void btmtk_reset_timer_del(struct btmtk_dev *bdev);
void btmtk_reset_trigger(struct btmtk_dev *bdev);
void btmtk_reset_waker(struct work_struct *work);

#endif /* __BTMTK_WOBLE_H__ */

