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

bool gps_dl_hw_dep_gps_control_adie_on_6989(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int chip_ver;
	enum connsys_clock_schematic clock_sch;
#endif

	/*mt6686 new pos -- beginning*/
	/*set pinmux for the interface between D-die and A-die*/
	GDL_HW_SET_AP_ENTRY(0x10005408, 0, 0xffffffff, 0x07770000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005400, 0, 0x77700000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x10005404, 0, 0xffffffff, 0x01110000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005400, 0, 0x77700000, 0x01110000);

	/*set pinmux driving to 4ma setting*/
	GDL_HW_SET_AP_ENTRY(0x11c30000, 12, 0x7000, 0x1);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11c30000, 12, 0x7000, 0x1);

	/*set pinmux PUPD setting*/
	GDL_HW_SET_AP_ENTRY(0x11c30088, 0, 0xffffffff, 0x100);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11c30080, 0, 0x100, 0x0);
	GDL_HW_SET_AP_ENTRY(0x11c30094, 0, 0xffffffff, 0x100);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11c30090, 0, 0x100, 0x100);

	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value low before patch download swtich back to CONN mode*/
#if GPS_DL_HAS_CONNINFRA_DRV
	clock_sch = (enum connsys_clock_schematic)conninfra_get_clock_schematic();
	switch (clock_sch) {
	case CONNSYS_CLOCK_SCHEMATIC_26M_COTMS:
	case CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO:
	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value low before patch download swtich back to CONN mode*/
		break;
	case CONNSYS_CLOCK_SCHEMATIC_52M_COTMS:
	case CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO:
	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value high before patch download swtich back to CONN mode*/
		break;
	default:
		break;
	}
	GDL_LOGW("clk: sch from conninfra = %d", clock_sch);
#endif
	GDL_HW_SET_AP_ENTRY(0x10005044, 0, 0xffffffff, 0x00000020);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005040, 0, 0x00000020, 0x00000020);
	GDL_HW_SET_AP_ENTRY(0x10005148, 0, 0xffffffff, 0x00000020);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005140, 0, 0x00000020, 0x0);
	GDL_HW_SET_AP_ENTRY(0x10005408, 0, 0xffffffff, 0x00700000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005400, 0, 0x00700000, 0x0);

	/*de-assert A-sie reset*/
	GDL_HW_SET_AP_ENTRY(0x18001010, 0, 0x1, 0x1);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x18001010, 0, 0x1, 0x1);

	/*CONN_TOP_DATA switch to CONN mode*/
	GDL_HW_SET_AP_ENTRY(0x10005404, 0, 0xffffffff, 0x00100000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005400, 0, 0x00100000, 0x00100000);

	/*enable A-die top_clk_en_5*/
	GDL_HW_ADIE_TOP_CLK_EN_6686(0x1);

#if GPS_DL_HAS_CONNINFRA_DRV
	/*read adie chip id*/
	if (conninfra_spi_read(SYS_SPI_TOP, 0x02C, &chip_ver) != 0) {
		GDL_LOGD("conninfra_spi_read_adie_not_okay");
		goto _fail_conninfra_spi_read_adie_not_okay;
	}
	GDL_LOGI("conninfra_spi_read_adie success, chip_ver = 0x%08x", chip_ver);
	chip_ver = chip_ver & 0xffff0000;
	if (chip_ver == 0x66860000)
		GDL_LOGD("conninfra_spi_read_adie_6686 success, chip_ver = 0x%08x", chip_ver);

	/**/
	if (conninfra_spi_write(SYS_SPI_TOP, 0x008, 0x00000000) != 0) {
		GDL_LOGI_RRW("_fail_conninfra_spi_write_top_data_driving not okay");
		goto _fail_conninfra_spi_write_top_data_driving_not_okay;
	}
	/**/
	if (conninfra_spi_read(SYS_SPI_TOP, 0x008, &chip_ver) == 0)
		GDL_LOGD("spi_data[0x008] = 0x%x", chip_ver);

	if (conninfra_spi_write(SYS_SPI_TOP, 0xB18, 0x00000007) != 0) {
		GDL_LOGI_RRW("conninfra_spi_write_atop_rg_top_xo_07 not okay");
		goto _fail_conninfra_spi_write_atop_rg_top_xo_07_not_okay;
	}

	if (conninfra_spi_read(SYS_SPI_TOP, 0xB18, &chip_ver) == 0)
		GDL_LOGD("spi_data[0xB18] = 0x%x", chip_ver);
#endif
	return true;

#if GPS_DL_HAS_CONNINFRA_DRV
_fail_conninfra_spi_write_atop_rg_top_xo_07_not_okay:
_fail_conninfra_spi_write_top_data_driving_not_okay:
_fail_conninfra_spi_read_adie_not_okay:
#endif
	return false;

}

void gps_dl_hw_dep_gps_control_adie_off_6989(void)
{
	gps_dl_hw_dep_adie_mt6686_dump_status();

	/*disable A-die top_clk_en_5*/
	GDL_HW_ADIE_TOP_CLK_EN_6686(0x0);

	/*de-assert A-sie reset*/
	GDL_HW_SET_AP_ENTRY(0x18001010, 0, 0x1, 0x0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x18001010, 0, 0x1, 0x0);

	/*mt6686 new pos -- beginning*/
	/*set pinmux for the interface between D-die and A-die*/
	GDL_HW_SET_AP_ENTRY(0x10005408, 0, 0xffffffff, 0x07770000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005400, 0, 0x07770000, 0x0);

	/*set pinmux PUPD setting*/
	GDL_HW_SET_AP_ENTRY(0x10005048, 0, 0xffffffff, 0x00000020);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x10005040, 0, 0x00000020, 0x0);
	GDL_HW_SET_AP_ENTRY(0x11c30098, 0, 0xffffffff, 0x100);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11c30090, 0, 0x100, 0x0);
	GDL_HW_SET_AP_ENTRY(0x11c30084, 0, 0xffffffff, 0x100);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x11c30080, 0, 0x100, 0x100);
}

