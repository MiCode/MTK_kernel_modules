// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/jiffies.h>

#include <linux/time.h>  //do_gettimeofday()

#include "mtk-interconnect.h"

#include "iommu_debug.h"

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <aee.h>
#endif

// --------- DMA-BUF ----------
#include <linux/dma-heap.h>
#include <linux/dma-direction.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>
// ----------------------------

#include <soc/mediatek/mmdvfs_v3.h>
#include "camera_pda.h"

// --------- define region --------
// #define FPGA_UT
// #define GET_PDA_TIME
// #define FOR_DEBUG_VA_DATA
// #define SMI_LOG
#define CHECK_IRQ_COUNT
#define PDA_MMQOS
// --------------------------------

#define PDA_DEV_NAME "camera-pda"

#define LOG_INF(format, args...)                                               \
	pr_info(PDA_DEV_NAME " [%s] " format, __func__, ##args)

#ifndef MTRUE
#define MTRUE 1
#endif
#ifndef MFALSE
#define MFALSE 0
#endif

//define the write register function
#define mt_reg_sync_writel(v, a) \
	do {    \
		*(unsigned int *)(a) = (v);    \
		mb();  /*make sure register access in order */ \
	} while (0)
#define PDA_WR32(addr, data) mt_reg_sync_writel(data, addr)
#define PDA_RD32(addr) ioread32(addr)

void __iomem *CAMSYS_CONFIG_BASE;
#define CAMSYS_BASE_ADDR CAMSYS_CONFIG_BASE
#define REG_CAMSYS_CG_SET               (CAMSYS_BASE_ADDR + 0x4)
#define REG_CAMSYS_CG_CLR               (CAMSYS_BASE_ADDR + 0x8)
#define REG_CAMSYS_SW_RST               (CAMSYS_BASE_ADDR + 0xA0)

#define PDA_DONE 0x00000001
#define PDA_ERROR 0x00000002
#define PDA_STATUS_REG 0x00000001
#define PDA_CLEAR_REG 0x00000000
#define PDA_TRIGGER 0x00000003
#define PDA_DOUBLE_BUFFER 0x00000009
#define PDA_MAKE_RESET 0x00000002
#define MASK_BIT_ZERO 0x00000001
#define PDA_RESET_VALUE 0x00000001
#define PDA_HW_RESET 0x00000004

struct device *g_dev1, *g_dev2;
struct device *g_smmu_dev1;

static unsigned int g_u4pm_cnt;

static spinlock_t g_PDA_SpinLock;

wait_queue_head_t g_wait_queue_head;

static DEFINE_MUTEX(pda_mutex);
static DEFINE_MUTEX(pda_pm_mutex);

// PDA HW quantity
static unsigned int g_PDA_quantity;

// debug command
static int32_t pda_log_dbg_en;

#ifdef CHECK_IRQ_COUNT
// Calculate reasonable irq counts
static unsigned int g_reasonable_IRQCount;
static int g_PDA0_IRQCount;
static int g_PDA1_IRQCount;
#endif

static struct PDA_Data_t g_pda_Pdadata;

// pda device information
static struct PDA_device PDA_devs[PDA_MAX_QUANTITY];

// Enable clock count
static unsigned int g_u4EnableClockCount;

#ifdef GET_PDA_TIME
// Get PDA process time
struct timespec64 time_end;
struct timespec64 total_time_begin, total_time_end;
struct timespec64 pda1_done_b, pda1_done_e;
struct timespec64 pda2_done_b, pda2_done_e;
#endif

//calculate 1024 roi data
unsigned int g_rgn_x_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_y_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_h_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_w_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_iw_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_obx_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_oby_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_nbx_buf[PDA_MAXROI_PER_ROUND];
unsigned int g_rgn_nby_buf[PDA_MAXROI_PER_ROUND];

// buffer mmu
struct pda_mmu g_image_mmu[64];
struct pda_mmu g_table_mmu[16];
struct pda_mmu g_output_mmu[16];

// Output buffer
unsigned long g_Address_LI[64];
unsigned long g_Address_RI[64];
unsigned long g_Address_LT[16];
unsigned long g_Address_RT[16];
unsigned long g_OutputBufferAddr[16];

// Record 16 set of fd to avoid repeated mapping and achieve MISP optimization
int fd_l_img_rec[64];
int fd_l_tbl_rec[16];
int fd_out_rec[16];

int g_cur_li_fd[4];
int g_cur_lt_fd;
int g_cur_out_fd;

unsigned int g_cur_out_idx;

// Ring buffer index record
static unsigned int g_ring_img_idx;
static unsigned int g_ring_tbl_idx;
static unsigned int g_ring_out_idx;

// current Process ROI number
unsigned int g_CurrentProcRoiNum[PDA_MAX_QUANTITY];

static unsigned int g_B_N;

static int g_last_sensor_dev;

static void pda_reset_nocheckclk(unsigned int PDA_Index)
{
	unsigned long end = 0;

	if (g_u4pm_cnt == 0) {
		LOG_INF("Cannot process without enable pda clock, pm:%d\n", g_u4pm_cnt);
		return;
	}

	end = jiffies + msecs_to_jiffies(100);

	// reset HW status
	PDA_devs[PDA_Index].HWstatus = 0;

	// clear dma_soft_rst_stat
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG,
		PDA_CLEAR_REG);
	// make reset
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG,
		PDA_MAKE_RESET);
	wmb(); /* TBC */

	while (time_before(jiffies, end)) {
		if ((PDA_RD32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG) &
			MASK_BIT_ZERO)) {
			// equivalent to hardware reset
			PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
				PDA_HW_RESET);
			// clear reset signal
			PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG,
				PDA_CLEAR_REG);
			wmb(); /* TBC */
			// clear hardware reset signal
			PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
				PDA_CLEAR_REG);
			// LOG_INF("reset PDA%d hw success\n", PDA_Index);
			return;
		}

		LOG_INF("PDA%d Wait EMI request, DMA_RST:0x%x\n",
			PDA_Index,
			PDA_RD32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG));

		usleep_range(10, 20);
	}

	LOG_INF("reset PDA%d hw timeout\n", PDA_Index);
}

#ifndef FPGA_UT
static void pda_nontransaction_reset_nocheckclk(unsigned int PDA_Index)
{
	unsigned int MRAW_reset_value = 0;
	unsigned int Reset_Bitmask = 0;

	if (g_u4pm_cnt == 0) {
		LOG_INF("Cannot process without enable pda clock, pm:%d\n", g_u4pm_cnt);
		return;
	}

	// equivalent to hardware reset
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
		PDA_HW_RESET);

	// clear hardware reset signal
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
		PDA_CLEAR_REG);

	//MRAW PDA reset
	MRAW_reset_value = PDA_RD32(REG_CAMSYS_SW_RST);

	Reset_Bitmask = GetResetBitMask(PDA_Index);

	// LOG_INF("before, MRAW_reset_value: %x\n", MRAW_reset_value);
	MRAW_reset_value |= Reset_Bitmask;
	PDA_WR32(REG_CAMSYS_SW_RST, MRAW_reset_value);
	// LOG_INF("after, MRAW_reset_value: %x\n", PDA_RD32(REG_CAMSYS_SW_RST));
	MRAW_reset_value &= (!Reset_Bitmask);
	PDA_WR32(REG_CAMSYS_SW_RST, MRAW_reset_value);
	// LOG_INF("clear bit, MRAW_reset_value: %x\n", PDA_RD32(REG_CAMSYS_SW_RST));
}
#endif

static inline void PDA_Prepare_Enable_ccf_clock(void)
{
#if IS_ENABLED(CONFIG_OF)
	int ret = 0;
#endif
	mtk_mmdvfs_enable_vcp(true, VCP_PWR_USR_PDA);

#if IS_ENABLED(CONFIG_OF)
	/* consumer device starting work*/
	if (g_PDA_quantity > 0) {
		ret = pm_runtime_get_sync(g_dev1); //Note: It‘s not larb's device.
		if (ret) {
			LOG_INF("pm_runtime_get_sync dev1 failed:(%d)\n", ret);
			return;
		}
	}
	if (g_PDA_quantity > 1) {
		ret = pm_runtime_get_sync(g_dev2); //Note: It‘s not larb's device.
		if (ret) {
			LOG_INF("pm_runtime_get_sync dev2 failed:(%d)\n", ret);
			pm_runtime_put_sync(g_dev1);
			return;
		}
	}
	g_u4pm_cnt++;
	if (pda_log_dbg_en == 1)
		LOG_INF("pm_runtime_get_sync done\n");
#endif

	pda_clk_prepare_enable();
}

static inline void PDA_Disable_Unprepare_ccf_clock(void)
{
#if IS_ENABLED(CONFIG_OF)
	int ret = 0;
#endif
	if (!g_u4pm_cnt) {
		LOG_INF("The power on flow is abnormal, no need to do power off flow\n");
		return;
	}

	pda_clk_disable_unprepare();

#if IS_ENABLED(CONFIG_OF)
	if (g_PDA_quantity > 1) {
		ret = pm_runtime_put_sync(g_dev2);
		if (ret) {
			LOG_INF("pm_runtime_put_sync dev2 failed:(%d)\n", ret);
		}
	}
	if (g_PDA_quantity > 0) {
		ret = pm_runtime_put_sync(g_dev1);
		if (ret) {
			LOG_INF("pm_runtime_put_sync dev1 failed:(%d)\n", ret);
		}
	}
	g_u4pm_cnt--;
	if (pda_log_dbg_en == 1)
		LOG_INF("pm_runtime_put_sync done\n");
#endif

	mtk_mmdvfs_enable_vcp(false, VCP_PWR_USR_PDA);
}
/**************************************************************
 *
 **************************************************************/
static void EnableClock(bool En)
{
	int i = 0;
	unsigned int nIRQstatus = 0;

	if (En) {			/* Enable clock. */

		// Enable clock count
		spin_lock(&g_PDA_SpinLock);
		switch (g_u4EnableClockCount) {
		case 0:
			spin_unlock(&g_PDA_SpinLock);

#ifndef FPGA_UT
			if (pda_log_dbg_en == 1)
				LOG_INF("It's real ic load, Enable Clock");

			mutex_lock(&pda_pm_mutex);
			PDA_Prepare_Enable_ccf_clock();
			mutex_unlock(&pda_pm_mutex);
#else
			// Enable clock by hardcode:
			LOG_INF("It's LDVT load, Enable Clock");
			//PDA_WR32(REG_CAMSYS_CG_CLR, 0xFFFFFFFF);
#endif
			spin_lock(&g_PDA_SpinLock);
			g_u4EnableClockCount++;
			spin_unlock(&g_PDA_SpinLock);
			break;
		default:
			g_u4EnableClockCount++;
			spin_unlock(&g_PDA_SpinLock);
			break;
		}
	} else {			/* Disable clock. */

		// Disable clock count
		spin_lock(&g_PDA_SpinLock);
		g_u4EnableClockCount--;
		switch (g_u4EnableClockCount) {
		case 0:
			spin_unlock(&g_PDA_SpinLock);
#ifndef FPGA_UT
			if (pda_log_dbg_en == 1)
				LOG_INF("It's real ic load, Disable Clock");

			mutex_lock(&pda_pm_mutex);

			for (i = 0; i < g_PDA_quantity; i++) {
				LOG_INF("PDA%d, ERR_STAT: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n", i,
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_ERR_STAT_REG));
				LOG_INF("PDA%d, ERR_STAT_P3: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n", i,
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_ERR_STAT_REG),
					PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_ERR_STAT_REG));
			}

			for (i = 0; i < g_PDA_quantity; i++) {
				pda_reset_nocheckclk(i);
				pda_nontransaction_reset_nocheckclk(i);
			}

#ifdef PDA_MMQOS
			pda_mmqos_bw_reset();
#endif

			for (i = 0; i < g_PDA_quantity; i++) {
				nIRQstatus = PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_EN_REG);
				if (nIRQstatus != 0x0) {
					// disable pda done irq
					PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_EN_REG,
						0x00000000);
					nIRQstatus =
						PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_EN_REG);
					LOG_INF("PDA%d, ERR_STAT_EN: 0x%x\n", i, nIRQstatus);
				}
			}

			PDA_Disable_Unprepare_ccf_clock();
			mutex_unlock(&pda_pm_mutex);
#else
			// Disable clock by hardcode:
			LOG_INF("It's LDVT load, Disable Clock");
			//PDA_WR32(REG_CAMSYS_CG_SET, 0xFFFFFFFF);
#endif

			break;
		default:
			spin_unlock(&g_PDA_SpinLock);
			break;
		}
	}
}

