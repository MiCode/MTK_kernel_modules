/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "connv3_hw_dbg.h"

#define MT6653_CONNINFRA_DEBUGSOP_DUMP_VERSION "20240620"

const struct connv3_dbg_command mt6653_conn_infra_bus_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023408, 0, 0x0, false, 0},
	{true, 0x2002340c, 0, 0x10001, true, 0x20023404},
	{true, 0x2002340c, 0, 0x20001, true, 0x20023404},
	{true, 0x2002340c, 0, 0x30001, true, 0x20023404},
	{true, 0x2002340c, 0, 0x10002, true, 0x20023404},
	{true, 0x2002340c, 0, 0x3, true, 0x20023404},
	{true, 0x2002340c, 0, 0x10003, true, 0x20023404},
	{true, 0x2002340c, 0, 0x20003, true, 0x20023404},
	{true, 0x2002340c, 0, 0x30003, true, 0x20023404},
	{true, 0x2002340c, 0, 0x10004, true, 0x20023404},
	{true, 0x2002340c, 0, 0x20004, true, 0x20023404},
	{true, 0x2002340c, 0, 0x30004, true, 0x20023404},
	{false, 0, 0, 0, true, 0x20023400},
	{false, 0, 0, 0, true, 0x20023410},
	{false, 0, 0, 0, true, 0x20023414},
	{false, 0, 0, 0, true, 0x20023418},
	{false, 0, 0, 0, true, 0x20023434},
	{false, 0, 0, 0, true, 0x20023b04},
	{false, 0, 0, 0, true, 0x2002344c},
	{false, 0, 0, 0, true, 0x20023450},
	{false, 0, 0, 0, true, 0x20023480},
	{false, 0, 0, 0, true, 0x20023484},
	{false, 0, 0, 0, true, 0x2002341c},
	{false, 0, 0, 0, true, 0x20023420},
	{false, 0, 0, 0, true, 0x20023424},
	{false, 0, 0, 0, true, 0x20023428},
	{false, 0, 0, 0, true, 0x2002342c},
	{false, 0, 0, 0, true, 0x20023430},
	{true, 0x20023408, 0, 0x13, true, 0x20023404},
	{true, 0x20023408, 0, 0x14, true, 0x20023404},
	{true, 0x20023408, 0, 0x15, true, 0x20023404},
	{true, 0x20023408, 0, 0x16, true, 0x20023404},
	{true, 0x20023408, 0, 0x17, true, 0x20023404},
	{true, 0x20023408, 0, 0x18, true, 0x20023404},
	{true, 0x20023408, 0, 0x19, true, 0x20023404},
	{true, 0x20023408, 0, 0x1a, true, 0x20023404},
	{true, 0x20023408, 0, 0x1b, true, 0x20023404},
	{true, 0x20023408, 0, 0x1c, true, 0x20023404},
	{true, 0x20023408, 0, 0x1d, true, 0x20023404},
	{true, 0x20023408, 0, 0x1e, true, 0x20023404},
	{true, 0x20023408, 0, 0x1f, true, 0x20023404},
	{true, 0x20023408, 0, 0x20, true, 0x20023404},
	{true, 0x20023408, 0, 0x21, true, 0x20023404},
	{true, 0x20023408, 0, 0x22, true, 0x20023404},
	{true, 0x20023408, 0, 0x23, true, 0x20023404},
	{true, 0x20023408, 0, 0x24, true, 0x20023404},
	{true, 0x20023408, 0, 0x25, true, 0x20023404},
	{true, 0x20023408, 0, 0x26, true, 0x20023404},
	{true, 0x20023408, 0, 0x27, true, 0x20023404},
	{true, 0x20023408, 0, 0x28, true, 0x20023404},
	{true, 0x20023408, 0, 0x29, true, 0x20023404},
	{true, 0x20023408, 0, 0x2a, true, 0x20023404},
	{true, 0x20023408, 0, 0x2b, true, 0x20023404},
	{true, 0x20023408, 0, 0x2c, true, 0x20023404},
	{true, 0x20023408, 0, 0x2d, true, 0x20023404},
	{true, 0x20023408, 0, 0x1, true, 0x20023404},
	{true, 0x20023408, 0, 0x2, true, 0x20023404},
	{true, 0x20023408, 0, 0x3, true, 0x20023404},
	{true, 0x20023408, 0, 0x4, true, 0x20023404},
	{true, 0x20023408, 0, 0x5, true, 0x20023404},
	{true, 0x20023408, 0, 0x6, true, 0x20023404},
	{true, 0x20023408, 0, 0x7, true, 0x20023404},
	{true, 0x20023408, 0, 0x8, true, 0x20023404},
	{true, 0x20023408, 0, 0x9, true, 0x20023404},
	{true, 0x20023408, 0, 0xa, true, 0x20023404},
	{true, 0x20023408, 0, 0xb, true, 0x20023404},
	{true, 0x20023408, 0, 0xc, true, 0x20023404},
	{true, 0x20023408, 0, 0xd, true, 0x20023404},
	{true, 0x20023408, 0, 0xe, true, 0x20023404},
	{true, 0x20023408, 0, 0xf, true, 0x20023404},
	{true, 0x20023408, 0, 0x10, true, 0x20023404},
	{true, 0x20023408, 0, 0x11, true, 0x20023404},
	{true, 0x20023408, 0, 0x12, true, 0x20023404}, //A72
	{true, 0x20023408, 0, 0x2e, true, 0x20023404}, //A73
	{false, 0, 0, 0, true, 0x20023454},
	{false, 0, 0, 0, true, 0x20023458},
};

