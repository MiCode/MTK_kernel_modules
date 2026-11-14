/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _SMI_H_
#define _SMI_H_

#include <linux/device.h>

struct met_smi {
	int mode;
	int master;
	unsigned int port;
	unsigned int rwtype;	/* 0 for R+W, 1 for read, 2 for write */
	unsigned int desttype;	/* 0 for EMI+internal mem, 1 for EMI, 3 for internal mem */
	unsigned int bustype;	/* 0 for GMC, 1 for AXI */
	/* unsigned long requesttype;// 0:All, 1:ultra high, 2:pre-ultrahigh, 3:normal. */
	struct kobject *kobj_bus_smi;
};

struct smi_cfg {
	int master;
	unsigned int port;
	unsigned int rwtype;	/* 0 for R+W, 1 for read, 2 for write */
	unsigned int desttype;	/* 0 for EMI+internal mem, 1 for EMI, 3 for internal mem */
	unsigned int bustype;	/*0 for GMC, 1 for AXI */
	/*unsigned long requesttype;// 0:All, 1:ultra high, 2:pre-ultrahigh, 3:normal. */
};

struct smi_mon_con {
	unsigned int requesttype;	/* 0:All, 1:ultra high, 2:pre-ultrahigh, 3:normal. */
};

/* ====================== SMI/EMI Interface ================================ */

struct met_smi_conf {
	unsigned int master;	/*Ex : Whitney: 0~8 for larb0~larb8,  9 for common larb*/
	int	port[4];	/* port select : [0] only for legacy mode, [0~3] ports for parallel mode, -1 no select*/
	unsigned int reqtype; /* Selects request type : 0 for all,1 for ultra,2 for preultra,3 for normal*/
	unsigned int rwtype[4]; /* Selects read/write:  0 for R+W,  1 for read,  2 for write;*/
				/* [0] for legacy and parallel larb0~8, [0~3] for parallel mode common*/
	unsigned int desttype; /* Selects destination: 0 and 3 for all memory, 1 for External,2 for internal*/
};

#endif				/* _SMI_H_ */
