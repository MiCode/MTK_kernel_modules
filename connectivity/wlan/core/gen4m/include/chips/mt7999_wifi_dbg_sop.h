/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "wlan_hw_dbg.h"

#define MT7999_WIFI_DEBUGSOP_DUMP_VERSION "20240416"

const struct wlan_dbg_command mt7999_cb_infra_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x70025000},
	{FALSE, 0, 0, 0, TRUE, 0x70025004},
	{FALSE, 0, 0, 0, TRUE, 0x70025008},
	{FALSE, 0, 0, 0, TRUE, 0x7002500c},
	{FALSE, 0, 0, 0, TRUE, 0x70025010},
	{FALSE, 0, 0, 0, TRUE, 0x70025090},
	{FALSE, 0, 0, 0, TRUE, 0x70025094},
	{FALSE, 0, 0, 0, TRUE, 0x70025014},
	{FALSE, 0, 0, 0, TRUE, 0x70025018},
	{FALSE, 0, 0, 0, TRUE, 0x7002501c},
	{FALSE, 0, 0, 0, TRUE, 0x70025020},
	{FALSE, 0, 0, 0, TRUE, 0x70025024},
	{FALSE, 0, 0, 0, TRUE, 0x70025028},
	{FALSE, 0, 0, 0, TRUE, 0x7002502c},
	{FALSE, 0, 0, 0, TRUE, 0x70025030},
	{FALSE, 0, 0, 0, TRUE, 0x70025040},
	{FALSE, 0, 0, 0, TRUE, 0x70025044},
	{FALSE, 0, 0, 0, TRUE, 0x70025050},
	{FALSE, 0, 0, 0, TRUE, 0x70025054},
	{FALSE, 0, 0, 0, TRUE, 0x7002505c},
	{FALSE, 0, 0, 0, TRUE, 0x70025060},
	{FALSE, 0, 0, 0, TRUE, 0x70025064},
	{FALSE, 0, 0, 0, TRUE, 0x70025070},
	{FALSE, 0, 0, 0, TRUE, 0x70025074},
	{FALSE, 0, 0, 0, TRUE, 0x70025080},
	{FALSE, 0, 0, 0, TRUE, 0x70025084},
	{FALSE, 0, 0, 0, TRUE, 0x700250f0},
	{FALSE, 0, 0, 0, TRUE, 0x70025100},
	{FALSE, 0, 0, 0, TRUE, 0x70025104},
	{FALSE, 0, 0, 0, TRUE, 0x70025108},
	{FALSE, 0, 0, 0, TRUE, 0x7002510c},
	{FALSE, 0, 0, 0, TRUE, 0x70025110},
	{FALSE, 0, 0, 0, TRUE, 0x70025114},
	{FALSE, 0, 0, 0, TRUE, 0x70025118},
	{FALSE, 0, 0, 0, TRUE, 0x70025160},
	{FALSE, 0, 0, 0, TRUE, 0x70025164},
	{FALSE, 0, 0, 0, TRUE, 0x70025168},
	{FALSE, 0, 0, 0, TRUE, 0x70025120},
	{FALSE, 0, 0, 0, TRUE, 0x70025130},
	{FALSE, 0, 0, 0, TRUE, 0x70025140},
	{FALSE, 0, 0, 0, TRUE, 0x70025180},
	{FALSE, 0, 0, 0, TRUE, 0x70025184},
	{FALSE, 0, 0, 0, TRUE, 0x70025200},
	{FALSE, 0, 0, 0, TRUE, 0x70025210},
	{FALSE, 0, 0, 0, TRUE, 0x70025214},
	{FALSE, 0, 0, 0, TRUE, 0x70025400},
	{FALSE, 0, 0, 0, TRUE, 0x70025404},
	{FALSE, 0, 0, 0, TRUE, 0x70025408},
	{FALSE, 0, 0, 0, TRUE, 0x7002540c},
	{FALSE, 0, 0, 0, TRUE, 0x70025410},
	{FALSE, 0, 0, 0, TRUE, 0x70025414},
	{FALSE, 0, 0, 0, TRUE, 0x70025418},
	{FALSE, 0, 0, 0, TRUE, 0x70025500},
	{FALSE, 0, 0, 0, TRUE, 0x70025504},
	{FALSE, 0, 0, 0, TRUE, 0x70025600},
	{FALSE, 0, 0, 0, TRUE, 0x70025800},
	{FALSE, 0, 0, 0, TRUE, 0x70025900},
	{FALSE, 0, 0, 0, TRUE, 0x70025904},
	{FALSE, 0, 0, 0, TRUE, 0x70025910},
	{TRUE, 0x70025300, 0, 0x10001, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x10203, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x10405, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x10607, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x10809, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x10a0b, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x10c0d, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x10e0f, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11011, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11213, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11415, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11617, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11819, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11a1b, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11c1d, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x11e1f, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x12021, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x12223, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x12425, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x12627, TRUE, 0x70025304},
	{TRUE, 0x70025300, 0, 0x12829, TRUE, 0x70025304},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_a = {
	"PSOP_9_1_A", "SectionA - cb_infra vlp",
	80, ARRAY_SIZE(mt7999_cb_infra_a),
	mt7999_cb_infra_a
};

