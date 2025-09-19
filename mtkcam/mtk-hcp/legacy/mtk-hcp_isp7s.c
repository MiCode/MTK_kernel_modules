// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/slab.h>
#include <linux/kref.h>
#include <mtk_heap.h>
#include "mtk-hcp_isp7s.h"


#if SMVR_DECOUPLE
static struct mtk_hcp_streaming_reserve_mblock *mb;
static struct mtk_hcp_smvr_reserve_mblock *smb;
static struct mtk_hcp_capture_reserve_mblock *cmb;
static struct mtk_hcp_gce_token_reserve_mblock *gmb;
#else
static struct mtk_hcp_reserve_mblock *mb;
#endif

#if !SMVR_DECOUPLE
enum isp7s_rsv_mem_id_t {
	DIP_MEM_FOR_HW_ID,
	IMG_MEM_FOR_HW_ID = DIP_MEM_FOR_HW_ID, /*shared buffer for ipi_param*/
	/*need replace DIP_MEM_FOR_HW_ID & DIP_MEM_FOR_SW_ID*/
	WPE_MEM_C_ID,	/*module cq buffer*/
	WPE_MEM_T_ID,	/*module tdr buffer*/
	TRAW_MEM_C_ID,	/*module cq buffer*/
	TRAW_MEM_T_ID,	/*module tdr buffer*/
	DIP_MEM_C_ID,	/*module cq buffer*/
	DIP_MEM_T_ID,	/*module tdr buffer*/
	PQDIP_MEM_C_ID,	/*module cq buffer*/
	PQDIP_MEM_T_ID,	/*module tdr buffer*/
	ADL_MEM_C_ID,	/*module cq buffer*/
	ADL_MEM_T_ID,	/*module tdr buffer*/
	IMG_MEM_G_ID,	/*gce cmd buffer*/
	NUMS_MEM_ID,
};
#else
enum isp7s_rsv_mem_id_t {
	/*need replace DIP_MEM_FOR_HW_ID & DIP_MEM_FOR_SW_ID*/
	WPE_MEM_C_ID,	/*module cq buffer*/
	WPE_MEM_T_ID,	/*module tdr buffer*/
	TRAW_MEM_C_ID,	/*module cq buffer*/
	TRAW_MEM_T_ID,	/*module tdr buffer*/
	DIP_MEM_C_ID,	/*module cq buffer*/
	DIP_MEM_T_ID,	/*module tdr buffer*/
	PQDIP_MEM_C_ID,	/*module cq buffer*/
	PQDIP_MEM_T_ID,	/*module tdr buffer*/
	ADL_MEM_C_ID,	/*module cq buffer*/
	ADL_MEM_T_ID,	/*module tdr buffer*/
	IMG_MEM_G_ID,	/*gce cmd buffer*/
	NUMS_WB_MEM_ID,
};

enum isp7s_rsv_gce_mem_id_t {
	DIP_MEM_FOR_HW_ID = 12,
	IMG_MEM_FOR_HW_ID = DIP_MEM_FOR_HW_ID, /*shared buffer for ipi_param*/
	IMG_MEM_G_TOKEN_ID,	/*gce cmd buffer*/
	NUMS_CM_MEM_ID,
};
#endif

