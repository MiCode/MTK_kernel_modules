/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#ifndef _CONNV3_DUMP_MNG_H_
#define _CONNV3_DUMP_MNG_H_

#include <linux/types.h>

struct connv3_coredump_platform_ops {
	u32 (*connv3_dump_plt_get_chipid) (void);
	u32 (*connv3_dump_plt_exception_filter) (char*);
};

int connv3_dump_mng_init(void* plat_data);
unsigned int connv3_dump_mng_get_platform_chipid(void);
char* connv3_dump_mng_get_exception_tag_name(int conn_type);
char* connv3_dump_mng_get_subsys_tag(int conn_type);
unsigned int connv3_dump_mng_exception_filter(char*);

#endif /* _CONNV3_DUMP_MNG_H_ */
