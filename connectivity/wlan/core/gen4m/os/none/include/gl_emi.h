/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GL_EMI_H
#define _GL_EMI_H

#define EMI_NAME		"WIFI-EMI"
#define WIFI_EMI_ADDR_MASK	0xFFFFFF

enum EMI_ALLOC_TYPE {
	EMI_ALLOC_TYPE_NONE,
	EMI_ALLOC_TYPE_WMT,
	EMI_ALLOC_TYPE_CONNINFRA,
	EMI_ALLOC_TYPE_IN_DRIVER,
	EMI_ALLOC_TYPE_LK,
	EMI_ALLOC_TYPE_DTS,
	EMI_ALLOC_TYPE_NUM
};

struct EMI_MEM_INFO {
	const enum EMI_ALLOC_TYPE type;
	const uint32_t coredump_size;
	phys_addr_t pa;
	void *va;
	uint32_t size;
	u_int8_t initialized;
};

struct mt66xx_chip_info;

static int32_t emi_mem_init(struct mt66xx_chip_info *chip, void *dev)
{ return -1; }
static void emi_mem_uninit(struct mt66xx_chip_info *chip, void *dev) {}
static int32_t emi_mem_write(struct mt66xx_chip_info *chip,
	uint32_t offset, void *buf, uint32_t size) { return -1; }
static int32_t emi_mem_read(struct mt66xx_chip_info *chip,
	uint32_t offset, void *buf, uint32_t size) { return -1; }
static void *emi_mem_get_vir_base(struct mt66xx_chip_info *chip)
{ return NULL; }
static phys_addr_t emi_mem_get_phy_base(struct mt66xx_chip_info *chip)
{ return 0; }
static uint32_t emi_mem_get_size(struct mt66xx_chip_info *chip) { return 0; }
static uint32_t emi_mem_offset_convert(uint32_t offset)
{ return offset; }

#endif /* _GL_EMI_H */