const struct connv3_dump_list mt6653_dump_list_conn_infra_bus_a = {
	"PSOP_1_1_A", "SectionA - IF CONN_INFRA ON read check ok",
	75, sizeof(mt6653_conn_infra_bus_a)/sizeof(struct connv3_dbg_command),
	mt6653_conn_infra_bus_a
};

const struct connv3_dbg_command mt6653_conn_infra_bus_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x7c04f280},
	{false, 0, 0, 0, true, 0x7c04f284},
	{false, 0, 0, 0, true, 0x7c04f288},
	{false, 0, 0, 0, true, 0x7c04f28c},
	{false, 0, 0, 0, true, 0x7c04f290},
	{false, 0, 0, 0, true, 0x7c04f294},
	{false, 0, 0, 0, true, 0x7c04f298},
	{false, 0, 0, 0, true, 0x7c04f29c},
	{false, 0, 0, 0, true, 0x7c04f2a0},
	{false, 0, 0, 0, true, 0x7c049408},
	{false, 0, 0, 0, true, 0x7c04940c},
	{false, 0, 0, 0, true, 0x7c049410},
	{false, 0, 0, 0, true, 0x7c049414},
	{false, 0, 0, 0, true, 0x7c049418},
	{false, 0, 0, 0, true, 0x7c04941c},
	{false, 0, 0, 0, true, 0x7c049420},
	{false, 0, 0, 0, true, 0x7c049424},
	{false, 0, 0, 0, true, 0x7c049428},
	{false, 0, 0, 0, true, 0x7c04942c},
	{false, 0, 0, 0, true, 0x7c049430},
	{false, 0, 0, 0, true, 0x7c04f650},
	{false, 0, 0, 0, true, 0x7c04f700},
	{false, 0, 0, 0, true, 0x20060d04},
	{false, 0, 0, 0, true, 0x20060d08},
	{false, 0, 0, 0, true, 0x20060d0c},
	{false, 0, 0, 0, true, 0x7c04f610},
	{false, 0, 0, 0, true, 0x7c04f614},
	{false, 0, 0, 0, true, 0x7c04f618},
	{false, 0, 0, 0, true, 0x7c04f61c},
	{false, 0, 0, 0, true, 0x7c049000},
	{false, 0, 0, 0, true, 0x7c049004},
	{false, 0, 0, 0, true, 0x7c049008},
	{false, 0, 0, 0, true, 0x7c04900c},
	{false, 0, 0, 0, true, 0x7c049010},
	{false, 0, 0, 0, true, 0x7c049014},
	{false, 0, 0, 0, true, 0x7c00e128},
	{false, 0, 0, 0, true, 0x7c00e12c},
	{false, 0, 0, 0, true, 0x7c00e130},
	{false, 0, 0, 0, true, 0x7c00e134},
	{false, 0, 0, 0, true, 0x7c00e138},
	{false, 0, 0, 0, true, 0x7c00e13c},
	{false, 0, 0, 0, true, 0x7c00e140},
	{false, 0, 0, 0, true, 0x7c00e110},
};

