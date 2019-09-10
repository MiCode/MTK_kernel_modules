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

#ifndef __ONDIEMET_H
#define __ONDIEMET_H

#include "ondiemet_log.h"

extern void ondiemet_extract(void);
extern void ondiemet_stop(void);
extern void ondiemet_start(void);

#define ONDIEMET_SSPM  0
#define ONDIEMET_NUM  3		/* total number of supported */
extern unsigned int ondiemet_module[];
extern void sspm_start(void);
extern void sspm_stop(void);
extern void sspm_extract(void);
extern int sspm_attr_init(struct device *dev);
extern int sspm_attr_uninit(struct device *dev);

extern int ondiemet_attr_init(struct device *dev);
extern int ondiemet_attr_uninit(struct device *dev);

extern int sspm_buffer_size;

#endif				/* __ONDIEMET_H */
