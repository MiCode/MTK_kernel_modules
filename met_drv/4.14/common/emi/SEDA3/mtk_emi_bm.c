// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 */

#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <asm/io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>
/* #include "mtk_typedefs.h" */
#include "core_plf_init.h"
#include "mtk_emi_bm.h"
#include "met_drv.h"
#include "interface.h"

#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT)
#include "sspm/ondiemet_sspm.h"
#elif defined(TINYSYS_SSPM_SUPPORT)
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#endif
#endif

#undef	DEBUG

#define	emi_readl		readl
#define	emi_reg_sync_writel	mt_reg_sync_writel

#define MASK_MASTER	0xFF
#define MASK_TRANS_TYPE	0xFF

static void __iomem *BaseAddrEMI;
const unsigned int emi_config[] = {
	EMI_BMEN,
	EMI_MSEL,
	EMI_MSEL2,
	EMI_MSEL3,
	EMI_MSEL4,
	EMI_MSEL5,
	EMI_MSEL6,
	EMI_MSEL7,
	EMI_MSEL8,
	EMI_MSEL9,
	EMI_MSEL10,
	EMI_BMID0,
	EMI_BMID1,
	EMI_BMID2,
	EMI_BMID3,
	EMI_BMID4,
	EMI_BMID5,
	EMI_BMID6,
	EMI_BMID7,
	EMI_BMID8,
	EMI_BMID9,
	EMI_BMID10,
	EMI_BMEN1,
	EMI_BMEN2,
	EMI_BMRW0,
	EMI_BMRW1
};
#define EMI_CONFIG_MX_NR (sizeof(emi_config)/sizeof(unsigned int))
static unsigned int emi_config_val[EMI_CONFIG_MX_NR];

int MET_BM_Init(void)
{
	/*emi*/
	if (mt_cen_emi_base_get_symbol) {
		BaseAddrEMI = mt_cen_emi_base_get_symbol();
	} else {
		pr_debug("mt_cen_emi_base_get_symbol = NULL\n");
		PR_BOOTMSG_ONCE("mt_cen_emi_base_get_symbol = NULL\n");
		BaseAddrEMI = 0;
	}

	if (BaseAddrEMI == 0) {
		pr_debug("BaseAddrEMI = 0\n");
		PR_BOOTMSG_ONCE("BaseAddrEMI = 0\n");
		return -1;
	}
	pr_debug("MET EMI: map emi to %p\n", BaseAddrEMI);
	PR_BOOTMSG("MET EMI: map emi to %p\n", BaseAddrEMI);

	return 0;
}

void MET_BM_DeInit(void)
{
}

void MET_BM_SaveCfg(void)
{
	int i;

	for (i = 0; i < EMI_CONFIG_MX_NR; i++)
		emi_config_val[i] = emi_readl(IOMEM(ADDR_EMI + emi_config[i]));
}

void MET_BM_RestoreCfg(void)
{
	int i;

	for (i = 0; i < EMI_CONFIG_MX_NR; i++)
		emi_reg_sync_writel(emi_config_val[i], ADDR_EMI + emi_config[i]);
}

int MET_BM_SetMonitorCounter(const unsigned int counter_num,
			     const unsigned int master, const unsigned int trans_type)
{
	unsigned int value, addr;
	const unsigned int iMask = (MASK_TRANS_TYPE << 8) | MASK_MASTER;

	if (counter_num < 1 || counter_num > BM_COUNTER_MAX)
		return BM_ERR_WRONG_REQ;


	if (counter_num == 1) {
		addr = EMI_BMEN;
		value = (emi_readl(IOMEM(ADDR_EMI + addr)) & ~(iMask << 16)) |
			((trans_type & MASK_TRANS_TYPE) << 24) | ((master & MASK_MASTER) << 16);
	} else {
		addr = (counter_num <= 3) ? EMI_MSEL : (EMI_MSEL2 + (counter_num / 2 - 2) * 8);


		value = emi_readl(IOMEM(ADDR_EMI + addr)) & ~(iMask << ((counter_num % 2) * 16));


		value |= (((trans_type & MASK_TRANS_TYPE) << 8) |
			  (master & MASK_MASTER)) << ((counter_num % 2) * 16);
	}

	emi_reg_sync_writel(value, ADDR_EMI + addr);

	return BM_REQ_OK;
}

int MET_BM_SetTtypeCounterRW(unsigned int bmrw0_val, unsigned int bmrw1_val)
{

	unsigned int value_origin;

	value_origin = emi_readl(IOMEM(ADDR_EMI + EMI_BMRW0));
	MET_TRACE("[MET_EMI_settype1] value_origin: %x\n", value_origin);
	if (value_origin != bmrw0_val) {
		emi_reg_sync_writel(bmrw0_val, ADDR_EMI + EMI_BMRW0);
		MET_TRACE("[MET_EMI_settype1] bmrw0_val: %x, value_origin: %x\n", bmrw0_val,
			   value_origin);
	}

	value_origin = emi_readl(IOMEM(ADDR_EMI + EMI_BMRW1));
	MET_TRACE("[MET_EMI_settype2] value_origin: %x\n", value_origin);
	if (value_origin != bmrw1_val) {
		emi_reg_sync_writel(bmrw1_val, ADDR_EMI + EMI_BMRW1);
		MET_TRACE("[MET_EMI_settype2] bmrw0_val: %x, value_origin: %x\n", bmrw1_val,
			   value_origin);

	}
	return BM_REQ_OK;
}

