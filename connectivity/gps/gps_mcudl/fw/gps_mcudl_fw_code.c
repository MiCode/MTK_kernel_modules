/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_fw_code.h"
#include "gps_dl_context.h"

#if 0
#include "plat/v060/fw/inc/mcu_rom_code.h"
#include "plat/v060/fw/inc/mcu_emi_code.h"
const struct gps_mcudl_fw_desc c_gps_mcudl_v060_fw_desc_list[] = {
	{MCU_FW_ROM, &bgf_minirom_bin[0], bgf_minirom_bin_len},
	{MCU_FW_EMI, &gnss_ram_bin[0], gnss_ram_bin_len},
};
#else
const unsigned char g_gps_mcudl_v060_mcu_rom_code[] = {
	#include "mcu_rom_code.h"
};
#define GPS_MCUDL_V060_MCU_ROM_CODE_SIZE \
	(sizeof(g_gps_mcudl_v060_mcu_rom_code))

#if 0
const unsigned char g_gps_mcudl_v060_mcu_emi_code[] = {
	#include "plat/v060/fw/inc/mcu_emi_code.h"
};
#define GPS_MCUDL_V060_MCU_EMI_CODE_SIZE \
	(sizeof(g_gps_mcudl_v060_mcu_emi_code))

const struct gps_mcudl_fw_desc c_gps_mcudl_v060_fw_desc_list[] = {
	{MCU_FW_ROM, &g_gps_mcudl_v060_mcu_rom_code[0], GPS_MCUDL_V060_MCU_ROM_CODE_SIZE},
	{MCU_FW_EMI, &g_gps_mcudl_v060_mcu_emi_code[0], GPS_MCUDL_V060_MCU_EMI_CODE_SIZE},
};
#endif
#endif

#if 0
const struct gps_mcudl_fw_list c_gps_mcudl_v060_fw_list = {
	&c_gps_mcudl_v060_fw_desc_list[0],
	ARRAY_SIZE(c_gps_mcudl_v060_fw_desc_list)
};
#endif

const unsigned char g_gps_mcudl_dummy_mcu_rom_code[] = {
	0x13, 0x00, 0x00, 0x00,
	0x13, 0x00, 0x00, 0x00,
	0x13, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};
#define GPS_MCUDL_DUMMY_MCU_ROM_CODE_SIZE \
	(sizeof(g_gps_mcudl_dummy_mcu_rom_code))

const unsigned char g_gps_mcudl_dummy_mcu_emi_code[] = {
	0x00, 0x00, 0x00, 0x00,
};
#define GPS_MCUDL_DUMMY_MCU_EMI_CODE_SIZE \
	(sizeof(g_gps_mcudl_dummy_mcu_emi_code))

const struct gps_mcudl_fw_desc c_gps_mcudl_dummy_fw_desc_list[] = {
	{MCU_FW_ROM, &g_gps_mcudl_dummy_mcu_rom_code[0], GPS_MCUDL_DUMMY_MCU_ROM_CODE_SIZE},
	{MCU_FW_EMI, &g_gps_mcudl_dummy_mcu_emi_code[0], GPS_MCUDL_DUMMY_MCU_EMI_CODE_SIZE},
};

const struct gps_mcudl_fw_list c_gps_mcudl_dummy_fw_list = {
	&c_gps_mcudl_dummy_fw_desc_list[0],
	ARRAY_SIZE(c_gps_mcudl_dummy_fw_desc_list)
};

const struct gps_mcudl_fw_desc c_gps_mcudl_rom_only_fw_desc_list[] = {
	{MCU_FW_ROM, &g_gps_mcudl_v060_mcu_rom_code[0], 0}, /* GPS_MCUDL_V060_MCU_ROM_CODE_SIZE */
	{MCU_FW_EMI, &g_gps_mcudl_dummy_mcu_emi_code[0], 0},
};

const struct gps_mcudl_fw_list c_gps_mcudl_rom_only_fw_list = {
	&c_gps_mcudl_rom_only_fw_desc_list[0],
	ARRAY_SIZE(c_gps_mcudl_rom_only_fw_desc_list)
};

