/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_mcusys_fsm.h"
#include "gps_mcusys_nv_data.h"
#include "gps_mcusys_nv_data_api.h"
#include "gps_mcusys_nv_data_layout.h"
#include "gps_mcusys_nv_common_impl.h"
#include "gps_mcusys_nv_per_side_macro.h"
#include "gps_nv_each_device.h"
#include "gps_each_link.h"
#include "gps_mcudl_emi_layout.h"
#if GPS_DL_ON_LINUX
#include <asm/io.h>
#endif


struct gps_mcusys_nv_data_layout *g_host_nv_layout_ptr;

struct gps_mcusys_nv_data_header *gps_mcusys_nv_data_get_hdr(enum gps_mcusys_nv_data_id nv_id)
{
	if (g_host_nv_layout_ptr == NULL) {
		g_host_nv_layout_ptr =
			(struct gps_mcusys_nv_data_layout *)gps_mcudl_plat_nv_emi_get_start_ptr();
		GPS_OFL_TRC("g_host_nv_layout_ptr = 0x%p, offset=0x%x, sz=0x%lx",
			g_host_nv_layout_ptr,
			gps_mcudl_get_offset_from_conn_base(g_host_nv_layout_ptr),
			sizeof(*g_host_nv_layout_ptr));
		if (g_host_nv_layout_ptr == NULL)
			return (struct gps_mcusys_nv_data_header *)NULL;
		gps_mcusys_nv_data_host_init();
	}

	switch (nv_id) {
	case GPS_MCUSYS_NV_DATA_ID_EPO:
		return &g_host_nv_layout_ptr->epo.hdr;
	case GPS_MCUSYS_NV_DATA_ID_GG_QEPO:
		return &g_host_nv_layout_ptr->qepo_gg.hdr;
	case GPS_MCUSYS_NV_DATA_ID_BD_QEPO:
		return &g_host_nv_layout_ptr->qepo_bd.hdr;
	case GPS_MCUSYS_NV_DATA_ID_GA_QEPO:
		return &g_host_nv_layout_ptr->qepo_ga.hdr;
	case GPS_MCUSYS_NV_DATA_ID_NVFILE:
		return &g_host_nv_layout_ptr->nvfile.hdr;
	case GPS_MCUSYS_NV_DATA_ID_CACHE:
		return &g_host_nv_layout_ptr->cache.hdr;
	case GPS_MCUSYS_NV_DATA_ID_CONFIG:
		return &g_host_nv_layout_ptr->config.hdr;
	case GPS_MCUSYS_NV_DATA_ID_CONFIG_WRITE:
		return &g_host_nv_layout_ptr->config_wr.hdr;
	case GPS_MCUSYS_NV_DATA_ID_DSPL1:
		return &g_host_nv_layout_ptr->dsp0.hdr;
	case GPS_MCUSYS_NV_DATA_ID_DSPL5:
		return &g_host_nv_layout_ptr->dsp1.hdr;
	case GPS_MCUSYS_NV_DATA_ID_XEPO:
		return &g_host_nv_layout_ptr->xepo.hdr;
	case GPS_MCUSYS_NV_DATA_ID_QZ_QEPO:
		return &g_host_nv_layout_ptr->qz_qepo.hdr;
	case GPS_MCUSYS_NV_DATA_ID_IR_QEPO:
		return &g_host_nv_layout_ptr->ir_qepo.hdr;
	case GPS_MCUSYS_NV_DATA_ID_KR_QEPO:
		return &g_host_nv_layout_ptr->kr_qepo.hdr;
	case GPS_MCUSYS_NV_DATA_ID_DSPL1_CW:
		return &g_host_nv_layout_ptr->dsp0_cw.hdr;
	case GPS_MCUSYS_NV_DATA_ID_DSPL5_CW:
		return &g_host_nv_layout_ptr->dsp1_cw.hdr;
	case GPS_MCUSYS_NV_DATA_ID_MPENV:
		return &g_host_nv_layout_ptr->mpenv.hdr;
	case GPS_MCUSYS_NV_DATA_ID_MPE_CFG:
		return &g_host_nv_layout_ptr->mpe_cfg.hdr;
	case GPS_MCUSYS_NV_DATA_ID_AP_MPE:
		return &g_host_nv_layout_ptr->ap_mpe.hdr;
	case GPS_MCUSYS_NV_DATA_ID_RAW_MEAS:
		return &g_host_nv_layout_ptr->raw_meas.hdr;
	case GPS_MCUSYS_NV_DATA_ID_RAW_HIGEO:
		return &g_host_nv_layout_ptr->raw_higeo.hdr;
	default:
		break;
	}
	return NULL;
}

