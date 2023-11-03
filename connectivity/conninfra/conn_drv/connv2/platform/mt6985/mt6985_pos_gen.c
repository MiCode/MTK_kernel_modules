/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This file, mt6985_pos_gen.c was automatically generated
 * by the tool from the POS data DE provided.
 * It should not be modified by hand.
 *
 * Reference POS file,
 * - Dxxxc_power_on_sequence_20220227.xlsx
 * - Dxxxc_conn_infra_sub_task_220222.xlsx
 * - conn_infra_cmdbt_instr_autogen_20220727.txt
*/


#ifndef CFG_CONNINFRA_ON_CTP
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <connectivity_build_in_adapter.h>
#endif
#include "consys_hw.h"
#include "consys_reg_util.h"
#include "mt6985_consys_reg_offset.h"
#include "mt6985.h"
#include "mt6985_pos.h"
#include "mt6985_pos_gen.h"
#include "conninfra.h"

const unsigned int g_cmdbt_dwn_value_ary_mt6985[1024] = {
	0x16000400, 0x16011805, 0x16100A00, 0x16111805, 0x16200034, 0x16210000, 0xCCCCCCCC, 0x160020A4,
	0x16011801, 0x16100001, 0x16110000, 0x31000100, 0x160004D0, 0x16011805, 0x16100AD0, 0x16111805,
	0x16200022, 0x16210000, 0xCCCCCCCC, 0x250503F0, 0x16107138, 0x16111804, 0x1C200300, 0x16300FFF,
	0x16310000, 0x16400000, 0x16418300, 0x17500203, 0x19600405, 0x31000601, 0x16300007, 0x16310000,
	0x16418200, 0x17500003, 0x19600405, 0x31000601, 0x160020A8, 0x16011801, 0x16100001, 0x16110000,
	0x31000100, 0x1600D000, 0x16011804, 0x16100200, 0x16110000, 0x31000100, 0x1600D000, 0x16011804,
	0x16100200, 0x16110EE9, 0x31000100, 0x1600D000, 0x16011804, 0x16100204, 0x16110EE9, 0x31000100,
	0x1600D000, 0x16011804, 0x1610020C, 0x16110EE9, 0x31000100, 0x1600D000, 0x16011804, 0x1610001C,
	0x16110EE9, 0x31000100, 0x06000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x16000400, 0x16011805, 0x16100A00, 0x16111805, 0x16200034, 0x16210000, 0xBBBBBBBB, 0x160020A4,
	0x16011801, 0x16100001, 0x16110000, 0x31000100, 0x160004D0, 0x16011805, 0x16100AD0, 0x16111805,
	0x16200022, 0x16210000, 0xBBBBBBBB, 0x16007144, 0x16011804, 0x32100000, 0x201503F0, 0x160020A8,
	0x16011801, 0x16100001, 0x16110000, 0x31000100, 0x06000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x18041014, 0x180410F4, 0x1804100C, 0x18041004, 0x18011050, 0x18011100, 0x18011104, 0x18012000,
	0x18012004, 0x18012020, 0x18012050, 0x1804B000, 0x1804B004, 0x1804B008, 0x1804B010, 0x1804B024,
	0x1804B070, 0x1804B074, 0x1804B350, 0x1804B354, 0x1804B358, 0x1804B35C, 0x1804B360, 0x1804B364,
	0x1804B368, 0x1804B36C, 0x1804B370, 0x1804B374, 0x1804B3C8, 0x1804B3CC, 0x1804B3D0, 0x1804B3D4,
	0x1804B3D8, 0x1804B3DC, 0x1804B3E0, 0x1804B3E4, 0x1804B3E8, 0x1804B3EC, 0x18040004, 0x18040008,
	0x1804000C, 0x18040010, 0x18040014, 0x18040018, 0x18040000, 0x1804001C, 0x18040020, 0x18040024,
	0x18040028, 0x18042008, 0x18012030, 0x18012038, 0x18047000, 0x18047004, 0x1804700C, 0x18047010,
	0x18047018, 0x1804701C, 0x18047020, 0x18047024, 0x1804702C, 0x18047030, 0x18047038, 0x1804703C,
	0x18047044, 0x18047048, 0x18047050, 0x18047054, 0x1804705C, 0x18047060, 0x18047068, 0x1804706C,
	0x18047070, 0x18047074, 0x1804707C, 0x18047080, 0x18047084, 0x18047088, 0x18047090, 0x18047094,
	0x18047098, 0x1804709C, 0x180470BC, 0x180470C0, 0x180470C4, 0x180470C8, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};


