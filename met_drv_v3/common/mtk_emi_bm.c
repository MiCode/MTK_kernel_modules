// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 */
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>

#ifdef USE_KERNEL_SYNC_WRITE_H
#include <mt-plat/sync_write.h>
#else
#include "sync_write.h"
#endif

#ifdef USE_KERNEL_MTK_IO_H
#include <mt-plat/mtk_io.h>
#else
#include "mtk_io.h"
#endif

#include "mtk_typedefs.h"
#include "core_plf_init.h"
#include "mtk_emi_bm.h"

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


#define DECLARE_MULTI_EMI_VAR_UINT_ARRAY(name, amount) \
	unsigned int name[amount]; \
	unsigned int name##_[MET_MAX_EMI_NUM][amount];

#define DECLARE_MULTI_EMI_VAR_INT_ARRAY(name, amount) \
	int name[amount]; \
	int name##_[MET_MAX_EMI_NUM][amount];

#define DECLARE_MULTI_EMI_VAR_UINT(name) \
	unsigned int name; \
	unsigned int name##_[MET_MAX_EMI_NUM];

#define DECLARE_MULTI_EMI_VAR_INT(name) \
	int name; \
	int name##_[MET_MAX_EMI_NUM];

#define STORE_EMI_PARA_ARRAY(name,emi_no,type,size) \
	memcpy(name##_[emi_no],name,sizeof(type)*size)

#define STORE_EMI_PARA(name,emi_no) \
	name##_[emi_no] = name;

void __iomem *BaseAddrEMI[MET_MAX_EMI_NUM];
void __iomem *BaseAddrCHN_EMI[MET_MAX_EMI_NUM][MET_MAX_DRAM_CH_NUM];

void __iomem *BaseAddrDRAMC[MET_MAX_EMI_NUM][MET_MAX_DRAM_CH_NUM];
void __iomem *BaseAddrDDRPHY_AO[MET_MAX_EMI_NUM][MET_MAX_DRAM_CH_NUM];
// void __iomem *BaseAddrDRAMC0_AO[EMI_NUM][MET_MAX_DRAM_CH_NUM]; //phase out,not use anymore
void __iomem *BaseAddrAPMIXEDSYS;
void __iomem *BaseAddrSLC_PMU[MET_MAX_EMI_NUM];

/*read from dts*/
int EMI_NUM;
int DRAM_CH_NUM_PER_EMI;
int DRAM_FREQ_DEFAULT;
int DDR_RATIO_DEFAULT;
int DRAM_TYPE_DEFAULT;
int MET_EMI_support_list = 0x0; /*read from dts*/
int ddrphy_ao_misc_cg_ctrl0;
int ddrphy_ao_misc_cg_ctrl2;



// #define CH0_MISC_CG_CTRL0 (((unsigned long) BaseAddrDDRPHY_AO[0]) + 0x284)
// #define CH1_MISC_CG_CTRL0 (((unsigned long) BaseAddrDDRPHY_AO[1]) + 0x284)
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
static unsigned int emi_config_val[MET_MAX_EMI_NUM][EMI_CONFIG_MX_NR];

const unsigned int emi_chn_config[] = {
	CHN_EMI_LOWEFF_CTL0
};
#define EMI_CHN_CONFIG_MX_NR (sizeof(emi_chn_config)/sizeof(unsigned int))
static unsigned int emi_chn_config_val[MET_MAX_EMI_NUM][MET_MAX_DRAM_CH_NUM][EMI_CHN_CONFIG_MX_NR];

const unsigned int slc_config[] = {
	SLC_PMU_CNT0_FILTER0,
	SLC_PMU_CNT0_FILTER1,
	SLC_PMU_CNT0_BW_LAT_SEL,
	SLC_PMU_CNT1_FILTER0,
	SLC_PMU_CNT1_FILTER1,
	SLC_PMU_CNT1_BW_LAT_SEL,
	SLC_PMU_CNT2_FILTER0,
	SLC_PMU_CNT2_FILTER1,
	SLC_PMU_CNT2_BW_LAT_SEL,
	SLC_PMU_CNT3_FILTER0,
	SLC_PMU_CNT3_FILTER1,
	SLC_PMU_CNT3_BW_LAT_SEL,
	SLC_PMU_CNT4_FILTER0,
	SLC_PMU_CNT4_FILTER1,
	SLC_PMU_CNT4_BW_LAT_SEL,
	SLC_PMU_CNT5_FILTER0,
	SLC_PMU_CNT5_FILTER1,
	SLC_PMU_CNT5_BW_LAT_SEL,
	SLC_PMU_CNT6_FILTER0,
	SLC_PMU_CNT6_FILTER1,
	SLC_PMU_CNT6_BW_LAT_SEL,
	SLC_PMU_CNT7_FILTER0,
	SLC_PMU_CNT7_FILTER1,
	SLC_PMU_CNT7_BW_LAT_SEL,
	SLC_PMU_CNT8_FILTER0,
	SLC_PMU_CNT8_FILTER1,
	SLC_PMU_CNT8_BW_LAT_SEL,
	SLC_PMU_CNT9_FILTER0,
	SLC_PMU_CNT9_FILTER1,
	SLC_PMU_CNT9_BW_LAT_SEL,
	SLC_PMU_CNT10_FILTER0,
	SLC_PMU_CNT10_FILTER1,
	SLC_PMU_CNT10_BW_LAT_SEL,
	SLC_PMU_CNT11_FILTER0,
	SLC_PMU_CNT11_FILTER1,
	SLC_PMU_CNT11_BW_LAT_SEL,
	SLC_PMU_CNT12_FILTER0,
	SLC_PMU_CNT12_FILTER1,
	SLC_PMU_CNT12_BW_LAT_SEL,
	SLC_PMU_CNT13_FILTER0,
	SLC_PMU_CNT13_FILTER1,
	SLC_PMU_CNT13_BW_LAT_SEL,
	SLC_PMU_CNT14_FILTER0,
	SLC_PMU_CNT14_FILTER1,
	SLC_PMU_CNT14_BW_LAT_SEL,
	SLC_PMU_CNT15_FILTER0,
	SLC_PMU_CNT15_FILTER1,
	SLC_PMU_CNT15_BW_LAT_SEL,
	SLC_PMU_CNT16_FILTER0,
	SLC_PMU_CNT16_FILTER1,
	SLC_PMU_CNT16_BW_LAT_SEL,
	SLC_PMU_CNT17_FILTER0,
	SLC_PMU_CNT17_FILTER1,
	SLC_PMU_CNT17_BW_LAT_SEL,
	SLC_PMU_CNT18_FILTER0,
	SLC_PMU_CNT18_FILTER1,
	SLC_PMU_CNT18_BW_LAT_SEL,
	SLC_PMU_CNT19_FILTER0,
	SLC_PMU_CNT19_FILTER1,
	SLC_PMU_CNT19_BW_LAT_SEL,
	SLC_PMU_CNT20_FILTER0,
	SLC_PMU_CNT20_FILTER1,
	SLC_PMU_CNT20_BW_LAT_SEL,
	SLC_PMU_CNT21_FILTER0,
	SLC_PMU_CNT21_FILTER1,
	SLC_PMU_CNT21_BW_LAT_SEL,
	SLC_PMU_CNT22_FILTER0,
	SLC_PMU_CNT22_FILTER1,
	SLC_PMU_CNT22_BW_LAT_SEL,
	SLC_PMU_CNT23_FILTER0,
	SLC_PMU_CNT23_FILTER1,
	SLC_PMU_CNT23_BW_LAT_SEL,
	SLC_PMU_CNT24_FILTER0,
	SLC_PMU_CNT24_FILTER1,
	SLC_PMU_CNT24_BW_LAT_SEL,
	SLC_PMU_CNT25_FILTER0,
	SLC_PMU_CNT25_FILTER1,
	SLC_PMU_CNT25_BW_LAT_SEL,
	SLC_PMU_CNT26_FILTER0,
	SLC_PMU_CNT26_FILTER1,
	SLC_PMU_CNT26_BW_LAT_SEL,
	SLC_PMU_CNT27_FILTER0,
	SLC_PMU_CNT27_FILTER1,
	SLC_PMU_CNT27_BW_LAT_SEL,
	SLC_PMU_CNT28_FILTER0,
	SLC_PMU_CNT28_FILTER1,
	SLC_PMU_CNT28_BW_LAT_SEL,
	SLC_PMU_CNT29_FILTER0,
	SLC_PMU_CNT29_FILTER1,
	SLC_PMU_CNT29_BW_LAT_SEL,
	SLC_PMU_CNT30_FILTER0,
	SLC_PMU_CNT30_FILTER1,
	SLC_PMU_CNT30_BW_LAT_SEL,
	SLC_PMU_CNT31_FILTER0,
	SLC_PMU_CNT31_FILTER1,
	SLC_PMU_CNT31_BW_LAT_SEL
};
#define SLC_CONFIG_MX_NR (sizeof(slc_config)/sizeof(unsigned int))
static unsigned int slc_config_val[MET_MAX_EMI_NUM][SLC_CONFIG_MX_NR];


enum MET_EMI_DEFAULT_VAL_LIST{
	e_MET_DRAM_FREQ = 0,
	e_MET_DRAM_TYPE,
	e_MET_DDR_RATIO,

	NR_MET_DEFAULT_VAL,
};

unsigned int met_emi_default_val[NR_MET_DEFAULT_VAL];


/* get the emi base addr */
int MET_BM_Init(void)
{
	int i,emi_no;

	/*read emi/dramc attribute from dts*/
	struct device_node *node = NULL;
	char metemi_desc[] = "mediatek,met_emi";
	int ret = 0;

	u32 DRAMC_NAO_PHY_ADDR[MET_MAX_EMI_NUM][MET_MAX_DRAM_CH_NUM];
	u32 DDRPHY_AO_PHY_ADDR[MET_MAX_EMI_NUM][MET_MAX_DRAM_CH_NUM];
	u32 CHN_EMI_PHY_ADDR[MET_MAX_EMI_NUM][MET_MAX_DRAM_CH_NUM];
	u32 CEN_EMI_PHY_ADDR[MET_MAX_EMI_NUM];
	u32 SLC_PMU_PHY_ADDR[MET_MAX_EMI_NUM];
	u32 APMIXEDSYS_ADDR = 0;

	unsigned int cen_emi_reg_size = 0x1000;
	unsigned int chn_emi_reg_size = 0xA90;
	unsigned int dramc_nao_reg_size = 0x76C;
	unsigned int ddrphy_ao_reg_size = 0x1650;
	unsigned int slc_pmu_reg_size = 0x1000;
	unsigned int apmixedsys_reg_size = 0x410;

	// node = of_find_node_by_name(NULL, "met");
	// if (!node) {
	// 	PR_BOOTMSG("of_find_node_by_name unable to find met device node\n");
	// 	return 0;
	// }
	node = of_find_compatible_node(NULL, NULL, metemi_desc);
	if (!node) {
		PR_BOOTMSG("of_find_compatible_node unable to find met device node\n");
		return -1;
	}
	ret = of_property_read_u32_index(node, // device node
									"emi_num",  //device name
									0, //offset
									&EMI_NUM);
	if (ret) {
		PR_BOOTMSG("Cannot get emi_num index from dts\n");
		return -1;
	}
	ret = of_property_read_u32_index(node, // device node
									"dram_num",  //device name
									0, //offset
									&dram_chann_num);
	if (ret) {
		PR_BOOTMSG("Cannot get emi_num index from dts\n");
		return -1;
	}
	DRAM_CH_NUM_PER_EMI = dram_chann_num;

	ret = of_property_read_u32_index(node, // device node
									"dram_freq_default",  //device name
									0, //offset
									&DRAM_FREQ_DEFAULT);
	if (ret) {
		PR_BOOTMSG("Cannot get dram_freq_default index from dts\n");
		return -1;
	}

	ret = of_property_read_u32_index(node, // device node
									"ddr_ratio_default",  //device name
									0, //offset
									&DDR_RATIO_DEFAULT);
	if (ret) {
		PR_BOOTMSG("Cannot get ddr_ratio_default index from dts\n");
		return -1;
	}

	ret = of_property_read_u32_index(node, // device node
									"dram_type_default",  //device name
									0, //offset
									&DRAM_TYPE_DEFAULT);
	if (ret) {
		PR_BOOTMSG("Cannot get dram_type_default index from dts\n");
		return -1;
	}

	ret = of_property_read_u32_index(node, // device node
									"met_emi_support_list",  //device name
									0, //offset
									&MET_EMI_support_list);
	if (ret) {
		PR_BOOTMSG("Cannot get MET_EMI_support_list index from dts\n");
		return -1;
	}

	ret = of_property_read_u32_index(node, // device node
									"cen_emi_reg_size",  //device name
									0, //offset
									&cen_emi_reg_size);
	if (ret) {
		PR_BOOTMSG("Cannot get cen_emi_reg_size index from dts\n");
		return -1;
	}
	ret = of_property_read_u32_index(node, // device node
									"chn_emi_reg_size",  //device name
									0, //offset
									&chn_emi_reg_size);
	if (ret) {
		PR_BOOTMSG("Cannot get chn_emi_reg_size index from dts\n");
		return -1;
	}
	ret = of_property_read_u32_index(node, // device node
									"dramc_nao_reg_size",  //device name
									0, //offset
									&dramc_nao_reg_size);
	if (ret) {
		PR_BOOTMSG("Cannot get dramc_nao_reg_size index from dts\n");
		return -1;
	}
	ret = of_property_read_u32_index(node, // device node
									"ddrphy_ao_reg_size",  //device name
									0, //offset
									&ddrphy_ao_reg_size);
	if (ret) {
		PR_BOOTMSG("Cannot get ddrphy_ao_reg_size index from dts\n");
		return -1;
	}

	ret = of_property_read_u32_index(node, // device node
									"ddrphy_ao_misc_cg_ctrl0",  //device name
									0, //offset
									&ddrphy_ao_misc_cg_ctrl0);
	if (ret) {
		PR_BOOTMSG("Cannot get ddrphy_ao_misc_cg_ctrl0 index from dts\n");
		return -1;
	}

	ret = of_property_read_u32_index(node, // device node
									"ddrphy_ao_misc_cg_ctrl2",  //device name
									0, //offset
									&ddrphy_ao_misc_cg_ctrl2);
	if (ret) {
		PR_BOOTMSG("Cannot get ddrphy_ao_misc_cg_ctrl2 index from dts\n");
		return -1;
	}

	if (MET_EMI_support_list & (1<<EMI_FREQ_SUPPORT))
	{
		ret = of_property_read_u32_index(node, // device node
										"apmixedsys_reg_size",  //device name
										0, //offset
										&apmixedsys_reg_size);
		if (ret) {
			PR_BOOTMSG("Cannot get apmixedsts_reg_size index from dts\n");
			return -1;
		}

		/*get the emi freq*/
		ret = of_property_read_u32_index(node, // device node
										"apmixedsys_reg_base",  //device name
										0, //offset
										&APMIXEDSYS_ADDR);
		if (ret) {
			PR_BOOTMSG("Cannot get apmixedsts_reg_base index from dts\n");
			return -1;
		}
	}

	if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
		ret = of_property_read_u32_index(node, // device node
										"slc_pmu_reg_size",  //device name
										0, //offset
										&slc_pmu_reg_size);
		if (ret) {
			PR_BOOTMSG("Cannot get slc_pmu_reg_size index from dts\n");
			return -1;
		}
	}

	/* emi channel number*/
	// DRAM_CH_NUM_PER_EMI = dram_chann_num = MET_EMI_GetDramChannNum(0);

	for (emi_no = 0; emi_no < EMI_NUM; emi_no++)
	{
		ret = of_property_read_u32_index(node, // device node
										"cen_emi_reg_base",  //device name
										emi_no,
										CEN_EMI_PHY_ADDR + emi_no);
		if (ret)
			PR_BOOTMSG("Cannot get cen_emi_reg_base index from dts\n");

		ret = of_property_read_u32_index(node, // device node
										"slc_pmu_reg_base",  //device name
										emi_no,
										SLC_PMU_PHY_ADDR + emi_no);
		if (ret)
			PR_BOOTMSG("Cannot get slc_pmu_reg_base index from dts\n");


		for (i=0; i<dram_chann_num; i++)
		{
			ret = of_property_read_u32_index(node, // device node
										"chn_emi_reg_base",  //device name
										emi_no,
										CHN_EMI_PHY_ADDR[emi_no] + i);
			if (ret) {
				PR_BOOTMSG("Cannot get chn_emi_reg_base index from dts\n");
				return -1;
			}

			ret = of_property_read_u32_index(node, // device node
										"dramc_nao_reg_base",  //device name
										emi_no,
										DRAMC_NAO_PHY_ADDR[emi_no] + i);
			if (ret) {
				PR_BOOTMSG("Cannot get dramc_nao_reg_base index from dts\n");
				return -1;
			}

			ret = of_property_read_u32_index(node, // device node
										"ddrphy_ao_reg_base",  //device name
										emi_no,
										DDRPHY_AO_PHY_ADDR[emi_no] + i);
			if (ret) {
				PR_BOOTMSG("Cannot get ddrphy_ao_reg_base index from dts\n");
				return -1;
			}
		}
	}

	for (emi_no = 0; emi_no < EMI_NUM; emi_no++)
	{
		/*central EMI*/
		BaseAddrEMI[emi_no] = ioremap(CEN_EMI_PHY_ADDR[emi_no], cen_emi_reg_size);
		if (BaseAddrEMI[emi_no]==NULL)
		{
			PR_BOOTMSG("cen_emi_%d ioremap fail\n",emi_no);
			return -1;
		}
		PR_BOOTMSG("MET EMI: map emi to %p\n", BaseAddrEMI[emi_no]);

		for(i=0; i<dram_chann_num; i++) {
			/* channel emi, in fact, the info in chn_emi is very less */
			BaseAddrCHN_EMI[emi_no][i] = ioremap(CHN_EMI_PHY_ADDR[emi_no][i], chn_emi_reg_size);
			if (BaseAddrCHN_EMI[emi_no][i]==NULL)
			{
				PR_BOOTMSG("chn_emi_%d at cen_emi_%d ioremap fail\n",i,emi_no);
				return -1;
			}
			/*  */
			BaseAddrDRAMC[emi_no][i] = ioremap(DRAMC_NAO_PHY_ADDR[emi_no][i], dramc_nao_reg_size);
			if (BaseAddrDRAMC[emi_no][i]==NULL)
			{
				PR_BOOTMSG("DRAMC_NAO_%d at cen_emi_%d ioremap fail\n",i,emi_no);
				return -1;
			}
			BaseAddrDDRPHY_AO[emi_no][i] = ioremap(DDRPHY_AO_PHY_ADDR[emi_no][i], ddrphy_ao_reg_size);
			if (BaseAddrDDRPHY_AO[emi_no][i]==NULL)
			{
				PR_BOOTMSG("DDRPHY_AO_%d at cen_emi_%d ioremap fail\n",i,emi_no);
				return -1;
			}
		}

		/*SLC PMU*/
		if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
			BaseAddrSLC_PMU[emi_no] = ioremap(SLC_PMU_PHY_ADDR[emi_no], slc_pmu_reg_size);
			if (BaseAddrSLC_PMU[emi_no]==NULL)
			{
				PR_BOOTMSG("slc_pmu_%d ioremap fail\n",emi_no);
				return -1;
			}
			PR_BOOTMSG("MET EMI: map slc pmu to %p\n", BaseAddrSLC_PMU[emi_no]);
		}
	}

	if (MET_EMI_support_list & (1<<EMI_FREQ_SUPPORT))
	{
		BaseAddrAPMIXEDSYS = ioremap(APMIXEDSYS_ADDR, apmixedsys_reg_size);
		if (BaseAddrAPMIXEDSYS == NULL)
		{
			PR_BOOTMSG("APMIXEDSYS_ADDR[%X] ioremap size[%X] fail\n",APMIXEDSYS_ADDR, apmixedsys_reg_size);
			return -1;
		}
	}

	/* set the init value */
	met_emi_default_val[e_MET_DRAM_FREQ] = DRAM_FREQ_DEFAULT;
	met_emi_default_val[e_MET_DRAM_TYPE] = DRAM_TYPE_DEFAULT;
	met_emi_default_val[e_MET_DDR_RATIO] = DDR_RATIO_DEFAULT;


	return 0;
}


