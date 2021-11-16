/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef _SMI_NAME_H_
#define _SMI_NAME_H_
#include "mtk_smi.h"

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