#if !SMVR_DECOUPLE
static struct mtk_hcp_reserve_mblock isp7s_smvr_mblock[] = {
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
		.size = 0x322000, //align_4096(0x321C90)
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
		.size = 0xB7C000, /*align_4096(0xB7C000) */
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
		.size = 0xB99000, /*align_4096(0xB987E0) */
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
		.size = 0x18D9000, /*align_4096(0x18D81E0) */
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
		.size = 0xEE8000, /*align_4096(0xEE7320) */
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
		.size = 0x1D22000, /*align_4096(0x1D22000) */
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
		.size = 0x126000, /*align_4096(0x125100) */
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
		.size = 0x1E1000, /*align_4096(0x1E0400) */
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
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
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

struct mtk_hcp_reserve_mblock isp7s_reserve_mblock[] = {
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
		.size = 0xD5000,   /* align_4096(0xD42BC) */
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
		.size = 0x30C000,   /* align_4096(0x30C000) */
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
		.size = 0x54B000,   /* align_4096(0x54A8E0) */
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
		.size = 0x1609000,   /* align_4096(0x16081E0) */
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
		.size = 0x627000,   /* align_4096(0x6261A0) */
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
		.size = 0x1962000,   /* align_4096(0x1962000) */
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
		.size = 0xDC000,   /* align_4096(0xDBB10) */
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
		.size = 0x169000,   /* align_4096(0x168400) */
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
		.name = "IMG_MEM_G_ID",
		.num = IMG_MEM_G_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
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

static struct mtk_hcp_smvr_reserve_mblock isp7s_smvr_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x311700, //align_4096(0x25D700)
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
		.size = 0xB40000, /*align_4096(0x8AC000) */
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
		.size = 0xB25360, /*align_4096(0xB987E0) */
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
		.size = 0x1735000, /*align_4096(0x18D81E0) */
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
		.size = 0xD22000, /*align_4096(0xEE7320) */
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
		.size = 0x1BBB000, /*align_4096(0x1D22000) */
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
		.size = 0xDC000, /*align_4096(0x125100) */
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
		.size = 0x16B000, /*align_4096(0x1E0400) */
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
		.size = 0x100000,
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
		.size = 0x200000,
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
		.size = 0x1F45E00,//to do for smvr //0xF68400, //0x2000000,
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


static struct mtk_hcp_streaming_reserve_mblock isp7s_streaming_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xC5000,   /* 0xE1000 900KB*/
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
		.size = 0x2E0000,   /*0x500000 4MB*/
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
		.size = 0x3B8000,   /*0x596000 5MB + 600KB*/
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
		.size = 0x1465000,   /*0x1E00000 30MB*/
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
		.size = 0x461000,   /*0x67D000 6MB + 500KB*/
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
		.size = 0x17FB000,   /*0x207D000 32MB + 500KB*/
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
		.size = 0xA0000,   /*0x100000 1MB*/
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
		.size = 0xF3000,   /*0x1C8000 1MB + 800KB*/
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
		.size = 0x100000,   /*1MB*/
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
		.size = 0x200000,   /*2MB*/
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
		.size = 0x11D8400,//to do for smvr //0xF68400, //0x2000000,
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

static struct mtk_hcp_capture_reserve_mblock isp7s_capture_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x11000, //0x11000,   /*900KB*/
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
		.size = 0x3D000, //0x3D000,   /*4MB*/
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
		.size = 0x194000, //0x194000,   /*5MB + 600KB*/
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
		.size = 0x14A1000, //0x14A1000,   /*30MB*/
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
		.size = 0x1C6000, //0x1C6000,   /*6MB + 500KB*/
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
		.size = 0x1783000, //0x1783000,   /*32MB + 500KB*/
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
		.size = 0x4A000,   /*1MB*/
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
		.size = 0x7B000,   /*1MB + 800KB*/
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
		.size = 0x100000,   /*1MB*/
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
		.size = 0x200000,   /*2MB*/
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
		.size = 0x11D8400,//to do for smvr //0xF68400, //0x2000000,
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

static struct mtk_hcp_gce_token_reserve_mblock isp7s_gce_mblock[] = {
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
#endif
#endif
#if SMVR_DECOUPLE
phys_addr_t isp7s_get_reserve_mem_phys(unsigned int id, unsigned int mode)
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
EXPORT_SYMBOL(isp7s_get_reserve_mem_phys);

void *isp7s_get_reserve_mem_virt(unsigned int id, unsigned int mode)
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
EXPORT_SYMBOL(isp7s_get_reserve_mem_virt);

phys_addr_t isp7s_get_reserve_mem_dma(unsigned int id, unsigned int mode)
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
EXPORT_SYMBOL(isp7s_get_reserve_mem_dma);

phys_addr_t isp7s_get_reserve_mem_size(unsigned int id, unsigned int mode)
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
EXPORT_SYMBOL(isp7s_get_reserve_mem_size);

uint32_t isp7s_get_reserve_mem_fd(unsigned int id, unsigned int mode)
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
EXPORT_SYMBOL(isp7s_get_reserve_mem_fd);

void *isp7s_get_gce_virt(unsigned int mode)
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
EXPORT_SYMBOL(isp7s_get_gce_virt);

void *isp7s_get_gce_token_virt(unsigned int mode)
{
    pr_info("mtk_hcp gce_start_virt(0x%lx)", (unsigned long)gmb[IMG_MEM_G_TOKEN_ID - IMG_MEM_FOR_HW_ID].start_virt);
    return gmb[IMG_MEM_G_TOKEN_ID - IMG_MEM_FOR_HW_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_gce_token_virt);

void *isp7s_get_wpe_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[WPE_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_C_ID].start_virt;
	else
		return smb[WPE_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_wpe_virt);

int isp7s_get_wpe_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[WPE_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_C_ID].fd;
	else
		return smb[WPE_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_wpe_cq_fd);

int isp7s_get_wpe_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[WPE_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_T_ID].fd;
	else
		return smb[WPE_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_wpe_tdr_fd);

void *isp7s_get_dip_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[DIP_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_C_ID].start_virt;
	else
		return smb[DIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_dip_virt);

int isp7s_get_dip_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[DIP_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_C_ID].fd;
	else
		return smb[DIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_dip_cq_fd);

int isp7s_get_dip_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[DIP_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_T_ID].fd;
	else
		return smb[DIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_dip_tdr_fd);

void *isp7s_get_traw_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[TRAW_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_C_ID].start_virt;
	else
		return smb[TRAW_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_traw_virt);

int isp7s_get_traw_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[TRAW_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_C_ID].fd;
	else
		return smb[TRAW_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_traw_cq_fd);

int isp7s_get_traw_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[TRAW_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_T_ID].fd;
	else
		return smb[TRAW_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_traw_tdr_fd);

void *isp7s_get_pqdip_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[PQDIP_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_C_ID].start_virt;
	else
		return smb[PQDIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_pqdip_virt);

int isp7s_get_pqdip_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[PQDIP_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_C_ID].fd;
	else
		return smb[PQDIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_pqdip_cq_fd);

int isp7s_get_pqdip_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
		return mb[PQDIP_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_T_ID].fd;
	else
		return smb[PQDIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_pqdip_tdr_fd);

void *isp7s_get_hwid_virt(unsigned int mode)
{
    return gmb[0].start_virt;
}
EXPORT_SYMBOL(isp7s_get_hwid_virt);

static int isp7s_module_driver_allocate_working_buffer_streaming(struct mtk_hcp *hcp_dev,
    unsigned int str_mode, struct mtk_hcp_streaming_reserve_mblock *str_mblock)
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
                        pr_info("mtk_hcp-gce_block\n");
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
                            dma_buf_attach(str_mblock[id].d_buf, hcp_dev->dev);
                        attach = str_mblock[id].attach;
                        if (IS_ERR(attach)) {
                            pr_info("dma_buf_attach fail :%ld\n",
                            PTR_ERR(attach));
                            return -1;
                        }

                        str_mblock[id].sgt =
                        dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
                        sgt = str_mblock[id].sgt;
                        if (IS_ERR(sgt)) {
                                dma_buf_detach(str_mblock[id].d_buf, attach);
                            pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                            PTR_ERR(sgt));
                            return -1;
                        }
                        str_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                        str_mblock[id].start_dma = str_mblock[id].start_phys;
                        ret = dma_buf_vmap(str_mblock[id].d_buf, &map);
                        if (ret) {
                            pr_info("sg_dma_address fail\n");
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
                            pr_debug("%s:[HCP_GCE][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                                __func__, str_mblock[id].name,
                                isp7s_get_reserve_mem_phys(id, str_mode),
                                isp7s_get_reserve_mem_virt(id, str_mode),
                                isp7s_get_reserve_mem_dma(id, str_mode),
                                isp7s_get_reserve_mem_size(id, str_mode),
                                str_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, str_mode),
                                str_mblock[id].d_buf);
                        }
                        break;
                case WPE_MEM_C_ID:
                case WPE_MEM_T_ID:
                case DIP_MEM_C_ID:
                case DIP_MEM_T_ID:
                case TRAW_MEM_C_ID:
                case TRAW_MEM_T_ID:
                case PQDIP_MEM_C_ID:
                case PQDIP_MEM_T_ID:
                case ADL_MEM_C_ID:
                case ADL_MEM_T_ID:
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
                    str_mblock[id].d_buf, hcp_dev->dev);
                    attach = str_mblock[id].attach;
                    if (IS_ERR(attach)) {
                        pr_info("dma_buf_attach fail :%ld\n",
                        PTR_ERR(attach));
                        return -1;
                    }

                    str_mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
                    sgt = str_mblock[id].sgt;
                    if (IS_ERR(sgt)) {
                        dma_buf_detach(str_mblock[id].d_buf, attach);
                        pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                        PTR_ERR(sgt));
                        return -1;
                    }
                    str_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                    str_mblock[id].start_dma = str_mblock[id].start_phys;
                    ret = dma_buf_vmap(str_mblock[id].d_buf, &map);
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
                    pr_debug("%s:[HCP_WORKING][%s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx,size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                            __func__, str_mblock[id].name, id,
                                isp7s_get_reserve_mem_phys(id, str_mode),
                                isp7s_get_reserve_mem_virt(id, str_mode),
                                isp7s_get_reserve_mem_dma(id, str_mode),
                                isp7s_get_reserve_mem_size(id, str_mode),
                                str_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, str_mode),
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
                    str_mblock[id].d_buf, hcp_dev->dev);
                    attach = str_mblock[id].attach;
                    if (IS_ERR(attach)) {
                        pr_info("dma_buf_attach fail :%ld\n",
                        PTR_ERR(attach));
                        return -1;
                    }

                    str_mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
                    sgt = str_mblock[id].sgt;
                    if (IS_ERR(sgt)) {
                        dma_buf_detach(str_mblock[id].d_buf, attach);
                        pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                        PTR_ERR(sgt));
                        return -1;
                    }
                    str_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                    str_mblock[id].start_dma = str_mblock[id].start_phys;
                    ret = dma_buf_vmap(str_mblock[id].d_buf, &map);
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
                        pr_debug("%s:[HCP_WORKING_DEFAULT][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                                __func__, str_mblock[id].name,
                                isp7s_get_reserve_mem_phys(id, str_mode),
                                isp7s_get_reserve_mem_virt(id, str_mode),
                                isp7s_get_reserve_mem_dma(id, str_mode),
                                isp7s_get_reserve_mem_size(id, str_mode),
                                str_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, str_mode),
                                str_mblock[id].d_buf);
                    }
                    break;
                }
            } else {
                str_mblock[id].start_virt =
                    kzalloc(str_mblock[id].size,
                        GFP_KERNEL);
                str_mblock[id].start_phys =
                    virt_to_phys(
                        str_mblock[id].start_virt);
                str_mblock[id].start_dma = 0;
            }
            if (hcp_dbg_enable()) {
                pr_debug(
                    "%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                    __func__, id,
                    isp7s_get_reserve_mem_phys(id, str_mode),
                    isp7s_get_reserve_mem_virt(id, str_mode),
                    isp7s_get_reserve_mem_dma(id, str_mode),
                    isp7s_get_reserve_mem_size(id, str_mode),
                    str_mblock[id].is_dma_buf,
                    isp7s_get_reserve_mem_fd(id, str_mode),
                    str_mblock[id].d_buf);
            }
        }
        return 0;
}

