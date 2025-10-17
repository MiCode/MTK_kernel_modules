/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _GPS_DL_HW_COMMON_H
#define _GPS_DL_HW_COMMON_H

#include "gps_dl_isr.h"

bool gps_dl_hw_is_gps_irq_triggerd(enum gps_dl_irq_index_enum irq_id);

#endif /* _GPS_DL_HW_COMMON_H */

