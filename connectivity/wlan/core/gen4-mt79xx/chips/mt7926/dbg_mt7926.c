// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#include "precomp.h"
#include "dbg_mt7926.h"
#include "dbg_comm.h"
#include "mt7926.h"
#include "hal.h"

struct dump_cr_set conninfra_pwr_on_domain_dump_list[] = {
	{ TRUE, 0x7C0602cc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7C0602dc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7C060014, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7C060054, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7C060010, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7C060050, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7C060018, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7C060058, 0xFFFFFFFF, 0 }
};

struct dump_cr_set conninfra_pwr_off_domain_dump_list[] = {
	{ TRUE, 0x7c001320, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c009a00, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001384, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c0013cc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000400, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000404, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000438, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c0050a8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c005120, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c005124, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c005128, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00512c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c005130, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c005134, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c004000, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c004050, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c004054, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c004058, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c004108, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c004004, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00904c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001620, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001610, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001600, 0xFFFFFFFF, 0 }
};

struct dump_cr_set conninfra_bus_on_domain_dump_list[] = {
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00010001 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00020001 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00010002 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00020002 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00030002 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00010003 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00020003 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00030003 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00010004 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00020004 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000138, 0xFFFFFFFF, 0, 0x00010005 },
	{ TRUE, 0x7c000150, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000410, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000414, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000418, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00041c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000420, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000434, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00042c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000430, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000424, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c000428, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00018c, 0xFFFFFFFF, 0 }
};

struct dump_cr_set conninfra_bus_off_domain_dump_list_b[] = {
	{ TRUE, 0x7c00e2a8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e2ac, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e2b0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e2b4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e2a0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e2a4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e230, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e234, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e398, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e2e8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e30c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e310, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e314, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e318, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e334, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e32c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e330, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e324, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e328, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00e18c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f408, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f40c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f410, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f414, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f418, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f41c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f420, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f424, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f428, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f42c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c00f430, 0xFFFFFFFF, 0 }
};

struct dump_cr_set conninfra_bus_off_domain_dump_list_c[] = {
	/* Section C */
	{ TRUE, 0x7c001514, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001546, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001554, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001584, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001574, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001504, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c001544, 0xFFFFFFFF, 0 }
};

#if defined(_HIF_PCIE)
struct dump_cr_set conninfra_clk_on_domain_dump_list[] = {
	{ TRUE, 0x7c060294, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 0 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 1 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 2 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 3 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 4 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 5 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 6 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c06015c, 0x7, 0, 7 },
	{ TRUE, 0x7c0602c8, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 0 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 1 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 2 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 3 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 4 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 5 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 7 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 8 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 9 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c000204, 0xF, 0, 10 },
	{ TRUE, 0x7c0602d0, 0xFFFFFFFF, 0 }
};

struct dump_cr_set conninfra_clk_off_domain_dump_list[] = {
	{ TRUE, 0x7c0602d8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c009050, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7c060000, 0xFFFFFFFF, 0 }
};
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
struct dump_cr_set conninfra_clk_on_domain_dump_list[] = {
	{ FALSE, 0x74000a4c, 0x80000000, 31, 1 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0xb },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0xa },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x0 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x1 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x2 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x3 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x4 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x5 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x6 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x7, 0, 0x7 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a24, 0x80000000, 31, 0x1 },
	{ FALSE, 0x74000a24, 0x0f000000, 24, 0x1f },
	{ FALSE, 0x74000a24, 0x000f0000, 16, 0x1e },
	{ TRUE, 0x74000a20, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a24, 0x80000000, 31, 0x1 },
	{ FALSE, 0x74000a24, 0x3f000000, 24, 0x21 },
	{ FALSE, 0x74000a24, 0x003f0000, 16, 0x20 },
	{ TRUE, 0x74000a20, 0xFFFFFFFF, 0 }
};

struct dump_cr_set conninfra_clk_off_domain_dump_list[] = {
	{ FALSE, 0x74000a24, 0x80000000, 31, 0x1 },
	{ FALSE, 0x74000a24, 0x3f000000, 24, 0x0 },
	{ FALSE, 0x74000a24, 0x003f0000, 16, 0x23 },
	{ TRUE, 0x74000a20, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x80000000, 31, 0x1 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x0 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x1 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x2 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x3 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x4 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x5 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x6 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0x78000000, 27, 0x7 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 }
};
#endif /* _HIF_USB */

#if defined(_HIF_SDIO)
struct dump_cr_set conninfra_clk_on_domain_dump_list[] = {
	{ FALSE, 0x00000044, 0x80000000, 31, 1 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0xb },
	{ FALSE, 0x00000048, 0x00000007, 0, 0x4 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0xa },
	{ FALSE, 0x00000048, 0x00000007, 0, 0x4 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x0 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x1 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x2 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x3 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x4 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x5 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x6 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x7 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x80000000, 31, 0x1 },
	{ FALSE, 0x00000048, 0x00000007, 0, 0x3 },
	{ FALSE, 0x00000044, 0x80000000, 31, 0x1 },
	{ FALSE, 0x00000044, 0x3f000000, 24, 0x1f },
	{ FALSE, 0x00000044, 0x003f0000, 16, 0x1e },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x80000000, 31, 0x1 },
	{ FALSE, 0x00000044, 0x3f000000, 24, 0x21 },
	{ FALSE, 0x00000044, 0x003f0000, 16, 0x20 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 }
};