int MET_BM_Set_WsctTsct_id_sel(unsigned int counter_num, unsigned int enable)
{
	unsigned int value;

	if (counter_num > 3)
		return BM_ERR_WRONG_REQ;

	value =
	    ((emi_readl(IOMEM(ADDR_EMI + EMI_BMEN2)) & (~(1 << (28 + counter_num)))) |
	     (enable << (28 + counter_num)));
	emi_reg_sync_writel(value, ADDR_EMI + EMI_BMEN2);

	return BM_REQ_OK;
}

int MET_BM_SetbusID_En(const unsigned int counter_num,
		       const unsigned int enable)
{
	unsigned int value;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX) || (enable > 1))
		return BM_ERR_WRONG_REQ;

	if (enable == 0) {

		value = (emi_readl(IOMEM(ADDR_EMI + EMI_BMEN2))
			 & ~(1 << (counter_num - 1)));
	} else {

		value = (emi_readl(IOMEM(ADDR_EMI + EMI_BMEN2))
			 | (1 << (counter_num - 1)));
	}
	emi_reg_sync_writel(value, ADDR_EMI + EMI_BMEN2);

	return BM_REQ_OK;
}

int MET_BM_SetbusID(const unsigned int counter_num,
		    const unsigned int id)
{
	unsigned int value, addr, shift_num;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX))
		return BM_ERR_WRONG_REQ;


	addr = EMI_BMID0 + (counter_num - 1) / 2 * 4;
	shift_num = ((counter_num - 1) % 2) * 16;

	value = emi_readl(IOMEM(ADDR_EMI + addr)) & ~(EMI_BMID_MASK << shift_num);


	if (id <= 0xffff)
		value |= id << shift_num;

	emi_reg_sync_writel(value, ADDR_EMI + addr);

	return BM_REQ_OK;
}

int MET_BM_SetUltraHighFilter(const unsigned int counter_num, const unsigned int enable)
{
	unsigned int value;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX) || (enable > 1))
		return BM_ERR_WRONG_REQ;


	value = (emi_readl(IOMEM(ADDR_EMI + EMI_BMEN1))
		 & ~(1 << (counter_num - 1)))
		| (enable << (counter_num - 1));

	emi_reg_sync_writel(value, ADDR_EMI + EMI_BMEN1);

	return BM_REQ_OK;
}

int MET_BM_SetLatencyCounter(unsigned int enable)
{
	unsigned int value;

	value = emi_readl(IOMEM(ADDR_EMI + EMI_BMEN2)) & ~(0x3 << 24);
	if (enable == 1)
		value |= (0x2 << 24);

	emi_reg_sync_writel(value, ADDR_EMI + EMI_BMEN2);

	return BM_REQ_OK;
}

unsigned int MET_EMI_GetDramChannNum(void)
{
	int num = -1;

	if (BaseAddrEMI) {
		num = emi_readl(IOMEM(ADDR_EMI + EMI_CONA));
		num = ((num >> 8) & 0x0000003);
	} else {
		return 1;
	}

	if (num == M0_DOUBLE_HALF_BW_1CH)
		return 1;
	else if (num == M0_DOUBLE_HALF_BW_2CH)
		return 2;
	else if (num == M0_DOUBLE_HALF_BW_4CH)
		return 4;
	else                    /* default return single channel */
		return 1;
}


unsigned int MET_EMI_GetDramRankNum(void)
{
	int dual_rank = 0;

	if (BaseAddrEMI) {
		dual_rank = emi_readl(IOMEM(ADDR_EMI + EMI_CONA));
		dual_rank = ((dual_rank >> 17) & RANK_MASK);
	} else {
		return DUAL_RANK;
	}

	if (dual_rank == DISABLE_DUAL_RANK_MODE)
		return ONE_RANK;
	else			/* default return dual rank */
		return DUAL_RANK;
}


unsigned int MET_EMI_GetDramRankNum_CHN1(void)
{
	int dual_rank = 0;

	if (BaseAddrEMI) {
		dual_rank = emi_readl(IOMEM(ADDR_EMI + EMI_CONA));
		dual_rank = ((dual_rank >> 16) & RANK_MASK);
	} else {
		return DUAL_RANK;
	}

	if (dual_rank == DISABLE_DUAL_RANK_MODE)
		return ONE_RANK;
	else			/* default return dual rank */
		return DUAL_RANK;
}

unsigned int MET_EMI_Get_BaseClock_Rate(void)
{
	unsigned int DRAM_TYPE;

	if (get_cur_ddr_ratio_symbol)
		return get_cur_ddr_ratio_symbol();
	else {
		if (mtk_dramc_get_ddr_type_symbol)
			DRAM_TYPE = mtk_dramc_get_ddr_type_symbol();
		else if (get_ddr_type_symbol)
			DRAM_TYPE = get_ddr_type_symbol();

		if (mtk_dramc_get_ddr_type_symbol || get_ddr_type_symbol) {
			if ((DRAM_TYPE == 2) || (DRAM_TYPE == 3))
				return DRAM_EMI_BASECLOCK_RATE_LP4;
			else
				return DRAM_EMI_BASECLOCK_RATE_LP3;

		} else {
			return DRAM_EMI_BASECLOCK_RATE_LP4;
		}
	}
}