const struct wlan_dbg_command mt7999_cb_infra_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x70028018},
	{FALSE, 0, 0, 0, TRUE, 0x7002801c},
	{FALSE, 0, 0, 0, TRUE, 0x70028024},
	{FALSE, 0, 0, 0, TRUE, 0x7002802c},
	{FALSE, 0, 0, 0, TRUE, 0x70028034},
	{FALSE, 0, 0, 0, TRUE, 0x70028350},
	{FALSE, 0, 0, 0, TRUE, 0x700283c0},
	{FALSE, 0, 0, 0, TRUE, 0x70028400},
	{FALSE, 0, 0, 0, TRUE, 0x70028408},
	{FALSE, 0, 0, 0, TRUE, 0x70028410},
	{FALSE, 0, 0, 0, TRUE, 0x70028420},
	{FALSE, 0, 0, 0, TRUE, 0x70028424},
	{FALSE, 0, 0, 0, TRUE, 0x70028428},
	{FALSE, 0, 0, 0, TRUE, 0x7002842c},
	{FALSE, 0, 0, 0, TRUE, 0x70028430},
	{FALSE, 0, 0, 0, TRUE, 0x70028434},
	{FALSE, 0, 0, 0, TRUE, 0x70028438},
	{FALSE, 0, 0, 0, TRUE, 0x7002843c},
	{FALSE, 0, 0, 0, TRUE, 0x70028440},
	{FALSE, 0, 0, 0, TRUE, 0x70028448},
	{FALSE, 0, 0, 0, TRUE, 0x7002844c},
	{FALSE, 0, 0, 0, TRUE, 0x70028458},
	{FALSE, 0, 0, 0, TRUE, 0x7002845c},
	{FALSE, 0, 0, 0, TRUE, 0x70028460},
	{FALSE, 0, 0, 0, TRUE, 0x70028464},
	{FALSE, 0, 0, 0, TRUE, 0x70028468},
	{FALSE, 0, 0, 0, TRUE, 0x7002846c},
	{FALSE, 0, 0, 0, TRUE, 0x70028500},
	{FALSE, 0, 0, 0, TRUE, 0x70028504},
	{FALSE, 0, 0, 0, TRUE, 0x70028508},
	{FALSE, 0, 0, 0, TRUE, 0x70028510},
	{FALSE, 0, 0, 0, TRUE, 0x70028514},
	{FALSE, 0, 0, 0, TRUE, 0x70028518},
	{FALSE, 0, 0, 0, TRUE, 0x70028520},
	{FALSE, 0, 0, 0, TRUE, 0x70028524},
	{FALSE, 0, 0, 0, TRUE, 0x70028528},
	{FALSE, 0, 0, 0, TRUE, 0x70028530},
	{FALSE, 0, 0, 0, TRUE, 0x70028534},
	{FALSE, 0, 0, 0, TRUE, 0x70028538},
	{FALSE, 0, 0, 0, TRUE, 0x70028540},
	{FALSE, 0, 0, 0, TRUE, 0x70028544},
	{FALSE, 0, 0, 0, TRUE, 0x70028548},
	{FALSE, 0, 0, 0, TRUE, 0x70028550},
	{FALSE, 0, 0, 0, TRUE, 0x70028554},
	{FALSE, 0, 0, 0, TRUE, 0x70028600},
	{FALSE, 0, 0, 0, TRUE, 0x70028604},
	{FALSE, 0, 0, 0, TRUE, 0x70028610},
	{FALSE, 0, 0, 0, TRUE, 0x70028614},
	{FALSE, 0, 0, 0, TRUE, 0x70028620},
	{FALSE, 0, 0, 0, TRUE, 0x70028730},
	{FALSE, 0, 0, 0, TRUE, 0x70028734},
	{FALSE, 0, 0, 0, TRUE, 0x70028738},
	{FALSE, 0, 0, 0, TRUE, 0x70028900},
	{FALSE, 0, 0, 0, TRUE, 0x70028a00},
	{FALSE, 0, 0, 0, TRUE, 0x70028a04},
	{FALSE, 0, 0, 0, TRUE, 0x70028a08},
	{FALSE, 0, 0, 0, TRUE, 0x70028a0c},
	{FALSE, 0, 0, 0, TRUE, 0x70028a20},
	{FALSE, 0, 0, 0, TRUE, 0x70028a24},
	{FALSE, 0, 0, 0, TRUE, 0x70028a60},
	{FALSE, 0, 0, 0, TRUE, 0x70028a64},
	{FALSE, 0, 0, 0, TRUE, 0x70028a68},
	{FALSE, 0, 0, 0, TRUE, 0x70028a70},
	{FALSE, 0, 0, 0, TRUE, 0x70028a74},
	{FALSE, 0, 0, 0, TRUE, 0x70028a78},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_b = {
	"PSOP_9_1_B", "SectionB - cb_infra vcore on",
	65, ARRAY_SIZE(mt7999_cb_infra_b),
	mt7999_cb_infra_b
};

