/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "../gps_dl_hw_priv_util.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#if GPS_DL_ON_LINUX
#include "conninfra.h"
#elif GPS_DL_ON_CTP
#include "conninfra_ext.h"
#endif
#endif
#include "gps_dl_hw_atf.h"
#include "gps_dl_hw_dep_api.h"

#include "gps_dl_hw_semaphore.h"
#if GPS_DL_GET_INFO_FROM_NODE
#include "gps_dl_info_node.h"
#endif

#if GPS_DL_USE_BGF_SEL_SEMA
int gps_dl_hw_give_conn_bgf_sel_hw_sema_atf(unsigned int index, unsigned int master_index)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_GIVE_SEMA_OPID,
			index, master_index, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}
#endif

void gps_dl_hw_dep_gps_control_adie_on_inner_1(bool if_on)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_CONTROL_ADIE_ON_1_OPID,
			if_on, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_gps_control_adie_on_inner_2(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_CONTROL_ADIE_ON_2_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

bool gps_dl_hw_dep_gps_control_adie_on_6878(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int chip_ver;
	enum connsys_clock_schematic clock_sch;
#endif
#if GPS_DL_USE_BGF_SEL_SEMA
	bool take_okay = false;
	int rel_okay = -1;
#endif

	/*mt6686 new pos -- beginning*/
	/*set pinmux for the interface between D-die and A-die*/
	GDL_HW_SET_AP_ENTRY(0x10005468, 0, 0xffffffff, 0x77700000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x77700000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x10005464, 0, 0xffffffff, 0x12200000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x77700000, 0x12200000);

	/*set pinmux driving to 4ma setting*/
	/*set pinmux driving to 4ma setting*/
	GDL_HW_SET_AP_ENTRY(0x11ec0008, 0, 0xffffffff, 0x1c7e00);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11ec0000, 0, 0x1c7e00, 0x0);
	GDL_HW_SET_AP_ENTRY(0x11ec0004, 0, 0xffffffff, 0x41200);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11ec0000, 0, 0x1c7e00, 0x41200);

	/*set pinmux PUPD setting -- PU*/
	GDL_HW_SET_AP_ENTRY(0x11ec0098, 0, 0xffffffff, 0x4000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11ec0090, 0, 0x4000, 0x0);

	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value low before patch download swtich back to CONN mode*/
#if GPS_DL_HAS_CONNINFRA_DRV
	clock_sch = (enum connsys_clock_schematic)conninfra_get_clock_schematic();
	switch (clock_sch) {
	case CONNSYS_CLOCK_SCHEMATIC_26M_COTMS:
	case CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO:
	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value low before patch download swtich back to CONN mode*/
		GDL_HW_SET_AP_ENTRY(0x10005158, 0, 0xffffffff, 0x400000);
		GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005150, 0, 0x400000, 0x0);
		/*GDL_HW_SET_AP_ENTRY(0x10005154, 0, 0xffffffff, 0x0);*/
		/*GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005150, 0, 0x400000, 0x0);*/
		break;
	case CONNSYS_CLOCK_SCHEMATIC_52M_COTMS:
	case CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO:
	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value high before patch download swtich back to CONN mode*/
		GDL_HW_SET_AP_ENTRY(0x10005158, 0, 0xffffffff, 0x400000);
		GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005150, 0, 0x400000, 0x0);
		GDL_HW_SET_AP_ENTRY(0x10005154, 0, 0xffffffff, 0x400000);
		GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005150, 0, 0x400000, 0x400000);
		break;
	default:
		break;
	}
	GDL_LOGW("clk: sch from conninfra = %d", clock_sch);
#endif
	GDL_HW_SET_AP_ENTRY(0x10005058, 0, 0xffffffff, 0x400000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005050, 0, 0x400000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x10005054, 0, 0xffffffff, 0x400000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005050, 0, 0x400000, 0x400000);
	GDL_HW_SET_AP_ENTRY(0x10005468, 0, 0xffffffff, 0x7000000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x7000000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x10005464, 0, 0xffffffff, 0x0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x7000000, 0x0);

#if GPS_DL_USE_BGF_SEL_SEMA
	/*take semaphore before rw 18001010*/
	take_okay = gps_dl_hw_take_conn_bgf_sel_hw_sema(100);
	if (!take_okay) {
		GDL_LOGE("gps_dl_hw_take_conn_bgf_sel_hw_sema fail");
		return false;
	}
#endif
	gps_dl_hw_dep_gps_control_adie_on_inner_1(true);