const struct connv3_dump_list mt6653_dump_list_conn_infra_bus_b = {
	"PSOP_1_1_B", "SectionB - IF CONN_INFRA OFF read check ok - bus",
	43, sizeof(mt6653_conn_infra_bus_b)/sizeof(struct connv3_dbg_command),
	mt6653_conn_infra_bus_b
};

const struct connv3_dbg_command mt6653_conn_infra_bus_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x7c011404},
	{false, 0, 0, 0, true, 0x7c011408},
	{false, 0, 0, 0, true, 0x7c011434},
	{false, 0, 0, 0, true, 0x7c0114a4},
	{false, 0, 0, 0, true, 0x7c011454},
	{false, 0, 0, 0, true, 0x7c011464},
	{false, 0, 0, 0, true, 0x7c011474},
	{false, 0, 0, 0, true, 0x7c011484},
	{false, 0, 0, 0, true, 0x7c0114d4},
	{false, 0, 0, 0, true, 0x7c001414},
	{false, 0, 0, 0, true, 0x7c001424},
	{false, 0, 0, 0, true, 0x7c001434},
	{false, 0, 0, 0, true, 0x7c001454},
	{false, 0, 0, 0, true, 0x70028730},
};

const struct connv3_dump_list mt6653_dump_list_conn_infra_bus_c = {
	"PSOP_1_1_C", "SectionC - IF CONN_INFRA OFF read check ok - slpprot",
	14, sizeof(mt6653_conn_infra_bus_c)/sizeof(struct connv3_dbg_command),
	mt6653_conn_infra_bus_c
};

const struct connv3_dbg_command mt6653_conn_infra_bus_d[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x20020074},
	{false, 0, 0, 0, true, 0x20020078},
	{false, 0, 0, 0, true, 0x2002007c},
	{false, 0, 0, 0, true, 0x20020080},
	{false, 0, 0, 0, true, 0x20020084},
	{false, 0, 0, 0, true, 0x20020088},
	{false, 0, 0, 0, true, 0x2002008c},
};

const struct connv3_dump_list mt6653_dump_list_conn_infra_bus_d = {
	"PSOP_1_1_D", "SectionD - IF through von TO,force conn_wake_up read check ok",
	7, sizeof(mt6653_conn_infra_bus_d)/sizeof(struct connv3_dbg_command),
	mt6653_conn_infra_bus_d
};

const struct connv3_dbg_command mt6653_connsys_power_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x20060a10},
	{false, 0, 0, 0, true, 0x20060014},
	{false, 0, 0, 0, true, 0x20060054},
	{false, 0, 0, 0, true, 0x20060010},
	{false, 0, 0, 0, true, 0x20060050},
	{false, 0, 0, 0, true, 0x20060018},
	{false, 0, 0, 0, true, 0x20060058},
};

const struct connv3_dump_list mt6653_dump_list_connsys_power_b = {
	"PSOP_2_1_B", "SectionB - Dump after conn_infra_on is ready",
	7, sizeof(mt6653_connsys_power_b)/sizeof(struct connv3_dbg_command),
	mt6653_connsys_power_b
};