static void pda_reset(unsigned int PDA_Index)
{
	unsigned long end = 0;

	spin_lock(&g_PDA_SpinLock);
	if (g_u4EnableClockCount == 0 || g_u4pm_cnt == 0) {
		LOG_INF("Cannot process without enable pda clock, clk/pm:%d/%d\n",
			g_u4EnableClockCount, g_u4pm_cnt);
		spin_unlock(&g_PDA_SpinLock);
		return;
	}
	spin_unlock(&g_PDA_SpinLock);

	end = jiffies + msecs_to_jiffies(100);

	// reset HW status
	PDA_devs[PDA_Index].HWstatus = 0;

	// clear dma_soft_rst_stat
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG,
		PDA_CLEAR_REG);
	// make reset
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG,
		PDA_MAKE_RESET);
	wmb(); /* TBC */

	while (time_before(jiffies, end)) {
		if ((PDA_RD32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG) &
			MASK_BIT_ZERO)) {
			// equivalent to hardware reset
			PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
				PDA_HW_RESET);
			// clear reset signal
			PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG,
				PDA_CLEAR_REG);
			wmb(); /* TBC */
			// clear hardware reset signal
			PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
				PDA_CLEAR_REG);
			// LOG_INF("reset PDA%d hw success\n", PDA_Index);
			return;
		}

		LOG_INF("PDA%d Wait EMI request, DMA_RST:0x%x\n",
			PDA_Index,
			PDA_RD32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_DMA_RST_REG));

		usleep_range(10, 20);
	}

	LOG_INF("reset PDA%d hw timeout\n", PDA_Index);
}

#ifndef FPGA_UT
static void pda_nontransaction_reset(unsigned int PDA_Index)
{
	unsigned int MRAW_reset_value = 0;
	unsigned int Reset_Bitmask = 0;

	spin_lock(&g_PDA_SpinLock);
	if (g_u4EnableClockCount == 0 || g_u4pm_cnt == 0) {
		LOG_INF("Cannot process without enable pda clock, clk/pm:%d/%d\n",
			g_u4EnableClockCount, g_u4pm_cnt);
		spin_unlock(&g_PDA_SpinLock);
		return;
	}
	spin_unlock(&g_PDA_SpinLock);

	// equivalent to hardware reset
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
		PDA_HW_RESET);

	// clear hardware reset signal
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDA_TOP_CTL_REG,
		PDA_CLEAR_REG);

	//MRAW PDA reset
	MRAW_reset_value = PDA_RD32(REG_CAMSYS_SW_RST);

	Reset_Bitmask = GetResetBitMask(PDA_Index);

	// LOG_INF("before, MRAW_reset_value: %x\n", MRAW_reset_value);
	MRAW_reset_value |= Reset_Bitmask;
	PDA_WR32(REG_CAMSYS_SW_RST, MRAW_reset_value);
	// LOG_INF("after, MRAW_reset_value: %x\n", PDA_RD32(REG_CAMSYS_SW_RST));
	MRAW_reset_value &= (!Reset_Bitmask);
	PDA_WR32(REG_CAMSYS_SW_RST, MRAW_reset_value);
	// LOG_INF("clear bit, MRAW_reset_value: %x\n", PDA_RD32(REG_CAMSYS_SW_RST));
}
#endif

static int pda_get_dma_buffer(struct pda_mmu *mmu, int fd)
{
	struct dma_buf *buf;

	if (pda_log_dbg_en == 1)
		LOG_INF("get_dma_buffer_fd= %d\n", fd);

	if (fd < 0)
		return -1;

	buf = dma_buf_get(fd);
	if (IS_ERR(buf))
		return -1;

	mmu->dma_buf = buf;
	mmu->attach = dma_buf_attach(mmu->dma_buf, g_smmu_dev1);
	if (IS_ERR(mmu->attach))
		goto err_attach;


#ifdef DMA_BUF_UNLOCKED_API
	mmu->sgt = dma_buf_map_attachment_unlocked(mmu->attach, DMA_BIDIRECTIONAL);
#else
	mmu->sgt = dma_buf_map_attachment(mmu->attach, DMA_BIDIRECTIONAL);
#endif
	if (IS_ERR(mmu->sgt))
		goto err_map;

	return 0;
err_map:
	dma_buf_detach(mmu->dma_buf, mmu->attach);
	LOG_INF("err_map!\n");
err_attach:
	LOG_INF("err_attach!\n");
	dma_buf_put(mmu->dma_buf);
	return -1;
}

static void pda_put_dma_buffer(struct pda_mmu *mmu)
{
	if (mmu->attach == NULL || mmu->sgt == NULL) {
		LOG_INF("attach or sgt is null, no need to free iova\n");
		return;
	}

	if (mmu->dma_buf) {

#ifdef DMA_BUF_UNLOCKED_API
		dma_buf_unmap_attachment_unlocked(mmu->attach, mmu->sgt, DMA_BIDIRECTIONAL);
#else
		dma_buf_unmap_attachment(mmu->attach, mmu->sgt, DMA_BIDIRECTIONAL);
#endif
		dma_buf_detach(mmu->dma_buf, mmu->attach);
		dma_buf_put(mmu->dma_buf);
	}
}

static void HWDMASettings(struct PDA_Data_t *pda_PdaConfig)
{
	unsigned int i;

	for (i = 0; i < g_PDA_quantity; i++) {
		// --------- Frame setting part -----------
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_0_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_0.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_1_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_1.Raw);
		// need set roi number every process
		// PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_2_REG,
		//     pda_PdaConfig->PDA_HW_Register.PDA_CFG_2.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_3_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_3.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_4_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_4.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_5_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_5.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_6_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_6.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_7_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_7.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_8_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_8.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_9_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_9.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_10_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_10.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_11_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_11.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_12_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_12.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_13_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_13.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_14_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_14.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_15_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_15.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_16_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_16.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_17_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_17.Raw);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_CFG_18_REG,
			pda_PdaConfig->PDA_HW_Register.PDA_CFG_18.Raw);

		// --------- DMA Secure part -------------
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_SECURE_REG, 0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_SECURE_1_REG, 0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_SECURE_2_REG, 0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_SECURE_3_REG, 0x00000000);

		// --------- config setting hard code part --------------
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_STRIDE_REG, OUT_BYTE_PER_ROI);

		// Left image
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON0_REG, 0x100000cc);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON1_REG, 0x10330033);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON2_REG, 0x00660066);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON3_REG, 0x00880088);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON4_REG, 0x00660066);

		// Left image 1
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON0_REG, 0x10000040);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON1_REG, 0x00100010);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON2_REG, 0x00200020);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON3_REG, 0x00300030);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON4_REG, 0x80200020);

		// Left image 2
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON0_REG, 0x10000040);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON1_REG, 0x00100010);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON2_REG, 0x00200020);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON3_REG, 0x00300030);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON4_REG, 0x80200020);

		// Left image 3
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON0_REG, 0x10000040);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON1_REG, 0x00100010);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON2_REG, 0x00200020);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON3_REG, 0x00300030);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON4_REG, 0x80200020);

		// Left table
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON0_REG, 0x10000033);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON1_REG, 0x000c000c);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON2_REG, 0x00190019);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON3_REG, 0x00220022);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON4_REG, 0x00190019);

		// Right image
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON0_REG, 0x100000cc);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON1_REG, 0x10330033);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON2_REG, 0x00660066);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON3_REG, 0x00880088);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON4_REG, 0x00660066);

		// Right image 1
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON0_REG, 0x10000040);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON1_REG, 0x00100010);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON2_REG, 0x10200020);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON3_REG, 0x80300030);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON4_REG, 0x80200020);

		// Right image 2
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON0_REG, 0x10000040);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON1_REG, 0x00100010);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON2_REG, 0x10200020);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON3_REG, 0x80300030);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON4_REG, 0x00200020);

		// Right image 3
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON0_REG, 0x10000040);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON1_REG, 0x00100010);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON2_REG, 0x00200020);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON3_REG, 0x00300030);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON4_REG, 0x00200020);

		// Right table
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON0_REG, 0x10000033);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON1_REG, 0x000c000c);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON2_REG, 0x00190019);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON3_REG, 0x00220022);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON4_REG, 0x00190019);

		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_XSIZE_REG, (OUT_BYTE_PER_ROI-1));

		// Output
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON0_REG, 0x10000060);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON1_REG, 0x00100010);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON2_REG, 0x00200020);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON3_REG, 0x00300030);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON4_REG, 0x00200020);

		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_DMA_EN_REG, 0x7ff);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_DMA_RST_REG, 0x1);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_DMA_TOP_REG, 0x7802);

		// DCM all off: 0x00001FFF
		// DCM all on:  0x00000000
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_DCM_DIS_REG, 0x00001FFF);

		//disable dma error irq: 0x00000000
		//enable dma error irq: 0xffff0000
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_ERR_STAT_REG,
			0x00000000);
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_ERR_STAT_REG,
			0x00000000);

		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_TOP_CTL_REG, 0x00000000);

		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_IRQ_TRIG_REG, 0x0);

		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_AUTO_TRIG_REG, 0x0);

		// clear AXSLC
		PDA_WR32(PDA_devs[i].m_pda_base + 0x06d8, 0x0);
		PDA_WR32(PDA_devs[i].m_pda_base + 0x06dc, 0x0);
		PDA_WR32(PDA_devs[i].m_pda_base + 0x06e0, 0x0);
		PDA_WR32(PDA_devs[i].m_pda_base + 0x06e4, 0x0);
		PDA_WR32(PDA_devs[i].m_pda_base + 0x06e8, 0x0);
		PDA_WR32(PDA_devs[i].m_pda_base + 0x06ec, 0x0);

		// ddren set hw mode
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_DDREN_CFG_REG, 0x8);

		// setting read clear
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_EN_REG, 0x00000001);

		// read 0x3b4, avoid the impact of previous data
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_REG);
		// read clear dma status
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_ERR_STAT_REG);
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_ERR_STAT_REG);
	}
}

static int ProcessROIData(struct PDA_Data_t *pda_data,
				unsigned int RoiProcNum,
				unsigned int ROIIndex)
{
	unsigned int i = 0;

	for (i = 0; i < RoiProcNum; i++) {
		g_rgn_x_buf[i] = pda_data->roi_x[ROIIndex+i];
		g_rgn_y_buf[i] = pda_data->roi_y[ROIIndex+i];
		g_rgn_w_buf[i] = pda_data->roi_w[ROIIndex+i];
		g_rgn_h_buf[i] = pda_data->roi_h[ROIIndex+i];
		g_rgn_iw_buf[i] = pda_data->roi_iw[ROIIndex+i];
		g_rgn_obx_buf[i] = pda_data->roi_obx[ROIIndex+i];
		g_rgn_oby_buf[i] = pda_data->roi_oby[ROIIndex+i];
		g_rgn_nbx_buf[i] = pda_data->roi_nbx[ROIIndex+i];
		g_rgn_nby_buf[i] = pda_data->roi_nby[ROIIndex+i];
	}

	if (pda_log_dbg_en == 1)
		LOG_INF("Fill ROI info to global var done\n");

	return 0;
}