void MET_BM_DeInit(void)
{
	/* TBC */
	/* do iounmap */
	PR_BOOTMSG("met_drv do MET_BM_DeInit\n");
	if (BaseAddrAPMIXEDSYS)
		iounmap(BaseAddrAPMIXEDSYS);

	BaseAddrAPMIXEDSYS = NULL;
}

/* support multi-emi */
void MET_BM_SaveCfg(void)
{
	int i,emi_no;

	/* emi central */
	for(emi_no=0; emi_no<EMI_NUM; emi_no++) {
		for (i = 0; i < EMI_CONFIG_MX_NR; i++)
			emi_config_val[emi_no][i] = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + emi_config[i]));

		/*only ch0 have CHN_EMI_LOWEFF_CTL0 now*/
		for (i = 0; i < EMI_CHN_CONFIG_MX_NR; i++) {
			emi_chn_config_val[emi_no][0][i] = emi_readl(IOMEM(BaseAddrCHN_EMI[emi_no][0] + emi_chn_config[i]));
		}

		if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
			for (i = 0; i < SLC_CONFIG_MX_NR; i++)
				slc_config_val[emi_no][i] = emi_readl(IOMEM((unsigned long)BaseAddrSLC_PMU[emi_no] + slc_config[i]));
		}
	}
}

/*support multi-emi*/
void MET_BM_RestoreCfg(void)
{
	int i,emi_no;

	/* emi central */
	for(emi_no=0; emi_no<EMI_NUM ;emi_no++){
		for (i = 0; i < EMI_CONFIG_MX_NR; i++)
			emi_reg_sync_writel(emi_config_val[emi_no][i], (unsigned long)BaseAddrEMI[emi_no] + emi_config[i]);
		for (i = 0; i < EMI_CHN_CONFIG_MX_NR; i++)
			emi_reg_sync_writel(emi_chn_config_val[emi_no][0][i], BaseAddrCHN_EMI[emi_no][0] + emi_chn_config[i]);

		if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
			for (i = 0; i < SLC_CONFIG_MX_NR; i++)
				emi_reg_sync_writel(slc_config_val[emi_no][i], (unsigned long)BaseAddrSLC_PMU[emi_no] + slc_config[i]);
		}
	}
}

/*support assign emi_no*/
void MET_BM_SetReadWriteType(const unsigned int ReadWriteType, unsigned int emi_no)
{
	volatile unsigned int value;

	for(emi_no=0; emi_no<EMI_NUM ;emi_no++)
	{
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN));
		emi_reg_sync_writel((value & 0xFFFFFFCF) | (rwtype << 4), (unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN);
	}
}

/*support assign emi_no*/
int MET_BM_SetMonitorCounter(const unsigned int counter_num,
			     const unsigned int master, const unsigned int trans_type, unsigned int emi_no)
{
	unsigned int value, addr;
	const unsigned int iMask = (MASK_TRANS_TYPE << 8) | MASK_MASTER;

	if (counter_num < 1 || counter_num > BM_COUNTER_MAX)
		return BM_ERR_WRONG_REQ;

	if (counter_num == 1) {
		addr = EMI_BMEN;
		value = (emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr)) & ~(iMask << 16)) |
		    ((trans_type & MASK_TRANS_TYPE) << 24) | ((master & MASK_MASTER) << 16);
	} else {
		addr = (counter_num <= 3) ? EMI_MSEL : (EMI_MSEL2 + (counter_num / 2 - 2) * 8);

		/* clear master and transaction type fields */
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr)) & ~(iMask << ((counter_num % 2) * 16));

		/* set master and transaction type fields */
		value |= (((trans_type & MASK_TRANS_TYPE) << 8) |
			  (master & MASK_MASTER)) << ((counter_num % 2) * 16);
	}

	emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);

	return BM_REQ_OK;
}

int MET_BM_SetTtypeCounterRW(unsigned int bmrw0_val, unsigned int bmrw1_val, unsigned int emi_no)
{
	volatile unsigned int value_origin;


	value_origin = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMRW0));
	MET_TRACE("[MET_EMI_settype1] value_origin: %x\n", value_origin);
	if (value_origin != bmrw0_val) {
		emi_reg_sync_writel(bmrw0_val, (unsigned long)BaseAddrEMI[emi_no] + EMI_BMRW0);
		MET_TRACE("[MET_EMI_settype1] bmrw0_val: %x, value_origin: %x\n", bmrw0_val,
			   value_origin);
	}

	value_origin = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMRW1));
	MET_TRACE("[MET_EMI_settype2] value_origin: %x\n", value_origin);
	if (value_origin != bmrw1_val) {
		emi_reg_sync_writel(bmrw1_val, (unsigned long)BaseAddrEMI[emi_no] + EMI_BMRW1);
		MET_TRACE("[MET_EMI_settype2] bmrw0_val: %x, value_origin: %x\n", bmrw1_val,
			   value_origin);
	}

	return BM_REQ_OK;
}

/*after SEDA 3.5, this counter will only control TSCT*/
int MET_BM_Set_WsctTsct_id_sel(unsigned int counter_num, unsigned int enable, unsigned int emi_no)
{
	volatile unsigned int value;

	if (counter_num > 3)
		return BM_ERR_WRONG_REQ;


	value =
	    ((emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2)) & (~(1 << (28 + counter_num)))) |
	     (enable << (28 + counter_num)));
	emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2);

	return BM_REQ_OK;
}

#if 0 /* not use anymore */
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
#endif

int MET_BM_SetbusID_En(const unsigned int counter_num,
		       const unsigned int enable, unsigned int emi_no)
{
	unsigned int value;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX) || (enable > 1))
		return BM_ERR_WRONG_REQ;

	if (enable == 0) {
		/* clear  EMI ID selection Enabling   SEL_ID_EN */
		value = (emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2))
			 & ~(1 << (counter_num - 1)));
	} else {
		/* enable  EMI ID selection Enabling   SEL_ID_EN */
		value = (emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2))
			 | (1 << (counter_num - 1)));
	}
	emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2);

	return BM_REQ_OK;
}


int MET_BM_SetbusID(const unsigned int counter_num,
		    const unsigned int id, unsigned int emi_no)
{
	unsigned int value, addr, shift_num;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX))
		return BM_ERR_WRONG_REQ;

	/* offset of EMI_BMIDx register */
	addr = EMI_BMID0 + (counter_num - 1) / 2 * 4;
	shift_num = ((counter_num - 1) % 2) * 16;
	/* clear SELx_ID field */
	value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr)) & ~(EMI_BMID_MASK << shift_num);

	/* set SELx_ID field */
	if (id <= 0xffff)       /*bigger then 0xff_ff : no select busid in master, reset busid as 0*/
		value |= id << shift_num;

	emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);

	return BM_REQ_OK;
}


int MET_BM_SetUltraHighFilter(const unsigned int counter_num, const unsigned int enable, unsigned int emi_no)
{
	unsigned int value;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX) || (enable > 1))
		return BM_ERR_WRONG_REQ;


	value = (emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN1))
		 & ~(1 << (counter_num - 1)))
		| (enable << (counter_num - 1));

	emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN1);

	return BM_REQ_OK;
}

/*need to asign emi_no*/
int MET_BM_SetLatencyCounter(unsigned int enable, unsigned int emi_no)
{
	unsigned int value;

	value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2)) & ~(0x3 << 24);
	/*
	 * emi_ttype1 -- emi_ttype8 change as total latencies
	 * for m0 -- m7,
	 * and emi_ttype9 -- emi_ttype16 change as total transaction counts
	 * for m0 -- m7
	 */
	if (enable == 1)
		value |= (0x2 << 24);

	emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2);

	return BM_REQ_OK;
}



unsigned int MET_EMI_GetDramChannNum(unsigned int emi_no)
{
	int num = -1;

	if (BaseAddrEMI[emi_no]) {
		num = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_CONA));
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


unsigned int MET_EMI_GetDramRankNum(unsigned int emi_no)
{
	int dual_rank = 0;

	if (BaseAddrEMI[emi_no]) {
		dual_rank = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_CONA));
		dual_rank = ((dual_rank >> 17) & RANK_MASK);
	} else {
		return DUAL_RANK;
	}

	if (dual_rank == DISABLE_DUAL_RANK_MODE)
		return ONE_RANK;
	else			/* default return dual rank */
		return DUAL_RANK;
}


// unsigned int MET_EMI_GetDramRankNum_CHN1(void)
// {
// 	int dual_rank = 0;

// 	if (BaseAddrEMI) {
// 		dual_rank = emi_readl(IOMEM(ADDR_EMI + EMI_CONA));
// 		dual_rank = ((dual_rank >> 16) & RANK_MASK);
// 	} else {
// 		return DUAL_RANK;
// 	}

// 	if (dual_rank == DISABLE_DUAL_RANK_MODE)
// 		return ONE_RANK;
// 	else			/* default return dual rank */
// 		return DUAL_RANK;
// }


#ifdef EMI_LOWEFF_SUPPORT
int MET_EMI_Get_LOWEFF_CTL0(unsigned int channel, unsigned int emi_no){

	if ( channel >= dram_chann_num )
		return 0;

	if (BaseAddrCHN_EMI[channel]) {
		return emi_readl(IOMEM(BaseAddrCHN_EMI[emi_no][channel] + CHN_EMI_LOWEFF_CTL0));
	} else {
		return 0;
	}
}

int MET_BM_SetLOWEFF_master_rw(unsigned int chan, unsigned int *wmask_msel , unsigned int *ageexp_msel, unsigned int *ageexp_rw, unsigned int emi_no)
{
	unsigned int value;

	const unsigned int Mask_master = 0xFF;
	const unsigned int offset_wmask_master= 16;
	const unsigned int offset_ageexp_master= 24;

	const unsigned int Mask_rw = 0x3;
	const unsigned int offset_rw = 3;

	if (chan >= dram_chann_num)
		return -1;

	if (BaseAddrCHN_EMI[emi_no][chan]) {
		value = emi_readl(IOMEM(BaseAddrCHN_EMI[emi_no][chan] + CHN_EMI_LOWEFF_CTL0));

		/* wmask msel */
		value = (value & ~(Mask_master << offset_wmask_master)) | ((*(wmask_msel + chan) & Mask_master) << offset_wmask_master);
		/* age msel */
		value = (value & ~(Mask_master << offset_ageexp_master)) | ((*(ageexp_msel + chan) & Mask_master) << offset_ageexp_master);
		/* age rw */
		value = (value & ~(Mask_rw << offset_rw)) | ((*(ageexp_rw + chan) & Mask_rw) << offset_rw);

		emi_reg_sync_writel(value, BaseAddrCHN_EMI[emi_no][chan] + CHN_EMI_LOWEFF_CTL0);
	} else {
		return -1;
	}
	return BM_REQ_OK;
}

#endif

/* For SEDA3.5 wsct setting*/
/* EMI_DBWX[15:8],    X=A~F   (SEL_MASTER) */
/* RW:    EMI_DBWX[1:0],    X=A~F */
int MET_BM_SetWSCT_master_rw(unsigned int *master , unsigned int *rw, unsigned int emi_no)
{
	volatile unsigned int value, addr;
	int i;

	const unsigned int Mask_master = 0xFF;
	const unsigned int offset_master = 8;

	const unsigned int Mask_rw = 0x3;
	const unsigned int offset_rw = 0;


	for (i=0; i<WSCT_AMOUNT; i++) {
		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_master << offset_master)) | ((*(master+i) & Mask_master) << offset_master);
		value = (value & ~(Mask_rw << offset_rw)) | ((*(rw+i) & Mask_rw) << offset_rw);


		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);
	}

	return BM_REQ_OK;
}

int MET_BM_SetWSCT_high_priority(unsigned int *disable, unsigned int *select, unsigned int emi_no)
{
	volatile unsigned int value, addr;
	int i;

	const unsigned int Mask_disable = 0x1;
	const unsigned int offset_disable  = 2;

	const unsigned int Mask_select = 0xF;
	const unsigned int offset_select  = 28;

	for (i=0;i<WSCT_AMOUNT;i++) {
		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));
		value = (value & ~(Mask_disable << offset_disable)) | ((*(disable+i) & Mask_disable) << offset_disable);
		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);

		/* ultra level setting */
		addr = EMI_DBWA_2ND + i*4;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));
		value = (value & ~(Mask_select << offset_select)) | ((*(select+i) & Mask_select) << offset_select);
		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);
	}

	return BM_REQ_OK;
}