void consys_set_gpio_tcxo_mode_mt6985_gen(unsigned int tcxo_mode, unsigned int enable)
{
	void __iomem *vir_addr_consys_gen_gpio_base = NULL;

	vir_addr_consys_gen_gpio_base = ioremap(CONSYS_GEN_GPIO_BASE_ADDR, 0x430);

	if (!vir_addr_consys_gen_gpio_base) {
		pr_notice("vir_addr_consys_gen_gpio_base(%x) ioremap fail\n",
			CONSYS_GEN_GPIO_BASE_ADDR);
		return;
	}

	if (enable == 1) {
		/* set GPIO159 pinmux for TCXO mode (Aux3) */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			if (tcxo_mode == 1) {
				CONSYS_REG_WRITE_MASK(vir_addr_consys_gen_gpio_base +
					CONSYS_GEN_GPIO_MODE19_OFFSET_ADDR, 0x30000000, 0x70000000);
			}
		#endif
	} else {
		/* set GPIO159 pinmux for TCXO mode (Aux0) */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			if (tcxo_mode == 1) {
				CONSYS_REG_WRITE_MASK(vir_addr_consys_gen_gpio_base +
					CONSYS_GEN_GPIO_MODE19_OFFSET_ADDR, 0x0, 0x70000000);
			}
		#endif
	}

	if (vir_addr_consys_gen_gpio_base)
		iounmap(vir_addr_consys_gen_gpio_base);
}