static int CheckDesignLimitation(struct PDA_Data_t *PDA_Data,
				unsigned int RoiProcNum,
				unsigned int ROIIndex)
{
	unsigned int i = 0;
	int nROIIndex = 0;
	int nTempVar = 0;

	if (pda_log_dbg_en == 1)
		LOG_INF("Check Design Limitation\n");

	// frame constraint
	if (PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_WIDTH % 4 != 0) {
		LOG_INF("Frame width must be multiple of 4\n");
		PDA_Data->status = -4;
		return -1;
	}

	// ROI constraint
	for (i = 0; i < RoiProcNum; i++) {
		nROIIndex = i + ROIIndex;

		if (g_rgn_w_buf[i] % 4 != 0) {
			LOG_INF("ROI_%d width(%d) must be multiple of 4\n",
				nROIIndex,
				g_rgn_w_buf[i]);
			PDA_Data->status = -5;
			return -1;
		}

		if (g_rgn_w_buf[i] > 3280) {
			LOG_INF("ROI_%d width(%d) must be less than 3280\n",
				nROIIndex,
				g_rgn_w_buf[i]);
			PDA_Data->status = -6;
			return -1;
		}

		if (g_rgn_x_buf[i] % 2 != 0) {
			LOG_INF("ROI_%d xoffset(%d) must be multiple of 2\n",
				nROIIndex,
				g_rgn_x_buf[i]);
			PDA_Data->status = -7;
			return -1;
		}

		if (g_rgn_iw_buf[i] < 41) {
			LOG_INF("ROI_%d IW(%d) must be greater than 41\n",
				nROIIndex,
				g_rgn_iw_buf[i]);
			PDA_Data->status = -8;
			return -1;
		}

		nTempVar = (1 << PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_BIN_FCTR);
		if (g_rgn_h_buf[i] % nTempVar != 0) {
			LOG_INF("ROI_%d height(%d) must be multiple of Binning number(%d)\n",
				i,
				g_rgn_h_buf[i],
				nTempVar);
			PDA_Data->status = -9;
			return -1;
		}

		// ROI boundary must be on the boundary of patch
		nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_PR_XNUM;
		if (g_rgn_x_buf[i] % nTempVar != 0) {
			LOG_INF("ROI_%d ROI boundary must be on the boundary of patch\n",
				nROIIndex);
			LOG_INF("ROI_%d_x: %d, PDA_PR_XNUM: %d\n",
				nROIIndex,
				g_rgn_x_buf[i],
				nTempVar);
			PDA_Data->status = -10;
			return -1;
		}

		if (g_rgn_w_buf[i] % nTempVar != 0) {
			LOG_INF("ROI_%d ROI boundary must be on the boundary of patch\n",
				nROIIndex);
			LOG_INF("ROI_%d_w: %d, PDA_PR_XNUM: %d\n",
				nROIIndex,
				g_rgn_w_buf[i],
				nTempVar);
			PDA_Data->status = -10;
			return -1;
		}

		nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_PR_YNUM;
		if (g_rgn_y_buf[i] % nTempVar != 0) {
			LOG_INF("ROI_%d ROI boundary must be on the boundary of patch\n",
				nROIIndex);
			LOG_INF("ROI_%d_y: %d, PDA_PR_YNUM: %d\n",
				nROIIndex,
				g_rgn_y_buf[i],
				nTempVar);
			PDA_Data->status = -10;
			return -1;
		}

		if (g_rgn_h_buf[i] % nTempVar != 0) {
			LOG_INF("ROI_%d ROI boundary must be on the boundary of patch\n",
				nROIIndex);
			LOG_INF("ROI_%d_h: %d, PDA_PR_YNUM: %d\n",
				nROIIndex,
				g_rgn_h_buf[i],
				nTempVar);
			PDA_Data->status = -10;
			return -1;
		}

		// ROI can't exceed the image region
		nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_WIDTH;
		if ((g_rgn_x_buf[i]+g_rgn_w_buf[i]) > nTempVar) {
			LOG_INF("ROI_%d ROI exceed the image region\n", nROIIndex);
			LOG_INF("Frame width/height: %d/%d, ROI x/y/w/h: %d/%d/%d/%d\n",
				PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_WIDTH,
				PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_HEIGHT,
				g_rgn_x_buf[i],
				g_rgn_y_buf[i],
				g_rgn_w_buf[i],
				g_rgn_h_buf[i]);
			PDA_Data->status = -11;
			return -1;
		}

		nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_HEIGHT;
		if ((g_rgn_y_buf[i]+g_rgn_h_buf[i]) > nTempVar) {
			LOG_INF("ROI_%d ROI exceed the image region\n", nROIIndex);
			LOG_INF("Frame width/height: %d/%d, ROI x/y/w/h: %d/%d/%d/%d\n",
				PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_WIDTH,
				PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_HEIGHT,
				g_rgn_x_buf[i],
				g_rgn_y_buf[i],
				g_rgn_w_buf[i],
				g_rgn_h_buf[i]);
			PDA_Data->status = -11;
			return -1;
		}

		// Register Range Limitation check
		if (g_rgn_x_buf[i] > 8191) {
			LOG_INF("ROI_X_%d (%d) out of range\n", nROIIndex, g_rgn_x_buf[i]);
			PDA_Data->status = -19;
			return -1;
		}

		if (g_rgn_y_buf[i] > 8191) {
			LOG_INF("ROI_Y_%d (%d) out of range\n", nROIIndex, g_rgn_y_buf[i]);
			PDA_Data->status = -20;
			return -1;
		}

		if (g_rgn_w_buf[i] < 20) {
			LOG_INF("ROI_W_%d (%d) out of range\n", nROIIndex, g_rgn_w_buf[i]);
			PDA_Data->status = -21;
			return -1;
		}

		if (g_rgn_h_buf[i] < 4 || g_rgn_h_buf[i] > 4092) {
			LOG_INF("ROI_H_%d (%d) out of range\n", nROIIndex, g_rgn_h_buf[i]);
			PDA_Data->status = -22;
			return -1;
		}

		if (g_rgn_iw_buf[i] > 3280) {
			LOG_INF("ROI_IW_%d (%d) out of range, w:%d, pat_width:%d\n",
				nROIIndex,
				g_rgn_iw_buf[i],
				g_rgn_w_buf[i],
				PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_PAT_WIDTH);
			PDA_Data->status = -23;
			return -1;
		}
	}

	// Register Range Limitation check
	nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_WIDTH;
	if (nTempVar < 20 || nTempVar > 8191) {
		LOG_INF("Frame width (%d) out of range\n", nTempVar);
		PDA_Data->status = -12;
		return -1;
	}

	nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_0.Bits.PDA_HEIGHT;
	if (nTempVar < 4 || nTempVar > 8191) {
		LOG_INF("Frame height (%d) out of range\n", nTempVar);
		PDA_Data->status = -13;
		return -1;
	}

	nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_PR_XNUM;
	if (nTempVar < 1 || nTempVar > 16) {
		LOG_INF("PDA_PR_XNUM (%d) out of range\n", nTempVar);
		PDA_Data->status = -14;
		return -1;
	}

	nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_PR_YNUM;
	if (nTempVar < 1 || nTempVar > 16) {
		LOG_INF("PDA_PR_YNUM (%d) out of range\n", nTempVar);
		PDA_Data->status = -15;
		return -1;
	}

	nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_PAT_WIDTH;
	if (nTempVar < 1 || nTempVar > 512) {
		LOG_INF("PDA_PAT_WIDTH (%d) out of range\n", nTempVar);
		PDA_Data->status = -16;
		return -1;
	}

	nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_1.Bits.PDA_RNG_ST;
	if (nTempVar > 40) {
		LOG_INF("PDA_RNG_ST (%d) out of range\n", nTempVar);
		PDA_Data->status = -17;
		return -1;
	}

	nTempVar = PDA_Data->PDA_HW_Register.PDA_CFG_2.Bits.PDA_TBL_STRIDE;
	if (nTempVar < 5 || nTempVar > 2048) {
		LOG_INF("PDA_TBL_STRIDE (%d) out of range\n", nTempVar);
		PDA_Data->status = -18;
		return -1;
	}

	return 0;
}

static void FillRegSettings(struct PDA_Data_t *pda_PdaConfig,
				unsigned int RoiProcNum,
				unsigned long OuputAddr,
				unsigned int PDA_Index)
{
	unsigned int RegIndex = 19;
	int ROI_MAX_INDEX = RoiProcNum-1;
	unsigned int i = 0;

	if (RoiProcNum > PDA_MAXROI_PER_ROUND) {
		LOG_INF("RoiProcNum out of range (%d)\n", RoiProcNum);
		return;
	}

	// modify roi number register, change [12:6] bit to RoiProcNum
	pda_PdaConfig->PDA_HW_Register.PDA_CFG_2.Bits.PDA_RGN_NUM = RoiProcNum;

	// roi number register setting
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_CFG_2_REG,
		pda_PdaConfig->PDA_HW_Register.PDA_CFG_2.Raw);

	if (pda_log_dbg_en == 1) {
		LOG_INF("ROIProcNum: %d, PDA_CFG_2_REG: 0x%x\n",
			RoiProcNum, PDA_RD32(PDA_devs[PDA_Index].m_pda_base + PDA_CFG_2_REG));
	}

	//ROI data sequentially fill to PDA_CFG[19] ~ PDA_CFG[114]
	for (i = 0; i <= ROI_MAX_INDEX; i++) {
		PDA_WR32((PDA_devs[PDA_Index].m_pda_base + 0x004*(RegIndex++)),
			(g_rgn_y_buf[i] << 16) + g_rgn_x_buf[i]);
		PDA_WR32((PDA_devs[PDA_Index].m_pda_base + 0x004*(RegIndex++)),
			(g_rgn_h_buf[i] << 16) + g_rgn_w_buf[i]);
		PDA_WR32((PDA_devs[PDA_Index].m_pda_base + 0x004*(RegIndex++)),
			(g_rgn_nby_buf[i] << 22) + (g_rgn_nbx_buf[i] << 16) + g_rgn_iw_buf[i]);
		PDA_WR32((PDA_devs[PDA_Index].m_pda_base + 0x004*(RegIndex++)),
			(g_rgn_oby_buf[i] << 16) + g_rgn_obx_buf[i]);

		if (pda_log_dbg_en == 1) {
			LOG_INF("PDA_CFG[%d:%d]: 0x%x/0x%x/0x%x/0x%x\n",
				(RegIndex-4), (RegIndex-1),
				(g_rgn_y_buf[i] << 16) + g_rgn_x_buf[i],
				(g_rgn_h_buf[i] << 16) + g_rgn_w_buf[i],
				(g_rgn_nby_buf[i] << 22) + (g_rgn_nbx_buf[i] << 16) + g_rgn_iw_buf[i],
				(g_rgn_oby_buf[i] << 16) + g_rgn_obx_buf[i]);
		}
	}

	// output buffer address setting
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDAO_P1_BASE_ADDR_MSB_REG,
		(unsigned int)(OuputAddr >> 32));
	PDA_WR32(PDA_devs[PDA_Index].m_pda_base + PDA_PDAO_P1_BASE_ADDR_REG,
		(unsigned int)(OuputAddr));

	if (pda_log_dbg_en == 1) {
		LOG_INF("PDAO_P1_BASE_ADDR_MSB: %d, PDAO_P1_BASE_ADDR: 0x%x\n",
			PDA_RD32(PDA_devs[PDA_Index].m_pda_base + PDA_PDAO_P1_BASE_ADDR_MSB_REG),
			PDA_RD32(PDA_devs[PDA_Index].m_pda_base + PDA_PDAO_P1_BASE_ADDR_REG));
	}
}

