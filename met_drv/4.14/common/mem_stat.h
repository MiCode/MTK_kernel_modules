/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
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