int consys_conninfra_on_power_ctrl_mt6985_gen(unsigned int enable)
{
	int check = 0;

	if (SPM_BASE_ADDR == 0) {
		pr_notice("SPM_BASE_ADDR is not defined\n");
		return -1;
	}

	if (enable == 1) {
		/* turn on SPM clock */
		/* (apply this for SPM's CONNSYS power control related CR accessing) */
		CONSYS_REG_WRITE_MASK(SPM_BASE_ADDR +
			CONSYS_GEN_POWERON_CONFIG_EN_OFFSET_ADDR, 0xB160001, 0xFFFF0001);

		/* assert "conn_infra_on" primary part power on, set "connsys_on_domain_pwr_on"=1 */
		CONSYS_SET_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 2));

		/* check "conn_infra_on" primary part power status, */
		/* check "connsys_on_domain_pwr_ack"=1 */
		/* (polling "10 times" and each polling interval is "0.5ms") */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			check = 0;
			CONSYS_REG_BIT_POLLING(SPM_BASE_ADDR +
				CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR,
				30, 1, 10, 500, check);
			if (check != 0) {
				pr_notice("check conn_infra_on primary part power status fail, Status=0x%08x\n",
					CONSYS_REG_READ(SPM_BASE_ADDR +
						CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR));
			}
		#endif

		/* assert "conn_infra_on" secondary part power on, */
		/* set "connsys_on_domain_pwr_on_s"=1 */
		CONSYS_SET_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 3));

		/* check "conn_infra_on" secondary part power status, */
		/* check "connsys_on_domain_pwr_ack_s"=1 */
		/* (polling "10 times" and each polling interval is "0.5ms") */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			check = 0;
			CONSYS_REG_BIT_POLLING(SPM_BASE_ADDR +
				CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR,
				31, 1, 10, 500, check);
			if (check != 0) {
				pr_notice("check conn_infra_on secondary part power status fail, Status=0x%08x\n",
					CONSYS_REG_READ(SPM_BASE_ADDR +
						CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR));
			}
		#endif

		/* turn on AP-to-CONNSYS bus clock, */
		/* set "conn_clk_dis"=0 */
		/* (apply this for bus clock toggling) */
		CONSYS_CLR_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 4));

		udelay(1);

		/* de-assert "conn_infra_on" isolation, set "connsys_iso_en"=0 */
		CONSYS_CLR_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 1));

		/* de-assert CONNSYS S/W reset (SPM CR), set "ap_sw_rst_b"=1 */
		CONSYS_SET_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 0));

		udelay(500);

		/* Turn off AP2CONN AHB RX bus sleep protect */
		/* (apply this for INFRA AXI bus accessing when CONNSYS had been turned on) */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985  +
			CONSYS_GEN_SLEEP_PROT_EN_W1C_14_OFFSET_ADDR, (0x1U << 0));

		/* check  AP2CONN AHB RX bus sleep protect turn off */
		/* (polling "100 times" and each polling interval is "0.5ms") */
		/* If AP2CONN (TX/RX) protect turn off fail, power on fail. */
		/* (DRV access connsys CR will get 0 ) */
		check = 0;
		CONSYS_REG_BIT_POLLING(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR,
			0, 0, 100, 500, check);
		if (check != 0) {
			pr_notice("check  AP2CONN AHB RX bus sleep protect turn off fail, Status=0x%08x\n",
				CONSYS_REG_READ(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
					CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR));
		}

		/* Turn off AP2CONN AHB TX bus sleep protect */
		/* (apply this for INFRA AXI bus accessing when CONNSYS had been turned on) */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_EN_W1C_0_OFFSET_ADDR, (0x1U << 25));

		/* check  AP2CONN AHB TX bus sleep protect turn off */
		/* (polling "100 times" and each polling interval is "0.5ms") */
		/* If AP2CONN (TX/RX) protect turn off fail, power on fail. */
		/* (DRV access connsys CR will get 0 ) */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			check = 0;
			CONSYS_REG_BIT_POLLING(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
				CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR,
				25, 0, 100, 500, check);
			if (check != 0) {
				pr_notice("check  AP2CONN AHB TX bus sleep protect turn off fail, Status=0x%08x\n",
					CONSYS_REG_READ(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
						CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR));
			}
		#endif

		/* Turn off CONN2AP AXI RX bus sleep protect */
		/* (disable sleep protection when CONNSYS had been turned on) */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_EN_W1C_0_OFFSET_ADDR, (0x1U << 26));

		/* Turn off CONN2AP AXI TX bus sleep protect */
		/* (disable sleep protection when CONNSYS had been turned on) */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_EN_W1C_14_OFFSET_ADDR, (0x1U << 1));

		mdelay(5);
	} else {
		/* Turn on AP2CONN AHB TX bus sleep protect */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_EN_W1S_0_OFFSET_ADDR, (0x1U << 25));

		/* check  AP2CONN AHB TX bus sleep protect turn on */
		/* (polling "100 times" and each polling interval is "0.5ms") */
		/* If AP2CONN (TX/RX) protect turn off fail, power on fail. */
		/* (DRV access connsys CR will get 0 ) */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			check = 0;
			CONSYS_REG_BIT_POLLING(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
				CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR,
				25, 1, 100, 500, check);
			if (check != 0) {
				pr_notice("check  AP2CONN AHB TX bus sleep protect turn on fail, Status=0x%08x\n",
					CONSYS_REG_READ(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
						CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR));
			}
		#endif

		/* Turn on AP2CONN AHB RX bus sleep protect */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_EN_W1S_14_OFFSET_ADDR, (0x1U << 0));

		/* check  AP2CONN AHB RX bus sleep protect turn on */
		/* (polling "100 times" and each polling interval is "0.5ms") */
		/* If AP2CONN (TX/RX) protect turn off fail, power on fail. */
		/* (DRV access connsys CR will get 0 ) */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			check = 0;
			CONSYS_REG_BIT_POLLING(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
				CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR,
				0, 1, 100, 500, check);
			if (check != 0) {
				pr_notice("check  AP2CONN AHB RX bus sleep protect turn on fail, Status=0x%08x\n",
					CONSYS_REG_READ(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
						CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR));
			}
		#endif

		/* Turn on CONN2AP AXI TX bus sleep protect */
		/* (disable sleep protection when CONNSYS had been turned on) */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_EN_W1S_14_OFFSET_ADDR, (0x1U << 1));

		/* check  CONN2AP AXI TX bus sleep protect turn on */
		/* (polling "100 times" and each polling interval is "0.5ms") */
		/* If CONN2AP (TX/RX) protect turn off fail, power on fail. */
		/* (DRV access connsys CR will get 0 ) */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			check = 0;
			CONSYS_REG_BIT_POLLING(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
				CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR,
				1, 1, 100, 500, check);
			if (check != 0) {
				pr_notice("check  CONN2AP AXI TX bus sleep protect turn on fail, Status=0x%08x\n",
					CONSYS_REG_READ(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
						CONSYS_GEN_SLEEP_PROT_RDY_14_OFFSET_ADDR));
			}
		#endif

		/* Turn on CONN2AP AXI RX bus sleep protect */
		/* (disable sleep protection when CONNSYS had been turned on) */
		CONSYS_SET_BIT(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
			CONSYS_GEN_SLEEP_PROT_EN_W1S_0_OFFSET_ADDR, (0x1U << 26));

		/* check  CONN2AP AXI RX bus sleep protect turn on */
		/* (polling "100 times" and each polling interval is "0.5ms") */
		/* If CONN2AP (TX/RX) protect turn off fail, power on fail. */
		/* (DRV access connsys CR will get 0 ) */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			check = 0;
			CONSYS_REG_BIT_POLLING(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
				CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR,
				26, 1, 100, 500, check);
			if (check != 0) {
				pr_notice("check  CONN2AP AXI RX bus sleep protect turn on fail, Status=0x%08x\n",
					CONSYS_REG_READ(INFRBUS_AO_REG_BASE_ADDR_MT6985 +
						CONSYS_GEN_SLEEP_PROT_RDY_0_OFFSET_ADDR));
			}
		#endif

		/* assert "conn_infra_on" isolation, set "connsys_iso_en"=1 */
		CONSYS_SET_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 1));

		/* assert CONNSYS S/W reset (SPM CR), set "ap_sw_rst_b"=0 */
		CONSYS_CLR_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 0));

		/* turn off AP-to-CONNSYS bus clock, */
		/* set "conn_clk_dis"=1 */
		/* (apply this for bus clock gating) */
		CONSYS_SET_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 4));

		udelay(1);

		/* de-assert "conn_infra_on" primary part power on, */
		/* set "connsys_on_domain_pwr_on"=0 */
		CONSYS_CLR_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 2));

		/* de-assert "conn_infra_on" secondary part power on, */
		/* set "connsys_on_domain_pwr_on_s"=0 */
		CONSYS_CLR_BIT(SPM_BASE_ADDR +
			CONSYS_GEN_CONN_PWR_CON_OFFSET_ADDR, (0x1U << 3));
	}

	return 0;
}