static void LOGHWRegister(unsigned int i)
{
	LOG_INF("CFG_0/1/2/3/4/5/6: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_4_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_5_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_6_REG));

	LOG_INF("CFG_7/8/9/10/11/12/13: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_7_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_8_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_9_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_10_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_11_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_12_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_13_REG));

	LOG_INF("CFG_14/15/16/17/18/19/20: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_14_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_15_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_16_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_17_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_18_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_19_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_20_REG));

	LOG_INF("CFG_21/22/23/24/25/26/27/28: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_21_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_22_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_23_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_24_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_25_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_26_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_27_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_CFG_28_REG));

	LOG_INF("I_P1/TI_P1/I_P2/TI_P2/Out: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_BASE_ADDR_REG));

	LOG_INF("LI_P3/RI_P3/LI_P4/RI_P4/LI_P5/RI_P5: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_BASE_ADDR_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_BASE_ADDR_REG));

	LOG_INF("SECURE/I_STRIDE/O_P1_XSIZE: 0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_SECURE_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_STRIDE_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_XSIZE_REG));

	LOG_INF("TILE_STATUS/TILE_STATUS_1: 0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_TILE_STATUS_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_TILE_STATUS_1_REG));

	LOG_INF("PDAI_P1_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_CON4_REG));

	LOG_INF("PDATI_P1_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_CON4_REG));

	LOG_INF("PDAI_P2_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_CON4_REG));

	LOG_INF("PDATI_P2_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_CON4_REG));

	LOG_INF("PDALI_P3_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_CON4_REG));

	LOG_INF("PDARI_P3_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_CON4_REG));

	LOG_INF("PDALI_P4_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_CON4_REG));

	LOG_INF("PDARI_P4_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_CON4_REG));

	LOG_INF("PDALI_P5_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_CON4_REG));

	LOG_INF("PDARI_P5_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_CON4_REG));

	LOG_INF("PDAO_P1_CON0/CON1/CON2/CON3/CON4: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON0_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON1_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON2_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON3_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_CON4_REG));

	LOG_INF("DMA_EN/DMA_RST/DMA_TOP/DCM_DIS/DCM_ST: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_DMA_EN_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_DMA_RST_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_DMA_TOP_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_DCM_DIS_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_DCM_ST_REG));

	LOG_INF("[ERR_STAT]I_P1/TI_P1/I_P2/TI_P2/O_P1: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_ERR_STAT_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_ERR_STAT_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_ERR_STAT_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_ERR_STAT_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_ERR_STAT_REG));

	LOG_INF("ERR_STAT_EN/ERR_STAT/TOP_CTL: 0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_EN_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_TOP_CTL_REG));

	LOG_INF("IRQ_TRIG/PDAO_DMA_EXISTED_ECO: 0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_IRQ_TRIG_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_DMA_EXISTED_ECO_REG));

	LOG_INF("[MSB]I_P1/TI_P1/I_P2/TI_P2/O_P1: 0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_BASE_ADDR_MSB_REG));

	LOG_INF("[MSB]LI_P3/RI_P3/LI_P3/RI_P3/LI_P3/RI_P3: 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x\n",
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_BASE_ADDR_MSB_REG),
		PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_BASE_ADDR_MSB_REG));
}

static void TF_dump_log(unsigned int hw_trigger_num)
{
	unsigned int i = 0;
	unsigned int sel_index = 0;

	unsigned int Debug_Sel[] = {0x00008120, 0x0000400e, 0x0000c000,
		0x11000000, 0x12000000, 0x13000000, 0x14000000, 0x15000000, 0x16000000,
		0x14000000, 0x14100000, 0x14200000, 0x14300000, 0x14400000, 0x14500000,
		0x21000000, 0x22000000, 0x23000000, 0x24000000, 0x25000000,
		0x24000000, 0x24100000, 0x24200000, 0x24300000, 0x24400000, 0x24500000,
		0x31000000, 0x32000000, 0x33000000, 0x34000000, 0x35000000,
		0x34000000, 0x34100000, 0x34200000, 0x34300000, 0x34400000, 0x34500000,
		0x41000000, 0x42000000, 0x43000000, 0x44000000, 0x45000000,
		0x44000000, 0x44100000, 0x44200000, 0x44300000, 0x44400000, 0x44500000,
		0x51000000, 0x52000000, 0x53000000, 0x54000000, 0x55000000,
		0x54000000, 0x54100000, 0x54200000, 0x54300000, 0x54400000, 0x54500000};
	unsigned int Length_Arr = sizeof(Debug_Sel)/sizeof(*Debug_Sel);

#ifdef SMI_LOG
	// SMI log
	mtk_smi_dbg_hang_detect("PDA device");
#endif

	// check debug data
	for (i = 0; i < hw_trigger_num; i++) {
		for (sel_index = 0; sel_index < Length_Arr; ++sel_index) {
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_DEBUG_SEL_REG,
				Debug_Sel[sel_index]);
			LOG_INF("PDA_%d DEBUG_SEL/DEBUG_DATA: 0x%x/0x%x\n",
				i, PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_DEBUG_SEL_REG),
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_DEBUG_DATA_REG));
		}
	}

	// check hw register setting
	for (i = 0; i < hw_trigger_num; i++) {
		LOG_INF("[outer] PDA_%d register LOG +++++\n", i);
		LOGHWRegister(i);
	}
}

static void TimeoutHandler(unsigned int hw_trigger_num)
{
#ifndef FPGA_UT
	unsigned int i = 0;
#endif

	TF_dump_log(hw_trigger_num);

#ifndef FPGA_UT
	// reset flow
	for (i = 0; i < hw_trigger_num; i++) {
		pda_reset(i);
		pda_nontransaction_reset(i);
	}
#endif
}

static void pda_execute(unsigned int hw_trigger_num)
{
	unsigned int i;

	if (pda_log_dbg_en == 1)
		LOG_INF("+\n");

	for (i = 0; i < hw_trigger_num; i++) {
		// PDA_TOP_CTL set 1'b1 to bit3, to load register from double buffer
		PDA_WR32(PDA_devs[i].m_pda_base  + PDA_PDA_TOP_CTL_REG, PDA_DOUBLE_BUFFER);

		// make sure all the pda setting take effect
		wmb();

		// PDA_TOP_CTL set 1'b1 to bit1, to trigger sof
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_TOP_CTL_REG, PDA_TRIGGER);

		// make sure all the pda setting take effect
		wmb();

		// write 0 after trigger
		PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDA_TOP_CTL_REG, PDA_CLEAR_REG);

#ifdef GET_PDA_TIME
		if (i == 0)
			ktime_get_real_ts64(&pda1_done_b);
		else
			ktime_get_real_ts64(&pda2_done_b);
#endif
	}

	if (pda_log_dbg_en == 1)
		LOG_INF("-\n");
}

#ifndef FPGA_UT
static int check_pda_status(unsigned int hw_trigger_num)
{
	unsigned int i = 0;

	for (i = 0; i < hw_trigger_num; i++) {
		if (PDA_devs[i].HWstatus == 0) {
			if (pda_log_dbg_en == 1)
				LOG_INF("PDA%d HWstatus = %d\n", i, PDA_devs[i].HWstatus);
			return 0;
		}
	}
	return 1;
}
#endif

static inline unsigned int pda_ms_to_jiffies(unsigned int ms)
{
	return ((ms * HZ + 512) >> 10);
}

static signed int pda_wait_irq(struct PDA_Data_t *pda_data, unsigned int hw_trigger_num)
{
	int ret = 0;
	unsigned int i = 0;
#ifdef FPGA_UT
	int nCount_irq = 0;
	int nstatus_one = 0, nstatus_two = 0;
#endif

	/* start to wait signal */
#ifndef FPGA_UT
	ret = wait_event_interruptible_timeout(g_wait_queue_head,
						check_pda_status(hw_trigger_num),
						pda_ms_to_jiffies(pda_data->timeout));
#else

	LOG_INF("hw_trigger_num = %d\n", hw_trigger_num);
	while (nCount_irq < 1000) {
		LOG_INF("nCount_irq = %d\n", nCount_irq);
		if (hw_trigger_num > 1) {
			nstatus_one = PDA_RD32(PDA_devs[0].m_pda_base + PDA_PDA_ERR_STAT_REG) &
				PDA_STATUS_REG;
			nstatus_two = PDA_RD32(PDA_devs[1].m_pda_base + PDA_PDA_ERR_STAT_REG) &
				PDA_STATUS_REG;

			if (nstatus_one == 1) {
				PDA_devs[0].HWstatus = 1;
				LOG_INF("nstatus_one = 1\n");
			}

			if (nstatus_two == 1) {
				PDA_devs[1].HWstatus = 1;
				LOG_INF("nstatus_two = 1\n");
			}

			if (PDA_devs[0].HWstatus == 1 && PDA_devs[1].HWstatus == 1) {
				ret = 1;
				break;
			}
		} else {
			nstatus_one = PDA_RD32(PDA_devs[0].m_pda_base + PDA_PDA_ERR_STAT_REG) &
				PDA_STATUS_REG;
			if (nstatus_one == 1) {
				LOG_INF("nstatus_one = 1\n");
				PDA_devs[0].HWstatus = 1;
				ret = 1;
				break;
			}
		}
		nCount_irq++;
	}
#endif
	if (ret == 0) {
		TimeoutHandler(hw_trigger_num);

		// timeout error
		LOG_INF("wait_event_interruptible_timeout Fail\n");
		pda_data->status = -2;
		return -1;
	} else if (ret < 0) {
		LOG_INF("wait_event return value:%d\n", ret);
		if (ret == -ERESTARTSYS)
			LOG_INF("Interrupted by a signal\n");
#ifndef FPGA_UT
		for (i = 0; i < hw_trigger_num; i++) {
			pda_reset(i);
			pda_nontransaction_reset(i);
		}
#endif
		return -1;
	}

#ifdef GET_PDA_TIME
	ktime_get_real_ts64(&time_end);
#endif

	// pda status
	pda_data->status = 1;

	// update status to user
	for (i = 0; i < hw_trigger_num; i++) {
		if (PDA_devs[i].HWstatus < 0) {
			pda_data->status = PDA_devs[i].HWstatus;

			LOG_INF("PDA%d HW error (%d)", i, PDA_devs[i].HWstatus);

			LOG_INF("PDA%d PDA_PDAI_P1_ERR_STAT_REG = 0x%x",
				i, PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_ERR_STAT_REG));
			LOG_INF("PDA%d PDA_PDATI_P1_ERR_STAT_REG = 0x%x",
				i, PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_ERR_STAT_REG));
			LOG_INF("PDA%d PDA_PDAI_P2_ERR_STAT_REG = 0x%x",
				i, PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_ERR_STAT_REG));
			LOG_INF("PDA%d PDA_PDATI_P2_ERR_STAT_REG = 0x%x",
				i, PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_ERR_STAT_REG));
			LOG_INF("PDA%d PDA_PDAO_P1_ERR_STAT_REG = 0x%x",
				i, PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_ERR_STAT_REG));

#ifndef FPGA_UT
			// reset flow
			pda_nontransaction_reset(i);
#endif
		}
	}

	return ret;
}

#ifndef FPGA_UT
static irqreturn_t pda_irqhandle(signed int Irq, void *DeviceId)
{
	unsigned int nPdaStatus = 0;

	if (g_u4EnableClockCount > 0 && g_u4pm_cnt > 0) {
		// read pda status
		nPdaStatus = PDA_RD32(PDA_devs[0].m_pda_base + PDA_PDA_ERR_STAT_REG) &
			PDA_STATUS_REG;
	}

	// for WCL=1 case, write 1 to clear pda done status
	// PDA_WR32(PDA_devs[0].m_pda_base + PDA_PDA_ERR_STAT_REG, 0x00000001);

	PDA_devs[0].HWstatus = 1;

#ifdef GET_PDA_TIME
	ktime_get_real_ts64(&pda1_done_e);
#endif

#ifdef CHECK_IRQ_COUNT
	++g_PDA0_IRQCount;
	if (g_PDA0_IRQCount > g_reasonable_IRQCount) {
		PDA_devs[0].HWstatus = -29;
		LOG_INF("Irq abnormal, rsn: %d, pda0: %d, roi/stat/stat_rg/clk/pm: %d/%d/%d/%d/%d\n",
			g_reasonable_IRQCount,
			g_PDA0_IRQCount,
			g_pda_Pdadata.roi_num,
			g_pda_Pdadata.status,
			nPdaStatus,
			g_u4EnableClockCount,
			g_u4pm_cnt);
		pda_nontransaction_reset(0);
	}
#endif

	// wake up user space WAIT_IRQ flag
	wake_up_interruptible(&g_wait_queue_head);
	return IRQ_HANDLED;
}

static irqreturn_t pda2_irqhandle(signed int Irq, void *DeviceId)
{
	unsigned int nPdaStatus = 0;

	if (g_u4EnableClockCount > 0 && g_u4pm_cnt > 0) {
		// read pda status
		nPdaStatus = PDA_RD32(PDA_devs[1].m_pda_base + PDA_PDA_ERR_STAT_REG) &
			PDA_STATUS_REG;
	}

	// for WCL=1 case, write 1 to clear pda done status
	// PDA_WR32(PDA_devs[1].m_pda_base + PDA_PDA_ERR_STAT_REG, 0x00000001);

	PDA_devs[1].HWstatus = 1;

#ifdef GET_PDA_TIME
	ktime_get_real_ts64(&pda2_done_e);
#endif

#ifdef CHECK_IRQ_COUNT
	++g_PDA1_IRQCount;
	if (g_PDA1_IRQCount > g_reasonable_IRQCount) {
		PDA_devs[1].HWstatus = -29;
		LOG_INF("Irq abnormal, rsn: %d, pda1: %d, roi/stat/stat_rg/clk/pm: %d/%d/%d/%d/%d\n",
			g_reasonable_IRQCount,
			g_PDA1_IRQCount,
			g_pda_Pdadata.roi_num,
			g_pda_Pdadata.status,
			nPdaStatus,
			g_u4EnableClockCount,
			g_u4pm_cnt);
		pda_nontransaction_reset(1);
	}
#endif

	// wake up user space WAIT_IRQ flag
	wake_up_interruptible(&g_wait_queue_head);
	return IRQ_HANDLED;
}
#endif

static int PDAProcessFunction(unsigned int nUserROINumber,
				unsigned int p_index)
{
	unsigned int i = 0, j = 0;
	unsigned int nOneRoundProcROI = 0;
	unsigned int hw_trigger_num = 0;
	unsigned int nRemainder = 0, nFactor = 0;
	unsigned int nCurrentProcRoiIndex = 0;
	unsigned long nOutputAddr = 0;
	int nirqRet = 0;
	unsigned int CheckAddress = 0, CheckAddressMSB = 0;
	// Init ROI count which needed to process
	unsigned int nROIcount = nUserROINumber;
	// record total roi num, including fix and flexible
	unsigned int nTotalROIrecord = 0;
	unsigned int nTempVarForPrecheckOutBuf = 0;

	while (nROIcount > 0) {

		// read 0x3b4, avoid the impact of previous data
		// LOG_INF("PDA status before process = %d\n",
		//	PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDA_ERR_STAT_REG));

		if (pda_log_dbg_en == 1)
			LOG_INF("nROIcount = %d\n", nROIcount);

		// reset HW status
		for (i = 0; i < g_PDA_quantity; i++)
			PDA_devs[i].HWstatus = 0;

		// reset local variable
		nOneRoundProcROI = 0;

		//reset hw trigger number
		hw_trigger_num = g_PDA_quantity;

		// assign strategy, used for multi-engine
		if (nROIcount >= (PDA_MAXROI_PER_ROUND * g_PDA_quantity)) {
			for (i = 0; i < g_PDA_quantity; i++) {
				g_CurrentProcRoiNum[i] = PDA_MAXROI_PER_ROUND;
				nOneRoundProcROI += g_CurrentProcRoiNum[i];
				if (pda_log_dbg_en == 1)
					LOG_INF("OneRoundProcROI = %d, g_CurrentProcRoiNum[%d] = %d\n",
						nOneRoundProcROI, i, g_CurrentProcRoiNum[i]);

			}
		} else {
			if (g_PDA_quantity == 0) {
				LOG_INF("Fail: g_PDA_quantity is zero\n");
				return -1;
			}
			nRemainder = nROIcount % g_PDA_quantity;
			nFactor = nROIcount / g_PDA_quantity;

			for (i = 0; i < g_PDA_quantity; i++) {
				g_CurrentProcRoiNum[i] = nFactor;
				if (nRemainder > 0) {
					g_CurrentProcRoiNum[i]++;
					nRemainder--;
				}
				nOneRoundProcROI += g_CurrentProcRoiNum[i];
				if (pda_log_dbg_en == 1)
					LOG_INF("OneRoundProcROI = %d, g_CurrentProcRoiNum[%d] = %d\n",
						nOneRoundProcROI, i, g_CurrentProcRoiNum[i]);
			}
		}

		// data preparing
		for (i = 0; i < g_PDA_quantity; i++) {
			// current process ROI index
			if (i == 0)
				nCurrentProcRoiIndex = (nUserROINumber - nROIcount);
			else if (i > 0)
				nCurrentProcRoiIndex += g_CurrentProcRoiNum[i - 1];
			else
				LOG_INF("Index is out of range, i = %d\n", i);

			if (g_CurrentProcRoiNum[i] == 0) {
				hw_trigger_num = i;
				if (pda_log_dbg_en == 1)
					LOG_INF("assign roi number is zero, no need to process\n");
				break;
			}

			if (pda_log_dbg_en == 1)
				LOG_INF("i:%d, g_CurrentProcRoiNum:%d, nCurrentProcRoiIndex:%d\n",
					i,
					g_CurrentProcRoiNum[i],
					nCurrentProcRoiIndex);

			// calculate 1024 ROI data
			if (ProcessROIData(&g_pda_Pdadata,
					g_CurrentProcRoiNum[i],
					nCurrentProcRoiIndex) < 0) {
				LOG_INF("ProcessROIData Fail\n");
				g_pda_Pdadata.status = -24;
				return -1;
			}

			if (CheckDesignLimitation(&g_pda_Pdadata,
					g_CurrentProcRoiNum[i],
					nCurrentProcRoiIndex) < 0) {
				LOG_INF("CheckDesignLimitation Fail\n");
				return -1;
			}

			// output address is equal to
			// total ROI number multiple by OUT_BYTE_PER_ROI
			nOutputAddr = g_OutputBufferAddr[g_cur_out_idx];
			nTotalROIrecord = 0;
			for (j = 0; j < nCurrentProcRoiIndex; j++) {
				nTotalROIrecord += (g_pda_Pdadata.roi_nbx[j] * g_pda_Pdadata.roi_nby[j]);
				if (pda_log_dbg_en == 1)
					LOG_INF("j=%d, nTotalROIrecord=%d\n", j, nTotalROIrecord);
			}
			nOutputAddr += (nTotalROIrecord) * OUT_BYTE_PER_ROI;

			// pre-check output buffer size, avoid causing translation fault
			for (j = 0; j < g_CurrentProcRoiNum[i]; j++) {
				nTempVarForPrecheckOutBuf +=
				(g_pda_Pdadata.roi_nbx[nCurrentProcRoiIndex + j] *
				g_pda_Pdadata.roi_nby[nCurrentProcRoiIndex + j]);
				if (pda_log_dbg_en == 1)
					LOG_INF("j=%d, nTempVarForPrecheckOutBuf=%d\n",
						nCurrentProcRoiIndex + j, nTempVarForPrecheckOutBuf);
			}
			if (nTempVarForPrecheckOutBuf * OUT_BYTE_PER_ROI >
				g_pda_Pdadata.output_size) {
				LOG_INF("fail, output buffer out of range\n");
				LOG_INF("Current output buffer addr: 0x%lx\n",
					nOutputAddr);
				LOG_INF("PDA%d ROI process num: %d\n",
					i, g_CurrentProcRoiNum[i]);
				LOG_INF("Output buffer size: %d\n",
					g_pda_Pdadata.output_size);
				LOG_INF("Current process ROI index: %d\n",
					nCurrentProcRoiIndex);
				LOG_INF("nUserROINumber: %d\n",
					nUserROINumber);
				LOG_INF("nROIcount: %d\n", nROIcount);
				g_pda_Pdadata.status = -30;
				return -1;
			}

			if (pda_log_dbg_en == 1)
				LOG_INF("nOutputAddr: 0x%lx\n", nOutputAddr);

			FillRegSettings(&g_pda_Pdadata,
				g_CurrentProcRoiNum[i],
				nOutputAddr,
				i);
		}

		// check input/output buffer address is valid
		for (i = 0; i < hw_trigger_num; i++) {
			CheckAddress =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_BASE_ADDR_REG);
			CheckAddressMSB =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_BASE_ADDR_MSB_REG);
			if (CheckAddress == 0 && CheckAddressMSB == 0) {
				LOG_INF("PDA_%d PDA_PDAI_P1_BASE_ADDR is zero\n", i);
				g_pda_Pdadata.status = -30;
				return -1;
			}

			CheckAddress =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_BASE_ADDR_REG);
			CheckAddressMSB =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_BASE_ADDR_MSB_REG);
			if (CheckAddress == 0 && CheckAddressMSB == 0) {
				LOG_INF("PDA_%d PDA_PDATI_P1_BASE_ADDR is zero\n", i);
				g_pda_Pdadata.status = -30;
				return -1;
			}

			CheckAddress =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_BASE_ADDR_REG);
			CheckAddressMSB =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_BASE_ADDR_MSB_REG);
			if (CheckAddress == 0 && CheckAddressMSB == 0) {
				LOG_INF("PDA_%d PDA_PDAI_P2_BASE_ADDR is zero\n", i);
				g_pda_Pdadata.status = -30;
				return -1;
			}

			CheckAddress =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_BASE_ADDR_REG);
			CheckAddressMSB =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_BASE_ADDR_MSB_REG);
			if (CheckAddress == 0 && CheckAddressMSB == 0) {
				LOG_INF("PDA_%d PDA_PDATI_P2_BASE_ADDR is zero\n", i);
				g_pda_Pdadata.status = -30;
				return -1;
			}

			CheckAddress =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_BASE_ADDR_REG);
			CheckAddressMSB =
			PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_BASE_ADDR_MSB_REG);
			if (CheckAddress == 0 && CheckAddressMSB == 0) {
				LOG_INF("PDA_%d PDA_PDAO_P1_BASE_ADDR is zero\n", i);
				g_pda_Pdadata.status = -30;
				return -1;
			}

			// blending part
			if (g_B_N > 0) {
				CheckAddress =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_BASE_ADDR_REG);
				CheckAddressMSB =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_BASE_ADDR_MSB_REG);
				if (CheckAddress == 0 && CheckAddressMSB == 0) {
					LOG_INF("PDA_%d PDA_PDALI_P3_BASE_ADDR B1 is zero\n", i);
					g_pda_Pdadata.status = -30;
					return -1;
				}

				CheckAddress =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_BASE_ADDR_REG);
				CheckAddressMSB =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_BASE_ADDR_MSB_REG);
				if (CheckAddress == 0 && CheckAddressMSB == 0) {
					LOG_INF("PDA_%d PDA_PDARI_P3_BASE_ADDR B1 is zero\n", i);
					g_pda_Pdadata.status = -30;
					return -1;
				}
			}

			if (g_B_N > 1) {
				CheckAddress =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_BASE_ADDR_REG);
				CheckAddressMSB =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_BASE_ADDR_MSB_REG);
				if (CheckAddress == 0 && CheckAddressMSB == 0) {
					LOG_INF("PDA_%d PDA_PDALI_P4_BASE_ADDR B2 is zero\n", i);
					g_pda_Pdadata.status = -30;
					return -1;
				}

				CheckAddress =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_BASE_ADDR_REG);
				CheckAddressMSB =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_BASE_ADDR_MSB_REG);
				if (CheckAddress == 0 && CheckAddressMSB == 0) {
					LOG_INF("PDA_%d PDA_PDARI_P4_BASE_ADDR B2 is zero\n", i);
					g_pda_Pdadata.status = -30;
					return -1;
				}
			}

			if (g_B_N > 2) {
				CheckAddress =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_BASE_ADDR_REG);
				CheckAddressMSB =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_BASE_ADDR_MSB_REG);
				if (CheckAddress == 0 && CheckAddressMSB == 0) {
					LOG_INF("PDA_%d PDA_PDALI_P5_BASE_ADDR B3 is zero\n", i);
					g_pda_Pdadata.status = -30;
					return -1;
				}

				CheckAddress =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_BASE_ADDR_REG);
				CheckAddressMSB =
				PDA_RD32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_BASE_ADDR_MSB_REG);
				if (CheckAddress == 0 && CheckAddressMSB == 0) {
					LOG_INF("PDA_%d PDA_PDARI_P5_BASE_ADDR B3 is zero\n", i);
					g_pda_Pdadata.status = -30;
					return -1;
				}
			}
		}

		// trigger PDA work
		pda_execute(hw_trigger_num);

		nirqRet = pda_wait_irq(&g_pda_Pdadata, hw_trigger_num);

		if (nirqRet < 0) {
			LOG_INF("pda_wait_irq Fail (%d)\n", nirqRet);
			return -1;
		}

		// update roi count which needed to process
		nROIcount -= nOneRoundProcROI;
	}

	if (pda_log_dbg_en == 1) {
		LOG_INF("total output size is %d bytes\n",
			nTempVarForPrecheckOutBuf * OUT_BYTE_PER_ROI);
	}

	return 1;
}