static int isp7s_module_driver_allocate_working_buffer_capture(struct mtk_hcp *hcp_dev,
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
                            dma_buf_attach(cap_mblock[id].d_buf, hcp_dev->dev);
                        attach = cap_mblock[id].attach;
                        if (IS_ERR(attach)) {
                            pr_info("dma_buf_attach fail :%ld\n",
                            PTR_ERR(attach));
                            return -1;
                        }

                        cap_mblock[id].sgt =
                        dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
                        sgt = cap_mblock[id].sgt;
                        if (IS_ERR(sgt)) {
                                dma_buf_detach(cap_mblock[id].d_buf, attach);
                            pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                            PTR_ERR(sgt));
                            return -1;
                        }
                        cap_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                        cap_mblock[id].start_dma = cap_mblock[id].start_phys;
                        ret = dma_buf_vmap(cap_mblock[id].d_buf, &map);
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
                            pr_debug("%s:[HCP_GCE][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                                __func__, cap_mblock[id].name,
                                isp7s_get_reserve_mem_phys(id, cap_mode),
                                isp7s_get_reserve_mem_virt(id, cap_mode),
                                isp7s_get_reserve_mem_dma(id, cap_mode),
                                isp7s_get_reserve_mem_size(id, cap_mode),
                                cap_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, cap_mode),
                                cap_mblock[id].d_buf);
                        }
                        break;
                case WPE_MEM_C_ID:
                case WPE_MEM_T_ID:
                case DIP_MEM_C_ID:
                case DIP_MEM_T_ID:
                case TRAW_MEM_C_ID:
                case TRAW_MEM_T_ID:
                case PQDIP_MEM_C_ID:
                case PQDIP_MEM_T_ID:
                case ADL_MEM_C_ID:
                case ADL_MEM_T_ID:
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
                    cap_mblock[id].d_buf, hcp_dev->dev);
                    attach = cap_mblock[id].attach;
                    if (IS_ERR(attach)) {
                        pr_info("dma_buf_attach fail :%ld\n",
                        PTR_ERR(attach));
                        return -1;
                    }

                    cap_mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
                    sgt = cap_mblock[id].sgt;
                    if (IS_ERR(sgt)) {
                        dma_buf_detach(cap_mblock[id].d_buf, attach);
                        pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                        PTR_ERR(sgt));
                        return -1;
                    }
                    cap_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                    cap_mblock[id].start_dma = cap_mblock[id].start_phys;
                    ret = dma_buf_vmap(cap_mblock[id].d_buf, &map);
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
                        pr_debug("%s:[HCP_WORKING][%s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx,size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                            __func__, cap_mblock[id].name, id,
                                isp7s_get_reserve_mem_phys(id, cap_mode),
                                isp7s_get_reserve_mem_virt(id, cap_mode),
                                isp7s_get_reserve_mem_dma(id, cap_mode),
                                isp7s_get_reserve_mem_size(id, cap_mode),
                                cap_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, cap_mode),
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
                    cap_mblock[id].d_buf, hcp_dev->dev);
                    attach = cap_mblock[id].attach;
                    if (IS_ERR(attach)) {
                        pr_info("dma_buf_attach fail :%ld\n",
                        PTR_ERR(attach));
                        return -1;
                    }

                    cap_mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
                    sgt = cap_mblock[id].sgt;
                    if (IS_ERR(sgt)) {
                        dma_buf_detach(cap_mblock[id].d_buf, attach);
                        pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                        PTR_ERR(sgt));
                        return -1;
                    }
                    cap_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                    cap_mblock[id].start_dma = cap_mblock[id].start_phys;
                    ret = dma_buf_vmap(cap_mblock[id].d_buf, &map);
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
                        pr_debug("%s:[HCP_WORKING_DEFAULT][%s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                            __func__, cap_mblock[id].name, id,
                                isp7s_get_reserve_mem_phys(id, cap_mode),
                                isp7s_get_reserve_mem_virt(id, cap_mode),
                                isp7s_get_reserve_mem_dma(id, cap_mode),
                                isp7s_get_reserve_mem_size(id, cap_mode),
                                cap_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, cap_mode),
                                cap_mblock[id].d_buf);
                    }
                    break;
                }
            } else {
                cap_mblock[id].start_virt =
                    kzalloc(cap_mblock[id].size,
                        GFP_KERNEL);
                cap_mblock[id].start_phys =
                    virt_to_phys(
                        cap_mblock[id].start_virt);
                cap_mblock[id].start_dma = 0;
            }
            if (hcp_dbg_enable()) {
                pr_debug(
                    "%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                    __func__, id,
                    isp7s_get_reserve_mem_phys(id, cap_mode),
                    isp7s_get_reserve_mem_virt(id, cap_mode),
                    isp7s_get_reserve_mem_dma(id, cap_mode),
                    isp7s_get_reserve_mem_size(id, cap_mode),
                    cap_mblock[id].is_dma_buf,
                    isp7s_get_reserve_mem_fd(id, cap_mode),
                    cap_mblock[id].d_buf);
            }
        }
        return 0;
}

