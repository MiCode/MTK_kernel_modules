/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *
 */

#ifndef _MTK_DIP_SYS_H_
#define _MTK_DIP_SYS_H_

#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/mutex.h>

#include "mtk-img-ipi.h"

#define DUMP_USING_CMDQ_TH
#define PRE_PWR_ON_0 (1UL << 0)
#define PRE_PWR_ON_1 (1UL << 1)
#define PRE_PWR_OFF_0 (1UL << 2)
#define PRE_PWR_OFF_1 (1UL << 3)

extern unsigned int nodes_num;

#endif /* _MTK_DIP_SYS_H_ */
