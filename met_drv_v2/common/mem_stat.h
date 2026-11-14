/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MEM_STAT_H__
#define __MEM_STAT_H__

#define MAX_EVENT_NAME_LEN 32

enum phy_mem_event_id {
	FREE_MEM = 0
};

enum vir_mem_event_id {
	FILE_PAGES = 0,
	FILE_DIRTY,
	NUM_DIRTIED,
	WRITE_BACK,
	NUM_WRITTEN,
	PG_FAULT_CNT
};

struct mem_event {
	int id;
	char name[32];
	char header_name[32];
};

#endif
