/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>
#include "mtk_typedefs.h"
#include "core_plf_init.h"
#include "mtk_emi_bm_35.h"

#include "met_drv.h"
#include "interface.h"

#undef	DEBUG
#undef	debug_reg
#ifdef	debug_reg
static inline unsigned int emi_readl(void __iomem *padr)
{
	unsigned int tmp;

	tmp = readl(padr);
	MET_TRACE("[MET_EMI] RD_Reg: %p: %08x\n", padr, tmp);
	return tmp;
}

static inline void __emi_reg_sync_writel(unsigned int data, void __iomem *padr)
{
	unsigned int tmp;

	mt_reg_sync_writel(data, padr);
	tmp = readl(padr);
	MET_TRACE("[MET_EMI] WR_Reg: %p: %08x, %08x\n", padr, data, tmp);
}

#define emi_reg_sync_writel(data, adr)  __emi_reg_sync_writel(data, IOMEM(adr))

#else
#define emi_readl               readl
#define emi_reg_sync_writel     mt_reg_sync_writel
#endif

#define MASK_MASTER     0xFF
#define MASK_TRANS_TYPE 0xFF

/* static int dram_chann_num; */
static void __iomem *BaseAddrEMI;
/* static void __iomem *BaseAddrCHN_EMI[2]; */

/* static int dramc0_dcm_enable;
static int dramc1_dcm_enable; */

#define CH0_MISC_CG_CTRL0 (((unsigned long) BaseAddrDDRPHY_AO[0]) + 0x284)
#define CH1_MISC_CG_CTRL0 (((unsigned long) BaseAddrDDRPHY_AO[1]) + 0x284)
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
	EMI_BMRW1,
	EMI_DBWA,
	EMI_DBWB,
	EMI_DBWC,
	EMI_DBWD,
	EMI_DBWE,
	EMI_DBWF,
	EMI_DBWI,
	EMI_DBWJ,
	EMI_DBWK,
	EMI_DBWA_2ND,
	EMI_DBWB_2ND,
	EMI_DBWC_2ND,
	EMI_DBWD_2ND,
	EMI_DBWE_2ND,
	EMI_DBWF_2ND,
	EMI_TTYPE1_CONA,
	EMI_TTYPE1_CONB,
	EMI_TTYPE2_CONA,
	EMI_TTYPE2_CONB,
	EMI_TTYPE3_CONA,
	EMI_TTYPE3_CONB,
	EMI_TTYPE4_CONA,
	EMI_TTYPE4_CONB,
	EMI_TTYPE5_CONA,
	EMI_TTYPE5_CONB,
	EMI_TTYPE6_CONA,
	EMI_TTYPE6_CONB,
	EMI_TTYPE7_CONA,
	EMI_TTYPE7_CONB,
	EMI_TTYPE8_CONA,
	EMI_TTYPE8_CONB,
	EMI_TTYPE9_CONA,
	EMI_TTYPE9_CONB,
	EMI_TTYPE10_CONA,
	EMI_TTYPE10_CONB,
	EMI_TTYPE11_CONA,
	EMI_TTYPE11_CONB,
	EMI_TTYPE12_CONA,
	EMI_TTYPE12_CONB,
	EMI_TTYPE13_CONA,
	EMI_TTYPE13_CONB,
	EMI_TTYPE14_CONA,
	EMI_TTYPE14_CONB,
	EMI_TTYPE15_CONA,
	EMI_TTYPE15_CONB,
	EMI_TTYPE16_CONA,
	EMI_TTYPE16_CONB,
	EMI_TTYPE17_CONA,
	EMI_TTYPE17_CONB,
	EMI_TTYPE18_CONA,
	EMI_TTYPE18_CONB,
	EMI_TTYPE19_CONA,
	EMI_TTYPE19_CONB,
	EMI_TTYPE20_CONA,
	EMI_TTYPE20_CONB,
	EMI_TTYPE21_CONA,
	EMI_TTYPE21_CONB
};
#define EMI_CONFIG_MX_NR (sizeof(emi_config)/sizeof(unsigned int))
static unsigned int emi_config_val[EMI_CONFIG_MX_NR];



static inline void MET_REG_BCLR(unsigned long reg, u32 shift)
{
	unsigned int read_val = 0;

	read_val = emi_readl(IOMEM(reg));
	emi_reg_sync_writel(read_val & (~((1 << shift) & 0xFFFFFFFF)), reg);
}


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







