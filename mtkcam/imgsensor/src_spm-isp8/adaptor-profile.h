/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 */

#ifndef _ADAPTOR_PROFILE_H__
#define _ADAPTOR_PROFILE_H__

#include <linux/time64.h>

struct adaptor_profile_tv {
	struct timespec64 begin;
	struct timespec64 end;
};

#define ADAPTOR_PROFILE_BEGIN(_ptv) \
	ktime_get_real_ts64(&(_ptv)->begin)

#define ADAPTOR_PROFILE_END(_ptv) \
	ktime_get_real_ts64(&(_ptv)->end)

#define ADAPTOR_PROFILE_G_DIFF_NS(_ptv) \
({ \
	struct timespec64 _diff = timespec64_sub((_ptv)->end, (_ptv)->begin); \
	s64 _diff_in_ns = timespec64_to_ns(&_diff); \
	_diff_in_ns; \
})

#endif
