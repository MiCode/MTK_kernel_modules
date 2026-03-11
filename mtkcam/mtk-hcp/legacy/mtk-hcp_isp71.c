// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/slab.h>
#include <linux/kref.h>
#include <mtk_heap.h>
#include "mtk-hcp_isp71.h"


#if SMVR_DECOUPLE
static struct mtk_hcp_streaming_reserve_mblock *mb;
static struct mtk_hcp_smvr_reserve_mblock *smb;
static struct mtk_hcp_capture_reserve_mblock *cmb;
static struct mtk_hcp_gce_token_reserve_mblock *gmb;
#else
static struct mtk_hcp_reserve_mblock *mb;
#endif

#if !SMVR_DECOUPLE
enum isp71_rsv_mem_id_t {
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
enum isp71_rsv_mem_id_t {
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

enum isp71_rsv_gce_mem_id_t {
	DIP_MEM_FOR_HW_ID,
	IMG_MEM_FOR_HW_ID = DIP_MEM_FOR_HW_ID, /*shared buffer for ipi_param*/
	IMG_MEM_G_TOKEN_ID,
	NUMS_CM_MEM_ID,
};
#endif

#if !SMVR_DECOUPLE
static struct mtk_hcp_reserve_mblock isp71_smvr_mblock[] = {
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
		.size = 0x300000,
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
		.size = 0x500000,
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
		.size = 0xB00000,
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
		.size = 0x3000000,
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
		.size = 0xF00000,
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
		.size = 0x3000000,
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
		.size = 0x200000,
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
		.size = 0x200000,
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
		.size = 0x3200000,
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


struct mtk_hcp_reserve_mblock isp71_reserve_mblock[] = {
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
		.size = 0xE1000,   /*900KB*/
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
		.size = 0x196000,   /*1MB + 600KB*/
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
		.size = 0x4C8000,   /*4MB + 800KB*/
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
		.size = 0x1CC8000,   /*28MB + 800KB*/
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
		.size = 0x5C8000,   /*5MB + 800KB*/
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
		.size = 0x1FAF000,   /*31MB + 700KB*/
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
		.size = 0x100000,   /*1MB*/
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
		.size = 0x170000,   /*1MB + 500KB*/
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
		.size = 0x2000000,
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
static struct mtk_hcp_smvr_reserve_mblock isp71_smvr_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0x300000,
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
		.size = 0x500000,
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
		.size = 0xB00000,
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
		.size = 0x3000000,
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
		.size = 0xF00000,
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
		.size = 0x3000000,
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
		.size = 0x200000,
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
		.size = 0x200000,
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
};


struct mtk_hcp_streaming_reserve_mblock isp71_streaming_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xE1000,   /*900KB*/
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
		.size = 0x196000,   /*1MB + 600KB*/
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
		.size = 0x4C8000,   /*4MB + 800KB*/
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
		.size = 0x1CC8000,   /*28MB + 800KB*/
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
		.size = 0x5C8000,   /*5MB + 800KB*/
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
		.size = 0x1FAF000,   /*31MB + 700KB*/
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
		.size = 0x3200000,
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
		.size = 0x170000,   /*1MB + 500KB*/
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
};

struct mtk_hcp_capture_reserve_mblock isp71_capture_mblock[] = {
	{
		.name = "WPE_MEM_C_ID",
		.num = WPE_MEM_C_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.start_dma  = 0x0,
		.size = 0xE1000,   /*900KB*/
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
		.size = 0x196000,   /*1MB + 600KB*/
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
		.size = 0x4C8000,   /*4MB + 800KB*/
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
		.size = 0x1CC8000,   /*28MB + 800KB*/
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
		.size = 0x5C8000,   /*5MB + 800KB*/
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
		.size = 0x1FAF000,   /*31MB + 700KB*/
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
		.size = 0x100000,   /*1MB*/
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
		.size = 0x170000,   /*1MB + 500KB*/
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
};

static struct mtk_hcp_gce_token_reserve_mblock isp71_gce_mblock[] = {
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
		.size = 0xF98400,//0xF68400, //0x2000000,
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

#if 0
static struct mtk_hcp_reserve_mblock isp71_gce_smvr_mblock[] = {
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
		.size = 0x2000000,
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
phys_addr_t isp71_get_reserve_mem_phys(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_WB_MEM_ID + NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %d", id);
		return 0;
	} else {
	    if ((id == IMG_MEM_G_ID) || (id == IMG_MEM_FOR_HW_ID)) {
            return gmb[id].start_phys;
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
EXPORT_SYMBOL(isp71_get_reserve_mem_phys);

void *isp71_get_reserve_mem_virt(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_WB_MEM_ID + NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
	    if ((id == IMG_MEM_G_ID) || (id == IMG_MEM_FOR_HW_ID)) {
            return gmb[id].start_virt;
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
EXPORT_SYMBOL(isp71_get_reserve_mem_virt);

phys_addr_t isp71_get_reserve_mem_dma(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_WB_MEM_ID + NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
	if ((id == IMG_MEM_G_ID) || (id == IMG_MEM_FOR_HW_ID)) {
            return gmb[id].start_dma;
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
EXPORT_SYMBOL(isp71_get_reserve_mem_dma);

phys_addr_t isp71_get_reserve_mem_size(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_WB_MEM_ID + NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
	if ((id == IMG_MEM_G_ID) || (id == IMG_MEM_FOR_HW_ID)) {
            return gmb[id].size;
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
EXPORT_SYMBOL(isp71_get_reserve_mem_size);

uint32_t isp71_get_reserve_mem_fd(unsigned int id, unsigned int mode)
{
	if (id >= (NUMS_WB_MEM_ID + NUMS_CM_MEM_ID)) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
	if ((id == IMG_MEM_G_ID) || (id == IMG_MEM_FOR_HW_ID)) {
            return gmb[id].fd;
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
EXPORT_SYMBOL(isp71_get_reserve_mem_fd);

void *isp71_get_gce_virt(unsigned int mode)
{
	return gmb[IMG_MEM_G_ID].start_virt;
    #if 0
	if (mode == imgsys_streaming)
	return mb[IMG_MEM_G_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[IMG_MEM_G_ID].start_virt;
	else
		return smb[IMG_MEM_G_ID].start_virt;
    #endif
}
EXPORT_SYMBOL(isp71_get_gce_virt);

void *isp71_get_hwid_virt(unsigned int mode)
{
	return gmb[DIP_MEM_FOR_HW_ID].start_virt;
    #if 0
	if (mode == imgsys_streaming)
	return mb[DIP_MEM_FOR_HW_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_FOR_HW_ID].start_virt;
	else
		return smb[DIP_MEM_FOR_HW_ID].start_virt;
    #endif
}
EXPORT_SYMBOL(isp71_get_hwid_virt);

#else
phys_addr_t isp71_get_reserve_mem_phys(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		return mb[id].start_phys;
	}
}
EXPORT_SYMBOL(isp71_get_reserve_mem_phys);

void *isp71_get_reserve_mem_virt(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else
		return mb[id].start_virt;
}
EXPORT_SYMBOL(isp71_get_reserve_mem_virt);

phys_addr_t isp71_get_reserve_mem_dma(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		return mb[id].start_dma;
	}
}
EXPORT_SYMBOL(isp71_get_reserve_mem_dma);

phys_addr_t isp71_get_reserve_mem_size(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else {
		return mb[id].size;
	}
}
EXPORT_SYMBOL(isp71_get_reserve_mem_size);

uint32_t isp71_get_reserve_mem_fd(unsigned int id)
{
	if (id >= NUMS_MEM_ID) {
		pr_info("[HCP] no reserve memory for %u", id);
		return 0;
	} else
		return mb[id].fd;
}
EXPORT_SYMBOL(isp71_get_reserve_mem_fd);

void *isp71_get_gce_virt(void)
{
	return mb[IMG_MEM_G_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_gce_virt);

void *isp71_get_hwid_virt(void)
{
	return mb[DIP_MEM_FOR_HW_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_hwid_virt);
#endif
#if SMVR_DECOUPLE
int isp71_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode, unsigned int gmb_en)
{
	enum isp71_rsv_mem_id_t id = 0;
	struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
    struct mtk_hcp_smvr_reserve_mblock *smblock = NULL;
    struct mtk_hcp_capture_reserve_mblock *cmblock = NULL;
    struct mtk_hcp_gce_token_reserve_mblock *gmblock = NULL;
	unsigned int block_num = 0;
	unsigned int block_num_gce = 0;
	struct sg_table *sgt = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct dma_heap *pdma_heap = NULL;
	struct iosys_map map = {0};
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

	gmblock = hcp_dev->data->gmblock;
    gmb = gmblock;
	block_num = hcp_dev->data->block_num;
    block_num_gce = hcp_dev->data->block_num_gce;
	for (id = 0; id < block_num_gce; id++) {
		if (gmblock[id].is_dma_buf) {
			switch (id) {
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
					gmblock[id].d_buf = dma_heap_buffer_alloc(
						pdma_heap,
						gmblock[id].size,
						O_RDWR | O_CLOEXEC,
						DMA_HEAP_VALID_HEAP_FLAGS);
					if (IS_ERR(gmblock[id].d_buf)) {
						pr_info("dma_heap_buffer_alloc fail :%ld\n",
						PTR_ERR(gmblock[id].d_buf));
						return -1;
					}
					mtk_dma_buf_set_name(gmblock[id].d_buf, gmblock[id].name);
					gmblock[id].attach =
						dma_buf_attach(gmblock[id].d_buf, hcp_dev->dev);
					attach = gmblock[id].attach;
					if (IS_ERR(attach)) {
						pr_info("dma_buf_attach fail :%ld\n",
						PTR_ERR(attach));
						return -1;
					}

					gmblock[id].sgt =
						dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
					sgt = gmblock[id].sgt;
					if (IS_ERR(sgt)) {
						dma_buf_detach(gmblock[id].d_buf, attach);
						pr_info("dma_buf_map_attachment fail sgt:%ld\n",
						PTR_ERR(sgt));
						return -1;
					}
					gmblock[id].start_phys = sg_dma_address(sgt->sgl);
					gmblock[id].start_dma = gmblock[id].start_phys;
					ret = dma_buf_vmap(gmblock[id].d_buf, &map);
					if (ret) {
						pr_info("sg_dma_address fail\n");
						return ret;
					}
					gmblock[id].start_virt = (void *)map.vaddr;
					gmblock[id].map = map;
					get_dma_buf(gmblock[id].d_buf);
					gmblock[id].fd =
						dma_buf_fd(gmblock[id].d_buf, O_RDWR | O_CLOEXEC);
					dma_buf_begin_cpu_access(gmblock[id].d_buf, DMA_BIDIRECTIONAL);
					kref_init(&gmblock[id].kref);
                    if (hcp_dbg_enable()) {
    					pr_debug("%s:[HCP_GCE][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    						__func__, gmblock[id].name,
    						isp71_get_reserve_mem_phys(id, mode),
    						isp71_get_reserve_mem_virt(id, mode),
    						isp71_get_reserve_mem_dma(id, mode),
    						isp71_get_reserve_mem_size(id, mode),
    						gmblock[id].is_dma_buf,
    						isp71_get_reserve_mem_fd(id, mode),
    						gmblock[id].d_buf);
                    }
					break;
				default:
					break;
			}
		} else {
			gmblock[id].start_virt =
				kzalloc(gmblock[id].size,
					GFP_KERNEL);
			gmblock[id].start_phys =
				virt_to_phys(
					gmblock[id].start_virt);
			gmblock[id].start_dma = 0;
		}
        if (hcp_dbg_enable()) {
    		pr_debug(
    			"%s: [HCP_GCE][gce_mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    			__func__, id,
    			isp71_get_reserve_mem_phys(id, mode),
    			isp71_get_reserve_mem_virt(id, mode),
    			isp71_get_reserve_mem_dma(id, mode),
    			isp71_get_reserve_mem_size(id, mode),
    			gmblock[id].is_dma_buf,
    			isp71_get_reserve_mem_fd(id, mode),
    			gmblock[id].d_buf);
        }
	}

	for (id = 0; id < block_num; id++) {
		if (mblock[id].is_dma_buf) {
			switch (id) {
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
                pr_info("imgsys_fw: alloc buffer mode6");
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
    			isp71_get_reserve_mem_phys(id, mode),
    			isp71_get_reserve_mem_virt(id, mode),
    			isp71_get_reserve_mem_dma(id, mode),
    			isp71_get_reserve_mem_size(id, mode),
    			mblock[id].is_dma_buf,
    			isp71_get_reserve_mem_fd(id, mode),
    			mblock[id].d_buf);
        }
	}

	return 0;
}
EXPORT_SYMBOL(isp71_allocate_working_buffer);
#else
int isp71_allocate_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
{
	enum isp71_rsv_mem_id_t id = 0;
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
    				pr_debug("%s:[HCP][%s] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    					__func__, mblock[id].name,
    					isp71_get_reserve_mem_phys(id),
    					isp71_get_reserve_mem_virt(id),
    					isp71_get_reserve_mem_dma(id),
    					isp71_get_reserve_mem_size(id),
    					mblock[id].is_dma_buf,
    					isp71_get_reserve_mem_fd(id),
    					mblock[id].d_buf);
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
			}
		} else {
			mblock[id].start_virt = kzalloc(mblock[id].size, GFP_KERNEL);
			mblock[id].start_phys = virt_to_phys(mblock[id].start_virt);
			mblock[id].start_dma = 0;
		}
        if (hcp_dbg_enable()) {
    		pr_debug(
    			"%s: [HCP][mem_reserve-%d] phys:0x%llx, virt:0x%p, dma:0x%llx, size:0x%llx, is_dma_buf:%d, fd:%d, d_buf:0x%p\n",
    			__func__, id,
    			isp71_get_reserve_mem_phys(id),
    			isp71_get_reserve_mem_virt(id),
    			isp71_get_reserve_mem_dma(id),
    			isp71_get_reserve_mem_size(id),
    			mblock[id].is_dma_buf,
    			isp71_get_reserve_mem_fd(id),
    			mblock[id].d_buf);
	    }
	}

	return 0;
}
EXPORT_SYMBOL(isp71_allocate_working_buffer);
#endif
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
    		isp71_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming),
    		isp71_get_reserve_mem_virt(IMG_MEM_G_ID, imgsys_streaming),
    		isp71_get_reserve_mem_dma(IMG_MEM_G_ID, imgsys_streaming),
    		isp71_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming),
    		mblock->is_dma_buf,
    		isp71_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming),
    		#else
    		isp71_get_reserve_mem_phys(IMG_MEM_G_ID),
    		isp71_get_reserve_mem_virt(IMG_MEM_G_ID),
    		isp71_get_reserve_mem_dma(IMG_MEM_G_ID),
    		isp71_get_reserve_mem_size(IMG_MEM_G_ID),
    		mblock->is_dma_buf,
    		isp71_get_reserve_mem_fd(IMG_MEM_G_ID),
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

#if SMVR_DECOUPLE
int isp71_release_gce_working_buffer(struct mtk_hcp *hcp_dev)
{
    return 0;
}
int isp71_release_working_buffer(struct mtk_hcp *hcp_dev, unsigned int mode)
{
	enum isp71_rsv_mem_id_t id;
	struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
    struct mtk_hcp_smvr_reserve_mblock *smblock = NULL;
    struct mtk_hcp_capture_reserve_mblock *cmblock = NULL;
    struct mtk_hcp_gce_token_reserve_mblock *gmblock = NULL;
	unsigned int block_num;
	unsigned int block_num_gce;

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

    gmblock = gmb;
	block_num = hcp_dev->data->block_num;
    block_num_gce = hcp_dev->data->block_num_gce;

	 /* release gce reserved memory */
	for (id = 0; id < block_num_gce; id++) {
			switch (id) {
			case IMG_MEM_FOR_HW_ID:
				/*allocated at probe via dts*/
				break;
			case IMG_MEM_G_ID:
				kref_put(&mblock[id].kref, gce_release);
				break;
			default:
				break;
        }
    }

	/* release reserved memory */
	for (id = 0; id < block_num; id++) {
		if (mblock[id].is_dma_buf) {
			switch (id) {
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
    			isp71_get_reserve_mem_phys(id, mode),
    			isp71_get_reserve_mem_virt(id, mode),
    			isp71_get_reserve_mem_dma(id, mode),
    			isp71_get_reserve_mem_size(id, mode),
    			mblock[id].is_dma_buf,
    			isp71_get_reserve_mem_fd(id, mode));
        }
	}

	return 0;
}
EXPORT_SYMBOL(isp71_release_working_buffer);

int isp71_get_init_info(struct img_init_info *info)
{

	if (!info) {
		pr_info("%s:NULL info\n", __func__);
		return -1;
	}

    if (!info->is_capture) {
        if (!info->smvr_mode) {
	/*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
       	info->module_info_streaming[0].c_wbuf =
       				isp71_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].c_wbuf_sz =
       				isp71_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf =
       				isp71_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf_sz =
       				isp71_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[0].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_streaming);

  // ADL
       	info->module_info_streaming[1].c_wbuf =
       				isp71_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].c_wbuf_sz =
       				isp71_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf =
       				isp71_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf_sz =
       				isp71_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[1].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_streaming);

	// TRAW
       	info->module_info_streaming[2].c_wbuf =
       				isp71_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].c_wbuf_sz =
       				isp71_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf =
       				isp71_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf_sz =
       				isp71_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[2].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_streaming);

		// DIP
       	info->module_info_streaming[3].c_wbuf =
       				isp71_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].c_wbuf_sz =
       				isp71_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf =
       				isp71_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf_sz =
       				isp71_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[3].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_streaming);

	// PQDIP
       	info->module_info_streaming[4].c_wbuf =
       				isp71_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].c_wbuf_sz =
       				isp71_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].c_wbuf_fd =
       			isp71_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf =
       				isp71_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf_sz =
       				isp71_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_streaming);
       	info->module_info_streaming[4].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_streaming);
        } else {
         /*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
       	info->module_info_smvr[0].c_wbuf =
       				isp71_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].c_wbuf_sz =
       				isp71_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf =
       				isp71_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf_sz =
       				isp71_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[0].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_smvr);

         // ADL
       	info->module_info_smvr[1].c_wbuf =
       				isp71_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].c_wbuf_sz =
       				isp71_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf =
       				isp71_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf_sz =
       				isp71_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[1].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_smvr);

       	// TRAW
       	info->module_info_smvr[2].c_wbuf =
       				isp71_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].c_wbuf_sz =
       				isp71_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf =
       				isp71_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf_sz =
       				isp71_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[2].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_smvr);

       		// DIP
       	info->module_info_smvr[3].c_wbuf =
       				isp71_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].c_wbuf_sz =
       				isp71_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].c_wbuf_fd =
       				isp71_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf =
       				isp71_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf_sz =
       				isp71_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[3].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_smvr);

       	// PQDIP
       	info->module_info_smvr[4].c_wbuf =
       				isp71_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].c_wbuf_dma =
       				isp71_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].c_wbuf_sz =
       				isp71_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].c_wbuf_fd =
       			isp71_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf =
       				isp71_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf_dma =
       				isp71_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf_sz =
       				isp71_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_smvr);
       	info->module_info_smvr[4].t_wbuf_fd =
       				isp71_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_smvr);
        }

    } else {
	/*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
       	info->module_info_capture[0].c_wbuf =
				isp71_get_reserve_mem_phys(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].c_wbuf_dma =
				isp71_get_reserve_mem_dma(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].c_wbuf_sz =
				isp71_get_reserve_mem_size(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].c_wbuf_fd =
				isp71_get_reserve_mem_fd(WPE_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf =
				isp71_get_reserve_mem_phys(WPE_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf_dma =
				isp71_get_reserve_mem_dma(WPE_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf_sz =
				isp71_get_reserve_mem_size(WPE_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[0].t_wbuf_fd =
				isp71_get_reserve_mem_fd(WPE_MEM_T_ID, imgsys_capture);

  // ADL
       	info->module_info_capture[1].c_wbuf =
				isp71_get_reserve_mem_phys(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].c_wbuf_dma =
				isp71_get_reserve_mem_dma(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].c_wbuf_sz =
				isp71_get_reserve_mem_size(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].c_wbuf_fd =
				isp71_get_reserve_mem_fd(ADL_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf =
				isp71_get_reserve_mem_phys(ADL_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf_dma =
				isp71_get_reserve_mem_dma(ADL_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf_sz =
				isp71_get_reserve_mem_size(ADL_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[1].t_wbuf_fd =
				isp71_get_reserve_mem_fd(ADL_MEM_T_ID, imgsys_capture);

	// TRAW
       	info->module_info_capture[2].c_wbuf =
				isp71_get_reserve_mem_phys(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].c_wbuf_dma =
				isp71_get_reserve_mem_dma(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].c_wbuf_sz =
				isp71_get_reserve_mem_size(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].c_wbuf_fd =
				isp71_get_reserve_mem_fd(TRAW_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf =
				isp71_get_reserve_mem_phys(TRAW_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf_dma =
				isp71_get_reserve_mem_dma(TRAW_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf_sz =
				isp71_get_reserve_mem_size(TRAW_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[2].t_wbuf_fd =
				isp71_get_reserve_mem_fd(TRAW_MEM_T_ID, imgsys_capture);

		// DIP
       	info->module_info_capture[3].c_wbuf =
				isp71_get_reserve_mem_phys(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].c_wbuf_dma =
				isp71_get_reserve_mem_dma(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].c_wbuf_sz =
				isp71_get_reserve_mem_size(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].c_wbuf_fd =
				isp71_get_reserve_mem_fd(DIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf =
				isp71_get_reserve_mem_phys(DIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf_dma =
				isp71_get_reserve_mem_dma(DIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf_sz =
				isp71_get_reserve_mem_size(DIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[3].t_wbuf_fd =
				isp71_get_reserve_mem_fd(DIP_MEM_T_ID, imgsys_capture);

	// PQDIP
       	info->module_info_capture[4].c_wbuf =
				isp71_get_reserve_mem_phys(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].c_wbuf_dma =
				isp71_get_reserve_mem_dma(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].c_wbuf_sz =
				isp71_get_reserve_mem_size(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].c_wbuf_fd =
			isp71_get_reserve_mem_fd(PQDIP_MEM_C_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf =
				isp71_get_reserve_mem_phys(PQDIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf_dma =
				isp71_get_reserve_mem_dma(PQDIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf_sz =
				isp71_get_reserve_mem_size(PQDIP_MEM_T_ID, imgsys_capture);
       	info->module_info_capture[4].t_wbuf_fd =
				isp71_get_reserve_mem_fd(PQDIP_MEM_T_ID, imgsys_capture);
    }

	info->hw_buf = isp71_get_reserve_mem_phys(DIP_MEM_FOR_HW_ID, imgsys_streaming);

	/*common*/
	/* info->g_wbuf_fd = isp71_get_reserve_mem_fd(IMG_MEM_G_ID); */
	info->g_wbuf_fd = isp71_get_reserve_mem_fd(IMG_MEM_G_ID, imgsys_streaming);
	info->g_wbuf = isp71_get_reserve_mem_phys(IMG_MEM_G_ID, imgsys_streaming);
	/*info->g_wbuf_sw = isp71_get_reserve_mem_virt(IMG_MEM_G_ID);*/
	info->g_wbuf_sz = isp71_get_reserve_mem_size(IMG_MEM_G_ID, imgsys_streaming);

	return 0;
}

static int isp71_put_gce(unsigned int mode)
{
	kref_put(&gmb[IMG_MEM_G_ID].kref, gce_release);
	return 0;
}

static int isp71_get_gce(unsigned int mode)
{
	kref_get(&gmb[IMG_MEM_G_ID].kref);
	return 0;
}

void *isp71_get_wpe_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[WPE_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_C_ID].start_virt;
	else
		return smb[WPE_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_wpe_virt);

int isp71_get_wpe_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[WPE_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_C_ID].fd;
	else
		return smb[WPE_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_wpe_cq_fd);

int isp71_get_wpe_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[WPE_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[WPE_MEM_T_ID].fd;
	else
		return smb[WPE_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_wpe_tdr_fd);

void *isp71_get_dip_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[DIP_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_C_ID].start_virt;
	else
		return smb[DIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_dip_virt);

int isp71_get_dip_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[DIP_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_C_ID].fd;
	else
		return smb[DIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_dip_cq_fd);

int isp71_get_dip_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[DIP_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[DIP_MEM_T_ID].fd;
	else
		return smb[DIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_dip_tdr_fd);

void *isp71_get_traw_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[TRAW_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_C_ID].start_virt;
	else
		return smb[TRAW_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_traw_virt);

int isp71_get_traw_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[TRAW_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_C_ID].fd;
	else
		return smb[TRAW_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_traw_cq_fd);

int isp71_get_traw_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[TRAW_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[TRAW_MEM_T_ID].fd;
	else
		return smb[TRAW_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_traw_tdr_fd);

void *isp71_get_pqdip_virt(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[PQDIP_MEM_C_ID].start_virt;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_C_ID].start_virt;
	else
		return smb[PQDIP_MEM_C_ID].start_virt;
}

EXPORT_SYMBOL(isp71_get_pqdip_virt);

int isp71_get_pqdip_cq_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[PQDIP_MEM_C_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_C_ID].fd;
	else
		return smb[PQDIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_pqdip_cq_fd);

int isp71_get_pqdip_tdr_fd(unsigned int mode)
{
	if (mode == imgsys_streaming)
	return mb[PQDIP_MEM_T_ID].fd;
	else if(mode == imgsys_capture)
		return cmb[PQDIP_MEM_T_ID].fd;
	else
		return smb[PQDIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_pqdip_tdr_fd);
#else
int isp71_release_working_buffer(struct mtk_hcp *hcp_dev)
{
	enum isp71_rsv_mem_id_t id;
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
    			isp71_get_reserve_mem_phys(id),
    			isp71_get_reserve_mem_virt(id),
    			isp71_get_reserve_mem_dma(id),
    			isp71_get_reserve_mem_size(id),
    			mblock[id].is_dma_buf,
    			isp71_get_reserve_mem_fd(id));
	    }
	}

	return 0;
}
EXPORT_SYMBOL(isp71_release_working_buffer);

int isp71_get_init_info(struct img_init_info *info)
{

	if (!info) {
		pr_info("%s:NULL info\n", __func__);
		return -1;
	}

	info->hw_buf = isp71_get_reserve_mem_phys(DIP_MEM_FOR_HW_ID);
	/*WPE:0, ADL:1, TRAW:2, DIP:3, PQDIP:4 */
	info->module_info[0].c_wbuf =
				isp71_get_reserve_mem_phys(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_dma =
				isp71_get_reserve_mem_dma(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_sz =
				isp71_get_reserve_mem_size(WPE_MEM_C_ID);
	info->module_info[0].c_wbuf_fd =
				isp71_get_reserve_mem_fd(WPE_MEM_C_ID);
	info->module_info[0].t_wbuf =
				isp71_get_reserve_mem_phys(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_dma =
				isp71_get_reserve_mem_dma(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_sz =
				isp71_get_reserve_mem_size(WPE_MEM_T_ID);
	info->module_info[0].t_wbuf_fd =
				isp71_get_reserve_mem_fd(WPE_MEM_T_ID);

  // ADL
	info->module_info[1].c_wbuf =
				isp71_get_reserve_mem_phys(ADL_MEM_C_ID);
	info->module_info[1].c_wbuf_dma =
				isp71_get_reserve_mem_dma(ADL_MEM_C_ID);
	info->module_info[1].c_wbuf_sz =
				isp71_get_reserve_mem_size(ADL_MEM_C_ID);
	info->module_info[1].c_wbuf_fd =
				isp71_get_reserve_mem_fd(ADL_MEM_C_ID);
	info->module_info[1].t_wbuf =
				isp71_get_reserve_mem_phys(ADL_MEM_T_ID);
	info->module_info[1].t_wbuf_dma =
				isp71_get_reserve_mem_dma(ADL_MEM_T_ID);
	info->module_info[1].t_wbuf_sz =
				isp71_get_reserve_mem_size(ADL_MEM_T_ID);
	info->module_info[1].t_wbuf_fd =
				isp71_get_reserve_mem_fd(ADL_MEM_T_ID);

	// TRAW
	info->module_info[2].c_wbuf =
				isp71_get_reserve_mem_phys(TRAW_MEM_C_ID);
	info->module_info[2].c_wbuf_dma =
				isp71_get_reserve_mem_dma(TRAW_MEM_C_ID);
	info->module_info[2].c_wbuf_sz =
				isp71_get_reserve_mem_size(TRAW_MEM_C_ID);
	info->module_info[2].c_wbuf_fd =
				isp71_get_reserve_mem_fd(TRAW_MEM_C_ID);
	info->module_info[2].t_wbuf =
				isp71_get_reserve_mem_phys(TRAW_MEM_T_ID);
	info->module_info[2].t_wbuf_dma =
				isp71_get_reserve_mem_dma(TRAW_MEM_T_ID);
	info->module_info[2].t_wbuf_sz =
				isp71_get_reserve_mem_size(TRAW_MEM_T_ID);
	info->module_info[2].t_wbuf_fd =
				isp71_get_reserve_mem_fd(TRAW_MEM_T_ID);

		// DIP
	info->module_info[3].c_wbuf =
				isp71_get_reserve_mem_phys(DIP_MEM_C_ID);
	info->module_info[3].c_wbuf_dma =
				isp71_get_reserve_mem_dma(DIP_MEM_C_ID);
	info->module_info[3].c_wbuf_sz =
				isp71_get_reserve_mem_size(DIP_MEM_C_ID);
	info->module_info[3].c_wbuf_fd =
				isp71_get_reserve_mem_fd(DIP_MEM_C_ID);
	info->module_info[3].t_wbuf =
				isp71_get_reserve_mem_phys(DIP_MEM_T_ID);
	info->module_info[3].t_wbuf_dma =
				isp71_get_reserve_mem_dma(DIP_MEM_T_ID);
	info->module_info[3].t_wbuf_sz =
				isp71_get_reserve_mem_size(DIP_MEM_T_ID);
	info->module_info[3].t_wbuf_fd =
				isp71_get_reserve_mem_fd(DIP_MEM_T_ID);

	// PQDIP
	info->module_info[4].c_wbuf =
				isp71_get_reserve_mem_phys(PQDIP_MEM_C_ID);
	info->module_info[4].c_wbuf_dma =
				isp71_get_reserve_mem_dma(PQDIP_MEM_C_ID);
	info->module_info[4].c_wbuf_sz =
				isp71_get_reserve_mem_size(PQDIP_MEM_C_ID);
	info->module_info[4].c_wbuf_fd =
			isp71_get_reserve_mem_fd(PQDIP_MEM_C_ID);
	info->module_info[4].t_wbuf =
				isp71_get_reserve_mem_phys(PQDIP_MEM_T_ID);
	info->module_info[4].t_wbuf_dma =
				isp71_get_reserve_mem_dma(PQDIP_MEM_T_ID);
	info->module_info[4].t_wbuf_sz =
				isp71_get_reserve_mem_size(PQDIP_MEM_T_ID);
	info->module_info[4].t_wbuf_fd =
				isp71_get_reserve_mem_fd(PQDIP_MEM_T_ID);

	/*common*/
	/* info->g_wbuf_fd = isp71_get_reserve_mem_fd(IMG_MEM_G_ID); */
	info->g_wbuf_fd = isp71_get_reserve_mem_fd(IMG_MEM_G_ID);
	info->g_wbuf = isp71_get_reserve_mem_phys(IMG_MEM_G_ID);
	/*info->g_wbuf_sw = isp71_get_reserve_mem_virt(IMG_MEM_G_ID);*/
	info->g_wbuf_sz = isp71_get_reserve_mem_size(IMG_MEM_G_ID);

	return 0;
}

static int isp71_put_gce(void)
{
	kref_put(&mb[IMG_MEM_G_ID].kref, gce_release);
	return 0;
}

static int isp71_get_gce(void)
{
	kref_get(&mb[IMG_MEM_G_ID].kref);
	return 0;
}

void *isp71_get_wpe_virt(void)
{
	return mb[WPE_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_wpe_virt);

int isp71_get_wpe_cq_fd(void)
{
	return mb[WPE_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_wpe_cq_fd);

int isp71_get_wpe_tdr_fd(void)
{
	return mb[WPE_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_wpe_tdr_fd);

void *isp71_get_dip_virt(void)
{
	return mb[DIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_dip_virt);

int isp71_get_dip_cq_fd(void)
{
	return mb[DIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_dip_cq_fd);

int isp71_get_dip_tdr_fd(void)
{
	return mb[DIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_dip_tdr_fd);

void *isp71_get_traw_virt(void)
{
	return mb[TRAW_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_traw_virt);

int isp71_get_traw_cq_fd(void)
{
	return mb[TRAW_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_traw_cq_fd);

int isp71_get_traw_tdr_fd(void)
{
	return mb[TRAW_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_traw_tdr_fd);

void *isp71_get_pqdip_virt(void)
{
	return mb[PQDIP_MEM_C_ID].start_virt;
}
EXPORT_SYMBOL(isp71_get_pqdip_virt);

int isp71_get_pqdip_cq_fd(void)
{
	return mb[PQDIP_MEM_C_ID].fd;
}
EXPORT_SYMBOL(isp71_get_pqdip_cq_fd);

int isp71_get_pqdip_tdr_fd(void)
{
	return mb[PQDIP_MEM_T_ID].fd;
}
EXPORT_SYMBOL(isp71_get_pqdip_tdr_fd);
#endif
#if SMVR_DECOUPLE
struct mtk_hcp_data isp71_hcp_data = {
	.mblock = isp71_streaming_mblock,
	.block_num = ARRAY_SIZE(isp71_streaming_mblock),
	.block_num_gce = ARRAY_SIZE(isp71_gce_mblock),
	.smblock = isp71_smvr_mblock,
	.cmblock = isp71_capture_mblock,
	.gmblock = isp71_gce_mblock,
	//.gsmblock = isp71_gce_smvr_mblock,
	.allocate = isp71_allocate_working_buffer,
	.release = isp71_release_working_buffer,
	.release_gce_buf = isp71_release_gce_working_buffer,
	.get_init_info = isp71_get_init_info,
	.get_mem_info = NULL,
	.get_gce_virt = isp71_get_gce_virt,
	.get_gce = isp71_get_gce,
	.put_gce = isp71_put_gce,
	.get_gce_token_virt = NULL,
	.get_hwid_virt = isp71_get_hwid_virt,
	.get_wpe_virt = isp71_get_wpe_virt,
	.get_wpe_cq_fd = isp71_get_wpe_cq_fd,
	.get_wpe_tdr_fd = isp71_get_wpe_tdr_fd,
	.get_dip_virt = isp71_get_dip_virt,
	.get_dip_cq_fd = isp71_get_dip_cq_fd,
	.get_dip_tdr_fd = isp71_get_dip_tdr_fd,
	.get_traw_virt = isp71_get_traw_virt,
	.get_traw_cq_fd = isp71_get_traw_cq_fd,
	.get_traw_tdr_fd = isp71_get_traw_tdr_fd,
	.get_pqdip_virt = isp71_get_pqdip_virt,
	.get_pqdip_cq_fd = isp71_get_pqdip_cq_fd,
	.get_pqdip_tdr_fd = isp71_get_pqdip_tdr_fd,
	.partial_flush = NULL,
};
#else
struct mtk_hcp_data isp71_hcp_data = {
	.mblock = isp71_reserve_mblock,
	.block_num = ARRAY_SIZE(isp71_reserve_mblock),
	.smblock = isp71_smvr_mblock,
	.allocate = isp71_allocate_working_buffer,
	.release = isp71_release_working_buffer,
	.get_init_info = isp71_get_init_info,
	.get_gce_virt = isp71_get_gce_virt,
	.get_gce = isp71_get_gce,
	.put_gce = isp71_put_gce,
	.get_hwid_virt = isp71_get_hwid_virt,
	.get_wpe_virt = isp71_get_wpe_virt,
	.get_wpe_cq_fd = isp71_get_wpe_cq_fd,
	.get_wpe_tdr_fd = isp71_get_wpe_tdr_fd,
	.get_dip_virt = isp71_get_dip_virt,
	.get_dip_cq_fd = isp71_get_dip_cq_fd,
	.get_dip_tdr_fd = isp71_get_dip_tdr_fd,
	.get_traw_virt = isp71_get_traw_virt,
	.get_traw_cq_fd = isp71_get_traw_cq_fd,
	.get_traw_tdr_fd = isp71_get_traw_tdr_fd,
	.get_pqdip_virt = isp71_get_pqdip_virt,
	.get_pqdip_cq_fd = isp71_get_pqdip_cq_fd,
	.get_pqdip_tdr_fd = isp71_get_pqdip_tdr_fd,
	.partial_flush = NULL,
};
#endif
MODULE_IMPORT_NS(DMA_BUF);