void MET_BM_SetReadWriteType(const unsigned int ReadWriteType)
{
	const unsigned int value = emi_readl(IOMEM(ADDR_EMI + EMI_BMEN));

	/*
	 * ReadWriteType: 00/11 --> both R/W
	 *                   01 --> only R
	 *                   10 --> only W
	 */
	emi_reg_sync_writel((value & 0xFFFFFFCF) | (ReadWriteType << 4), ADDR_EMI + EMI_BMEN);
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

		/* clear master and transaction type fields */
		value = emi_readl(IOMEM(ADDR_EMI + addr)) & ~(iMask << ((counter_num % 2) * 16));

		/* set master and transaction type fields */
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


int MET_BM_SetMaster(const unsigned int counter_num, const unsigned int master)
{
	unsigned int value, addr;
	const unsigned int iMask = 0x7F;

	if (counter_num < 1 || counter_num > BM_COUNTER_MAX)
		return BM_ERR_WRONG_REQ;


	if (counter_num == 1) {
		addr = EMI_BMEN;
		value =
		    (emi_readl(IOMEM(ADDR_EMI + addr)) & ~(iMask << 16)) | ((master & iMask) << 16);
	} else {
		addr = (counter_num <= 3) ? EMI_MSEL : (EMI_MSEL2 + (counter_num / 2 - 2) * 8);

		/* clear master and transaction type fields */
		value = emi_readl(IOMEM(ADDR_EMI + addr)) & ~(iMask << ((counter_num % 2) * 16));

		/* set master and transaction type fields */
		value |= ((master & iMask) << ((counter_num % 2) * 16));
	}

	emi_reg_sync_writel(value, ADDR_EMI + addr);

	return BM_REQ_OK;
}


int MET_BM_SetbusID_En(const unsigned int counter_num,
		       const unsigned int enable)
{
	unsigned int value;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX) || (enable > 1))
		return BM_ERR_WRONG_REQ;

	if (enable == 0) {
		/* clear  EMI ID selection Enabling   SEL_ID_EN */
		value = (emi_readl(IOMEM(ADDR_EMI + EMI_BMEN2))
			 & ~(1 << (counter_num - 1)));
	} else {
		/* enable  EMI ID selection Enabling   SEL_ID_EN */
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

	/* offset of EMI_BMIDx register */
	addr = EMI_BMID0 + (counter_num - 1) / 2 * 4;
	shift_num = ((counter_num - 1) % 2) * 16;
	/* clear SELx_ID field */
	value = emi_readl(IOMEM(ADDR_EMI + addr)) & ~(EMI_BMID_MASK << shift_num);

	/* set SELx_ID field */
	if (id <= 0xffff)       /*bigger then 0xff_ff : no select busid in master, reset busid as 0*/
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
	/*
	 * emi_ttype1 -- emi_ttype8 change as total latencies
	 * for m0 -- m7,
	 * and emi_ttype9 -- emi_ttype16 change as total transaction counts
	 * for m0 -- m7
	 */
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

		if (get_ddr_type_symbol) {	
			DRAM_TYPE = get_ddr_type_symbol();

			if ((DRAM_TYPE == 2) || (DRAM_TYPE == 3))
				return DRAM_EMI_BASECLOCK_RATE_LP4;
			else
				return DRAM_EMI_BASECLOCK_RATE_LP3;

		} else {
			return DRAM_EMI_BASECLOCK_RATE_LP4;
		}
	}
}

/* For SEDA3.5 wsct setting*/
/* EMI_DBWX[15:8],    X=A~F   (SEL_MASTER) */
/* RW:    EMI_DBWX[1:0],    X=A~F */
int MET_BM_SetWSCT_master_rw(unsigned int *master , unsigned int *rw)
{
	unsigned int value, addr;
	int i;

	const unsigned int Mask_master = 0xFF;
	const unsigned int offset_master = 8;

	const unsigned int Mask_rw = 0x3;
	const unsigned int offset_rw = 0;

	for (i=0; i<WSCT_AMOUNT; i++) {
		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_master << offset_master)) | ((*(master+i) & Mask_master) << offset_master);
		value = (value & ~(Mask_rw << offset_rw)) | ((*(rw+i) & Mask_rw) << offset_rw);


		emi_reg_sync_writel(value, ADDR_EMI + addr);
	}

	return BM_REQ_OK;
}