/* busid enbale: EMI_DBWX[3],    X=A~F */
/* busid sel: EMI_DBWX[28:16],    X=A~F  (SEL_ID_TMP) */
/* busid mask : EMI_DBWY[12:0]  ??EMI_DBWY[28:16],  Y=I~K    (SEL_ID_MSK) */
int MET_BM_SetWSCT_busid_idmask(unsigned int *busid, unsigned int *idMask, unsigned int emi_no)
{
	volatile unsigned int value, addr;
	volatile unsigned int enable_tmp, busid_tmp, idmask_tmp;
	int i;

	const unsigned int Mask_busid = 0xFFFF;
	const unsigned int offset_busid  = 16;

	const unsigned int Mask_enable = 0x1;
	const unsigned int offset_enable  = 3;

	const unsigned int Mask_idMask = 0xFFFF;
	const unsigned int offset_idMask_even  = 0;
	const unsigned int offset_idMask_odd  = 16;


	for (i=0;i<WSCT_AMOUNT;i++) {

		/*enable, SEL_ID_TMP*/
		if (*(busid+i)>0xffff) {
			enable_tmp = 0;
			busid_tmp = 0xFFFF;
			idmask_tmp = 0xFFFF;
		}
		else {
			enable_tmp = 1;
			busid_tmp = *(busid+i) & Mask_busid;
			idmask_tmp = *(idMask+i) & Mask_idMask;
		}

		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_busid << offset_busid)) | (busid_tmp << offset_busid);
		value = (value & ~(Mask_enable << offset_enable)) | (enable_tmp << offset_enable);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);

		/*SEL_ID_MSK*/
		addr = EMI_DBWI + (i/2)*4;

		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		if (i%2==0)
			value = (value & ~(Mask_idMask << offset_idMask_even)) | (idmask_tmp << offset_idMask_even);
		else
			value = (value & ~(Mask_idMask << offset_idMask_odd)) | (idmask_tmp << offset_idMask_odd);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);
	}

	return BM_REQ_OK;
}


int MET_BM_SetWSCT_chn_rank_sel(unsigned int *chn_rank_sel, unsigned int emi_no)
{
	volatile unsigned int value, addr;
	int i;

	const unsigned int Mask = 0xF;
	const unsigned int offset = 12;

	for (i=0;i<WSCT_AMOUNT;i++) {
		addr = EMI_DBWA_2ND + i*4;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask << offset)) | ((*(chn_rank_sel+i) & Mask) << offset);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);
	}

	return BM_REQ_OK;
}

int MET_BM_SetWSCT_burst_range(unsigned int *bnd_dis, unsigned int *low_bnd, unsigned int *up_bnd, unsigned int emi_no)
{
	volatile unsigned int value, addr;
	int i;

	const unsigned int Mask_dis = 0x1, Mask_low_bnd = 0x1FF, Mask_up_bnd = 0x1FF;
	const unsigned int offset_dis = 4, offset_low_bnd = 16 , offset_up_bnd = 0 ;


	for (i=0;i<WSCT_AMOUNT;i++) {

		addr = EMI_DBWA + i*4;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_dis << offset_dis)) | ((*(bnd_dis+i) & Mask_dis) << offset_dis);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);


		addr = EMI_DBWA_2ND + i*4;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_low_bnd << offset_low_bnd)) | ((*(low_bnd+i) & Mask_low_bnd) << offset_low_bnd);
		value = (value & ~(Mask_up_bnd << offset_up_bnd)) | ((*(up_bnd+i) & Mask_up_bnd) << offset_up_bnd);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);
	}

	return BM_REQ_OK;

}

int MET_BM_SetTSCT_busid_enable(unsigned int *enable, unsigned int emi_no)
{
	int i;

	for (i=0;i<TSCT_AMOUNT;i++) {
		MET_BM_Set_WsctTsct_id_sel(i, *(enable+i), emi_no);
	}

	return BM_REQ_OK;
}

//use the origin together, MET_BM_SetUltraHighFilter()
/* EMI_TTYPEN_CONA [23:20],  N=1~21   (HPRI_SEL) */
int MET_BM_SetTtype_high_priority_sel(unsigned int _high_priority_filter, unsigned int *select, unsigned int emi_no)
{
	int i;
	volatile unsigned int enable;
	volatile unsigned int value, addr;

	const unsigned int Mask_sel = 0xF;
	const unsigned int offset_sel = 20;

	for (i = 0; i < BM_COUNTER_MAX; i++) {
		if ((_high_priority_filter & (1 << i)) == 0){
			enable = 0;
		}
		else {
			enable = 1;
		}

		MET_BM_SetUltraHighFilter(i + 1, enable, emi_no);

		/* ultra level select */
		addr = EMI_TTYPE1_CONA + i*8;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_sel << offset_sel)) | ((*(select+i) & Mask_sel) << offset_sel);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);

	}

	return BM_REQ_OK;
}


//always call this API to init the reg
//related API, MET_BM_SetbusID, MET_BM_SetbusID_En
int MET_BM_SetTtype_busid_idmask(unsigned int *busid, unsigned int *idMask, int _ttype1_16_en,
								int _ttype17_21_en, unsigned int emi_no)
{
	int i;
	volatile unsigned int value, addr;

	const unsigned int Mask_idMask = 0xFFFF;
	const unsigned int offset_idMask = 0;

	if (_ttype1_16_en != BM_TTYPE1_16_ENABLE) {
		/* mask set 0x1FFF , busid set disable*/
		for (i = 1; i <= 16; i++) {
			*(busid + i - 1) = 0xfffff;
			*(idMask + i - 1) = 0xFFFF;
		}
	}

	if (_ttype17_21_en != BM_TTYPE17_21_ENABLE) {
		for (i = 17; i <= 21; i++) {
			*(busid + i - 1) = 0xfffff;
			*(idMask + i - 1) = 0xFFFF;
		}
	}

	for (i = 1; i <= BM_COUNTER_MAX; i++) {
		MET_BM_SetbusID(i, *(busid + i - 1), emi_no);
		MET_BM_SetbusID_En(i, ( *(busid + i - 1) > 0xffff) ? 0 : 1, emi_no);

		/* set idMask */
		addr = EMI_TTYPE1_CONA + (i-1)*8;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_idMask << offset_idMask)) | ((*(idMask+i-1) & Mask_idMask) << offset_idMask);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);

	}

	return BM_REQ_OK;
}


int MET_BM_SetTtype_chn_rank_sel(unsigned int *chn_rank_sel, unsigned int emi_no)
{
	volatile unsigned int value, addr;
	int i;

	const unsigned int Mask = 0xF;
	const unsigned int offset  = 16;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		addr = EMI_TTYPE1_CONA + i*8;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask << offset)) | ((*(chn_rank_sel+i) & Mask) << offset);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);
	}

	return BM_REQ_OK;
}

int MET_BM_SetTtype_burst_range(unsigned int *bnd_dis, unsigned int *low_bnd, unsigned int *up_bnd, unsigned int emi_no)
{
	volatile unsigned int value, addr;
	int i;

	const unsigned int Mask_dis = 0x1, Mask_low_bnd = 0x1FF, Mask_up_bnd = 0x1FF;
	const unsigned int offset_dis = 24, offset_low_bnd = 16 , offset_up_bnd = 0 ;


	for (i=0;i<BM_COUNTER_MAX;i++) {

		/* set dis bit */
		addr = EMI_TTYPE1_CONA + i*8;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_dis << offset_dis)) | ((*(bnd_dis+i) & Mask_dis) << offset_dis);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);


		addr = EMI_TTYPE1_CONB + i*8;
		value = emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + addr));

		value = (value & ~(Mask_low_bnd << offset_low_bnd)) | ((*(low_bnd+i) & Mask_low_bnd) << offset_low_bnd);
		value = (value & ~(Mask_up_bnd << offset_up_bnd)) | ((*(up_bnd+i) & Mask_up_bnd) << offset_up_bnd);

		emi_reg_sync_writel(value, (unsigned long)BaseAddrEMI[emi_no] + addr);
	}

	return BM_REQ_OK;
}

int MET_BM_SetSLC_pmu_reg(unsigned int counter_num, unsigned int offset, unsigned int filter_setting, unsigned int emi_no)
{
	//volatile unsigned int value;
	//value = 0;
	//value =
	//    ((emi_readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_BMEN2)) & (~(1 << (28 + counter_num)))) |
	//     (enable << (28 + counter_num)));
	//PR_BOOTMSG("emi init!\n")
	emi_reg_sync_writel(filter_setting, (unsigned long)BaseAddrSLC_PMU[emi_no] + offset + (counter_num << 4));

	return BM_REQ_OK;
}

int MET_BM_SetSLC_pmu_cnt_filter(unsigned int *enable, unsigned int *filter0, unsigned int *filter1, unsigned int *bw_lat_sel, unsigned int emi_no)
{
	int i;

	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		if(*(enable+i)){
			MET_BM_SetSLC_pmu_reg(i, SLC_PMU_CNT0_FILTER0, *(filter0+i), emi_no);
			MET_BM_SetSLC_pmu_reg(i, SLC_PMU_CNT0_FILTER1, *(filter1+i), emi_no);
			MET_BM_SetSLC_pmu_reg(i, SLC_PMU_CNT0_BW_LAT_SEL, *(bw_lat_sel+i), emi_no);
		}
	}

	return BM_REQ_OK;
}

unsigned int MET_EMI_Get_CONH_2ND(unsigned int emi_no)
{
	return readl(IOMEM((unsigned long)BaseAddrEMI[emi_no] + EMI_CONH_2ND));
}



/* for file node use*/
/*======================================================================*/
/*	Global variable definitions					*/
/*======================================================================*/
/*ondiemet emi sampling interval in us */
int emi_tsct_enable = 1;
int emi_mdct_enable = 1;
int emi_TP_busfiltr_enable;
int mdmcu_sel_enable;
unsigned int rd_mdmcu_rsv_num = 0x5;
int metemi_func_opt;


int met_emi_regdump;
/* Dynamic MonitorCounter selection !!!EXPERIMENT!!! */
// int msel_enable;
DECLARE_MULTI_EMI_VAR_UINT(msel_enable)

/*use this to set para is belong which one emi */
// unsigned int emi_select = 0xffffffff;
unsigned int parameter_apply = 0xffffffff;
int diff_config_per_emi = 0;
// unsigned int use_first_emi_para_to_all = 1;

/* Global variables */
struct kobject *kobj_emi;
// int rwtype = BM_BOTH_READ_WRITE;
DECLARE_MULTI_EMI_VAR_UINT(rwtype);

/* BW Limiter */
/*#define CNT_COUNTDOWN	(1000-1)*/		/* 1000 * 1ms = 1sec */
/* static int countdown; */
// int bw_limiter_enable = BM_BW_LIMITER_ENABLE; /*phase out @ SEDA3.5*/

/* TTYPE counter */
// int ttype1_16_en = BM_TTYPE1_16_DISABLE;
// int ttype17_21_en = BM_TTYPE17_21_DISABLE;
DECLARE_MULTI_EMI_VAR_INT(ttype1_16_en);
DECLARE_MULTI_EMI_VAR_INT(ttype17_21_en);


int dramc_pdir_enable;
int dram_chann_num = 1;
int DRAM_TYPE;

// int ttype_master_val[21];
// int ttype_busid_val[21];
// int ttype_nbeat_val[21];
// int ttype_nbyte_val[21];
// int ttype_burst_val[21];
// int ttype_rw_val[21];

DECLARE_MULTI_EMI_VAR_INT_ARRAY(ttype_master_val,BM_COUNTER_MAX);
DECLARE_MULTI_EMI_VAR_INT_ARRAY(ttype_busid_val,BM_COUNTER_MAX);
DECLARE_MULTI_EMI_VAR_INT_ARRAY(ttype_nbeat_val,BM_COUNTER_MAX);
DECLARE_MULTI_EMI_VAR_INT_ARRAY(ttype_nbyte_val,BM_COUNTER_MAX);
DECLARE_MULTI_EMI_VAR_INT_ARRAY(ttype_burst_val,BM_COUNTER_MAX);
DECLARE_MULTI_EMI_VAR_INT_ARRAY(ttype_rw_val,BM_COUNTER_MAX);


// unsigned int msel_group_ext_val[WSCT_AMOUNT];
// unsigned int wsct_rw_val[WSCT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(msel_group_ext_val,WSCT_AMOUNT);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wsct_rw_val,WSCT_AMOUNT);

char* const delim_comma = ",";
char* const delim_coclon = ":";


