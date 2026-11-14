/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_dma_buf_lock.h"
#if GPS_DL_HAS_MCUDL
#include "gps_mcudl_link_util.h"
#endif

void gps_dl_dma_buf_lock_take(struct gps_dl_dma_buf *p_dma,
	enum gps_each_link_spinlock spin_lock_id)
{
	if (p_dma->is_for_mcudl) {
#if GPS_DL_HAS_MCUDL
		gps_mcudl_each_link_spin_lock_take((enum gps_mcudl_xid)p_dma->dev_index, spin_lock_id);
#endif
	} else
		gps_each_link_spin_lock_take((enum gps_dl_link_id_enum)p_dma->dev_index, spin_lock_id);
}

void gps_dl_dma_buf_lock_give(struct gps_dl_dma_buf *p_dma,
	enum gps_each_link_spinlock spin_lock_id)
{
	if (p_dma->is_for_mcudl) {
#if GPS_DL_HAS_MCUDL
		gps_mcudl_each_link_spin_lock_give((enum gps_mcudl_xid)p_dma->dev_index, spin_lock_id);
#endif
	} else
		gps_each_link_spin_lock_give((enum gps_dl_link_id_enum)p_dma->dev_index, spin_lock_id);
}

