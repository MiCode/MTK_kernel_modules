/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_DL_DMA_BUF_LOCK_H
#define _GPS_DL_DMA_BUF_LOCK_H

#include "gps_each_link.h"
#include "gps_dl_dma_buf.h"

void gps_dl_dma_buf_lock_take(struct gps_dl_dma_buf *p_dma,
	enum gps_each_link_spinlock spin_lock_id);

void gps_dl_dma_buf_lock_give(struct gps_dl_dma_buf *p_dma,
	enum gps_each_link_spinlock spin_lock_id);

#endif /* _GPS_DL_DMA_BUF_LOCK_H */

