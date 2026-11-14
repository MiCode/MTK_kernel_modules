/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUSYS_NV_DATA_LAYOUT_H
#define _GPS_MCUSYS_NV_DATA_LAYOUT_H

#include "gps_mcusys_data.h" /* import gps_mcusys_nv_data_id */

#define GPS_MCUSYS_NV_DATA_EPO_MAX_SIZE           (50 * 1024)
#define GPS_MCUSYS_NV_DATA_GG_QEPO_MAX_SIZE       (6 * 1024)
#define GPS_MCUSYS_NV_DATA_BD_QEPO_MAX_SIZE       (6 * 1024)
#define GPS_MCUSYS_NV_DATA_GA_QEPO_MAX_SIZE       (6 * 1024)
#define GPS_MCUSYS_NV_DATA_NVFILE_MAX_SIZE        (128 * 1024)
#define GPS_MCUSYS_NV_DATA_CACHE_MAX_SIZE         (128 * 1024)
#define GPS_MCUSYS_NV_DATA_CONFIG_MAX_SIZE        (10 * 1024)
#define GPS_MCUSYS_NV_DATA_CONFIG_WRITE_MAX_SIZE  (10 * 1024)
#define GPS_MCUSYS_NV_DATA_DSPL1_MAX_SIZE         (1 * 1024)
#define GPS_MCUSYS_NV_DATA_DSPL5_MAX_SIZE         (1 * 1024)
#define GPS_MCUSYS_NV_DATA_XEPO_MAX_SIZE          (20 * 1024)
#define GPS_MCUSYS_NV_DATA_QZ_QEPO_MAX_SIZE       (1 * 1024)
#define GPS_MCUSYS_NV_DATA_IR_QEPO_MAX_SIZE       (1 * 1024)
#define GPS_MCUSYS_NV_DATA_KR_QEPO_MAX_SIZE       (1 * 1024)
#define GPS_MCUSYS_NV_DATA_DSPL1_CW_MAX_SIZE      (1 * 1024)
#define GPS_MCUSYS_NV_DATA_DSPL5_CW_MAX_SIZE      (1 * 1024)
#define GPS_MCUSYS_NV_DATA_MPENV_MAX_SIZE         (1 * 1024)
#define GPS_MCUSYS_NV_DATA_MPE_CFG_MAX_SIZE       (1 * 1024)
#define GPS_MCUSYS_NV_DATA_AP_MPE_MAX_SIZE        (32 * 1024)
#define GPS_MCUSYS_NV_DATA_RAW_MEAS_MAX_SIZE      (80 * 1024)
#define GPS_MCUSYS_NV_DATA_RAW_HIGEO_MAX_SIZE     (24 * 1024)

#define GPS_MCUSYS_NV_DATA_HEADER_MAGIC 0x12345678
struct gps_mcusys_nv_data_sub_header {
	gpsmdl_u32 block_size; /* with hdr */
	enum gps_mcusys_nv_data_id id;
	gpsmdl_u32 attribute;
	gpsmdl_u32 magic;

	gpsmdl_u32 occupied;
	gpsmdl_u32 version;
	gpsmdl_u32 data_size; /* max data_size = block_size - all hdr length */
	gpsmdl_u32 read_times;

	gpsmdl_u32 write_times;
	gpsmdl_u32 flags; /*bit0, is open */
	gpsmdl_u32 reserved2;
	gpsmdl_u32 reserved3;
};

struct gps_mcusys_nv_data_header {
	struct gps_mcusys_nv_data_sub_header hdr_host;   /* write by host */
	struct gps_mcusys_nv_data_sub_header hdr_target; /* write by target */
	gpsmdl_u32 data_start[1];
};

union gps_mcusys_nv_data_epo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_EPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_gg_qepo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_GG_QEPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_bd_qepo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_BD_QEPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_ga_qepo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_GA_QEPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_nvfile {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_NVFILE_MAX_SIZE/4];
};

union gps_mcusys_nv_data_cache {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_CACHE_MAX_SIZE/4];
};

union gps_mcusys_nv_data_config {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_CONFIG_MAX_SIZE/4];
};

union gps_mcusys_nv_data_config_write {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_CONFIG_WRITE_MAX_SIZE/4];
};

union gps_mcusys_nv_data_dspl1 {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_DSPL1_MAX_SIZE/4];
};

union gps_mcusys_nv_data_dspl5 {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_DSPL5_MAX_SIZE/4];
};

union gps_mcusys_nv_data_xepo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_XEPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_qz_qepo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_QZ_QEPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_ir_qepo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_IR_QEPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_kr_qepo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_KR_QEPO_MAX_SIZE/4];
};

union gps_mcusys_nv_data_dspl1_cw {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_DSPL1_CW_MAX_SIZE/4];
};

union gps_mcusys_nv_data_dspl5_cw {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_DSPL5_CW_MAX_SIZE/4];
};

union gps_mcusys_nv_data_mpenv {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_MPENV_MAX_SIZE/4];
};

union gps_mcusys_nv_data_mpe_cfg {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_MPE_CFG_MAX_SIZE/4];
};

union gps_mcusys_nv_data_ap_mpe {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_AP_MPE_MAX_SIZE/4];
};

union gps_mcusys_nv_data_raw_meas {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_RAW_MEAS_MAX_SIZE/4];
};

union gps_mcusys_nv_data_raw_higeo {
	struct gps_mcusys_nv_data_header hdr;
	gpsmdl_u32 hdr_and_data[GPS_MCUSYS_NV_DATA_RAW_HIGEO_MAX_SIZE/4];
};

#define GPS_MCU_NV_DATA_EMI_OFFSET      (0xD00000) /* 0xD0_0000 == 13MB */
#define GPS_MCU_NV_DATA_MCU_START_ADDR  (0x70D00000)

struct gps_mcusys_nv_data_layout {
	union gps_mcusys_nv_data_epo          epo;
	union gps_mcusys_nv_data_gg_qepo      qepo_gg;
	union gps_mcusys_nv_data_bd_qepo      qepo_bd;
	union gps_mcusys_nv_data_ga_qepo      qepo_ga;
	union gps_mcusys_nv_data_nvfile       nvfile;
	union gps_mcusys_nv_data_cache        cache;
	union gps_mcusys_nv_data_config       config;
	union gps_mcusys_nv_data_config_write config_wr;
	union gps_mcusys_nv_data_dspl1        dsp0;
	union gps_mcusys_nv_data_dspl5        dsp1;
	union gps_mcusys_nv_data_xepo         xepo;
	union gps_mcusys_nv_data_qz_qepo      qz_qepo;
	union gps_mcusys_nv_data_ir_qepo      ir_qepo;
	union gps_mcusys_nv_data_kr_qepo      kr_qepo;
	union gps_mcusys_nv_data_dspl1_cw     dsp0_cw;
	union gps_mcusys_nv_data_dspl5_cw     dsp1_cw;
	union gps_mcusys_nv_data_mpenv        mpenv;
	union gps_mcusys_nv_data_mpe_cfg      mpe_cfg;
	union gps_mcusys_nv_data_ap_mpe       ap_mpe;
	union gps_mcusys_nv_data_raw_meas     raw_meas;
	union gps_mcusys_nv_data_raw_higeo    raw_higeo;
};

#endif /* _GPS_MCUSYS_NV_DATA_LAYOUT_H */

