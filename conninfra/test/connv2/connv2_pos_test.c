// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "consys_reg_util.h"
#include "connv2_pos_test.h"

#ifdef CONFIG_FPGA_EARLY_PORTING
#define LEGACY_MODE	1
#else
#define LEGACY_MODE	1
#endif

struct pos_check_item {
	unsigned int addr;
	unsigned int mask; /* 0x0 means read full 32 bits */
	unsigned int exp_value;
};

struct pos_check_item mt6985_pos_test[] = {
	{0x1804b364, 0x0, 0x00001000},
	{0x1804b368, 0x0, 0x00001100},
	{0x1804b36c, 0x0, 0x00001c00},
	{0x1804b070, 0x0, 0x04000000},
	{0x1804b074, 0x0, 0x43ffffff},
	{0x18000050, 0x0, 0x00000000},
	{0x18000054, 0x0, 0x00000003},
	{0x180000b0, 0x0, 0x0000000f},
	{0x180000b4, 0x0, 0x00000010},
	{0x18001200, 0xf000, 0x1000},
#ifndef CONFIG_FPGA_EARLY_PORTING
	/* osc en relative config, not setup on FPGA */
#if LEGACY_MODE
	{0x18001340, 0x80, 0x0},
	{0x18001300, 0x00ffffff, 0x00080706},
	{0x18001304, 0x10000, 0x0},
#else
	/* RC MODE */
	{0x18001354, 0x0, 0x02080706},
	{0x18001350, 0x8000, 0x0},
	{0x18001384, 0x0, 0x02080706},
	{0x18001380, 0x8000, 0x0},
	{0x18001348, 0x8, 0x8},
	{0x18001348, 0x1, 0x1},
	{0x18001348, 0x100000, 0x100000}, //0x18001348[20]=1'b1
	{0x18001340, 0x80000000, 0x0}, //0x18001340[31]=1'b0
	{0x18001340, 0x1, 0x0}, //0x18001340[0]=1'b0
	{0x18001300, 0x00ffffff, 0x00080706}, //0x18001300[23:0] = 0x080706
	{0x18001304, 0x10000, 0x0}, //0x18001304[16]=1'b0
#endif /* LEGACY_MODE */
	{0x18000020, 0x40, 0x0}, //0x18000020[6]=1'b0 (with key wdata[31:16] = 0x4254)
	{0x18011100, 0x40000, 0x40000}, //0x18011100[18]=1'b1
	{0x18011100, 0x7ff0, 0x3ff0}, //0x18011100[14:4]=11'd1023
	{0x18011100, 0x8000, 0x8000}, //0x18011100[15]=1'b1
	{0x1800E110, 0x14000, 0x14000},//0x1800_E110[14]=1'b1, 0x1800_E110[16]=1'b1
	{0x1800E104, 0x14000, 0x14000},//0x1800_E104[14]=1'b1, 0x1800_E104[16]=1'b1
#endif /* CONFIG_FPGA_EARLY_PORTING */
	{0x1804B024, 0x79f, 0x111}, //0x1804_B024[10:3]=8'h22, 0x1804_B024[0]=1'b1
	{0x1800E038, 0x79f, 0x189}, //0x1800_E038[10:3]=8'h31, 0x1800_E038[0]=1'b1
	{0x1800E024, 0x79f, 0x499}, //0x1800_E024[10:3]=8'h93, 0x1800_E024[0]=1'b1
	{0x1804D000, 0xffff0000, 0x07f40000}, //0x1804_D000[31:16]=16'h7f4
	{0x1804D000, 0x1c, 0x1c}, //0x1804_D000[2][3][4]=3'b111
#ifndef CONFIG_FPGA_EARLY_PORTING
	{0x18012000, 0x1, 0x1}, //0x1801_2000[0]=1'b1
	{0x18012004, 0x1, 0x1}, //0x1801_2004[0]=1'b1
	{0x18012038, 0xd0, 0x50}, //0x1801_2038[7]=1'b0, [6]=1'b1, [4]=1'b1
	{0x18060380, 0x80000001, 0x80000001},//0x1806_0380[31]=1'b1, [0]=1'b1
#endif
	{0x18012050, 0x1, 0x0},// 0x1801_2050[0]=1'b0
	{0x18001200, 0x41, 0x41}, // 0x1800_1200[6]=1'b1, [0]=1'b1

};

unsigned int _pos_reg_read(unsigned int addr, unsigned int mask)
{
	unsigned int reg;
	void __iomem *vir_addr = NULL;

	vir_addr = ioremap(addr, 0x4);
	if (!vir_addr) {
		pr_notice("[%s] remap 0x%08x failed", __func__, addr);
		return 0x0;
	}

	if (mask == 0x0) {
		reg = CONSYS_REG_READ(vir_addr);
	} else {
		reg = CONSYS_REG_READ_BIT(vir_addr, mask);
	}

	iounmap(vir_addr);
	return reg;
}

int pos_test_6985(void)
{
	int i, reg;
	int len = osal_array_size(mt6985_pos_test);
	int ret = 0;

	pr_info("===== [%s] start =====", __func__);

	for (i = 0; i < len; i++) {
		reg = _pos_reg_read(mt6985_pos_test[i].addr, mt6985_pos_test[i].mask);
		if (reg != mt6985_pos_test[i].exp_value) {
			pr_err("[%s] REG[0x%08x]=[0x%08x] fail, exp:[0x%08x], mask=[0x%08x]",
				__func__, mt6985_pos_test[i].addr, reg, mt6985_pos_test[i].exp_value, mt6985_pos_test[i].mask);
			ret = -1;
		}
	}
	pr_info("===== [%s] end =====", __func__);
	return ret;
}

int conninfra_pos_tc(int par1, int par2, int par3)
{
	int ret = 0;

	pr_info("[%s] chip=0x%x", __func__, par2);
	if (par2 == 0x6985) {
		ret = pos_test_6985();
	}
	return ret;
}