static bool findfd(int cur_fd, int fd_rec[], int size, int *fd_idx)
{
	int i = 0;

	for (i = 0; i < size; ++i) {
		if (pda_log_dbg_en == 1)
			LOG_INF("index:%d, fd:%d\n", i, fd_rec[i]);

		if (fd_rec[i] == 0) {
			// fd isn't found and pass the current index
			if (i == 0)
				LOG_INF("no fd in array table\n");
			*fd_idx = i;
			return false;
		}

		if (cur_fd == fd_rec[i]) {
			if (pda_log_dbg_en == 1)
				LOG_INF("find fd:%d in array, index:%d\n",
					cur_fd, i);
			*fd_idx = i;
			return true;
		}
	}

	// did not find fd in array
	*fd_idx = size;

	return false;
}

static void free_alldmabufferandfdarraytable(void)
{

	int i = 0;

	if (pda_log_dbg_en == 1)
		LOG_INF("+\n");

	// ------------- free image related (include blending) -------------------
	for (i = 0; i < ARRAY_SIZE(fd_l_img_rec); ++i) {
		if (pda_log_dbg_en == 1)
			LOG_INF("index:%d, img fd:%d\n", i, fd_l_img_rec[i]);

		if (fd_l_img_rec[i] == 0) {
			// array is empty
			if (pda_log_dbg_en == 1)
				LOG_INF("unmap img done\n");
			break;
		}
		fd_l_img_rec[i] = 0;
		pda_put_dma_buffer(&g_image_mmu[i]);
		g_Address_LI[i] = 0;
		g_Address_RI[i] = 0;
	}

	// ------------- free table related -------------------
	for (i = 0; i < ARRAY_SIZE(fd_l_tbl_rec); ++i) {
		if (pda_log_dbg_en == 1)
			LOG_INF("index:%d, tbl fd:%d\n", i, fd_l_tbl_rec[i]);

		if (fd_l_tbl_rec[i] == 0) {
			// array is empty
			if (pda_log_dbg_en == 1)
				LOG_INF("unmap tbl done\n");
			break;
		}
		fd_l_tbl_rec[i] = 0;
		pda_put_dma_buffer(&g_table_mmu[i]);
		g_Address_LT[i] = 0;
		g_Address_RT[i] = 0;
	}

	// ------------- free output related -------------------
	for (i = 0; i < ARRAY_SIZE(fd_out_rec); ++i) {
		if (pda_log_dbg_en == 1)
			LOG_INF("index:%d, output fd:%d\n", i, fd_out_rec[i]);

		if (fd_out_rec[i] == 0) {
			// array is empty
			if (pda_log_dbg_en == 1)
				LOG_INF("unmap output done\n");
			break;
		}
		fd_out_rec[i] = 0;
		pda_put_dma_buffer(&g_output_mmu[i]);
		g_OutputBufferAddr[i] = 0;
	}

	//clear ring buffer index
	g_ring_img_idx = 0;
	g_ring_tbl_idx = 0;
	g_ring_out_idx = 0;

	if (pda_log_dbg_en == 1)
		LOG_INF("-\n");
}

