/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "mtk_wcn_consys_hw.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

int g_mapped_reg_table_sz;
P_REG_MAP_ADDR g_mapped_reg_table;

static struct reg_map_addr *get_mapped_reg(unsigned int mapped_tbl_idx)
{
	struct reg_map_addr *reg_addr = NULL;
	UINT8 *addr = NULL;

	if (mapped_tbl_idx >= g_mapped_reg_table_sz)
		return NULL;
	reg_addr = &g_mapped_reg_table[mapped_tbl_idx];

	if (reg_addr->vir_addr == NULL) {
		addr = ioremap(reg_addr->phy_addr, reg_addr->size);
		if (addr == NULL)
			return NULL;
		WMT_PLAT_PR_INFO("mapidx=[%d] phy[%x] addr=[%p] sz=[%d]",
				mapped_tbl_idx, reg_addr->phy_addr, (void *)addr, reg_addr->size);
		reg_addr->vir_addr = addr;
	}
	return reg_addr;
}

#define TMP_STR_BUF_SZ 64
#define DUMP_STR_BUF_SZ 512
#define DUMP_ACTION_PRINT 0
INT32 execute_dump_action(const char *trg_str, const char *dump_prefix, struct consys_dump_item *dump_ary, int ary_sz)
{
	int i, len, read_idx = 0;
	struct reg_map_addr *reg_addr = NULL;
	struct consys_dump_item *dump_item;
	char buf[TMP_STR_BUF_SZ];
	char str_buf[DUMP_STR_BUF_SZ];
	int remain = DUMP_STR_BUF_SZ;
	const char *p_trg_str = trg_str, *p_dump_prefix = dump_prefix;
	const char *p_undefined_str = "undefined";
	UINT8 *addr = NULL;
	UINT32 mask_tmp = 0;

	if (!dump_ary) {
		WMT_PLAT_PR_WARN("[%s] dump_ary is null", __func__);
		return -1;
	}
	if (p_trg_str == NULL)
		p_trg_str = p_undefined_str;
	if (p_dump_prefix)
		p_dump_prefix = p_undefined_str;

	str_buf[0] = '\0';

	for (i = 0; i < ary_sz; i++) {
		dump_item = &dump_ary[i];

		reg_addr = get_mapped_reg(dump_item->base_addr);
		if (reg_addr == NULL) {
			WMT_PLAT_PR_WARN("[%s] can't get reg_addr [%d]",
								__func__, dump_item->base_addr);
			return -1;
		}

		addr = reg_addr->vir_addr;
		switch (dump_item->action) {
		case DUMP_ACT_WRITE:
			mask_tmp = dump_item->mask;
			if (dump_item->mask == 0x0)
				mask_tmp = 0xffffffff;
			CONSYS_REG_WRITE_MASK(addr + dump_item->offset,
									dump_item->value, mask_tmp);
#if DUMP_ACTION_PRINT
			WMT_PLAT_PR_INFO("[W] addr=[%p] offset=[%x] value=[%x] mask=[%x]",
								(void *)addr, dump_item->offset,
								dump_item->value, dump_item->mask);
#endif
			break;
		case DUMP_ACT_READ:

			len = snprintf(buf, TMP_STR_BUF_SZ, "0x%08X ",
					CONSYS_REG_READ(addr + dump_item->offset));

#if DUMP_ACTION_PRINT
			WMT_PLAT_PR_INFO("[R] addr=[%p] offset=[%x] value=[%s] ", (void *)addr, dump_item->offset,
								buf);
#endif
			if (len > 0) {
				strncat(str_buf, buf, (remain > len ? len : remain-len));
				remain -= len;
			}

			if (read_idx % 8 == 7) {
				WMT_PLAT_PR_INFO("[%s][%s] %s", trg_str, dump_prefix, str_buf);
				str_buf[0] = '\0';
				remain = DUMP_STR_BUF_SZ;
			}
			read_idx++;
			break;

		default:
			break;
		}
	}
	if (strlen(str_buf) > 0)
		WMT_PLAT_PR_INFO("[%s][%s] %s", trg_str, dump_prefix, str_buf);
	WMT_PLAT_PR_INFO("[%s][%s] <<<<<< Count=[%d]", trg_str, dump_prefix, read_idx);
	return 0;
}

VOID init_dump_util_variable(UINT32 chipid)
{
	int table_ary_sz = 0;
	P_REG_MAP_ADDR table_ary = NULL;

	table_ary_sz = mtk_wcn_consys_get_debug_reg_ary_size();
	table_ary = mtk_wcn_consys_get_debug_reg_ary();

	if (table_ary_sz && table_ary) {
		g_mapped_reg_table_sz = table_ary_sz;
		g_mapped_reg_table = table_ary;
	} else
		WMT_PLAT_PR_INFO("chipid(0x%x) not support\n", chipid);
}

VOID mtk_wcn_dump_util_init(UINT32 chipid)
{
	struct reg_map_addr *reg_addr = NULL;
	int i;
	UINT8 *addr = NULL;

	init_dump_util_variable(chipid);

	for (i = 0; i < g_mapped_reg_table_sz; i++) {
		reg_addr = &g_mapped_reg_table[i];
		if (reg_addr->vir_addr == NULL) {
			addr = ioremap(reg_addr->phy_addr, reg_addr->size);
			if (addr == NULL)
				return;
			WMT_PLAT_PR_INFO("mapidx=[%d] phy[%x] addr=[%p] sz=[%d]",
				i, reg_addr->phy_addr, (void *)addr, reg_addr->size);
			reg_addr->vir_addr = addr;
		}
	}
}

VOID mtk_wcn_dump_util_destroy(VOID)
{
	struct reg_map_addr *reg_addr = NULL;
	int i;

	for (i = 0; i < g_mapped_reg_table_sz; i++) {
		reg_addr = &g_mapped_reg_table[i];
		if (reg_addr->vir_addr) {
			iounmap(reg_addr->vir_addr);
			reg_addr->vir_addr = NULL;
		}
	}
}
