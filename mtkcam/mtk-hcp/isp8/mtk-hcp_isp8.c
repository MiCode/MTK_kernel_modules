// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/slab.h>
#include <linux/kref.h>
#include <linux/version.h>
#include <mtk_heap.h>
#include "mtk-hcp_isp8.h"


#if SMVR_DECOUPLE
static struct mtk_hcp_streaming_reserve_mblock *mb;
static struct mtk_hcp_smvr_reserve_mblock *smb;
static struct mtk_hcp_capture_reserve_mblock *cmb;
static struct mtk_hcp_gce_token_reserve_mblock *gmb;
#else
static struct mtk_hcp_reserve_mblock *mb;
#endif

#if !SMVR_DECOUPLE
enum isp8_rsv_mem_id_t {
	DIP_MEM_FOR_HW_ID,
	IMG_MEM_FOR_HW_ID = DIP_MEM_FOR_HW_ID, /*shared buffer for ipi_param*/
	/*need replace DIP_MEM_FOR_HW_ID & DIP_MEM_FOR_SW_ID*/
	WPE_MEM_C_ID,	/*module cq buffer*/
	WPE_MEM_T_ID,	/*module tdr buffer*/
	OMC_MEM_C_ID,	/*module cq buffer*/
	OMC_MEM_T_ID,	/*module tdr buffer*/
	TRAW_MEM_C_ID,	/*module cq buffer*/
	TRAW_MEM_T_ID,	/*module tdr buffer*/
	DIP_MEM_C_ID,	/*module cq buffer*/
	DIP_MEM_T_ID,	/*module tdr buffer*/
	PQDIP_MEM_C_ID,	/*module cq buffer*/
	PQDIP_MEM_T_ID,	/*module tdr buffer*/
	ADL_MEM_C_ID,	/*module cq buffer*/
	ADL_MEM_T_ID,	/*module tdr buffer*/
	ME_MEM_C_ID,    /*module cq buffer*/
	IMG_MEM_G_ID,	/*gce cmd buffer*/
	NUMS_MEM_ID,
};
#else
enum isp8_rsv_mem_id_t {
	/*need replace DIP_MEM_FOR_HW_ID & DIP_MEM_FOR_SW_ID*/
	WPE_MEM_C_ID,	/*module cq buffer*/
	WPE_MEM_T_ID,	/*module tdr buffer*/
	OMC_MEM_C_ID,	/*module cq buffer*/
	OMC_MEM_T_ID,	/*module tdr buffer*/
	TRAW_MEM_C_ID,	/*module cq buffer*/
	TRAW_MEM_T_ID,	/*module tdr buffer*/
	DIP_MEM_C_ID,	/*module cq buffer*/
	DIP_MEM_T_ID,	/*module tdr buffer*/
	PQDIP_MEM_C_ID,	/*module cq buffer*/
	PQDIP_MEM_T_ID,	/*module tdr buffer*/
	ADL_MEM_C_ID,	/*module cq buffer*/
	ADL_MEM_T_ID,	/*module tdr buffer*/
	ME_MEM_C_ID,    /*module cq buffer*/
	IMG_MEM_G_ID,	/*gce cmd buffer*/
	NUMS_WB_MEM_ID,
};

enum isp8_rsv_gce_mem_id_t {
	DIP_MEM_FOR_HW_ID = (IMG_MEM_G_ID + 1),
	IMG_MEM_FOR_HW_ID = DIP_MEM_FOR_HW_ID, /*shared buffer for ipi_param*/
	IMG_MEM_G_TOKEN_ID,	/*gce cmd buffer*/
	NUMS_CM_MEM_ID,
};
#endif