static long PDA_Ioctl(struct file *a_pstFile,
			unsigned int a_u4Command,
			unsigned long a_u4Param)
{
	long nRet = 0;
	unsigned int nUserROINumber = 0;
	unsigned int i;
	int ret = 0;
	struct PDA_Init_Data Init_Data;

	unsigned int cur_img_idx = 0;
	unsigned int cur_tbl_idx = 0;
	unsigned int cur_out_idx = 0;

	if (g_PDA_quantity == 0) {
		LOG_INF("no PDA support\n");
		return -1;
	}

	switch (a_u4Command) {
	case PDA_GET_VERSION:

		if (copy_from_user(&Init_Data,
				   (void *)a_u4Param,
				   sizeof(struct PDA_Init_Data)) != 0) {
			LOG_INF("PDA_GET_VERSION copy_from_user failed\n");
			nRet = -EFAULT;
			break;
		}

		Init_Data.Kversion = PDA_KERNEL_VERSION;
		LOG_INF("kernel version: %d\n", Init_Data.Kversion);

		if (copy_to_user((void *)a_u4Param,
		    &Init_Data,
		    sizeof(struct PDA_Init_Data)) != 0) {
			LOG_INF("PDA_GET_VERSION copy_to_user failed\n");
			nRet = -EFAULT;
		}

		break;
	case PDA_ENQUE_WAITIRQ:
		if (pda_log_dbg_en == 1)
			LOG_INF("PDA_ENQUE_WAITIRQ\n");

		spin_lock(&g_PDA_SpinLock);
		if (g_u4EnableClockCount == 0) {
			LOG_INF("Cannot process without enable pda clock\n");
			spin_unlock(&g_PDA_SpinLock);
			return -1;
		}
		spin_unlock(&g_PDA_SpinLock);

#ifdef GET_PDA_TIME
		ktime_get_real_ts64(&total_time_begin);
#endif

#ifndef FPGA_UT
		// MRAW PDA reset
		for (i = 0; i < g_PDA_quantity; i++)
			pda_nontransaction_reset(i);
#endif

		// reset HW status
		for (i = 0; i < g_PDA_quantity; i++)
			PDA_devs[i].HWstatus = 0;

#ifdef CHECK_IRQ_COUNT
		// reset PDA0/PDA1 IRQ count
		g_PDA0_IRQCount = 0;
		g_PDA1_IRQCount = 0;
#endif

		if (copy_from_user(&g_pda_Pdadata,
				   (void *)a_u4Param,
				   sizeof(struct PDA_Data_t)) != 0) {
			LOG_INF("PDA_ENQUE_WAITIRQ copy_from_user failed\n");
			nRet = -EFAULT;
			break;
		}

		/* Protect the Multi Process */
		mutex_lock(&pda_mutex);

		ret = g_pda_Pdadata.roi_num == 0;
		if (g_pda_Pdadata.roi_num > kFlexibleROIMaxNum || ret) {
			g_pda_Pdadata.status = -28;
			LOG_INF("ROI number out of range, roi_num:%d\n",
				g_pda_Pdadata.roi_num);
			LOG_INF("roi_h:%d,out_siz:%d,FD:%d,cfg0:0x%x,sta:%d/%d\n",
				g_pda_Pdadata.roi_h[0],
				g_pda_Pdadata.output_size,
				g_pda_Pdadata.fd_output,
				g_pda_Pdadata.PDA_HW_Register.PDA_CFG_0.Raw,
				g_pda_Pdadata.status,
				g_pda_Pdadata.timeout);
			goto EXIT;
		}

#ifdef PDA_MMQOS
		pda_mmqos_bw_set(&g_pda_Pdadata);
#endif

		// Record all fd info.
		g_cur_li_fd[0] = g_pda_Pdadata.fd_left_image[0];
		g_cur_lt_fd = g_pda_Pdadata.fd_left_table;
		g_cur_out_fd = g_pda_Pdadata.fd_output;

		g_B_N = g_pda_Pdadata.PDA_HW_Register.PDA_CFG_14.Bits.PDA_B_N;
		switch (g_B_N) {
		case 3:
			g_cur_li_fd[3] = g_pda_Pdadata.fd_left_image[3];
			if (pda_log_dbg_en == 1)
				LOG_INF("sync blending 3, image fd\n");
			fallthrough;
		case 2:
			g_cur_li_fd[2] = g_pda_Pdadata.fd_left_image[2];
			if (pda_log_dbg_en == 1)
				LOG_INF("sync blending 2, image fd\n");
			fallthrough;
		case 1:
			g_cur_li_fd[1] = g_pda_Pdadata.fd_left_image[1];
			if (pda_log_dbg_en == 1)
				LOG_INF("sync blending 1, image fd\n");
			break;
		default:
			if (pda_log_dbg_en == 1)
				LOG_INF("no need to sync blending fd\n");
			break;
		}

		// image fd---------------------------------
		// Find the index of img fd in fd array table and store it in cur_img_idx
		if (pda_log_dbg_en == 1)
			LOG_INF("img fd buffer check\n");
		if (findfd(g_cur_li_fd[0], fd_l_img_rec, ARRAY_SIZE(fd_l_img_rec),
				&cur_img_idx)) {
			if (pda_log_dbg_en == 1)
				LOG_INF("find fd in img array, fd:%d, idx:%d, MVA = 0x%lx\n",
					g_cur_li_fd[0],
					cur_img_idx,
					g_Address_LI[cur_img_idx]);

		} else {
			if (cur_img_idx == ARRAY_SIZE(fd_l_img_rec))
				cur_img_idx = g_ring_img_idx;

			// override, need to unmap buffer first
			if (fd_l_img_rec[cur_img_idx] > 0) {
				pda_put_dma_buffer(&g_image_mmu[cur_img_idx]);
				if (pda_log_dbg_en == 1)
					LOG_INF("clear, img array, fd:%d, idx:%d, MVA = 0x%lx\n",
						fd_l_img_rec[cur_img_idx],
						cur_img_idx,
						g_Address_LI[cur_img_idx]);
				fd_l_img_rec[cur_img_idx] = 0;
				g_Address_LI[cur_img_idx] = 0;
			}

			// input buffer mapping iova
			if (pda_get_dma_buffer(&g_image_mmu[cur_img_idx],
					g_cur_li_fd[0]) < 0) {
				LOG_INF("Left image, pda_get_dma_buffer fail!\n");
				g_pda_Pdadata.status = -26;
				goto EXIT;
			}
			g_Address_LI[cur_img_idx] =
				(unsigned long) sg_dma_address(g_image_mmu[cur_img_idx].sgt->sgl) +
				+ g_pda_Pdadata.address_offset[0];

			// update fd array table
			fd_l_img_rec[cur_img_idx] = g_cur_li_fd[0];

			// update g_ring_img_idx
			g_ring_img_idx++;
			if (g_ring_img_idx >= (unsigned int)ARRAY_SIZE(fd_l_img_rec))
				g_ring_img_idx = 0;

			if (pda_log_dbg_en == 1) {
				LOG_INF("new, img array, fd:%d, idx:%d, MVA = 0x%lx\n",
					g_cur_li_fd[0],
					cur_img_idx,
					g_Address_LI[cur_img_idx]);
				LOG_INF("g_ring_img_idx: %d, ARRAY_SIZE(fd_l_img_rec):%d\n",
					g_ring_img_idx, (unsigned int)ARRAY_SIZE(fd_l_img_rec));
			}
		}

		for (i = 0; i < g_PDA_quantity; i++) {
			// Left image buffer
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_BASE_ADDR_MSB_REG,
				(unsigned int)(g_Address_LI[cur_img_idx] >> 32));
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P1_BASE_ADDR_REG,
				(unsigned int)(g_Address_LI[cur_img_idx]));

			// Right image buffer
			g_Address_RI[cur_img_idx] =
				g_Address_LI[cur_img_idx] + g_pda_Pdadata.image_size;
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_BASE_ADDR_MSB_REG,
				(unsigned int)(g_Address_RI[cur_img_idx] >> 32));
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAI_P2_BASE_ADDR_REG,
				(unsigned int)(g_Address_RI[cur_img_idx]));
		}

		// image blending fd---------------------------------
		if (pda_log_dbg_en == 1)
			LOG_INF("img blending fd buffer check\n");
		switch (g_B_N) {
		case 3:
			if (pda_log_dbg_en == 1)
				LOG_INF("blending 3, image fd mapping process\n");

			// Find the index of img fd and store it in cur_img_idx
			if (findfd(g_cur_li_fd[3], fd_l_img_rec, ARRAY_SIZE(fd_l_img_rec),
					&cur_img_idx)) {
				if (pda_log_dbg_en == 1)
					LOG_INF("find b3 fd in img array, fd:%d, idx:%d, MVA = 0x%lx\n",
						g_cur_li_fd[3],
						cur_img_idx,
						g_Address_LI[cur_img_idx]);

			} else {
				if (cur_img_idx == ARRAY_SIZE(fd_l_img_rec))
					cur_img_idx = g_ring_img_idx;

				// override, need to unmap buffer first
				if (fd_l_img_rec[cur_img_idx] > 0) {
					pda_put_dma_buffer(&g_image_mmu[cur_img_idx]);
					if (pda_log_dbg_en == 1)
						LOG_INF("clear, img array b3, fd:%d, idx:%d, MVA = 0x%lx\n",
							fd_l_img_rec[cur_img_idx],
							cur_img_idx,
							g_Address_LI[cur_img_idx]);
					fd_l_img_rec[cur_img_idx] = 0;
					g_Address_LI[cur_img_idx] = 0;
				}

				// input buffer mapping iova
				if (pda_get_dma_buffer(&g_image_mmu[cur_img_idx],
						g_cur_li_fd[3]) < 0) {
					LOG_INF("Left image b3, pda_get_dma_buffer fail!\n");
					g_pda_Pdadata.status = -26;
					goto EXIT;
				}
				g_Address_LI[cur_img_idx] =
					(unsigned long) sg_dma_address(
						g_image_mmu[cur_img_idx].sgt->sgl) +
						g_pda_Pdadata.address_offset[3];

				// update fd array table
				fd_l_img_rec[cur_img_idx] = g_cur_li_fd[3];

				// update g_ring_img_idx
				g_ring_img_idx++;
				if (g_ring_img_idx >= ARRAY_SIZE(fd_l_img_rec))
					g_ring_img_idx = 0;

				if (pda_log_dbg_en == 1) {
					LOG_INF("new, img array b3, fd:%d, idx:%d, MVA = 0x%lx\n",
						g_cur_li_fd[3],
						cur_img_idx,
						g_Address_LI[cur_img_idx]);
					LOG_INF("g_ring_img_idx: %d\n", g_ring_img_idx);
				}
			}

			for (i = 0; i < g_PDA_quantity; i++) {
				// Left image buffer
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_BASE_ADDR_MSB_REG,
					(unsigned int)(g_Address_LI[cur_img_idx] >> 32));
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P5_BASE_ADDR_REG,
					(unsigned int)(g_Address_LI[cur_img_idx]));

				// Right image buffer
				g_Address_RI[cur_img_idx] =
					g_Address_LI[cur_img_idx] + g_pda_Pdadata.image_size;
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_BASE_ADDR_MSB_REG,
					(unsigned int)(g_Address_RI[cur_img_idx] >> 32));
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P5_BASE_ADDR_REG,
					(unsigned int)(g_Address_RI[cur_img_idx]));
			}

			fallthrough;
		case 2:
			if (pda_log_dbg_en == 1)
				LOG_INF("blending 2, image fd mapping process\n");

			// Find the index of img fd and store it in cur_img_idx
			if (findfd(g_cur_li_fd[2], fd_l_img_rec, ARRAY_SIZE(fd_l_img_rec),
					&cur_img_idx)) {
				// Find the index of img fd and store it in cur_img_idx
				if (pda_log_dbg_en == 1)
					LOG_INF("find b2 fd in img array, fd:%d, idx:%d, MVA = 0x%lx\n",
						g_cur_li_fd[2],
						cur_img_idx,
						g_Address_LI[cur_img_idx]);

			} else {
				if (cur_img_idx == ARRAY_SIZE(fd_l_img_rec))
					cur_img_idx = g_ring_img_idx;

				// override, need to unmap buffer first
				if (fd_l_img_rec[cur_img_idx] > 0) {
					pda_put_dma_buffer(&g_image_mmu[cur_img_idx]);
					if (pda_log_dbg_en == 1)
						LOG_INF("clear, img array b2, fd:%d, idx:%d, MVA = 0x%lx\n",
							fd_l_img_rec[cur_img_idx],
							cur_img_idx,
							g_Address_LI[cur_img_idx]);
					fd_l_img_rec[cur_img_idx] = 0;
					g_Address_LI[cur_img_idx] = 0;
				}

				// input buffer mapping iova
				if (pda_get_dma_buffer(&g_image_mmu[cur_img_idx],
						g_cur_li_fd[2]) < 0) {
					LOG_INF("Left image b2, pda_get_dma_buffer fail!\n");
					g_pda_Pdadata.status = -26;
					goto EXIT;
				}
				g_Address_LI[cur_img_idx] =
					(unsigned long) sg_dma_address(
						g_image_mmu[cur_img_idx].sgt->sgl) +
						g_pda_Pdadata.address_offset[2];

				// update fd array table
				fd_l_img_rec[cur_img_idx] = g_cur_li_fd[2];

				// update g_ring_img_idx
				g_ring_img_idx++;
				if (g_ring_img_idx >= ARRAY_SIZE(fd_l_img_rec))
					g_ring_img_idx = 0;

				if (pda_log_dbg_en == 1) {
					LOG_INF("new, img array b2, fd:%d, idx:%d, MVA = 0x%lx\n",
						g_cur_li_fd[2],
						cur_img_idx,
						g_Address_LI[cur_img_idx]);
					LOG_INF("g_ring_img_idx: %d\n", g_ring_img_idx);
				}
			}

			for (i = 0; i < g_PDA_quantity; i++) {
				// Left image buffer
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_BASE_ADDR_MSB_REG,
					(unsigned int)(g_Address_LI[cur_img_idx] >> 32));
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P4_BASE_ADDR_REG,
					(unsigned int)(g_Address_LI[cur_img_idx]));

				// Right image buffer
				g_Address_RI[cur_img_idx] =
					g_Address_LI[cur_img_idx] + g_pda_Pdadata.image_size;
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_BASE_ADDR_MSB_REG,
					(unsigned int)(g_Address_RI[cur_img_idx] >> 32));
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P4_BASE_ADDR_REG,
					(unsigned int)(g_Address_RI[cur_img_idx]));
			}

			fallthrough;
		case 1:
			if (pda_log_dbg_en == 1)
				LOG_INF("blending 1, image fd mapping process\n");

			// Find the index of img fd and store it in cur_img_idx
			if (findfd(g_cur_li_fd[1], fd_l_img_rec, ARRAY_SIZE(fd_l_img_rec),
					&cur_img_idx)) {
				// Find the index of img fd and store it in cur_img_idx
				if (pda_log_dbg_en == 1)
					LOG_INF("find b1 fd in img array, fd:%d, idx:%d, MVA = 0x%lx\n",
						g_cur_li_fd[1],
						cur_img_idx,
						g_Address_LI[cur_img_idx]);

			} else {
				if (cur_img_idx == ARRAY_SIZE(fd_l_img_rec))
					cur_img_idx = g_ring_img_idx;

				// override, need to unmap buffer first
				if (fd_l_img_rec[cur_img_idx] > 0) {
					pda_put_dma_buffer(&g_image_mmu[cur_img_idx]);
					if (pda_log_dbg_en == 1)
						LOG_INF("clear, img array b1, fd:%d, idx:%d, MVA = 0x%lx\n",
							fd_l_img_rec[cur_img_idx],
							cur_img_idx,
							g_Address_LI[cur_img_idx]);
					fd_l_img_rec[cur_img_idx] = 0;
					g_Address_LI[cur_img_idx] = 0;
				}

				// input buffer mapping iova
				if (pda_get_dma_buffer(&g_image_mmu[cur_img_idx],
						g_cur_li_fd[1]) < 0) {
					LOG_INF("Left image b1, pda_get_dma_buffer fail!\n");
					g_pda_Pdadata.status = -26;
					goto EXIT;
				}
				g_Address_LI[cur_img_idx] =
					(unsigned long) sg_dma_address(
						g_image_mmu[cur_img_idx].sgt->sgl) +
						g_pda_Pdadata.address_offset[1];

				// update fd array table
				fd_l_img_rec[cur_img_idx] = g_cur_li_fd[1];

				// update g_ring_img_idx
				g_ring_img_idx++;
				if (g_ring_img_idx >= ARRAY_SIZE(fd_l_img_rec))
					g_ring_img_idx = 0;

				if (pda_log_dbg_en == 1) {
					LOG_INF("new, img array b1, fd:%d, idx:%d, MVA = 0x%lx\n",
						g_cur_li_fd[1],
						cur_img_idx,
						g_Address_LI[cur_img_idx]);
					LOG_INF("g_ring_img_idx: %d\n", g_ring_img_idx);
				}
			}

			for (i = 0; i < g_PDA_quantity; i++) {
				// Left image buffer
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_BASE_ADDR_MSB_REG,
					(unsigned int)(g_Address_LI[cur_img_idx] >> 32));
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDALI_P3_BASE_ADDR_REG,
					(unsigned int)(g_Address_LI[cur_img_idx]));

				// Right image buffer
				g_Address_RI[cur_img_idx] =
					g_Address_LI[cur_img_idx] + g_pda_Pdadata.image_size;
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_BASE_ADDR_MSB_REG,
					(unsigned int)(g_Address_RI[cur_img_idx] >> 32));
				PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDARI_P3_BASE_ADDR_REG,
					(unsigned int)(g_Address_RI[cur_img_idx]));
			}

			break;
		default:
			if (pda_log_dbg_en == 1)
				LOG_INF("no need to find blending fd\n");
			break;
		}

		//table fd---------------------------------
		if (pda_log_dbg_en == 1)
			LOG_INF("tbl fd buffer check\n");

		if (findfd(g_cur_lt_fd, fd_l_tbl_rec, ARRAY_SIZE(fd_l_tbl_rec),
				&cur_tbl_idx)) {
			if (pda_log_dbg_en == 1)
				LOG_INF("find fd in tbl array, fd:%d, idx:%d, MVA = 0x%lx\n",
					g_cur_lt_fd,
					cur_tbl_idx,
					g_Address_LT[cur_tbl_idx]);

		} else {
			if (cur_tbl_idx == ARRAY_SIZE(fd_l_tbl_rec))
				cur_tbl_idx = g_ring_tbl_idx;

			// override, need to unmap buffer first
			if (fd_l_tbl_rec[cur_tbl_idx] > 0) {
				pda_put_dma_buffer(&g_table_mmu[cur_tbl_idx]);
				if (pda_log_dbg_en == 1)
					LOG_INF("clear, tbl array, fd:%d, idx:%d, MVA = 0x%lx\n",
						fd_l_tbl_rec[cur_tbl_idx],
						cur_tbl_idx,
						g_Address_LT[cur_tbl_idx]);
				fd_l_tbl_rec[cur_tbl_idx] = 0;
				g_Address_LT[cur_tbl_idx] = 0;
			}

			// input buffer mapping iova
			if (pda_get_dma_buffer(&g_table_mmu[cur_tbl_idx],
					g_cur_lt_fd) < 0) {
				LOG_INF("Left table, pda_get_dma_buffer fail!\n");
				g_pda_Pdadata.status = -26;
				goto EXIT;
			}
			g_Address_LT[cur_tbl_idx] =
				(unsigned long) sg_dma_address(g_table_mmu[cur_tbl_idx].sgt->sgl);

			// update fd array table
			fd_l_tbl_rec[cur_tbl_idx] = g_cur_lt_fd;

			// update g_ring_tbl_idx
			g_ring_tbl_idx++;
			if (g_ring_tbl_idx >= ARRAY_SIZE(fd_l_tbl_rec))
				g_ring_tbl_idx = 0;

			if (pda_log_dbg_en == 1) {
				LOG_INF("new, tbl array, fd:%d, idx:%d, MVA = 0x%lx\n",
					g_cur_lt_fd,
					cur_tbl_idx,
					g_Address_LT[cur_tbl_idx]);
					LOG_INF("g_ring_tbl_idx: %d\n", g_ring_tbl_idx);
			}
		}

		for (i = 0; i < g_PDA_quantity; i++) {
			// Left image buffer
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_BASE_ADDR_MSB_REG,
				(unsigned int)(g_Address_LT[cur_tbl_idx] >> 32));
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P1_BASE_ADDR_REG,
				(unsigned int)(g_Address_LT[cur_tbl_idx]));

			// Right image buffer
			g_Address_RT[cur_tbl_idx] =
				g_Address_LT[cur_tbl_idx] + g_pda_Pdadata.table_size;
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_BASE_ADDR_MSB_REG,
				(unsigned int)(g_Address_RT[cur_tbl_idx] >> 32));
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDATI_P2_BASE_ADDR_REG,
				(unsigned int)(g_Address_RT[cur_tbl_idx]));
		}

		//output fd---------------------------------
		if (pda_log_dbg_en == 1)
			LOG_INF("output fd buffer check\n");

		if (findfd(g_cur_out_fd, fd_out_rec, ARRAY_SIZE(fd_out_rec),
				&cur_out_idx) && g_pda_Pdadata.is_outputBuffer_updated == 0) {
			if (pda_log_dbg_en == 1)
				LOG_INF("find fd in output array, fd:%d, idx:%d, MVA = 0x%lx\n",
					g_cur_out_fd,
					cur_out_idx,
					g_OutputBufferAddr[cur_out_idx]);

		} else {
			// override, need to unmap buffer first
			if (cur_out_idx == ARRAY_SIZE(fd_out_rec))
				cur_out_idx = g_ring_out_idx;

			if (fd_out_rec[cur_out_idx] > 0) {
				// override, need to unmap buffer first
				pda_put_dma_buffer(&g_output_mmu[cur_out_idx]);
				if (pda_log_dbg_en == 1)
					LOG_INF("clear, output array, fd:%d, idx:%d, MVA = 0x%lx\n",
						fd_out_rec[cur_out_idx],
						cur_out_idx,
						g_OutputBufferAddr[cur_out_idx]);
				fd_out_rec[cur_out_idx] = 0;
				g_OutputBufferAddr[cur_out_idx] = 0;
			}

			// output buffer mapping iova
			if (pda_get_dma_buffer(&g_output_mmu[cur_out_idx],
					g_cur_out_fd) < 0) {
				LOG_INF("Output, pda_get_dma_buffer fail!\n");
				g_pda_Pdadata.status = -27;
				goto EXIT;
			}
			g_OutputBufferAddr[cur_out_idx] =
				(unsigned long) sg_dma_address(g_output_mmu[cur_out_idx].sgt->sgl);

			// update fd array table
			fd_out_rec[cur_out_idx] = g_cur_out_fd;

			// update g_ring_out_idx
			g_ring_out_idx++;
			if (g_ring_out_idx >= ARRAY_SIZE(fd_out_rec))
				g_ring_out_idx = 0;

			if (pda_log_dbg_en == 1) {
				LOG_INF("new, output array, fd:%d, idx:%d, MVA = 0x%lx\n",
					g_cur_out_fd,
					cur_out_idx,
					g_OutputBufferAddr[cur_out_idx]);
					LOG_INF("g_ring_out_idx: %d\n", g_ring_out_idx);
			}
		}

		for (i = 0; i < g_PDA_quantity; i++) {
			// Left image buffer
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_BASE_ADDR_MSB_REG,
				(unsigned int)(g_OutputBufferAddr[cur_out_idx] >> 32));
			PDA_WR32(PDA_devs[i].m_pda_base + PDA_PDAO_P1_BASE_ADDR_REG,
				(unsigned int)(g_OutputBufferAddr[cur_out_idx]));
		}

		// cur_out_idx will be used in PDAProcessFunction
		g_cur_out_idx = cur_out_idx;

		// buffer control
		if (pda_log_dbg_en == 1) {
			LOG_INF("Last/sensor dev: %d/%d, isBufferUpdated in/out: %d/%d\n",
						g_last_sensor_dev,
						g_pda_Pdadata.sensor_dev,
						g_pda_Pdadata.is_inputBuffer_updated,
						g_pda_Pdadata.is_outputBuffer_updated);
		}

		// update sensor dev
		g_last_sensor_dev = g_pda_Pdadata.sensor_dev;

		HWDMASettings(&g_pda_Pdadata);

		// ------------------------ PDA pre-process done -----------------

		// Process flexible roi
		nUserROINumber = g_pda_Pdadata.roi_num;

		if (pda_log_dbg_en == 1) {
			LOG_INF("nUserROINumber = %d, g_PDA_quantity = %d\n",
				nUserROINumber, g_PDA_quantity);
		}