char msel_group_ext[FILE_NODE_DATA_LEN] = {'\0'};
char wsct_rw[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int WSCT_HPRI_DIS[WSCT_AMOUNT];
// unsigned int WSCT_HPRI_SEL[WSCT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(WSCT_HPRI_DIS,WSCT_AMOUNT);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(WSCT_HPRI_SEL,WSCT_AMOUNT);

char wsct_high_priority_enable[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int wsct_busid_val[WSCT_AMOUNT];
// unsigned int wsct_idMask_val[WSCT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wsct_busid_val,WSCT_AMOUNT);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wsct_idMask_val,WSCT_AMOUNT);


char wsct_busid[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int  wsct_chn_rank_sel_val[WSCT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wsct_chn_rank_sel_val,WSCT_AMOUNT);
char wsct_chn_rank_sel[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int  wsct_byte_low_bnd_val[WSCT_AMOUNT];
// unsigned int  wsct_byte_up_bnd_val[WSCT_AMOUNT];
// unsigned int  wsct_byte_bnd_dis[WSCT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wsct_byte_low_bnd_val,WSCT_AMOUNT);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wsct_byte_up_bnd_val,WSCT_AMOUNT);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wsct_byte_bnd_dis,WSCT_AMOUNT);
char wsct_burst_range[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int tsct_busid_enable_val[TSCT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(tsct_busid_enable_val,TSCT_AMOUNT);
char tsct_busid_enable[FILE_NODE_DATA_LEN] = {'\0'};

/* use the origin para high_priority_filter to save the en/dis setting */
// int high_priority_filter;
// unsigned int TTYPE_HPRI_SEL[BM_COUNTER_MAX];
DECLARE_MULTI_EMI_VAR_UINT(high_priority_filter);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(TTYPE_HPRI_SEL,BM_COUNTER_MAX);
char ttype_high_priority_ext[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int ttype_idMask_val[BM_COUNTER_MAX];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(ttype_idMask_val,BM_COUNTER_MAX);
char ttype_busid_ext[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int  ttype_chn_rank_sel_val[BM_COUNTER_MAX];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(ttype_chn_rank_sel_val,BM_COUNTER_MAX);
char ttype_chn_rank_sel[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int  ttype_byte_low_bnd_val[BM_COUNTER_MAX];
// unsigned int  ttype_byte_up_bnd_val[BM_COUNTER_MAX];
// unsigned int  ttype_byte_bnd_dis[BM_COUNTER_MAX];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(ttype_byte_low_bnd_val,BM_COUNTER_MAX);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(ttype_byte_up_bnd_val,BM_COUNTER_MAX);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(ttype_byte_bnd_dis,BM_COUNTER_MAX);
char ttype_burst_range[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int wmask_msel_val[EMI][DRAM_CH_NUM_PER_EMI];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(wmask_msel_val,MET_MAX_DRAM_CH_NUM);
char wmask_msel[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int ageexp_msel_val[EMI_NUM][MET_MAX_DRAM_CH_NUM];
// unsigned int ageexp_rw_val[EMI_NUM][MET_MAX_DRAM_CH_NUM];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(ageexp_msel_val,MET_MAX_DRAM_CH_NUM);
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(ageexp_rw_val,MET_MAX_DRAM_CH_NUM);
char ageexp_msel_rw[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int slc_pmu_cnt_setting_enable_val[SLC_PMU_CNT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(slc_pmu_cnt_setting_enable_val,SLC_PMU_CNT_AMOUNT);
char slc_pmu_cnt_setting_enable[FILE_NODE_DATA_LEN] = {'\0'};

// unsigned int slc_pmu_cnt_filter0_val[SLC_PMU_CNT_AMOUNT];
// unsigned int slc_pmu_cnt_filter1_val[SLC_PMU_CNT_AMOUNT];
// unsigned int slc_pmu_cnt_bw_lat_sel_val[SLC_PMU_CNT_AMOUNT];
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(slc_pmu_cnt_filter0_val,SLC_PMU_CNT_AMOUNT);
char slc_pmu_cnt_filter0[FILE_NODE_DATA_LEN] = {'\0'};
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(slc_pmu_cnt_filter1_val,SLC_PMU_CNT_AMOUNT);
char slc_pmu_cnt_filter1[FILE_NODE_DATA_LEN] = {'\0'};
DECLARE_MULTI_EMI_VAR_UINT_ARRAY(slc_pmu_cnt_bw_lat_sel_val,SLC_PMU_CNT_AMOUNT);
char slc_pmu_cnt_bw_lat_sel[FILE_NODE_DATA_LEN] = {'\0'};


// int reserve_wsct_setting;
// DECLARE_MULTI_EMI_VAR_UINT(reserve_wsct_setting);


char header_str[MAX_HEADER_LEN];

unsigned int output_header_len;
unsigned int output_str_len;

int emi_use_ondiemet = 0;
int emi_inited;


char default_val[FILE_NODE_DATA_LEN] = {'\0'};

void _clear_default_val(void) {
	met_emi_default_val[e_MET_DRAM_FREQ] = DRAM_FREQ_DEFAULT;
	met_emi_default_val[e_MET_DRAM_TYPE] = DRAM_TYPE_DEFAULT;
	met_emi_default_val[e_MET_DDR_RATIO] = DDR_RATIO_DEFAULT;
	/*WSCT 4~5 default is ultra, pre-ultra total*/
	default_val[0] = '\0';
}

ssize_t default_val_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= default_val;
	char *_id = NULL, *_value = NULL;
	int id_int = 0;

	_clear_default_val();

	ret = snprintf(default_val, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	default_val[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_value = strsep(&token, delim_coclon);

		// PR_BOOTMSG("_id[%s] _value[%s]\n",_id,_value);

		if (_id == NULL || _value == NULL) {
			PR_BOOTMSG("err: _id[%s] _value[%s], para can't be NULL\n",_id,_value);
			_clear_default_val();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to int err\n",_id);
			_clear_default_val();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < NR_MET_DEFAULT_VAL) {
			if (kstrtouint(_value, 0, &met_emi_default_val[id_int]) != 0) {
				PR_BOOTMSG("_value[%s] trans to hex err\n",_value);
				_clear_default_val();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, NR_MET_DEFAULT_VAL-1);
			_clear_default_val();
			return -EINVAL;
		}
	}
	return n;
}

ssize_t default_val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;
	int ret = 0;

	for (i=0; i<NR_MET_DEFAULT_VAL; i++)
	{
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d:%d \n", i, met_emi_default_val[i]);
	}
	return strlen(buf);
}

void _clear_msel_group_ext(void) {
	int i,j;
	for(j=0;j<EMI_NUM;j++) {
		for (i=0; i<WSCT_AMOUNT; i++) {
			msel_group_ext_val_[j][i] = BM_MASTER_ALL;
		}
	}
	/*WSCT 4~5 default is ultra, pre-ultra total*/
	msel_group_ext[0] = '\0';
}

ssize_t msel_group_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	/*parse wsct_id:group,
	1. split data  by ","
	2. split subdata by ":"
	3. check the value is OK

	don't clear the setting, do this by echo 1 > clear_setting
	*/

	int ret;
	char *token, *cur= msel_group_ext;
	char *_id = NULL, *_master_group = NULL;
	int id_int = 0;

	_clear_msel_group_ext();

	ret = snprintf(msel_group_ext, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	msel_group_ext[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_master_group = strsep(&token, delim_coclon);

		// PR_BOOTMSG("_id[%s] _master_group[%s]\n",_id,_master_group);

		if (_id == NULL || _master_group == NULL) {
			PR_BOOTMSG("err: _id[%s] _master_group[%s], para can't be NULL\n",_id,_master_group);
			_clear_msel_group_ext();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_msel_group_ext();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			if (kstrtouint(_master_group, 0, &msel_group_ext_val[id_int]) != 0) {
				PR_BOOTMSG("master_group[%s] trans to hex err\n",_master_group);
				_clear_msel_group_ext();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_msel_group_ext();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",msel_group_ext);
	/*PR_BOOTMSG("msel_group_ext_store size para n[%d]\n",n);*/
	int i;
	PR_BOOTMSG("save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,msel_group_ext_val[i]);
	}
#endif
	return n;
}

ssize_t msel_group_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, msel_group_ext_val[i]);
	}
	return strlen(buf);
}

void _clear_wsct_rw(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_rw_val[i] = BM_WSCT_RW_RWBOTH;
	}
	wsct_rw[0] = '\0';
}

ssize_t wsct_rw_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= wsct_rw;
	char *_id = NULL, *_rw_type = NULL;
	int id_int = 0;

	_clear_wsct_rw();

	ret = snprintf(wsct_rw, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	wsct_rw[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_rw_type = strsep(&token, delim_coclon);

		if (_id == NULL || _rw_type == NULL) {
			PR_BOOTMSG("err: _id[%s] _rw_type[%s], para can't be NULL\n",_id, _rw_type);
			_clear_wsct_rw();
			return -EINVAL;
		}

		// PR_BOOTMSG("_id[%s] _rw_type[%s]\n",_id, _rw_type);
		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_rw();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			if ( 0 == strncmp("NONE",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_DISABLE;
			else if (0 == strncmp("R",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_READONLY;
			else if (0 == strncmp("W",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_WRITEONLY;
			else if (0 == strncmp("RW",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_RWBOTH;
			else {
				PR_BOOTMSG("_id[%s] has err rwtype[%s]\n", _id, _rw_type);
				_clear_wsct_rw();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_rw();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("wsct_rw_store input data [%s]\n",wsct_rw);
	int i;
	PR_BOOTMSG("rwtype save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%d\n",i,wsct_rw_val[i]);
	}
#endif
	return n;
}

ssize_t wsct_rw_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, wsct_rw_val[i]);
	}
	return strlen(buf);
}

void _clear_wsct_high_priority_enable(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		WSCT_HPRI_DIS[i] = 1;
		WSCT_HPRI_SEL[i] = 0xF;
	}

	// WSCT_HPRI_DIS[4] = 0;
	// WSCT_HPRI_SEL[4] = 0x8;  /* ultra */

	// WSCT_HPRI_DIS[5] = 0;
	// WSCT_HPRI_SEL[5] = 0x4; /* pre_ultra */


	wsct_high_priority_enable[0] = '\0';
}

ssize_t wsct_high_priority_enable_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= wsct_high_priority_enable;
	char *_id = NULL, *_enable = NULL,  *_level = NULL;
	int  id_int = 0, level_int = 0;

	_clear_wsct_high_priority_enable();

	ret = snprintf(wsct_high_priority_enable, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	wsct_high_priority_enable[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_enable = strsep(&token, delim_coclon);
		_level = strsep(&token, delim_coclon);

		/*PR_BOOTMSG("_id[%s] _enable[%s] _level[%s]\n",_id, _enable, _level);*/

		if (_id == NULL || _enable == NULL || _level == NULL ) {
			PR_BOOTMSG("err : _id[%s] _enable[%s] _level[%s], para can't be NULL\n",_id, _enable, _level);
			_clear_wsct_high_priority_enable();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_high_priority_enable();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			if ( 0 == strncmp("disable", _enable, 7)) {

				WSCT_HPRI_DIS[id_int] = 1;
				WSCT_HPRI_SEL[id_int] = 0xf;
			} else if ( 0 == strncmp("enable", _enable, 6)) {

				WSCT_HPRI_DIS[id_int] = 0;
				if (kstrtouint(_level, 0, &level_int) != 0) {
					PR_BOOTMSG("_id[%s] trans ultraLevel[%s] to hex err\n",_id, _level);
					_clear_wsct_high_priority_enable();
					return -EINVAL;
				}
				WSCT_HPRI_SEL[id_int] = level_int & 0xF;
			} else {
				PR_BOOTMSG("_id[%s] has err enable[%s] (enable/disable)\n", _id, _enable);
				_clear_wsct_high_priority_enable();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_high_priority_enable();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",wsct_high_priority_enable);
	int i;
	PR_BOOTMSG("wsct_high_priority_enable save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=(%X,%X)\n", i, WSCT_HPRI_DIS[i], WSCT_HPRI_SEL[i]);
	}
#endif
	return n;
}

ssize_t wsct_high_priority_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X:%X\n", i, WSCT_HPRI_DIS[i], WSCT_HPRI_SEL[i]);
	}
	return strlen(buf);
}

void _clear_wsct_busid(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_busid_val[i] = 0xfffff;
		wsct_idMask_val[i] = 0xFFFF;
	}
	wsct_busid[0] = '\0';
}

ssize_t wsct_busid_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= wsct_busid;

	char *_id = NULL, *_busid = NULL, *_idMask = NULL;
	int id_int = 0, busid_int = 0, idMask_int = 0;

	_clear_wsct_busid();

	ret = snprintf(wsct_busid, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	wsct_busid[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_busid = strsep(&token, delim_coclon);
		_idMask = strsep(&token, delim_coclon);

		pr_debug("_id[%s] _busid[%s] _idMask[%s]\n",_id, _busid, _idMask);

		if (_id == NULL || _busid == NULL || _idMask == NULL) {
			PR_BOOTMSG("err: _id[%s] _busid[%s] _idMask[%s] ,parameter can't be NULL\n",_id, _busid, _idMask);
			_clear_wsct_busid();
			return -EINVAL;
		}


		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_busid();
			return -EINVAL;
		}
		if (kstrtouint(_busid, 0, &busid_int) != 0) {
			PR_BOOTMSG("_busid[%s] trans to hex err\n",_busid);
			_clear_wsct_busid();
			return -EINVAL;
		}
		if (kstrtouint(_idMask, 0, &idMask_int) != 0) {
			PR_BOOTMSG("_idMask[%s] trans to hex err\n",_idMask);
			_clear_wsct_busid();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			wsct_busid_val[id_int] = busid_int;
			wsct_idMask_val[id_int] = idMask_int;

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_busid();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",wsct_busid);
	int i;
	PR_BOOTMSG("wsct_busid save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d](busid,idMask)=(%X,%X)\n", i, wsct_busid_val[i], wsct_idMask_val[i]);
	}
#endif
	return n;
}

ssize_t wsct_busid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X:%X\n", i, wsct_busid_val[i], wsct_idMask_val[i]);
	}
	return strlen(buf);
}


void _clear_wsct_chn_rank_sel(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_chn_rank_sel_val[i] = 0xF;
	}
	wsct_chn_rank_sel[0] = '\0';
}

ssize_t wsct_chn_rank_sel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= wsct_chn_rank_sel;
	char *_id = NULL, *_chn_rank = NULL;
	int id_int = 0, chn_rank_int = 0;

	_clear_wsct_chn_rank_sel();

	ret = snprintf(wsct_chn_rank_sel, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	wsct_chn_rank_sel[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_chn_rank = strsep(&token, delim_coclon);

		pr_debug("_id[%s] _chn_rank[%s]\n",_id, _chn_rank);

		if (_id == NULL || _chn_rank == NULL) {
			PR_BOOTMSG("err : _id[%s] _chn_rank[%s], para can't be NULL\n",_id, _chn_rank);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}


		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}
		if (kstrtouint(_chn_rank, 0, &chn_rank_int) != 0) {
			PR_BOOTMSG("_chn_rank[%s] trans to hex err\n",_id);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}

		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			wsct_chn_rank_sel_val[id_int] = chn_rank_int;

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("wsct_chn_rank_sel input data [%s]\n",wsct_chn_rank_sel);
	int i;
	PR_BOOTMSG("wsct_chn_rank_sel_val save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,wsct_chn_rank_sel_val[i]);
	}
#endif
	return n;
}

ssize_t wsct_chn_rank_sel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, wsct_chn_rank_sel_val[i]);
	}
	return strlen(buf);
}

void _clear_wsct_burst_range(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_byte_low_bnd_val[i] = 0x0;
		wsct_byte_up_bnd_val[i] = 0x1FF;
		wsct_byte_bnd_dis[i] = 1;
	}
	wsct_burst_range[0] = '\0';
}

 ssize_t wsct_burst_range_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= wsct_burst_range;
	char *_id = NULL, *_low_bnd = NULL, *_up_bnd = NULL;
	int id_int = 0, low_bnd_int = 0, up_bnd_int = 0;

	_clear_wsct_burst_range();

	ret = snprintf(wsct_burst_range, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	wsct_burst_range[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_low_bnd = strsep(&token, delim_coclon);
		_up_bnd = strsep(&token, delim_coclon);

		pr_debug("_id[%s] _low_bnd[%s] _up_bnd[%s]\n",_id, _low_bnd, _up_bnd);

		if (_id == NULL || _low_bnd == NULL || _up_bnd == NULL) {
			PR_BOOTMSG("err : _id[%s] _low_bnd[%s] _up_bnd[%s], para can't be NULL\n",_id, _low_bnd, _up_bnd);
			_clear_wsct_burst_range();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_low_bnd, 0, &low_bnd_int) != 0) {
			PR_BOOTMSG("_low_bnd[%s] trans to hex err\n",_id);
			_clear_wsct_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_up_bnd, 0, &up_bnd_int) != 0) {
			PR_BOOTMSG("_up_bnd[%s] trans to hex err\n",_id);
			_clear_wsct_burst_range();
			return -EINVAL;
		}

		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			wsct_byte_low_bnd_val[id_int] = low_bnd_int;
			wsct_byte_up_bnd_val[id_int] = up_bnd_int;
			wsct_byte_bnd_dis[id_int] = 0;
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_burst_range();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("wsct_burst_range_store input data [%s]\n",wsct_burst_range);
	int i;
	PR_BOOTMSG("wsct_burst_range save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d](low_bnd,up_bnd)=(%X,%X)\n",i,wsct_byte_low_bnd_val[i],wsct_byte_up_bnd_val[i]);
	}
#endif
	return n;
}

ssize_t wsct_burst_range_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%d:%X:%X\n",
						i, wsct_byte_bnd_dis[i],wsct_byte_low_bnd_val[i], wsct_byte_up_bnd_val[i]);
	}
	return strlen(buf);
}

void _clear_tsct_busid_enable(void) {
	int i;

	for (i=0;i<TSCT_AMOUNT;i++) {
		tsct_busid_enable_val[i] = 0;
	}
	tsct_busid_enable[0] = '\0';
}

ssize_t tsct_busid_enable_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= tsct_busid_enable;
	char *_id = NULL, *_enable = NULL;
	int  id_int = 0;

	_clear_tsct_busid_enable();

	ret = snprintf(tsct_busid_enable, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	tsct_busid_enable[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_enable = strsep(&token, delim_coclon);


		pr_debug("_id[%s] _enable[%s]\n",_id, _enable);

		if (_id == NULL || _enable == NULL) {
			PR_BOOTMSG("err : _id[%s] _enable[%s], para can't be NULL\n",_id, _enable);
			_clear_tsct_busid_enable();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_tsct_busid_enable();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < TSCT_AMOUNT) {
			if ( 0 == strncmp("disable", _enable, 7)) {
				tsct_busid_enable_val[id_int] = 0;
			} else if ( 0 == strncmp("enable", _enable, 6)) {
				tsct_busid_enable_val[id_int] = 1;
			} else {
				PR_BOOTMSG("_id[%s] has err enable[%s] (enable/disable)\n", _id, _enable);
				_clear_tsct_busid_enable();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, TSCT_AMOUNT-1);
			_clear_tsct_busid_enable();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("tsct_busid_enable input data [%s]\n",tsct_busid_enable);
	int i;
	PR_BOOTMSG("wsct_high_priority_enable save data\n");
	for (i=0;i<TSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=(%d)\n", i, tsct_busid_enable_val[i]);
	}
#endif
	return n;
}

ssize_t tsct_busid_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<TSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, tsct_busid_enable_val[i]);
	}
	return strlen(buf);
}

void _clear_ttype_high_priority_ext(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		TTYPE_HPRI_SEL[i] = 0xf;
	}

	high_priority_filter = 0x0;
	ttype_high_priority_ext[0] = '\0';
}

ssize_t ttype_high_priority_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= ttype_high_priority_ext;
	char *_id = NULL, *_enable = NULL,  *_level = NULL;
	int  id_int = 0, level_int = 0;

	_clear_ttype_high_priority_ext();

	ret = snprintf(ttype_high_priority_ext, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	ttype_high_priority_ext[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_enable = strsep(&token, delim_coclon);
		_level = strsep(&token, delim_coclon);

		pr_debug("_id[%s] _enable[%s] _level[%s]\n",_id, _enable, _level);

		if (_id == NULL || _enable == NULL || _level == NULL ) {
			PR_BOOTMSG("err : _id[%s] _enable[%s] _level[%s], para can't be NULL\n",_id, _enable, _level);
			_clear_ttype_high_priority_ext();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_high_priority_ext();
			return -EINVAL;
		}

		id_int = id_int - 1;
		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			if ( 0 == strncmp("disable", _enable, 7)) {

				high_priority_filter = ( high_priority_filter & ~(1<<id_int) ) | ( 0<<id_int );
				TTYPE_HPRI_SEL[id_int] = 0xf;
			} else if ( 0 == strncmp("enable", _enable, 6)) {

				high_priority_filter = ( high_priority_filter & ~(1<<id_int) ) | ( 1<<id_int );
				if (kstrtouint(_level, 0, &level_int) != 0) {
					PR_BOOTMSG("_id[%s] trans ultraLevel[%s] to hex err\n",_id, _level);
					_clear_ttype_high_priority_ext();
					return -EINVAL;
				}
				TTYPE_HPRI_SEL[id_int] = level_int & 0xF;
			} else {
				PR_BOOTMSG("ttype_high_priority_ext: _id[%s] has err enable[%s] (enable/disable)\n", _id, _enable);
				_clear_ttype_high_priority_ext();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 1~%d\n",id_int+1, BM_COUNTER_MAX);
			_clear_ttype_high_priority_ext();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",ttype_high_priority_ext);

	int i;
	PR_BOOTMSG("wsct_high_priority_enable save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d]=(%X,%X)\n", i+1, high_priority_filter>>i & 0x1, TTYPE_HPRI_SEL[i]);
	}
#endif
	return n;
}

ssize_t ttype_high_priority_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"id[%d]=(%X,%X)\n", i+1, high_priority_filter>>i & 0x1, TTYPE_HPRI_SEL[i]);
	}
	return strlen(buf);
}