int consys_polling_chipid_mt6985_gen(unsigned int *pconsys_ver_id)
{
	int check = 0;
	int retry = 0;
	unsigned int consys_ver_id = 0;
	unsigned int bus_status;

	if (CONN_CFG_BASE == 0) {
		pr_notice("CONN_CFG_BASE is not defined\n");
		return -1;
	}

	/* check CONN_INFRA IP Version */
	/* (polling "10 times" for specific project code and each polling interval is "1ms") */
	retry = 11;
	while (retry-- > 0) {
		bus_status = consys_is_bus_hang_mt6985();
		pr_info("[%s] bus status=%d\n", __func__, bus_status);

		consys_ver_id = CONSYS_REG_READ(
			CONN_CFG_BASE +
			CONSYS_GEN_IP_VERSION_OFFSET_ADDR);
		if (consys_ver_id == CONSYS_GEN_CONN_HW_VER) {
			check = 0;
			pr_info("Consys HW version id(0x%08x), retry(%d)\n", consys_ver_id, retry);
			if (pconsys_ver_id != NULL)
				*pconsys_ver_id = consys_ver_id;
			break;
		}
		check = -1;
		usleep_range(1000, 2000);
	}

	if (check != 0) {
		pr_notice("Read CONSYS version id fail. Expect 0x%08x but get 0x%08x\n",
			CONSYS_GEN_CONN_HW_VER, consys_ver_id);
		#if defined(KERNEL_clk_buf_show_status_info)
			KERNEL_clk_buf_show_status_info();  /* dump clock buffer */
		#endif
		return -1;
	}

	return 0;
}

unsigned int consys_emi_set_remapping_reg_mt6985_gen(
		phys_addr_t con_emi_base_addr,
		phys_addr_t md_shared_emi_base_addr,
		phys_addr_t gps_emi_base_addr,
		unsigned int emi_base_addr_offset)
{
	if (CONN_HOST_CSR_TOP_BASE == 0) {
		pr_notice("CONN_HOST_CSR_TOP_BASE is not defined\n");
		return -1;
	}

	if (CONN_BUS_CR_BASE == 0) {
		pr_notice("CONN_BUS_CR_BASE is not defined\n");
		return -1;
	}

	/* driver should set the following configuration */
	if (con_emi_base_addr) {
		CONSYS_REG_WRITE_OFFSET_RANGE(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_CONN_HOST_CSR_TOP_18060354_OFFSET,
			con_emi_base_addr, 0, emi_base_addr_offset, 20);
	}

	pr_info("connsys_emi_base=[0x%llx] remap cr: connsys=[0x%08x]\n",
		con_emi_base_addr,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_CONN_HOST_CSR_TOP_18060354_OFFSET));

	if (md_shared_emi_base_addr) {
		CONSYS_REG_WRITE_OFFSET_RANGE(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_CONN_HOST_CSR_TOP_1806035C_OFFSET,
			md_shared_emi_base_addr, 0, emi_base_addr_offset, 20);
	}

	pr_info("mcif_emi_base=[0x%llx] remap cr: mcif=[0x%08x]\n",
		md_shared_emi_base_addr,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_CONN_HOST_CSR_TOP_1806035C_OFFSET));

	if (gps_emi_base_addr) {
		CONSYS_REG_WRITE_OFFSET_RANGE(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_CONN_HOST_CSR_TOP_18060360_OFFSET,
			gps_emi_base_addr, 0, emi_base_addr_offset, 20);
	}

	pr_info("gps_emi_base=[0x%llx] remap cr: gps=[0x%08x]\n",
		gps_emi_base_addr,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_CONN_HOST_CSR_TOP_18060360_OFFSET));

	CONSYS_REG_WRITE_MASK(CONN_BUS_CR_BASE +
		CONSYS_GEN_INFRA_BUS_1804B364_OFFSET, 0x1000, 0xFFFFF);
	CONSYS_REG_WRITE_MASK(CONN_BUS_CR_BASE +
		CONSYS_GEN_INFRA_BUS_1804B368_OFFSET, 0x1100, 0xFFFFF);
	CONSYS_REG_WRITE_MASK(CONN_BUS_CR_BASE +
		CONSYS_GEN_INFRA_BUS_1804B36C_OFFSET, 0x1C00, 0xFFFFF);

	return 0;
}

void consys_init_conninfra_sysram_mt6985_gen(void)
{
	/* initial conn_infra_sysram value */
	if (CONN_INFRA_SYSRAM_SIZE == 0) {
		pr_notice("CONN_INFRA_SYSRAM_SIZE is 0.\n");
		return;
	}

	#ifdef CFG_CONNINFRA_ON_CTP
		memset_io(CONSYS_GEN_CONN_INFRA_SYSRAM_OFFSET_ADDR, 0x0, CONN_INFRA_SYSRAM_SIZE);
	#else
		memset_io((volatile void*)CONN_INFRA_SYSRAM_BASE, 0x0, CONN_INFRA_SYSRAM_SIZE);
	#endif
}

void connsys_get_d_die_efuse_mt6985_gen(unsigned int *p_d_die_efuse)
{
	if (CONN_CFG_BASE == 0) {
		pr_notice("CONN_CFG_BASE is not defined\n");
		return;
	}

	/* read D-die Efuse */
	if (p_d_die_efuse != NULL) {
		*p_d_die_efuse = CONSYS_REG_READ(CONN_CFG_BASE +
			CONSYS_GEN_EFUSE_OFFSET_ADDR);
	}
}

