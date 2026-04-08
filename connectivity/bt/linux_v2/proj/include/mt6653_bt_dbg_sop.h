/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "bt_hw_dbg.h"

#define MT6653_BT_DEBUGSOP_DUMP_VERSION "20240216"

const struct bt_dbg_command mt6653_bg_top_hostcsr_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0000000, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000001, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000002, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000003, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000004, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000005, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000100, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000101, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000102, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000103, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000104, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000105, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0000200, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_top_hostcsr_a = {
	"PSOP_5_1_A", "SectionA - [BG TOP vcore off] bg_bt_cfg_Debug_Signal, bg_bt_clkgen_off_Debug_Signal, bg_bt_met_Debug_Signal",
	13, sizeof(mt6653_bg_top_hostcsr_a)/sizeof(struct bt_dbg_command),
	mt6653_bg_top_hostcsr_a
};

const struct bt_dbg_command mt6653_bg_top_hostcsr_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a1c, 0, 0x300040, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300041, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300042, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300043, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300044, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300045, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300046, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300047, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300048, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300049, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30004a, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30004b, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30004c, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30004d, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30004e, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30004f, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300050, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300051, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300052, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300053, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300054, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300055, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300056, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300057, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300058, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300059, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300001, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300002, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300003, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300004, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300005, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300006, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300007, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300008, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300009, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30000a, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30000b, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30000c, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30000d, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x30000e, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300080, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300081, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300082, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300083, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300084, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300100, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300140, true, 0x20023a20},
};

const struct bt_dump_list mt6653_dump_list_bg_top_hostcsr_b = {
	"PSOP_5_1_B", "SectionB - [BG TOP vcore on] bg_bt_cfg_on_Debug_Signal, bg_bt_rgu_on_Debug_Signal, bg_bt_rgu_on_Debug_Signal",
	47, sizeof(mt6653_bg_top_hostcsr_b)/sizeof(struct bt_dbg_command),
	mt6653_bg_top_hostcsr_b
};

const struct bt_dbg_command mt6653_bg_top_hostcsr_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20060c04, 0, 0x30000, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30010, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30020, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30050, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30040, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30100, true, 0x20060c00},
};

const struct bt_dump_list mt6653_dump_list_bg_top_hostcsr_c = {
	"PSOP_5_1_C", "SectionC - [BG TOP vlp] bg_bt_misc_vlp_Debug_Signal, bg_bt_met_on_Debug_Signal, bg_bt_kmem_Debug_Signal",
	6, sizeof(mt6653_bg_top_hostcsr_c)/sizeof(struct bt_dbg_command),
	mt6653_bg_top_hostcsr_c
};

const struct bt_dbg_command mt6653_bg_top_apb_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{false, 0, 0, 0, true, 0x7c860000},
	{false, 0, 0, 0, true, 0x7c860020},
	{false, 0, 0, 0, true, 0x7c860028},
	{false, 0, 0, 0, true, 0x7c860030},
	{false, 0, 0, 0, true, 0x7c860034},
	{false, 0, 0, 0, true, 0x7c860128},
	{false, 0, 0, 0, true, 0x7c86012c},
	{false, 0, 0, 0, true, 0x7c821120},
	{false, 0, 0, 0, true, 0x7c821148},
	{false, 0, 0, 0, true, 0x7c821200},
	{false, 0, 0, 0, true, 0x7c821210},
	{false, 0, 0, 0, true, 0x7c821504},
	{false, 0, 0, 0, true, 0x7c821508},
	{false, 0, 0, 0, true, 0x7c82150c},
	{false, 0, 0, 0, true, 0x7c821510},
	{false, 0, 0, 0, true, 0x7c821640},
	{false, 0, 0, 0, true, 0x7c821644},
	{false, 0, 0, 0, true, 0x7c821648},
	{false, 0, 0, 0, true, 0x7c82164c},
	{false, 0, 0, 0, true, 0x7c821700},
	{false, 0, 0, 0, true, 0x7c820004},
	{false, 0, 0, 0, true, 0x7c820010},
	{false, 0, 0, 0, true, 0x7c82006c},
	{false, 0, 0, 0, true, 0x7c820088},
	{false, 0, 0, 0, true, 0x7c820090},
	{false, 0, 0, 0, true, 0x7c812100},
	{false, 0, 0, 0, true, 0x7c812104},
	{false, 0, 0, 0, true, 0x7c812108},
	{false, 0, 0, 0, true, 0x7c812114},
	{false, 0, 0, 0, true, 0x7c812118},
	{false, 0, 0, 0, true, 0x7c812120},
	{false, 0, 0, 0, true, 0x7c812124},
	{false, 0, 0, 0, true, 0x7c812150},
	{false, 0, 0, 0, true, 0x7c812168},
	{false, 0, 0, 0, true, 0x7c81216c},
	{false, 0, 0, 0, true, 0x7c812170},
	{false, 0, 0, 0, true, 0x7c812174},
	{false, 0, 0, 0, true, 0x7c812178},
	{false, 0, 0, 0, true, 0x7c81217c},
	{false, 0, 0, 0, true, 0x7c812400},
	{false, 0, 0, 0, true, 0x7c825230},
	{false, 0, 0, 0, true, 0x7c82523c},
};

