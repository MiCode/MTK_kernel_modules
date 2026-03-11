/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_XLINK_BUF_H
#define _GPS_MCUDL_XLINK_BUF_H

#include "gps_mcudl_xlink.h"
#include "gps_dl_dma_buf.h"

void gps_mcudl_dma_buf_free(struct gps_dl_dma_buf *p_dma_buf, enum gps_mcudl_xid x_id);
int gps_mcudl_dma_buf_alloc(struct gps_dl_dma_buf *p_dma_buf, enum gps_mcudl_xid x_id,
	enum gps_dl_dma_dir dir, unsigned int len);
int gps_mcudl_dma_buf_alloc2(enum gps_mcudl_xid x_id);


#endif /* #define _GPS_MCUDL_XLINK_BUF_H */