struct dump_cr_set conninfra_clk_off_domain_dump_list[] = {
	{ FALSE, 0x00000044, 0x80000000, 31, 0x1 },
	{ FALSE, 0x00000044, 0x3f000000, 24, 0x0 },
	{ FALSE, 0x00000044, 0x003f0000, 16, 0x23 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x80000000, 31, 0x1 },
	{ FALSE, 0x00000044, 0x00000007, 0, 0x4 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x0 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x1 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x2 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x3 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x4 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x5 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x6 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0x78000000, 27, 0x7 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
};
#endif /* _HIF_USB */

struct dump_cr_set wf_top_dump_list_pcie[] = {
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x00100000 },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x00108421 },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x00184210 },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x00194a52 },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x001bdef7 },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x001c6318 },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x001e739c },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060094, 0xFFFFFFFF, 0, 0x001ef7bd },
	{ TRUE, 0x7c06021c, 0xFFFFFFFF, 0 }
};

struct dump_cr_set wf_top_dump_list_usb[] = {
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x80000000 },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x80008421 },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x80084210 },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x80094a52 },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x800bdef7 },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x800c6318 },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x800e739c },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a14, 0xFFFFFFFF, 0, 0x800ef7bd },
	{ TRUE, 0x74000a10, 0xFFFFFFFF, 0 }
};

struct dump_cr_set wf_top_dump_list_sdio[] = {
	{ FALSE, 0x00000048, 0xFFFFFFFF, 0, 0x0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x80000000 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x80008421 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x80084210 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x80094a52 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x800BDEF7 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x800C6318 },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x800E739C },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x00000044, 0xFFFFFFFF, 0, 0x800EF7BD },
	{ TRUE, 0x00000040, 0xFFFFFFFF, 0 }
};

#if defined(_HIF_PCIE)
struct dump_cr_set wf_bus_vdnr_timeout_host_side_dump_list[] = {
	{ TRUE, 0x7c06016c, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00010001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00020001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00030001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00040001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00050001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00060001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00070001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00080001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00090001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x000A0001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x000B0001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x000C0001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x000D0001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x000E0001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x000F0001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00100001 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00010002 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0 },
	{ FALSE, 0x7c060164, 0xFFFFFFFF, 0, 0x00010003 },
	{ TRUE, 0x7c060168, 0xFFFFFFFF, 0}
};
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
struct dump_cr_set wf_bus_vdnr_timeout_host_side_dump_list[] = {
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8010001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8020001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8030001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8040001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8050001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8060001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8070001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8080001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8090001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc80A0001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc80B0001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc80C0001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc80D0001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc80E0001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc80F0001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8100001 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8010002 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74000a4c, 0xFFFFFFFF, 0, 0xc8010003 },
	{ TRUE, 0x74000a48, 0xFFFFFFFF, 0}
};
#endif /* _HIF_USB */

#if defined(_HIF_SDIO)
struct dump_cr_set wf_bus_vdnr_timeout_host_side_dump_list[] = {
	{ FALSE, 0x0048, 0xFFFFFFFF, 0, 0x4 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8010001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8020001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8030001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8040001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8050001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8060001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8070001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8080001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8090001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc80A0001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc80B0001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc80C0001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc80D0001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc80E0001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc80F0001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8100001 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8010002 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0 },
	{ FALSE, 0x0044, 0xFFFFFFFF, 0, 0xc8010003 },
	{ TRUE, 0x0040, 0xFFFFFFFF, 0}
};
#endif /* _HIF_SDIO */

struct dump_cr_set wf_bus_vdnr_timeout_wf_side_dump_list[] = {
	// { FALSE, 0x830c0120, 0xFFFFFFFF, 0, 0x810f0000 },
	{ TRUE, 0x810f0408, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f040c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0410, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0414, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0418, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f041c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0420, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0424, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0428, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f042c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0430, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0434, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0438, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f043c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0440, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0444, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0448, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f044c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x810f0000, 0xFFFFFFFF, 0 }
};

struct dump_cr_set wf_bus_ahb_apb_timeout_dump_list[] = {
	{ TRUE, 0x88000444, 0xFFFFFFFF, 0 },
	{ TRUE, 0x88000430, 0xFFFFFFFF, 0 },
	{ TRUE, 0x8800044c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x88000450, 0xFFFFFFFF, 0 },
	{ TRUE, 0x88000440, 0xFFFFFFFF, 0 }
};

