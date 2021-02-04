/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <asm/cpu.h>
#include "met_kernel_symbol.h"
#include "cpu_dsu.h"

//dsu support 6 event
#define DSU_EVENT_MAX_CNT 6

static int armv8_dsu_hw_check_event(struct met_dsu *pmu, int idx, int event)
{
	int i;

	/* Check if event is duplicate */
	for (i = 0; i < idx; i++) {
		if (pmu[i].event == event)
			break;
	}
	if (i < idx) {
		/* pr_debug("++++++ found duplicate event 0x%02x i=%d\n", event, i); */
		return -1;
	}
	return 0;
}


static struct met_dsu	pmus[MXNR_DSU_EVENTS];

struct cpu_dsu_hw armv8_dsu = {
	.name = "armv8_dsu",
	.check_event = armv8_dsu_hw_check_event,
};

static void init_dsu(void)
{
	armv8_dsu.event_count = DSU_EVENT_MAX_CNT;
}

struct cpu_dsu_hw *cpu_dsu_hw_init(void)
{

	init_dsu();
	armv8_dsu.pmu = pmus;
	return &armv8_dsu;
}