#if !SMVR_DECOUPLE
static struct mtk_hcp_reserve_mblock isp8_smvr_mblock[] = {
	{
		/*share buffer for frame setting, to be sw usage*/
		.name = "IMG_MEM_FOR_HW_ID",
		.num = IMG_MEM_FOR_HW_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x400000,   /*need more than 4MB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x19B000,   /*1680132 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "WPE_MEM_T_ID",
		.num = WPE_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x233000,  /*2305536 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "OMC_MEM_C_ID",
		.num = OMC_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x19B000,   /*1680132 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "OMC_MEM_T_ID",
		.num = OMC_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x233000,  /*2305536 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "TRAW_MEM_C_ID",
		.num = TRAW_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x71F000,  /*7463736 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "TRAW_MEM_T_ID",
		.num = TRAW_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xFD6000,  /*24065056 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "DIP_MEM_C_ID",
		.num = DIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xA91000,  /*11077920 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "DIP_MEM_T_ID",
		.num = DIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1443000,  /*24987648 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "PQDIP_MEM_C_ID",
		.num = PQDIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x16E000,  /*1498320 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "PQDIP_MEM_T_ID",
		.num = PQDIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x11E000,  /*1169408 bytes - use 6897 size*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "ADL_MEM_C_ID",
		.num = ADL_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x100000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL
	},
	{
		.name = "ADL_MEM_T_ID",
		.num = ADL_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x200000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL
	},
	{
		.name = "ME_MEM_C_ID",
		.num = ME_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xE100,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
	},
	{
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		/* align with isp7s for newp2 change */
		.size = 0x1F45E00, //0x1F21E00, //0x1885E00,//0x17F5E00, //0x3400000
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
};


struct mtk_hcp_reserve_mblock isp8_reserve_mblock[] = {
	{
		/*share buffer for frame setting, to be sw usage*/
		.name = "IMG_MEM_FOR_HW_ID",
		.num = IMG_MEM_FOR_HW_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x400000,   /*need more than 4MB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x73000,   /*470532 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "WPE_MEM_T_ID",
		.num = WPE_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x9E000,   /*646656 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "OMC_MEM_C_ID",
		.num = OMC_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x73000,   /*470532 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "OMC_MEM_T_ID",
		.num = OMC_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x9E000,   /*646656 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "TRAW_MEM_C_ID",
		.num = TRAW_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x380000,   /*3668616 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "TRAW_MEM_T_ID",
		.num = TRAW_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xD66000,   /*21509152 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "DIP_MEM_C_ID",
		.num = DIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x564000,   /*5652000 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "DIP_MEM_T_ID",
		.num = DIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x10D1000,   /*21374976 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "PQDIP_MEM_C_ID",
		.num = PQDIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xDC000,   /*898992 bytes*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "PQDIP_MEM_T_ID",
		.num = PQDIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xAC000,   /*702464 bytes - use 6897 size*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "ADL_MEM_C_ID",
		.num = ADL_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x100000,   /*1MB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL
	},
	{
		.name = "ADL_MEM_T_ID",
		.num = ADL_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x200000,   /*2MB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL
	},
	{
		.name = "ME_MEM_C_ID",
		.num = ME_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xE100,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
	},
	{
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		/* align with isp7s for newp2 change */
		.size = 0x11D8400, //0x11CC400, //0xF98400,//0xF68400, //0x2000000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
};
#else

static struct mtk_hcp_smvr_reserve_mblock isp8_smvr_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xD6000, //align_4096(0xD5140)
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "WPE_MEM_T_ID",
		.num = WPE_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x19F000, /*align_4096(0x19E400) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "OMC_MEM_C_ID",
		.num = OMC_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x4F000, //align_4096(0x4EC00)
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "OMC_MEM_T_ID",
		.num = OMC_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x95000, /*align_4096(0x94C00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "TRAW_MEM_C_ID",
		.num = TRAW_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x4A2000, /*align_4096(0x4A1C00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "TRAW_MEM_T_ID",
		.num = TRAW_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x72C000, /*align_4096(0x72B780) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "DIP_MEM_C_ID",
		.num = DIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x776000, /*align_4096(0x775D00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "DIP_MEM_T_ID",
		.num = DIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xAC9000, /*align_4096(0xAC8E00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "PQDIP_MEM_C_ID",
		.num = PQDIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x24A000, /*align_4096(0x249480) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "PQDIP_MEM_T_ID",
		.num = PQDIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x241000, /*align_4096(0x240800) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
	{
		.name = "ADL_MEM_C_ID",
		.num = ADL_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.smvr = 1
	},
	{
		.name = "ADL_MEM_T_ID",
		.num = ADL_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.smvr = 1
	},
	{
		.name = "ME_MEM_C_ID",
		.num = ME_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1D000, /*align_4096(0x1C200) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.smvr = 1
	},
	{
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xE7C400, //0x120C400,//to do for smvr //0xF68400, //0x2000000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.smvr = 1
	},
};


static struct mtk_hcp_streaming_reserve_mblock isp8_streaming_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x48000,   /*align_4096(0x47140) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "WPE_MEM_T_ID",
		.num = WPE_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x46000,   /*align_4096(0x45400) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "OMC_MEM_C_ID",
		.num = OMC_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1B000,   /*align_4096(0x1A400) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "OMC_MEM_T_ID",
		.num = OMC_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x20000,   /*align_4096(0x1FC00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "TRAW_MEM_C_ID",
		.num = TRAW_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x251000,   /*align_4096(0x250E00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "TRAW_MEM_T_ID",
		.num = TRAW_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x585000,   /*align_4096(0x584780) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "DIP_MEM_C_ID",
		.num = DIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x3BB000,   /*align_4096(0x3BAE80) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "DIP_MEM_T_ID",
		.num = DIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x78D000,   /*align_4096(0x78CE00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "PQDIP_MEM_C_ID",
		.num = PQDIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x125000,    /*align_4096(0x124A40) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "PQDIP_MEM_T_ID",
		.num = PQDIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xD9000,   /*align_4096(0xD8800) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
	{
		.name = "ADL_MEM_C_ID",
		.num = ADL_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1000,   /*4KB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.streaming = 1
	},
	{
		.name = "ADL_MEM_T_ID",
		.num = ADL_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1000,   /*4KB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.streaming = 1
	},
	{
		.name = "ME_MEM_C_ID",
		.num = ME_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xF000,   /*align_4096(0xE100) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.streaming = 1
	},
	{
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x62D000, /* align_4096(0x62CF00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.streaming = 1
	},
};

static struct mtk_hcp_capture_reserve_mblock isp8_capture_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x5000, /*align_4096(0x4840) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "WPE_MEM_T_ID",
		.num = WPE_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xB000, /*align_4096(0xA400) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "OMC_MEM_C_ID",
		.num = OMC_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x5000, /*align_4096(0x4600) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "OMC_MEM_T_ID",
		.num = OMC_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1000, /*align_4096(0x400) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "TRAW_MEM_C_ID",
		.num = TRAW_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x84000, /*align_4096(0x83C00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "TRAW_MEM_T_ID",
		.num = TRAW_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x80A000, /*align_4096(0x809780) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "DIP_MEM_C_ID",
		.num = DIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1A2000, /*align_4096(0x1A1C80) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "DIP_MEM_T_ID",
		.num = DIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x92B000, /*align_4096(0x92AE00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "PQDIP_MEM_C_ID",
		.num = PQDIP_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x31000,   /*align_4096(0x30C80) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "PQDIP_MEM_T_ID",
		.num = PQDIP_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x21000,   /*align_4096(0x20800) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
	{
		.name = "ADL_MEM_C_ID",
		.num = ADL_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1000,   /*4KB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.capture = 1
	},
	{
		.name = "ADL_MEM_T_ID",
		.num = ADL_MEM_T_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1000,   /*4KB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.capture = 1
	},
	{
		.name = "ME_MEM_C_ID",
		.num = ME_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x5000,   /*align_4096(0x4B00) */
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.capture = 1
	},
	{
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x4C5000, /* align_4096(0x4C4C00)*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.capture = 1
	},
};

static struct mtk_hcp_gce_token_reserve_mblock isp8_gce_mblock[] = {
	{
		/*share buffer for frame setting, to be sw usage*/
		.name = "IMG_MEM_FOR_HW_ID",
		.num = IMG_MEM_FOR_HW_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x400000,   /*need more than 4MB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.gce = 1
	},
	{
		.name = "IMG_MEM_G_TOKEN_ID",
		.num = IMG_MEM_G_TOKEN_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x200,//to do for smvr //0xF68400, //0x2000000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL,
		.gce = 1
	},
};
#if 0
static struct mtk_hcp_reserve_mblock isp7s_gce_smvr_mblock[] = {
{
		/*share buffer for frame setting, to be sw usage*/
		.name = "IMG_MEM_FOR_HW_ID",
		.num = IMG_MEM_FOR_HW_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x400000,   /*need more than 4MB*/
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
	{
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x1F45E00,//to do for smvr //0xF68400, //0x2000000,
		.is_dma_buf = true,
		.mmap_cnt = 0,
		.mem_priv = NULL,
		.d_buf = NULL,
		.fd = -1,
		.pIonHandle = NULL,
		.attach = NULL,
		.sgt = NULL
	},
};
#endif /* #if 0 */
#endif /* #if !SMVR_DECOUPLE */
#if SMVR_DECOUPLE
phys_addr_t isp8_get_reserve_mem_phys(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %d", id);
		return 0;
	} else {
		if ((id == IMG_MEM_G_TOKEN_ID) || (id == IMG_MEM_FOR_HW_ID)) {
			return gmb[id - IMG_MEM_FOR_HW_ID].start_phys;
		} else {
			if (mode == imgsys_streaming)
				return mb[id].start_phys;
			else if(mode == imgsys_capture)
				return cmb[id].start_phys;
			else
				return smb[id].start_phys;
		}
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_phys);

void *isp8_get_reserve_mem_virt(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		if ((id == IMG_MEM_G_TOKEN_ID) || (id == IMG_MEM_FOR_HW_ID)) {
			return gmb[id - IMG_MEM_FOR_HW_ID].start_virt;
		} else {
			if (mode == imgsys_streaming)
				return mb[id].start_virt;
			else if(mode == imgsys_capture)
				return cmb[id].start_virt;
			else
				return smb[id].start_virt;
		}
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_virt);

phys_addr_t isp8_get_reserve_mem_dma(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		if ((id == IMG_MEM_G_TOKEN_ID) || (id == IMG_MEM_FOR_HW_ID)) {
			return gmb[id - IMG_MEM_FOR_HW_ID].start_dma;
		} else {
			if (mode == imgsys_streaming)
				return mb[id].start_dma;
			else if(mode == imgsys_capture)
				return cmb[id].start_dma;
			else
				return smb[id].start_dma;
		}
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_dma);

phys_addr_t isp8_get_reserve_mem_size(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		if ((id == IMG_MEM_G_TOKEN_ID) || (id == IMG_MEM_FOR_HW_ID)) {
			return gmb[id - IMG_MEM_FOR_HW_ID].size;
		} else {
			if (mode == imgsys_streaming)
				return mb[id].size;
			else if(mode == imgsys_capture)
				return cmb[id].size;
			else
				return smb[id].size;
		}
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_size);

uint32_t isp8_get_reserve_mem_fd(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		if ((id == IMG_MEM_G_TOKEN_ID) || (id == IMG_MEM_FOR_HW_ID)) {
			return gmb[id - IMG_MEM_FOR_HW_ID].fd;
		} else {
			if (mode == imgsys_streaming)
				return mb[id].fd;
			else if(mode == imgsys_capture)
				return cmb[id].fd;
			else
				return smb[id].fd;
		}
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_fd);

void *isp8_get_gce_virt(unsigned int mode)
{
	if (mode == imgsys_streaming) {
		//pr_info("mtk_hcp-get_gce-streaming(0x%lx)", (unsigned long)mb[IMG_MEM_G_ID].start_virt);
		return mb[IMG_MEM_G_ID].start_virt;
	}
	else if(mode == imgsys_capture) {
		//pr_info("mtk_hcp-get_gce-capture(0x%lx)", (unsigned long)cmb[IMG_MEM_G_ID].start_virt);
		return cmb[IMG_MEM_G_ID].start_virt;
	}
	else {
		//pr_info("mtk_hcp-get_gce-smvr(0x%lx)", (unsigned long)smb[IMG_MEM_G_ID].start_virt);
		return smb[IMG_MEM_G_ID].start_virt;
	}
}
EXPORT_SYMBOL(isp8_get_gce_virt);

void *isp8_get_gce_token_virt(unsigned int mode)
{
	if (hcp_dbg_enable()) {
		pr_info("mtk_hcp gce_start_virt(0x%lx)",
			(unsigned long)gmb[IMG_MEM_G_TOKEN_ID - IMG_MEM_FOR_HW_ID].start_virt);
	}
	return gmb[IMG_MEM_G_TOKEN_ID - IMG_MEM_FOR_HW_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_gce_token_virt);

void *isp8_get_wpe_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[WPE_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_C_ID].start_virt;
	else
		return smb[WPE_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_wpe_virt);

int isp8_get_wpe_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[WPE_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_C_ID].fd;
	else
		return smb[WPE_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_wpe_cq_fd);

int isp8_get_wpe_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[WPE_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_T_ID].fd;
	else
		return smb[WPE_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_wpe_tdr_fd);

void *isp8_get_omc_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[OMC_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[OMC_MEM_C_ID].start_virt;
	else
		return smb[OMC_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_omc_virt);

int isp8_get_omc_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[OMC_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[OMC_MEM_C_ID].fd;
	else
		return smb[OMC_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_omc_cq_fd);

int isp8_get_omc_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[OMC_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[OMC_MEM_T_ID].fd;
	else
		return smb[OMC_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_omc_tdr_fd);

void *isp8_get_dip_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[DIP_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_C_ID].start_virt;
	else
		return smb[DIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_dip_virt);

int isp8_get_dip_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[DIP_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_C_ID].fd;
	else
		return smb[DIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_dip_cq_fd);

int isp8_get_dip_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[DIP_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_T_ID].fd;
	else
		return smb[DIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_dip_tdr_fd);

void *isp8_get_traw_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[TRAW_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_C_ID].start_virt;
	else
		return smb[TRAW_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_traw_virt);

int isp8_get_traw_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[TRAW_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_C_ID].fd;
	else
		return smb[TRAW_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_traw_cq_fd);

int isp8_get_traw_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[TRAW_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_T_ID].fd;
	else
		return smb[TRAW_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_traw_tdr_fd);

void *isp8_get_pqdip_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[PQDIP_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_C_ID].start_virt;
	else
		return smb[PQDIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_pqdip_virt);

int isp8_get_pqdip_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[PQDIP_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_C_ID].fd;
	else
		return smb[PQDIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_pqdip_cq_fd);

int isp8_get_pqdip_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[PQDIP_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_T_ID].fd;
	else
		return smb[PQDIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_pqdip_tdr_fd);

void *isp8_get_hwid_virt(unsigned int mode)
{
    return gmb[0].start_virt;
}
EXPORT_SYMBOL(isp8_get_hwid_virt);

void *isp8_get_me_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[ME_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[ME_MEM_C_ID].start_virt;
	else
		return smb[ME_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_me_virt);

int isp8_get_me_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[ME_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[ME_MEM_C_ID].fd;
	else
		return smb[ME_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_me_cq_fd);

static int isp8_module_driver_allocate_working_buffer_streaming(struct mtk_hcp *hcp_dev,
	unsigned int str_mode, struct mtk_hcp_streaming_reserve_mblock *str_mblock)
{
	int id = 0;
	struct sg_table *sgt = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct dma_heap *pdma_heap = NULL;
	struct iosys_map map = {0};
	int ret = 0;
	unsigned int block_num = 0;

	block_num = hcp_dev->data->block_num;
	pr_info("mtk_hcp-%s-%d", __func__, block_num);
	for (id = 0; id < block_num; id++) {
		if (str_mblock[id].is_dma_buf) {
			switch (id) {
			case IMG_MEM_G_ID:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					str_mblock[id].d_buf = dma_heap_buffer_alloc(
					pdma_heap,
					str_mblock[id].size,
					O_RDWR | O_CLOEXEC,
					DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(str_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
							PTR_ERR(str_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(str_mblock[id].d_buf, str_mblock[id].name);
					str_mblock[id].attach =
						dma_buf_attach(str_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = str_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					str_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					str_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = str_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(str_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					str_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					str_mblock[id].start_dma = str_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(str_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(str_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("vmap_dma_address fail(%d)\n", ret);
						return ret;
					}
					str_mblock[id].start_virt = (void *)map.vaddr;
					str_mblock[id].map = map;
					get_dma_buf(str_mblock[id].d_buf);
					str_mblock[id].fd =
						dma_buf_fd(str_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					dma_buf_begin_cpu_access(str_mblock[id].d_buf, DMA_BIDIRECTIONAL);
					kref_init(&str_mblock[id].kref);
					if (hcp_dbg_enable()) {
						pr_info("%s:[HCP_GCE][%s] phys:0x%llx, virt:0x%p, dma:0x%llx,"
							" size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, str_mblock[id].name,
							isp8_get_reserve_mem_phys(id, str_mode),
							isp8_get_reserve_mem_virt(id, str_mode),
							isp8_get_reserve_mem_dma(id, str_mode),
							isp8_get_reserve_mem_size(id, str_mode),
							str_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, str_mode),
							str_mblock[id].d_buf);
					}
					break;
			case WPE_MEM_C_ID:
			case WPE_MEM_T_ID:
			case OMC_MEM_C_ID:
			case OMC_MEM_T_ID:
			case DIP_MEM_C_ID:
			case DIP_MEM_T_ID:
			case TRAW_MEM_C_ID:
			case TRAW_MEM_T_ID:
			case PQDIP_MEM_C_ID:
			case PQDIP_MEM_T_ID:
			case ADL_MEM_C_ID:
			case ADL_MEM_T_ID:
			case ME_MEM_C_ID:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm-uncached");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					str_mblock[id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						str_mblock[id].size, O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(str_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
						PTR_ERR(str_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(str_mblock[id].d_buf, str_mblock[id].name);
					str_mblock[id].attach = dma_buf_attach(
					str_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = str_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					str_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					str_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = str_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(str_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					str_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					str_mblock[id].start_dma = str_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(str_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(str_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("vmap_dma_address fail\n");
						return ret;
					}
					str_mblock[id].start_virt = (void *)map.vaddr;
					str_mblock[id].map = map;
					get_dma_buf(str_mblock[id].d_buf);
					str_mblock[id].fd =
					dma_buf_fd(str_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					if (hcp_dbg_enable()) {
					pr_debug("%s:[HCP_WORKING][%s] phys:0x%llx, virt:0x%p,"
						" dma:0x%llx,size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
						__func__, str_mblock[id].name,
						isp8_get_reserve_mem_phys(id, str_mode),
						isp8_get_reserve_mem_virt(id, str_mode),
						isp8_get_reserve_mem_dma(id, str_mode),
						isp8_get_reserve_mem_size(id, str_mode),
						str_mblock[id].is_dma_buf,
						isp8_get_reserve_mem_fd(id, str_mode),
						str_mblock[id].d_buf);
					}
					break;
			default:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm-uncached");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					str_mblock[id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						str_mblock[id].size, O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(str_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
						PTR_ERR(str_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(str_mblock[id].d_buf, str_mblock[id].name);
					str_mblock[id].attach = dma_buf_attach(
					str_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = str_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					str_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					str_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = str_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(str_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					str_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					str_mblock[id].start_dma = str_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(str_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(str_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("sg_dma_address fail\n");
						return ret;
					}
					str_mblock[id].start_virt = (void *)map.vaddr;
					str_mblock[id].map = map;
					get_dma_buf(str_mblock[id].d_buf);
					str_mblock[id].fd =
					dma_buf_fd(str_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					if (hcp_dbg_enable()) {
						pr_debug("%s:[HCP_WORKING_DEFAULT][%s] phys:0x%llx,"
							" virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, str_mblock[id].name,
							isp8_get_reserve_mem_phys(id, str_mode),
							isp8_get_reserve_mem_virt(id, str_mode),
							isp8_get_reserve_mem_dma(id, str_mode),
							isp8_get_reserve_mem_size(id, str_mode),
							str_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, str_mode),
							str_mblock[id].d_buf);
					}
					break;
			}
		} else {
			str_mblock[id].start_virt =
				kzalloc(str_mblock[id].size, GFP_KERNEL);
			str_mblock[id].start_phys =
				virt_to_phys(str_mblock[id].start_virt);
			str_mblock[id].start_dma = 0;
		}
		if (hcp_dbg_enable()) {
			pr_debug(
				"%s: [HCP][mem_reserve-%d] phys:0x%llx,"
				" virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
				__func__, id,
				isp8_get_reserve_mem_phys(id, str_mode),
				isp8_get_reserve_mem_virt(id, str_mode),
				isp8_get_reserve_mem_dma(id, str_mode),
				isp8_get_reserve_mem_size(id, str_mode),
				str_mblock[id].is_dma_buf,
				isp8_get_reserve_mem_fd(id, str_mode),
				str_mblock[id].d_buf);
		}
	}
	return 0;
}

static int isp8_module_driver_allocate_working_buffer_capture(struct mtk_hcp *hcp_dev,
    unsigned int cap_mode, struct mtk_hcp_capture_reserve_mblock *cap_mblock)
{
	int id = 0;
	struct sg_table *sgt = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct dma_heap *pdma_heap = NULL;
	struct iosys_map map = {0};
	int ret = 0;
	unsigned int block_num;

	block_num = hcp_dev->data->block_num;
	for (id = 0; id < block_num; id++) {
		if (cap_mblock[id].is_dma_buf) {
			switch (id) {
			case IMG_MEM_G_ID:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					cap_mblock[id].d_buf = dma_heap_buffer_alloc(
					pdma_heap,
					cap_mblock[id].size,
					O_RDWR | O_CLOEXEC,
					DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(cap_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
							PTR_ERR(cap_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(cap_mblock[id].d_buf, cap_mblock[id].name);
					cap_mblock[id].attach =
						dma_buf_attach(cap_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = cap_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					cap_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					cap_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = cap_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(cap_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					cap_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					cap_mblock[id].start_dma = cap_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(cap_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(cap_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("sg_dma_address fail\n");
						return ret;
					}
					cap_mblock[id].start_virt = (void *)map.vaddr;
					cap_mblock[id].map = map;
					get_dma_buf(cap_mblock[id].d_buf);
					cap_mblock[id].fd =
						dma_buf_fd(cap_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					dma_buf_begin_cpu_access(cap_mblock[id].d_buf, DMA_BIDIRECTIONAL);
					kref_init(&cap_mblock[id].kref);
					if (hcp_dbg_enable()) {
						pr_debug("%s:[HCP_GCE][%s] phys:0x%llx, virt:0x%p,"
							" dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, cap_mblock[id].name,
							isp8_get_reserve_mem_phys(id, cap_mode),
							isp8_get_reserve_mem_virt(id, cap_mode),
							isp8_get_reserve_mem_dma(id, cap_mode),
							isp8_get_reserve_mem_size(id, cap_mode),
							cap_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, cap_mode),
							cap_mblock[id].d_buf);
					}
					break;
			case WPE_MEM_C_ID:
			case WPE_MEM_T_ID:
			case OMC_MEM_C_ID:
			case OMC_MEM_T_ID:
			case DIP_MEM_C_ID:
			case DIP_MEM_T_ID:
			case TRAW_MEM_C_ID:
			case TRAW_MEM_T_ID:
			case PQDIP_MEM_C_ID:
			case PQDIP_MEM_T_ID:
			case ADL_MEM_C_ID:
			case ADL_MEM_T_ID:
			case ME_MEM_C_ID:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm-uncached");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					cap_mblock[id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						cap_mblock[id].size, O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(cap_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
						PTR_ERR(cap_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(cap_mblock[id].d_buf, cap_mblock[id].name);
					cap_mblock[id].attach = dma_buf_attach(
					cap_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = cap_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					cap_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					cap_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = cap_mblock[id].sgt;
					if (IS_ERR(sgt)) {
					    dma_buf_detach(cap_mblock[id].d_buf, attach);
					    pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					    	PTR_ERR(sgt));
					    return -1;
					}
					cap_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					cap_mblock[id].start_dma = cap_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(cap_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(cap_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("vmap_dma_address fail\n");
						return ret;
					}
					cap_mblock[id].start_virt = (void *)map.vaddr;
					cap_mblock[id].map = map;
					get_dma_buf(cap_mblock[id].d_buf);
					cap_mblock[id].fd =
					dma_buf_fd(cap_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					if (hcp_dbg_enable()) {
						pr_debug("%s:[HCP_WORKING][%s] phys:0x%llx, virt:0x%p,"
							" dma:0x%llx,size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, cap_mblock[id].name,
							isp8_get_reserve_mem_phys(id, cap_mode),
							isp8_get_reserve_mem_virt(id, cap_mode),
							isp8_get_reserve_mem_dma(id, cap_mode),
							isp8_get_reserve_mem_size(id, cap_mode),
							cap_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, cap_mode),
							cap_mblock[id].d_buf);
					}
					break;
			default:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm-uncached");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					cap_mblock[id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						cap_mblock[id].size, O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(cap_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
						PTR_ERR(cap_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(cap_mblock[id].d_buf, cap_mblock[id].name);
					cap_mblock[id].attach = dma_buf_attach(
					cap_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = cap_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					cap_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					cap_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = cap_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(cap_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					cap_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					cap_mblock[id].start_dma = cap_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(cap_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(cap_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("sg_dma_address fail\n");
						return ret;
					}
					cap_mblock[id].start_virt = (void *)map.vaddr;
					cap_mblock[id].map = map;
					get_dma_buf(cap_mblock[id].d_buf);
					cap_mblock[id].fd =
					dma_buf_fd(cap_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					if (hcp_dbg_enable()) {
						pr_debug("%s:[HCP_WORKING_DEFAULT][%s] phys:0x%llx,"
							" virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, cap_mblock[id].name,
							isp8_get_reserve_mem_phys(id, cap_mode),
							isp8_get_reserve_mem_virt(id, cap_mode),
							isp8_get_reserve_mem_dma(id, cap_mode),
							isp8_get_reserve_mem_size(id, cap_mode),
							cap_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, cap_mode),
							cap_mblock[id].d_buf);
					}
					break;
			}
		} else {
			cap_mblock[id].start_virt =
				kzalloc(cap_mblock[id].size, GFP_KERNEL);
			cap_mblock[id].start_phys =
				virt_to_phys(cap_mblock[id].start_virt);
			cap_mblock[id].start_dma = 0;
		}
		if (hcp_dbg_enable()) {
			pr_debug(
				"%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p,"
				" dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
				__func__, id,
				isp8_get_reserve_mem_phys(id, cap_mode),
				isp8_get_reserve_mem_virt(id, cap_mode),
				isp8_get_reserve_mem_dma(id, cap_mode),
				isp8_get_reserve_mem_size(id, cap_mode),
				cap_mblock[id].is_dma_buf,
				isp8_get_reserve_mem_fd(id, cap_mode),
				cap_mblock[id].d_buf);
		}
	}
	return 0;
}

static int isp8_module_driver_allocate_working_buffer_smvr(struct mtk_hcp *hcp_dev,
    unsigned int smvr_mode, struct mtk_hcp_smvr_reserve_mblock *smvr_mblock)
{
	int id = 0;
	struct sg_table *sgt = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct dma_heap *pdma_heap = NULL;
	struct iosys_map map = {0};
	int ret = 0;
	unsigned int block_num;
	block_num = hcp_dev->data->block_num;
	for (id = 0; id < block_num; id++) {
		if (smvr_mblock[id].is_dma_buf) {
			switch (id) {
			case IMG_MEM_G_ID:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					smvr_mblock[id].d_buf = dma_heap_buffer_alloc(
					pdma_heap,
					smvr_mblock[id].size,
					O_RDWR | O_CLOEXEC,
					DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(smvr_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
							PTR_ERR(smvr_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(smvr_mblock[id].d_buf, smvr_mblock[id].name);
					smvr_mblock[id].attach =
						dma_buf_attach(smvr_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = smvr_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					smvr_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					smvr_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = smvr_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(smvr_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					smvr_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					smvr_mblock[id].start_dma = smvr_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(smvr_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(smvr_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("sg_dma_address fail\n");
						return ret;
					}
					smvr_mblock[id].start_virt = (void *)map.vaddr;
					smvr_mblock[id].map = map;
					get_dma_buf(smvr_mblock[id].d_buf);
					smvr_mblock[id].fd =
						dma_buf_fd(smvr_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					dma_buf_begin_cpu_access(smvr_mblock[id].d_buf, DMA_BIDIRECTIONAL);
					kref_init(&smvr_mblock[id].kref);
					if (hcp_dbg_enable()) {
						pr_debug("%s:[HCP_GCE][%s] phys:0x%llx, virt:0x%p, dma:0x%llx,"
							" size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, smvr_mblock[id].name,
							isp8_get_reserve_mem_phys(id, smvr_mode),
							isp8_get_reserve_mem_virt(id, smvr_mode),
							isp8_get_reserve_mem_dma(id, smvr_mode),
							isp8_get_reserve_mem_size(id, smvr_mode),
							smvr_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, smvr_mode),
							smvr_mblock[id].d_buf);
					}
					break;
			case WPE_MEM_C_ID:
			case WPE_MEM_T_ID:
			case OMC_MEM_C_ID:
			case OMC_MEM_T_ID:
			case DIP_MEM_C_ID:
			case DIP_MEM_T_ID:
			case TRAW_MEM_C_ID:
			case TRAW_MEM_T_ID:
			case PQDIP_MEM_C_ID:
			case PQDIP_MEM_T_ID:
			case ADL_MEM_C_ID:
			case ADL_MEM_T_ID:
			case ME_MEM_C_ID:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm-uncached");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					smvr_mblock[id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						smvr_mblock[id].size, O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(smvr_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
						PTR_ERR(smvr_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(smvr_mblock[id].d_buf, smvr_mblock[id].name);
					smvr_mblock[id].attach = dma_buf_attach(
					smvr_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = smvr_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}

					#ifdef HCP_NEW_DMA_BUF_API
					smvr_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
					smvr_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = smvr_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(smvr_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					smvr_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					smvr_mblock[id].start_dma = smvr_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(smvr_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(smvr_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("sg_dma_address fail\n");
						return ret;
					}
					smvr_mblock[id].start_virt = (void *)map.vaddr;
					smvr_mblock[id].map = map;
					get_dma_buf(smvr_mblock[id].d_buf);
					smvr_mblock[id].fd =
					dma_buf_fd(smvr_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					if (hcp_dbg_enable()) {
						pr_debug("%s:[HCP_WORKING][%s] phys:0x%llx, virt:0x%p,"
							" dma:0x%llx,size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, smvr_mblock[id].name,
							isp8_get_reserve_mem_phys(id, smvr_mode),
							isp8_get_reserve_mem_virt(id, smvr_mode),
							isp8_get_reserve_mem_dma(id, smvr_mode),
							isp8_get_reserve_mem_size(id, smvr_mode),
							smvr_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, smvr_mode),
							smvr_mblock[id].d_buf);
					}
					break;
			default:
					/* all supported heap name you can find with cmd */
					/* (ls /dev/dma_heap/) in shell */
					pdma_heap = dma_heap_find("mtk_mm-uncached");
					if (!pdma_heap) {
						pr_info("pdma_heap find fail\n");
						return -1;
					}
					smvr_mblock[id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						smvr_mblock[id].size, O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(smvr_mblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
						PTR_ERR(smvr_mblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(smvr_mblock[id].d_buf, smvr_mblock[id].name);
					smvr_mblock[id].attach = dma_buf_attach(
					smvr_mblock[id].d_buf, hcp_dev->smmu_dev);
					attach = smvr_mblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}
					#ifdef HCP_NEW_DMA_BUF_API
					smvr_mblock[id].sgt =
						dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
					#else
						smvr_mblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					#endif
					sgt = smvr_mblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(smvr_mblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					smvr_mblock[id].start_phys = sg_dma_address(sgt->sgl);
					smvr_mblock[id].start_dma = smvr_mblock[id].start_phys;
					#ifdef HCP_NEW_DMA_BUF_API
					ret = dma_buf_vmap_unlocked(smvr_mblock[id].d_buf, &map);
					#else
					ret = dma_buf_vmap(smvr_mblock[id].d_buf, &map);
					#endif
					if (ret) {
						pr_info("vma_dma_address fail\n");
						return ret;
					}
					smvr_mblock[id].start_virt = (void *)map.vaddr;
					smvr_mblock[id].map = map;
					get_dma_buf(smvr_mblock[id].d_buf);
					smvr_mblock[id].fd =
					dma_buf_fd(smvr_mblock[id].d_buf, O_RDWR | O_CLOEXEC);
					if (hcp_dbg_enable()) {
						pr_debug("%s:[HCP_WORKING_DEFAULT][%s] phys:0x%llx,"
							"virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
							__func__, smvr_mblock[id].name,
							isp8_get_reserve_mem_phys(id, smvr_mode),
							isp8_get_reserve_mem_virt(id, smvr_mode),
							isp8_get_reserve_mem_dma(id, smvr_mode),
							isp8_get_reserve_mem_size(id, smvr_mode),
							smvr_mblock[id].is_dma_buf,
							isp8_get_reserve_mem_fd(id, smvr_mode),
							smvr_mblock[id].d_buf);
					}
					break;
			}
		} else {
			smvr_mblock[id].start_virt =
				kzalloc(smvr_mblock[id].size, GFP_KERNEL);
			smvr_mblock[id].start_phys =
				virt_to_phys(smvr_mblock[id].start_virt);
			smvr_mblock[id].start_dma = 0;
		}
		if (hcp_dbg_enable()) {
			pr_debug(
				"%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p,"
				" dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
				__func__, id,
				isp8_get_reserve_mem_phys(id, smvr_mode),
				isp8_get_reserve_mem_virt(id, smvr_mode),
				isp8_get_reserve_mem_dma(id, smvr_mode),
				isp8_get_reserve_mem_size(id, smvr_mode),
				smvr_mblock[id].is_dma_buf,
				isp8_get_reserve_mem_fd(id, smvr_mode),
				smvr_mblock[id].d_buf);
		}
	}
	return 0;
}

int isp8_allocate_gce_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
{
		unsigned int block_num_gce = 0;
		unsigned int g_id = 0;
		struct mtk_hcp_gce_token_reserve_mblock *gmblock = NULL;
		struct sg_table *sgt = NULL;
		struct dma_buf_attachment *attach = NULL;
		struct dma_heap *pdma_heap = NULL;
		struct iosys_map map = {0};
		int ret = 0;

		gmblock = hcp_dev->data->gmblock;
		gmb = gmblock;
		block_num_gce = hcp_dev->data->block_num_gce;
		for (g_id = 0; g_id < block_num_gce; g_id++) {
			if (gmblock[g_id].is_dma_buf) {
				switch (g_id + IMG_MEM_FOR_HW_ID) {
				case IMG_MEM_FOR_HW_ID:
						/*allocated at probe via dts*/
							break;
				case IMG_MEM_G_TOKEN_ID:
						/* all supported heap name you can find with cmd */
						/* (ls /dev/dma_heap/) in shell */
						pdma_heap = dma_heap_find("mtk_mm");
						if (!pdma_heap) {
							pr_info("pdma_heap find fail\n");
							return -1;
						}
						gmblock[g_id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						gmblock[g_id].size,
						O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
						if (IS_ERR(gmblock[g_id].d_buf)) {
							pr_info("dma_heap_buffer_alloc fail :%ld\n",
								PTR_ERR(gmblock[g_id].d_buf));
							return -1;
						}
						mtk_dma_buf_set_name(gmblock[g_id].d_buf, gmblock[g_id].name);
						gmblock[g_id].attach =
							dma_buf_attach(gmblock[g_id].d_buf, hcp_dev->smmu_dev);
						attach = gmblock[g_id].attach;
						if (IS_ERR(attach)) {
							pr_info("dma_buf_attach fail :%ld\n",
							PTR_ERR(attach));
							return -1;
						}
						#ifdef HCP_NEW_DMA_BUF_API
						gmblock[g_id].sgt =
							dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
						#else
						gmblock[g_id].sgt =
							dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
						#endif
						sgt = gmblock[g_id].sgt;
						if (IS_ERR(sgt)) {
							dma_buf_detach(gmblock[g_id].d_buf, attach);
							pr_info("dma_buf_map_attachment fail sgt:%ld\n",
							PTR_ERR(sgt));
							return -1;
						}
						gmblock[g_id].start_phys = sg_dma_address(sgt->sgl);
						gmblock[g_id].start_dma = gmblock[g_id].start_phys;
						#ifdef HCP_NEW_DMA_BUF_API
						ret = dma_buf_vmap_unlocked(gmblock[g_id].d_buf, &map);
						#else
						ret = dma_buf_vmap(gmblock[g_id].d_buf, &map);
						#endif
						if (ret) {
							pr_info("sg_dma_address fail\n");
							return ret;
						}
						gmblock[g_id].start_virt = (void *)map.vaddr;
						gmblock[g_id].map = map;
						get_dma_buf(gmblock[g_id].d_buf);
						gmblock[g_id].fd =
							dma_buf_fd(gmblock[g_id].d_buf, O_RDWR | O_CLOEXEC);
						dma_buf_begin_cpu_access(gmblock[g_id].d_buf, DMA_BIDIRECTIONAL);
						kref_init(&gmblock[g_id].kref);
						if (hcp_dbg_enable()) {
							pr_debug("%s:[HCP_GCE_TOKEN][%s] phys:0x%llx, virt:0x%p, dma:0x%llx,"
								" size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
								__func__, gmblock[g_id].name,
								isp8_get_reserve_mem_phys(g_id + IMG_MEM_FOR_HW_ID, mode),
								isp8_get_reserve_mem_virt(g_id + IMG_MEM_FOR_HW_ID, mode),
								isp8_get_reserve_mem_dma(g_id + IMG_MEM_FOR_HW_ID, mode),
								isp8_get_reserve_mem_size(g_id + IMG_MEM_FOR_HW_ID, mode),
								gmblock[g_id].is_dma_buf,
								isp8_get_reserve_mem_fd(g_id + IMG_MEM_FOR_HW_ID, mode),
								gmblock[g_id].d_buf);
						}
						break;
				default:
						break;
				}
			} else {
				gmblock[g_id].start_virt =
					kzalloc(gmblock[g_id].size,GFP_KERNEL);
				gmblock[g_id].start_phys =
					virt_to_phys(gmblock[g_id].start_virt);
				gmblock[g_id].start_dma = 0;
			}
			if (hcp_dbg_enable()) {
				pr_debug(
					"%s: [HCP_GCE_TOKEN][gce_mem_reserve-%d] phys:0x%llx, virt:0x%p,"
					" dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
					__func__, g_id,
					isp8_get_reserve_mem_phys(g_id + IMG_MEM_FOR_HW_ID, mode),
					isp8_get_reserve_mem_virt(g_id + IMG_MEM_FOR_HW_ID, mode),
					isp8_get_reserve_mem_dma(g_id + IMG_MEM_FOR_HW_ID, mode),
					isp8_get_reserve_mem_size(g_id + IMG_MEM_FOR_HW_ID, mode),
					gmblock[g_id].is_dma_buf,
					isp8_get_reserve_mem_fd(g_id + IMG_MEM_FOR_HW_ID, mode),
					gmblock[g_id].d_buf);
			}
		}
		return 0;
}

int isp8_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode, unsigned int gmb_en)
{
		struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
		struct mtk_hcp_smvr_reserve_mblock *smblock = NULL;
		struct mtk_hcp_capture_reserve_mblock *cmblock = NULL;
		int ret = 0;

		if (mode == imgsys_streaming) {
			mblock = hcp_dev->data->mblock;
			mb = mblock;
		} else if (mode == imgsys_capture) {
			cmblock = hcp_dev->data->cmblock;
			cmb = cmblock;
		} else {
			smblock = hcp_dev->data->smblock;
			smb = smblock;
		}

		if (gmb_en) {
			pr_info("mtk_hcp: allocate gce buffer\n");
			isp8_allocate_gce_working_buffer(hcp_dev, mode);
		}

		if (mode == imgsys_streaming) {
			pr_info("mtk_hcp: allocate streaming buffer\n");
			ret = isp8_module_driver_allocate_working_buffer_streaming(hcp_dev, mode, mblock);
		} else if (mode == imgsys_capture) {
			pr_info("mtk_hcp: allocate capture buffer\n");
			ret = isp8_module_driver_allocate_working_buffer_capture(hcp_dev, mode, cmblock);
		} else {
			pr_info("mtk_hcp: allocate smvr buffer\n");
			ret = isp8_module_driver_allocate_working_buffer_smvr(hcp_dev, mode, smblock);
		}

		return ret;
}

EXPORT_SYMBOL(isp8_allocate_working_buffer);

static void gce_release_streaming(struct kref *ref)
{
	struct mtk_hcp_streaming_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_streaming_reserve_mblock, kref);

	if (IS_ERR(mblock->d_buf)) {
		pr_info("streaming gce dma_heap_buffer_alloc fail :%ld\n",
			PTR_ERR(mblock->d_buf));
		mblock->mem_priv = NULL;
		mblock->mmap_cnt = 0;
		mblock->start_dma = 0x0;
		mblock->start_virt = 0x0;
		mblock->start_phys = 0x0;
		mblock->d_buf = NULL;
		mblock->fd = -1;
		mblock->pIonHandle = NULL;
		mblock->attach = NULL;
		mblock->sgt = NULL;
		return;
	} else {
		#ifdef HCP_NEW_DMA_BUF_API
		dma_buf_vunmap_unlocked(mblock->d_buf, &mblock->map);
		/* free iova */
		dma_buf_unmap_attachment_unlocked(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
		#else
		dma_buf_vunmap(mblock->d_buf, &mblock->map);
		/* free iova */
		dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
		#endif
		dma_buf_detach(mblock->d_buf, mblock->attach);
		dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(mblock->d_buf);
	}

	if (hcp_dbg_enable()) {
		pr_debug("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx,"
			" is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
			__func__, mblock->name,
			#if SMVR_DECOUPLE
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_streaming),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_streaming),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming),
			#else
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID),
			#endif
			mblock->d_buf);
	}

	mblock->mem_priv = NULL;
	mblock->mmap_cnt = 0;
	mblock->start_dma = 0x0;
	mblock->start_virt = 0x0;
	mblock->start_phys = 0x0;
	mblock->d_buf = NULL;
	mblock->fd = -1;
	mblock->pIonHandle = NULL;
	mblock->attach = NULL;
	mblock->sgt = NULL;
}

static void gce_release_capture(struct kref *ref)
{
	struct mtk_hcp_capture_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_capture_reserve_mblock, kref);

	if (IS_ERR(mblock->d_buf)) {
		pr_info("capture gce dma_heap_buffer_alloc fail :%ld\n",
			PTR_ERR(mblock->d_buf));
		mblock->mem_priv = NULL;
		mblock->mmap_cnt = 0;
		mblock->start_dma = 0x0;
		mblock->start_virt = 0x0;
		mblock->start_phys = 0x0;
		mblock->d_buf = NULL;
		mblock->fd = -1;
		mblock->pIonHandle = NULL;
		mblock->attach = NULL;
		mblock->sgt = NULL;
	} else {
		#ifdef HCP_NEW_DMA_BUF_API
		dma_buf_vunmap_unlocked(mblock->d_buf, &mblock->map);
		/* free iova */
		dma_buf_unmap_attachment_unlocked(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
		#else
		dma_buf_vunmap(mblock->d_buf, &mblock->map);
		/* free iova */
		dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
		#endif
		dma_buf_detach(mblock->d_buf, mblock->attach);
		dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(mblock->d_buf);
	}

	if (hcp_dbg_enable()) {
		pr_debug("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx,"
			" is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
			__func__, mblock->name,
			#if SMVR_DECOUPLE
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_capture),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_capture),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_capture),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_capture),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_capture),
			#else
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID),
			#endif
			mblock->d_buf);
	}
	// close fd in user space driver, you can't close fd in kernel site
	// dma_heap_buffer_free(mblock[id].d_buf);
	//dma_buf_put(my_dma_buf);
	//also can use this api, but not recommended
	mblock->mem_priv = NULL;
	mblock->mmap_cnt = 0;
	mblock->start_dma = 0x0;
	mblock->start_virt = 0x0;
	mblock->start_phys = 0x0;
	mblock->d_buf = NULL;
	mblock->fd = -1;
	mblock->pIonHandle = NULL;
	mblock->attach = NULL;
	mblock->sgt = NULL;
}

static void gce_release_smvr(struct kref *ref)
{
	struct mtk_hcp_smvr_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_smvr_reserve_mblock, kref);

	if (IS_ERR(mblock->d_buf)) {
		pr_info("smvr gce dma_heap_buffer_alloc fail :%ld\n",
				PTR_ERR(mblock->d_buf));
		mblock->mem_priv = NULL;
		mblock->mmap_cnt = 0;
		mblock->start_dma = 0x0;
		mblock->start_virt = 0x0;
		mblock->start_phys = 0x0;
		mblock->d_buf = NULL;
		mblock->fd = -1;
		mblock->pIonHandle = NULL;
		mblock->attach = NULL;
		mblock->sgt = NULL;
		return;
	} else {
		#ifdef HCP_NEW_DMA_BUF_API
		dma_buf_vunmap_unlocked(mblock->d_buf, &mblock->map);
		/* free iova */
		dma_buf_unmap_attachment_unlocked(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
		#else
		dma_buf_vunmap(mblock->d_buf, &mblock->map);
		/* free iova */
		dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
		#endif
		dma_buf_detach(mblock->d_buf, mblock->attach);
		dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(mblock->d_buf);
    }

	if (hcp_dbg_enable()) {
		pr_debug("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx,"
			" is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
			__func__, mblock->name,
			#if SMVR_DECOUPLE
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_smvr),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_smvr),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_smvr),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_smvr),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_smvr),
			#else
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID),
			#endif
			mblock->d_buf);
	}
	// close fd in user space driver, you can't close fd in kernel site
	// dma_heap_buffer_free(mblock[id].d_buf);
	//dma_buf_put(my_dma_buf);
	//also can use this api, but not recommended
	mblock->mem_priv = NULL;
	mblock->mmap_cnt = 0;
	mblock->start_dma = 0x0;
	mblock->start_virt = 0x0;
	mblock->start_phys = 0x0;
	mblock->d_buf = NULL;
	mblock->fd = -1;
	mblock->pIonHandle = NULL;
	mblock->attach = NULL;
	mblock->sgt = NULL;
}
#else
phys_addr_t isp8_get_reserve_mem_phys(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %d", id);
		return 0;
	} else {
		return mb[id].start_phys;
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_phys);

void *isp8_get_reserve_mem_virt(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else
		return mb[id].start_virt;
}
EXPORT_SYMBOL(isp8_get_reserve_mem_virt);

phys_addr_t isp8_get_reserve_mem_dma(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		return mb[id].start_dma;
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_dma);

phys_addr_t isp8_get_reserve_mem_size(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		return mb[id].size;
	}
}
EXPORT_SYMBOL(isp8_get_reserve_mem_size);

uint32_t isp8_get_reserve_mem_fd(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else
		return mb[id].fd;
}
EXPORT_SYMBOL(isp8_get_reserve_mem_fd);

void *isp8_get_gce_virt(void)
{
	return mb[IMG_MEM_G_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_gce_virt);

void *isp8_get_wpe_virt(void)
{
	return mb[WPE_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_wpe_virt);

int isp8_get_wpe_cq_fd(void)
{
	return mb[WPE_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_wpe_cq_fd);

int isp8_get_wpe_tdr_fd(void)
{
	return mb[WPE_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_wpe_tdr_fd);

void *isp8_get_omc_virt(void)
{
	return mb[OMC_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_omc_virt);

int isp8_get_omc_cq_fd(void)
{
	return mb[OMC_MEM_C_ID].fd;
}
EXPORT_SYMBOL(8_get_omc_cq_fd);

int isp8_get_omc_tdr_fd(void)
{
	return mb[OMC_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_omc_tdr_fd);

void *isp8_get_dip_virt(void)
{
	return mb[DIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_dip_virt);

int isp8_get_dip_cq_fd(void)
{
	return mb[DIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_dip_cq_fd);

int isp8_get_dip_tdr_fd(void)
{
	return mb[DIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_dip_tdr_fd);

void *isp8_get_traw_virt(void)
{
	return mb[TRAW_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_traw_virt);

int isp8_get_traw_cq_fd(void)
{
	return mb[TRAW_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_traw_cq_fd);

int isp8_get_traw_tdr_fd(void)
{
	return mb[TRAW_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_traw_tdr_fd);

void *isp8_get_pqdip_virt(void)
{
	return mb[PQDIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_pqdip_virt);

int isp8_get_pqdip_cq_fd(void)
{
	return mb[PQDIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp8_get_pqdip_cq_fd);

int isp8_get_pqdip_tdr_fd(void)
{
	return mb[PQDIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp8_get_pqdip_tdr_fd);

void *isp8_get_hwid_virt(void)
{
	return mb[DIP_MEM_FOR_HW_ID].start_virt;
}
EXPORT_SYMBOL(isp8_get_hwid_virt);

int isp8_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
{
	enum isp8_rsv_mem_id_t id = 0;
	struct mtk_hcp_reserve_mblock *mblock = NULL;
	unsigned int block_num = 0;
	struct sg_table *sgt = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct dma_heap *pdma_heap = NULL;
	struct iosys_map map = {0};
	int ret = 0;

	if (mode)
		mblock = hcp_dev->data->smblock;
	else
		mblock = hcp_dev->data->mblock;

	mb = mblock;
	block_num = hcp_dev->data->block_num;
	for (id = 0; id < block_num; id++) {
		if (mblock[id].is_dma_buf) {
			switch (id) {
			case IMG_MEM_FOR_HW_ID:
				/*allocated at probe via dts*/
				break;
			case IMG_MEM_G_ID:
				/* all supported heap name you can find with cmd */
				/* (ls /dev/dma_heap/) in shell */
				pdma_heap = dma_heap_find("mtk_mm");
				if (!pdma_heap) {
					pr_info("pdma_heap find fail\n");
					return -1;
				}
				mblock[id].d_buf = dma_heap_buffer_alloc(
					pdma_heap,
					mblock[id].size,
					O_RDWR | O_CLOEXEC,
					DMA_HEAP_VALID_HEAP_FLAGS);
				if (IS_ERR(mblock[id].d_buf)) {
					pr_info("dma_heap_buffer_alloc fail :%ld\n",
					PTR_ERR(mblock[id].d_buf));
					return -1;
				}
				mtk_dma_buf_set_name(mblock[id].d_buf, mblock[id].name);
				mblock[id].attach =
					dma_buf_attach(mblock[id].d_buf, hcp_dev->smmu_dev);
				attach = mblock[id].attach;
				if (IS_ERR(attach)) {
					pr_info("dma_buf_attach fail :%ld\n",
					PTR_ERR(attach));
					return -1;
				}

				#ifdef HCP_NEW_DMA_BUF_API
				mblock[id].sgt =
					dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
				#else
				mblock[id].sgt =
					dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
				#endif
				sgt = mblock[id].sgt;
				if (IS_ERR(sgt)) {
					dma_buf_detach(mblock[id].d_buf, attach);
					pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					PTR_ERR(sgt));
					return -1;
				}
				mblock[id].start_phys = sg_dma_address(sgt->sgl);
				mblock[id].start_dma = mblock[id].start_phys;
				#ifdef HCP_NEW_DMA_BUF_API
				ret = dma_buf_vmap_unlocked(mblock[id].d_buf, &map);
				#else
				ret = dma_buf_vmap(mblock[id].d_buf, &map);
				#endif
				if (ret) {
					pr_info("sg_dma_address fail\n");
					return ret;
				}
				mblock[id].start_virt = (void *)map.vaddr;
				mblock[id].map = map;
				get_dma_buf(mblock[id].d_buf);
				mblock[id].fd =
					dma_buf_fd(mblock[id].d_buf, O_RDWR | O_CLOEXEC);
				dma_buf_begin_cpu_access(mblock[id].d_buf, DMA_BIDIRECTIONAL);
				kref_init(&mblock[id].kref);
				if (hcp_dbg_enable()) {
					pr_debug("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx,"
					" size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
					__func__, mblock[id].name,
					isp8_get_reserve_mem_phys(id),
					isp8_get_reserve_mem_virt(id),
					isp8_get_reserve_mem_dma(id),
					isp8_get_reserve_mem_size(id),
					mblock[id].is_dma_buf,
					isp8_get_reserve_mem_fd(id),
					mblock[id].d_buf);
				}
				break;
			case WPE_MEM_C_ID:
			case WPE_MEM_T_ID:
			case OMC_MEM_C_ID:
			case OMC_MEM_T_ID:
			case DIP_MEM_C_ID:
			case DIP_MEM_T_ID:
			case TRAW_MEM_C_ID:
			case TRAW_MEM_T_ID:
			case PQDIP_MEM_C_ID:
			case PQDIP_MEM_T_ID:
			case ME_MEM_C_ID:
				/* all supported heap name you can find with cmd */
				/* (ls /dev/dma_heap/) in shell */
				pdma_heap = dma_heap_find("mtk_mm-uncached");
				if (!pdma_heap) {
					pr_info("pdma_heap find fail\n");
					return -1;
				}
				mblock[id].d_buf = dma_heap_buffer_alloc(
					pdma_heap,
					mblock[id].size, O_RDWR | O_CLOEXEC,
					DMA_HEAP_VALID_HEAP_FLAGS);
				if (IS_ERR(mblock[id].d_buf)) {
					pr_info("dma_heap_buffer_alloc fail :%ld\n",
					PTR_ERR(mblock[id].d_buf));
					return -1;
				}
				mtk_dma_buf_set_name(mblock[id].d_buf, mblock[id].name);
				mblock[id].attach = dma_buf_attach(
				mblock[id].d_buf, hcp_dev->smmu_dev);
				attach = mblock[id].attach;
				if (IS_ERR(attach)) {
					pr_info("dma_buf_attach fail :%ld\n",
					PTR_ERR(attach));
					return -1;
				}

				#ifdef HCP_NEW_DMA_BUF_API
				mblock[id].sgt =
					dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
				#else
				mblock[id].sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
				#endif
				sgt = mblock[id].sgt;
				if (IS_ERR(sgt)) {
					dma_buf_detach(mblock[id].d_buf, attach);
					pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					PTR_ERR(sgt));
					return -1;
				}
				mblock[id].start_phys = sg_dma_address(sgt->sgl);
				mblock[id].start_dma = mblock[id].start_phys;
				#ifdef HCP_NEW_DMA_BUF_API
				ret = dma_buf_vmap_unlocked(mblock[id].d_buf, &map);
				#else
				ret = dma_buf_vmap(mblock[id].d_buf, &map);
				#endif
				if (ret) {
					pr_info("sg_dma_address fail\n");
					return ret;
				}
				mblock[id].start_virt = (void *)map.vaddr;
				mblock[id].map = map;
				get_dma_buf(mblock[id].d_buf);
				mblock[id].fd =
					dma_buf_fd(mblock[id].d_buf, O_RDWR | O_CLOEXEC);
				break;
			default:

				/* all supported heap name you can find with cmd */
				/* (ls /dev/dma_heap/) in shell */
				pdma_heap = dma_heap_find("mtk_mm-uncached");
				if (!pdma_heap) {
					pr_info("pdma_heap find fail\n");
					return -1;
				}
				mblock[id].d_buf = dma_heap_buffer_alloc(
					pdma_heap,
					mblock[id].size, O_RDWR | O_CLOEXEC,
					DMA_HEAP_VALID_HEAP_FLAGS);
				if (IS_ERR(mblock[id].d_buf)) {
					pr_info("dma_heap_buffer_alloc fail :%ld\n",
					PTR_ERR(mblock[id].d_buf));
					return -1;
				}
				mtk_dma_buf_set_name(mblock[id].d_buf, mblock[id].name);
				mblock[id].attach = dma_buf_attach(
				mblock[id].d_buf, hcp_dev->smmu_dev);
				attach = mblock[id].attach;
				if (IS_ERR(attach)) {
					pr_info("dma_buf_attach fail :%ld\n",
					PTR_ERR(attach));
					return -1;
				}

				#ifdef HCP_NEW_DMA_BUF_API
				mblock[id].sgt =
					dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
				#else
				mblock[id].sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
				#endif
				sgt = mblock[id].sgt;
				if (IS_ERR(sgt)) {
					dma_buf_detach(mblock[id].d_buf, attach);
					pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					PTR_ERR(sgt));
					return -1;
				}
				mblock[id].start_phys = sg_dma_address(sgt->sgl);
				mblock[id].start_dma = mblock[id].start_phys;
				#ifdef HCP_NEW_DMA_BUF_API
				ret = dma_buf_vmap_unlocked(mblock[id].d_buf, &map);
				#else
				ret = dma_buf_vmap(mblock[id].d_buf, &map);
				#endif
				if (ret) {
					pr_info("sg_dma_address fail\n");
					return ret;
				}
				mblock[id].start_virt = (void *)map.vaddr;
				mblock[id].map = map;
				get_dma_buf(mblock[id].d_buf);
				mblock[id].fd =
					dma_buf_fd(mblock[id].d_buf, O_RDWR | O_CLOEXEC);
				//dma_buf_begin_cpu_access(mblock[id].d_buf, DMA_BIDIRECTIONAL);
				break;
			}
		} else {
			mblock[id].start_virt =
				kzalloc(mblock[id].size,
					GFP_KERNEL);
			mblock[id].start_phys =
				virt_to_phys(
					mblock[id].start_virt);
			mblock[id].start_dma = 0;
		}
		if (hcp_dbg_enable()) {
			pr_debug(
				"%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx,"
				" size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
				__func__, id,
				isp8_get_reserve_mem_phys(id),
				isp8_get_reserve_mem_virt(id),
				isp8_get_reserve_mem_dma(id),
				isp8_get_reserve_mem_size(id),
				mblock[id].is_dma_buf,
				isp8_get_reserve_mem_fd(id),
				mblock[id].d_buf);
		}
	}

	return 0;
}
EXPORT_SYMBOL(isp8_allocate_working_buffer);

static void gce_release(struct kref *ref)
{
	struct mtk_hcp_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_reserve_mblock, kref);

	#ifdef HCP_NEW_DMA_BUF_API
	dma_buf_vunmap_unlocked(mblock->d_buf, &mblock->map);
	/* free iova */
	dma_buf_unmap_attachment_unlocked(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
	#else
	dma_buf_vunmap(mblock->d_buf, &mblock->map);
	/* free iova */
	dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
	#endif
	dma_buf_detach(mblock->d_buf, mblock->attach);
	dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
	dma_buf_put(mblock->d_buf);
	if (hcp_dbg_enable()) {
		pr_debug("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx,"
			" is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
			__func__, mblock->name,
			#if SMVR_DECOUPLE
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_streaming),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_streaming),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming),
			#else
			isp8_get_reserve_mem_phys(IMG_MEM_G_ID),
			isp8_get_reserve_mem_virt(IMG_MEM_G_ID),
			isp8_get_reserve_mem_dma(IMG_MEM_G_ID),
			isp8_get_reserve_mem_size(IMG_MEM_G_ID),
			mblock->is_dma_buf,
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID),
			#endif
			mblock->d_buf);
	}
	// close fd in user space driver, you can't close fd in kernel site
	// dma_heap_buffer_free(mblock[id].d_buf);
	//dma_buf_put(my_dma_buf);
	//also can use this api, but not recommended
	mblock->mem_priv = NULL;
	mblock->mmap_cnt = 0;
	mblock->start_dma = 0x0;
	mblock->start_virt = 0x0;
	mblock->start_phys = 0x0;
	mblock->d_buf = NULL;
	mblock->fd = -1;
	mblock->pIonHandle = NULL;
	mblock->attach = NULL;
	mblock->sgt = NULL;
}
#endif
#if SMVR_DECOUPLE
static int isp8_module_driver_release_working_buffer_streaming(struct mtk_hcp *hcp_dev,
    unsigned int str_mode, struct mtk_hcp_streaming_reserve_mblock *str_mblock)
{
	enum isp8_rsv_mem_id_t id;
	unsigned int block_num;
	block_num = hcp_dev->data->block_num;

	/* release reserved memory */
	for (id = 0; id < block_num; id++) {
		if (!str_mblock[id].is_dma_buf) {
			kfree(str_mblock[id].start_virt);
			str_mblock[id].start_virt = 0x0;
			str_mblock[id].start_phys = 0x0;
			str_mblock[id].start_dma = 0x0;
			str_mblock[id].mmap_cnt = 0;
			return 0;
		}

		switch (id) {
		case IMG_MEM_G_ID:
				kref_put(&str_mblock[id].kref, gce_release_streaming);
				break;
		case WPE_MEM_C_ID:
		case WPE_MEM_T_ID:
		case OMC_MEM_C_ID:
		case OMC_MEM_T_ID:
		case DIP_MEM_C_ID:
		case DIP_MEM_T_ID:
		case TRAW_MEM_C_ID:
		case TRAW_MEM_T_ID:
		case PQDIP_MEM_C_ID:
		case PQDIP_MEM_T_ID:
		case ME_MEM_C_ID:
		default:
				if (IS_ERR(str_mblock[id].d_buf) || IS_ERR(str_mblock[id].sgt)) {
					pr_info("streaming dma_heap_buffer_alloc already fail :%ld\n",
						PTR_ERR(str_mblock[id].d_buf));
					str_mblock[id].mem_priv = NULL;
					str_mblock[id].mmap_cnt = 0;
					str_mblock[id].start_dma = 0x0;
					str_mblock[id].start_virt = 0x0;
					str_mblock[id].start_phys = 0x0;
					str_mblock[id].d_buf = NULL;
					str_mblock[id].fd = -1;
					str_mblock[id].pIonHandle = NULL;
					str_mblock[id].attach = NULL;
					str_mblock[id].sgt = NULL;
					return -1;
				}
				if (str_mblock[id].fd != -1) {
					#ifdef HCP_NEW_DMA_BUF_API
					/* free va */
					dma_buf_vunmap_unlocked(str_mblock[id].d_buf, &str_mblock[id].map);
					/* free iova */
					dma_buf_unmap_attachment_unlocked(str_mblock[id].attach,
					str_mblock[id].sgt, DMA_BIDIRECTIONAL);
					#else
					/* free va */
					dma_buf_vunmap(str_mblock[id].d_buf, &str_mblock[id].map);
					/* free iova */
					dma_buf_unmap_attachment(str_mblock[id].attach,
					str_mblock[id].sgt, DMA_BIDIRECTIONAL);
					#endif
					dma_buf_detach(str_mblock[id].d_buf,
					str_mblock[id].attach);
					dma_buf_put(str_mblock[id].d_buf);
				}
				// close fd in user space driver, you can't close fd in kernel site
				// dma_heap_buffer_free(mblock[id].d_buf);
				//dma_buf_put(my_dma_buf);
				//also can use this api, but not recommended
				str_mblock[id].mem_priv = NULL;
				str_mblock[id].mmap_cnt = 0;
				str_mblock[id].start_dma = 0x0;
				str_mblock[id].start_virt = 0x0;
				str_mblock[id].start_phys = 0x0;
				str_mblock[id].d_buf = NULL;
				str_mblock[id].fd = -1;
				str_mblock[id].pIonHandle = NULL;
				str_mblock[id].attach = NULL;
				str_mblock[id].sgt = NULL;
				break;
		}

		if (hcp_dbg_enable()) {
			pr_debug(
				"%s: [HCP][mem_reserve-%s(%d)] phys:0x%llx, virt:0x%p,"
				" dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d\n",
				__func__, str_mblock[id].name, id,
				isp8_get_reserve_mem_phys(id, str_mode),
				isp8_get_reserve_mem_virt(id, str_mode),
				isp8_get_reserve_mem_dma(id, str_mode),
				isp8_get_reserve_mem_size(id, str_mode),
				str_mblock[id].is_dma_buf,
				isp8_get_reserve_mem_fd(id, str_mode));
		}
	}

	return 0;
}

static int isp8_module_driver_release_working_buffer_capture(struct mtk_hcp *hcp_dev,
    unsigned int cap_mode, struct mtk_hcp_capture_reserve_mblock *cap_mblock)
{
	enum isp8_rsv_mem_id_t id;
	unsigned int block_num;
	block_num = hcp_dev->data->block_num;

	/* release reserved memory */
	for (id = 0; id < block_num; id++) {
		if (cap_mblock[id].is_dma_buf) {
			switch (id) {
			case IMG_MEM_G_ID:
					kref_put(&cap_mblock[id].kref, gce_release_capture);
					break;
			case WPE_MEM_C_ID:
			case WPE_MEM_T_ID:
			case OMC_MEM_C_ID:
			case OMC_MEM_T_ID:
			case DIP_MEM_C_ID:
			case DIP_MEM_T_ID:
			case TRAW_MEM_C_ID:
			case TRAW_MEM_T_ID:
			case PQDIP_MEM_C_ID:
			case PQDIP_MEM_T_ID:
			case ME_MEM_C_ID:
			default:
					if (IS_ERR(cap_mblock[id].d_buf) || IS_ERR(cap_mblock[id].sgt)) {
						pr_info("capture dma_heap_buffer_alloc already fail :%ld\n",
								PTR_ERR(cap_mblock[id].d_buf));
						cap_mblock[id].mem_priv = NULL;
						cap_mblock[id].mmap_cnt = 0;
						cap_mblock[id].start_dma = 0x0;
						cap_mblock[id].start_virt = 0x0;
						cap_mblock[id].start_phys = 0x0;
						cap_mblock[id].d_buf = NULL;
						cap_mblock[id].fd = -1;
						cap_mblock[id].pIonHandle = NULL;
						cap_mblock[id].attach = NULL;
						cap_mblock[id].sgt = NULL;
						return -1;
					}
					if (cap_mblock[id].fd != -1) {
						#ifdef HCP_NEW_DMA_BUF_API
						/* free va */
						dma_buf_vunmap_unlocked(cap_mblock[id].d_buf, &cap_mblock[id].map);
						/* free iova */
						dma_buf_unmap_attachment_unlocked(cap_mblock[id].attach,
						cap_mblock[id].sgt, DMA_BIDIRECTIONAL);
						#else
						/* free va */
						dma_buf_vunmap(cap_mblock[id].d_buf, &cap_mblock[id].map);
						/* free iova */
						dma_buf_unmap_attachment(cap_mblock[id].attach,
						    cap_mblock[id].sgt, DMA_BIDIRECTIONAL);
						#endif
						dma_buf_detach(cap_mblock[id].d_buf,
							cap_mblock[id].attach);
						dma_buf_put(cap_mblock[id].d_buf);
					}
					// close fd in user space driver, you can't close fd in kernel site
					// dma_heap_buffer_free(mblock[id].d_buf);
					//dma_buf_put(my_dma_buf);
					//also can use this api, but not recommended
					cap_mblock[id].mem_priv = NULL;
					cap_mblock[id].mmap_cnt = 0;
					cap_mblock[id].start_dma = 0x0;
					cap_mblock[id].start_virt = 0x0;
					cap_mblock[id].start_phys = 0x0;
					cap_mblock[id].d_buf = NULL;
					cap_mblock[id].fd = -1;
					cap_mblock[id].pIonHandle = NULL;
					cap_mblock[id].attach = NULL;
					cap_mblock[id].sgt = NULL;
					break;
			}
		} else {
			kfree(cap_mblock[id].start_virt);
			cap_mblock[id].start_virt = 0x0;
			cap_mblock[id].start_phys = 0x0;
			cap_mblock[id].start_dma = 0x0;
			cap_mblock[id].mmap_cnt = 0;
		}
		if (hcp_dbg_enable()) {
			pr_debug(
				"%s: [HCP][mem_reserve-%s(%d)] phys:0x%llx,"
				" virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d\n",
				__func__, cap_mblock[id].name, id,
				isp8_get_reserve_mem_phys(id, cap_mode),
				isp8_get_reserve_mem_virt(id, cap_mode),
				isp8_get_reserve_mem_dma(id, cap_mode),
				isp8_get_reserve_mem_size(id, cap_mode),
				cap_mblock[id].is_dma_buf,
				isp8_get_reserve_mem_fd(id, cap_mode));
		}
	}

	return 0;
}

static int isp8_module_driver_release_working_buffer_smvr(struct mtk_hcp *hcp_dev,
    unsigned int smvr_mode, struct mtk_hcp_smvr_reserve_mblock *smvr_mblock)
{
	enum isp8_rsv_mem_id_t id;
	unsigned int block_num;
	block_num = hcp_dev->data->block_num;

	/* release reserved memory */
	for (id = 0; id < block_num; id++) {
		if (smvr_mblock[id].is_dma_buf) {
			switch (id) {
			case IMG_MEM_G_ID:
				kref_put(&smvr_mblock[id].kref, gce_release_smvr);
				break;
			case WPE_MEM_C_ID:
			case WPE_MEM_T_ID:
			case OMC_MEM_C_ID:
			case OMC_MEM_T_ID:
			case DIP_MEM_C_ID:
			case DIP_MEM_T_ID:
			case TRAW_MEM_C_ID:
			case TRAW_MEM_T_ID:
			case PQDIP_MEM_C_ID:
			case PQDIP_MEM_T_ID:
			case ME_MEM_C_ID:
			default:
					if (IS_ERR(smvr_mblock[id].d_buf) || IS_ERR(smvr_mblock[id].sgt)) {
						pr_info("smvr dma_heap_buffer_alloc already fail :%ld\n",
								PTR_ERR(smvr_mblock[id].d_buf));
						smvr_mblock[id].mem_priv = NULL;
						smvr_mblock[id].mmap_cnt = 0;
						smvr_mblock[id].start_dma = 0x0;
						smvr_mblock[id].start_virt = 0x0;
						smvr_mblock[id].start_phys = 0x0;
						smvr_mblock[id].d_buf = NULL;
						smvr_mblock[id].fd = -1;
						smvr_mblock[id].pIonHandle = NULL;
						smvr_mblock[id].attach = NULL;
						smvr_mblock[id].sgt = NULL;
						return -1;
					}
					if (smvr_mblock[id].fd != -1) {
						#ifdef HCP_NEW_DMA_BUF_API
						/* free va */
						dma_buf_vunmap_unlocked(smvr_mblock[id].d_buf, &smvr_mblock[id].map);
						/* free iova */
						dma_buf_unmap_attachment_unlocked(smvr_mblock[id].attach,
						smvr_mblock[id].sgt, DMA_BIDIRECTIONAL);
						#else
						/* free va */
						dma_buf_vunmap(smvr_mblock[id].d_buf, &smvr_mblock[id].map);
						/* free iova */
						dma_buf_unmap_attachment(smvr_mblock[id].attach,
						smvr_mblock[id].sgt, DMA_BIDIRECTIONAL);
						#endif
						dma_buf_detach(smvr_mblock[id].d_buf,
						smvr_mblock[id].attach);
						dma_buf_put(smvr_mblock[id].d_buf);
					}
					// close fd in user space driver, you can't close fd in kernel site
					// dma_heap_buffer_free(mblock[id].d_buf);
					//dma_buf_put(my_dma_buf);
					//also can use this api, but not recommended
					smvr_mblock[id].mem_priv = NULL;
					smvr_mblock[id].mmap_cnt = 0;
					smvr_mblock[id].start_dma = 0x0;
					smvr_mblock[id].start_virt = 0x0;
					smvr_mblock[id].start_phys = 0x0;
					smvr_mblock[id].d_buf = NULL;
					smvr_mblock[id].fd = -1;
					smvr_mblock[id].pIonHandle = NULL;
					smvr_mblock[id].attach = NULL;
					smvr_mblock[id].sgt = NULL;
					break;
			}
		} else {
			kfree(smvr_mblock[id].start_virt);
			smvr_mblock[id].start_virt = 0x0;
			smvr_mblock[id].start_phys = 0x0;
			smvr_mblock[id].start_dma = 0x0;
			smvr_mblock[id].mmap_cnt = 0;
		}
		if (hcp_dbg_enable()) {
			pr_debug(
				"%s: [HCP][mem_reserve- %s(%d)] phys:0x%llx, virt:0x%p,"
				" dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d\n",
				__func__, smvr_mblock[id].name, id,
				isp8_get_reserve_mem_phys(id, smvr_mode),
				isp8_get_reserve_mem_virt(id, smvr_mode),
				isp8_get_reserve_mem_dma(id, smvr_mode),
				isp8_get_reserve_mem_size(id, smvr_mode),
				smvr_mblock[id].is_dma_buf,
				isp8_get_reserve_mem_fd(id, smvr_mode));
		}
	}

	return 0;
}

int isp8_release_gce_working_buffer(struct mtk_hcp *hcp_dev)
{
	enum isp8_rsv_gce_mem_id_t gid;
	struct mtk_hcp_gce_token_reserve_mblock *gmblock = NULL;
	unsigned int block_num_gce;

	gmblock = hcp_dev->data->gmblock;
	block_num_gce = hcp_dev->data->block_num_gce;
	/* release gce reserved memory */
	for (gid = 0; gid < block_num_gce; gid++) {
		switch (gid + IMG_MEM_FOR_HW_ID) {
		case IMG_MEM_FOR_HW_ID:
				/*allocated at probe via dts*/
				break;
		case IMG_MEM_G_TOKEN_ID:
				if (IS_ERR(gmblock[gid].d_buf) || IS_ERR(gmblock[gid].sgt)) {
					pr_info("smvr dma_heap_buffer_alloc already fail :%ld\n",
						PTR_ERR(gmblock[gid].d_buf));
					gmblock[gid].mem_priv = NULL;
					gmblock[gid].mmap_cnt = 0;
					gmblock[gid].start_dma = 0x0;
					gmblock[gid].start_virt = 0x0;
					gmblock[gid].start_phys = 0x0;
					gmblock[gid].d_buf = NULL;
					gmblock[gid].fd = -1;
					gmblock[gid].pIonHandle = NULL;
					gmblock[gid].attach = NULL;
					gmblock[gid].sgt = NULL;
					return -1;
				}
				if (gmblock[gid].fd != -1) {
					#ifdef HCP_NEW_DMA_BUF_API
					/* free va */
					dma_buf_vunmap_unlocked(gmblock[gid].d_buf, &gmblock[gid].map);
					/* free iova */
					dma_buf_unmap_attachment_unlocked(gmblock[gid].attach,
					gmblock[gid].sgt, DMA_BIDIRECTIONAL);
					#else
					/* free va */
					dma_buf_vunmap(gmblock[gid].d_buf, &gmblock[gid].map);
					/* free iova */
					dma_buf_unmap_attachment(gmblock[gid].attach,
					gmblock[gid].sgt, DMA_BIDIRECTIONAL);
					#endif
					dma_buf_detach(gmblock[gid].d_buf,
						gmblock[gid].attach);
					dma_buf_end_cpu_access(gmblock[gid].d_buf, DMA_BIDIRECTIONAL);
					dma_buf_put(gmblock[gid].d_buf);
				}
				// close fd in user space driver, you can't close fd in kernel site
				// dma_heap_buffer_free(mblock[id].d_buf);
				//dma_buf_put(my_dma_buf);
				//also can use this api, but not recommended
				gmblock[gid].mem_priv = NULL;
				gmblock[gid].mmap_cnt = 0;
				gmblock[gid].start_dma = 0x0;
				gmblock[gid].start_virt = 0x0;
				gmblock[gid].start_phys = 0x0;
				gmblock[gid].d_buf = NULL;
				gmblock[gid].fd = -1;
				gmblock[gid].pIonHandle = NULL;
				gmblock[gid].attach = NULL;
				gmblock[gid].sgt = NULL;
				pr_info("%s:[HCP][%s]\n",
					__func__, gmblock[gid].name);
				break;
		default:
				break;
		}
	}

	return 0;
}
EXPORT_SYMBOL(isp8_release_gce_working_buffer);

int isp8_release_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
{
	struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
	struct mtk_hcp_smvr_reserve_mblock *smblock = NULL;
	struct mtk_hcp_capture_reserve_mblock *cmblock = NULL;
	//struct mtk_hcp_gce_token_reserve_mblock *gmblock = NULL;
	//unsigned int block_num;

	if (mode == imgsys_streaming) {
		mblock = hcp_dev->data->mblock;
		mb = mblock;
	} else if (mode == imgsys_capture) {
		cmblock = hcp_dev->data->cmblock;
		cmb = cmblock;
	} else {
		smblock = hcp_dev->data->smblock;
		smb = smblock;
	}

	if (mode == imgsys_streaming)
		isp8_module_driver_release_working_buffer_streaming(hcp_dev, mode, mblock);
	else if (mode == imgsys_capture)
		isp8_module_driver_release_working_buffer_capture(hcp_dev, mode, cmblock);
	else
		isp8_module_driver_release_working_buffer_smvr(hcp_dev, mode, smblock);

	return 0;
}

EXPORT_SYMBOL(isp8_release_working_buffer);

int isp8_get_init_info(struct img_init_info *info)
{
	if (!info) {
		pr_info("%s:NULL info\n", __func__);
		return -1;
	}
	if (!info->is_capture) {
		if (!info->smvr_mode) {
			/*WPE:0, OMC:1, ADL:2, TRAW:3, DIP:4, PQDIP:5, ME:6  */
			info->module_info_streaming[0].c_wbuf =
						isp8_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[0].c_wbuf_dma =
						isp8_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[0].c_wbuf_sz =
						isp8_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[0].c_wbuf_fd =
						isp8_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[0].t_wbuf =
						isp8_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[0].t_wbuf_dma =
						isp8_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[0].t_wbuf_sz =
						isp8_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[0].t_wbuf_fd =
						isp8_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_streaming);

			//OMC
			info->module_info_streaming[1].c_wbuf =
						isp8_get_reserve_mem_phys(OMC_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[1].c_wbuf_dma =
						isp8_get_reserve_mem_dma(OMC_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[1].c_wbuf_sz =
						isp8_get_reserve_mem_size(OMC_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[1].c_wbuf_fd =
						isp8_get_reserve_mem_fd(OMC_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[1].t_wbuf =
						isp8_get_reserve_mem_phys(OMC_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[1].t_wbuf_dma =
						isp8_get_reserve_mem_dma(OMC_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[1].t_wbuf_sz =
						isp8_get_reserve_mem_size(OMC_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[1].t_wbuf_fd =
						isp8_get_reserve_mem_fd(OMC_MEM_T_ID, imgsys_streaming);

			// ADL
			info->module_info_streaming[2].c_wbuf =
						isp8_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[2].c_wbuf_dma =
						isp8_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[2].c_wbuf_sz =
						isp8_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[2].c_wbuf_fd =
						isp8_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[2].t_wbuf =
						isp8_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[2].t_wbuf_dma =
						isp8_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[2].t_wbuf_sz =
						isp8_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[2].t_wbuf_fd =
						isp8_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_streaming);

			// TRAW
			info->module_info_streaming[3].c_wbuf =
						isp8_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[3].c_wbuf_dma =
						isp8_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[3].c_wbuf_sz =
						isp8_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[3].c_wbuf_fd =
						isp8_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[3].t_wbuf =
						isp8_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[3].t_wbuf_dma =
						isp8_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[3].t_wbuf_sz =
						isp8_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[3].t_wbuf_fd =
						isp8_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_streaming);

			// DIP
			info->module_info_streaming[4].c_wbuf =
						isp8_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[4].c_wbuf_dma =
						isp8_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[4].c_wbuf_sz =
						isp8_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[4].c_wbuf_fd =
						isp8_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[4].t_wbuf =
						isp8_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[4].t_wbuf_dma =
						isp8_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[4].t_wbuf_sz =
						isp8_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[4].t_wbuf_fd =
						isp8_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_streaming);

			// PQDIP
			info->module_info_streaming[5].c_wbuf =
						isp8_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[5].c_wbuf_dma =
						isp8_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[5].c_wbuf_sz =
						isp8_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[5].c_wbuf_fd =
					    isp8_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[5].t_wbuf =
						isp8_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[5].t_wbuf_dma =
						isp8_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[5].t_wbuf_sz =
						isp8_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_streaming);
			info->module_info_streaming[5].t_wbuf_fd =
						isp8_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_streaming);

			// ME
			info->module_info_streaming[6].c_wbuf =
						isp8_get_reserve_mem_phys(ME_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[6].c_wbuf_dma =
						isp8_get_reserve_mem_dma(ME_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[6].c_wbuf_sz =
						isp8_get_reserve_mem_size(ME_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[6].c_wbuf_fd =
					    isp8_get_reserve_mem_fd(ME_MEM_C_ID, imgsys_streaming);
			info->module_info_streaming[6].t_wbuf = 0;
			info->module_info_streaming[6].t_wbuf_dma = 0;
			info->module_info_streaming[6].t_wbuf_sz = 0;
			info->module_info_streaming[6].t_wbuf_fd = 0;

			if (hcp_dbg_enable()) {
				pr_info("mtk_hcp:streaming(fd/buf/sz(%d/%lx/%lx))",
				isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming),
				(unsigned long)isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming),
				(unsigned long)isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming));
			}
			info->gce_info[imgsys_streaming].g_wbuf_fd = isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming);
			info->gce_info[imgsys_streaming].g_wbuf = isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming);
			info->gce_info[imgsys_streaming].g_wbuf_sz = isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming);
		} else {
			/*WPE:0, OMC:1, ADL:2, TRAW:3, DIP:4, PQDIP:5, ME:6  */
			info->module_info_smvr[0].c_wbuf =
						isp8_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[0].c_wbuf_dma =
						isp8_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[0].c_wbuf_sz =
						isp8_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[0].c_wbuf_fd =
						isp8_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[0].t_wbuf =
						isp8_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[0].t_wbuf_dma =
						isp8_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[0].t_wbuf_sz =
						isp8_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[0].t_wbuf_fd =
						isp8_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_smvr);

			//OMC
			info->module_info_smvr[1].c_wbuf =
						isp8_get_reserve_mem_phys(OMC_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[1].c_wbuf_dma =
						isp8_get_reserve_mem_dma(OMC_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[1].c_wbuf_sz =
						isp8_get_reserve_mem_size(OMC_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[1].c_wbuf_fd =
						isp8_get_reserve_mem_fd(OMC_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[1].t_wbuf =
						isp8_get_reserve_mem_phys(OMC_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[1].t_wbuf_dma =
						isp8_get_reserve_mem_dma(OMC_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[1].t_wbuf_sz =
						isp8_get_reserve_mem_size(OMC_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[1].t_wbuf_fd =
						isp8_get_reserve_mem_fd(OMC_MEM_T_ID, imgsys_smvr);

			// ADL
			info->module_info_smvr[2].c_wbuf =
						isp8_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[2].c_wbuf_dma =
						isp8_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[2].c_wbuf_sz =
						isp8_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[2].c_wbuf_fd =
						isp8_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[2].t_wbuf =
						isp8_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[2].t_wbuf_dma =
						isp8_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[2].t_wbuf_sz =
						isp8_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[2].t_wbuf_fd =
						isp8_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_smvr);

			// TRAW
			info->module_info_smvr[3].c_wbuf =
						isp8_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[3].c_wbuf_dma =
						isp8_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[3].c_wbuf_sz =
						isp8_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[3].c_wbuf_fd =
						isp8_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[3].t_wbuf =
						isp8_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[3].t_wbuf_dma =
						isp8_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[3].t_wbuf_sz =
						isp8_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[3].t_wbuf_fd =
						isp8_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_smvr);

			// DIP
			info->module_info_smvr[4].c_wbuf =
						isp8_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[4].c_wbuf_dma =
						isp8_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[4].c_wbuf_sz =
						isp8_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[4].c_wbuf_fd =
						isp8_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[4].t_wbuf =
						isp8_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[4].t_wbuf_dma =
						isp8_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[4].t_wbuf_sz =
						isp8_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[4].t_wbuf_fd =
						isp8_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_smvr);

			// PQDIP
			info->module_info_smvr[5].c_wbuf =
						isp8_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[5].c_wbuf_dma =
						isp8_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[5].c_wbuf_sz =
						isp8_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[5].c_wbuf_fd =
						isp8_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[5].t_wbuf =
						isp8_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[5].t_wbuf_dma =
						isp8_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[5].t_wbuf_sz =
						isp8_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_smvr);
			info->module_info_smvr[5].t_wbuf_fd =
						isp8_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_smvr);

			// ME
			info->module_info_smvr[6].c_wbuf =
						isp8_get_reserve_mem_phys(ME_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[6].c_wbuf_dma =
						isp8_get_reserve_mem_dma(ME_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[6].c_wbuf_sz =
						isp8_get_reserve_mem_size(ME_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[6].c_wbuf_fd =
						isp8_get_reserve_mem_fd(ME_MEM_C_ID, imgsys_smvr);
			info->module_info_smvr[6].t_wbuf = 0;
			info->module_info_smvr[6].t_wbuf_dma = 0;
			info->module_info_smvr[6].t_wbuf_sz = 0;
			info->module_info_smvr[6].t_wbuf_fd = 0;

			if (hcp_dbg_enable()) {
				pr_info("mtk_hcp:smvr(fd/buf/sz(%d/%lx/%lx))",
				isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_smvr),
				(unsigned long)isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_smvr),
				(unsigned long)isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_smvr));
			}
			info->gce_info[imgsys_smvr].g_wbuf_fd = isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_smvr);
			info->gce_info[imgsys_smvr].g_wbuf = isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_smvr);
			info->gce_info[imgsys_smvr].g_wbuf_sz = isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_smvr);
		}
	} else {
		/*WPE:0, OMC:1, ADL:2, TRAW:3, DIP:4, PQDIP:5, ME:6  */
		info->module_info_capture[0].c_wbuf =
				isp8_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_capture);
		info->module_info_capture[0].c_wbuf_dma =
				isp8_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_capture);
		info->module_info_capture[0].c_wbuf_sz =
				isp8_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_capture);
		info->module_info_capture[0].c_wbuf_fd =
				isp8_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_capture);
		info->module_info_capture[0].t_wbuf =
				isp8_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_capture);
		info->module_info_capture[0].t_wbuf_dma =
				isp8_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_capture);
		info->module_info_capture[0].t_wbuf_sz =
				isp8_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_capture);
		info->module_info_capture[0].t_wbuf_fd =
				isp8_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_capture);

		// OMC
		info->module_info_capture[1].c_wbuf =
				isp8_get_reserve_mem_phys(OMC_MEM_C_ID, imgsys_capture);
		info->module_info_capture[1].c_wbuf_dma =
				isp8_get_reserve_mem_dma(OMC_MEM_C_ID, imgsys_capture);
		info->module_info_capture[1].c_wbuf_sz =
				isp8_get_reserve_mem_size(OMC_MEM_C_ID, imgsys_capture);
		info->module_info_capture[1].c_wbuf_fd =
				isp8_get_reserve_mem_fd(OMC_MEM_C_ID, imgsys_capture);
		info->module_info_capture[1].t_wbuf =
				isp8_get_reserve_mem_phys(OMC_MEM_T_ID, imgsys_capture);
		info->module_info_capture[1].t_wbuf_dma =
				isp8_get_reserve_mem_dma(OMC_MEM_T_ID, imgsys_capture);
		info->module_info_capture[1].t_wbuf_sz =
				isp8_get_reserve_mem_size(OMC_MEM_T_ID, imgsys_capture);
		info->module_info_capture[1].t_wbuf_fd =
				isp8_get_reserve_mem_fd(OMC_MEM_T_ID, imgsys_capture);

		// ADL
		info->module_info_capture[2].c_wbuf =
				isp8_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_capture);
		info->module_info_capture[2].c_wbuf_dma =
				isp8_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_capture);
		info->module_info_capture[2].c_wbuf_sz =
				isp8_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_capture);
		info->module_info_capture[2].c_wbuf_fd =
				isp8_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_capture);
		info->module_info_capture[2].t_wbuf =
				isp8_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_capture);
		info->module_info_capture[2].t_wbuf_dma =
				isp8_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_capture);
		info->module_info_capture[2].t_wbuf_sz =
				isp8_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_capture);
		info->module_info_capture[2].t_wbuf_fd =
				isp8_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_capture);

		// TRAW
		info->module_info_capture[3].c_wbuf =
				isp8_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_capture);
		info->module_info_capture[3].c_wbuf_dma =
				isp8_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_capture);
		info->module_info_capture[3].c_wbuf_sz =
				isp8_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_capture);
		info->module_info_capture[3].c_wbuf_fd =
				isp8_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_capture);
		info->module_info_capture[3].t_wbuf =
				isp8_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_capture);
		info->module_info_capture[3].t_wbuf_dma =
				isp8_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_capture);
		info->module_info_capture[3].t_wbuf_sz =
				isp8_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_capture);
		info->module_info_capture[3].t_wbuf_fd =
				isp8_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_capture);

		// DIP
		info->module_info_capture[4].c_wbuf =
				isp8_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[4].c_wbuf_dma =
				isp8_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[4].c_wbuf_sz =
				isp8_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[4].c_wbuf_fd =
				isp8_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[4].t_wbuf =
				isp8_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_capture);
		info->module_info_capture[4].t_wbuf_dma =
				isp8_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_capture);
		info->module_info_capture[4].t_wbuf_sz =
				isp8_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_capture);
		info->module_info_capture[4].t_wbuf_fd =
				isp8_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_capture);

		// PQDIP
		info->module_info_capture[5].c_wbuf =
				isp8_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[5].c_wbuf_dma =
				isp8_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[5].c_wbuf_sz =
				isp8_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[5].c_wbuf_fd =
			isp8_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_capture);
		info->module_info_capture[5].t_wbuf =
				isp8_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_capture);
		info->module_info_capture[5].t_wbuf_dma =
				isp8_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_capture);
		info->module_info_capture[5].t_wbuf_sz =
				isp8_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_capture);
		info->module_info_capture[5].t_wbuf_fd =
				isp8_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_capture);

		// ME
		info->module_info_capture[6].c_wbuf =
				isp8_get_reserve_mem_phys(ME_MEM_C_ID, imgsys_capture);
		info->module_info_capture[6].c_wbuf_dma =
				isp8_get_reserve_mem_dma(ME_MEM_C_ID, imgsys_capture);
		info->module_info_capture[6].c_wbuf_sz =
				isp8_get_reserve_mem_size(ME_MEM_C_ID, imgsys_capture);
		info->module_info_capture[6].c_wbuf_fd =
			isp8_get_reserve_mem_fd(ME_MEM_C_ID, imgsys_capture);
		info->module_info_capture[6].t_wbuf = 0;
		info->module_info_capture[6].t_wbuf_dma = 0;
		info->module_info_capture[6].t_wbuf_sz = 0;
		info->module_info_capture[6].t_wbuf_fd = 0;

		if (hcp_dbg_enable()) {
			pr_info("mtk_hcp:capture(fd/buf/sz(%d/%lx/%lx))",
			isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_capture),
			(unsigned long)isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_capture),
			(unsigned long)isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_capture));
		}
		info->gce_info[imgsys_capture].g_wbuf_fd = isp8_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_capture);
		info->gce_info[imgsys_capture].g_wbuf = isp8_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_capture);
		info->gce_info[imgsys_capture].g_wbuf_sz = isp8_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_capture);
	}
	info->hw_buf = isp8_get_reserve_mem_phys(DIP_MEM_FOR_HW_ID, imgsys_streaming);

	/*common*/
	info->g_token_wbuf_fd = isp8_get_reserve_mem_fd(IMG_MEM_G_TOKEN_ID, imgsys_streaming);
	info->g_token_wbuf = isp8_get_reserve_mem_phys(IMG_MEM_G_TOKEN_ID, imgsys_streaming);
	/*info->g_wbuf_sw = isp7s_get_reserve_mem_virt(IMG_MEM_G_ID);*/
	info->g_token_wbuf_sz = isp8_get_reserve_mem_size(IMG_MEM_G_TOKEN_ID, imgsys_streaming);


	return 0;
}