int MET_BM_SetWSCT_high_priority(unsigned int *disable, unsigned int *select)
{
	unsigned int value, addr;
	int i;

	const unsigned int Mask_disable = 0x1;
	const unsigned int offset_disable  = 2;

	const unsigned int Mask_select = 0xF;
	const unsigned int offset_select  = 28;

	for (i=0;i<WSCT_AMOUNT;i++) {
		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM(ADDR_EMI + addr));
		value = (value & ~(Mask_disable << offset_disable)) | ((*(disable+i) & Mask_disable) << offset_disable);
		emi_reg_sync_writel(value, ADDR_EMI + addr);

		/* ultra level setting */
		addr = EMI_DBWA_2ND + i*4;
		value = emi_readl(IOMEM(ADDR_EMI + addr));
		value = (value & ~(Mask_select << offset_select)) | ((*(select+i) & Mask_select) << offset_select);
		emi_reg_sync_writel(value, ADDR_EMI + addr);
	}
	return BM_REQ_OK;
}

/* busid enbale: EMI_DBWX[3],    X=A~F */
/* busid sel: EMI_DBWX[28:16],    X=A~F  (SEL_ID_TMP) */
/* busid mask : EMI_DBWY[12:0]  或 EMI_DBWY[28:16],  Y=I~K    (SEL_ID_MSK) */
int MET_BM_SetWSCT_busid_idmask(unsigned int *busid, unsigned int *idMask)
{
	unsigned int value, addr;
	unsigned int enable_tmp, busid_tmp, idmask_tmp; 
	int i;

	const unsigned int Mask_busid = 0x1FFF;
	const unsigned int offset_busid  = 16;

	const unsigned int Mask_enable = 0x1;
	const unsigned int offset_enable  = 3;

	const unsigned int Mask_idMask = 0x1FFF;
	const unsigned int offset_idMask_even  = 0;
	const unsigned int offset_idMask_odd  = 16;

	for (i=0;i<WSCT_AMOUNT;i++) {

		/*enable, SEL_ID_TMP*/
		if (*(busid+i)>0xffff) {
			enable_tmp = 0;
			busid_tmp = 0x1FFF;
			idmask_tmp = 0x1FFF;
		}
		else {
			enable_tmp = 1;
			busid_tmp = *(busid+i) & Mask_busid;
			idmask_tmp = *(idMask+i) & Mask_idMask;
		}

		
		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_busid << offset_busid)) | (busid_tmp << offset_busid);
		value = (value & ~(Mask_enable << offset_enable)) | (enable_tmp << offset_enable);

		emi_reg_sync_writel(value, ADDR_EMI + addr);

		/*SEL_ID_MSK*/
		addr = EMI_DBWI + (i/2)*4;

		value = emi_readl(IOMEM(ADDR_EMI + addr));

		if (i%2==0)
			value = (value & ~(Mask_idMask << offset_idMask_even)) | (idmask_tmp << offset_idMask_even);
		else
			value = (value & ~(Mask_idMask << offset_idMask_odd)) | (idmask_tmp << offset_idMask_odd);

		emi_reg_sync_writel(value, ADDR_EMI + addr);
	}


	return BM_REQ_OK;
}


int MET_BM_SetWSCT_chn_rank_sel(unsigned int *chn_rank_sel)
{
	unsigned int value, addr;
	int i;

	const unsigned int Mask = 0xF;
	const unsigned int offset  = 12;


	for (i=0;i<WSCT_AMOUNT;i++) {
		addr = EMI_DBWA_2ND + i*4;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask << offset)) | ((*(chn_rank_sel+i) & Mask) << offset);

		emi_reg_sync_writel(value, ADDR_EMI + addr);
	}
	return BM_REQ_OK;
}

int MET_BM_SetWSCT_burst_range(unsigned int *bnd_dis, unsigned int *low_bnd, unsigned int *up_bnd)
{
	unsigned int value, addr;
	int i;

	const unsigned int Mask_dis = 0x1, Mask_low_bnd = 0x1FF, Mask_up_bnd = 0x1FF;
	const unsigned int offset_dis = 4, offset_low_bnd = 16 , offset_up_bnd = 0 ;


	for (i=0;i<WSCT_AMOUNT;i++) {

		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_dis << offset_dis)) | ((*(bnd_dis+i) & Mask_dis) << offset_dis);

		emi_reg_sync_writel(value, ADDR_EMI + addr);


		addr = EMI_DBWA_2ND + i*4;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_low_bnd << offset_low_bnd)) | ((*(low_bnd+i) & Mask_low_bnd) << offset_low_bnd);
		value = (value & ~(Mask_up_bnd << offset_up_bnd)) | ((*(up_bnd+i) & Mask_up_bnd) << offset_up_bnd);

		emi_reg_sync_writel(value, ADDR_EMI + addr);
	}
	return BM_REQ_OK;

}