const struct wlan_dbg_command mt7999_cb_infra_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x70026000},
	{FALSE, 0, 0, 0, TRUE, 0x70026004},
	{FALSE, 0, 0, 0, TRUE, 0x70026008},
	{FALSE, 0, 0, 0, TRUE, 0x7002600c},
	{FALSE, 0, 0, 0, TRUE, 0x70026014},
	{FALSE, 0, 0, 0, TRUE, 0x70026018},
	{FALSE, 0, 0, 0, TRUE, 0x70026090},
	{FALSE, 0, 0, 0, TRUE, 0x70026094},
	{FALSE, 0, 0, 0, TRUE, 0x70026100},
	{FALSE, 0, 0, 0, TRUE, 0x70026104},
	{FALSE, 0, 0, 0, TRUE, 0x70026110},
	{FALSE, 0, 0, 0, TRUE, 0x70026554},
	{FALSE, 0, 0, 0, TRUE, 0x70026558},
	{FALSE, 0, 0, 0, TRUE, 0x7002655c},
	{FALSE, 0, 0, 0, TRUE, 0x70026704},
	{FALSE, 0, 0, 0, TRUE, 0x70026708},
	{FALSE, 0, 0, 0, TRUE, 0x7002670c},
	{FALSE, 0, 0, 0, TRUE, 0x70026a00},
	{FALSE, 0, 0, 0, TRUE, 0x70026a04},
	{FALSE, 0, 0, 0, TRUE, 0x70026a1c},
	{FALSE, 0, 0, 0, TRUE, 0x70026a20},
	{FALSE, 0, 0, 0, TRUE, 0x70026a50},
	{FALSE, 0, 0, 0, TRUE, 0x70026a54},
	{FALSE, 0, 0, 0, TRUE, 0x70026a58},
	{FALSE, 0, 0, 0, TRUE, 0x70026a5c},
	{FALSE, 0, 0, 0, TRUE, 0x70026a60},
	{FALSE, 0, 0, 0, TRUE, 0x70026a70},
	{FALSE, 0, 0, 0, TRUE, 0x70026a64},
	{FALSE, 0, 0, 0, TRUE, 0x70026a68},
	{FALSE, 0, 0, 0, TRUE, 0x70026a6c},
	{FALSE, 0, 0, 0, TRUE, 0x70020004},
	{FALSE, 0, 0, 0, TRUE, 0x70020008},
	{FALSE, 0, 0, 0, TRUE, 0x70020080},
	{FALSE, 0, 0, 0, TRUE, 0x70020110},
	{FALSE, 0, 0, 0, TRUE, 0x70021060},
	{FALSE, 0, 0, 0, TRUE, 0x70021064},
	{FALSE, 0, 0, 0, TRUE, 0x70021068},
	{FALSE, 0, 0, 0, TRUE, 0x7002106c},
	{FALSE, 0, 0, 0, TRUE, 0x70021070},
	{FALSE, 0, 0, 0, TRUE, 0x70021074},
	{FALSE, 0, 0, 0, TRUE, 0x70021078},
	{FALSE, 0, 0, 0, TRUE, 0x7002107c},
	{FALSE, 0, 0, 0, TRUE, 0x70021080},
	{FALSE, 0, 0, 0, TRUE, 0x70021084},
	{FALSE, 0, 0, 0, TRUE, 0x70021088},
	{FALSE, 0, 0, 0, TRUE, 0x7002108c},
	{FALSE, 0, 0, 0, TRUE, 0x70021090},
	{FALSE, 0, 0, 0, TRUE, 0x70021094},
	{FALSE, 0, 0, 0, TRUE, 0x70021098},
	{FALSE, 0, 0, 0, TRUE, 0x7002109c},
	{FALSE, 0, 0, 0, TRUE, 0x700210a0},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_c = {
	"PSOP_9_1_C", "SectionC - cb_infra off, read check ok",
	51, ARRAY_SIZE(mt7999_cb_infra_c),
	mt7999_cb_infra_c
};