int isp8_get_mem_info(struct img_init_info *info)
{
	return 0;
}
static int isp8_put_gce(unsigned int mode)
{
	if (mode == imgsys_streaming) {
		kref_put(&mb[IMG_MEM_G_ID].kref, gce_release_streaming);
		if (hcp_dbg_enable()) {
			pr_info("put-streaming gce count(%d)\n", kref_read(&mb[IMG_MEM_G_ID].kref));
		}
	}
	else if(mode == imgsys_capture) {
		kref_put(&cmb[IMG_MEM_G_ID].kref, gce_release_capture);
		if (hcp_dbg_enable()) {
			pr_info("put-capture gce count(%d)\n", kref_read(&cmb[IMG_MEM_G_ID].kref));
		}
	}
	else {
		kref_put(&smb[IMG_MEM_G_ID].kref, gce_release_smvr);
		if (hcp_dbg_enable()) {
			pr_info("put-smvr gce count(%d)\n", kref_read(&smb[IMG_MEM_G_ID].kref));
		}
	}

	return 0;
}

static int isp8_get_gce(unsigned int mode)
{
	if (mode == imgsys_streaming) {
		kref_get(&mb[IMG_MEM_G_ID].kref);
		if (hcp_dbg_enable()) {
			pr_info("get-streaming gce count(%d)\n", kref_read(&mb[IMG_MEM_G_ID].kref));
		}
    }
	else if(mode == imgsys_capture) {
		kref_get(&cmb[IMG_MEM_G_ID].kref);
		if (hcp_dbg_enable()) {
			pr_info("get-capture gce count(%d)\n", kref_read(&cmb[IMG_MEM_G_ID].kref));
		}
	}
	else {
		kref_get(&smb[IMG_MEM_G_ID].kref);
		if (hcp_dbg_enable()) {
			pr_info("get-smvr gce count(%d)\n", kref_read(&smb[IMG_MEM_G_ID].kref));
		}
	}
	//kref_get(&gmb[IMG_MEM_G_ID - IMG_MEM_FOR_HW_ID].kref);
	return 0;
}

