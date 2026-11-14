/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _SMI_NAME_H_
#define _SMI_NAME_H_
/* #include "mtk_smi.h" */

enum SMI_DEST {
	SMI_DEST_ALL		= 0,
	SMI_DEST_EMI		= 1,
	SMI_DEST_INTERNAL	= 2,
	SMI_DEST_NONE		= 9
};

enum SMI_RW {
	SMI_RW_ALL		= 0,
	SMI_READ_ONLY		= 1,
	SMI_WRITE_ONLY		= 2,
	SMI_RW_RESPECTIVE	= 3,
	SMI_RW_NONE		= 9
};

enum SMI_BUS {
	SMI_BUS_GMC		= 0,
	SMI_BUS_AXI		= 1,
	SMI_BUS_NONE		= 9
};

enum SMI_REQUEST {
	SMI_REQ_ALL		= 0,
	SMI_REQ_ULTRA		= 1,
	SMI_REQ_PREULTRA	= 2,
	SMI_NORMAL_ULTRA	= 3,
	SMI_REQ_NONE		= 9
};

struct smi_desc {
	unsigned long port;
	enum SMI_DEST desttype;
	enum SMI_RW rwtype;
	enum SMI_BUS bustype;
};
#endif	/* _SMI_NAME_H_ */