void _clear_ttype_busid_ext(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ttype_busid_val[i] = 0xfffff;
		ttype_idMask_val[i] = 0xFFFF;
	}
	ttype_busid_ext[0] = '\0';
}

/*id: 1~21*/
ssize_t ttype_busid_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= ttype_busid_ext;

	char *_id = NULL, *_busid = NULL, *_idMask = NULL;
	int id_int = 0, busid_int = 0, idMask_int = 0;

	_clear_ttype_busid_ext();

	ret = snprintf(ttype_busid_ext, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	ttype_busid_ext[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_busid = strsep(&token, delim_coclon);
		_idMask = strsep(&token, delim_coclon);

		pr_debug("_id[%s] _busid[%s] _idMask[%s]\n",_id, _busid, _idMask);

		if (_id == NULL || _busid == NULL || _idMask == NULL) {
			PR_BOOTMSG("err: ttype_busid_ext _id[%s] _busid[%s] _idMask[%s] ,parameter can't be NULL\n",_id, _busid, _idMask);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}
		if (kstrtouint(_busid, 0, &busid_int) != 0) {
			PR_BOOTMSG("_busid[%s] trans to hex err\n",_busid);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}
		if (kstrtouint(_idMask, 0, &idMask_int) != 0) {
			PR_BOOTMSG("_idMask[%s] trans to hex err\n",_idMask);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}

		id_int = id_int - 1;
		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			ttype_busid_val[id_int] = busid_int;
			ttype_idMask_val[id_int] = idMask_int;

		} else {
			PR_BOOTMSG("ttype_busid_ext id[%d] exceed the range, it must be 1~%d\n",id_int+1, BM_COUNTER_MAX);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("ttype_busid_ext input data [%s]\n",ttype_busid_ext);

	int i;
	PR_BOOTMSG("ttype_busid_ext save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d](busid,idMask)=(%X,%X)\n", i+1, ttype_busid_val[i], ttype_idMask_val[i]);
	}
#endif
	return n;
}

ssize_t ttype_busid_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"id[%d](busid,idMask)=(%X,%X)\n", i+1, ttype_busid_val[i], ttype_idMask_val[i]);
	}
	return strlen(buf);
}

void _clear_ttype_chn_rank_sel(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ttype_chn_rank_sel_val[i] = 0xF;
	}
	ttype_chn_rank_sel[0] = '\0';
}

ssize_t ttype_chn_rank_sel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= ttype_chn_rank_sel;
	char *_id = NULL, *_chn_rank = NULL;
	int id_int = 0, chn_rank_int = 0;

	_clear_ttype_chn_rank_sel();

	ret = snprintf(ttype_chn_rank_sel, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	ttype_chn_rank_sel[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_chn_rank = strsep(&token, delim_coclon);

		pr_debug("_id[%s] _chn_rank[%s]\n",_id, _chn_rank);

		if (_id == NULL || _chn_rank == NULL) {
			PR_BOOTMSG("err (ttype_chn_rank_sel): _id[%s] _chn_rank[%s], para can't be NULL\n",_id, _chn_rank);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}


		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}
		if (kstrtouint(_chn_rank, 0, &chn_rank_int) != 0) {
			PR_BOOTMSG("_chn_rank[%s] trans to hex err\n",_id);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}

		id_int = id_int -1;

		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			ttype_chn_rank_sel[id_int] = chn_rank_int;

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 1~%d\n",id_int+1, BM_COUNTER_MAX);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("ttype_chn_rank_sel input data [%s]\n",ttype_chn_rank_sel);

	int i;
	PR_BOOTMSG("wsct_chn_rank_sel_val save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i+1,ttype_chn_rank_sel[i]);
	}
#endif
	return n;
}

ssize_t ttype_chn_rank_sel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"id[%d]=%X\n",i+1,ttype_chn_rank_sel[i]);
	}
	return strlen(buf);
}

void _clear_ttype_burst_range(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ttype_byte_low_bnd_val[i] = 0x0;
		ttype_byte_up_bnd_val[i] = 0x1FF;
		ttype_byte_bnd_dis[i] = 1;
	}
	ttype_burst_range[0] = '\0';
}

ssize_t ttype_burst_range_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= ttype_burst_range;
	char *_id = NULL, *_low_bnd = NULL, *_up_bnd = NULL;
	int id_int = 0, low_bnd_int = 0, up_bnd_int = 0;

	_clear_ttype_burst_range();

	ret = snprintf(ttype_burst_range, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	ttype_burst_range[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_low_bnd = strsep(&token, delim_coclon);
		_up_bnd = strsep(&token, delim_coclon);

		pr_debug("_id[%s] _low_bnd[%s] _up_bnd[%s]\n",_id, _low_bnd, _up_bnd);

		if (_id == NULL || _low_bnd == NULL || _up_bnd == NULL) {
			PR_BOOTMSG("err (ttype_burst_range): _id[%s] _low_bnd[%s] _up_bnd[%s], para can't be NULL\n",
				        _id, _low_bnd, _up_bnd);
			_clear_ttype_burst_range();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_low_bnd, 0, &low_bnd_int) != 0) {
			PR_BOOTMSG("_low_bnd[%s] trans to hex err\n",_id);
			_clear_ttype_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_up_bnd, 0, &up_bnd_int) != 0) {
			PR_BOOTMSG("_up_bnd[%s] trans to hex err\n",_id);
			_clear_ttype_burst_range();
			return -EINVAL;
		}

		id_int = id_int - 1;
		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			ttype_byte_low_bnd_val[id_int] = low_bnd_int;
			ttype_byte_up_bnd_val[id_int] = up_bnd_int;
			ttype_byte_bnd_dis[id_int] = 0;
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 1~%d\n",id_int, BM_COUNTER_MAX);
			_clear_ttype_burst_range();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("ttype_burst_range_store input data [%s]\n",ttype_burst_range);

	int i;
	PR_BOOTMSG("ttype_burst_range save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d](low_bnd,up_bnd)=(%X,%X)\n",i+1,ttype_byte_low_bnd_val[i],ttype_byte_up_bnd_val[i]);
	}
#endif
	return n;
}

ssize_t ttype_burst_range_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"id[%d](low_bnd,up_bnd)=(%X,%X)\n",
								i+1, ttype_byte_low_bnd_val[i], ttype_byte_up_bnd_val[i]);
	}
	return strlen(buf);
}

void _clear_wmask_msel(void)
{
	int i,j;
	for(j=0;j<EMI_NUM;j++){
		for(i=0;i<DRAM_CH_NUM_PER_EMI;i++) {
			wmask_msel_val_[j][i] = 0xFF;
		}
	}
	wmask_msel[0] = '\0';
}

ssize_t wmask_msel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	/* EX: 0:rw,1:r */
	int ret;
	char *token, *cur= wmask_msel;
	char *_id = NULL, *_master_group = NULL;
	int id_int = 0;

	_clear_wmask_msel();

	ret = snprintf(wmask_msel, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	wmask_msel[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_master_group = strsep(&token, delim_coclon);

		pr_debug("wmask_msel: _id[%s] _master_group[%s]\n",_id,_master_group);

		if (_id == NULL || _master_group == NULL) {
			PR_BOOTMSG("wmask_msel err: _id[%s] _master_group[%s], para can't be NULL\n",_id,_master_group);
			_clear_wmask_msel();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("wmask_msel _id[%s] trans to hex err\n",_id);
			_clear_wmask_msel();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < DRAM_CH_NUM_PER_EMI) {
			if (kstrtouint(_master_group, 0, &wmask_msel_val[id_int]) != 0) {
				PR_BOOTMSG("master_group[%s] trans to hex err\n",_master_group);
				_clear_wmask_msel();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("wmask_msel id[%d] exceed the range, it must be 0~%d\n",id_int, DRAM_CH_NUM_PER_EMI-1);
			_clear_wmask_msel();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",wmask_msel);
	/*PR_BOOTMSG("msel_group_ext_store size para n[%d]\n",n);*/
	int i;
	PR_BOOTMSG("save data\n");
	for (i=0;i<DRAM_CH_NUM_PER_EMI;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,wmask_msel_val[i]);
	}
#endif
	return n;
}

ssize_t wmask_msel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;  i<dram_chann_num && i< DRAM_CH_NUM_PER_EMI; i++) {
		ret += snprintf( buf + ret, PAGE_SIZE-ret, "%d:%d \n",i ,wmask_msel_val[i]);
	}

	return strlen(buf);
}

void _clear_ageexp_msel_rw(void)
{
	int i;

	for(i=0;i<DRAM_CH_NUM_PER_EMI;i++) {
		ageexp_msel_val[i] = 0xFF;
		ageexp_rw_val[i] = 0x3;
	}
	ageexp_msel_rw[0] = '\0';
}

ssize_t ageexp_msel_rw_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	/* EX: 0:rw,1:r */
	int ret;
	char *token, *cur= ageexp_msel_rw;
	char *_id = NULL, *_master_group = NULL, *_rw_type = NULL;
	int id_int = 0;

	_clear_ageexp_msel_rw();

	ret = snprintf(ageexp_msel_rw, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	ageexp_msel_rw[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_master_group = strsep(&token, delim_coclon);
		_rw_type = strsep(&token, delim_coclon);

		pr_debug("ageexp_msel_rw: _id[%s] _master_group[%s] rwtype[%s]\n",_id,_master_group,_rw_type);

		if (_id == NULL || _master_group == NULL || _rw_type==NULL) {
			PR_BOOTMSG("ageexp_msel_rw err: _id[%s] _master_group[%s] rwtype[%s], para can't be NULL\n",_id,_master_group,_rw_type);
			_clear_ageexp_msel_rw();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("ageexp_msel_rw _id[%s] trans to hex err\n",_id);
			_clear_ageexp_msel_rw();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < DRAM_CH_NUM_PER_EMI) {
			if (kstrtouint(_master_group, 0, &ageexp_msel_val[id_int]) != 0) {
				PR_BOOTMSG("ageexp_msel_rw master_group[%s] trans to hex err\n",_master_group);
				_clear_ageexp_msel_rw();
				return -EINVAL;
			}

			if ( 0 == strncmp("NONE",_rw_type,4))
				ageexp_rw_val[id_int] = BM_WSCT_RW_DISABLE;
			else if (0 == strncmp("R",_rw_type,4))
				ageexp_rw_val[id_int] = BM_WSCT_RW_READONLY;
			else if (0 == strncmp("W",_rw_type,4))
				ageexp_rw_val[id_int] = BM_WSCT_RW_WRITEONLY;
			else if (0 == strncmp("RW",_rw_type,4))
				ageexp_rw_val[id_int] = BM_WSCT_RW_RWBOTH;
			else {
				PR_BOOTMSG("ageexp_msel_rw _id[%s] has err rwtype[%s]\n", _id, _rw_type);
				_clear_ageexp_msel_rw();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("ageexp_msel_rw id[%d] exceed the range, it must be 0~%d\n",id_int, DRAM_CH_NUM_PER_EMI-1);
			_clear_ageexp_msel_rw();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",ageexp_msel_rw);
	/*PR_BOOTMSG("msel_group_ext_store size para n[%d]\n",n);*/
	int i;
	PR_BOOTMSG("save data\n");
	for (i=0;i<DRAM_CH_NUM_PER_EMI;i++) {
		PR_BOOTMSG("id[%d]=%X:%X\n",i, ageexp_msel_val[i], ageexp_rw_val[i]);
	}
#endif
	return n;
}

ssize_t ageexp_msel_rw_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;  i<dram_chann_num && i< DRAM_CH_NUM_PER_EMI; i++) {
		ret += snprintf( buf + ret, PAGE_SIZE-ret, "%d:%d:%d \n",i ,ageexp_msel_val[i], ageexp_rw_val[i]);
	}

	return strlen(buf);
}


void _clear_slc_pmu_cnt_setting_enable(void) {
	int i;
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		slc_pmu_cnt_setting_enable_val[i] = 0;
	}
	slc_pmu_cnt_setting_enable[0] = '\0';
}

ssize_t slc_pmu_cnt_setting_enable_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	int ret;
	char *token, *cur= slc_pmu_cnt_setting_enable;
	char *_id = NULL, *_enable = NULL;
	int  id_int = 0;

	_clear_slc_pmu_cnt_setting_enable();

	ret = snprintf(slc_pmu_cnt_setting_enable, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	slc_pmu_cnt_setting_enable[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_enable = strsep(&token, delim_coclon);


		pr_debug("_id[%s] _enable[%s]\n",_id, _enable);

		if (_id == NULL || _enable == NULL) {
			PR_BOOTMSG("err : _id[%s] _enable[%s], para can't be NULL\n",_id, _enable);
			_clear_slc_pmu_cnt_setting_enable();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_slc_pmu_cnt_setting_enable();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < SLC_PMU_CNT_AMOUNT) {
			if ( 0 == strncmp("disable", _enable, 7)) {
				slc_pmu_cnt_setting_enable_val[id_int] = 0;
			} else if ( 0 == strncmp("enable", _enable, 6)) {
				slc_pmu_cnt_setting_enable_val[id_int] = 1;
			} else {
				PR_BOOTMSG("_id[%s] has err enable[%s] (enable/disable)\n", _id, _enable);
				_clear_slc_pmu_cnt_setting_enable();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, SLC_PMU_CNT_AMOUNT-1);
			_clear_slc_pmu_cnt_setting_enable();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("slc_pmu_cnt_setting_enable input data [%s]\n",slc_pmu_cnt_setting_enable);
	int i;
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=(%d)\n", i, slc_pmu_cnt_setting_enable_val[i]);
	}
#endif
	return n;
}

ssize_t slc_pmu_cnt_setting_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, slc_pmu_cnt_setting_enable_val[i]);
	}
	return strlen(buf);
}

void _clear_slc_pmu_cnt_filter0(void) {
	int i;
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		slc_pmu_cnt_filter0_val[i] = 0;
	}
	slc_pmu_cnt_filter0[0] = '\0';
}