struct dump_cr_set cbtop_off_vcore_on_dump_list[] = {
	{ FALSE, 0x70020140, 0x0000FFFF, 0, 0x7002 },
	{ TRUE, 0x70020000, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020004, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020008, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7002000c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020010, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020080, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020100, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020104, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020108, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7002010c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020110, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020120, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020124, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020130, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020134, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020140, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020144, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70020148, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70017000, 0xFFFFFFFF, 0 },
	{ FALSE, 0x70020140, 0x0000FFFF, 0, 0x740a },
	{ TRUE, 0x740ae010, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae014, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae018, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae01c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae020, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae024, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae028, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae02c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae030, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae034, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae038, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae03c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae040, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae044, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae048, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae04c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae060, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae064, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae068, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae06c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae070, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae074, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae078, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae07c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae080, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740ae084, 0xFFFFFFFF, 0 },
	{ FALSE, 0x70020140, 0x0000FFFF, 0, 0x7000 },
};

struct dump_cr_set cbtop_off_vcore_off_dump_list[] = {
	{ TRUE, 0x70013000, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013004, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013008, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7001300c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013100, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013104, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013a50, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013a58, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013a5c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013a60, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70013a64, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7001942c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019430, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019434, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019438, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7001943c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019440, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019448, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7001944c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019458, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7001945c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019460, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019464, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019468, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7001946c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019470, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019474, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019478, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7001947c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019480, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70019484, 0xFFFFFFFF, 0 }
};

struct dump_cr_set cbtop_pcie_dump_list[] = {
	{ FALSE, 0x70020140, 0xFFFF0000, 16, 0x7403 },
	{ TRUE, 0x74030150, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74030154, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74030184, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74031010, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74030168, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74030164, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7403002c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74031204, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74031210, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74030184, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740331c0, 0xFFFFFFFF, 0 }
};

struct dump_cr_set cbtop_usb_dump_list[] = {
	{ FALSE, 0x70020144, 0x0000FFFF, 0, 0x7400 },
	{ TRUE, 0x74000000, 0xFFFFFFFF,	0 },
	{ TRUE, 0x74000018, 0xFFFFFFFF,	0 },
	{ TRUE, 0x74000100, 0xFFFFFFFF,	0 },
	{ TRUE,	0x74000104, 0xFFFFFFFF,	0 },
	{ TRUE,	0x74000120, 0xFFFFFFFF,	0 },
	{ TRUE, 0x74000170, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000174, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000004, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000034, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740001c4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740001cc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000044, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740001d4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740001dc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000064, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740001e4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740001ec, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000324, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000404, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7400040c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000314, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000488, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000490, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000054, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740001f8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000260, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000074, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000208, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000264, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000334, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740004a8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740004b0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740000d8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740000e8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74000584, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7400058c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740000f0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740005a4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740005ac, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740000fc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740005c8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740005d0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003bc, 0xFFFFFFFF, 0 },
	{ FALSE, 0x740003bc, 0x00000005, 0, 0x1 },
	{ TRUE, 0x740003c0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003c4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003c8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003cc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003d0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003d4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003d8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003dc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003bc, 0xFFFFFFFF, 0 },
	{ FALSE, 0x740003bc, 0x00000005, 0, 0x5 },
	{ TRUE, 0x740003c0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003c4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003c8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003cc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003d0, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003d4, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003d8, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003dc, 0xFFFFFFFF, 0 },
	{ TRUE, 0x740003bc, 0xFFFFFFFF, 0 },
	{ FALSE, 0x74020144, 0x0000FFFF, 0, 0x7401 },
	{ TRUE, 0x74013460, 0xFFFFFFFF, 0 },
	{ TRUE, 0x74012534, 0xFFFFFFFF, 0 },
};

struct dump_cr_set cbtop_cr_dump_list[] = {
	{ TRUE,	0x70003000, 0xFFFFFFFF,	0 },
	{ TRUE,	0x70003004, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003008, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000300c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003010, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003014, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003018, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000301c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003020, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003024, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003100, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003200, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003204, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003208, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003300, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003400, 0xFFFFFFFF, 0 },
	{ TRUE,	0x70003800, 0xFFFFFFFF, 0 },
	{ TRUE,	0x70003810, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003840, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003850, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003880, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003890, 0xFFFFFFFF, 0 },
	{ TRUE, 0x700038c0, 0xFFFFFFFF,	0 },
	{ TRUE, 0x700038d0, 0xFFFFFFFF, 0 },
	{ TRUE,	0x70003900, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003910, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003940, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70003950, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001050, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001054, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001058, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001200, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001204, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001208, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000120c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001210, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001214, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001218, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000121c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001220, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001224, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001228, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000122c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001300, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001304, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001308, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000130c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001350, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001354, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001358, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000135c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001400, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001404, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001408, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000140c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001410, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001414, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000141c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001420, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001424, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001428, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000142c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001454, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001500, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001504, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70001508, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000150c, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70000148, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70000200, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70000204, 0xFFFFFFFF, 0 },
	{ TRUE, 0x70000214, 0xFFFFFFFF, 0 },
	{ TRUE, 0x7000202c, 0xFFFFFFFF, 0 }
};