#ifdef CHECK_IRQ_COUNT
		// setting reasonable IRQ count
		g_reasonable_IRQCount = 1 +
			(int)((nUserROINumber-1)/(PDA_MAXROI_PER_ROUND*g_PDA_quantity));
		// reset PDA0/PDA1 IRQ count
		g_PDA0_IRQCount = 0;
		g_PDA1_IRQCount = 0;
#endif

		if (PDAProcessFunction(nUserROINumber, 0) < 0) {
			LOG_INF("Flexible ROI, PDA process fail\n");
			goto EXIT;
		}

//////////////////////////////////////////////////////////////////////////////
EXIT:
		if (pda_log_dbg_en == 1)
			LOG_INF("Exit\n");

		mutex_unlock(&pda_mutex);

#ifdef GET_PDA_TIME
		// for compute pda process time
		ktime_get_real_ts64(&total_time_end);

		LOG_INF("PDA 1 execute time (%d)\n",
			(pda1_done_e.tv_nsec - pda1_done_b.tv_nsec)/1000);
		LOG_INF("PDA 2 execute time (%d)\n",
			(pda2_done_e.tv_nsec - pda2_done_b.tv_nsec)/1000);
		LOG_INF("SW wait time (%d)\n",
			(time_end.tv_nsec - pda1_done_b.tv_nsec)/1000);
		LOG_INF("kernel total cost time (%d)\n",
		(total_time_end.tv_nsec-total_time_begin.tv_nsec)/1000);
#endif

		if (copy_to_user((void *)a_u4Param,
		    &g_pda_Pdadata,
		    sizeof(struct PDA_Data_t)) != 0) {
			LOG_INF("copy_to_user failed\n");
			nRet = -EFAULT;
		}
		break;
	case PDA_PUT_DMA_BUF:
		// all fd array and iova array need to be cleared and unmap
		free_alldmabufferandfdarraytable();
		break;
	default:
		LOG_INF("Unknown Cmd(%d)\n", a_u4Command);
		break;
	}
	return nRet;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long PDA_Ioctl_Compat(struct file *a_pstFile, unsigned int a_u4Command,
			    unsigned long a_u4Param)
{
	long i4RetValue = 0;
	return i4RetValue;
}
#endif

static int PDA_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
	//Enable clock
	EnableClock(MTRUE);
	spin_lock(&g_PDA_SpinLock);
	LOG_INF("PDA open g_u4EnableClockCount: %d", g_u4EnableClockCount);
	spin_unlock(&g_PDA_SpinLock);

#ifdef CHECK_IRQ_COUNT
	g_reasonable_IRQCount = 0;
	g_PDA0_IRQCount = 0;
	g_PDA1_IRQCount = 0;
#endif

	return 0;
}

static int PDA_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	// all fd array and iova array need to be cleared and unmap
	free_alldmabufferandfdarraytable();

	//Disable clock
	EnableClock(MFALSE);
	spin_lock(&g_PDA_SpinLock);
	LOG_INF("PDA release g_u4EnableClockCount: %d", g_u4EnableClockCount);
	spin_unlock(&g_PDA_SpinLock);
	return 0;
}