ssize_t slc_pmu_cnt_filter0_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	/*parse cnt_id:setting,
	1. split data  by ","
	2. split subdata by ":"
	3. check the value is OK

	don't clear the setting, do this by echo 1 > clear_setting
	*/

	int ret;
	char *token, *cur= slc_pmu_cnt_filter0;
	char *_id = NULL, *_setting = NULL;
	int id_int = 0;

	_clear_slc_pmu_cnt_filter0();

	ret = snprintf(slc_pmu_cnt_filter0, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	slc_pmu_cnt_filter0[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_setting = strsep(&token, delim_coclon);

		/* PR_BOOTMSG("_id[%s] _setting[%s]\n",_id,_setting); */

		if (_id == NULL || _setting == NULL) {
			PR_BOOTMSG("err: _id[%s] _setting[%s], para can't be NULL\n",_id,_setting);
			_clear_slc_pmu_cnt_filter0();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_slc_pmu_cnt_filter0();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < SLC_PMU_CNT_AMOUNT) {
			if (kstrtouint(_setting, 0, &slc_pmu_cnt_filter0_val[id_int]) != 0) {
				PR_BOOTMSG("_setting[%s] trans to hex err\n",_setting);
				_clear_slc_pmu_cnt_filter0();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, SLC_PMU_CNT_AMOUNT-1);
			_clear_slc_pmu_cnt_filter0();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",slc_pmu_cnt_filter0);
	int i;
	PR_BOOTMSG("save data\n");
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,slc_pmu_cnt_filter0_val[i]);
	}
#endif
	return n;
}

ssize_t slc_pmu_cnt_filter0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, slc_pmu_cnt_filter0_val[i]);
	}
	return strlen(buf);
}

void _clear_slc_pmu_cnt_filter1(void) {
	int i;
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		slc_pmu_cnt_filter1_val[i] = 0;
	}
	slc_pmu_cnt_filter1[0] = '\0';
}
ssize_t slc_pmu_cnt_filter1_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	/*parse cnt_id:setting,
	1. split data  by ","
	2. split subdata by ":"
	3. check the value is OK

	don't clear the setting, do this by echo 1 > clear_setting
	*/

	int ret;
	char *token, *cur= slc_pmu_cnt_filter1;
	char *_id = NULL, *_setting = NULL;
	int id_int = 0;

	_clear_slc_pmu_cnt_filter1();

	ret = snprintf(slc_pmu_cnt_filter1, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	slc_pmu_cnt_filter1[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_setting = strsep(&token, delim_coclon);

		/* PR_BOOTMSG("_id[%s] _setting[%s]\n",_id,_setting); */

		if (_id == NULL || _setting == NULL) {
			PR_BOOTMSG("err: _id[%s] _setting[%s], para can't be NULL\n",_id,_setting);
			_clear_slc_pmu_cnt_filter1();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_slc_pmu_cnt_filter1();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < SLC_PMU_CNT_AMOUNT) {
			if (kstrtouint(_setting, 0, &slc_pmu_cnt_filter1_val[id_int]) != 0) {
				PR_BOOTMSG("_setting[%s] trans to hex err\n",_setting);
				_clear_slc_pmu_cnt_filter1();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, SLC_PMU_CNT_AMOUNT-1);
			_clear_slc_pmu_cnt_filter1();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",slc_pmu_cnt_filter1);
	int i;
	PR_BOOTMSG("save data\n");
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,slc_pmu_cnt_filter1_val[i]);
	}
#endif
	return n;
}

ssize_t slc_pmu_cnt_filter1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, slc_pmu_cnt_filter1_val[i]);
	}
	return strlen(buf);
}

void _clear_slc_pmu_cnt_bw_lat_sel(void) {
	int i;
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		slc_pmu_cnt_bw_lat_sel_val[i] = 0;
	}
	slc_pmu_cnt_bw_lat_sel[0] = '\0';
}
ssize_t slc_pmu_cnt_bw_lat_sel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	/*parse cnt_id:setting,
	1. split data  by ","
	2. split subdata by ":"
	3. check the value is OK

	don't clear the setting, do this by echo 1 > clear_setting
	*/

	int ret;
	char *token, *cur= slc_pmu_cnt_bw_lat_sel;
	char *_id = NULL, *_setting = NULL;
	int id_int = 0;

	_clear_slc_pmu_cnt_bw_lat_sel();

	ret = snprintf(slc_pmu_cnt_bw_lat_sel, FILE_NODE_DATA_LEN, "%s", buf);
	if (ret < 0) return -EINVAL;

	slc_pmu_cnt_bw_lat_sel[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		/*PR_BOOTMSG("token: %s\n",token);*/
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_setting = strsep(&token, delim_coclon);

		/*PR_BOOTMSG("_id[%s] _setting[%s]\n",_id,_setting); */

		if (_id == NULL || _setting == NULL) {
			PR_BOOTMSG("err: _id[%s] _setting[%s], para can't be NULL\n",_id,_setting);
			_clear_slc_pmu_cnt_bw_lat_sel();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_slc_pmu_cnt_bw_lat_sel();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < SLC_PMU_CNT_AMOUNT) {
			if (kstrtouint(_setting, 0, &slc_pmu_cnt_bw_lat_sel_val[id_int]) != 0) {
				PR_BOOTMSG("_setting[%s] trans to hex err\n",_setting);
				_clear_slc_pmu_cnt_bw_lat_sel();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, SLC_PMU_CNT_AMOUNT-1);
			_clear_slc_pmu_cnt_bw_lat_sel();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",slc_pmu_cnt_bw_lat_sel);
	int i;
	PR_BOOTMSG("save data\n");
	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,slc_pmu_cnt_bw_lat_sel_val[i]);
	}
#endif
	return n;
}

ssize_t slc_pmu_cnt_bw_lat_sel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,"%d:%X\n", i, slc_pmu_cnt_bw_lat_sel_val[i]);
	}
	return strlen(buf);
}

/* KOBJ: emi_clock_rate */
ssize_t sspm_support_feature_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", get_sspm_support_feature());
}


void _clear_setting(void) {
	/*clear all file node para here*/
	int i;
	PR_BOOTMSG("clear EMI file node setting\n");

	_clear_default_val();
	_clear_msel_group_ext();
	_clear_wsct_rw();
	_clear_wsct_high_priority_enable();
	_clear_wsct_busid();
	_clear_wsct_chn_rank_sel();
	_clear_wsct_burst_range();

	_clear_tsct_busid_enable();
	_clear_ttype_high_priority_ext();
	_clear_ttype_busid_ext();
	_clear_ttype_chn_rank_sel();
	_clear_ttype_burst_range();
#ifdef EMI_LOWEFF_SUPPORT
	_clear_wmask_msel();
	_clear_ageexp_msel_rw();
#endif
	// reserve_wsct_setting = 0;
	if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
		_clear_slc_pmu_cnt_setting_enable();
		_clear_slc_pmu_cnt_filter0();
		_clear_slc_pmu_cnt_filter1();
		_clear_slc_pmu_cnt_bw_lat_sel();
	}

	emi_TP_busfiltr_enable = 0;

	high_priority_filter = 0x0;
	rwtype = BM_BOTH_READ_WRITE;
	dramc_pdir_enable = 1;
	met_emi_regdump = 0;

	msel_enable = 0;

	ttype1_16_en = BM_TTYPE1_16_DISABLE;
	ttype17_21_en = BM_TTYPE17_21_DISABLE;


	for (i = 0; i < 21; i++) {
		ttype_master_val[i] = BM_MASTER_M0;
		ttype_nbeat_val[i] = BM_TRANS_TYPE_1BEAT;
		ttype_nbyte_val[i] = BM_TRANS_TYPE_8Byte;
		ttype_burst_val[i] = BM_TRANS_TYPE_BURST_INCR;
		ttype_busid_val[i] = 0xfffff;   /*default disable ttype bus sel if busid > 0xff_ff */
		ttype_rw_val[i] =  BM_TRANS_RW_DEFAULT;
	}
	emi_tsct_enable = 0;
	emi_mdct_enable = 0;
	metemi_func_opt = 0xf00e;
	rd_mdmcu_rsv_num = 0;
	mdmcu_sel_enable = 0;

	parameter_apply = 0xffffffff;

}

ssize_t clear_setting_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value == 1)
		_clear_setting();

	return n;
}