#if defined(_HIF_PCIE)
#define HAL_MCR_RD_FIELD(_A, _O, _ucShft, _u4Mask, pu4Val) \
{ \
	HAL_MCR_RD(_A, _O, pu4Val); \
	*pu4Val = ((*pu4Val & _u4Mask) >> _ucShft); \
}

static void HAL_DYNAMIC_MCR_RD(struct ADAPTER *ad,
			uint32_t addr, uint32_t *value)
{
	uint32_t u4BackupVal = 0;
	uint32_t u4Val = 0;
	/* set addr to ap2wf_public_remapping_0_start_address */
	HAL_MCR_WR(ad, 0x830c0120, addr);

	/* set 0x1850 to AP2CONN remapping register for 0x06 */
	HAL_MCR_RD(ad, 0x7c06080c, &u4BackupVal);
	u4Val = (u4BackupVal & 0xffff0000) | 0x1850;
	HAL_MCR_WR(ad, 0x7c06080c, u4Val);

	HAL_MCR_RD(ad, 0x7c500000, value);

	HAL_MCR_WR(ad, 0x7c06080c, u4BackupVal);
}
#endif /* _HIF_PCIE */

#if defined(_HIF_PCIE)
static void pcie_mt7926_dumpCbtopOffVcoreOn(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t addr = 0;

	dump = cbtop_off_vcore_on_dump_list;
	size = ARRAY_SIZE(cbtop_off_vcore_on_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			if ((dump[i].addr & 0xFFFF0000) == 0x740a0000 ||
				(dump[i].addr & 0xFFFF0000) == 0x70020000)
				addr = (dump[i].addr & 0x0000ffff) | 0x70000000;
			else
				addr = dump[i].addr;
			HAL_MCR_RD(ad, addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_MCR_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void pcie_mt7926_dumpCbtopOffVcoreOff(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexB = 1;

	dump = cbtop_off_vcore_off_dump_list;
	size = ARRAY_SIZE(cbtop_off_vcore_off_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_B%d=0x%08X=0x%08X\n",
				indexB, u4Val, dump[i].addr);
			indexB++;
		} else
			HAL_MCR_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void pcie_mt7926_dumpCbtopPcie(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexD = 1;

	dump = cbtop_pcie_dump_list;
	size = ARRAY_SIZE(cbtop_pcie_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_D%d=0x%08X=0x%08X\n",
				indexD, u4Val, dump[i].addr);
			indexD++;
		} else
			HAL_MCR_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void pcie_mt7926_dumpCbtopCr(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexF = 1;

	dump = cbtop_cr_dump_list;
	size = ARRAY_SIZE(cbtop_cr_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_F%d=0x%08X=0x%08X\n",
				indexF, u4Val, dump[i].addr);
			indexF++;
		} else
			HAL_MCR_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void pcie_mt7926_dumpCbtopReg(struct ADAPTER *ad)
{
	DBGLOG(HAL, INFO, "dump cbtop reg\n");
	pcie_mt7926_dumpCbtopOffVcoreOn(ad);
	pcie_mt7926_dumpCbtopOffVcoreOff(ad);
	pcie_mt7926_dumpCbtopPcie(ad);
	pcie_mt7926_dumpCbtopCr(ad);
}

static u_int8_t pcie_mt7926_ConninfraOffRdableChk(struct ADAPTER *ad)
{
#define CONNINFRA_OFF_POLLING_TIME 4
#define CONNINFRA_IP_VER 0x02060100
#define CONN_DBG_CTL_CONN_INFRA_BUS_CLK_DETECT_ADDR 0x7c060000
#define CONN_CFG_IP_VERSION_ADDR 0x7c001000
	uint32_t u4RdAddr = 0, u4Addr, u4Val = 0;
	uint32_t i = 0;

	u4Addr = CONN_DBG_CTL_CONN_INFRA_BUS_CLK_DETECT_ADDR;
	HAL_MCR_WR_FIELD(ad, u4Addr, 1, 0, 0x1);

	DBGLOG(HAL, INFO, "\tW 0x%08x[0]=[0x1]\n", u4Addr);

	for (i = 0; i <= CONNINFRA_OFF_POLLING_TIME; i++) {
		HAL_MCR_RD_FIELD(ad, u4Addr, 1, (BIT(1) | BIT(2)), &u4Val);

		if (u4Val == 0x3)
			break;
		else if (i == CONNINFRA_OFF_POLLING_TIME) {
			DBGLOG(HAL, INFO,
				"clock not exist, skip further sop dump!\n");

			return FALSE;
		}
		kalUsleep_range(900, 1000);
	}
	u4RdAddr = CONN_CFG_IP_VERSION_ADDR;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, INFO, "\tR 0x%08x=[0x%08x]\n", u4RdAddr, u4Val);

	if (u4Val != CONNINFRA_IP_VER) {
		DBGLOG(HAL, INFO,
			"IP ver not match, skip further sop dump!\n");
		return FALSE;
	}

	return TRUE;
}

static void pcie_mt7926_dumpConninfraPwrDbg(struct ADAPTER *ad,
					u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexB = 1;
	uint32_t indexC = 1;

	if (fgOnDomain) {
		dump = conninfra_pwr_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_2_1_B%d=0x%08X=0x%08X\n",
					indexB, u4Val, dump[i].addr);
				indexB++;
			} else
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	} else {
		dump = conninfra_pwr_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_pwr_off_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_2_1_C%d=0x%08X=0x%08X\n",
					indexC, u4Val, dump[i].addr);
				indexC++;
			} else
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	}
}