const struct wlan_dbg_command mt7999_cb_infra_d[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x74030150},
	{FALSE, 0, 0, 0, TRUE, 0x74030154},
	{FALSE, 0, 0, 0, TRUE, 0x74030184},
	{FALSE, 0, 0, 0, TRUE, 0x74031010},
	{TRUE, 0x74030168, 0, 0x22cc0100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x81804845, TRUE, 0x7403002c},
	{FALSE, 0, 0, 0, TRUE, 0x74031204},
	{FALSE, 0, 0, 0, TRUE, 0x74031210},
	{FALSE, 0, 0, 0, TRUE, 0x74030184},
	{FALSE, 0, 0, 0, TRUE, 0x740331c0},
	{FALSE, 0, 0, 0, TRUE, 0x74030018},
	{FALSE, 0, 0, 0, TRUE, 0x74030028},
	{FALSE, 0, 0, 0, TRUE, 0x740300c0},
	{FALSE, 0, 0, 0, TRUE, 0x740300c8},
	{FALSE, 0, 0, 0, TRUE, 0x74030194},
	{FALSE, 0, 0, 0, TRUE, 0x74030198},
	{FALSE, 0, 0, 0, TRUE, 0x7403019c},
	{FALSE, 0, 0, 0, TRUE, 0x740301a4},
	{FALSE, 0, 0, 0, TRUE, 0x740301a8},
	{FALSE, 0, 0, 0, TRUE, 0x740301ac},
	{FALSE, 0, 0, 0, TRUE, 0x740301d4},
	{FALSE, 0, 0, 0, TRUE, 0x74030d38},
	{FALSE, 0, 0, 0, TRUE, 0x74030d3c},
	{FALSE, 0, 0, 0, TRUE, 0x74030d40},
	{FALSE, 0, 0, 0, TRUE, 0x74030e00},
	{FALSE, 0, 0, 0, TRUE, 0x74030e04},
	{FALSE, 0, 0, 0, TRUE, 0x74030e08},
	{FALSE, 0, 0, 0, TRUE, 0x74030e40},
	{FALSE, 0, 0, 0, TRUE, 0x74030e44},
	{FALSE, 0, 0, 0, TRUE, 0x74030e48},
	{FALSE, 0, 0, 0, TRUE, 0x74031004},
	{FALSE, 0, 0, 0, TRUE, 0x74031008},
	{FALSE, 0, 0, 0, TRUE, 0x7403100c},
	{FALSE, 0, 0, 0, TRUE, 0x74031010},
	{FALSE, 0, 0, 0, TRUE, 0x74031014},
	{FALSE, 0, 0, 0, TRUE, 0x74031018},
	{FALSE, 0, 0, 0, TRUE, 0x7403101c},
	{FALSE, 0, 0, 0, TRUE, 0x74031020},
	{FALSE, 0, 0, 0, TRUE, 0x74031024},
	{FALSE, 0, 0, 0, TRUE, 0x74031080},
	{FALSE, 0, 0, 0, TRUE, 0x74031084},
	{FALSE, 0, 0, 0, TRUE, 0x74031088},
	{FALSE, 0, 0, 0, TRUE, 0x7403108c},
	{FALSE, 0, 0, 0, TRUE, 0x74031090},
	{FALSE, 0, 0, 0, TRUE, 0x74031094},
	{FALSE, 0, 0, 0, TRUE, 0x74031098},
	{FALSE, 0, 0, 0, TRUE, 0x740310a8},
	{FALSE, 0, 0, 0, TRUE, 0x7403109c},
	{FALSE, 0, 0, 0, TRUE, 0x740310e0},
	{FALSE, 0, 0, 0, TRUE, 0x740310e4},
	{FALSE, 0, 0, 0, TRUE, 0x740310e8},
	{FALSE, 0, 0, 0, TRUE, 0x740310ec},
	{FALSE, 0, 0, 0, TRUE, 0x740310f0},
	{FALSE, 0, 0, 0, TRUE, 0x740310f4},
	{FALSE, 0, 0, 0, TRUE, 0x74031108},
	{FALSE, 0, 0, 0, TRUE, 0x74031110},
	{FALSE, 0, 0, 0, TRUE, 0x74031114},
	{FALSE, 0, 0, 0, TRUE, 0x74031118},
	{FALSE, 0, 0, 0, TRUE, 0x7403111c},
	{FALSE, 0, 0, 0, TRUE, 0x74031200},
	{FALSE, 0, 0, 0, TRUE, 0x74031204},
	{FALSE, 0, 0, 0, TRUE, 0x74031208},
	{FALSE, 0, 0, 0, TRUE, 0x7403120c},
	{FALSE, 0, 0, 0, TRUE, 0x74031210},
	{FALSE, 0, 0, 0, TRUE, 0x74031214},
	{FALSE, 0, 0, 0, TRUE, 0x74031218},
	{FALSE, 0, 0, 0, TRUE, 0x7403121c},
	{TRUE, 0x74030168, 0, 0xcccc0100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x45474849, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0xcccc0100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x4a4b4c4d, TRUE, 0x7403002c},
	{FALSE, 0, 0, 0, TRUE, 0x7403002c},
	{FALSE, 0, 0, 0, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0xa0b1516, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0xc0d0e0f, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x22220100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x4050607, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x22220100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x80818287, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x33330100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x10716, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x33330100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x51525455, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x33330100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x51525455, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x44440100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0xc9cacbcd, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x55550100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x8090a0b, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0xcccc0100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x4c4d4e4f, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x88880100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x47484a4d, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x99990100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x4143494b, TRUE, 0x7403002c},
	{TRUE, 0x74030168, 0, 0x33330100, FALSE, 0},
	{TRUE, 0x74030164, 0, 0x10e1a, TRUE, 0x7403002c},
	{FALSE, 0, 0, 0, TRUE, 0x7403002c},
	{FALSE, 0, 0, 0, TRUE, 0x7403002c},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_d = {
	"PSOP_9_1_D", "SectionD - pcie",
	85, ARRAY_SIZE(mt7999_cb_infra_d),
	mt7999_cb_infra_d
};

