/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MET_REG_ADDR_H__
#define __MET_REG_ADDR_H__

#define EMI_REG_BASE           (0x10219000)
#define EMI_REG_SIZE		   0x1000


#define EMI_CH0_REG_BASE 	(0x10235000) 
/*#define EMI_CH1_REG_BASE	(0x10245000)*/
#define EMI_CH_REG_SIZE		0xA90
#define EMI_CHN_BASE(n)	(EMI_CH0_REG_BASE + n*(0x10000))

#endif