static void pcie_mt7926_dumpConninfraBusDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t indexB = 1;
	uint32_t indexC = 1;

	if (fgOnDomain) {
		dump = conninfra_bus_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_bus_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad,
					   dump[i].addr,
					   &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_1_1_A%d=0x%08X=0x%08X\n",
					indexA, u4Val, dump[i].addr);
				indexA++;
			} else
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	} else {
		dump = conninfra_bus_off_domain_dump_list_b;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_list_b);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_1_1_B%d=0x%08X=0x%08X\n",
					indexB, u4Val, dump[i].addr);
				indexB++;
			} else
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}

		dump = conninfra_bus_off_domain_dump_list_c;
		size = ARRAY_SIZE(conninfra_bus_off_domain_dump_list_c);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_1_1_C%d=0x%08X=0x%08X\n",
					indexC, u4Val, dump[i].addr);
				indexC++;
			} else
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	}
}


static void pcie_mt7926_dumpConninfraClkDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t indexB = 1;

	if (fgOnDomain) {
		dump = conninfra_clk_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_8_1_A%d=0x%08X=0x%08X\n",
					indexA, u4Val, dump[i].addr);
				indexA++;
			} else
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	} else {
		dump = conninfra_clk_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_off_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_8_1_B%d=0x%08X=0x%08X\n",
					indexB, u4Val, dump[i].addr);
				indexB++;
			} else
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	}
}

static void pcie_mt7926_dumpConninfraReg(struct ADAPTER *ad)
{
	/* Dump on domain */
	pcie_mt7926_dumpConninfraPwrDbg(ad, TRUE);
	pcie_mt7926_dumpConninfraBusDbg(ad, TRUE);
	pcie_mt7926_dumpConninfraClkDbg(ad, TRUE);

	if (pcie_mt7926_ConninfraOffRdableChk(ad) == FALSE)
		DBGLOG(HAL, INFO,
			"pcie_mt7926_ConninfraOffRdableChk fail\n");

	/* Dump off domain */
	pcie_mt7926_dumpConninfraPwrDbg(ad, FALSE);
	pcie_mt7926_dumpConninfraBusDbg(ad, FALSE);
	pcie_mt7926_dumpConninfraClkDbg(ad, FALSE);
}


static void pcie_mt7926_dumpWfTopMiscOn(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;

	dump = wf_top_dump_list_pcie;
	size = ARRAY_SIZE(wf_top_dump_list_pcie);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_3_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_MCR_WR(ad, dump[i].addr, dump[i].value);
	}
}

static u_int8_t pcie_mt7926_ConninfraAp2WfRdableWfPend(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0;

	u4RdAddr = 0x7C06016c;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, INFO, "\tR 0x%08x=[0x%08x]\n", u4RdAddr, u4Val);

	if ((u4Val & BIT(0)) == 0) {
		DBGLOG(HAL, INFO,
			"conn2wf WF BUS not Pend; skip further sop dump!\n");
		return FALSE;
	}
	return TRUE;
}

static u_int8_t pcie_mt7926_ConninfraAp2WfRdableChk(struct ADAPTER *ad)
{
#define WIFI_IP_VER 0x02050100

	uint32_t u4RdAddr, u4Val = 0;

	u4RdAddr = 0x7C001544;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, INFO, "\tR 0x%08x=[0x%08x]\n", u4RdAddr, u4Val);

	if ((u4Val & BIT(31)) != 0) {
		DBGLOG(HAL, INFO,
		  "conn2wf sleep protection is on, skip further sop dump!\n");
		return FALSE;
	}

	u4RdAddr = 0x80020010;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	DBGLOG(HAL, INFO, "\tR 0x%08x=[0x%08x]\n", u4RdAddr, u4Val);

	if (u4Val != WIFI_IP_VER) {
		DBGLOG(HAL, INFO,
			"wifi IP ver not match, skip further sop dump!\n");
		return FALSE;
	}

	return TRUE;
}

static void pcie_mt7926_dumpVdnrTimeoutHostSideInfo(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 0;

	dump = wf_bus_vdnr_timeout_host_side_dump_list;
	size = ARRAY_SIZE(wf_bus_vdnr_timeout_host_side_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_4_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_MCR_WR(ad, dump[i].addr, dump[i].value);
	}
}