const struct bt_dump_list mt6653_dump_list_bg_top_apb_a = {
	"PSOP_5_2_A", "SectionA - [BG TOP APB] bg_bt_top_misc_vlp_reg, bg_bt_cfg_on_reg, bg_bt_rgu_on_reg, bg_bt_cfg_reg",
	40, sizeof(mt6653_bg_top_apb_a)/sizeof(struct bt_dbg_command),
	mt6653_bg_top_apb_a
};

const struct bt_dbg_command mt6653_bt_mcu_bus_timeout_hostcsr_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0041f00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f0a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f0b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f0c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f0d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f0e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f0f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041f10, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_a = {
	"PSOP_8_2_A", "SectionA - Bus timeout CR",
	17, sizeof(mt6653_bt_mcu_bus_timeout_hostcsr_a)/sizeof(struct bt_dbg_command),
	mt6653_bt_mcu_bus_timeout_hostcsr_a
};

const struct bt_dbg_command mt6653_bt_mcu_bus_timeout_hostcsr_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0041910, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041920, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041930, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041940, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041950, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041960, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041970, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041980, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041990, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_b = {
	"PSOP_8_2_B", "SectionB - bus vdnr debug probe",
	9, sizeof(mt6653_bt_mcu_bus_timeout_hostcsr_b)/sizeof(struct bt_dbg_command),
	mt6653_bt_mcu_bus_timeout_hostcsr_b
};