int connsys_d_die_cfg_mt6985_gen(void)
{
	if (CONN_BUS_CR_BASE == 0) {
		pr_notice("CONN_BUS_CR_BASE is not defined\n");
		return -1;
	}
	/* set infra top emi address range */
	CONSYS_REG_WRITE(CONN_BUS_CR_BASE +
		CONSYS_GEN_INFRA_BUS_1804B070_OFFSET, 0x4000000);
	CONSYS_REG_WRITE(CONN_BUS_CR_BASE +
		CONSYS_GEN_INFRA_BUS_1804B074_OFFSET, 0x43FFFFFF);

	if (CONN_RGU_ON_BASE == 0) {
		pr_notice("CONN_RGU_ON_BASE is not defined\n");
		return -1;
	}

	/* conn_infra sysram hw control setting -> disable hw power dowm */
	CONSYS_REG_WRITE(CONN_RGU_ON_BASE +
		CONSYS_GEN_SYSRAM_HWCTL_PDN_OFFSET_ADDR, 0x0);

	/* conn_infra sysram hw control setting -> enable hw sleep */
	CONSYS_REG_WRITE(CONN_RGU_ON_BASE +
		CONSYS_GEN_SYSRAM_HWCTL_SLP_OFFSET_ADDR, 0x3);

	/* conn_mawd memory  hw control setting -> disable hw power dowm */
	CONSYS_REG_WRITE(CONN_RGU_ON_BASE +
		CONSYS_GEN_MAWD_MEM_HWCTL_PDN_OFFSET_ADDR, 0xF);

	/* conn_mawd memory  hw control setting -> enable hw sleep */
	CONSYS_REG_WRITE(CONN_RGU_ON_BASE +
		CONSYS_GEN_MAWD_MEM_HWCTL_SLP_OFFSET_ADDR, 0x10);

	return 0;
}

void connsys_wt_slp_top_ctrl_adie6686_mt6985_gen(void)
{
	if (CONN_WT_SLP_CTL_REG_BASE == 0) {
		pr_notice("CONN_WT_SLP_CTL_REG_BASE is not defined\n");
		return;
	}

	/* wt_slp CR for A-die ck_en/wake_en control (ref. A-die low power control) */
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_BASE +
		CONSYS_GEN_WB_TOP_CK_ADDR_OFFSET_ADDR, 0xA00, 0xFFF);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_BASE +
		CONSYS_GEN_WB_GPS_CK_ADDR_OFFSET_ADDR, 0xAFC0A0C, 0xFFF0FFF);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_BASE +
		CONSYS_GEN_WB_GPS_RFBUF_ADR_OFFSET_ADDR, 0xFC, 0xFFF);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_BASE +
		CONSYS_GEN_WB_GPS_L5_EN_ADDR_OFFSET_ADDR, 0xF8, 0xFFF);
}

int connsys_afe_wbg_cal_mt6985_gen(
		unsigned int spi_semaphore_index,
		unsigned int spi_semaphore_timeout_usec)
{
	/* empty content for table afe_wbg_cal */

	return 0;
}

int connsys_subsys_pll_initial_xtal_26000k_mt6985_gen(void)
{
	if (CONN_AFE_CTL_BASE == 0) {
		pr_notice("CONN_AFE_CTL_BASE is not defined\n");
		return -1;
	}

	/* CASE SYS_XTAL_26000K */
	CONSYS_REG_WRITE_MASK(CONN_AFE_CTL_BASE +
		CONSYS_GEN_RG_PLL_STB_TIME_OFFSET_ADDR, 0x3140521, 0x7FFF7FFF);
	CONSYS_REG_WRITE_MASK(CONN_AFE_CTL_BASE +
		CONSYS_GEN_RG_DIG_EN_02_OFFSET_ADDR, 0x30047, 0x300CF);
	CONSYS_REG_WRITE_MASK(CONN_AFE_CTL_BASE +
		CONSYS_GEN_RG_DIG_TOP_01_OFFSET_ADDR, 0x68000, 0x78000);

	return 0;
}