static void pcie_mt7926_dumpVdnrTimeoutWfSideInfo(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexB = 0;

	HAL_MCR_RD(ad, 0x7c02362c, &u4Val);
	DBGLOG(HAL, INFO,
		"=PSOP_4_1_B%d=0x%08X=0x%08X\n",
		indexB, u4Val, 0x7c02362c);

	indexB = 2;
	dump = wf_bus_vdnr_timeout_wf_side_dump_list;
	size = ARRAY_SIZE(wf_bus_vdnr_timeout_wf_side_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_DYNAMIC_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_4_1_B%d=0x%08X=0x%08X\n",
				indexB, u4Val, dump[i].addr);
			indexB++;
		} else
			HAL_MCR_WR(ad, dump[i].addr, dump[i].value);
	}
}

static void pcie_mt7926_dumpAhbApbTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexC = 1;

	dump = wf_bus_ahb_apb_timeout_dump_list;
	size = ARRAY_SIZE(wf_bus_ahb_apb_timeout_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_DYNAMIC_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_4_1_C%d=0x%08X=0x%08X\n",
				indexC, u4Val, dump[i].addr);
			indexC++;
		} else
			HAL_MCR_WR(ad, dump[i].addr, dump[i].value);
	}
}

static void pcie_mt7926_dumpWfsysReg(struct ADAPTER *ad)
{
	if (pcie_mt7926_ConninfraAp2WfRdableWfPend(ad) == FALSE) {
		DBGLOG(HAL, INFO,
			"pcie_mt7926_ConninfraAp2WfRdableWfPend fail\n");
		return;
	}
	/* Section A: Dump wf_top_misc_on monflag */
	pcie_mt7926_dumpWfTopMiscOn(ad);

	/* Section A: Dump VDNR timeout host side info */
	pcie_mt7926_dumpVdnrTimeoutHostSideInfo(ad);

	if (pcie_mt7926_ConninfraAp2WfRdableChk(ad) == FALSE) {
		DBGLOG(HAL, INFO,
			"pcie_mt7926_ConninfraAp2WfRdableChk fail\n");
		return;
	}

	/* Section B: Dump VDNR timeout wf side info */
	pcie_mt7926_dumpVdnrTimeoutWfSideInfo(ad);

	/* Section C: Dump AHB APB timeout info */
	pcie_mt7926_dumpAhbApbTimeoutInfo(ad);
}

void pcie_mt7926_dump_power_debug_cr(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	uint32_t connsys_pwr_status_cr_addr = 0x7c0602cc;
	uint32_t dbg_7_cr_addr = 0x7c0602dc;
	uint32_t conn_bus_mcu_stat_dbg_cr_addr = 0x7c060000;

	HAL_MCR_RD(ad, connsys_pwr_status_cr_addr, &u4Val);
	DBGLOG(HAL, INFO,
		"connsys_pwr_status_cr_addr: 0x%08x, Val: 0x%08x\n",
		connsys_pwr_status_cr_addr, u4Val);

	HAL_MCR_RD(ad, dbg_7_cr_addr, &u4Val);
	DBGLOG(HAL, INFO,
		"dbg_7_cr_addr: 0x%08x, Val: 0x%08x\n",
		dbg_7_cr_addr, u4Val);

	HAL_MCR_RD(ad, conn_bus_mcu_stat_dbg_cr_addr, &u4Val);
	DBGLOG(HAL, INFO,
		"conn_bus_mcu_stat_dbg_cr_addr: 0x%08x, Val: 0x%08x\n",
		conn_bus_mcu_stat_dbg_cr_addr, u4Val);
}

void pcie_mt7926_dump_wfsys_debug_cr(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	uint32_t wf_sysstrap_rd_dbg = 0x7c060200;

	HAL_MCR_RD(ad, wf_sysstrap_rd_dbg, &u4Val);
	DBGLOG(HAL, INFO,
		"wf_sysstrap_rd_dbg: 0x%08x, Val: 0x%08x\n",
		wf_sysstrap_rd_dbg, u4Val);
	if (u4Val & BIT(31))
		pcie_mt7926_dumpWfTopMiscOn(ad);
}

void pcie_mt7926_dump_bgfsys_debug_cr(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	uint32_t zb_sysstrap_rd_dbg = 0x7c060240;

	HAL_MCR_RD(ad, zb_sysstrap_rd_dbg, &u4Val);
	DBGLOG(HAL, INFO,
		"zb_sysstrap_rd_dbg: 0x%08x, Val: 0x%08x\n",
		zb_sysstrap_rd_dbg, u4Val);
}