static int isp7s_module_driver_allocate_working_buffer_smvr(struct mtk_hcp *hcp_dev,
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
                            dma_buf_attach(smvr_mblock[id].d_buf, hcp_dev->dev);
                        attach = smvr_mblock[id].attach;
                        if (IS_ERR(attach)) {
                            pr_info("dma_buf_attach fail :%ld\n",
                            PTR_ERR(attach));
                            return -1;
                        }

                        smvr_mblock[id].sgt =
                        dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
                        sgt = smvr_mblock[id].sgt;
                        if (IS_ERR(sgt)) {
                                dma_buf_detach(smvr_mblock[id].d_buf, attach);
                            pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                            PTR_ERR(sgt));
                            return -1;
                        }
                        smvr_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                        smvr_mblock[id].start_dma = smvr_mblock[id].start_phys;
                        ret = dma_buf_vmap(smvr_mblock[id].d_buf, &map);
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
                            pr_debug("%s:[HCP_GCE][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                                __func__, smvr_mblock[id].name,
                                isp7s_get_reserve_mem_phys(id, smvr_mode),
                                isp7s_get_reserve_mem_virt(id, smvr_mode),
                                isp7s_get_reserve_mem_dma(id, smvr_mode),
                                isp7s_get_reserve_mem_size(id, smvr_mode),
                                smvr_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, smvr_mode),
                                smvr_mblock[id].d_buf);
                        }
                        break;
                case WPE_MEM_C_ID:
                case WPE_MEM_T_ID:
                case DIP_MEM_C_ID:
                case DIP_MEM_T_ID:
                case TRAW_MEM_C_ID:
                case TRAW_MEM_T_ID:
                case PQDIP_MEM_C_ID:
                case PQDIP_MEM_T_ID:
                case ADL_MEM_C_ID:
                case ADL_MEM_T_ID:
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
                    smvr_mblock[id].d_buf, hcp_dev->dev);
                    attach = smvr_mblock[id].attach;
                    if (IS_ERR(attach)) {
                        pr_info("dma_buf_attach fail :%ld\n",
                        PTR_ERR(attach));
                        return -1;
                    }

                    smvr_mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
                    sgt = smvr_mblock[id].sgt;
                    if (IS_ERR(sgt)) {
                        dma_buf_detach(smvr_mblock[id].d_buf, attach);
                        pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                        PTR_ERR(sgt));
                        return -1;
                    }
                    smvr_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                    smvr_mblock[id].start_dma = smvr_mblock[id].start_phys;
                    ret = dma_buf_vmap(smvr_mblock[id].d_buf, &map);
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
                        pr_debug("%s:[HCP_WORKING][%s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx,size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                            __func__, smvr_mblock[id].name, id,
                                isp7s_get_reserve_mem_phys(id, smvr_mode),
                                isp7s_get_reserve_mem_virt(id, smvr_mode),
                                isp7s_get_reserve_mem_dma(id, smvr_mode),
                                isp7s_get_reserve_mem_size(id, smvr_mode),
                                smvr_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, smvr_mode),
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
                    smvr_mblock[id].d_buf, hcp_dev->dev);
                    attach = smvr_mblock[id].attach;
                    if (IS_ERR(attach)) {
                        pr_info("dma_buf_attach fail :%ld\n",
                        PTR_ERR(attach));
                        return -1;
                    }

                    smvr_mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
                    sgt = smvr_mblock[id].sgt;
                    if (IS_ERR(sgt)) {
                        dma_buf_detach(smvr_mblock[id].d_buf, attach);
                        pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                        PTR_ERR(sgt));
                        return -1;
                    }
                    smvr_mblock[id].start_phys = sg_dma_address(sgt->sgl);
                    smvr_mblock[id].start_dma = smvr_mblock[id].start_phys;
                    ret = dma_buf_vmap(smvr_mblock[id].d_buf, &map);
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
                        pr_debug("%s:[HCP_WORKING_DEFAULT][%s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                            __func__, smvr_mblock[id].name, id,
                                isp7s_get_reserve_mem_phys(id, smvr_mode),
                                isp7s_get_reserve_mem_virt(id, smvr_mode),
                                isp7s_get_reserve_mem_dma(id, smvr_mode),
                                isp7s_get_reserve_mem_size(id, smvr_mode),
                                smvr_mblock[id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(id, smvr_mode),
                                smvr_mblock[id].d_buf);
                    }
                    break;
                }
            } else {
                smvr_mblock[id].start_virt =
                    kzalloc(smvr_mblock[id].size,
                        GFP_KERNEL);
                smvr_mblock[id].start_phys =
                    virt_to_phys(
                        smvr_mblock[id].start_virt);
                smvr_mblock[id].start_dma = 0;
            }
            if (hcp_dbg_enable()) {
                pr_debug(
                    "%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                    __func__, id,
                    isp7s_get_reserve_mem_phys(id, smvr_mode),
                    isp7s_get_reserve_mem_virt(id, smvr_mode),
                    isp7s_get_reserve_mem_dma(id, smvr_mode),
                    isp7s_get_reserve_mem_size(id, smvr_mode),
                    smvr_mblock[id].is_dma_buf,
                    isp7s_get_reserve_mem_fd(id, smvr_mode),
                    smvr_mblock[id].d_buf);
            }
        }
        return 0;
}

int isp7s_allocate_gce_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
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
                            dma_buf_attach(gmblock[g_id].d_buf, hcp_dev->dev);
                        attach = gmblock[g_id].attach;
                        if (IS_ERR(attach)) {
                            pr_info("dma_buf_attach fail :%ld\n",
                            PTR_ERR(attach));
                            return -1;
                        }

                        gmblock[g_id].sgt =
                        dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
                            sgt = gmblock[g_id].sgt;
                        if (IS_ERR(sgt)) {
                                dma_buf_detach(gmblock[g_id].d_buf, attach);
                            pr_info("dma_buf_map_attachment fail sgt:%ld\n",
                            PTR_ERR(sgt));
                            return -1;
                        }
                        gmblock[g_id].start_phys = sg_dma_address(sgt->sgl);
                        gmblock[g_id].start_dma = gmblock[g_id].start_phys;
                        ret = dma_buf_vmap(gmblock[g_id].d_buf, &map);
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
                            pr_debug("%s:[HCP_GCE_TOKEN][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                                __func__, gmblock[g_id].name,
                                isp7s_get_reserve_mem_phys(g_id + IMG_MEM_FOR_HW_ID, mode),
                                isp7s_get_reserve_mem_virt(g_id + IMG_MEM_FOR_HW_ID, mode),
                                isp7s_get_reserve_mem_dma(g_id + IMG_MEM_FOR_HW_ID, mode),
                                isp7s_get_reserve_mem_size(g_id + IMG_MEM_FOR_HW_ID, mode),
                                gmblock[g_id].is_dma_buf,
                                isp7s_get_reserve_mem_fd(g_id + IMG_MEM_FOR_HW_ID, mode),
                                gmblock[g_id].d_buf);
                        }
                        break;
                    default:
                        break;
                }
            } else {
                gmblock[g_id].start_virt =
                    kzalloc(gmblock[g_id].size,
                        GFP_KERNEL);
                gmblock[g_id].start_phys =
                    virt_to_phys(
                        gmblock[g_id].start_virt);
                gmblock[g_id].start_dma = 0;
            }
            if (hcp_dbg_enable()) {
                pr_debug(
                    "%s: [HCP_GCE_TOKEN][gce_mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
                    __func__, g_id,
                    isp7s_get_reserve_mem_phys(g_id + IMG_MEM_FOR_HW_ID, mode),
                    isp7s_get_reserve_mem_virt(g_id + IMG_MEM_FOR_HW_ID, mode),
                    isp7s_get_reserve_mem_dma(g_id + IMG_MEM_FOR_HW_ID, mode),
                    isp7s_get_reserve_mem_size(g_id + IMG_MEM_FOR_HW_ID, mode),
                    gmblock[g_id].is_dma_buf,
                    isp7s_get_reserve_mem_fd(g_id + IMG_MEM_FOR_HW_ID, mode),
                    gmblock[g_id].d_buf);
            }
        }
        return 0;
}

int isp7s_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode, unsigned int gmb_en)
{
        //enum isp7s_rsv_mem_id_t id = 0;
        //enum isp7s_rsv_gce_mem_id_t g_id = 0;
        //unsigned int id = 0;
        struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
        struct mtk_hcp_smvr_reserve_mblock *smblock = NULL;
        struct mtk_hcp_capture_reserve_mblock *cmblock = NULL;
        //struct mtk_hcp_gce_token_reserve_mblock *gmblock = NULL;
        //unsigned int block_num = 0;
        //int ret = 0;

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
            isp7s_allocate_gce_working_buffer(hcp_dev, mode);
        }

        //block_num = hcp_dev->data->block_num;


        if (mode == imgsys_streaming) {
            pr_info("mtk_hcp: allocate streaming buffer\n");
            isp7s_module_driver_allocate_working_buffer_streaming(hcp_dev, mode, mblock);
        } else if (mode == imgsys_capture) {
            pr_info("mtk_hcp: allocate capture buffer\n");
            isp7s_module_driver_allocate_working_buffer_capture(hcp_dev, mode, cmblock);
        } else {
            pr_info("mtk_hcp: allocate smvr buffer\n");
            isp7s_module_driver_allocate_working_buffer_smvr(hcp_dev, mode, smblock);
        }

        return 0;
}

EXPORT_SYMBOL(isp7s_allocate_working_buffer);

static void gce_release_streaming(struct kref *ref)
{
	struct mtk_hcp_streaming_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_streaming_reserve_mblock, kref);

	dma_buf_vunmap(mblock->d_buf, &mblock->map);
	/* free iova */
	dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mblock->d_buf, mblock->attach);
	dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
	dma_buf_put(mblock->d_buf);
    if (hcp_dbg_enable()) {
	    pr_info("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    		__func__, mblock->name,
    		#if SMVR_DECOUPLE
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming),
    		#else
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID),
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