gpsmdl_u32 gps_mcusys_nv_data_get_block_size(enum gps_mcusys_nv_data_id nv_id)
{
	switch (nv_id) {
	case GPS_MCUSYS_NV_DATA_ID_EPO:
		return GPS_MCUSYS_NV_DATA_EPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_GG_QEPO:
		return GPS_MCUSYS_NV_DATA_GG_QEPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_BD_QEPO:
		return GPS_MCUSYS_NV_DATA_BD_QEPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_GA_QEPO:
		return GPS_MCUSYS_NV_DATA_GA_QEPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_NVFILE:
		return GPS_MCUSYS_NV_DATA_NVFILE_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_CACHE:
		return GPS_MCUSYS_NV_DATA_CACHE_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_CONFIG:
		return GPS_MCUSYS_NV_DATA_CONFIG_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_CONFIG_WRITE:
		return GPS_MCUSYS_NV_DATA_CONFIG_WRITE_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_DSPL1:
		return GPS_MCUSYS_NV_DATA_DSPL1_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_DSPL5:
		return GPS_MCUSYS_NV_DATA_DSPL5_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_XEPO:
		return GPS_MCUSYS_NV_DATA_XEPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_QZ_QEPO:
		return GPS_MCUSYS_NV_DATA_QZ_QEPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_IR_QEPO:
		return GPS_MCUSYS_NV_DATA_IR_QEPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_KR_QEPO:
		return GPS_MCUSYS_NV_DATA_KR_QEPO_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_DSPL1_CW:
		return GPS_MCUSYS_NV_DATA_DSPL1_CW_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_DSPL5_CW:
		return GPS_MCUSYS_NV_DATA_DSPL5_CW_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_MPENV:
		return GPS_MCUSYS_NV_DATA_MPENV_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_MPE_CFG:
		return GPS_MCUSYS_NV_DATA_MPE_CFG_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_AP_MPE:
		return GPS_MCUSYS_NV_DATA_AP_MPE_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_RAW_MEAS:
		return GPS_MCUSYS_NV_DATA_RAW_MEAS_MAX_SIZE;
	case GPS_MCUSYS_NV_DATA_ID_RAW_HIGEO:
		return GPS_MCUSYS_NV_DATA_RAW_HIGEO_MAX_SIZE;
	default:
		break;
	}
	return 0;
}

void gps_mcusys_nv_data_host_hdr_init(enum gps_mcusys_nv_data_id nv_id,
	struct gps_mcusys_nv_data_sub_header *p_host)
{
	gpsmdl_u32 block_size = gps_mcusys_nv_data_get_block_size(nv_id);

	GPS_OFL_DBG("block_size=%d, p_host=0x%p", block_size, p_host);

	if (block_size == 0)
		return;

	/*variable in nv data layout  is union.
	* Size of union is determined by the size of the largest member.
	* thus, block_size should be less than initial block_size.
	* block_size = block_size - sizeof(header)
	*/
	block_size = block_size - sizeof(struct gps_mcusys_nv_data_header);

