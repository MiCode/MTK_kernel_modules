/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef CONSYS_REG_BASE_H
#define CONSYS_REG_BASE_H

struct consys_reg_base_addr {
	unsigned long vir_addr;
	unsigned long phy_addr;
	unsigned long long size;
};

#endif	/* CONSYS_REG_BASE_H */