static void gce_release_capture(struct kref *ref)
{
	struct mtk_hcp_capture_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_capture_reserve_mblock, kref);

	dma_buf_vunmap(mblock->d_buf, &mblock->map);
	/* free iova */
	dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mblock->d_buf, mblock->attach);
	dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
	dma_buf_put(mblock->d_buf);
    if (hcp_dbg_enable()) {
	    pr_info("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    		__func__, mblock->name,
    		#if SMVR_DECOUPLE
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_capture),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_capture),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_capture),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_capture),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_capture),
    		#else
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID),
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

	dma_buf_vunmap(mblock->d_buf, &mblock->map);
	/* free iova */
	dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mblock->d_buf, mblock->attach);
	dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
	dma_buf_put(mblock->d_buf);
    if (hcp_dbg_enable()) {
	    pr_info("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    		__func__, mblock->name,
    		#if SMVR_DECOUPLE
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_smvr),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_smvr),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_smvr),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_smvr),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_smvr),
    		#else
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID),
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

static void gce_release_token(struct kref *ref)
{
	struct mtk_hcp_gce_token_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_gce_token_reserve_mblock, kref);

	dma_buf_vunmap(mblock->d_buf, &mblock->map);
	/* free iova */
	dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mblock->d_buf, mblock->attach);
	dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
	dma_buf_put(mblock->d_buf);

    if (hcp_dbg_enable()) {
	    pr_info("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    		__func__, mblock->name,
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_TOKEN_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_TOKEN_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_TOKEN_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_TOKEN_ID, imgsys_streaming),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_TOKEN_ID, imgsys_streaming),
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
phys_addr_t isp7s_get_reserve_mem_phys(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %d", id);
		return 0;
	} else {
		return mb[id].start_phys;
	}
}
EXPORT_SYMBOL(isp7s_get_reserve_mem_phys);

void *isp7s_get_reserve_mem_virt(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else
		return mb[id].start_virt;
}
EXPORT_SYMBOL(isp7s_get_reserve_mem_virt);

phys_addr_t isp7s_get_reserve_mem_dma(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		return mb[id].start_dma;
	}
}
EXPORT_SYMBOL(isp7s_get_reserve_mem_dma);

phys_addr_t isp7s_get_reserve_mem_size(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		return mb[id].size;
	}
}
EXPORT_SYMBOL(isp7s_get_reserve_mem_size);

uint32_t isp7s_get_reserve_mem_fd(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else
		return mb[id].fd;
}
EXPORT_SYMBOL(isp7s_get_reserve_mem_fd);

