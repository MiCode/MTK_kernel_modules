// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/jiffies.h>

#include <linux/time.h>		//do_gettimeofday()

#include "mtk-interconnect.h"

#include "camera_pda.h"
#include "mtk_cam-bwr.h"

// --------- define region --------
#define PDA_MMQOS
// --------------------------------

#define PDA_DEV_NAME "camera-pda"

#define LOG_INF(format, args...)                                               \
	pr_info(PDA_DEV_NAME " [%s] " format, __func__, ##args)

//define the write register function
#define mt_reg_sync_writel(v, a) \
	do {    \
		*(unsigned int *)(a) = (v);    \
		mb();  /*make sure register access in order */ \
	} while (0)
#define PDA_WR32(addr, data) mt_reg_sync_writel(data, addr)
#define PDA_RD32(addr) ioread32(addr)

static unsigned int g_Frame_Width, g_Frame_Height, g_B_N, g_FOV;

// debug command
static int32_t pda_log_dbg_en;

/*******************************************************************************
 *                               Porting Part
 ******************************************************************************/
#define CAMSYS_NODE_COMPATIBLE "mediatek,mt6991-camsys_mraw"
#define PDA_0_RESET_BITMASK (BIT(12) | BIT(13))
#define PDA_1_RESET_BITMASK (BIT(14) | BIT(15))

// clock relate
static const char * const clk_names[] = {
	"camsys_mraw_pda0",
	"camsys_mraw_pda1",
	"mraw_larbx",
	"cam_main_cam2mm0_gals_cg_con",
	"cam_main_cam2mm1_gals_cg_con",
	"cam_main_cam_cg_con",
};
#define PDA_CLK_NUM ARRAY_SIZE(clk_names)
struct PDA_CLK_STRUCT pda_clk[PDA_CLK_NUM];

#ifdef PDA_MMQOS
// mmqos relate
static const char * const mmqos_pda1_names_rdma[] = {
	"l25_pdai_a0",
	"l25_pdai_a1"
};
#define PDA_MMQOS_PDA1_RDMA_NUM ARRAY_SIZE(mmqos_pda1_names_rdma)
struct icc_path *icc_path_pda1_rdma[PDA_MMQOS_PDA1_RDMA_NUM];

static const char * const mmqos_pda2_names_rdma[] = {
	"l26_pdai_b0",
	"l26_pdai_b1"
};
#define PDA_MMQOS_PDA2_RDMA_NUM ARRAY_SIZE(mmqos_pda2_names_rdma)
struct icc_path *icc_path_pda2_rdma[PDA_MMQOS_PDA2_RDMA_NUM];

static const char * const mmqos_names_pda1_rdma_b[] = {
	"l25_pdai_a2",
	"l25_pdai_a3",
	"l25_pdai_a4"
};
#define PDA_MMQOS_PDA1_RDMA_B_NUM ARRAY_SIZE(mmqos_names_pda1_rdma_b)
struct icc_path *icc_path_pda1_rdma_b[PDA_MMQOS_PDA1_RDMA_B_NUM];

static const char * const mmqos_names_pda2_rdma_b[] = {
	"l26_pdai_b2",
	"l26_pdai_b3",
	"l26_pdai_b4"
};
#define PDA_MMQOS_PDA2_RDMA_B_NUM ARRAY_SIZE(mmqos_names_pda2_rdma_b)
struct icc_path *icc_path_pda2_rdma_b[PDA_MMQOS_PDA2_RDMA_B_NUM];

static const char * const mmqos_names_pda1_wdma[] = {
	"l25_pdao_a"
};
#define PDA_MMQOS_PDA1_WDMA_NUM ARRAY_SIZE(mmqos_names_pda1_wdma)
struct icc_path *icc_path_pda1_wdma[PDA_MMQOS_PDA1_WDMA_NUM];

static const char * const mmqos_names_pda2_wdma[] = {
	"l26_pdao_b"
};
#define PDA_MMQOS_PDA2_WDMA_NUM ARRAY_SIZE(mmqos_names_pda2_wdma)
struct icc_path *icc_path_pda2_wdma[PDA_MMQOS_PDA2_WDMA_NUM];
#endif

struct mtk_bwr_device *bwr_device;

/*******************************************************************************
 *                               Internal function
 ******************************************************************************/
void pda_debug_log(int32_t debug_log_en)
{
	pda_log_dbg_en = debug_log_en;
}

struct device *init_larb(struct platform_device *pdev, int idx)
{
	struct device_node *node;
	struct platform_device *larb_pdev;
	struct device_link *link;

	/* get larb node from dts */
	node = of_parse_phandle(pdev->dev.of_node, "mediatek,larbs", idx);
	if (!node) {
		LOG_INF("fail to parse mediatek,larb\n");
		return NULL;
	}

	larb_pdev = of_find_device_by_node(node);
	if (WARN_ON(!larb_pdev)) {
		of_node_put(node);
		LOG_INF("no larb for idx %d\n", idx);
		return NULL;
	}
	of_node_put(node);

	link = device_link_add(&pdev->dev, &larb_pdev->dev,
					DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
	if (!link)
		LOG_INF("unable to link smi larb%d\n", idx);

	LOG_INF("pdev %p idx %d\n", pdev, idx);

	return &larb_pdev->dev;
}

/*******************************************************************************
 *                                     API
 ******************************************************************************/
#ifdef PDA_MMQOS
void pda_mmqos_init(struct device *pdev)
{
	int i = 0;

	// get interconnect path for MMQOS
	for (i = 0; i < PDA_MMQOS_PDA1_RDMA_NUM; ++i) {
		LOG_INF("rdma index: %d, mmqos name: %s\n", i, mmqos_pda1_names_rdma[i]);
		icc_path_pda1_rdma[i] = of_mtk_icc_get(pdev, mmqos_pda1_names_rdma[i]);
	}
	for (i = 0; i < PDA_MMQOS_PDA2_RDMA_NUM; ++i) {
		LOG_INF("rdma index: %d, mmqos name: %s\n", i, mmqos_pda2_names_rdma[i]);
		icc_path_pda2_rdma[i] = of_mtk_icc_get(pdev, mmqos_pda2_names_rdma[i]);
	}

	// get interconnect path for MMQOS
	for (i = 0; i < PDA_MMQOS_PDA1_RDMA_B_NUM; ++i) {
		LOG_INF("rdma b index: %d, mmqos name: %s\n", i, mmqos_names_pda1_rdma_b[i]);
		icc_path_pda1_rdma_b[i] = of_mtk_icc_get(pdev, mmqos_names_pda1_rdma_b[i]);
	}
	for (i = 0; i < PDA_MMQOS_PDA2_RDMA_B_NUM; ++i) {
		LOG_INF("rdma b index: %d, mmqos name: %s\n", i, mmqos_names_pda2_rdma_b[i]);
		icc_path_pda2_rdma_b[i] = of_mtk_icc_get(pdev, mmqos_names_pda2_rdma_b[i]);
	}

	// get interconnect path for MMQOS
	for (i = 0; i < PDA_MMQOS_PDA1_WDMA_NUM; ++i) {
		LOG_INF("wdma index: %d, mmqos name: %s\n", i, mmqos_names_pda1_wdma[i]);
		icc_path_pda1_wdma[i] = of_mtk_icc_get(pdev, mmqos_names_pda1_wdma[i]);
	}
	for (i = 0; i < PDA_MMQOS_PDA2_WDMA_NUM; ++i) {
		LOG_INF("wdma index: %d, mmqos name: %s\n", i, mmqos_names_pda2_wdma[i]);
		icc_path_pda2_wdma[i] = of_mtk_icc_get(pdev, mmqos_names_pda2_wdma[i]);
	}
}

void pda_mmqos_bw_set(struct PDA_Data_t *pda_Pdadata)
{
	int i = 0;
	unsigned int Inter_Frame_Size_Width = 0;
	unsigned int Inter_Frame_Size_Height = 0;
	unsigned int B_N = 0;

	unsigned int Mach_ROI_Max_Width = 2048;
	unsigned int Mach_ROI_Max_Height = 96;
	unsigned int Mach_Frame_Size_Width = 4096;
	unsigned int Mach_Frame_Size_Height = 192;

	unsigned int Freqency = 360;
	unsigned int FOV = 0;
	unsigned int ROI_Number = PDA_MAXROI_PER_ROUND;
	unsigned int Frame_Rate = 30;
	unsigned int Search_Range = 40;
	#define Operation_Margin (12 / 10)

	unsigned int Inter_Frame_Size = 0, Mach_Frame_Size = 0;
	unsigned int Inter_Frame_Size_FOV = 0, Mach_Frame_Size_FOV = 0;
	unsigned int Inter_Input_Total_pixel_Itar = 0;
	unsigned int Inter_Input_Total_pixel_Iref = 0;
	unsigned int Mach_Input_Total_pixel_Itar = 0;

	unsigned int Required_Operation_Cycle = 0;
	unsigned int WDMA_Data = 0, temp = 0, RDMA_Data = 0;
	unsigned int OperationTime = 0;

	unsigned int WDMA_PEAK_BW = 0, WDMA_AVG_BW = 0;
	unsigned int RDMA_PEAK_BW = 0, RDMA_AVG_BW = 0;
	unsigned int IMAGE_TABLE_RDMA_PEAK_BW = 0;
	unsigned int IMAGE_TABLE_RDMA_AVG_BW = 0;

	unsigned int IMAGE_IMAGE_RDMA_PEAK_BW = 0;
	unsigned int IMAGE_IMAGE_RDMA_AVG_BW = 0;

	unsigned int total_area = 0;

	unsigned int pda_rdma_bw_port = 0, pda_wdma_bw_port = 0;
	unsigned int rdma_bw_temp = 0, wdma_bw_temp = 0, ttl_bw_temp = 0;

	// -------------------------- parameter estimate ------------------------
	Inter_Frame_Size_Width = pda_Pdadata->PDA_HW_Register.PDA_CFG_0.Bits.PDA_WIDTH;
	Inter_Frame_Size_Height = pda_Pdadata->PDA_HW_Register.PDA_CFG_0.Bits.PDA_HEIGHT;
	B_N = pda_Pdadata->PDA_HW_Register.PDA_CFG_14.Bits.PDA_B_N;

	if (Inter_Frame_Size_Width == 0 || Inter_Frame_Size_Height == 0) {
		LOG_INF("Frame size is zero WIDTH/HEIGHT: %d/%d\n",
			Inter_Frame_Size_Width,
			Inter_Frame_Size_Height);
		return;
	}

	if (pda_log_dbg_en == 1)
		LOG_INF("roi_num:%d\n", pda_Pdadata->roi_num);

	for (i = 0; i < pda_Pdadata->roi_num; ++i) {
		if (pda_log_dbg_en == 1)
			LOG_INF("ROI:%d, w:%d, h:%d\n",
				i, pda_Pdadata->roi_w[i], pda_Pdadata->roi_h[i]);
		total_area += pda_Pdadata->roi_w[i] * pda_Pdadata->roi_h[i];
	}

	FOV = total_area * 100 / (Inter_Frame_Size_Width*Inter_Frame_Size_Height);
	if (FOV > ((B_N > 0) ? 100 : 200)) {
		LOG_INF("FOV(%d) is out of range, max FOV is %d\n",
			FOV, (B_N > 0) ? 100 : 200);
		FOV = (B_N > 0) ? 100 : 200;
	}
	if (pda_log_dbg_en == 1) {
		LOG_INF("FOV:%d, total_area:%d\n", FOV, total_area);
		LOG_INF("Frame WIDTH/HEIGHT/B_N: %d/%d/%d\n",
			Inter_Frame_Size_Width,
			Inter_Frame_Size_Height,
			B_N);
	}
	// ----------------------------------------------------------------------

	if (g_Frame_Width == Inter_Frame_Size_Width &&
		g_Frame_Height == Inter_Frame_Size_Height &&
		g_B_N == B_N &&
		g_FOV == FOV) {
		if (pda_log_dbg_en == 1)
			LOG_INF("Frame WIDTH/HEIGHT/B_N/FOV no change, no need to set qos\n");
		return;
	}

	Inter_Frame_Size = Inter_Frame_Size_Width * Inter_Frame_Size_Height;
	if (pda_log_dbg_en == 1)
		LOG_INF("E16 Inter_Frame_Size: %d\n", Inter_Frame_Size);

	Mach_Frame_Size = Mach_Frame_Size_Width * Mach_Frame_Size_Height;

	Inter_Frame_Size_FOV = Inter_Frame_Size * FOV / 100;
	if (pda_log_dbg_en == 1)
		LOG_INF("E19 Inter_Frame_Size_FOV: %d\n", Inter_Frame_Size_FOV);

	Mach_Frame_Size_FOV = Mach_Frame_Size * FOV / 100;
	Inter_Input_Total_pixel_Itar = Inter_Frame_Size_FOV;
	Inter_Input_Total_pixel_Iref = Inter_Frame_Size_FOV;
	Mach_Input_Total_pixel_Itar =
		(ROI_Number*Search_Range*Mach_ROI_Max_Height) +
		Mach_Frame_Size_FOV +
		(1*Mach_ROI_Max_Width);

	Required_Operation_Cycle =
		(unsigned int)(Mach_Input_Total_pixel_Itar *
		Operation_Margin *
		(Search_Range+1)) /
		Search_Range + 1;
	WDMA_Data = OUT_BYTE_PER_ROI*ROI_Number;
	temp = Inter_Input_Total_pixel_Itar+Inter_Input_Total_pixel_Iref;
	RDMA_Data = temp*(16+2)/8;
	if (pda_log_dbg_en == 1)
		LOG_INF("E27 RDMA_Data: %d\n", RDMA_Data);

	OperationTime = Required_Operation_Cycle / Freqency / 1000;

	// WDMA BW estimate
	WDMA_PEAK_BW = WDMA_Data / OperationTime;
	WDMA_AVG_BW = WDMA_Data * Frame_Rate / 1000;

	// RDMA BW estimate
	RDMA_PEAK_BW = RDMA_Data / OperationTime;
	RDMA_AVG_BW = RDMA_Data * Frame_Rate / 1000;

	// Left/Right RDMA BW
	IMAGE_TABLE_RDMA_PEAK_BW = RDMA_PEAK_BW / 2;
	IMAGE_TABLE_RDMA_AVG_BW = RDMA_AVG_BW / 2;
	// IMAGE IMAGE SMI port is equal to IMAGE TABLE SMI port * 16/9
	IMAGE_IMAGE_RDMA_AVG_BW = IMAGE_TABLE_RDMA_AVG_BW * 16 / 9;

	// pda is not HRT engine, no need to set HRT bw
	IMAGE_TABLE_RDMA_PEAK_BW = 0;
	IMAGE_IMAGE_RDMA_PEAK_BW = 0;
	WDMA_PEAK_BW = 0;

	// MMQOS set bw, image and table port
	for (i = 0; i < PDA_MMQOS_PDA1_RDMA_NUM; ++i) {
		if (icc_path_pda1_rdma[i]) {
			if (PDA_MMQOS_PDA2_RDMA_NUM > 0) {
				// two pda, share mmqos
				rdma_bw_temp = (unsigned int)(IMAGE_TABLE_RDMA_AVG_BW/2);
				mtk_icc_set_bw(icc_path_pda1_rdma[i],
					(int)(rdma_bw_temp),
					(int)(IMAGE_TABLE_RDMA_PEAK_BW));
			} else {
				// one pda, no need to share
				rdma_bw_temp = (unsigned int)(IMAGE_TABLE_RDMA_AVG_BW);
				mtk_icc_set_bw(icc_path_pda1_rdma[i],
					(int)(rdma_bw_temp),
					(int)(IMAGE_TABLE_RDMA_PEAK_BW));
			}
			pda_rdma_bw_port += rdma_bw_temp;
		}
	}
	for (i = 0; i < PDA_MMQOS_PDA2_RDMA_NUM; ++i) {
		if (icc_path_pda2_rdma[i]) {
			// share mmqos
			mtk_icc_set_bw(icc_path_pda2_rdma[i],
				(int)(IMAGE_TABLE_RDMA_AVG_BW/2),
				(int)(IMAGE_TABLE_RDMA_PEAK_BW));
		}
	}

	LOG_INF("RDMA_BW_PORT ImageTable AVG/PEAK: %d/%d\n",
		rdma_bw_temp, IMAGE_TABLE_RDMA_PEAK_BW);

	// MMQOS set bw, image and image port
	if (B_N <= 3) {
		// MMQOS set bw
		for (i = 0; i < B_N; ++i) {
			if (icc_path_pda1_rdma_b[i]) {
				if (PDA_MMQOS_PDA2_RDMA_B_NUM > 0) {
					rdma_bw_temp = (unsigned int)(IMAGE_IMAGE_RDMA_AVG_BW/2);
					mtk_icc_set_bw(icc_path_pda1_rdma_b[i],
						(int)(rdma_bw_temp),
						(int)(IMAGE_IMAGE_RDMA_PEAK_BW));
				} else {
					rdma_bw_temp = (unsigned int)(IMAGE_IMAGE_RDMA_AVG_BW);
					mtk_icc_set_bw(icc_path_pda1_rdma_b[i],
						(int)(rdma_bw_temp),
						(int)(IMAGE_IMAGE_RDMA_PEAK_BW));
				}
				pda_rdma_bw_port += rdma_bw_temp;
			}

			if (PDA_MMQOS_PDA2_RDMA_B_NUM > 0) {
				if (icc_path_pda2_rdma_b[i]) {
					mtk_icc_set_bw(icc_path_pda2_rdma_b[i],
						(int)(IMAGE_IMAGE_RDMA_AVG_BW/2),
						(int)(IMAGE_IMAGE_RDMA_PEAK_BW));
				}
			}
		}
		if (B_N > 0)
			LOG_INF("RDMA_BW_PORT ImageImage AVG/PEAK: %d/%d\n",
				rdma_bw_temp, IMAGE_IMAGE_RDMA_PEAK_BW);
	} else {
		LOG_INF("B_N out of range, B_N:%d\n", B_N);
	}

	// MMQOS set bw, output port
	for (i = 0; i < PDA_MMQOS_PDA1_WDMA_NUM; ++i) {
		if (icc_path_pda1_wdma[i]) {
			if (PDA_MMQOS_PDA2_WDMA_NUM > 0) {
				wdma_bw_temp = (unsigned int)(WDMA_AVG_BW/2);
				mtk_icc_set_bw(icc_path_pda1_wdma[i],
					(int)(wdma_bw_temp),
					(int)(WDMA_PEAK_BW));
			} else {
				wdma_bw_temp = (unsigned int)(WDMA_AVG_BW);
				mtk_icc_set_bw(icc_path_pda1_wdma[i],
					(int)(wdma_bw_temp),
					(int)(WDMA_PEAK_BW));
			}
			pda_wdma_bw_port += wdma_bw_temp;
		}
	}
	for (i = 0; i < PDA_MMQOS_PDA2_WDMA_NUM; ++i) {
		if (icc_path_pda2_wdma[i]) {
			mtk_icc_set_bw(icc_path_pda2_wdma[i],
				(int)(WDMA_AVG_BW/2),
				(int)(WDMA_PEAK_BW));
		}
	}

	if (pda_log_dbg_en == 1)
		LOG_INF("WDMA_BW_PORT AVG/PEAK: %d/%d\n", wdma_bw_temp, WDMA_PEAK_BW);

	// unit: KB/s to MB/s
	pda_rdma_bw_port /= 1000;
	pda_wdma_bw_port /= 1000;

	// larb25 setting
	mtk_cam_bwr_set_chn_bw(bwr_device, ENGINE_PDA, DISP_PORT,
		(int)(pda_rdma_bw_port), (int)(pda_wdma_bw_port), 0, 0, true);

	// larb26 setting
	if (PDA_MMQOS_PDA2_RDMA_B_NUM > 0) {
		mtk_cam_bwr_set_chn_bw(bwr_device, ENGINE_PDA, MDP0_PORT,
			(int)(pda_rdma_bw_port), (int)(pda_wdma_bw_port), 0, 0, true);

		// two pda, need to multiply by two
		ttl_bw_temp = (pda_rdma_bw_port + pda_wdma_bw_port) * 2;
	} else {
		ttl_bw_temp = (pda_rdma_bw_port + pda_wdma_bw_port);
	}
	mtk_cam_bwr_set_ttl_bw(bwr_device, ENGINE_PDA,
		(int)(ttl_bw_temp), 0, true);

	if (pda_log_dbg_en == 1) {
		LOG_INF("RDMA_BW TOTAL AVG: %d MB/s, WDMA_BW TOTAL AVG: %d MB/s\n",
			pda_rdma_bw_port, pda_wdma_bw_port);
		LOG_INF("Total(RDMA+WDMA) BW AVG: %d MB/s\n", ttl_bw_temp);
	}

	g_Frame_Width = Inter_Frame_Size_Width;
	g_Frame_Height = Inter_Frame_Size_Height;
	g_B_N = B_N;
	g_FOV = FOV;
}

void pda_mmqos_bw_reset(void)
{
	int i = 0;

	if (pda_log_dbg_en == 1)
		LOG_INF("mmqos reset\n");

	// MMQOS reset bw
	for (i = 0; i < PDA_MMQOS_PDA1_RDMA_NUM; ++i) {
		if (icc_path_pda1_rdma[i])
			mtk_icc_set_bw(icc_path_pda1_rdma[i], 0, 0);
	}
	for (i = 0; i < PDA_MMQOS_PDA2_RDMA_NUM; ++i) {
		if (icc_path_pda2_rdma[i])
			mtk_icc_set_bw(icc_path_pda2_rdma[i], 0, 0);
	}

	for (i = 0; i < PDA_MMQOS_PDA1_RDMA_B_NUM; ++i) {
		if (icc_path_pda1_rdma_b[i])
			mtk_icc_set_bw(icc_path_pda1_rdma_b[i], 0, 0);
	}
	for (i = 0; i < PDA_MMQOS_PDA2_RDMA_B_NUM; ++i) {
		if (icc_path_pda2_rdma_b[i])
			mtk_icc_set_bw(icc_path_pda2_rdma_b[i], 0, 0);
	}

	for (i = 0; i < PDA_MMQOS_PDA1_WDMA_NUM; ++i) {
		if (icc_path_pda1_wdma[i])
			mtk_icc_set_bw(icc_path_pda1_wdma[i], 0, 0);
	}
	for (i = 0; i < PDA_MMQOS_PDA2_WDMA_NUM; ++i) {
		if (icc_path_pda2_wdma[i])
			mtk_icc_set_bw(icc_path_pda2_wdma[i], 0, 0);
	}

	// larb25 setting
	mtk_cam_bwr_clr_bw(bwr_device, ENGINE_PDA, DISP_PORT);

	// larb26 setting
	if (PDA_MMQOS_PDA2_RDMA_B_NUM > 0)
		mtk_cam_bwr_clr_bw(bwr_device, ENGINE_PDA, MDP0_PORT);

	g_Frame_Width = 0;
	g_Frame_Height = 0;
	g_B_N = 0;
}
#endif

void pda_init_larb(struct platform_device *pdev)
{
	int larbs, i;
	struct device *larb;

	// must porting in dts
	larbs = of_count_phandle_with_args(
				pdev->dev.of_node, "mediatek,larbs", NULL);
	LOG_INF("larb_num:%d\n", larbs);
	for (i = 0; i < larbs; i++) {
		larb = init_larb(pdev, i);
		if (larb == NULL)
			LOG_INF("larb%d is NULL\n", i);
	}

	//get bwr device
	bwr_device = mtk_cam_bwr_get_dev(pdev);
}

int pda_devm_clk_get(struct platform_device *pdev)
{
	int i = 0;

	for (i = 0; i < PDA_CLK_NUM; ++i) {
		// CCF: Grab clock pointer (struct clk*)
		LOG_INF("index: %d, clock name: %s\n", i, clk_names[i]);
		pda_clk[i].CG_PDA_TOP_MUX = devm_clk_get(&pdev->dev, clk_names[i]);
		if (IS_ERR(pda_clk[i].CG_PDA_TOP_MUX)) {
			LOG_INF("cannot get %s clock\n", clk_names[i]);
			return PTR_ERR(pda_clk[i].CG_PDA_TOP_MUX);
		}
	}
	return 0;
}

void pda_clk_prepare_enable(void)
{
	int ret, i;

	mtk_cam_bwr_enable(bwr_device);

	for (i = 0; i < PDA_CLK_NUM; i++) {
		ret = clk_prepare_enable(pda_clk[i].CG_PDA_TOP_MUX);
		if (ret)
			LOG_INF("cannot enable clock (%s)\n", clk_names[i]);
		if (pda_log_dbg_en == 1)
			LOG_INF("clk_prepare_enable (%s) done", clk_names[i]);
	}
}

void pda_clk_disable_unprepare(void)
{
	int i;

	for (i = 0; i < PDA_CLK_NUM; i++) {
		clk_disable_unprepare(pda_clk[i].CG_PDA_TOP_MUX);
		if (pda_log_dbg_en == 1)
			LOG_INF("clk_disable_unprepare (%s) done\n", clk_names[i]);
	}

	mtk_cam_bwr_disable(bwr_device);
}

void __iomem *pda_get_camsys_address(void)
{
	struct device_node *camsys_node;

	// camsys node
	camsys_node = of_find_compatible_node(NULL, NULL, CAMSYS_NODE_COMPATIBLE);

	return of_iomap(camsys_node, 0);
}

unsigned int GetResetBitMask(int PDA_Index)
{
	unsigned int ret = 0;

	if (PDA_Index == 0)
		ret = PDA_0_RESET_BITMASK;
	else if (PDA_Index == 1)
		ret = PDA_1_RESET_BITMASK;
	return ret;
}