const struct connv3_dbg_command mt6653_connsys_power_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x7c011030},
	{false, 0, 0, 0, true, 0x7c012050},
	{false, 0, 0, 0, true, 0x7c001344},
	{false, 0, 0, 0, true, 0x7c000400},
	{false, 0, 0, 0, true, 0x7c000404},
	{false, 0, 0, 0, true, 0x20095204},
	{false, 0, 0, 0, true, 0x200910a8},
	{false, 0, 0, 0, true, 0x20091120},
	{false, 0, 0, 0, true, 0x20091124},
	{false, 0, 0, 0, true, 0x20091128},
	{false, 0, 0, 0, true, 0x2009112c},
	{false, 0, 0, 0, true, 0x20091130},
	{false, 0, 0, 0, true, 0x20091134},
	{true, 0x7c011100, 0x600000, 0x0, true, 0x7c011134},
	{true, 0x7c011100, 0x600000, 0x200000, true, 0x7c011134},
	{true, 0x7c011100, 0x600000, 0x400000, true, 0x7c011134},
	{true, 0x7c011100, 0x600000, 0x600000, true, 0x7c011134},
	{false, 0, 0, 0, true, 0x7c050c50},
	{false, 0, 0, 0, true, 0x7c050c54},
	{false, 0, 0, 0, true, 0x7c050c58},
	{false, 0, 0, 0, true, 0x7c050c5c},
	{false, 0, 0, 0, true, 0x7c050c60},
	{false, 0, 0, 0, true, 0x7c050c64},
	{false, 0, 0, 0, true, 0x7c098000},
	{false, 0, 0, 0, true, 0x7c098050},
	{false, 0, 0, 0, true, 0x7c098054},
	{false, 0, 0, 0, true, 0x7c098058},
	{false, 0, 0, 0, true, 0x7c098108},
	{false, 0, 0, 0, true, 0x7c098004},
	{false, 0, 0, 0, true, 0x7c001620},
	{false, 0, 0, 0, true, 0x7c001610},
	{false, 0, 0, 0, true, 0x7c001600},
};

const struct connv3_dump_list mt6653_dump_list_connsys_power_c = {
	"PSOP_2_1_C", "SectionC - Dump after conn_infra_off is ready",
	32, sizeof(mt6653_connsys_power_c)/sizeof(struct connv3_dbg_command),
	mt6653_connsys_power_c
};

const struct connv3_dbg_command mt6653_conn_infra_top_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x20060a00},
	{false, 0, 0, 0, true, 0x20060a0c},
	{true, 0x2006015c, 0x7, 0x0, true, 0x20060a04},
	{true, 0x2006015c, 0x7, 0x1, true, 0x20060a04},
	{true, 0x2006015c, 0x7, 0x2, true, 0x20060a04},
	{true, 0x2006015c, 0x7, 0x3, true, 0x20060a04},
	{true, 0x2006015c, 0x7, 0x4, true, 0x20060a04},
	{true, 0x2006015c, 0x7, 0x5, true, 0x20060a04},
	{true, 0x2006015c, 0x7, 0x6, true, 0x20060a04},
	{true, 0x2006015c, 0x7, 0x7, true, 0x20060a04},
	{true, 0x20060160, 0xf, 0x0, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x1, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x2, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x3, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x4, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x5, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x6, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x7, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x8, true, 0x20060a08},
	{true, 0x20060160, 0xf, 0x9, true, 0x20060a08},
	{false, 0, 0, 0, true, 0x20060740},
	{false, 0, 0, 0, true, 0x200b0000},
	{false, 0, 0, 0, true, 0x200b0004},
	{false, 0, 0, 0, true, 0x200b0008},
	{false, 0, 0, 0, true, 0x200b000c},
	{false, 0, 0, 0, true, 0x200b0010},
	{false, 0, 0, 0, true, 0x200b0014},
	{false, 0, 0, 0, true, 0x200b0018},
	{false, 0, 0, 0, true, 0x200b001c},
	{false, 0, 0, 0, true, 0x200b0020},
	{false, 0, 0, 0, true, 0x200b0024},
	{false, 0, 0, 0, true, 0x200b0028},
	{false, 0, 0, 0, true, 0x200b002c},
	{false, 0, 0, 0, true, 0x200b0030},
	{false, 0, 0, 0, true, 0x200b0034},
	{false, 0, 0, 0, true, 0x200b0038},
	{false, 0, 0, 0, true, 0x20023200},
};

const struct connv3_dump_list mt6653_dump_list_conn_infra_top_a = {
	"PSOP_7_1_A", "SectionA - Dump after conn_infra_on is ready",
	37, sizeof(mt6653_conn_infra_top_a)/sizeof(struct connv3_dbg_command),
	mt6653_conn_infra_top_a
};