int isp8_partial_flush(struct mtk_hcp *hcp_dev, struct flush_buf_info *b_info)
{
	struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
	struct mtk_hcp_smvr_reserve_mblock *smblock = NULL;
	struct mtk_hcp_capture_reserve_mblock *cmblock = NULL;
	unsigned int block_num = 0;
	unsigned int id = 0;
	unsigned int mode = 0;
	//pr_info("imgsys_fw no partial flush");
	if (b_info->is_tuning)
		dma_buf_end_cpu_access_partial(b_info->dbuf,
					DMA_BIDIRECTIONAL,
					b_info->offset,
					b_info->len);
	else {
		mode = b_info->mode;
		if (mode == imgsys_streaming) {
			mblock = hcp_dev->data->mblock;
		} else if (mode == imgsys_capture) {
			cmblock = hcp_dev->data->cmblock;
		} else {
			smblock = hcp_dev->data->smblock;
		}

		block_num = hcp_dev->data->block_num;
		for (id = 0; id < block_num; id++) {
			if (mode == imgsys_streaming) {
				if (b_info->fd == mblock[id].fd) {
					dma_buf_end_cpu_access_partial(mblock[id].d_buf, DMA_BIDIRECTIONAL, b_info->offset, b_info->len);
					break;
				}
			} else if (mode == imgsys_capture) {
				if (b_info->fd == cmblock[id].fd) {
					dma_buf_end_cpu_access_partial(cmblock[id].d_buf, DMA_BIDIRECTIONAL, b_info->offset, b_info->len);
					break;
				}
			} else {
				if (b_info->fd == smblock[id].fd) {
					dma_buf_end_cpu_access_partial(smblock[id].d_buf, DMA_BIDIRECTIONAL, b_info->offset, b_info->len);
					break;
				}
			}
		}
		if (hcp_dbg_enable()) {
			pr_debug("imgsys_fw partial flush info(%d/%d/%d)", b_info->fd, b_info->len, b_info->offset);
		}
	}

	return 0;
}