const struct wlan_dbg_command mt7999_cb_infra_e[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x70000104},
	{FALSE, 0, 0, 0, TRUE, 0x70000248},
	{FALSE, 0, 0, 0, TRUE, 0x7000024c},
	{FALSE, 0, 0, 0, TRUE, 0x70007400},
	{FALSE, 0, 0, 0, TRUE, 0x70007404},
	{FALSE, 0, 0, 0, TRUE, 0x70003000},
	{FALSE, 0, 0, 0, TRUE, 0x70006414},
	{FALSE, 0, 0, 0, TRUE, 0x70007414},
	{FALSE, 0, 0, 0, TRUE, 0x70003014},
	{FALSE, 0, 0, 0, TRUE, 0x70000258},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_e = {
	"PSOP_9_1_E", "SectionE - cbtop CR",
	10, ARRAY_SIZE(mt7999_cb_infra_e),
	mt7999_cb_infra_e
};

const struct wlan_dbg_command mt7999_cb_infra_g[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x70070000},
	{FALSE, 0, 0, 0, TRUE, 0x7007000c},
	{FALSE, 0, 0, 0, TRUE, 0x70070010},
	{FALSE, 0, 0, 0, TRUE, 0x70070408},
	{FALSE, 0, 0, 0, TRUE, 0x7007040c},
	{FALSE, 0, 0, 0, TRUE, 0x70070410},
	{FALSE, 0, 0, 0, TRUE, 0x70070414},
	{FALSE, 0, 0, 0, TRUE, 0x70070418},
	{FALSE, 0, 0, 0, TRUE, 0x7007041c},
	{FALSE, 0, 0, 0, TRUE, 0x70070420},
	{FALSE, 0, 0, 0, TRUE, 0x70070424},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_g = {
	"PSOP_9_1_G", "SectionG - dma dbg_ctl",
	11, ARRAY_SIZE(mt7999_cb_infra_g),
	mt7999_cb_infra_g
};

const struct wlan_dbg_command mt7999_cb_infra_h[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x74130200},
	{FALSE, 0, 0, 0, TRUE, 0x74130204},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_h = {
	"PSOP_9_1_H", "SectionH - cb_infra_mbu",
	2, ARRAY_SIZE(mt7999_cb_infra_h),
	mt7999_cb_infra_h
};

const struct wlan_dbg_command mt7999_cb_infra_i[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x75010010},
	{FALSE, 0, 0, 0, TRUE, 0x75010014},
	{FALSE, 0, 0, 0, TRUE, 0x7501001c},
	{FALSE, 0, 0, 0, TRUE, 0x7501003c},
	{FALSE, 0, 0, 0, TRUE, 0x7501004c},
	{FALSE, 0, 0, 0, TRUE, 0x7501005c},
	{FALSE, 0, 0, 0, TRUE, 0x75010060},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_i = {
	"PSOP_9_1_I", "SectionI - cb_infra_bus (cb_infra_off_dma_bcrm)",
	7, ARRAY_SIZE(mt7999_cb_infra_i),
	mt7999_cb_infra_i
};

const struct wlan_dbg_command mt7999_cb_infra_j[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x740700b0},
	{FALSE, 0, 0, 0, TRUE, 0x740700c0},
	{TRUE, 0x74070000, 0, 0x101, FALSE, 0},
	{TRUE, 0x74070008, 0, 0x80004, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x0, FALSE, 0},
	{TRUE, 0x74070004, 0, 0x1d, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x0, FALSE, 0},
	{TRUE, 0x74070008, 0, 0x10, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0x920091, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0x940093, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0x960095, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0xae00ad, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0xb500ad, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0xb700b6, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0xd300d2, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0xd500d4, TRUE, 0x740700d0},
	{TRUE, 0x74070000, 0, 0x404, FALSE, 0},
	{TRUE, 0x74070010, 0, 0xd700d6, TRUE, 0x740700d0},
};