	/* it has once been initialised if magic is matching and no need to do init again.*/
	if (p_host->magic == GPS_MCUSYS_NV_DATA_HEADER_MAGIC &&
		p_host->block_size == block_size &&
		p_host->id == nv_id)
		return;

#if GPS_DL_ON_LINUX
	memset_io(p_host, 0, sizeof(*p_host));
#else
	memset(p_host, 0, sizeof(*p_host));
#endif
	p_host->block_size = block_size;
	p_host->id = nv_id;
	p_host->magic = GPS_MCUSYS_NV_DATA_HEADER_MAGIC;
}

void gps_mcusys_nv_data_host_init(void)
{
	enum gps_mcusys_nv_data_id nv_id;
	struct gps_mcusys_nv_data_header *p_hdr;

	gps_mcudl_plat_nv_emi_clear();
	for (nv_id = 0; nv_id < GPS_MCUSYS_NV_DATA_NUM; nv_id++) {
		p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
		if (p_hdr != NULL) {
			GPS_OFL_DBG("nv_id=%d, p_hdr=0x%p, offset=0x%x",
				nv_id, p_hdr, gps_mcudl_get_offset_from_conn_base(p_hdr));
			gps_mcusys_nv_data_host_hdr_init(nv_id, &p_hdr->hdr_host);
		}
	}
	GPS_OFL_DBG("");
}


void gps_mcusys_nv_data_on_gpsbin_state(enum gps_mcusys_gpsbin_state gpsbin_state)
{
	enum gps_mcusys_nv_data_id nv_id;
	enum gps_mcusys_nvlock_event_id evt_id;

	switch (gpsbin_state) {
	case GPS_MCUSYS_GPSBIN_PRE_ON:
#if 1
		/* Toggle gps_mcusys_nv_data_host_init()
		 * by gps_mcusys_nv_data_get_hdr if not ever init.
		 */
		(void)gps_mcusys_nv_data_get_hdr(GPS_MCUSYS_NV_DATA_ID_EPO);
#endif
		evt_id = GPS_MCUSYS_NVLOCK_MCU_PRE_ON;
		break;
	case GPS_MCUSYS_GPSBIN_POST_ON:
		evt_id = GPS_MCUSYS_NVLOCK_MCU_POST_ON;
		break;
	case GPS_MCUSYS_GPSBIN_PRE_OFF:
		evt_id = GPS_MCUSYS_NVLOCK_MCU_PRE_OFF;
		break;
	case GPS_MCUSYS_GPSBIN_POST_OFF:
		evt_id = GPS_MCUSYS_NVLOCK_MCU_POST_OFF;
		break;
	default:
		return;
	}

	for (nv_id = 0; nv_id < GPS_MCUSYS_NV_DATA_NUM; nv_id++) {
		/*gps_mcusys_nvlock_fsm(nv_id, evt_id);*/
		/*gps_mcusys_nvlock_fsm(nv_id, GPS_MCUSYS_NVLOCK_MCU_POST_ON);*/
	}

	GPS_OFL_TRC("evt_id=%d", evt_id);
}

void gps_mcusys_nvdata_on_remote_event(enum gps_mcusys_nv_data_id nv_id,
	enum gps_mcusys_nvdata_event_id data_evt)
{
	struct gps_each_link_waitable *p;
	bool do_wake_really;

	switch (data_evt) {
	case GPS_MCUSYS_NVDATA_MCU_WRITE:
	case GPS_MCUSYS_NVDATA_MCU_DELETE:
#if GPS_DL_ON_LINUX
		p = gps_nv_each_link_get_read_waitable_ptr(nv_id);
		if (!p) {
			GPS_OFL_TRC("nv_id=%d, bypass do_wake", nv_id);
			break;
		}
		do_wake_really = gps_dl_link_wake_up2(p);
		GPS_OFL_TRC("nv_id=%d, data_evt=%d, do_wake_really=%d", nv_id, data_evt, do_wake_really);
#endif
		break;
	default:
		GPS_OFL_TRC("nv_id=%d, data_evt=%d", nv_id, data_evt);
		break;
	}
}

