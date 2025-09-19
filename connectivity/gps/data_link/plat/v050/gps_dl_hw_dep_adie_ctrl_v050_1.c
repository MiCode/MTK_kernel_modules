/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "../gps_dl_hw_priv_util.h"
#include "gps_dl_hw_dep_macro.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#if GPS_DL_ON_LINUX
#include "conninfra.h"
#elif GPS_DL_ON_CTP
#include "conninfra_ext.h"
#endif
#endif
#include "gps_dl_hw_dep_api.h"
#if GPS_DL_USE_BGF_SEL_SEMA
#include "gps_dl_hw_semaphore.h"
#endif
#if GPS_DL_GET_INFO_FROM_NODE
#include "gps_dl_info_node.h"
#endif

bool gps_dl_hw_dep_gps_control_adie_on_6878(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int chip_ver;
	enum connsys_clock_schematic clock_sch;
#endif
	bool okay = false;

	/*mt6686 new pos -- beginning*/
	/*set pinmux for the interface between D-die and A-die*/
	GDL_HW_SET_AP_ENTRY(0x10005468, 0, 0xffffffff, 0x77700000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x77700000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x10005464, 0, 0xffffffff, 0x12200000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x77700000, 0x12200000);

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
	okay = gps_dl_hw_take_conn_bgf_sel_hw_sema(100);
	if (!okay) {
		GDL_LOGE("gps_dl_hw_take_conn_bgf_sel_hw_sema fail");
		return false;
	}
#endif
	/*de-assert A-sie reset*/
	GDL_HW_SET_AP_ENTRY(0x18001010, 2, 0x4, 0x1);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x18001010, 2, 0x4, 0x1);
#if GPS_DL_USE_BGF_SEL_SEMA
	/*relase semaphore after rw 18001010*/
	gps_dl_hw_give_conn_bgf_sel_hw_sema();
#endif

	/*CONN_TOP_DATA switch to CONN mode*/
	GDL_HW_SET_AP_ENTRY(0x10005464, 0, 0xffffffff, 0x2000000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005460, 0, 0x2000000, 0x2000000);

#if GPS_DL_USE_BGF_SEL_SEMA
	/*take semaphore before rw 18001010*/
	okay = gps_dl_hw_take_conn_bgf_sel_hw_sema(100);
	if (!okay) {
		GDL_LOGE("gps_dl_hw_take_conn_bgf_sel_hw_sema fail");
		return false;
	}
#endif
	/*set to spi1 for 6686*/
	GDL_HW_SET_AP_ENTRY(0x18001010, 5, 0x20, 0x1);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x18001010, 5, 0x20, 0x1);
#if GPS_DL_USE_BGF_SEL_SEMA
	/*relase semaphore after rw 18001010*/
	gps_dl_hw_give_conn_bgf_sel_hw_sema();
#endif

	/*enable A-die top_clk_en_5*/
	GDL_HW_ADIE_TOP_CLK2_EN_6686(0x1);

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
	bool okay = false;

	/*gps_dl_hw_dep_adie_mt6686_dump_status();*/

	/*disable A-die top_clk_en_5*/
	GDL_HW_ADIE_TOP_CLK2_EN_6686(0x0);
#if GPS_DL_USE_BGF_SEL_SEMA
	/*take semaphore before rw 18001010*/
	okay = gps_dl_hw_take_conn_bgf_sel_hw_sema(100);
	if (!okay) {
		GDL_LOGE("gps_dl_hw_take_conn_bgf_sel_hw_sema fail");
		return;
	}
#endif
	/*de-assert A-sie reset*/
	GDL_HW_SET_AP_ENTRY(0x18001010, 2, 0x4, 0x0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x18001010, 2, 0x4, 0x0);
#if GPS_DL_USE_BGF_SEL_SEMA
	/*relase semaphore after rw 18001010*/
	gps_dl_hw_give_conn_bgf_sel_hw_sema();
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