void store_emi_para(unsigned int emi_no)
{
	STORE_EMI_PARA(msel_enable,emi_no);
	STORE_EMI_PARA(rwtype,emi_no);
	STORE_EMI_PARA(ttype1_16_en,emi_no);
	STORE_EMI_PARA(ttype17_21_en,emi_no);
	STORE_EMI_PARA_ARRAY(ttype_master_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_busid_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_nbeat_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_nbyte_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_burst_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_rw_val, emi_no ,unsigned int, BM_COUNTER_MAX);

	STORE_EMI_PARA_ARRAY(TTYPE_HPRI_SEL, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_idMask_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_chn_rank_sel_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_byte_low_bnd_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_byte_up_bnd_val, emi_no ,unsigned int, BM_COUNTER_MAX);
	STORE_EMI_PARA_ARRAY(ttype_byte_bnd_dis, emi_no ,unsigned int, BM_COUNTER_MAX);


	STORE_EMI_PARA_ARRAY(msel_group_ext_val, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(wsct_rw_val, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(WSCT_HPRI_DIS, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(WSCT_HPRI_SEL, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(wsct_busid_val, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(wsct_idMask_val, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(wsct_chn_rank_sel_val, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(wsct_byte_low_bnd_val, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(wsct_byte_up_bnd_val, emi_no ,unsigned int, WSCT_AMOUNT);
	STORE_EMI_PARA_ARRAY(wsct_byte_bnd_dis, emi_no ,unsigned int, WSCT_AMOUNT);

	STORE_EMI_PARA_ARRAY(tsct_busid_enable_val, emi_no ,unsigned int, TSCT_AMOUNT);

	STORE_EMI_PARA(high_priority_filter,emi_no);

	STORE_EMI_PARA_ARRAY(wmask_msel_val, emi_no ,unsigned int, DRAM_CH_NUM_PER_EMI);
	STORE_EMI_PARA_ARRAY(ageexp_msel_val, emi_no ,unsigned int, DRAM_CH_NUM_PER_EMI);
	STORE_EMI_PARA_ARRAY(ageexp_rw_val, emi_no ,unsigned int, DRAM_CH_NUM_PER_EMI);

	if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
		STORE_EMI_PARA_ARRAY(slc_pmu_cnt_setting_enable_val, emi_no ,unsigned int, SLC_PMU_CNT_AMOUNT);
		STORE_EMI_PARA_ARRAY(slc_pmu_cnt_filter0_val, emi_no ,unsigned int, SLC_PMU_CNT_AMOUNT);
		STORE_EMI_PARA_ARRAY(slc_pmu_cnt_filter1_val, emi_no ,unsigned int, SLC_PMU_CNT_AMOUNT);
		STORE_EMI_PARA_ARRAY(slc_pmu_cnt_bw_lat_sel_val, emi_no ,unsigned int, SLC_PMU_CNT_AMOUNT);
	}
	// STORE_EMI_PARA(reserve_wsct_setting,emi_no);
}

ssize_t parameter_apply_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	/*bit31: done , copy data to separate met var array
	  bit30(phase out @12/17): 1 use the first emi para to apply other emi

	*/
	int value,emi_no;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value & 0x80000000)
	{
		/*copy data to separate met var array*/
		for(emi_no=0;emi_no<EMI_NUM;emi_no++)
		{
			if( (value>>emi_no)&0x1)
				store_emi_para(emi_no);
		}
	}

	// if (value & 0x40000000)
	// 	use_first_emi_para_to_all = 1;
	// else
	// 	use_first_emi_para_to_all = 0;

	return n;
}

ssize_t parameter_apply_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%x\n", parameter_apply);
}

struct kobj_attribute clear_setting_attr = __ATTR_WO(clear_setting); // OK
struct kobj_attribute msel_group_ext_attr = __ATTR(msel_group_ext, 0664, msel_group_ext_show, msel_group_ext_store); //OK
struct kobj_attribute wsct_rw_attr = __ATTR(wsct_rw, 0664, wsct_rw_show, wsct_rw_store);
struct kobj_attribute wsct_high_priority_enable_attr = __ATTR(wsct_high_priority_enable, 0664, wsct_high_priority_enable_show, wsct_high_priority_enable_store);
struct kobj_attribute wsct_busid_attr = __ATTR(wsct_busid, 0664, wsct_busid_show, wsct_busid_store);
struct kobj_attribute wsct_chn_rank_sel_attr = __ATTR(wsct_chn_rank_sel, 0664, wsct_chn_rank_sel_show, wsct_chn_rank_sel_store);
struct kobj_attribute wsct_burst_range_attr = __ATTR(wsct_burst_range, 0664, wsct_burst_range_show, wsct_burst_range_store);
struct kobj_attribute tsct_busid_enable_attr = __ATTR(tsct_busid_enable, 0664, tsct_busid_enable_show, tsct_busid_enable_store);
struct kobj_attribute ttype_high_priority_ext_attr = __ATTR(ttype_high_priority_ext, 0664, ttype_high_priority_ext_show, ttype_high_priority_ext_store);
struct kobj_attribute ttype_busid_ext_attr = __ATTR(ttype_busid_ext, 0664, ttype_busid_ext_show, ttype_busid_ext_store);
struct kobj_attribute ttype_chn_rank_sel_attr = __ATTR(ttype_chn_rank_sel, 0664, ttype_chn_rank_sel_show, ttype_chn_rank_sel_store);
struct kobj_attribute ttype_burst_range_attr = __ATTR(ttype_burst_range, 0664, ttype_burst_range_show, ttype_burst_range_store);

struct kobj_attribute wmask_msel_attr = __ATTR(wmask_msel, 0664, wmask_msel_show, wmask_msel_store);
struct kobj_attribute ageexp_msel_rw_attr = __ATTR(ageexp_msel_rw, 0664, ageexp_msel_rw_show, ageexp_msel_rw_store);
struct kobj_attribute default_val_attr = __ATTR(default_val, 0664, default_val_show, default_val_store); //OK
DECLARE_KOBJ_ATTR_RO(sspm_support_feature);
struct kobj_attribute parameter_apply_attr = __ATTR(parameter_apply, 0664, parameter_apply_show, parameter_apply_store); //OK
DECLARE_KOBJ_ATTR_INT(diff_config_per_emi, diff_config_per_emi);

struct kobj_attribute slc_pmu_cnt_setting_enable_attr = __ATTR(slc_pmu_cnt_setting_enable, 0664, slc_pmu_cnt_setting_enable_show, slc_pmu_cnt_setting_enable_store);
struct kobj_attribute slc_pmu_cnt_filter0_attr = __ATTR(slc_pmu_cnt_filter0, 0664, slc_pmu_cnt_filter0_show, slc_pmu_cnt_filter0_store);
struct kobj_attribute slc_pmu_cnt_filter1_attr = __ATTR(slc_pmu_cnt_filter1, 0664, slc_pmu_cnt_filter1_show, slc_pmu_cnt_filter1_store);
struct kobj_attribute slc_pmu_cnt_bw_lat_sel_attr = __ATTR(slc_pmu_cnt_bw_lat_sel, 0664, slc_pmu_cnt_bw_lat_sel_show, slc_pmu_cnt_bw_lat_sel_store);


void emi_init(void)
{
	unsigned int bmrw0_val, bmrw1_val, i, emi_no;
	/*unsigned int msel_group_val[4];*/

	/*save origianl EMI config*/
	MET_BM_SaveCfg();
	/* get dram channel number */
	dram_chann_num = MET_EMI_GetDramChannNum(0);

	/*check if use diff config per emi*/
	if (diff_config_per_emi == 0)
	{
		for( emi_no=0;emi_no<EMI_NUM;emi_no++ )
			store_emi_para(emi_no);
	}


	for( emi_no=0;emi_no<EMI_NUM;emi_no++ )
	{
		/* Init. EMI bus monitor */
		MET_BM_SetReadWriteType(rwtype_[emi_no],emi_no);

		/*latency or */
		if (ttype1_16_en_[emi_no] != BM_TTYPE1_16_ENABLE) {
			MET_BM_SetLatencyCounter(1, emi_no);    /*enable latency count*/

			MET_BM_SetMonitorCounter(1,
							 0xff,   /*set master_sel all for BSCT/BACT... */
							 0x0,
							 emi_no);
		}
		else {
			MET_BM_SetLatencyCounter(0, emi_no);    /*disable latency count*/

			for (i = 1; i <= 16; i++) {
				MET_BM_SetMonitorCounter(i,
							 ttype_master_val_[emi_no][i - 1],
							 ttype_nbeat_val_[emi_no][i - 1] |
							 ttype_nbyte_val_[emi_no][i - 1] |
							 ttype_burst_val_[emi_no][i - 1],
							 emi_no);
			}
		}

		if (ttype17_21_en_[emi_no]  == BM_TTYPE17_21_ENABLE) {
			for (i = 17; i <= 21; i++) {
				MET_BM_SetMonitorCounter(i,
							 ttype_master_val_[emi_no][i - 1],
							 ttype_nbeat_val_[emi_no][i - 1] |
							 ttype_nbyte_val_[emi_no][i - 1] |
							 ttype_burst_val_[emi_no][i - 1],
							 emi_no);
			}
		}

		// PR_BOOTMSG("[%s]reserve_wsct_setting[%d]=%d\n",__func__,emi_no, reserve_wsct_setting_[emi_no]);

		// if (reserve_wsct_setting_[emi_no] == 0) {
		// 	/* wsct 0 : total-all*/
		// 	msel_group_ext_val_[emi_no][0] = BM_MASTER_ALL;
		// 	wsct_rw_val_[emi_no][0] = BM_WSCT_RW_RWBOTH;
		// 	WSCT_HPRI_DIS_[emi_no][0] = 1;
		// 	WSCT_HPRI_SEL_[emi_no][0] = 0xF;
		// 	wsct_busid_val_[emi_no][0] = 0xFFFFF;
		// 	wsct_idMask_val_[emi_no][0] = 0xFFFF;
		// 	wsct_chn_rank_sel_val_[emi_no][0] = 0xF;
		// 	wsct_byte_bnd_dis_[emi_no][0] = 1;

		// 	/* wsct 4 : total-Read , modify @2020/12/17 */
		// 	msel_group_ext_val_[emi_no][4] = BM_MASTER_ALL;
		// 	wsct_rw_val_[emi_no][4] = BM_WSCT_RW_READONLY;
		// 	WSCT_HPRI_DIS_[emi_no][4] = 1;
		// 	WSCT_HPRI_SEL_[emi_no][4] = 0xF;
		// 	wsct_busid_val_[emi_no][4] = 0xFFFFF;
		// 	wsct_idMask_val_[emi_no][4] = 0xFFFF;
		// 	wsct_chn_rank_sel_val_[emi_no][4] = 0xF;
		// 	wsct_byte_bnd_dis_[emi_no][4] = 1;

		// 	/* wsct 5 : total-write , modify @2020/12/17 */
		// 	msel_group_ext_val_[emi_no][5] = BM_MASTER_ALL;
		// 	wsct_rw_val_[emi_no][5] = BM_WSCT_RW_WRITEONLY;
		// 	WSCT_HPRI_DIS_[emi_no][5] = 1;
		// 	WSCT_HPRI_SEL_[emi_no][5] = 0xF;
		// 	wsct_busid_val_[emi_no][5] = 0xFFFFF;
		// 	wsct_idMask_val_[emi_no][5] = 0xFFFF;
		// 	wsct_chn_rank_sel_val_[emi_no][5] = 0xF;
		// 	wsct_byte_bnd_dis_[emi_no][5] = 1;
		// }

		/*if msel_enable is disable, the use total(0xff) to set config*/
		if (msel_enable_[emi_no]==0) {
			for ( i=0; i<WSCT_AMOUNT; i++) {
				msel_group_ext_val_[emi_no][i] = BM_MASTER_ALL;
			}
		}

		MET_BM_SetWSCT_master_rw(msel_group_ext_val_[emi_no], wsct_rw_val_[emi_no], emi_no);
		MET_BM_SetWSCT_high_priority(WSCT_HPRI_DIS_[emi_no], WSCT_HPRI_SEL_[emi_no], emi_no);
		MET_BM_SetWSCT_busid_idmask(wsct_busid_val_[emi_no], wsct_idMask_val_[emi_no], emi_no);
		MET_BM_SetWSCT_chn_rank_sel(wsct_chn_rank_sel_val_[emi_no], emi_no);
		MET_BM_SetWSCT_burst_range(wsct_byte_bnd_dis_[emi_no], wsct_byte_low_bnd_val_[emi_no],
									 wsct_byte_up_bnd_val_[emi_no], emi_no);
		MET_BM_SetTSCT_busid_enable(tsct_busid_enable_val_[emi_no], emi_no);

		MET_BM_SetTtype_high_priority_sel(high_priority_filter_[emi_no], TTYPE_HPRI_SEL_[emi_no], emi_no);
		MET_BM_SetTtype_busid_idmask(ttype_busid_val_[emi_no], ttype_idMask_val_[emi_no],
									ttype1_16_en_[emi_no], ttype17_21_en_[emi_no], emi_no);
		MET_BM_SetTtype_chn_rank_sel(ttype_chn_rank_sel_val_[emi_no], emi_no);
		MET_BM_SetTtype_burst_range(ttype_byte_bnd_dis_[emi_no], ttype_byte_low_bnd_val_[emi_no],
									ttype_byte_up_bnd_val_[emi_no], emi_no);

		if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
			MET_BM_SetSLC_pmu_cnt_filter(slc_pmu_cnt_setting_enable_val_[emi_no], slc_pmu_cnt_filter0_val_[emi_no],
											slc_pmu_cnt_filter1_val_[emi_no], slc_pmu_cnt_bw_lat_sel_val_[emi_no], emi_no);
		}
#ifdef EMI_LOWEFF_SUPPORT
		MET_BM_SetLOWEFF_master_rw(0, wmask_msel_val_[emi_no], ageexp_msel_val_[emi_no],
										ageexp_rw_val_[emi_no],emi_no);
#endif

		bmrw0_val = 0;
		for (i = 0; i < 16; i++)
			bmrw0_val |= (ttype_rw_val_[emi_no][i] << (i * 2));

		bmrw1_val = 0;
		for (i = 16; i < 21; i++)
			bmrw1_val |= (ttype_rw_val_[emi_no][i] << ((i-16) * 2));

		MET_BM_SetTtypeCounterRW(bmrw0_val, bmrw1_val, emi_no);

	} /*end of for( emi_no=0;emi_no<EMI_NUM;emi_no++ )*/

}

/*restore the emi origin setting , prevent infect other module*/
void emi_uninit(void)
{
	MET_BM_RestoreCfg();
}

#ifdef MET_SSPM
void MET_BM_IPI_REGISTER_CB(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[1] = {0};

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | (MID_EMI << MID_BIT_SHIFT) | MET_ARGU | SET_REGISTER_CB;
		ret = met_scmi_to_sspm_command((void *)ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
	}
}


void MET_BM_IPI_configs(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[3] = {0, 0, 0};

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | (MID_EMI << MID_BIT_SHIFT) | MET_ARGU | SET_EBM_CONFIGS1;
		ipi_buf[2] = EMI_VER_MAJOR << 24 | EMI_VER_MINOR << 16 | DRAMC_VER << 8 | 0;
		ret = met_scmi_to_sspm_command((void *)ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
	}
}
#endif

unsigned int get_sspm_support_feature(void)
{
	unsigned int rdata=0;

#ifdef MET_SSPM
	int ret;
	unsigned int ipi_buf[1] = {0};

	if (met_sspm_api_ready && met_scmi_api_ready) {
		if (sspm_buf_available == 1) {
			ipi_buf[0] = MET_MAIN_ID | (MID_EMI << MID_BIT_SHIFT) | MET_REQ_AP2MD ;

			ret = met_scmi_to_sspm_command((void *)ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);

			if (ret != 0) {
				PR_BOOTMSG("met_scmi_to_sspm_command fail(%d)\n", ret);
				rdata = 0;
			}
		}
	}
#endif

	return rdata;
}

unsigned check_sspm_support(unsigned int module_id)
{
	unsigned int suppor_list = get_sspm_support_feature();

	/*PR_BOOTMSG("sspm_support_module:%x\n",suppor_list);*/
#if 1
	return (suppor_list & module_id);
#else
	/*force to AP polling*/
	return 0;
#endif
}



unsigned int MET_GET_DRAM_TYPE()
{
/*
enum DRAM_TYPE {
??TYPE_DDR1 = 1,
??TYPE_LPDDR2,
??TYPE_LPDDR3,
??TYPE_PCDDR3,
??TYPE_LPDDR4,
??TYPE_LPDDR4X,
??TYPE_LPDDR4P
};
*/
	unsigned int dram_type=0;

#if IS_ENABLED(CONFIG_MTK_DRAMC)
	if (mtk_dramc_get_ddr_type_symbol)
		dram_type = mtk_dramc_get_ddr_type_symbol();
#endif
	if (dram_type == 0)
		dram_type = met_emi_default_val[e_MET_DRAM_TYPE];

	return dram_type;
}

/* get the ddr ratio */
unsigned int MET_EMI_Get_BaseClock_Rate(void)
{
	unsigned int ddr_ratio = 0;

#if IS_ENABLED(CONFIG_MTK_DVFSRC_MET)
	if (get_cur_ddr_ratio_symbol)
		ddr_ratio = get_cur_ddr_ratio_symbol();
#endif
	if (ddr_ratio == 0)
		ddr_ratio = met_emi_default_val[e_MET_DDR_RATIO];

	return ddr_ratio;

}

unsigned met_get_dram_data_rate(void)
{
	unsigned int dram_data_rate_MHz = 0;

#if IS_ENABLED(CONFIG_MTK_DRAMC)
	if (mtk_dramc_get_data_rate_symbol)
		dram_data_rate_MHz = mtk_dramc_get_data_rate_symbol();
#endif
	if (dram_data_rate_MHz == 0)
		dram_data_rate_MHz = met_emi_default_val[e_MET_DRAM_FREQ];

	return dram_data_rate_MHz;
}

int emi_create_header(char *buf, int buf_len)
{
	int ret = 0;
	int i = 0;
	int err;
	int emi_no;

	unsigned int dram_data_rate_MHz;
	// unsigned int DRAM_TYPE;  //change to global var
	unsigned int base_clock_rate;


// #ifdef EMI_NUM
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: EMI_NUM: %d\n", EMI_NUM);
// #endif

	dram_chann_num = MET_EMI_GetDramChannNum(0);
	/*	met_dram_chann_num_header
	 *	channel number
	 *	LP4: 2, LP3: 1
	 */

	/*
	 *	the ddr type define :
	 *	enum DDRTYPE {
	 *	TYPE_DDR1 = 1,
	 *	TYPE_LPDDR2,
	 *	TYPE_LPDDR3,
	 *	TYPE_PCDDR3,
	 *	TYPE_LPDDR4,
	 *	TYPE_LPDDR4X,
	 *	TYPE_LPDDR4P
	 *	};
	 */

	//for kernel 5.X, can't use symbol_get
	// if (!get_cur_ddr_ratio_symbol){
	// 	PR_BOOTMSG("[%s][%d]get_cur_ddr_ratio_symbol = NULL , use the TYPE_LPDDR4 setting\n", __func__, __LINE__);
	// 	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ##_EMI_warning: get_cur_ddr_ratio_symbol = NULL , use the TYPE_LPDDR4 setting\n");
	// }

	DRAM_TYPE = MET_GET_DRAM_TYPE();
	base_clock_rate = MET_EMI_Get_BaseClock_Rate();

	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_dram_type: %d\n", DRAM_TYPE);

	if ((DRAM_TYPE == 5) || (DRAM_TYPE == 6) || (DRAM_TYPE == 7))
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_dram_chann_num_header: %d,%d,%d,%d\n",
				dram_chann_num, base_clock_rate,
				DRAM_IO_BUS_WIDTH_LP4, DRAM_DATARATE);
	else
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_dram_chann_num_header: %d,%d,%d,%d\n",
				dram_chann_num, base_clock_rate,
				DRAM_IO_BUS_WIDTH_LP3, DRAM_DATARATE);


	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: metemi_func_opt_header: %d\n",
		metemi_func_opt);


	/* met_emi_clockrate */
	dram_data_rate_MHz = met_get_dram_data_rate();

	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_dram_clockrate: %d\n",dram_data_rate_MHz);

	/* 1 : by ondiemet, 0: by pure linux */
	ret += snprintf(buf + ret, buf_len - ret,
					"met-info [000] 0.0: ##_emi_use_ondiemet: %u,%X\n",
					emi_use_ondiemet, get_sspm_support_feature());

	ret += snprintf(buf + ret, buf_len - ret,
					"met-info [000] 0.0: emi_use_ondiemet: %u,%X\n",
					emi_use_ondiemet, get_sspm_support_feature());

	/*dram bank num*/
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_dram_rank_num_header: %u,%u\n", MET_EMI_GetDramRankNum(0),
				MET_EMI_GetDramRankNum(0));

	/* read from dts, pass to MW to calculate data */
	ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: MET_EMI_support_list: %x\n",MET_EMI_support_list);

	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_diff_config_per_emi: %d\n", diff_config_per_emi);

	ret += snprintf(buf + ret, buf_len - ret,
		"met-info [000] 0.0: met_emi_wsct_amount: %d\n",WSCT_AMOUNT);

	for ( emi_no=0; emi_no<EMI_NUM; emi_no++)
	{
		/* master selection header */
		ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_emi_msel: %d,%x,%x,%x,%x,%x,%x\n", emi_no,
			msel_group_ext_val_[emi_no][0] & BM_MASTER_ALL,
			msel_group_ext_val_[emi_no][1] & BM_MASTER_ALL,
			msel_group_ext_val_[emi_no][2] & BM_MASTER_ALL,
			msel_group_ext_val_[emi_no][3] & BM_MASTER_ALL,
			msel_group_ext_val_[emi_no][4] & BM_MASTER_ALL,
			msel_group_ext_val_[emi_no][5] & BM_MASTER_ALL);

		/*Ttype RW type header*/
		pr_debug("rwtype=%d\n",rwtype);
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_rw_cfg: %d,",emi_no);
		if (rwtype == BM_READ_ONLY)
			ret += snprintf(buf + ret, buf_len - ret, "R");
		else if (rwtype == BM_WRITE_ONLY)
			ret += snprintf(buf + ret, buf_len - ret, "W");
		else
			ret += snprintf(buf + ret, buf_len - ret, "BOTH");

		for (i = 0; i < 21; i++) {
			if (ttype_rw_val[i] == BM_TRANS_RW_DEFAULT)
				ret += snprintf(buf + ret, buf_len - ret, ",DEFAULT");
			else if (ttype_rw_val[i] == BM_TRANS_RW_READONLY)
				ret += snprintf(buf + ret, buf_len - ret, ",R");
			else if (ttype_rw_val[i] == BM_TRANS_RW_WRITEONLY)
				ret += snprintf(buf + ret, buf_len - ret, ",W");
			else    /*BM_TRANS_RW_RWBOTH*/
				ret += snprintf(buf + ret, buf_len - ret, ",BOTH");
		}
		ret += snprintf(buf + ret, buf_len - ret, "\n");

		/*ultra header*/
		ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: met_emi_ultra_filter: %d,%x\n",emi_no, high_priority_filter_[emi_no]);

		/* ttype header */
		if (ttype17_21_en == BM_TTYPE17_21_ENABLE) {
			int i = 0;
			int j = 0;

			/* ttype master list */
			/* todo*/
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_master_list: %d,",emi_no);
			for (i = 0; i < 21; i++) {
				for (j = 0; j < ARRAY_SIZE(ttype_master_list_item); j++) {
					if (ttype_master_val_[emi_no][i] == ttype_master_list_item[j].key) {
						ret += snprintf(buf + ret, buf_len - ret, "%s,", ttype_master_list_item[j].val);
					}
				}
			}
			/* remove the last comma */
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			/* ttype busid list */
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_busid_list: %d,",emi_no);
			for (i = 0; i < 21; i++)
				ret += snprintf(buf + ret, buf_len - ret, "%x,", ttype_busid_val_[emi_no][i]);

			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			/* ttype nbeat list */
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_nbeat_list: %d,",emi_no);
			for (i = 0; i < 21; i++) {
				for (j = 0; j < ARRAY_SIZE(ttype_nbeat_list_item); j++) {
					if (ttype_nbeat_val_[emi_no][i] == ttype_nbeat_list_item[j].key) {
						ret += snprintf(buf + ret, buf_len - ret, "%d,", ttype_nbeat_list_item[j].val);
					}
				}
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			/* ttype nbyte list */
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_nbyte_list: %d,",emi_no);
			for (i = 0; i < 21; i++) {
				for (j = 0; j < ARRAY_SIZE(ttype_nbyte_list_item); j++) {
					if (ttype_nbyte_val_[emi_no][i] == ttype_nbyte_list_item[j].key) {
						ret += snprintf(buf + ret, buf_len - ret, "%d,", ttype_nbyte_list_item[j].val);
					}
				}
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			/* ttype burst list */
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_burst_list: %d,",emi_no);
			for (i = 0; i < 21; i++) {
				for (j = 0; j < ARRAY_SIZE(ttype_burst_list_item); j++) {
					if (ttype_burst_val_[emi_no][i] == ttype_burst_list_item[j].key) {
						ret += snprintf(buf + ret, buf_len - ret, "%s,", ttype_burst_list_item[j].val);
					}
				}
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;
		}
		// /* ttype enable */
		// ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_enable: %d,%d,%d\n",
		// 								emi_no,ttype1_16_en_[emi_no], ttype17_21_en_[emi_no]);


#if 1 /*SEDA 3.5*/

		/* phase out for merge into met_emi_msel , it look more clear */
		// ret += snprintf(buf + ret, buf_len - ret,
		// 	"met-info [000] 0.0: met_emi_msel_ext: %d,%x,%x,%x\n",emi_no,
		// 	msel_group_ext_val_[emi_no][0] & BM_MASTER_ALL,
		// 	msel_group_ext_val_[emi_no][4] & BM_MASTER_ALL,
		// 	msel_group_ext_val_[emi_no][5] & BM_MASTER_ALL);

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_rw: %d,",emi_no);

		for (i=0;i<WSCT_AMOUNT;i++) {
			if (wsct_rw_val_[emi_no][i] == BM_WSCT_RW_RWBOTH)
				ret += snprintf(buf + ret, buf_len - ret, "RW,");
			else if (wsct_rw_val_[emi_no][i] == BM_WSCT_RW_READONLY)
				ret += snprintf(buf + ret, buf_len - ret, "R,");
			else if (wsct_rw_val_[emi_no][i] == BM_WSCT_RW_WRITEONLY)
				ret += snprintf(buf + ret, buf_len - ret, "W,");
			else    /*disable*/
				ret += snprintf(buf + ret, buf_len - ret, "NONE,");
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_HPRI_DIS: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%d,",WSCT_HPRI_DIS_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;


		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_HPRI_SEL: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",WSCT_HPRI_SEL_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_busid: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_busid_val_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_idMask: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_idMask_val_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_chn_rank_sel: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_chn_rank_sel_val_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_byte_bnd_dis: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%d,",wsct_byte_bnd_dis_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_byte_low_bnd: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_byte_low_bnd_val_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_byte_up_bnd: %d,",emi_no);
		for (i=0;i<WSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_byte_up_bnd_val_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: tsct_busid_enable: %d,",emi_no);
		for (i=0;i<TSCT_AMOUNT;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%d,",tsct_busid_enable_val_[emi_no][i]);
		}
		err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
		if (err < 0) return err;

		/***************************** ttype ****************************************/
		if (ttype17_21_en == BM_TTYPE17_21_ENABLE) {

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: TTYPE_HPRI_SEL: %d,",emi_no);
			for (i=0;i<BM_COUNTER_MAX;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",TTYPE_HPRI_SEL_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_idMask: %d,",emi_no);
			for (i=0;i<BM_COUNTER_MAX;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_idMask_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_chn_rank_sel: %d,",emi_no);
			for (i=0;i<BM_COUNTER_MAX;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_chn_rank_sel_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_byte_bnd_dis: %d,",emi_no);
			for (i=0;i<BM_COUNTER_MAX;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%d,",ttype_byte_bnd_dis_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_byte_low_bnd_val: %d,",emi_no);
			for (i=0;i<BM_COUNTER_MAX;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_byte_low_bnd_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_byte_up_bnd_val: %d,",emi_no);
			for (i=0;i<BM_COUNTER_MAX;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_byte_up_bnd_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;
		}
#endif
		/* ttype enable */
		/* this header will trigger MW ttype rename operation*/
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_enable: %d,%d,%d\n",
										emi_no,ttype1_16_en_[emi_no], ttype17_21_en_[emi_no]);

		if (MET_EMI_support_list & (1<<SLC_PMU_SUPPORT_IDX)) {
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: slc_pmu_cnt_setting_enable: %d,",emi_no);
			for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%d,",slc_pmu_cnt_setting_enable_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: slc_pmu_cnt_filter0: %d,",emi_no);
			for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",slc_pmu_cnt_filter0_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: slc_pmu_cnt_filter1: %d,",emi_no);
			for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",slc_pmu_cnt_filter1_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;

			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: slc_pmu_cnt_bw_lat_sel: %d,",emi_no);
			for (i=0;i<SLC_PMU_CNT_AMOUNT;i++) {
				ret += snprintf(buf + ret, buf_len - ret, "%x,",slc_pmu_cnt_bw_lat_sel_val_[emi_no][i]);
			}
			err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;
		}

	}/* END OF for ( emi_no=0; emi_no<EMI_NUM; emi_no++) */


#if 1 /* SEDA3.5 header print move to AP side */


	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_emi_header: TS0,TS1,GP0_WSCT,GP1_WSCT,GP2_WSCT,GP3_WSCT,GP4_WSCT,GP5_WSCT,");
	ret += snprintf(buf + ret, buf_len - ret,
			"M0_LATENCY,M1_LATENCY,M2_LATENCY,M3_LATENCY,M4_LATENCY,M5_LATENCY,M6_LATENCY,M7_LATENCY,");
	ret += snprintf(buf + ret, buf_len - ret,
			"M0_TRANS,M1_TRANS,M2_TRANS,M3_TRANS,M4_TRANS,M5_TRANS,M6_TRANS,M7_TRANS,");
	ret += snprintf(buf + ret, buf_len - ret,
			"BACT,BSCT,BCNT,WACT,DCM_CTRL,TACT,");

	for (i = 0; i < dram_chann_num; i++) {
		if (i != 0)
			ret += snprintf(buf + ret, buf_len - ret,
					",");
		ret += snprintf(buf + ret, buf_len - ret,
				"PageHit_%d,PageMiss_%d,InterBank_%d,Idle_%d,", i, i, i, i);
		ret += snprintf(buf + ret, buf_len - ret,
				"mr4_%d,refresh_pop_%d,freerun_26m_%d,", i, i, i);
		ret += snprintf(buf + ret, buf_len - ret,
				"read_bytes_%d,write_bytes_%d", i, i);
	}
	ret += snprintf(buf + ret, buf_len - ret, "\n");

	/*TSCT header*/
	if (emi_tsct_enable == 1) {
		ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: ms_emi_tsct_header: ms_emi_tsct,");
		ret += snprintf(buf + ret, buf_len - ret,
				"tsct1,tsct2,tsct3\n");
	}

	/*MDCT header*/
	if (emi_mdct_enable == 1) {
		ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: ms_emi_mdct_header: ms_emi_mdct,");
		ret += snprintf(buf + ret, buf_len - ret,
				"RD_ULTRA,RD_MDMCU\n");
	}


	/* met_bw_rgtor_header */
	ret += snprintf(buf + ret, buf_len - ret,	"met-info [000] 0.0: met_bw_rgtor_unit_header: %x\n", MET_EMI_Get_CONH_2ND(0));

	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_bw_rglr_header: metadata\n");


	/* DRAM DVFS header */
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: DRAM_DVFS_header: datarate(MHz)\n");

	/*PDIR met_dramc_header*/
	if (dramc_pdir_enable == 1 && DRAMC_VER >= 2 ) {
		ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: met_dramc_header: ");
		for (i = 0; i < dram_chann_num; i++) {
			if (i != 0)
				ret += snprintf(buf + ret, buf_len - ret,
						",");
			ret += snprintf(buf + ret, buf_len - ret, "freerun_26m_%d,", i);
			ret += snprintf(buf + ret, buf_len - ret,
					"rk0_pre_sb_%d,rk0_pre_pd_%d,rk0_act_sb_%d,rk0_act_pd_%d,", i, i, i, i);
			ret += snprintf(buf + ret, buf_len - ret,
					"rk1_pre_sb_%d,rk1_pre_pd_%d,rk1_act_sb_%d,rk1_act_pd_%d,", i, i, i, i);
			ret += snprintf(buf + ret, buf_len - ret,
					"rk2_pre_sb_%d,rk2_pre_pd_%d,rk2_act_sb_%d,rk2_act_pd_%d", i, i, i, i);
		}
		ret += snprintf(buf + ret, buf_len - ret, "\n");
	}

	/* DRS header */
	/* phase out @ seda 3.51 because no user */
	// ret += snprintf(buf + ret, buf_len - ret,
	// 		"met-info [000] 0.0: emi_drs_header: ch0_RANK1_GP(%%),ch0_RANK1_SF(%%),ch0_ALL_SF(%%),ch1_RANK1_GP(%%),ch1_RANK1_SF(%%),ch1_ALL_SF(%%)\n");
#endif

#ifdef EMI_LOWEFF_SUPPORT
	/*this info is not open util the EMI DE need*/
	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: chn_emi_loweff_ctl0: ");

	for (emi_no=0;emi_no<EMI_NUM;emi_no++)
		ret += snprintf(buf + ret, buf_len - ret, "%x,",MET_EMI_Get_LOWEFF_CTL0(0,emi_no));

	err = snprintf(buf + ret -1, buf_len - ret + 1, "\n");
			if (err < 0) return err;
#endif
		/*IP version*/
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: EMI_VER: %d.%d\n", EMI_VER_MAJOR, EMI_VER_MINOR);
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: DRAMC_VER: %d\n", DRAMC_VER);
	return ret;
}

static struct metdevice *emi_device;

int met_emi_create_basic(struct kobject *parent, struct metdevice *emi_dev)
{
	int ret = 0;

	_clear_setting();

	ret = MET_BM_Init();
	if (ret != 0) {
		pr_notice("MET_BM_Init failed!!!\n");
		ret = 0;        /* will retry later */
	} else {
		emi_inited = 1;
	}

	kobj_emi = parent;
	emi_device = emi_dev;

PR_BOOTMSG("MET sspm_support_list=%X \n",get_sspm_support_feature());

#define KOBJ_ATTR_ITEM(attr_name) \
	do { \
		ret = sysfs_create_file(kobj_emi, &attr_name ## _attr.attr); \
		if (ret != 0) { \
			pr_notice("Failed to create " #attr_name " in sysfs\n"); \
			return ret; \
		} \
	} while (0)
	KOBJ_ATTR_LIST;
#undef  KOBJ_ATTR_ITEM

	return ret;
}

int do_emi(void)
{
	return emi_device->mode;
}

void met_emi_delete_basic(void)
{
#define	KOBJ_ATTR_ITEM(attr_name) \
	sysfs_remove_file(kobj_emi, &attr_name##_attr.attr)
	if (kobj_emi != NULL) {
		KOBJ_ATTR_LIST;
		kobj_emi = NULL;
	}
#undef	KOBJ_ATTR_ITEM

	if (emi_inited)
		MET_BM_DeInit();

	emi_device = NULL;
}

void met_emi_resume_basic(void)
{
	if (!do_emi())
		return;

	emi_init();
}

int emi_print_header_basic(char *buf, int len)
{
	if( (strlen(header_str) - output_str_len) > PAGE_SIZE ){
		char output_buf[PAGE_SIZE/4];

		strncpy(output_buf, header_str+output_str_len, (PAGE_SIZE/4) -1);
		output_buf[(PAGE_SIZE/4) - 1] = '\0';

		len = snprintf(buf, PAGE_SIZE, "%s", output_buf);
		if (len > 0)
			output_str_len += len;
		emi_device->header_read_again = 1;

		PR_BOOTMSG("EMI header read again!\n");
	}
	else{
		len = snprintf(buf, PAGE_SIZE, "%s\n", header_str+output_str_len);
		if (len < 0) return -1;

		/* reset state */
		emi_device->header_read_again = 0;
		output_header_len = 0;
		output_str_len = 0;
	}
	return len;
}

#ifdef MET_SSPM
void ondiemet_emi_start_basic(void)
{
	int ret;

	emi_use_ondiemet = 1;

	MET_BM_IPI_REGISTER_CB();
	if (!emi_inited) {
		if (MET_BM_Init() != 0) {
			emi_device->mode = 0;
			pr_notice("MET_BM_Init failed!!!\n");
			return;
		}
		emi_inited = 1;
	}
	MET_BM_IPI_configs();

	if (do_emi())
		emi_init();

	ondiemet_module[ONDIEMET_SSPM] |= ID_EMI;
	ret = emi_create_header(header_str, MAX_HEADER_LEN);

	emi_device->header_read_again = 0;
	output_header_len = 0;
	output_str_len = 0;
}

void ondiemet_emi_stop_basic(void)
{
	if (!emi_inited)
		return;

	if (do_emi())
		emi_uninit();
}
#endif

/*para*/
EXPORT_SYMBOL(emi_inited);
EXPORT_SYMBOL(output_str_len);
EXPORT_SYMBOL(dramc_pdir_enable);
EXPORT_SYMBOL(kobj_emi);
EXPORT_SYMBOL(emi_mdct_enable);
EXPORT_SYMBOL(rd_mdmcu_rsv_num);
EXPORT_SYMBOL(met_emi_regdump);
EXPORT_SYMBOL(output_header_len);
EXPORT_SYMBOL(BaseAddrEMI);
EXPORT_SYMBOL(BaseAddrCHN_EMI);
EXPORT_SYMBOL(BaseAddrDRAMC);
EXPORT_SYMBOL(BaseAddrDDRPHY_AO);
EXPORT_SYMBOL(BaseAddrAPMIXEDSYS);
EXPORT_SYMBOL(header_str);
EXPORT_SYMBOL(ttype1_16_en);
EXPORT_SYMBOL(ttype17_21_en);
EXPORT_SYMBOL(emi_tsct_enable);
EXPORT_SYMBOL(emi_use_ondiemet);
EXPORT_SYMBOL(dram_chann_num);
EXPORT_SYMBOL(metemi_func_opt);
EXPORT_SYMBOL(mdmcu_sel_enable);
EXPORT_SYMBOL(BaseAddrSLC_PMU);
/*read from dts*/
EXPORT_SYMBOL(EMI_NUM);
EXPORT_SYMBOL(DRAM_CH_NUM_PER_EMI);
// EXPORT_SYMBOL(DRAM_FREQ_DEFAULT);
// EXPORT_SYMBOL(DDR_RATIO_DEFAULT);
// EXPORT_SYMBOL(DRAM_TYPE_DEFAULT);
EXPORT_SYMBOL(DRAM_TYPE);
EXPORT_SYMBOL(MET_EMI_support_list);
EXPORT_SYMBOL(ddrphy_ao_misc_cg_ctrl0);
EXPORT_SYMBOL(ddrphy_ao_misc_cg_ctrl2);


/*func*/
EXPORT_SYMBOL(emi_init);
#ifdef MET_SSPM
EXPORT_SYMBOL(ondiemet_emi_start_basic);
#endif
EXPORT_SYMBOL(do_emi);
EXPORT_SYMBOL(emi_create_header);
EXPORT_SYMBOL(MET_EMI_Get_BaseClock_Rate);
EXPORT_SYMBOL(emi_uninit);
EXPORT_SYMBOL(met_emi_create_basic);
EXPORT_SYMBOL(met_emi_delete_basic);
EXPORT_SYMBOL(emi_print_header_basic);
EXPORT_SYMBOL(MET_EMI_GetDramChannNum);
EXPORT_SYMBOL(MET_BM_Init);
EXPORT_SYMBOL(check_sspm_support);
EXPORT_SYMBOL(met_get_dram_data_rate);
EXPORT_SYMBOL(MET_GET_DRAM_TYPE);