const struct wlan_dump_list mt7999_dump_list_cb_infra_j = {
	"PSOP_9_1_J", "SectionJ - pcie phy",
	14, ARRAY_SIZE(mt7999_cb_infra_j),
	mt7999_cb_infra_j
};

const struct wlan_dbg_command mt7999_wf_top_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x20060b00, 0, 0x1, FALSE, 0},
	{TRUE, 0x20060b04, 0, 0x0, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x1, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x2, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x3, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x4, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x10, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x12, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x17, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x18, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x19, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x1a, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x1b, TRUE, 0x20060b10},
	{TRUE, 0x20060b04, 0, 0x1d, TRUE, 0x20060b10},
};

const struct wlan_dump_list mt7999_dump_list_wf_top_a = {
	"PSOP_3_1_A", "SectionA - Dump wf_top_misc_on monflg",
	13, ARRAY_SIZE(mt7999_wf_top_a),
	mt7999_wf_top_a
};

const struct wlan_dbg_command mt7999_wf_top_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x20060b18, 0, 0x1, FALSE, 0},
	{TRUE, 0x20060b1c, 0, 0x0, TRUE, 0x20023638},
	{TRUE, 0x20060b1c, 0, 0x1, TRUE, 0x20023638},
	{TRUE, 0x20060b1c, 0, 0x2, TRUE, 0x20023638},
	{TRUE, 0x20060b1c, 0, 0x3, TRUE, 0x20023638},
	{TRUE, 0x20060b1c, 0, 0x4, TRUE, 0x20023638},
	{TRUE, 0x20060b1c, 0, 0x8, TRUE, 0x20023638},
};

const struct wlan_dump_list mt7999_dump_list_wf_top_b = {
	"PSOP_3_1_B", "SectionB - Dump wf_top_misc_von monflg",
	6, ARRAY_SIZE(mt7999_wf_top_b),
	mt7999_wf_top_b
};

const struct wlan_dbg_command mt7999_wf_top_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b00},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b04},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b08},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b0c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b10},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b14},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b18},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b1c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b20},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b24},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b28},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b2c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b30},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b34},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b38},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b3c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b40},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b50},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b54},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b58},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b5c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b60},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b64},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b68},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b6c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b70},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b74},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b78},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b7c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b80},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b84},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b88},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b8c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c1b90},
};

const struct wlan_dump_list mt7999_dump_list_wf_top_c = {
	"PSOP_3_1_C", "SectionC - Dump wf_top_cfg_on debug CR",
	34, ARRAY_SIZE(mt7999_wf_top_c),
	mt7999_wf_top_c
};

const struct wlan_dbg_command mt7999_wf_top_d[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b00},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b04},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b08},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b0c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b10},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b14},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b18},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b1c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b20},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b24},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b28},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b2c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b30},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b34},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b38},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b3c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b40},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b44},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b48},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b4c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b50},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b54},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b58},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b5c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b60},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b64},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b68},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b6c},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b70},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b74},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b78},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c0b7c},
};

const struct wlan_dump_list mt7999_dump_list_wf_top_d = {
	"PSOP_3_1_D", "SectionD - Dump wf_top_rgu_on debug CR",
	32, ARRAY_SIZE(mt7999_wf_top_d),
	mt7999_wf_top_d
};

const struct wlan_dbg_command mt7999_wf_top_e[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x7c400120, 0, 0x87000000, FALSE, 0},
	{TRUE, 0x7c500830, 0, 0xf, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x10, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x11, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x12, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x13, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x14, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x15, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x16, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x17, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x18, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x19, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x1a, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x1b, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x1c, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x1d, TRUE, 0x7c500834},
	{TRUE, 0x7c500830, 0, 0x1e, TRUE, 0x7c500834},
};

const struct wlan_dump_list mt7999_dump_list_wf_top_e = {
	"PSOP_3_1_E", "SectionE - Dump wf_top_rgu_von monflg",
	16, ARRAY_SIZE(mt7999_wf_top_e),
	mt7999_wf_top_e
};

const struct wlan_dbg_command mt7999_wf_top_f[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{FALSE, 0, 0, 0, TRUE, 0x7c4c3fa0},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c3fa4},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c3fa8},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c3fac},
	{FALSE, 0, 0, 0, TRUE, 0x7c4c3fb0},
};