int MET_BM_SetTSCT_busid_enable(unsigned int *enable)
{
	//MET_BM_Set_WsctTsct_id_sel

	int i;

	for (i=0;i<TSCT_AMOUNT;i++) {
		MET_BM_Set_WsctTsct_id_sel(i, *(enable+i));
	}

	return BM_REQ_OK;
}

//use the origin together, MET_BM_SetUltraHighFilter()
/* EMI_TTYPEN_CONA [23:20],  N=1~21   (HPRI_SEL) */
int MET_BM_SetTtype_high_priority_sel(unsigned int _high_priority_filter, unsigned int *select)
{
	int i;
	unsigned int enable;
	unsigned int value, addr;

	const unsigned int Mask_sel = 0xF;
	const unsigned int offset_sel = 20;

	for (i = 0; i < BM_COUNTER_MAX; i++) {
		if ((_high_priority_filter & (1 << i)) == 0){
			enable = 0;
		}
		else {
			enable = 1;
		}

		MET_BM_SetUltraHighFilter(i + 1, enable);

		/* ultra level select */
		addr = EMI_TTYPE1_CONA + i*8;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_sel << offset_sel)) | ((*(select+i) & Mask_sel) << offset_sel);

		emi_reg_sync_writel(value, ADDR_EMI + addr);

	}

	return BM_REQ_OK;
}


//always call this API to init the reg
//related API, MET_BM_SetbusID, MET_BM_SetbusID_En
int MET_BM_SetTtype_busid_idmask(unsigned int *busid, unsigned int *idMask, int _ttype1_16_en, int _ttype17_21_en)
{
	int i;
	unsigned int value, addr;

	const unsigned int Mask_idMask = 0x1FFF;
	const unsigned int offset_idMask = 0;

	if (_ttype1_16_en != BM_TTYPE1_16_ENABLE) {
		/* mask set 0x1FFF , busid set disable*/
		for (i = 1; i <= 16; i++) {
			*(busid + i - 1) = 0xfffff;
			*(idMask + i - 1) = 0x1FFF;
		}
	}

	if (_ttype17_21_en != BM_TTYPE17_21_ENABLE) {
		for (i = 17; i <= 21; i++) {
			*(busid + i - 1) = 0xfffff;
			*(idMask + i - 1) = 0x1FFF;
		}
	}

	for (i = 1; i <= BM_COUNTER_MAX; i++) {
		MET_BM_SetbusID(i, *(busid + i - 1));
		MET_BM_SetbusID_En(i, ( *(busid + i - 1) > 0xffff) ? 0 : 1);

		/* set idMask */
		addr = EMI_TTYPE1_CONA + (i-1)*8;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_idMask << offset_idMask)) | ((*(idMask+i-1) & Mask_idMask) << offset_idMask);

		emi_reg_sync_writel(value, ADDR_EMI + addr);

	}

	return BM_REQ_OK;
}


int MET_BM_SetTtype_chn_rank_sel(unsigned int *chn_rank_sel)
{
	unsigned int value, addr;
	int i;

	const unsigned int Mask = 0xF;
	const unsigned int offset  = 16;


	for (i=0;i<BM_COUNTER_MAX;i++) {
		addr = EMI_TTYPE1_CONA + i*8;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask << offset)) | ((*(chn_rank_sel+i) & Mask) << offset);

		emi_reg_sync_writel(value, ADDR_EMI + addr);
	}
	return BM_REQ_OK;
}

int MET_BM_SetTtype_burst_range(unsigned int *bnd_dis, unsigned int *low_bnd, unsigned int *up_bnd)
{
	unsigned int value, addr;
	int i;

	const unsigned int Mask_dis = 0x1, Mask_low_bnd = 0x1FF, Mask_up_bnd = 0x1FF;
	const unsigned int offset_dis = 24, offset_low_bnd = 16 , offset_up_bnd = 0 ;


	for (i=0;i<BM_COUNTER_MAX;i++) {

		/* set dis bit */
		addr = EMI_TTYPE1_CONA + i*8;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_dis << offset_dis)) | ((*(bnd_dis+i) & Mask_dis) << offset_dis);

		emi_reg_sync_writel(value, ADDR_EMI + addr);


		addr = EMI_TTYPE1_CONB + i*8;
		value = emi_readl(IOMEM(ADDR_EMI + addr));

		value = (value & ~(Mask_low_bnd << offset_low_bnd)) | ((*(low_bnd+i) & Mask_low_bnd) << offset_low_bnd);
		value = (value & ~(Mask_up_bnd << offset_up_bnd)) | ((*(up_bnd+i) & Mask_up_bnd) << offset_up_bnd);

		emi_reg_sync_writel(value, ADDR_EMI + addr);
	}
	return BM_REQ_OK;
}