int connsys_low_power_setting_mt6985_gen(void)
{
	int i = 0;
	unsigned int addr_offset = 0;

	if (CONN_CFG_ON_BASE == 0) {
		pr_notice("CONN_CFG_ON_BASE is not defined\n");
		return -1;
	}

	if (CONN_RGU_ON_BASE == 0) {
		pr_notice("CONN_RGU_ON_BASE is not defined\n");
		return -1;
	}

	if (CONN_CFG_BASE == 0) {
		pr_notice("CONN_CFG_BASE is not defined\n");
		return -1;
	}

	if (CONN_BUS_CR_ON_BASE == 0) {
		pr_notice("CONN_BUS_CR_ON_BASE is not defined\n");
		return -1;
	}

	if (CONN_BUS_CR_BASE == 0) {
		pr_notice("CONN_BUS_CR_BASE is not defined\n");
		return -1;
	}

	if (CONN_OFF_DEBUG_CTRL_AO_BASE == 0) {
		pr_notice("CONN_OFF_DEBUG_CTRL_AO_BASE is not defined\n");
		return -1;
	}

	if (CONN_CLKGEN_TOP_BASE == 0) {
		pr_notice("CONN_CLKGEN_TOP_BASE is not defined\n");
		return -1;
	}

	if (CONN_HOST_CSR_TOP_BASE == 0) {
		pr_notice("CONN_HOST_CSR_TOP_BASE is not defined\n");
		return -1;
	}

	/* Unmask off2on slpprot_rdy enable checker @conn_infra off power off=> check slpprot_rdy = 1'b1 and go to sleep */
	CONSYS_REG_WRITE_MASK(CONN_CFG_ON_BASE +
		CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL0_OFFSET_ADDR, 0x1000, 0xF000);

	if (!consys_is_rc_mode_enable_mt6985()) {
		/* disable conn_top rc osc_ctrl_top */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_CLR_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR, (0x1U << 7));
		#endif

		/* Legacy OSC control stable time */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_REG_WRITE_MASK(CONN_CFG_ON_BASE +
				CONSYS_GEN_OSC_CTL_0_OFFSET_ADDR, 0x80706, 0xFFFFFF);
		#endif

		/* Legacy OSC control unmask conn_srcclkena_ack */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_CLR_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_OSC_CTL_1_OFFSET_ADDR, (0x1U << 16));
		#endif
	} else {
		/* GPS RC OSC control stable time */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_REG_WRITE(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_GPS_OFFSET_ADDR, 0x2080706);
		#endif

		/* GPS RC OSC control unmask conn_srcclkena_ack */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_CLR_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_GPS_OFFSET_ADDR, (0x1U << 15));
		#endif

		/* TOP RC OSC control stable time */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_REG_WRITE(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_TOP_OFFSET_ADDR, 0x2080706);
		#endif

		/* TOP RC OSC control unmask conn_srcclkena_ack */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_CLR_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_TOP_OFFSET_ADDR, (0x1U << 15));
		#endif

		/* enable conn_infra rc osc_ctl_top output */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_SET_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_OFFSET_ADDR, (0x1U << 3));
		#endif

		udelay(30);

		/* enable conn_top rc osc_ctrl_gps output */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_SET_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_OFFSET_ADDR, (0x1U << 0));
		#endif

		/* enable conn_top rc osc_ctrl_gps */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_SET_BIT(CONN_CFG_ON_BASE +
					CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR, (0x1U << 4));
		#endif

		/* set conn_srcclkena control by conn_infra_emi_ctl */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_SET_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_1_OFFSET_ADDR, (0x1U << 20));
		#endif

		/* disable legacy osc control output */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_CLR_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR, (0x1U << 31));
		#endif

		/* disable legacy osc control */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_CLR_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_RC_CTL_0_OFFSET_ADDR, (0x1U << 0));
		#endif

		/* Legacy OSC control stable time */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_REG_WRITE_MASK(CONN_CFG_ON_BASE +
				CONSYS_GEN_OSC_CTL_0_OFFSET_ADDR, 0x80706, 0xFFFFFF);
		#endif

		/* Legacy OSC control unmask conn_srcclkena_ack */
		#ifndef CONFIG_FPGA_EARLY_PORTING
			CONSYS_CLR_BIT(CONN_CFG_ON_BASE +
				CONSYS_GEN_OSC_CTL_1_OFFSET_ADDR, (0x1U << 16));
		#endif
	}

	/* prevent subsys from power on/off in a short time interval */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_REG_WRITE_MASK(CONN_RGU_ON_BASE +
			CONSYS_GEN_BGFYS_ON_TOP_PWR_CTL_OFFSET_ADDR, 0x42540000, 0xFFFF0040);
	#endif

	/* conn2ap sleep protect release bypass ddr_en_ack check */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_CFG_BASE +
			CONSYS_GEN_EMI_CTL_0_OFFSET_ADDR, (0x1U << 18));
	#endif

	/* enable ddr_en timeout, timeout value = 1023 T (Bus clock) */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_REG_WRITE_MASK(CONN_CFG_BASE +
			CONSYS_GEN_EMI_CTL_0_OFFSET_ADDR, 0x3FF0, 0x7FF0);
	#endif

	/* update ddr_en timeout value enable */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_CFG_BASE +
			CONSYS_GEN_EMI_CTL_0_OFFSET_ADDR, (0x1U << 15));
	#endif

	/* conn_infra off domain bus dcm mode setting to off mode */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_BUS_CR_ON_BASE +
			CONSYS_GEN_CONN_OFF_BUS_DCM_CTL_1_OFFSET_ADDR, (0x1U << 14));
	#endif

	/* conn_infra off domain bus dcm enable */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_BUS_CR_ON_BASE +
			CONSYS_GEN_CONN_OFF_BUS_DCM_CTL_1_OFFSET_ADDR, (0x1U << 16));
	#endif

	/* conn_infra von domain bus dcm mode setting to off mode */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_BUS_CR_ON_BASE +
			CONSYS_GEN_CONN_VON_BUS_DCM_CTL_1_OFFSET_ADDR, (0x1U << 14));
	#endif

	/* conn_infra von domain bus dcm enable */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_BUS_CR_ON_BASE +
			CONSYS_GEN_CONN_VON_BUS_DCM_CTL_1_OFFSET_ADDR, (0x1U << 16));
	#endif

	/* bus access protector: prevent gps mcu to access conninfra cmdbt
	 * CONN_BUS_CR_M3_LIGHT_SECURITY_START_ADDR_2_R_M3_LIGHT_SECURITY_START_ADDR_2 = 0x18050
	 * CONN_BUS_CR_M3_LIGHT_SECURITY_END_ADDR_2_R_M3_LIGHT_SECURITY_END_ADDR_2 = 0x18050
	 * CONN_BUS_CR_LIGHT_SECURITY_CTRL_R_CONN_INFRA_GPS_PAIR2_EN = 1
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_BUS_CR_M3_LIGHT_SECURITY_START_ADDR_2_R_M3_LIGHT_SECURITY_START_ADDR_2,
		0x18050);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_BUS_CR_M3_LIGHT_SECURITY_END_ADDR_2_R_M3_LIGHT_SECURITY_END_ADDR_2,
		0x18050);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_BUS_CR_LIGHT_SECURITY_CTRL_R_CONN_INFRA_GPS_PAIR2_EN, 1);

	/* set conn_infra_off bus apb/ahb/axi layer timeout - step 1 set timing */
	/* CONN_INFRA_BUS_CR_CONN_INFRA_OFF_BUS_TIMEOUT_CTRL_TIMEOUT_TIME_SETTING
	 * 0x1804_B024[10:3]	8'h40
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_BUS_CR_CONN_INFRA_OFF_BUS_TIMEOUT_CTRL_TIMEOUT_TIME_SETTING, 0x40);

	/* set conn_infra_off bus apb/ahb/axi layer timeout - step 2 enable function */
	CONSYS_SET_BIT(CONN_BUS_CR_BASE +
		CONSYS_GEN_INFRA_BUS_1804B024_OFFSET, (0x1U << 0));

	/* set conn_infra_on bus apb timeout - step 1 set timing */
	CONSYS_REG_WRITE_MASK(CONN_BUS_CR_ON_BASE +
		CONSYS_GEN_INFRA_BUS_ON_1800E038_OFFSET, 0x188, 0x7F8);

	/* set conn_infra_on bus apb timeout - step 2 enable function */
	CONSYS_SET_BIT(CONN_BUS_CR_ON_BASE +
		CONSYS_GEN_INFRA_BUS_ON_1800E038_OFFSET, (0x1U << 0));

	/* set conn_von_top bus apb timeout - step 1 set timing */
	CONSYS_REG_WRITE_MASK(CONN_BUS_CR_ON_BASE +
		CONSYS_GEN_INFRA_BUS_ON_1800E024_OFFSET, 0x498, 0x7F8);

	/* set conn_von_top bus apb timeout - step 2 enable function */
	CONSYS_SET_BIT(CONN_BUS_CR_ON_BASE +
		CONSYS_GEN_INFRA_BUS_ON_1800E024_OFFSET, (0x1U << 0));

	/* enable conn_infra off bus tool auto gen timeout feature */
	CONSYS_SET_BIT(CONN_OFF_DEBUG_CTRL_AO_BASE +
		CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET, (0x1U << 9));

	/* 0x1804_D000[31:16] = 16'h0FDE (159ms)
	 */
	CONSYS_REG_WRITE_MASK(CONN_OFF_DEBUG_CTRL_AO_BASE +
		CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET, 0x0FDE0000, 0xFFFF0000);

	/* write 0x1804d000[2] = 0x4 */
	CONSYS_SET_BIT(CONN_OFF_DEBUG_CTRL_AO_BASE +
		CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET, (0x1U << 2));

	/* write 0x1804d000[3] = 0x8 */
	CONSYS_SET_BIT(CONN_OFF_DEBUG_CTRL_AO_BASE +
		CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET, (0x1U << 3));

	/* write 0x1804d000[4] = 0x10 */
	CONSYS_SET_BIT(CONN_OFF_DEBUG_CTRL_AO_BASE +
		CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET, (0x1U << 4));

	/* write 0x1804d000[9] = 0x0 */
	CONSYS_CLR_BIT(CONN_OFF_DEBUG_CTRL_AO_BASE +
		CONSYS_GEN_INFRA_OFF_DCTRL_AO_1804D000_OFFSET, (0x1U << 9));

	/* enable conn_infra bus bpll div_2 */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_CLKGEN_TOP_BASE +
			CONSYS_GEN_CKGEN_BUS_BPLL_DIV_2_OFFSET_ADDR, (0x1U << 0));
	#endif

	/* set rfspi pll to bpll div 13 */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_CLR_BIT(CONN_CLKGEN_TOP_BASE +
			CONSYS_GEN_CLKGEN_RFSPI_CK_CTRL_OFFSET_ADDR, (0x1U << 7));
	#endif

	/* set rfsp bpll div 13 en */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_CLKGEN_TOP_BASE +
			CONSYS_GEN_CLKGEN_RFSPI_CK_CTRL_OFFSET_ADDR, (0x1U << 6));
	#endif

	/* set rfspi pll to bpll */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_CLKGEN_TOP_BASE +
			CONSYS_GEN_CLKGEN_RFSPI_CK_CTRL_OFFSET_ADDR, (0x1U << 4));
	#endif

	/* set conn_infra sleep count to host side control */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_HOST_CONN_INFRA_SLP_CNT_CTL_OFFSET_ADDR, (0x1U << 31));
	#endif

	/* set conn_infra sleep count enable (host side) */
	#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_SET_BIT(CONN_HOST_CSR_TOP_BASE +
			CONSYS_GEN_HOST_CONN_INFRA_SLP_CNT_CTL_OFFSET_ADDR, (0x1U << 0));
	#endif

	/* disable conn_infra bus clock sw control  ==> conn_infra bus clock hw control */
	CONSYS_CLR_BIT(CONN_CLKGEN_TOP_BASE +
		CONSYS_GEN_CKGEN_BUS_OFFSET_ADDR, (0x1U << 0));

	/* conn_infra cmdbt code download */
	CONSYS_REG_WRITE(CONN_CFG_BASE +
		CONSYS_GEN_CMDBT_FETCH_START_ADDR0_OFFSET_ADDR, 0x18050200);

	addr_offset = 0x0;
	for (i = 0; i < 1024; i++) {
		CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_BASE +
			addr_offset, g_cmdbt_dwn_value_ary_mt6985[i]);
		addr_offset += 0x4;
	}

	/* conn_infra wakeup need restore first */
	CONSYS_SET_BIT(CONN_CFG_ON_BASE +
		CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL0_OFFSET_ADDR, (0x1U << 6));

	/* Conn_infra HW_CONTROL => conn_infra enter dsleep mode */
	CONSYS_SET_BIT(CONN_CFG_ON_BASE +
		CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL0_OFFSET_ADDR, (0x1U << 0));

	return 0;
}