void pcie_mt7926_dump_zbsys_debug_cr(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	uint32_t bgf_sysstrap_rd_dbg = 0x7c060230;

	HAL_MCR_RD(ad, bgf_sysstrap_rd_dbg, &u4Val);
	DBGLOG(HAL, INFO,
		"bgf_sysstrap_rd_dbg: 0x%08x, Val: 0x%08x\n",
		bgf_sysstrap_rd_dbg, u4Val);
}
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
#define HAL_UHW_WR_FIELD(_prAdapter, _u4Offset, _u4FieldVal, _ucShft, _u4Mask) \
{ \
	uint32_t u4CrValue = 0; \
	u_int8_t fgStatus = FALSE; \
	HAL_UHW_RD(_prAdapter, _u4Offset, &u4CrValue, &fgStatus); \
	u4CrValue &= (~_u4Mask); \
	u4CrValue |= ((_u4FieldVal << _ucShft) & _u4Mask); \
	HAL_UHW_WR(_prAdapter, _u4Offset, u4CrValue, &fgStatus); \
}

static void usb_mt7926_dumpCbtopOffVcoreOn(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;

	dump = cbtop_off_vcore_on_dump_list;
	size = ARRAY_SIZE(cbtop_off_vcore_on_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_UHW_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void usb_mt7926_dumpCbtopOffVcoreOff(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexB = 1;

	dump = cbtop_off_vcore_off_dump_list;
	size = ARRAY_SIZE(cbtop_off_vcore_off_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_B%d=0x%08X=0x%08X\n",
				indexB, u4Val, dump[i].addr);
			indexB++;
		} else
			HAL_UHW_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void usb_mt7926_dumpCbtopUsb(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexD = 1;

	dump = cbtop_usb_dump_list;
	size = ARRAY_SIZE(cbtop_usb_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_E%d=0x%08X=0x%08X\n",
				indexD, u4Val, dump[i].addr);
			indexD++;
		} else
			HAL_UHW_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void usb_mt7926_dumpCbtopCr(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexF = 1;

	dump = cbtop_cr_dump_list;
	size = ARRAY_SIZE(cbtop_cr_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
			DBGLOG(HAL, INFO,
				"=PSOP_9_1_F%d=0x%08X=0x%08X\n",
				indexF, u4Val, dump[i].addr);
			indexF++;
		} else
			HAL_UHW_WR_FIELD(ad,
				dump[i].addr, dump[i].value,
				dump[i].shift, dump[i].mask);
	}
}

static void usb_mt7926_dumpCbtopReg(struct ADAPTER *ad)
{
	DBGLOG(HAL, INFO, "dump cbtop reg\n");
	usb_mt7926_dumpCbtopOffVcoreOn(ad);
	usb_mt7926_dumpCbtopOffVcoreOff(ad);
	usb_mt7926_dumpCbtopUsb(ad);
	usb_mt7926_dumpCbtopCr(ad);
}

static void usb_mt7926_dumpConninfraClkDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t indexB = 1;

	if (fgOnDomain) {
		dump = conninfra_clk_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
				DBGLOG(HAL, INFO,
					"=PSOP_8_1_A%d=0x%08X=0x%08X\n",
					indexA, u4Val, dump[i].addr);
				indexA++;
			} else
				HAL_UHW_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	} else {
		dump = conninfra_clk_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_off_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
				DBGLOG(HAL, INFO,
					"=PSOP_8_1_B%d=0x%08X=0x%08X\n",
					indexB, u4Val, dump[i].addr);
				indexB++;
			} else
				HAL_UHW_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
		}
	}
}

static void usb_mt7926_dumpConninfraReg(struct ADAPTER *ad)
{
	/* Dump on domain */
	usb_mt7926_dumpConninfraClkDbg(ad, TRUE);

	/* Dump off domain */
	usb_mt7926_dumpConninfraClkDbg(ad, FALSE);
}


static void usb_mt7926_dumpWfTopMiscOn(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;

	dump = wf_top_dump_list_usb;
	size = ARRAY_SIZE(wf_top_dump_list_usb);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
			DBGLOG(HAL, INFO,
				"=PSOP_3_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_UHW_WR(ad, dump[i].addr, dump[i].value, &fgStatus);
	}
}