const struct wlan_dump_list mt7999_dump_list_wf_top_f = {
	"PSOP_3_1_F", "SectionF - Dump wf_top_slpprot_on debug CR",
	5, ARRAY_SIZE(mt7999_wf_top_f),
	mt7999_wf_top_f
};

const struct wlan_dbg_command mt7999_wf_bus_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x20023604, 0, 0x4, TRUE, 0x2002362c},
	{TRUE, 0x20023628, 0, 0x10001, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x20001, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x30001, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x10002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x20002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x30002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x40002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x50002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x60002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x70002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x80002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x90002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0xa0002, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x10003, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x20003, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x30003, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x40003, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x50003, TRUE, 0x20023608},
	{TRUE, 0x20023628, 0, 0x60003, TRUE, 0x20023608},
};

const struct wlan_dump_list mt7999_dump_list_wf_bus_a = {
	"PSOP_4_1_A", "SectionA - Dump VDNR timeout host side info",
	20, ARRAY_SIZE(mt7999_wf_bus_a),
	mt7999_wf_bus_a
};

const struct wlan_dbg_command mt7999_wf_bus_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x7c400120, 0, 0x810f0000, FALSE, 0},
	{FALSE, 0, 0, 0, TRUE, 0x7c500408},
	{FALSE, 0, 0, 0, TRUE, 0x7c50040c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500410},
	{FALSE, 0, 0, 0, TRUE, 0x7c500414},
	{FALSE, 0, 0, 0, TRUE, 0x7c500418},
	{FALSE, 0, 0, 0, TRUE, 0x7c50041c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500420},
	{FALSE, 0, 0, 0, TRUE, 0x7c500424},
	{FALSE, 0, 0, 0, TRUE, 0x7c500428},
	{FALSE, 0, 0, 0, TRUE, 0x7c50042c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500430},
	{FALSE, 0, 0, 0, TRUE, 0x7c500434},
	{FALSE, 0, 0, 0, TRUE, 0x7c500438},
	{FALSE, 0, 0, 0, TRUE, 0x7c50043c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500440},
	{FALSE, 0, 0, 0, TRUE, 0x7c500444},
	{FALSE, 0, 0, 0, TRUE, 0x7c500448},
	{FALSE, 0, 0, 0, TRUE, 0x7c50044c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500450},
	{FALSE, 0, 0, 0, TRUE, 0x7c500000},
};

const struct wlan_dump_list mt7999_dump_list_wf_bus_b = {
	"PSOP_4_1_B", "SectionB - Dump VDNR timeout wf side info",
	20, ARRAY_SIZE(mt7999_wf_bus_b),
	mt7999_wf_bus_b
};

const struct wlan_dbg_command mt7999_wf_bus_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x7c400120, 0, 0x830c0000, FALSE, 0},
	{FALSE, 0, 0, 0, TRUE, 0x7c501004},
	{FALSE, 0, 0, 0, TRUE, 0x7c501010},
	{FALSE, 0, 0, 0, TRUE, 0x7c501008},
	{FALSE, 0, 0, 0, TRUE, 0x7c50100c},
	{FALSE, 0, 0, 0, TRUE, 0x7c501000},
};

const struct wlan_dump_list mt7999_dump_list_wf_bus_c = {
	"PSOP_4_1_C", "SectionC - Dump AHB APB timeout info",
	5, ARRAY_SIZE(mt7999_wf_bus_c),
	mt7999_wf_bus_c
};

const struct wlan_dbg_command mt7999_wf_bus_d[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x7c400120, 0, 0x80020000, FALSE, 0},
	{FALSE, 0, 0, 0, TRUE, 0x7c50096c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500970},
	{FALSE, 0, 0, 0, TRUE, 0x7c500974},
	{FALSE, 0, 0, 0, TRUE, 0x7c500978},
	{FALSE, 0, 0, 0, TRUE, 0x7c50097c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500980},
	{FALSE, 0, 0, 0, TRUE, 0x7c500984},
	{FALSE, 0, 0, 0, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{FALSE, 0, 0, 0, TRUE, 0x7c500990},
};

const struct wlan_dump_list mt7999_dump_list_wf_bus_d = {
	"PSOP_4_1_D", "SectionD - Dump WF2AP bus status",
	10, ARRAY_SIZE(mt7999_wf_bus_d),
	mt7999_wf_bus_d
};