#if GPS_DL_USE_BGF_SEL_SEMA
	/*mt6878 use index = 0, master = 5 sema*/
	rel_okay = gps_dl_hw_give_conn_bgf_sel_hw_sema_atf(0, 5);
	if (rel_okay == -1)
		GDL_LOGW("gps_dl_hw_give_conn_bgf_sel_hw_sema_atf fail");
#endif

	/*CONN_TOP_DATA switch to CONN mode*/
	GDL_HW_SET_AP_ENTRY(0x10005464, 0, 0xffffffff, 0x2000000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x2000000, 0x2000000);

#if GPS_DL_HAS_CONNINFRA_DRV
	/*read adie chip id with spi1 api*/
	if (conninfra_spi_1_read(SYS_SPI_TOP, 0x02C, &chip_ver) != 0) {
		GDL_LOGD("conninfra_spi_1_read_adie_not_okay");
		goto _fail_conninfra_spi_1_read_adie_not_okay;
	}
	GDL_LOGI("conninfra_spi_1_read_adie success, chip_ver = 0x%08x", chip_ver);
#if GPS_DL_GET_INFO_FROM_NODE
	gps_dl_info_node_set_adie_info(chip_ver);
#endif
	chip_ver = chip_ver & 0xffff0000;
	if (chip_ver == 0x66860000)
		GDL_LOGD("conninfra_spi_1_read_adie_6686 success, chip_ver = 0x%08x", chip_ver);

	/**/
	if (conninfra_spi_1_write(SYS_SPI_TOP, 0x008, 0x00000000) != 0) {
		GDL_LOGI_RRW("_fail_conninfra_spi_1_write_top_data_driving not okay");
		goto _fail_conninfra_spi_1_write_top_data_driving_not_okay;
	}
	/**/
	if (conninfra_spi_1_read(SYS_SPI_TOP, 0x008, &chip_ver) == 0)
		GDL_LOGD("spi_data[0x008] = 0x%x", chip_ver);

	if (conninfra_spi_1_write(SYS_SPI_TOP, 0xB18, 0x00000007) != 0) {
		GDL_LOGI_RRW("conninfra_spi_1_write_atop_rg_top_xo_07 not okay");
		goto _fail_conninfra_spi_1_write_atop_rg_top_xo_07_not_okay;
	 }

	if (conninfra_spi_1_read(SYS_SPI_TOP, 0xB18, &chip_ver) == 0)
		GDL_LOGD("spi_data[0xB18] = 0x%x", chip_ver);
#endif
	return true;

#if GPS_DL_HAS_CONNINFRA_DRV
_fail_conninfra_spi_1_write_atop_rg_top_xo_07_not_okay:
_fail_conninfra_spi_1_write_top_data_driving_not_okay:
_fail_conninfra_spi_1_read_adie_not_okay:
#endif
	return false;

}

void gps_dl_hw_dep_gps_control_adie_off_6878(void)
{
#if GPS_DL_USE_BGF_SEL_SEMA
	bool take_okay = false;
	int rel_okay = -1;
#endif

	/*gps_dl_hw_dep_adie_mt6686_dump_status();*/

#if GPS_DL_USE_BGF_SEL_SEMA
	/*take semaphore before rw 18001010*/
	take_okay = gps_dl_hw_take_conn_bgf_sel_hw_sema(100);
	if (!take_okay) {
		GDL_LOGE("gps_dl_hw_take_conn_bgf_sel_hw_sema fail");
		return;
	}
#endif

	gps_dl_hw_dep_gps_control_adie_on_inner_1(false);

#if GPS_DL_USE_BGF_SEL_SEMA
	/*mt6878 use index = 0, master = 5 sema*/
	rel_okay = gps_dl_hw_give_conn_bgf_sel_hw_sema_atf(0, 5);
	if (rel_okay == -1)
		GDL_LOGW("gps_dl_hw_give_conn_bgf_sel_hw_sema_atf fail");
#endif

	/*mt6686 new pos -- beginning*/
	/*set pinmux for the interface between D-die and A-die*/
	GDL_HW_SET_AP_ENTRY(0x10005468, 0, 0xffffffff, 0x77700000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x77700000, 0x0);

	/*set pinmux PUPD setting -- PD*/
	GDL_HW_SET_AP_ENTRY(0x11ec0098, 0, 0xffffffff, 0x4000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11ec0090, 0, 0x4000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x11ec0094, 0, 0xffffffff, 0x4000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11ec0090, 0, 0x4000, 0x4000);
}

