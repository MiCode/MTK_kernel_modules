/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifndef _GPS_DL_LINUX_H
#define _GPS_DL_LINUX_H

#include "linux/interrupt.h"
#include "gps_dl_isr.h"
#include "gps_each_link.h"

/* put linux proprietary items here otherwise put into gps_dl_osal.h */
irqreturn_t gps_dl_linux_irq_dispatcher(int irq, void *data);
int gps_dl_linux_irqs_register(struct gps_each_irq *p_irqs, int irq_num);
int gps_dl_linux_irqs_unregister(struct gps_each_irq *p_irqs, int irq_num);

#endif /* _GPS_DL_LINUX_H */