static struct class *pda_class;
static struct device *pda_device;
static dev_t pda_devno;
/* torch status sysfs */
static ssize_t pda_debug_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t pda_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int val = 0, ret = 0;

	ret = kstrtoint(buf, 10, &val);
	if (ret < 0)
		LOG_INF("ret fail\n");

	pda_log_dbg_en = val & 0x1;

	LOG_INF("log(%d), buf:%s\n",
		pda_log_dbg_en,
		buf);

	// enable/disable pda_mtXXXX.c's debug log
	pda_debug_log(pda_log_dbg_en);

	return size;
}
static DEVICE_ATTR_RW(pda_debug);
/*******************************************************************************
 *
 ******************************************************************************/

static dev_t g_PDA_devno;
static struct cdev *g_pPDA_CharDrv;
static struct class *actuator_class;
static struct device *lens_device;

static const struct file_operations g_stPDA_fops = {
	.owner = THIS_MODULE,
	.open = PDA_Open,
	.release = PDA_Release,
	.unlocked_ioctl = PDA_Ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = PDA_Ioctl_Compat,
#endif
};

static inline int PDA_RegCharDev(void)
{
	int nRet = 0;

	LOG_INF("Register char driver Start\n");

	/* Allocate char driver no. */
	nRet = alloc_chrdev_region(&g_PDA_devno, 0, 1, PDA_DEV_NAME);
	if (nRet < 0) {
		LOG_INF("Allocate device no failed\n");
		return nRet;
	}

	/* Allocate driver */
	g_pPDA_CharDrv = cdev_alloc();
	if (g_pPDA_CharDrv == NULL) {
		unregister_chrdev_region(g_PDA_devno, 1);
		LOG_INF("cdev_alloc failed\n");
		return nRet;
	}

	/* Attach file operation. */
	cdev_init(g_pPDA_CharDrv, &g_stPDA_fops);

	g_pPDA_CharDrv->owner = THIS_MODULE;

	/* Add to system */
	nRet = cdev_add(g_pPDA_CharDrv, g_PDA_devno, 1);
	if (nRet < 0) {
		LOG_INF("Attach file operation failed\n");
		unregister_chrdev_region(g_PDA_devno, 1);
		return nRet;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	actuator_class = class_create("PDAdrv");
#else
	actuator_class = class_create(THIS_MODULE, "PDAdrv");
#endif
	if (IS_ERR(actuator_class)) {
		int ret = PTR_ERR(actuator_class);

		LOG_INF("Unable to create class, err = %d\n", ret);
		// unregister_chrdev_region(g_PDA_devno, 1);
		return ret;
	}

	lens_device = device_create(actuator_class, NULL, g_PDA_devno, NULL,
				    PDA_DEV_NAME);

	if (IS_ERR(lens_device)) {
		int ret = PTR_ERR(lens_device);

		LOG_INF("create dev err: /dev/%s, err = %d\n", PDA_DEV_NAME, ret);
		// unregister_chrdev_region(g_PDA_devno, 1);
		return ret;
	}

	LOG_INF("Register char driver End\n");
	return nRet;
}

static inline void PDA_UnRegCharDev(void)
{
	LOG_INF("UnRegCharDev Start\n");

	/* Release char driver */
	cdev_del(g_pPDA_CharDrv);

	unregister_chrdev_region(g_PDA_devno, 1);

	device_destroy(actuator_class, g_PDA_devno);

	class_destroy(actuator_class);

	LOG_INF("UnRegCharDev End\n");
}

/*****************************************************************************
 *
 ****************************************************************************/
static int PDA_probe(struct platform_device *pdev)
{
	int nRet = 0;
	unsigned int irq_info[3];	/* Record interrupts info from device tree */
	struct device_node *node;

	LOG_INF("probe Start\n");

	// init pda quantity
	g_PDA_quantity = 0;

	/* Register char driver */
	nRet = PDA_RegCharDev();
	if (nRet < 0) {
		LOG_INF(" register char device failed!\n");
		return nRet;
	}

	LOG_INF("probe - register char driver\n");

	spin_lock_init(&g_PDA_SpinLock);
	LOG_INF("spin_lock_init done\n");

	init_waitqueue_head(&g_wait_queue_head);
	LOG_INF("init_waitqueue_head done\n");

	//PDA node
	node = of_find_compatible_node(NULL, NULL, "mediatek,camera-pda");
	if (!node) {
		LOG_INF("find camera-pda node failed\n");
		return -1;
	}
	LOG_INF("find camera-pda node done\n");

	// must porting in dts
	pda_init_larb(pdev);

#if IS_ENABLED(CONFIG_OF)
	g_dev1 = &pdev->dev;

	if (dma_set_mask_and_coherent(g_dev1, DMA_BIT_MASK(34)))
		LOG_INF("No suitable DMA available\n");

	//power on smi
	/* consumer driver probe*/
	pm_runtime_enable(g_dev1); //Note: It‘s not larb's device.
	LOG_INF("pm_runtime_enable pda1 done\n");
#endif

	if (pda_devm_clk_get(pdev))
		return -1;

	// get PDA address, and PDA quantity
	PDA_devs[0].m_pda_base = of_iomap(node, 0);
	if (!PDA_devs[0].m_pda_base)
		LOG_INF("PDA0 base m_pda_base failed\n");

	// get IRQ ID and request IRQ
	PDA_devs[0].irq = irq_of_parse_and_map(node, 0);
	LOG_INF("PDA_dev[0]->irq: %d", PDA_devs[0].irq);

	if (PDA_devs[0].irq != 0)
		g_PDA_quantity++;
	LOG_INF("PDA quantity: %d\n", g_PDA_quantity);

	CAMSYS_CONFIG_BASE = pda_get_camsys_address();
	if (!CAMSYS_CONFIG_BASE)
		LOG_INF("base CAMSYS_CONFIG_BASE failed\n");

	if (PDA_devs[0].irq > 0) {
		if (PDA_devs[0].irq > 0 && g_PDA_quantity > 0) {
			// Get IRQ Flag from device node
			nRet = of_property_read_u32_array(node,
								"interrupts",
								irq_info,
								ARRAY_SIZE(irq_info));
			if (nRet) {
				LOG_INF("PDA1 get irq flags from DTS fail!!\n");
				return -ENODEV;
			}
			LOG_INF("PDA1 irq_info: %d\n", irq_info[2]);
			nRet = request_irq(PDA_devs[0].irq,
					(irq_handler_t) pda_irqhandle,
					irq_info[2],
					(const char *)node->name,
					NULL);
			if (nRet) {
				LOG_INF("PDA1 request_irq Fail: %d\n", nRet);
				return nRet;
			}
		} else {
			LOG_INF("PDA1 get IRQ ID Fail or No IRQ: %d\n", PDA_devs[0].irq);
		}
	}

	// use iommu
	g_smmu_dev1 = g_dev1;

	// check smmu or iommu
	if (smmu_v3_enabled()) {
		//get smmu shared device
		g_smmu_dev1 = mtk_smmu_get_shared_device(g_dev1);
		if (!g_smmu_dev1) {
			LOG_INF("failed to get pda smmu device\n");
			return -EINVAL;
		}
	}

#ifdef PDA_MMQOS
	pda_mmqos_init(g_dev1);
#endif

	/* create class */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	pda_class = class_create(PDA_DEV_NAME);
#else
	pda_class = class_create(THIS_MODULE, PDA_DEV_NAME);
#endif
	if (IS_ERR(pda_class)) {
		pr_info("Failed to create class (%d)\n",
				(int)PTR_ERR(pda_class));
		goto err_create_pda_class;
	}

	/* create device */
	pda_device =
	    device_create(pda_class, NULL, pda_devno,
				NULL, PDA_DEV_NAME);
	if (!pda_device) {
		pr_info("Failed to create device\n");
		goto err_create_pda_device;
	}

	if (device_create_file(pda_device, &dev_attr_pda_debug)) {
		pr_info("Failed to create device file(pda_debug)\n");
		goto err_create_pda_device_file;
	}

	LOG_INF("Attached!!\n");
	LOG_INF("probe End\n");

	return nRet;

err_create_pda_device_file:
	device_destroy(pda_class, pda_devno);
	class_destroy(pda_class);
	return 0;

err_create_pda_device:
	class_destroy(pda_class);
	return 0;

err_create_pda_class:
	return 0;
}

static int PDA_remove(struct platform_device *pdev)
{
	PDA_UnRegCharDev();
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static int PDA_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int PDA_resume(struct platform_device *pdev)
{
	return 0;
}

static void PDA_shutdown(struct platform_device *pdev)
{
	//Disable clock
	EnableClock(MFALSE);
	spin_lock(&g_PDA_SpinLock);
	LOG_INF("PDA shutdown g_u4EnableClockCount: %d", g_u4EnableClockCount);
	spin_unlock(&g_PDA_SpinLock);
	pm_runtime_disable(&pdev->dev);
}

static int PDA2_probe(struct platform_device *pdev)
{
	int nRet = 0;
	struct device_node *node;
	unsigned int irq_info[3];

	LOG_INF("PDA2 probe Start\n");

	// get PDA node
	node = of_find_compatible_node(NULL, NULL, "mediatek,camera-pda2");
	if (!node) {
		LOG_INF("find camera-pda node failed\n");
		return -1;
	}
	LOG_INF("find camera-pda node done\n");

	// must porting in dts
	pda_init_larb(pdev);

#if IS_ENABLED(CONFIG_OF)
	g_dev2 = &pdev->dev;

	if (dma_set_mask_and_coherent(g_dev2, DMA_BIT_MASK(34)))
		LOG_INF("No suitable DMA available\n");

	//power on smi
	// consumer driver probe
	pm_runtime_enable(g_dev2); //Note: It‘s not larb's device.
	LOG_INF("pm_runtime_enable pda2 done\n");
#endif

	// get PDA address, and PDA quantity
	PDA_devs[1].m_pda_base = of_iomap(node, 0);
	if (!PDA_devs[1].m_pda_base)
		LOG_INF("base m_pda_base failed, index: %d\n", 1);

	// get IRQ ID and request IRQ
	PDA_devs[1].irq = irq_of_parse_and_map(node, 0);
	LOG_INF("PDA_dev[1]->irq: %d", PDA_devs[1].irq);

	if (PDA_devs[1].irq != 0)
		g_PDA_quantity++;

	LOG_INF("PDA quantity: %d\n", g_PDA_quantity);

	if (PDA_devs[1].irq > 0) {
		if (PDA_devs[1].irq > 0 && g_PDA_quantity > 1) {
			// Get IRQ Flag from device node
			nRet = of_property_read_u32_array(node,
								"interrupts",
								irq_info,
								ARRAY_SIZE(irq_info));
			if (nRet) {
				LOG_INF("PDA2 get irq flags from DTS fail!!\n");
				return -ENODEV;
			}
			LOG_INF("PDA2 irq_info: %d\n", irq_info[2]);
			nRet = request_irq(PDA_devs[1].irq,
					(irq_handler_t) pda2_irqhandle,
					irq_info[2],
					(const char *)node->name,
					NULL);
			if (nRet) {
				LOG_INF("PDA2 request_irq Fail: %d\n", nRet);
				return nRet;
			}
		} else {
			LOG_INF("PDA2 get IRQ ID Fail or No IRQ: %d\n", PDA_devs[1].irq);
		}
	}

	LOG_INF("PDA2 probe End\n");

	return nRet;
}

static int PDA2_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	return 0;
}

//////////////////////////////////////// PDA driver //////////////////////////
#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id gpda_of_device_id[] = {
	{.compatible = "mediatek,camera-pda",},
	{}
};
#endif

MODULE_DEVICE_TABLE(of, gpda_of_device_id);

static struct platform_driver PDADriver = {
	.probe = PDA_probe,
	.remove = PDA_remove,
	.suspend = PDA_suspend,
	.resume = PDA_resume,
	.shutdown = PDA_shutdown,
	.driver = {
		   .name = PDA_DEV_NAME,
		   .owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		   .of_match_table = of_match_ptr(gpda_of_device_id),
#endif
		}
};

//////////////////////////////////////// PDA 2 driver ////////////////////////
#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id gpda2_of_device_id[] = {
	{.compatible = "mediatek,camera-pda2",},
	{}
};
#endif

MODULE_DEVICE_TABLE(of, gpda2_of_device_id);

static struct platform_driver PDA2Driver = {
	.probe = PDA2_probe,
	.remove = PDA2_remove,
	.driver = {
		   .name = "camera-pda2",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(gpda2_of_device_id),
	}
};

static int __init camera_pda_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&PDADriver);
	if (ret < 0) {
		LOG_INF("platform_driver_register PDADriver\n");
		return ret;
	}
	ret = platform_driver_register(&PDA2Driver);
	if (ret < 0) {
		LOG_INF("platform_driver_register PDADriver\n");
		return ret;
	}

	return ret;
}

static void __exit camera_pda_exit(void)
{
	platform_driver_unregister(&PDADriver);
	platform_driver_unregister(&PDA2Driver);
}
/****************************************************************************
 *
 ****************************************************************************/
module_init(camera_pda_init);
module_exit(camera_pda_exit);
MODULE_DESCRIPTION("Camera PDA driver");
MODULE_AUTHOR("MM6SW3");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL");