const struct wlan_dbg_command mt7999_wf_bus_e[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x7c400120, 0, 0x80020000, FALSE, 0},
	{TRUE, 0x7c500990, 0, 0x0, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x1, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x2, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x3, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x4, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x5, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x6, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x7, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x8, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x9, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0xa, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0xb, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0xc, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0xd, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0xe, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0xf, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x10, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x11, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x12, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x13, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x14, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x15, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x16, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x17, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x18, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x19, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x1a, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x1b, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x1c, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x1d, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x1e, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x1f, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x20, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x21, TRUE, 0x7c500988},
	{FALSE, 0, 0, 0, TRUE, 0x7c50098c},
	{TRUE, 0x7c500990, 0, 0x22, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x23, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x24, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x25, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x26, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x27, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x28, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x29, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x2a, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x2b, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x2c, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x2d, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x2e, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x2f, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x30, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x31, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x32, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x33, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x34, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x35, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x36, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x37, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x38, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x39, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x3a, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x3b, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x3c, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x3d, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x3e, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x3f, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x40, TRUE, 0x7c500988},
	{TRUE, 0x7c500990, 0, 0x41, TRUE, 0x7c500988},
};

const struct wlan_dump_list mt7999_dump_list_wf_bus_e = {
	"PSOP_4_1_E", "SectionE - Dump WF2AP access detect info",
	100, ARRAY_SIZE(mt7999_wf_bus_e),
	mt7999_wf_bus_e
};

const struct wlan_dbg_command mt7999_wf_mcu_hostcsr_a[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x20023604, 0x1, 0x0, FALSE, 0},
	{TRUE, 0x2002360c, 0x3f, 0x0, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x1, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x2, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x3, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x4, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x5, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x6, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x7, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x8, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x9, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0xa, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0xb, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0xc, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0xd, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0xe, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0xf, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x10, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x11, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x12, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x13, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x14, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x15, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x16, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x17, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x18, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x19, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x1a, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x1b, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x1c, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x1d, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x1e, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x1f, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x20, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x21, TRUE, 0x20023610},
	{TRUE, 0x2002360c, 0x3f, 0x22, TRUE, 0x20023610},
};

const struct wlan_dump_list mt7999_dump_list_wf_mcu_hostcsr_a = {
	"PSOP_4_2_A", "SectionA - Dump WFMCU PC_log",
	35, ARRAY_SIZE(mt7999_wf_mcu_hostcsr_a),
	mt7999_wf_mcu_hostcsr_a
};

const struct wlan_dbg_command mt7999_wf_mcu_hostcsr_b[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x20023604, 0x6, 0x0, FALSE, 0},
	{TRUE, 0x20023614, 0x3f, 0x0, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x1, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x2, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x3, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x4, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x5, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x6, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x7, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x8, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x9, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0xa, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0xb, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0xc, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0xd, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0xe, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0xf, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x10, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x11, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x12, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x13, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x14, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x15, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x16, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x17, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x18, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x19, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x1a, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x1b, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x1c, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x1d, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x1e, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x1f, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x20, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x21, TRUE, 0x20023608},
	{TRUE, 0x20023614, 0x3f, 0x22, TRUE, 0x20023608},
};

const struct wlan_dump_list mt7999_dump_list_wf_mcu_hostcsr_b = {
	"PSOP_4_2_B", "SectionB - Dump WFMCU GPR_log",
	35, ARRAY_SIZE(mt7999_wf_mcu_hostcsr_b),
	mt7999_wf_mcu_hostcsr_b
};

const struct wlan_dbg_command mt7999_wf_mcu_hostcsr_c[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x7c4f000c, 0x10, 0x0, FALSE, 0},
	{TRUE, 0x20023604, 0x6, 0x0, FALSE, 0},
	{TRUE, 0x20023620, 0, 0x1000, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1001, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1002, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1003, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1004, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1005, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1006, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1007, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1008, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1009, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x100a, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x100b, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x100c, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x100d, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x100e, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x100f, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1010, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1011, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1012, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1013, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1014, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1015, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1016, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1017, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1018, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1019, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x101a, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x101b, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x101c, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x101d, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x101e, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x101f, TRUE, 0x20023608},
};

const struct wlan_dump_list mt7999_dump_list_wf_mcu_hostcsr_c = {
	"PSOP_4_2_C", "SectionC - Dump WFMCU GPR",
	32, ARRAY_SIZE(mt7999_wf_mcu_hostcsr_c),
	mt7999_wf_mcu_hostcsr_c
};

const struct wlan_dbg_command mt7999_wf_mcu_hostcsr_d[] = {
	/* write, w_addr, mask, value, read, r_addr*/
	{TRUE, 0x20023620, 0, 0x0, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x2, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x3, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x4, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x5, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x6, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x7, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x8, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x9, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0xa, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0xb, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0xc, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0xd, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0xe, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0xf, TRUE, 0x20023608},
	{TRUE, 0x20023620, 0, 0x1001, FALSE, 0},
};

const struct wlan_dump_list mt7999_dump_list_wf_mcu_hostcsr_d = {
	"PSOP_4_2_D", "SectionD - Dump WFMCU CSR",
	16, ARRAY_SIZE(mt7999_wf_mcu_hostcsr_d),
	mt7999_wf_mcu_hostcsr_d
};