int consys_conninfra_wakeup_mt6985_gen(void)
{
	int check = 0;
	unsigned int cmdbt_done;
	unsigned int dump1, dump2;

	if (CONN_HOST_CSR_TOP_BASE == 0) {
		pr_notice("CONN_HOST_CSR_TOP_BASE is not defined\n");
		return -1;
	}

	if (CONN_CFG_ON_BASE == 0) {
		pr_notice("CONN_CFG_ON_BASE is not defined\n");
		return -1;
	}

	/* wake up conn_infra */
	CONSYS_REG_WRITE(CONN_HOST_CSR_TOP_BASE +
		CONSYS_GEN_CONN_INFRA_WAKEPU_TOP_OFFSET_ADDR, 0x1);

	udelay(200);

	/* check CONN_INFRA IP versionn */
	/* (polling "10 times" for specific project code and each polling interval is "1ms") */
	if (consys_polling_chipid_mt6985_gen(NULL))
		return -1;

	check = consys_is_bus_hang_mt6985();
	pr_info("[%s] bus status=%d\n", __func__, check);

	/* check CONN_INFRA cmdbt restore done */
	/* (polling "10 times" for specific project code and each polling interval is "0.5ms") */
	/* 0x1800_1210[16]	1'b1	polling */
	check = 0;
	while (check < 10) {
		cmdbt_done = CONSYS_REG_READ(
				CONN_CFG_ON_BASE + CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL1_OFFSET_ADDR);
		if ((cmdbt_done & (0x1U << 16)) != 0) {
			pr_info("[%s] conninfra cmdbt restore done: 0x%08x", __func__, cmdbt_done);
			return 0;
		} else {
			/* Debug dump
			 * wirte 0x1806_015c[2:0] =3'b001
			 * read 0x1806_0a04
			 * read 0x1800_1344
			 * read 0x1800_1210
			 */
			CONSYS_REG_WRITE_HW_ENTRY(
				CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL,
				0x1);
			dump1 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR);
			dump2 = CONSYS_REG_READ(CONN_CFG_ON_CONN_INFRA_CFG_RC_STATUS_ADDR);
			pr_info("[%s][%d] check cmdbt restore done: [0x%08x][0x%08x][0x%08x]",
				__func__, check, dump1, dump2, cmdbt_done);
		}
		check++;
	}
#if 0
	CONSYS_REG_BIT_POLLING(CONN_CFG_ON_BASE +
		CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL1_OFFSET_ADDR,
		16, 1, 10, 500, check);
	if (check != 0) {
		pr_notice("check CONN_INFRA cmdbt restore done fail, Status=0x%08x\n",
			CONSYS_REG_READ(CONN_CFG_ON_BASE +
				CONSYS_GEN_CONN_INFRA_CFG_PWRCTRL1_OFFSET_ADDR));
	}
#endif
	pr_notice("[%s] conninfra cmdbt restore FAIL: 0x%08x", __func__, cmdbt_done);
	return -1;
}

int consys_conninfra_sleep_mt6985_gen(void)
{
	if (CONN_HOST_CSR_TOP_BASE == 0) {
		pr_notice("CONN_HOST_CSR_TOP_BASE is not defined\n");
		return -1;
	}

	/* release conn_infra force on */
	CONSYS_REG_WRITE(CONN_HOST_CSR_TOP_BASE +
		CONSYS_GEN_CONN_INFRA_WAKEPU_TOP_OFFSET_ADDR, 0x0);

	return 0;
}
