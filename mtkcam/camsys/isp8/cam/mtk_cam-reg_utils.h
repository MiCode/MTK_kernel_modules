/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MTK_CAM_REGS_UTILS_H
#define _MTK_CAM_REGS_UTILS_H

#define FMASK(pos, width)	(((1 << (width)) - 1) << (pos))

static inline u32 _read_field(u32 val, int pos, int width)
{
	return (val & FMASK(pos, width)) >> pos;
}

static inline u32 _set_field(u32 *val, int pos, int width, u32 fval)
{
	u32 fmask = FMASK(pos, width);

	*val &= ~fmask;
	*val |= ((fval << pos) & fmask);
	return *val;
}

#define FBIT(field)	BIT(F_ ## field ## _POS)

#define READ_FIELD(val, field)	\
	_read_field(val, F_ ## field ## _POS, F_ ## field ## _WIDTH)

#define SET_FIELD(val, field, fval)	\
	_set_field(val, F_ ## field ## _POS, F_ ## field ## _WIDTH, fval)

#endif	/* _MTK_CAM_REGS_UTILS_H */