struct mtk_hcp_data isp8_hcp_data = {
	.mblock = isp8_streaming_mblock,
	.smblock = isp8_smvr_mblock,
	.cmblock = isp8_capture_mblock,
	.gmblock = isp8_gce_mblock,
	//.gsmblock = isp7s_gce_smvr_mblock,
	.block_num = ARRAY_SIZE(isp8_streaming_mblock),
	.block_num_gce = ARRAY_SIZE(isp8_gce_mblock),
	.allocate = isp8_allocate_working_buffer,
	.release = isp8_release_working_buffer,
	.release_gce_buf = isp8_release_gce_working_buffer,
	.get_init_info = isp8_get_init_info,
	.get_mem_info = isp8_get_mem_info,
	.get_gce_virt = isp8_get_gce_virt,
	.get_gce = isp8_get_gce,
	.put_gce = isp8_put_gce,
	.get_gce_token_virt = isp8_get_gce_token_virt,
	.get_hwid_virt = isp8_get_hwid_virt,
	.get_wpe_virt = isp8_get_wpe_virt,
	.get_wpe_cq_fd = isp8_get_wpe_cq_fd,
	.get_wpe_tdr_fd = isp8_get_wpe_tdr_fd,
	.get_omc_virt = isp8_get_omc_virt,
	.get_omc_cq_fd = isp8_get_omc_cq_fd,
	.get_omc_tdr_fd = isp8_get_omc_tdr_fd,
	.get_dip_virt = isp8_get_dip_virt,
	.get_dip_cq_fd = isp8_get_dip_cq_fd,
	.get_dip_tdr_fd = isp8_get_dip_tdr_fd,
	.get_traw_virt = isp8_get_traw_virt,
	.get_traw_cq_fd = isp8_get_traw_cq_fd,
	.get_traw_tdr_fd = isp8_get_traw_tdr_fd,
	.get_pqdip_virt = isp8_get_pqdip_virt,
	.get_pqdip_cq_fd = isp8_get_pqdip_cq_fd,
	.get_pqdip_tdr_fd = isp8_get_pqdip_tdr_fd,
	.get_me_virt = isp8_get_me_virt,
	.get_me_cq_fd = isp8_get_me_cq_fd,
	.partial_flush = NULL,
};
#else
int isp8_release_working_buffer(struct mtk_hcp *hcp_dev)
{
	enum isp8_rsv_mem_id_t id;
	struct mtk_hcp_reserve_mblock *mblock;
	unsigned int block_num;

	mblock = mb;
	block_num = hcp_dev->data->block_num;

	/* release reserved memory */
	for (id = 0; id < NUMS_MEM_ID; id++) {
		if (mblock[id].is_dma_buf) {
			switch (id) {
			case IMG_MEM_FOR_HW_ID:
				/*allocated at probe via dts*/
				break;
			case IMG_MEM_G_ID:
				kref_put(&mblock[id].kref, gce_release);
				break;
			default:
				#ifdef HCP_NEW_DMA_BUF_API
				/* free va */
				dma_buf_vunmap_unlocked(mblock[id].d_buf, &mblock[id].map);
				/* free iova */
				dma_buf_unmap_attachment_unlocked(mblock[id].attach,
				mblock[id].sgt, DMA_BIDIRECTIONAL);
				#else
				/* free va */
				dma_buf_vunmap(mblock[id].d_buf, &mblock[id].map);
				/* free iova */
				dma_buf_unmap_attachment(mblock[id].attach,
				mblock[id].sgt, DMA_BIDIRECTIONAL);
				#endif
				dma_buf_detach(mblock[id].d_buf,
				mblock[id].attach);
				dma_buf_put(mblock[id].d_buf);
				// close fd in user space driver, you can't close fd in kernel site
				// dma_heap_buffer_free(mblock[id].d_buf);
				//dma_buf_put(my_dma_buf);
				//also can use this api, but not recommended
				mblock[id].mem_priv = NULL;
				mblock[id].mmap_cnt = 0;
				mblock[id].start_dma = 0x0;
				mblock[id].start_virt = 0x0;
				mblock[id].start_phys = 0x0;
				mblock[id].d_buf = NULL;
				mblock[id].fd = -1;
				mblock[id].pIonHandle = NULL;
				mblock[id].attach = NULL;
				mblock[id].sgt = NULL;
				break;
			}
		} else {
			kfree(mblock[id].start_virt);
			mblock[id].start_virt = 0x0;
			mblock[id].start_phys = 0x0;
			mblock[id].start_dma = 0x0;
			mblock[id].mmap_cnt = 0;
		}
        if (hcp_dbg_enable()) {
    		pr_debug(
    			"%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d\n",
    			__func__, id,
    			isp8_get_reserve_mem_phys(id),
    			isp8_get_reserve_mem_virt(id),
    			isp8_get_reserve_mem_dma(id),
    			isp8_get_reserve_mem_size(id),
    			mblock[id].is_dma_buf,
    			isp8_get_reserve_mem_fd(id));
	    }
	}

	return 0;
}
EXPORT_SYMBOL(isp8_release_working_buffer);

