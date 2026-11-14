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
#include "gps_dl_hw_atf.h"
#include "gps_dl_hw_dep_api.h"

bool gps_dl_hw_dep_gps_control_adie_on_6993(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int chip_ver;
	enum connsys_clock_schematic clock_sch;
#endif

	/*mt6686 new pos -- beginning*/
	/*set pinmux for the interface between D-die and A-die*/
	GDL_HW_SET_AP_ENTRY(0x1002d6a8, 0, 0xffffffff, 0x000f0f0f);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x000f0f0f, 0x0);
	GDL_HW_SET_AP_ENTRY(0x1002d6a4, 0, 0xffffffff, 0x00010101);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x000f0f0f, 0x00010101);

	/*set pinmux driving to 4ma setting*/
	GDL_HW_SET_AP_ENTRY(0x13820018, 0, 0xffffffff, 0x00000ff8);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x13820010, 0, 0x00000ff8, 0x0);
	GDL_HW_SET_AP_ENTRY(0x13820014, 0, 0xffffffff, 0x00000248);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x13820010, 0, 0x00000ff8, 0x00000248);

	/*set pinmux PUPD setting*/
	GDL_HW_SET_AP_ENTRY(0x138200b8, 0, 0xffffffff, 0x00002000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200b0, 0, 0x00002000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x138200b4, 0, 0xffffffff, 0x0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200b0, 0, 0x00002000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x138200c8, 0, 0xffffffff, 0x00002000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200c0, 0, 0x00002000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x138200c4, 0, 0xffffffff, 0x00002000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200c0, 0, 0x00002000, 0x00002000);

	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value low before patch download swtich back to CONN mode*/
#if GPS_DL_HAS_CONNINFRA_DRV
	clock_sch = (enum connsys_clock_schematic)conninfra_get_clock_schematic();
	switch (clock_sch) {
	case CONNSYS_CLOCK_SCHEMATIC_26M_COTMS:
	case CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO:
	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value low before patch download swtich back to CONN mode*/
	GDL_HW_SET_AP_ENTRY(0x1002d178, 0, 0xffffffff, 0x00000200);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d170, 0, 0x00000200, 0x0);
	GDL_HW_SET_AP_ENTRY(0x1002d174, 0, 0xffffffff, 0x0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d170, 0, 0x00000200, 0x0);
		break;
	case CONNSYS_CLOCK_SCHEMATIC_52M_COTMS:
	case CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO:
	/*CONN_TOP_DATA swtich to GPIO mode, GPIO output value high before patch download swtich back to CONN mode*/
	GDL_HW_SET_AP_ENTRY(0x1002d178, 0, 0xffffffff, 0x00000200);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d170, 0, 0x00000200, 0x0);
	GDL_HW_SET_AP_ENTRY(0x1002d174, 0, 0xffffffff, 0x00000200);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d170, 0, 0x00000200, 0x00000200);
		break;
	default:
		break;
	}
	GDL_LOGW("clk: sch from conninfra = %d", clock_sch);
#endif
	GDL_HW_SET_AP_ENTRY(0x1002d078, 0, 0xffffffff, 0x00000200);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d070, 0, 0x00000200, 0x0);
	GDL_HW_SET_AP_ENTRY(0x1002d074, 0, 0xffffffff, 0x00000200);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d170, 0, 0x00000200, 0x00000200);
	GDL_HW_SET_AP_ENTRY(0x1002d6a8, 0, 0xffffffff, 0x00000f00);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x00000f00, 0x0);
	GDL_HW_SET_AP_ENTRY(0x1002d6a4, 0, 0xffffffff, 0x0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x00000f00, 0x0);

	/*de-assert A-sie reset*/
	GDL_HW_SET_AP_ENTRY(0x40001010, 0, 0x1, 0x1);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x40001010, 0, 0x1, 0x1);

	/*CONN_TOP_DATA switch to CONN mode*/
	GDL_HW_SET_AP_ENTRY(0x1002d6a8, 0, 0xffffffff, 0x00000f00);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x00000f00, 0x0);
	GDL_HW_SET_AP_ENTRY(0x1002d6a4, 0, 0xffffffff, 0x00000100);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x00000100, 0x00000100);

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

void gps_dl_hw_dep_gps_control_adie_off_6993(void)
{
	gps_dl_hw_dep_adie_mt6686_dump_status();

	/*disable A-die top_clk_en_5*/
	GDL_HW_ADIE_TOP_CLK_EN_6686(0x0);

	/*de-assert A-sie reset*/
	GDL_HW_SET_AP_ENTRY(0x40001010, 0, 0x1, 0x0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x40001010, 0, 0x1, 0x0);

	/*mt6686 new pos -- beginning*/
	/*set pinmux for the interface between D-die and A-die*/
	GDL_HW_SET_AP_ENTRY(0x1002d6a8, 0, 0xffffffff, 0x000f0f0f);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x000f0f0f, 0x0);
	GDL_HW_SET_AP_ENTRY(0x1002d6a4, 0, 0xffffffff, 0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x1002d6a0, 0, 0x000f0f0f, 0);

	/*set pinmux PUPD setting*/
	GDL_HW_SET_AP_ENTRY(0x138200b8, 0, 0xffffffff, 0x00002000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200b0, 0, 0x00002000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x138200b4, 0, 0xffffffff, 0x00002000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200b0, 0, 0x00002000, 0x00002000);
	GDL_HW_SET_AP_ENTRY(0x138200c8, 0, 0xffffffff, 0x00002000);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200c0, 0, 0x00002000, 0x0);
	GDL_HW_SET_AP_ENTRY(0x138200c4, 0, 0xffffffff, 0);
	GDL_HW_SET_AP_ENTRY_TO_CHECK(0x138200c0, 0, 0x00002000, 0);
}