const struct connv3_dbg_command mt6653_conn_infra_top_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x7c011130},
	{true, 0x7c011100, 0x600000, 0x0, true, 0x7c011134},
	{true, 0x7c011100, 0x600000, 0x200000, true, 0x7c011134},
	{true, 0x7c011100, 0x600000, 0x400000, true, 0x7c011134},
	{true, 0x7c011100, 0x600000, 0x600000, true, 0x7c011134},
	{true, 0x7c01603c, 0x3f, 0xe, false, 0},
	{true, 0x7c01603c, 0x3f00, 0x1100, false, 0},
	{true, 0x7c01601c, 0, 0x3020100, false, 0},
	{true, 0x7c016020, 0, 0x7060504, false, 0},
	{true, 0x7c016024, 0, 0xb0a0908, false, 0},
	{true, 0x7c016028, 0, 0xf0e0d0c, false, 0},
	{true, 0x7c01602c, 0, 0x13121110, false, 0},
	{true, 0x7c016030, 0, 0x17161514, false, 0},
	{true, 0x7c016034, 0, 0x1b1a1918, false, 0},
	{true, 0x7c016038, 0, 0x1f1e1d1c, false, 0},
	{true, 0x7c016058, 0x2, 0x2, true, 0x20023200},
	{true, 0x7c01603c, 0x3f, 0x18, false, 0},
	{true, 0x7c01603c, 0x3f00, 0x0, false, 0},
	{true, 0x7c01601c, 0, 0x3020100, false, 0},
	{true, 0x7c016020, 0, 0x7060504, false, 0},
	{true, 0x7c016024, 0, 0xb0a0908, false, 0},
	{true, 0x7c016028, 0, 0xf0e0d0c, false, 0},
	{true, 0x7c01602c, 0, 0x13121110, false, 0},
	{true, 0x7c016030, 0, 0x17161514, false, 0},
	{true, 0x7c016034, 0, 0x1b1a1918, false, 0},
	{true, 0x7c016038, 0, 0x1f1e1d1c, false, 0},
	{true, 0x7c016058, 0x2, 0x2, true, 0x20023200},
	{true, 0x20093080, 0x8, 0x0, false, 0},
	{true, 0x20097058, 0, 0x2, false, 0},
	{true, 0x2009703c, 0x3f, 0x14, false, 0},
	{true, 0x2009703c, 0x3f00, 0x1500, false, 0},
	{true, 0x2009701c, 0, 0x3020100, false, 0},
	{true, 0x20097020, 0, 0x7060504, false, 0},
	{true, 0x20097024, 0, 0xb0a0908, false, 0},
	{true, 0x20097028, 0, 0xf0e0d0c, false, 0},
	{true, 0x2009702c, 0, 0x13121110, false, 0},
	{true, 0x20097030, 0, 0x17161514, false, 0},
	{true, 0x20097034, 0, 0x1b1a1918, false, 0},
	{true, 0x20097038, 0, 0x1f1e1d1c, true, 0x20097054},
	{true, 0x20093080, 0x8, 0x8, false, 0},
	{true, 0x20097058, 0, 0x2, false, 0},
	{true, 0x2009703c, 0x3f, 0x14, false, 0},
	{true, 0x2009703c, 0x3f00, 0x1500, false, 0},
	{true, 0x2009701c, 0, 0x3020100, false, 0},
	{true, 0x20097020, 0, 0x7060504, false, 0},
	{true, 0x20097024, 0, 0xb0a0908, false, 0},
	{true, 0x20097028, 0, 0xf0e0d0c, false, 0},
	{true, 0x2009702c, 0, 0x13121110, false, 0},
	{true, 0x20097030, 0, 0x17161514, false, 0},
	{true, 0x20097034, 0, 0x1b1a1918, false, 0},
	{true, 0x20097038, 0, 0x1f1e1d1c, true, 0x20097054},
};

const struct connv3_dump_list mt6653_dump_list_conn_infra_top_b = {
	"PSOP_7_1_B", "SectionB - Dump after conn_infra_off is ready",
	9, sizeof(mt6653_conn_infra_top_b)/sizeof(struct connv3_dbg_command),
	mt6653_conn_infra_top_b
};
