/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HW_TYPE_H
#define _GPS_MCUDL_HW_TYPE_H

#include "gps_dl_config.h"
#include "gps_mcudl_config.h"

/* to import u32 series, bool types */
#if GPS_DL_ON_LINUX
#include <linux/types.h>
#elif GPS_DL_ON_CTP
#include "kernel_to_ctp.h"
#endif

#endif /* _GPS_MCUDL_HW_TYPE_H */