void *isp7s_get_gce_virt(void)
{
	return mb[IMG_MEM_G_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_gce_virt);

void *isp7s_get_wpe_virt(void)
{
	return mb[WPE_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_wpe_virt);

int isp7s_get_wpe_cq_fd(void)
{
	return mb[WPE_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_wpe_cq_fd);

int isp7s_get_wpe_tdr_fd(void)
{
	return mb[WPE_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_wpe_tdr_fd);

void *isp7s_get_dip_virt(void)
{
	return mb[DIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_dip_virt);

int isp7s_get_dip_cq_fd(void)
{
	return mb[DIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_dip_cq_fd);

int isp7s_get_dip_tdr_fd(void)
{
	return mb[DIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_dip_tdr_fd);

void *isp7s_get_traw_virt(void)
{
	return mb[TRAW_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_traw_virt);

int isp7s_get_traw_cq_fd(void)
{
	return mb[TRAW_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_traw_cq_fd);

int isp7s_get_traw_tdr_fd(void)
{
	return mb[TRAW_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_traw_tdr_fd);

void *isp7s_get_pqdip_virt(void)
{
	return mb[PQDIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_pqdip_virt);

int isp7s_get_pqdip_cq_fd(void)
{
	return mb[PQDIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_pqdip_cq_fd);

int isp7s_get_pqdip_tdr_fd(void)
{
	return mb[PQDIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp7s_get_pqdip_tdr_fd);

void *isp7s_get_hwid_virt(void)
{
	return mb[DIP_MEM_FOR_HW_ID].start_virt;
}
EXPORT_SYMBOL(isp7s_get_hwid_virt);


int isp7s_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
{
	enum isp7s_rsv_mem_id_t id = 0;
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
					dma_buf_attach(mblock[id].d_buf, hcp_dev->dev);
				attach = mblock[id].attach;
				if (IS_ERR(attach)) {
					pr_info("dma_buf_attach fail :%ld\n",
					PTR_ERR(attach));
					return -1;
				}

				mblock[id].sgt =
					dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
				sgt = mblock[id].sgt;
				if (IS_ERR(sgt)) {
					dma_buf_detach(mblock[id].d_buf, attach);
					pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					PTR_ERR(sgt));
					return -1;
				}
				mblock[id].start_phys = sg_dma_address(sgt->sgl);
				mblock[id].start_dma = mblock[id].start_phys;
				ret = dma_buf_vmap(mblock[id].d_buf, &map);
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
    				pr_info("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    					__func__, mblock[id].name,
    					isp7s_get_reserve_mem_phys(id),
    					isp7s_get_reserve_mem_virt(id),
    					isp7s_get_reserve_mem_dma(id),
    					isp7s_get_reserve_mem_size(id),
    					mblock[id].is_dma_buf,
    					isp7s_get_reserve_mem_fd(id),
    					mblock[id].d_buf);
                }
				break;
			case WPE_MEM_C_ID:
			case WPE_MEM_T_ID:
			case DIP_MEM_C_ID:
			case DIP_MEM_T_ID:
			case TRAW_MEM_C_ID:
			case TRAW_MEM_T_ID:
			case PQDIP_MEM_C_ID:
			case PQDIP_MEM_T_ID:
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
				mblock[id].d_buf, hcp_dev->dev);
				attach = mblock[id].attach;
				if (IS_ERR(attach)) {
					pr_info("dma_buf_attach fail :%ld\n",
					PTR_ERR(attach));
					return -1;
				}

				mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
				sgt = mblock[id].sgt;
				if (IS_ERR(sgt)) {
					dma_buf_detach(mblock[id].d_buf, attach);
					pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					PTR_ERR(sgt));
					return -1;
				}
				mblock[id].start_phys = sg_dma_address(sgt->sgl);
				mblock[id].start_dma = mblock[id].start_phys;
				ret = dma_buf_vmap(mblock[id].d_buf, &map);
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
				mblock[id].d_buf, hcp_dev->dev);
				attach = mblock[id].attach;
				if (IS_ERR(attach)) {
					pr_info("dma_buf_attach fail :%ld\n",
					PTR_ERR(attach));
					return -1;
				}

				mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
				sgt = mblock[id].sgt;
				if (IS_ERR(sgt)) {
					dma_buf_detach(mblock[id].d_buf, attach);
					pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					PTR_ERR(sgt));
					return -1;
				}
				mblock[id].start_phys = sg_dma_address(sgt->sgl);
				mblock[id].start_dma = mblock[id].start_phys;
				ret = dma_buf_vmap(mblock[id].d_buf, &map);
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
    			"%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    			__func__, id,
    			isp7s_get_reserve_mem_phys(id),
    			isp7s_get_reserve_mem_virt(id),
    			isp7s_get_reserve_mem_dma(id),
    			isp7s_get_reserve_mem_size(id),
    			mblock[id].is_dma_buf,
    			isp7s_get_reserve_mem_fd(id),
    			mblock[id].d_buf);
	    }
	}

	return 0;
}
EXPORT_SYMBOL(isp7s_allocate_working_buffer);

static void gce_release(struct kref *ref)
{
	struct mtk_hcp_reserve_mblock *mblock =
		container_of(ref, struct mtk_hcp_reserve_mblock, kref);

	dma_buf_vunmap(mblock->d_buf, &mblock->map);
	/* free iova */
	dma_buf_unmap_attachment(mblock->attach, mblock->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(mblock->d_buf, mblock->attach);
	dma_buf_end_cpu_access(mblock->d_buf, DMA_BIDIRECTIONAL);
	dma_buf_put(mblock->d_buf);
    if (hcp_dbg_enable()) {
    	pr_debug("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    		__func__, mblock->name,
    		#if SMVR_DECOUPLE
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_streaming),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming),
    		#else
    		isp7s_get_reserve_mem_phys(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_virt(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_dma(IMG_MEM_G_ID),
    		isp7s_get_reserve_mem_size(IMG_MEM_G_ID),
    		mblock->is_dma_buf,
    		isp7s_get_reserve_mem_fd(IMG_MEM_G_ID),
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
static int isp7s_module_driver_release_working_buffer_streaming(struct mtk_hcp *hcp_dev,
    unsigned int str_mode, struct mtk_hcp_streaming_reserve_mblock *str_mblock)
{
        enum isp7s_rsv_mem_id_t id;
        unsigned int block_num;
        block_num = hcp_dev->data->block_num;

        /* release reserved memory */
        for (id = 0; id < block_num; id++) {
            if (str_mblock[id].is_dma_buf) {
                switch (id) {
                case IMG_MEM_G_ID:
                    kref_put(&str_mblock[id].kref, gce_release_streaming);
                    break;
                case WPE_MEM_C_ID:
                case WPE_MEM_T_ID:
                case DIP_MEM_C_ID:
                case DIP_MEM_T_ID:
                case TRAW_MEM_C_ID:
                case TRAW_MEM_T_ID:
                case PQDIP_MEM_C_ID:
                case PQDIP_MEM_T_ID:
                default:
                    /* free va */
                    dma_buf_vunmap(str_mblock[id].d_buf, &str_mblock[id].map);
                    /* free iova */
                    dma_buf_unmap_attachment(str_mblock[id].attach,
                    str_mblock[id].sgt, DMA_TO_DEVICE);
                    dma_buf_detach(str_mblock[id].d_buf,
                    str_mblock[id].attach);
                    dma_buf_put(str_mblock[id].d_buf);
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
            } else {
                kfree(str_mblock[id].start_virt);
                str_mblock[id].start_virt = 0x0;
                str_mblock[id].start_phys = 0x0;
                str_mblock[id].start_dma = 0x0;
                str_mblock[id].mmap_cnt = 0;
            }
            if (hcp_dbg_enable()) {
                pr_debug(
                "%s: [HCP][mem_reserve-%s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d\n",
                __func__, str_mblock[id].name, id,
                    isp7s_get_reserve_mem_phys(id, str_mode),
                    isp7s_get_reserve_mem_virt(id, str_mode),
                    isp7s_get_reserve_mem_dma(id, str_mode),
                    isp7s_get_reserve_mem_size(id, str_mode),
                    str_mblock[id].is_dma_buf,
                    isp7s_get_reserve_mem_fd(id, str_mode));
            }
        }

        return 0;
}

static int isp7s_module_driver_release_working_buffer_capture(struct mtk_hcp *hcp_dev,
    unsigned int cap_mode, struct mtk_hcp_capture_reserve_mblock *cap_mblock)
{
        enum isp7s_rsv_mem_id_t id;
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
                case DIP_MEM_C_ID:
                case DIP_MEM_T_ID:
                case TRAW_MEM_C_ID:
                case TRAW_MEM_T_ID:
                case PQDIP_MEM_C_ID:
                case PQDIP_MEM_T_ID:
                default:
                    /* free va */
                    dma_buf_vunmap(cap_mblock[id].d_buf, &cap_mblock[id].map);
                    /* free iova */
                    dma_buf_unmap_attachment(cap_mblock[id].attach,
                    cap_mblock[id].sgt, DMA_TO_DEVICE);
                    dma_buf_detach(cap_mblock[id].d_buf,
                    cap_mblock[id].attach);
                    dma_buf_put(cap_mblock[id].d_buf);
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
                "%s: [HCP][mem_reserve-%s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d\n",
                __func__, cap_mblock[id].name,id,
                    isp7s_get_reserve_mem_phys(id, cap_mode),
                    isp7s_get_reserve_mem_virt(id, cap_mode),
                    isp7s_get_reserve_mem_dma(id, cap_mode),
                    isp7s_get_reserve_mem_size(id, cap_mode),
                    cap_mblock[id].is_dma_buf,
                    isp7s_get_reserve_mem_fd(id, cap_mode));
            }
        }

        return 0;
}

static int isp7s_module_driver_release_working_buffer_smvr(struct mtk_hcp *hcp_dev,
    unsigned int smvr_mode, struct mtk_hcp_smvr_reserve_mblock *smvr_mblock)
{
        enum isp7s_rsv_mem_id_t id;
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
                case DIP_MEM_C_ID:
                case DIP_MEM_T_ID:
                case TRAW_MEM_C_ID:
                case TRAW_MEM_T_ID:
                case PQDIP_MEM_C_ID:
                case PQDIP_MEM_T_ID:
                default:
                    /* free va */
                    dma_buf_vunmap(smvr_mblock[id].d_buf, &smvr_mblock[id].map);
                    /* free iova */
                    dma_buf_unmap_attachment(smvr_mblock[id].attach,
                    smvr_mblock[id].sgt, DMA_TO_DEVICE);
                    dma_buf_detach(smvr_mblock[id].d_buf,
                    smvr_mblock[id].attach);
                    dma_buf_put(smvr_mblock[id].d_buf);
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
                "%s: [HCP][mem_reserve- %s(%d)] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d\n",
                __func__, smvr_mblock[id].name,id,
                    isp7s_get_reserve_mem_phys(id, smvr_mode),
                    isp7s_get_reserve_mem_virt(id, smvr_mode),
                    isp7s_get_reserve_mem_dma(id, smvr_mode),
                    isp7s_get_reserve_mem_size(id, smvr_mode),
                    smvr_mblock[id].is_dma_buf,
                    isp7s_get_reserve_mem_fd(id, smvr_mode));
            }
        }

        return 0;
}

int isp7s_release_gce_working_buffer(struct mtk_hcp *hcp_dev)
{
    enum isp7s_rsv_gce_mem_id_t gid;
    struct mtk_hcp_gce_token_reserve_mblock *gmblock = NULL;
    unsigned int block_num_gce;

    gmblock = gmb;
    block_num_gce = hcp_dev->data->block_num_gce;
    /* release gce reserved memory */
	for (gid = 0; gid < block_num_gce; gid++) {
        switch (gid + IMG_MEM_FOR_HW_ID) {
			case IMG_MEM_FOR_HW_ID:
				/*allocated at probe via dts*/
				break;
			case IMG_MEM_G_TOKEN_ID:
				kref_put(&gmblock[gid].kref, gce_release_token);
				break;
			default:
				break;
        }
    }

    return 0;
}
EXPORT_SYMBOL(isp7s_release_gce_working_buffer);

int isp7s_release_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
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

        if (mode == imgsys_streaming) {
            isp7s_module_driver_release_working_buffer_streaming(hcp_dev, mode, mblock);
        } else if (mode == imgsys_capture) {
            isp7s_module_driver_release_working_buffer_capture(hcp_dev, mode, cmblock);
        } else {
            isp7s_module_driver_release_working_buffer_smvr(hcp_dev, mode, smblock);
        }

        return 0;
}

EXPORT_SYMBOL(isp7s_release_working_buffer);

int isp7s_get_init_info(struct img_init_info *info)
{
	if (!info) {
		pr_info("%s:NULL info\n", __func__);
		return -1;
	}
    if (!info->is_capture) {
        if (!info->smvr_mode) {
        /*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
       	info->module_info_streaming[0].c_wbuf =
       				isp7s_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf =
       				isp7s_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_streaming);

         // ADL
       	info->module_info_streaming[1].c_wbuf =
       				isp7s_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf =
       				isp7s_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_streaming);

       	// TRAW
       	info->module_info_streaming[2].c_wbuf =
       				isp7s_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf =
       				isp7s_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_streaming);

       		// DIP
       	info->module_info_streaming[3].c_wbuf =
       				isp7s_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf =
       				isp7s_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_streaming);

       	// PQDIP
       	info->module_info_streaming[4].c_wbuf =
       				isp7s_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].c_wbuf_fd =
       			    isp7s_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf =
       				isp7s_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_streaming);

        // GCE
        info->gce_info[imgsys_streaming].g_wbuf_fd = isp7s_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming);
        info->gce_info[imgsys_streaming].g_wbuf = isp7s_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming);
        info->gce_info[imgsys_streaming].g_wbuf_sz = isp7s_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming);
        } else {
	/*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
       	info->module_info_smvr[0].c_wbuf =
       				isp7s_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf =
       				isp7s_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_smvr);

         // ADL
       	info->module_info_smvr[1].c_wbuf =
       				isp7s_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf =
       				isp7s_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_smvr);

       	// TRAW
       	info->module_info_smvr[2].c_wbuf =
       				isp7s_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf =
       				isp7s_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_smvr);

       		// DIP
       	info->module_info_smvr[3].c_wbuf =
       				isp7s_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].c_wbuf_fd =
       				isp7s_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf =
       				isp7s_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_smvr);

       	// PQDIP
       	info->module_info_smvr[4].c_wbuf =
       				isp7s_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].c_wbuf_dma =
       				isp7s_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].c_wbuf_sz =
       				isp7s_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].c_wbuf_fd =
       			    isp7s_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf =
       				isp7s_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf_dma =
       				isp7s_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf_sz =
       				isp7s_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf_fd =
       				isp7s_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_smvr);

        // GCE
        info->gce_info[imgsys_smvr].g_wbuf_fd = isp7s_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_smvr);
        info->gce_info[imgsys_smvr].g_wbuf = isp7s_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_smvr);
        info->gce_info[imgsys_smvr].g_wbuf_sz = isp7s_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_smvr);
        }

    } else {
	/*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
       	info->module_info_capture[0].c_wbuf =
				isp7s_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].c_wbuf_sz =
				isp7s_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf =
				isp7s_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf_sz =
				isp7s_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_capture);

  // ADL
       	info->module_info_capture[1].c_wbuf =
				isp7s_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].c_wbuf_sz =
				isp7s_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf =
				isp7s_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf_sz =
				isp7s_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_capture);

	// TRAW
       	info->module_info_capture[2].c_wbuf =
				isp7s_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].c_wbuf_sz =
				isp7s_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf =
				isp7s_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf_sz =
				isp7s_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_capture);

		// DIP
       	info->module_info_capture[3].c_wbuf =
				isp7s_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].c_wbuf_sz =
				isp7s_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf =
				isp7s_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf_sz =
				isp7s_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_capture);

	// PQDIP
       	info->module_info_capture[4].c_wbuf =
				isp7s_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].c_wbuf_sz =
				isp7s_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].c_wbuf_fd =
			isp7s_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf =
				isp7s_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf_sz =
				isp7s_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_capture);

        // GCE
        info->gce_info[imgsys_capture].g_wbuf_fd = isp7s_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_capture);
        info->gce_info[imgsys_capture].g_wbuf = isp7s_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_capture);
        info->gce_info[imgsys_capture].g_wbuf_sz = isp7s_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_capture);
    }
	info->hw_buf = isp7s_get_reserve_mem_phys(DIP_MEM_FOR_HW_ID, imgsys_streaming);

	/*common*/
    info->g_token_wbuf_fd = isp7s_get_reserve_mem_fd(IMG_MEM_G_TOKEN_ID, imgsys_streaming);
	info->g_token_wbuf = isp7s_get_reserve_mem_phys(IMG_MEM_G_TOKEN_ID, imgsys_streaming);
	/*info->g_wbuf_sw = isp7s_get_reserve_mem_virt(IMG_MEM_G_ID);*/
	info->g_token_wbuf_sz = isp7s_get_reserve_mem_size(IMG_MEM_G_TOKEN_ID, imgsys_streaming);


	return 0;
}

int isp7s_get_mem_info(struct img_init_info *info)
{
    return 0;
}
static int isp7s_put_gce(unsigned int mode)
{
    if (mode == imgsys_streaming)
		kref_put(&mb[IMG_MEM_G_ID].kref, gce_release_streaming);
	else if(mode == imgsys_capture)
		kref_put(&cmb[IMG_MEM_G_ID].kref, gce_release_capture);
	else
		kref_put(&smb[IMG_MEM_G_ID].kref, gce_release_smvr);

	return 0;
}

static int isp7s_get_gce(unsigned int mode)
{
    if (mode == imgsys_streaming)
		kref_get(&mb[IMG_MEM_G_ID].kref);
	else if(mode == imgsys_capture)
		kref_get(&cmb[IMG_MEM_G_ID].kref);
	else
		kref_get(&smb[IMG_MEM_G_ID].kref);
	//kref_get(&gmb[IMG_MEM_G_ID - IMG_MEM_FOR_HW_ID].kref);
	return 0;
}

int isp7s_partial_flush(struct mtk_hcp *hcp_dev, struct flush_buf_info *b_info)
{
    struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
    struct mtk_hcp_smvr_reserve_mblock *smblock = NULL;
    struct mtk_hcp_capture_reserve_mblock *cmblock = NULL;
    unsigned int block_num = 0;
    unsigned int id = 0;
    unsigned int mode = 0;
    //pr_info("imgsys_fw no partial flush");
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

    return 0;
}

struct mtk_hcp_data isp7s_hcp_data = {
	.mblock = isp7s_streaming_mblock,
	.smblock = isp7s_smvr_mblock,
	.cmblock = isp7s_capture_mblock,
	.gmblock = isp7s_gce_mblock,
	//.gsmblock = isp7s_gce_smvr_mblock,
	.block_num = ARRAY_SIZE(isp7s_streaming_mblock),
	.block_num_gce = ARRAY_SIZE(isp7s_gce_mblock),
	.allocate = isp7s_allocate_working_buffer,
	.release = isp7s_release_working_buffer,
	.release_gce_buf = isp7s_release_gce_working_buffer,
	.get_init_info = isp7s_get_init_info,
	.get_mem_info = isp7s_get_mem_info,
	.get_gce_virt = isp7s_get_gce_virt,
	.get_gce = isp7s_get_gce,
	.put_gce = isp7s_put_gce,
	.get_gce_token_virt = isp7s_get_gce_token_virt,
	.get_hwid_virt = isp7s_get_hwid_virt,
	.get_wpe_virt = isp7s_get_wpe_virt,
	.get_wpe_cq_fd = isp7s_get_wpe_cq_fd,
	.get_wpe_tdr_fd = isp7s_get_wpe_tdr_fd,
	.get_dip_virt = isp7s_get_dip_virt,
	.get_dip_cq_fd = isp7s_get_dip_cq_fd,
	.get_dip_tdr_fd = isp7s_get_dip_tdr_fd,
	.get_traw_virt = isp7s_get_traw_virt,
	.get_traw_cq_fd = isp7s_get_traw_cq_fd,
	.get_traw_tdr_fd = isp7s_get_traw_tdr_fd,
	.get_pqdip_virt = isp7s_get_pqdip_virt,
	.get_pqdip_cq_fd = isp7s_get_pqdip_cq_fd,
	.get_pqdip_tdr_fd = isp7s_get_pqdip_tdr_fd,
	.partial_flush = NULL,
};
#else
int isp7s_release_working_buffer(struct mtk_hcp *hcp_dev)
{
	enum isp7s_rsv_mem_id_t id;
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
				/* free va */
				dma_buf_vunmap(mblock[id].d_buf, &mblock[id].map);
				/* free iova */
				dma_buf_unmap_attachment(mblock[id].attach,
				mblock[id].sgt, DMA_TO_DEVICE);
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
			isp7s_get_reserve_mem_phys(id),
			isp7s_get_reserve_mem_virt(id),
			isp7s_get_reserve_mem_dma(id),
			isp7s_get_reserve_mem_size(id),
			mblock[id].is_dma_buf,
			isp7s_get_reserve_mem_fd(id));
	}
	}

	return 0;
}
EXPORT_SYMBOL(isp7s_release_working_buffer);

int isp7s_get_init_info(struct img_init_info *info)
{

	if (!info) {
		pr_info("%s:NULL info\n", __func__);
		return -1;
	}

	info->hw_buf = isp7s_get_reserve_mem_phys(DIP_MEM_FOR_HW_ID);
	/*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
	info->module_info[0].c_wbuf =
				isp7s_get_reserve_mem_phys(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_sz =
				isp7s_get_reserve_mem_size(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(WPE_MEM_C_ID);
	info->module_info[0].t_wbuf =
				isp7s_get_reserve_mem_phys(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_sz =
				isp7s_get_reserve_mem_size(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(WPE_MEM_T_ID);

  // ADL
	info->module_info[1].c_wbuf =
				isp7s_get_reserve_mem_phys(ADL_MEM_C_ID);
	info->module_info[1].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(ADL_MEM_C_ID);
	info->module_info[1].c_wbuf_sz =
				isp7s_get_reserve_mem_size(ADL_MEM_C_ID);
	info->module_info[1].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(ADL_MEM_C_ID);
	info->module_info[1].t_wbuf =
				isp7s_get_reserve_mem_phys(ADL_MEM_T_ID);
	info->module_info[1].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(ADL_MEM_T_ID);
	info->module_info[1].t_wbuf_sz =
				isp7s_get_reserve_mem_size(ADL_MEM_T_ID);
	info->module_info[1].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(ADL_MEM_T_ID);

	// TRAW
	info->module_info[2].c_wbuf =
				isp7s_get_reserve_mem_phys(TRAW_MEM_C_ID);
	info->module_info[2].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(TRAW_MEM_C_ID);
	info->module_info[2].c_wbuf_sz =
				isp7s_get_reserve_mem_size(TRAW_MEM_C_ID);
	info->module_info[2].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(TRAW_MEM_C_ID);
	info->module_info[2].t_wbuf =
				isp7s_get_reserve_mem_phys(TRAW_MEM_T_ID);
	info->module_info[2].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(TRAW_MEM_T_ID);
	info->module_info[2].t_wbuf_sz =
				isp7s_get_reserve_mem_size(TRAW_MEM_T_ID);
	info->module_info[2].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(TRAW_MEM_T_ID);

		// DIP
	info->module_info[3].c_wbuf =
				isp7s_get_reserve_mem_phys(DIP_MEM_C_ID);
	info->module_info[3].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(DIP_MEM_C_ID);
	info->module_info[3].c_wbuf_sz =
				isp7s_get_reserve_mem_size(DIP_MEM_C_ID);
	info->module_info[3].c_wbuf_fd =
				isp7s_get_reserve_mem_fd(DIP_MEM_C_ID);
	info->module_info[3].t_wbuf =
				isp7s_get_reserve_mem_phys(DIP_MEM_T_ID);
	info->module_info[3].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(DIP_MEM_T_ID);
	info->module_info[3].t_wbuf_sz =
				isp7s_get_reserve_mem_size(DIP_MEM_T_ID);
	info->module_info[3].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(DIP_MEM_T_ID);

	// PQDIP
	info->module_info[4].c_wbuf =
				isp7s_get_reserve_mem_phys(PQDIP_MEM_C_ID);
	info->module_info[4].c_wbuf_dma =
				isp7s_get_reserve_mem_dma(PQDIP_MEM_C_ID);
	info->module_info[4].c_wbuf_sz =
				isp7s_get_reserve_mem_size(PQDIP_MEM_C_ID);
	info->module_info[4].c_wbuf_fd =
			isp7s_get_reserve_mem_fd(PQDIP_MEM_C_ID);
	info->module_info[4].t_wbuf =
				isp7s_get_reserve_mem_phys(PQDIP_MEM_T_ID);
	info->module_info[4].t_wbuf_dma =
				isp7s_get_reserve_mem_dma(PQDIP_MEM_T_ID);
	info->module_info[4].t_wbuf_sz =
				isp7s_get_reserve_mem_size(PQDIP_MEM_T_ID);
	info->module_info[4].t_wbuf_fd =
				isp7s_get_reserve_mem_fd(PQDIP_MEM_T_ID);

	/*common*/
	/* info->g_wbuf_fd = isp7s_get_reserve_mem_fd(IMG_MEM_G_ID); */
	info->g_wbuf_fd = isp7s_get_reserve_mem_fd(IMG_MEM_G_ID);
	info->g_wbuf = isp7s_get_reserve_mem_phys(IMG_MEM_G_ID);
	/*info->g_wbuf_sw = isp7s_get_reserve_mem_virt(IMG_MEM_G_ID);*/
	info->g_wbuf_sz = isp7s_get_reserve_mem_size(IMG_MEM_G_ID);

	return 0;
}

static int isp7s_put_gce(void)
{
	kref_put(&mb[IMG_MEM_G_ID].kref, gce_release);
	return 0;
}

static int isp7s_get_gce(void)
{
	kref_get(&mb[IMG_MEM_G_ID].kref);
	return 0;
}

int isp7s_partial_flush(struct mtk_hcp *hcp_dev, struct flush_buf_info *b_info)
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
struct mtk_hcp_data isp7s_hcp_data = {
	.mblock = isp7s_reserve_mblock,
	.block_num = ARRAY_SIZE(isp7s_reserve_mblock),
	.smblock = isp7s_smvr_mblock,
	.allocate = isp7s_allocate_working_buffer,
	.release = isp7s_release_working_buffer,
	.get_init_info = isp7s_get_init_info,
	.get_gce_virt = isp7s_get_gce_virt,
	.get_gce = isp7s_get_gce,
	.put_gce = isp7s_put_gce,
	.get_hwid_virt = isp7s_get_hwid_virt,
	.get_wpe_virt = isp7s_get_wpe_virt,
	.get_wpe_cq_fd = isp7s_get_wpe_cq_fd,
	.get_wpe_tdr_fd = isp7s_get_wpe_tdr_fd,
	.get_dip_virt = isp7s_get_dip_virt,
	.get_dip_cq_fd = isp7s_get_dip_cq_fd,
	.get_dip_tdr_fd = isp7s_get_dip_tdr_fd,
	.get_traw_virt = isp7s_get_traw_virt,
	.get_traw_cq_fd = isp7s_get_traw_cq_fd,
	.get_traw_tdr_fd = isp7s_get_traw_tdr_fd,
	.get_pqdip_virt = isp7s_get_pqdip_virt,
	.get_pqdip_cq_fd = isp7s_get_pqdip_cq_fd,
	.get_pqdip_tdr_fd = isp7s_get_pqdip_tdr_fd,
	.partial_flush = NULL,
};
#endif
MODULE_IMPORT_NS(DMA_BUF);