int isp8_get_init_info(struct img_init_info *info)
{

	if (!info) {
		pr_info("%s:NULL info\n", __func__);
		return -1;
	}

	info->hw_buf = isp8_get_reserve_mem_phys(DIP_MEM_FOR_HW_ID);
	/*WPE:0, OMC:1, ADL:2, TRAW:3, DIP:4, PQDIP:5 */
	info->module_info[0].c_wbuf =
				isp8_get_reserve_mem_phys(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_dma =
				isp8_get_reserve_mem_dma(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_sz =
				isp8_get_reserve_mem_size(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_fd =
				isp8_get_reserve_mem_fd(WPE_MEM_C_ID);
	info->module_info[0].t_wbuf =
				isp8_get_reserve_mem_phys(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_dma =
				isp8_get_reserve_mem_dma(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_sz =
				isp8_get_reserve_mem_size(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_fd =
				isp8_get_reserve_mem_fd(WPE_MEM_T_ID);

	// OMC
	info->module_info[1].c_wbuf =
				isp8_get_reserve_mem_phys(OMC_MEM_C_ID);
	info->module_info[1].c_wbuf_dma =
				isp8_get_reserve_mem_dma(OMC_MEM_C_ID);
	info->module_info[1].c_wbuf_sz =
				isp8_get_reserve_mem_size(OMC_MEM_C_ID);
	info->module_info[1].c_wbuf_fd =
				isp8_get_reserve_mem_fd(OMC_MEM_C_ID);
	info->module_info[1].t_wbuf =
				isp8_get_reserve_mem_phys(OMC_MEM_T_ID);
	info->module_info[1].t_wbuf_dma =
				isp8_get_reserve_mem_dma(OMC_MEM_T_ID);
	info->module_info[1].t_wbuf_sz =
				isp8_get_reserve_mem_size(OMC_MEM_T_ID);
	info->module_info[1].t_wbuf_fd =
				isp8_get_reserve_mem_fd(OMC_MEM_T_ID);

  // ADL
	info->module_info[2].c_wbuf =
				isp8_get_reserve_mem_phys(ADL_MEM_C_ID);
	info->module_info[2].c_wbuf_dma =
				isp8_get_reserve_mem_dma(ADL_MEM_C_ID);
	info->module_info[2].c_wbuf_sz =
				isp8_get_reserve_mem_size(ADL_MEM_C_ID);
	info->module_info[2].c_wbuf_fd =
				isp8_get_reserve_mem_fd(ADL_MEM_C_ID);
	info->module_info[2].t_wbuf =
				isp8_get_reserve_mem_phys(ADL_MEM_T_ID);
	info->module_info[2].t_wbuf_dma =
				isp8_get_reserve_mem_dma(ADL_MEM_T_ID);
	info->module_info[2].t_wbuf_sz =
				isp8_get_reserve_mem_size(ADL_MEM_T_ID);
	info->module_info[2].t_wbuf_fd =
				isp8_get_reserve_mem_fd(ADL_MEM_T_ID);

	// TRAW
	info->module_info[3].c_wbuf =
				isp8_get_reserve_mem_phys(TRAW_MEM_C_ID);
	info->module_info[3].c_wbuf_dma =
				isp8_get_reserve_mem_dma(TRAW_MEM_C_ID);
	info->module_info[3].c_wbuf_sz =
				isp8_get_reserve_mem_size(TRAW_MEM_C_ID);
	info->module_info[3].c_wbuf_fd =
				isp8_get_reserve_mem_fd(TRAW_MEM_C_ID);
	info->module_info[3].t_wbuf =
				isp8_get_reserve_mem_phys(TRAW_MEM_T_ID);
	info->module_info[3].t_wbuf_dma =
				isp8_get_reserve_mem_dma(TRAW_MEM_T_ID);
	info->module_info[3].t_wbuf_sz =
				isp8_get_reserve_mem_size(TRAW_MEM_T_ID);
	info->module_info[3].t_wbuf_fd =
				isp8_get_reserve_mem_fd(TRAW_MEM_T_ID);

		// DIP
	info->module_info[4].c_wbuf =
				isp8_get_reserve_mem_phys(DIP_MEM_C_ID);
	info->module_info[4].c_wbuf_dma =
				isp8_get_reserve_mem_dma(DIP_MEM_C_ID);
	info->module_info[4].c_wbuf_sz =
				isp8_get_reserve_mem_size(DIP_MEM_C_ID);
	info->module_info[4].c_wbuf_fd =
				isp8_get_reserve_mem_fd(DIP_MEM_C_ID);
	info->module_info[4].t_wbuf =
				isp8_get_reserve_mem_phys(DIP_MEM_T_ID);
	info->module_info[4].t_wbuf_dma =
				isp8_get_reserve_mem_dma(DIP_MEM_T_ID);
	info->module_info[4].t_wbuf_sz =
				isp8_get_reserve_mem_size(DIP_MEM_T_ID);
	info->module_info[4].t_wbuf_fd =
				isp8_get_reserve_mem_fd(DIP_MEM_T_ID);

	// PQDIP
	info->module_info[5].c_wbuf =
				isp8_get_reserve_mem_phys(PQDIP_MEM_C_ID);
	info->module_info[5].c_wbuf_dma =
				isp8_get_reserve_mem_dma(PQDIP_MEM_C_ID);
	info->module_info[5].c_wbuf_sz =
				isp8_get_reserve_mem_size(PQDIP_MEM_C_ID);
	info->module_info[5].c_wbuf_fd =
				isp8_get_reserve_mem_fd(PQDIP_MEM_C_ID);
	info->module_info[5].t_wbuf =
				isp8_get_reserve_mem_phys(PQDIP_MEM_T_ID);
	info->module_info[5].t_wbuf_dma =
				isp8_get_reserve_mem_dma(PQDIP_MEM_T_ID);
	info->module_info[5].t_wbuf_sz =
				isp8_get_reserve_mem_size(PQDIP_MEM_T_ID);
	info->module_info[5].t_wbuf_fd =
				isp8_get_reserve_mem_fd(PQDIP_MEM_T_ID);

	// ME
	info->module_info[5].c_wbuf =
				isp8_get_reserve_mem_phys(NE_MEM_C_ID);
	info->module_info[5].c_wbuf_dma =
				isp8_get_reserve_mem_dma(NE_MEM_C_ID);
	info->module_info[5].c_wbuf_sz =
				isp8_get_reserve_mem_size(NE_MEM_C_ID);
	info->module_info[5].c_wbuf_fd =
				isp8_get_reserve_mem_fd(NE_MEM_C_ID);
	info->module_info[5].t_wbuf = 0;
	info->module_info[5].t_wbuf_dma = 0;
	info->module_info[5].t_wbuf_sz = 0;
	info->module_info[5].t_wbuf_fd = 0;

	/*common*/
	/* info->g_wbuf_fd = isp7sp_get_reserve_mem_fd(IMG_MEM_G_ID); */
	info->g_wbuf_fd = isp8_get_reserve_mem_fd(IMG_MEM_G_ID);
	info->g_wbuf = isp8_get_reserve_mem_phys(IMG_MEM_G_ID);
	/*info->g_wbuf_sw = isp7sp_get_reserve_mem_virt(IMG_MEM_G_ID);*/
	info->g_wbuf_sz = isp8_get_reserve_mem_size(IMG_MEM_G_ID);

	return 0;
}

static int isp8_put_gce(void)
{
	kref_put(&mb[IMG_MEM_G_ID].kref, gce_release);
	return 0;
}

static int isp8_get_gce(void)
{
	kref_get(&mb[IMG_MEM_G_ID].kref);
	return 0;
}

int isp8_partial_flush(struct mtk_hcp *hcp_dev, struct flush_buf_info *b_info)
{
	struct mtk_hcp_reserve_mblock *mblock = NULL;
	unsigned int block_num = 0;
	unsigned int id = 0;
	unsigned int mode = 0;

	if (b_info->is_tuning)
		dma_buf_end_cpu_access_partial(b_info->dbuf,
					       DMA_BIDIRECTIONAL,
					       b_info->offset,
					       b_info->len);
	else {
		mode = b_info->mode;
		if (mode == imgsys_smvr)
			mblock = hcp_dev->data->smblock;
		else if (mode == imgsys_capture)
			mblock = hcp_dev->data->mblock;
		else
			mblock = hcp_dev->data->mblock;

		block_num = hcp_dev->data->block_num;
		for (id = 0; id < block_num; id++) {
			if (b_info->fd == mblock[id].fd) {
				dma_buf_end_cpu_access_partial(mblock[id].d_buf,
							       DMA_BIDIRECTIONAL,
							       b_info->offset,
							       b_info->len);
				break;
			}
		}
	}
    if (hcp_dbg_enable()) {
    	pr_debug("imgsys_fw partial flush info(%d/0x%x/0x%x), mode(%d), is_tuning(%d)",
    		b_info->fd, b_info->len, b_info->offset, b_info->mode, b_info->is_tuning);
    }
	return 0;
}
struct mtk_hcp_data isp8_hcp_data = {
	.mblock = isp8_reserve_mblock,
	.block_num = ARRAY_SIZE(isp8_reserve_mblock),
	.smblock = isp8_smvr_mblock,
	.allocate = isp8_allocate_working_buffer,
	.release = isp8_release_working_buffer,
	.get_init_info = isp8_get_init_info,
	.get_gce_virt = isp8_get_gce_virt,
	.get_gce = isp8_get_gce,
	.put_gce = isp8_put_gce,
	.get_hwid_virt = isp8_get_hwid_virt,
	.get_wpe_virt = isp8_get_wpe_virt,
	.get_wpe_cq_fd = isp8_get_wpe_cq_fd,
	.get_wpe_tdr_fd = isp8_get_wpe_tdr_fd,
	.get_omc_virt = isp8_get_omc_virt,
	.get_omc_cq_fd = isp8_get_omc_cq_fd,
	.get_omc_tdr_fd = isp8_get_omc_tdr_fd,
	.get_dip_virt = isp8_get_dip_virt,
	.get_dip_cq_fd = isp8_get_dip_cq_fd,
	.get_dip_tdr_fd = isp8_get_dip_tdr_fd,
	.get_traw_virt = isp8_get_traw_virt,
	.get_traw_cq_fd = isp8_get_traw_cq_fd,
	.get_traw_tdr_fd = isp8_get_traw_tdr_fd,
	.get_pqdip_virt = isp8_get_pqdip_virt,
	.get_pqdip_cq_fd = isp8_get_pqdip_cq_fd,
	.get_pqdip_tdr_fd = isp8_get_pqdip_tdr_fd,
	.get_me_virt = isp8_get_me_virt,
	.get_me_cq_fd = isp8_get_me_cq_fd,
	.partial_flush = NULL,
};
#endif
MODULE_IMPORT_NS(DMA_BUF);
