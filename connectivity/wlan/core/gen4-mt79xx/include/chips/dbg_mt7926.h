/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#ifndef _DBG_MT7926_H
#define _DBG_MT7926_H

struct dump_cr_set {
	u_int8_t read;
	uint32_t addr;
	uint32_t mask;
	uint32_t shift;
	uint32_t value;
};

#if (CFG_SUPPORT_DEBUG_SOP == 1)
u_int8_t mt7926_show_debug_sop_info(struct ADAPTER *ad,	uint8_t ucCase);
#endif /* CFG_SUPPORT_DEBUG_SOP */

#endif