static void usb_mt7926_dumpVdnrTimeoutHostSideInfo(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	u_int8_t fgStatus = FALSE;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 0;

	dump = wf_bus_vdnr_timeout_host_side_dump_list;
	size = ARRAY_SIZE(wf_bus_vdnr_timeout_host_side_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_UHW_RD(ad, dump[i].addr, &u4Val, &fgStatus);
			DBGLOG(HAL, INFO,
				"=PSOP_4_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_UHW_WR(ad, dump[i].addr, dump[i].value, &fgStatus);
	}
}


static void usb_mt7926_dumpWfsysReg(struct ADAPTER *ad)
{
	/* Section A: Dump wf_top_misc_on monflag */
	usb_mt7926_dumpWfTopMiscOn(ad);

	/* Section A: Dump VDNR timeout host side info */
	usb_mt7926_dumpVdnrTimeoutHostSideInfo(ad);
}

#endif /* _HIF_USB */

#if defined(_HIF_SDIO)
#define HAL_MCR_WR_FIELD(_prAdapter, _u4Offset, _u4FieldVal, _ucShft, _u4Mask) \
{ \
	uint32_t u4CrValue = 0; \
	HAL_MCR_RD(_prAdapter, _u4Offset, &u4CrValue); \
	u4CrValue &= (~_u4Mask); \
	u4CrValue |= ((_u4FieldVal << _ucShft) & _u4Mask); \
	HAL_MCR_WR(_prAdapter, _u4Offset, u4CrValue); \
}

static void sdio_mt7926_dumpConninfraClkDbg(struct ADAPTER *ad,
	u_int8_t fgOnDomain)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;
	uint32_t indexB = 1;

	if (fgOnDomain) {
		dump = conninfra_clk_on_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_on_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_8_1_A%d=0x%08X=0x%08X\n",
					indexA, u4Val, dump[i].addr);
				indexA++;
			} else {
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
			}
		}
	} else {
		dump = conninfra_clk_off_domain_dump_list;
		size = ARRAY_SIZE(conninfra_clk_off_domain_dump_list);
		for (i = 0; i < size; i++) {
			if (dump[i].read) {
				HAL_MCR_RD(ad, dump[i].addr, &u4Val);
				DBGLOG(HAL, INFO,
					"=PSOP_8_1_B%d=0x%08X=0x%08X\n",
					indexB, u4Val, dump[i].addr);
				indexB++;
			} else {
				HAL_MCR_WR_FIELD(ad,
					dump[i].addr, dump[i].value,
					dump[i].shift, dump[i].mask);
			}
		}
	}
}

static void sdio_mt7926_dumpConninfraReg(struct ADAPTER *ad)
{
	/* Dump on domain */
	sdio_mt7926_dumpConninfraClkDbg(ad, TRUE);

	/* Dump off domain */
	sdio_mt7926_dumpConninfraClkDbg(ad, FALSE);
}

static void sdio_mt7926_dumpWfTopMiscOn(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 1;

	dump = wf_top_dump_list_sdio;
	size = ARRAY_SIZE(wf_top_dump_list_sdio);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_3_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_MCR_WR(ad, dump[i].addr, dump[i].value);
	}
}

static void sdio_mt7926_dumpVdnrTimeoutHostSideInfo(struct ADAPTER *ad)
{
	uint32_t u4Val = 0;
	struct dump_cr_set *dump = NULL;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t indexA = 0;

	dump = wf_bus_vdnr_timeout_host_side_dump_list;
	size = ARRAY_SIZE(wf_bus_vdnr_timeout_host_side_dump_list);
	for (i = 0; i < size; i++) {
		if (dump[i].read) {
			HAL_MCR_RD(ad, dump[i].addr, &u4Val);
			DBGLOG(HAL, INFO,
				"=PSOP_4_1_A%d=0x%08X=0x%08X\n",
				indexA, u4Val, dump[i].addr);
			indexA++;
		} else
			HAL_MCR_WR(ad, dump[i].addr, dump[i].value);
	}
}

static void sdio_mt7926_dumpWfsysReg(struct ADAPTER *ad)
{
	/* Section A: Dump wf_top_misc_on monflag */
	sdio_mt7926_dumpWfTopMiscOn(ad);

	sdio_mt7926_dumpVdnrTimeoutHostSideInfo(ad);
}
#endif /* _HIF_SDIO */

u_int8_t mt7926_show_debug_sop_info(struct ADAPTER *prAdapter, uint8_t ucCase)
{
	switch (ucCase) {
	case SLEEP:
		/* Check power status */
		DBGLOG(HAL, ERROR, "Sleep Fail!\n");
#if defined(_HIF_PCIE)
		pcie_mt7926_dump_power_debug_cr(prAdapter);
		pcie_mt7926_dump_wfsys_debug_cr(prAdapter);
		pcie_mt7926_dump_bgfsys_debug_cr(prAdapter);
		pcie_mt7926_dump_zbsys_debug_cr(prAdapter);
#endif /* _HIF_PCIE */
		break;
	case SLAVENORESP:
		DBGLOG(HAL, ERROR, "Device no response!\n");
#if defined(_HIF_PCIE)
		pcie_mt7926_dumpCbtopReg(prAdapter);
		pcie_mt7926_dumpConninfraReg(prAdapter);
		pcie_mt7926_dumpWfsysReg(prAdapter);
#endif /* _HIF_PCIE */
#if defined(_HIF_USB)
		usb_mt7926_dumpCbtopReg(prAdapter);
		usb_mt7926_dumpConninfraReg(prAdapter);
		usb_mt7926_dumpWfsysReg(prAdapter);
#endif /* _HIF_USB */
#if defined(_HIF_SDIO)
		sdio_mt7926_dumpConninfraReg(prAdapter);
		sdio_mt7926_dumpWfsysReg(prAdapter);
#endif /* _HIF_SDIO */
		break;
	default:
		break;
	}

	return TRUE;
}