const struct bt_dbg_command mt6653_bt_mcu_bus_timeout_hostcsr_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0041b00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b0a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b0b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b0c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b0d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b0e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041b0f, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_c = {
	"PSOP_8_2_C", "SectionC - bus manual debug probe",
	16, sizeof(mt6653_bt_mcu_bus_timeout_hostcsr_c)/sizeof(struct bt_dbg_command),
	mt6653_bt_mcu_bus_timeout_hostcsr_c
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x7c800a00, 0x1, 0x0, false, 0},
	{true, 0x20023a0c, 0, 0xc0040200, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040201, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040202, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040203, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040204, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040205, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040206, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040207, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040208, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040209, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004020a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004020b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004020c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004020d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004020e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004020f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040210, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040211, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040212, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040213, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040214, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040215, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040216, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040217, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040218, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040219, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004021a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004021b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004021c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004021d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004021e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004021f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040220, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040221, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040222, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040223, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040224, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040225, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040400, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040401, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040402, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040403, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_a = {
	"PSOP_6_1_A", "SectionA - [BG MCU core registers] mcu_flg1, mcu_flg2",
	42, sizeof(mt6653_bg_mcu_off_hostcsr_a)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_a
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0040600, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040800, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040801, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040802, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040803, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040804, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040805, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040806, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040807, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040808, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040809, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004080a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004080b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004080c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004080d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004080e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004080f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040810, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040811, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040812, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040813, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040814, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040815, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040816, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040817, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_b = {
	"PSOP_6_1_B", "SectionB - [BG DSP debug flags] - mcu_flg3, mcu_flg4",
	25, sizeof(mt6653_bg_mcu_off_hostcsr_b)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_b
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0040a00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040a01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040a02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040a03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c0a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c0b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c0c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c0d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c0e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040c0f, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_c = {
	"PSOP_6_1_C", "SectionC - [BG MCUSYS CLK and GALS debug flags] - mcu_flg5, mcu_flg6",
	20, sizeof(mt6653_bg_mcu_off_hostcsr_c)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_c
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_d[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0040e00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e10, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e11, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e12, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e13, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e14, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e15, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e16, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e17, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e18, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e19, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e1a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e1b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e1c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e1d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e1e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e1f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e20, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e21, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e22, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e23, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e24, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e25, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e26, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e27, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e28, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e29, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e2a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e2b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e2c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e2d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e2e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e2f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e30, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e31, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e32, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e33, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e34, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e35, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e36, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e37, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e38, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e39, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e3a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e3b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e3c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e3d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e3e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e3f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e40, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e41, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e42, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e43, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e44, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e45, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e46, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e47, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e48, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e49, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e4a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e4b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e4c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e4d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e4e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e4f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e50, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e51, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e52, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e53, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e54, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_d = {
	"PSOP_6_1_D", "SectionD - [BG MCU PC/LR log] - mcu_flg7[84:168] cpu_dbg_pc_log0 ~ conn_debug_port84",
	85, sizeof(mt6653_bg_mcu_off_hostcsr_d)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_d
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_e[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0040e55, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e56, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e57, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e58, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e59, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e5a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e5b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e5c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e5d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e5e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e5f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e60, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e61, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e62, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e63, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e64, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e65, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e66, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e67, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e68, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e69, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e6a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e6b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e6c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e6d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e6e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e6f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e70, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e71, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e72, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e73, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e74, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e75, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e76, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e77, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e78, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e79, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e7a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e7b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e7c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e7d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e7e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e7f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e80, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e81, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e82, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e83, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e84, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e85, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e86, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e87, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e88, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e89, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e8a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e8b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e8c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e8d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e8e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e8f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e90, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e91, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e92, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e93, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e94, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e95, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e96, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e97, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e98, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_e = {
	"PSOP_6_1_E", "SectionE - [BG DSP PC/LR log] - mcu_flg7[169:236] cpu1_dbg_pc_log0 ~ cpu1_lr",
	68, sizeof(mt6653_bg_mcu_off_hostcsr_e)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_e
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_f[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0041002, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041003, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041004, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041005, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041006, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041007, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041008, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041009, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041800, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041801, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041802, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041803, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041804, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041805, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042200, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042400, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_f = {
	"PSOP_6_1_F", "SectionF - [BG SEC] - mcu_flg8, mcu_flg12, mcu_flg17, mcu_flg18",
	16, sizeof(mt6653_bg_mcu_off_hostcsr_f)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_f
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_g[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0041600, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041601, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041602, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041603, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_g = {
	"PSOP_6_1_G", "SectionG - [BG MEM sleep] mcu_flg11",
	4, sizeof(mt6653_bg_mcu_off_hostcsr_g)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_g
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_h[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0041a01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041a11, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041a21, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041a31, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0041a41, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e0f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e10, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0040e11, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042000, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042001, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042002, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042003, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042004, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042005, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042006, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042007, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042008, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042009, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004200a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004200b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004200c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004200d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004200e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004200f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042010, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_h = {
	"PSOP_6_1_H", "SectionH - [BG BUS debug flags] - mcu_flg13, mcu_flg14, mcu_flg16",
	40, sizeof(mt6653_bg_mcu_off_hostcsr_h)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_h
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_i[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0042600, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042601, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042602, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042603, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042604, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042605, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042606, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042607, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042608, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042609, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004260a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042800, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042a00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042c00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e0a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e0b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e0c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e0d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e10, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e11, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e12, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e13, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e14, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e15, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e16, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e17, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e18, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e19, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e1a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e1b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e1c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0042e1d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043000, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043001, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043002, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043003, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043004, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043005, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043006, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043007, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043008, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043009, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004300a, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004300b, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004300c, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004300d, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004300e, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004300f, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc004301e, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_i = {
	"PSOP_6_1_I", "SectionI - [BG DMA and UART debug flags] - mcu_flg19, mcu_flg20, mcu_flg21, mcu_flg22, mcu_flg23, mcu_flg24",
	59, sizeof(mt6653_bg_mcu_off_hostcsr_i)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_i
};

const struct bt_dbg_command mt6653_bg_mcu_off_hostcsr_j[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0043400, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0043000, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_off_hostcsr_j = {
	"PSOP_6_1_J", "SectionJ - [BG PC] - mcu_flg26, mcu_flg29",
	2, sizeof(mt6653_bg_mcu_off_hostcsr_j)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_off_hostcsr_j
};

const struct bt_dbg_command mt6653_bg_mcu_on_hostcsr_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a1c, 0, 0x300d80, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300dc0, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300e00, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300e40, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300e80, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300ec0, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f00, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f40, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f80, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f81, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f82, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f83, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f84, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f85, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f86, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f87, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f88, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f89, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f8a, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300f8b, true, 0x20023a20},
	{true, 0x20023a1c, 0, 0x300fc0, true, 0x20023a20},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_on_hostcsr_a = {
	"PSOP_6_2_A", "SectionA - [BG MCU ON]",
	21, sizeof(mt6653_bg_mcu_on_hostcsr_a)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_on_hostcsr_a
};

const struct bt_dbg_command mt6653_bg_mcu_vlp_hostcsr_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20060c04, 0, 0x30510, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30511, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30512, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30520, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30530, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30540, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30550, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30551, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30552, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30553, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30554, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30555, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30556, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30560, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30570, true, 0x20060c00},
	{true, 0x20060c04, 0, 0x30580, true, 0x20060c00},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_vlp_hostcsr_a = {
	"PSOP_6_3_A", "SectionA - [BG MCU VLP]",
	16, sizeof(mt6653_bg_mcu_vlp_hostcsr_a)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_vlp_hostcsr_a
};

const struct bt_dbg_command mt6653_bg_mcu_common_off_hostcsr_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{true, 0x20023a0c, 0, 0xc0050000, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050100, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050241, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050300, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050400, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050500, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050600, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050700, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050800, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050900, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050a00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050b09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c01, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c02, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c03, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c04, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c05, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c06, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c07, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c08, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050c09, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050d00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050e00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0050f00, true, 0x20023a10},
	{true, 0x20023a0c, 0, 0xc0051000, true, 0x20023a10},
};

const struct bt_dump_list mt6653_dump_list_bg_mcu_common_off_hostcsr_a = {
	"PSOP_6_4_A", "SectionA - [BG MCU common off]",
	35, sizeof(mt6653_bg_mcu_common_off_hostcsr_a)/sizeof(struct bt_dbg_command),
	mt6653_bg_mcu_common_off_hostcsr_a
};
