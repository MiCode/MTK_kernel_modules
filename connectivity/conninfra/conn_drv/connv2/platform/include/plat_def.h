/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef PLAT_DEF_H
#define PLAT_DEF_H

#ifdef mapped_addr
#undef mapped_addr
#endif
#define mapped_addr void __iomem *

#ifdef strnlcat
#undef strnlcat
#endif
#define strnlcat(des, src, length, size) strncat(des, src, length)

#ifdef get_jiffies
#undef get_jiffies
#endif
#define get_jiffies() jiffies

#ifdef time_duration
#undef time_duration
#endif
#define time_duration(x)  jiffies_to_msecs(jiffies - x)

#endif	/* PLAT_DEF_H */
