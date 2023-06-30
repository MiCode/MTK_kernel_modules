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

#include "ondiemet.h"

/* record enabled modules */
unsigned int ondiemet_module[ONDIEMET_NUM];
EXPORT_SYMBOL(ondiemet_module);

void (*scp_start[ONDIEMET_NUM]) (void) = {
sspm_start, NULL, NULL};

void (*scp_stop[ONDIEMET_NUM]) (void) = {
sspm_stop, NULL, NULL};

void (*scp_extract[ONDIEMET_NUM]) (void) = {
sspm_extract, NULL, NULL};

/* record which MCU is started to generate data */
int ondiemet_module_started[ONDIEMET_NUM];

int ondiemet_attr_init(struct device *dev)
{
	int ret;

	ret = sspm_attr_init(dev);
	if (ret != 0) {
		pr_debug("can not create device file: sspm related\n");
		return ret;
	}

	return 0;

}

int ondiemet_attr_uninit(struct device *dev)
{
	int ret;

	ret = sspm_attr_uninit(dev);
	if (ret != 0) {
		pr_debug("can not delete device file: sspm related\n");
		return ret;
	}

	return 0;

}

void ondiemet_start(void)
{
	int i;

	for (i = 0; i < ONDIEMET_NUM; i++) {
		if (ondiemet_module[i] != 0) {
			ondiemet_module_started[i] = 1;
			(*scp_start[i]) ();
		}
	}
}

void ondiemet_stop(void)
{
	int i;

	for (i = 0; i < ONDIEMET_NUM; i++) {
		if (ondiemet_module[i] != 0) {
			(*scp_stop[i]) ();
			ondiemet_module_started[i] = 0;
		}
	}
}

void ondiemet_extract(void)
{
	int i;

	for (i = 0; i < ONDIEMET_NUM; i++) {
		if (ondiemet_module[i] != 0)
			(*scp_extract[i]) ();
	}
}
