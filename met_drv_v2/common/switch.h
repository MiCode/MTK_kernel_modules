/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MT_SWITCH__
#define __MT_SWITCH__
/*
 * =========================
 * !!!!!!!!!!!NOTICE!!!!!!!!
 * =========================
 * MT_SWITCH OPTION must delcare as Mask value
 * And sort them from smallest to largest
 * MT_SWITCH_MX_ITEM was used to determine argument range
*/
enum {
	/* =================== */
	/* user define mt switch event */
	/* =================== */
	MT_SWITCH_SCHEDSWITCH = 0x0001,
	MT_SWITCH_64_32BIT = 0x0002,
	MT_SWITCH_TAGPOLLING = 0x0004,
	MT_SWITCH_EVENT_TIMER = 0x0008,
	/* =================== */
	MT_SWITCH_MX_ITEM
};

extern struct metdevice met_switch;
#endif
