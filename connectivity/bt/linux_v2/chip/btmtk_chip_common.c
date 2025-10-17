/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_chip_common.h"

char efuse_r_cmd[READ_EFUSE_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0E,
			0x01, 0x0D, 0x0A, 0x00, 0x02, 0x04,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00};/*4 sub block number(sub block 0~3)*/

char efuse_r_evt[READ_EFUSE_EVT_HDR_LEN] = {0x04, 0xE4, 0x1E, 0x02, 0x0D, 0x1A, 0x00, 02, 04};

char hwerr_evt[HWERR_EVT_LEN] = {0x04, 0x10, 0x01, 0xff};

char hci_reset_cmd[HCI_RESET_CMD_LEN] = {0x01, 0x03, 0x0C, 0x00};
char hci_reset_evt[HCI_RESET_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x0c, 0x00};

char stp0_cmd[SET_STP_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x09, 0x01, 0x04, 0x05, 0x00, 0x03, 0x11, 0x0E, 0x00, 0x00};
char stp0_evt[SET_STP_EVT_LEN] = {0x04, 0xE4, 0x06, 0x02, 0x04, 0x02, 0x00, 0x00, 0x03};

char stp1_cmd[SET_STP1_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0C,
	0x01, 0x08, 0x08, 0x00, 0x02, 0x01, 0x00, 0x01, 0x08, 0x00, 0x00, 0x80};
char stp1_evt[SET_STP1_EVT_LEN] = {0x04, 0xE4, 0x10, 0x02, 0x08,
	0x0C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x80, 0x63, 0x76, 0x00, 0x00};

char wmt_power_on_cmd[WMT_POWER_ON_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01};
char wmt_power_on_evt[WMT_POWER_ON_EVT_HDR_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00};	/* event[7] is key */

char wmt_power_off_cmd[WMT_POWER_OFF_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x00};
char wmt_power_off_evt[WMT_POWER_OFF_EVT_HDR_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00};

char phase1_wmt_evt[EVT_HDR_LEN] = {0x04, 0xE4};
char vendor_evt[EVT_HDR_LEN] = {0x04, 0x0E};

char picus_enable_cmd[PICUS_ENABLE_CMD_LEN] = {0x01, 0x5D, 0xFC, 0x04, 0x00, 0x00, 0x02, 0x02};
char picus_enable_event[PICUS_ENABLE_EVT_HDR_LEN] = {0x04, 0x0E, 0x08, 0x01, 0x5D, 0xFC, 0x00, 0x00, 0x00};

char picus_disable_cmd[PICUS_DISABLE_CMD_LEN] = {0x01, 0x5D, 0xFC, 0x04, 0x00, 0x00, 0x02, 0x00};
char picus_disable_evt[PICUS_DISABLE_EVT_HDR_LEN] = {0x04, 0x0E, 0x08, 0x01, 0x5D, 0xFC, 0x00, 0x00, 0x00};

char fd5b_assert_cmd[FD5B_ASSERT_CMD_LEN] = {0x01, 0x5B, 0xFD, 0x00};
char rhw_assert_cmd[RHW_ASSERT_CMD_LEN] = {0x40, 0x00, 0x00, 0x08, 0x00, 0x18, 0x89, 0x02, 0x81, 0x10, 0x00, 0x00, 0x00};
char wmt_assert_cmd[WMT_ASSERT_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08};

char txpower_cmd[TXPOWER_CMD_LEN] = {0x01, 0x2C, 0xFC, 0x0C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char txpower_evt[TXPOWER_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x2C, 0xFC, 0x00};

char audio_slave_cmd[AUDIO_SETTING_CMD_LEN] = {0x01, 0x72, 0xFC, 0x04, 0x49, 0x00, 0x80, 0x00};
char audio_slave_evt[AUDIO_SETTING_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x72, 0xFC, 0x00};

char read_pinmux_cmd[READ_PINMUX_CMD_LEN] = {0x01, 0xD1, 0xFC, 0x04, 0x50, 0x50, 0x00, 0x70};
char read_pinmux_evt[READ_PINMUX_EVT_CMP_LEN] = {0x04, 0x0E, 0x08, 0x01, 0xD1, 0xFC};
char write_pinmux_cmd[WRITE_PINMUX_CMD_LEN] = {0x01, 0xD0, 0xFC, 0x08, 0x50, 0x50, 0x00, 0x70,
						0x00, 0x10, 0x11, 0x01};
char write_pinmux_evt[WRITE_PINMUX_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0xD0, 0xFC, 0x00};

char read_address_cmd[READ_ADDRESS_CMD_LEN] = {0x01, 0x09, 0x10, 0x00};
char read_address_evt[READ_ADDRESS_EVT_HDR_LEN] = {0x04, 0x0E, 0x0A, 0x01, 0x09, 0x10, 0x00, /* AA, BB, CC, DD, EE, FF */};

char res_apcf_cmd[RES_APCF_CMD_LEN] = {0x01, 0xC9, 0xFC, 0x05, 0x01, 0x30, 0x02, 0x61, 0x02};
char res_apcf_evt[RES_APCF_EVT_LEN] = {0x04, 0xE6, 0x02, 0x08, 0x11};

char woble_enable_default_cmd[WOBLE_ENABLE_DEFAULT_CMD_LEN] = {0x01, 0xC9, 0xFC, 0x24, 0x01, 0x20, 0x02, 0x00, 0x01,
	0x02, 0x01, 0x00, 0x05, 0x10, 0x00, 0x00, 0x40, 0x06,
	0x02, 0x40, 0x0A, 0x02, 0x41, 0x0F, 0x05, 0x24, 0x20,
	0x04, 0x32, 0x00, 0x09, 0x26, 0xC0, 0x12, 0x00, 0x00,
	0x12, 0x00, 0x00, 0x00};
char woble_enable_default_evt[WOBLE_ENABLE_DEFAULT_EVT_LEN] = {0x04, 0xE6, 0x02, 0x08, 0x00};

char woble_disable_default_cmd[WOBLE_DISABLE_DEFAULT_CMD_LEN] = {0x01, 0xC9, 0xFC, 0x05, 0x01, 0x21, 0x02, 0x00, 0x00};
char woble_disable_default_evt[WOBLE_DISABLE_DEFAULT_EVT_LEN] = {0x04, 0xE6, 0x02, 0x08, 0x01};

char radio_off_cmd[RADIO_OFF_CMD_LEN] = {0x01, 0xC9, 0xFC, 0x05, 0x01, 0x20, 0x02, 0x00, 0x00};
char radio_off_evt[RADIO_OFF_EVT_LEN] = {0x04, 0xE6, 0x02, 0x08, 0x00};

char radio_on_cmd[RADIO_ON_CMD_LEN] = {0x01, 0xC9, 0xFC, 0x05, 0x01, 0x21, 0x02, 0x00, 0x00};
char radio_on_evt[RADIO_ON_EVT_LEN] = {0x04, 0xE6, 0x02, 0x08, 0x01};

char apcf_filter_cmd[APCF_FILTER_CMD_LEN] = {0x01, 0x57, 0xFD, 0x0A,
	0x01, 0x00, 0x0A, 0x20, 0x00, 0x20, 0x00, 0x01, 0x80, 0x00};
char apcf_filter_evt[APCF_FILTER_EVT_HDR_LEN] = {0x04, 0x0E, 0x07,
	0x01, 0x57, 0xFD, 0x00, 0x01/*, 00, 63*/};

char apcf_cmd[APCF_CMD_LEN] = {0x01, 0x57, 0xFD, 0x27, 0x06, 0x00, 0x0A,
	0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x43, 0x52, 0x4B, 0x54, 0x4D,
	0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
char apcf_evt[APCF_EVT_HDR_LEN] = {0x04, 0x0E, 0x07, 0x01, 0x57, 0xFD, 0x00, /* 0x06 00 63 */};

char apcf_delete_cmd[APCF_DELETE_CMD_LEN] = {0x01, 0x57, 0xFD, 0x03, 0x01, 0x01, 0x0A};
char apcf_delete_evt[APCF_DELETE_EVT_HDR_LEN] = {0x04, 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, 0x01, /* 00, 63 */};

char apcf_resume_evt[APCF_RESUME_EVT_HDR_LEN] = {0x04, 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00};

char check_wobx_debug_cmd[CHECK_WOBX_DEBUG_CMD_LEN] = {0X01, 0xCE, 0xFC, 0x04, 0xFF, 0xFF, 0xFF, 0xFF};
char check_wobx_debug_evt[CHECK_WOBX_DEBUG_EVT_HDR_LEN] = {0x04, 0xE8};

char notify_alt_evt[NOTIFY_ALT_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x0c, 0x00};

char ld_patch_cmd[LD_PATCH_CMD_LEN] = {0x02, 0x6F, 0xFC, 0x05, 0x00, 0x01, 0x01, 0x01, 0x00, PATCH_PHASE3};
char ld_patch_evt[LD_PATCH_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; /* event[7] is status*/

char ld_patch_cmd_usb[LD_PATCH_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x05, 0x01, 0x01, 0x01, 0x00, PATCH_PHASE3};

char read_register_cmd[READ_REGISTER_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0C,
			0x01, 0x08, 0x08, 0x00,
			0x02, 0x01, 0x00, 0x01,
			0x00, 0x00, 0x00, 0x00};

char read_register_evt[READ_REGISTER_EVT_HDR_LEN] = {0x04, 0xE4, 0x10, 0x02,
		0x08, 0x0C, 0x00, 0x00,
		0x00, 0x00, 0x01};

char write_register_cmd[WRITE_REGISTER_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x14,
		0x01, 0x08, 0x10, 0x00,
		0x01, 0x01, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF};
char write_register_evt[WRITE_REGISTER_EVT_HDR_LEN] = {0x04, 0xE4, 0x08,
		0x02, 0x08, 0x04, 0x00,
		0x00, 0x00, 0x00, 0x01};

char set_baud_cmd_115200[SETBAUD_CMD_LEN_CONNAC2] = {0x01, 0x6F, 0xFC, 0x09,
	0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0xC2, 0x01, 0x00};
char set_baud_cmd_230400[SETBAUD_CMD_LEN_CONNAC2] = {0x01, 0x6F, 0xFC, 0x09,
	0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x84, 0x03, 0x00};
char set_baud_cmd_921600[SETBAUD_CMD_LEN_CONNAC2] = {0x01, 0x6F, 0xFC, 0x09,
	0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x10, 0x0E, 0x00};
char set_baud_cmd_2M[SETBAUD_CMD_LEN_CONNAC2] = {0x01, 0x6F, 0xFC, 0x09,
	0x01, 0x04, 0x05, 0x00, 0x01, 0x80, 0x84, 0x1E, 0x00};
char set_baud_cmd_3M[SETBAUD_CMD_LEN_CONNAC2] = {0x01, 0x6F, 0xFC, 0x09,
	0x01, 0x04, 0x05, 0x00, 0x01, 0xC0, 0xC6, 0x2D, 0x00};
char set_baud_cmd_4M[SETBAUD_CMD_LEN_CONNAC2] = {0x01, 0x6F, 0xFC, 0x09,
	0x01, 0x04, 0x05, 0x00, 0x01, 0x00, 0x09, 0x3D, 0x00};

char set_baud_evt[SETBAUD_EVT_LEN] = {0x04, 0xE4, 0x06, 0x02, 0x04, 0x02, 0x00, 0x00, 0x01};

char get_baud_cmd[GETBAUD_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x05, 0x01, 0x04, 0x01, 0x00, 0x02};
char get_baud_evt[GETBAUD_EVT_LEN] = {0x04, 0xE4, 0x0a, 0x02, 0x04, 0x06, 0x00, 0x00, 0x02};

char uart_wakeup_cmd[WAKEUP_CMD_LEN] = {0x01, 0x6f, 0xfc, 0x01, 0xFF};
char uart_wakeup_evt[WAKEUP_EVT_LEN] = {0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x03};

char uart_fw_own_cmd[FWOWN_TEST_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x01};
char uart_fw_own_evt[OWNTYPE_TEST_EVT_LEN] = {0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x01};

char uart_driver_own_cmd[DRVOWN_TEST_CMD_LEN] = {0xFF};
char uart_driver_own_evt[OWNTYPE_TEST_EVT_LEN] = {0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x03};

char set_address_cmd[SET_ADDRESS_CMD_LEN] = {0x01, 0x1A, 0xFC, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char set_address_evt[SET_ADDRESS_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x1A, 0xFC, 0x00};

char set_radio_cmd[SET_RADIO_CMD_LEN] = {0x01, 0x2C, 0xFC, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char set_radio_evt[SET_RADIO_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x2C, 0xFC, 0x00};

char set_grp_cmd[SET_GRP_CMD_LEN] = {0x01, 0xEA, 0xFC, 0x09, 0x02, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char set_grp_evt[SET_GRP_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0xEA, 0xFC, 0x00};

char set_pwr_offset_cmd[SET_PWR_OFFSET_CMD_LEN] = {0x01, 0xEA, 0xFC, 0x0A,
	0x02, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char set_pwr_offset_evt[SET_PWR_OFFSET_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0xEA, 0xFC, 0x00};

char write_pinmux_pin_num[PINMUX_REG_NUM_7902] = {0x00, 0x01, 0x02, 0x04};
char write_pinmux_pin_mode[PINMUX_REG_NUM_7902] = {0x01, 0x01, 0x01, 0x01};

char set_efem1_cmd[SET_EFEM_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0B, 0x01, 0x55, 0x07, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
char set_efem1_evt[SET_EFEM_EVT_LEN] = {0x04, 0xE4, 0x06, 0x02, 0x55, 0x02, 0x00, 0x00, 0x02};

char wifi_patch_query_cmd[WIFI_PATCH_QUERY_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x05, 0x01, 0x54, 0x01, 0x00, 0xF0};
char wifi_patch_query_evt[WIFI_PATCH_QUERY_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x54, 0x01, 0x00};

char wifi_patch_enable_cmd[WIFI_PATCH_ENABLE_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x05, 0x01, 0x54, 0x01, 0x00, 0xF1};
char wifi_patch_enable_evt[WIFI_PATCH_ENBALE_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x54, 0x01, 0x00, 0x00};

char set_radio_cmd_6639[SET_RADIO_CMD_LEN_6639] = {0x01, 0x2D, 0xFC, 0x17, 0x03,
						0x00, 0x00, 0x00,
						0x00, 0x00, 0x00,
						0x00, 0x00, 0x00,
						0x00, 0x00, 0x00,
						0x00, 0x00, 0x00,
						0x00, 0x00, 0x00,
						0x00, 0x00, 0x00,
						0x00};
char set_radio_evt_6639[SET_RADIO_EVT_LEN_6639] = {0x04, 0x0E, 0x05, 0x01, 0x2D, 0xFC, 0x00, 0x03};

char set_bt_loss_cmd[SET_BT_LOSS_CMD_LEN] = {0x01, 0xEA, 0xFC, 0x04, 0x02, 0x09, 0x07, 0x00};
char set_bt_loss_evt[SET_BT_LOSS_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0xEA, 0xFC, 0x00};

char set_compensation_cmd[SET_COMPENSATION_CMD_LEN] = {0x01, 0x93, 0xFC, 0x10,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char set_compensation_evt[SET_COMPENSATION_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x93, 0xFC, 0x00};

char set_compensation_cmd_6639[SET_COMPENSATION_CMD_LEN_6639] = {0x01, 0x93, 0xFC, 0x31, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char set_compensation_evt_6639[SET_COMPENSATION_EVT_LEN_6639] = {0x04, 0x0E, 0x04, 0x01, 0x93, 0xFC, 0x00};


struct bt_cmds_s commands[] = {
	{"UNKNOWN", {{0, NULL}, {0, NULL}}, 0},
	{"READ_EFUSE_EVTREAD_EFUSE_CMD", {{READ_EFUSE_CMD_LEN, efuse_r_cmd}, {0, NULL}}, DEFAULT},
	{"READ_EFUSE_EVTREAD_EFUSE_EVT", {{READ_EFUSE_EVT_HDR_LEN, efuse_r_evt}, {0, NULL}}, DEFAULT},
	{"HWERR_EVT", {{HWERR_EVT_LEN, hwerr_evt}, {0, NULL}}, DEFAULT},
	{"HCI_RESET_CMD", {{HCI_RESET_CMD_LEN, hci_reset_cmd}, {0, NULL}}, DEFAULT},
	{"HCI_RESET_EVT", {{HCI_RESET_EVT_LEN, hci_reset_evt}, {0, NULL}}, DEFAULT},
	{"STP0_CMD", {{SET_STP_CMD_LEN, stp0_cmd}, {0, NULL}}, DEFAULT},
	{"STP0_EVT", {{SET_STP_EVT_LEN, stp0_evt}, {0, NULL}}, DEFAULT},
	{"STP1_CMD", {{SET_STP1_CMD_LEN, stp1_cmd}, {0, NULL}}, DEFAULT},
	{"STP1_EVT", {{SET_STP1_EVT_LEN, stp1_evt}, {0, NULL}}, DEFAULT},
	{"WMT_POWER_ON_CMD", {{WMT_POWER_ON_CMD_LEN, wmt_power_on_cmd}, {0, NULL}}, DEFAULT},
	{"WMT_POWER_ON_EVT", {{WMT_POWER_ON_EVT_HDR_LEN, wmt_power_on_evt}, {0, NULL}}, DEFAULT},
	{"WMT_POWER_OFF_CMD", {{WMT_POWER_OFF_CMD_LEN, wmt_power_off_cmd}, {0, NULL}}, DEFAULT},
	{"WMT_POWER_OFF_EVT", {{WMT_POWER_OFF_EVT_HDR_LEN, wmt_power_off_evt}, {0, NULL}}, DEFAULT},
	{"PHASE1_WMT_EVT", {{EVT_HDR_LEN, phase1_wmt_evt}, {0, NULL}}, DEFAULT},
	{"VENDOR_EVT", {{EVT_HDR_LEN, vendor_evt}, {0, NULL}}, DEFAULT},
	{"PICUS_ENABLE_CMD", {{PICUS_ENABLE_CMD_LEN, picus_enable_cmd}, {0, NULL}}, DEFAULT},
	{"PICUS_ENABLE_EVT", {{PICUS_ENABLE_EVT_HDR_LEN, picus_enable_event}, {0, NULL}}, DEFAULT},
	{"PICUS_DISABLE_CMD", {{PICUS_DISABLE_CMD_LEN, picus_disable_cmd}, {0, NULL}}, DEFAULT},
	{"PICUS_DISABLE_EVT", {{PICUS_DISABLE_EVT_HDR_LEN, picus_disable_evt}, {0, NULL}}, DEFAULT},
	{"WMT_ASSERT_CMD", {{WMT_ASSERT_CMD_LEN, wmt_assert_cmd}, {0, NULL}}, DEFAULT},
	{"FD5B_ASSERT_CMD", {{FD5B_ASSERT_CMD_LEN, fd5b_assert_cmd}, {0, NULL}}, DEFAULT},
	{"TXPOWER_CMD", {{TXPOWER_CMD_LEN, txpower_cmd}, {0, NULL}}, DEFAULT},
	{"TXPOWER_EVT", {{TXPOWER_EVT_LEN, txpower_evt}, {0, NULL}}, DEFAULT},
	{"AUDIO_SLAVE_CMD", {{AUDIO_SETTING_CMD_LEN, audio_slave_cmd}, {0, NULL}}, DEFAULT},
	{"AUDIO_SLAVE_EVT", {{AUDIO_SETTING_EVT_LEN, audio_slave_evt}, {0, NULL}}, DEFAULT},
	{"READ_PINMUX_CMD", {{READ_PINMUX_CMD_LEN, read_pinmux_cmd}, {0, NULL}}, DEFAULT},
	{"READ_PINMUX_EVT", {{READ_PINMUX_EVT_CMP_LEN, read_pinmux_evt}, {0, NULL}}, DEFAULT},
	{"WRITE_PINMUX_CMD", {{WRITE_PINMUX_CMD_LEN, write_pinmux_cmd}, {0, NULL}}, DEFAULT},
	{"WRITE_PINMUX_EVT", {{WRITE_PINMUX_EVT_LEN, write_pinmux_evt}, {0, NULL}}, DEFAULT},
	{"READ_ADDRESS_CMD", {{READ_ADDRESS_CMD_LEN, read_address_cmd}, {0, NULL}}, DEFAULT},
	{"READ_ADDRESS_EVT", {{READ_ADDRESS_EVT_HDR_LEN, read_address_evt}, {0, NULL}}, DEFAULT},
	{"RES_APCF_CMD", {{RES_APCF_CMD_LEN, res_apcf_cmd}, {0, NULL}}, DEFAULT},
	{"RES_APCF_EVT", {{RES_APCF_EVT_LEN, res_apcf_evt}, {0, NULL}}, DEFAULT},
	{"WOBLE_ENABLE_DEFAULT_CMD", {{WOBLE_ENABLE_DEFAULT_CMD_LEN, woble_enable_default_cmd}, {0, NULL}}, DEFAULT},
	{"WOBLE_ENABLE_DEFAULT_EVT", {{WOBLE_ENABLE_DEFAULT_EVT_LEN, woble_enable_default_evt}, {0, NULL}}, DEFAULT},
	{"WOBLE_DISABLE_DEFAULT_CMD", {{WOBLE_DISABLE_DEFAULT_CMD_LEN, woble_disable_default_cmd}, {0, NULL}}, DEFAULT},
	{"WOBLE_DISABLE_DEFAULT_EVT", {{WOBLE_DISABLE_DEFAULT_EVT_LEN, woble_disable_default_evt}, {0, NULL}}, DEFAULT},
	{"RADIO_OFF_CMD", {{RADIO_OFF_CMD_LEN, radio_off_cmd}, {0, NULL}}, DEFAULT},
	{"RADIO_OFF_EVT", {{RADIO_OFF_EVT_LEN, radio_off_evt}, {0, NULL}}, DEFAULT},
	{"RADIO_ON_CMD", {{RADIO_ON_CMD_LEN, radio_on_cmd}, {0, NULL}}, DEFAULT},
	{"RADIO_ON_EVT", {{RADIO_ON_EVT_LEN, radio_on_evt}, {0, NULL}}, DEFAULT},
	{"APCF_FILTER_CMD", {{APCF_FILTER_CMD_LEN, apcf_filter_cmd}, {0, NULL}}, DEFAULT},
	{"APCF_FILTER_EVT", {{APCF_FILTER_EVT_HDR_LEN, apcf_filter_evt}, {0, NULL}}, DEFAULT},
	{"WOBLE_APCF_CMD", {{APCF_CMD_LEN, apcf_cmd}, {0, NULL}}, DEFAULT},
	{"WOBLE_APCF_EVT", {{APCF_EVT_HDR_LEN, apcf_evt}, {0, NULL}}, DEFAULT},
	{"APCF_DELETE_CMD", {{APCF_DELETE_CMD_LEN, apcf_delete_cmd}, {0, NULL}}, DEFAULT},
	{"APCF_DELETE_EVT", {{APCF_DELETE_EVT_HDR_LEN, apcf_delete_evt}, {0, NULL}}, DEFAULT},
	{"APCF_RESUME_EVT", {{APCF_RESUME_EVT_HDR_LEN, apcf_resume_evt}, {0, NULL}}, DEFAULT},
	{"CHECK_WOBX_DEBUG_CMD", {{CHECK_WOBX_DEBUG_CMD_LEN, check_wobx_debug_cmd}, {0, NULL}}, DEFAULT},
	{"CHECK_WOBX_DEBUG_EVT", {{CHECK_WOBX_DEBUG_EVT_HDR_LEN, check_wobx_debug_evt}, {0, NULL}}, DEFAULT},
	{"NOTIFY_ALT_EVT", {{NOTIFY_ALT_EVT_LEN, notify_alt_evt}, {0, NULL}}, DEFAULT},
	{"LD_PATCH_CMD", {{LD_PATCH_CMD_LEN, ld_patch_cmd}, {0, NULL}}, DEFAULT},
	{"LD_PATCH_EVT", {{LD_PATCH_EVT_LEN, ld_patch_evt}, {0, NULL}}, DEFAULT},
	{"LD_PATCH_CMD_USB", {{LD_PATCH_CMD_LEN_USB, ld_patch_cmd_usb}, {0, NULL}}, DEFAULT},
	{"READ_REGISTER_CMD", {{READ_REGISTER_CMD_LEN, read_register_cmd}, {0, NULL}}, DEFAULT},
	{"READ_REGISTER_EVT", {{READ_REGISTER_EVT_HDR_LEN, read_register_evt}, {0, NULL}}, DEFAULT},
	{"WRITE_REGISTER_CMD", {{WRITE_REGISTER_CMD_LEN, write_register_cmd}, {0, NULL}}, DEFAULT},
	{"WRITE_REGISTER_EVT", {{WRITE_REGISTER_EVT_HDR_LEN, write_register_evt}, {0, NULL}}, DEFAULT},
	{"SET_BAUD_CMD_115200", {{SETBAUD_CMD_LEN_CONNAC2, set_baud_cmd_115200}, {0, NULL}}, DEFAULT},
	{"SET_BAUD_CMD_230400", {{SETBAUD_CMD_LEN_CONNAC2, set_baud_cmd_230400}, {0, NULL}}, DEFAULT},
	{"SET_BAUD_CMD_921600", {{SETBAUD_CMD_LEN_CONNAC2, set_baud_cmd_921600}, {0, NULL}}, DEFAULT},
	{"SET_BAUD_CMD_2M", {{SETBAUD_CMD_LEN_CONNAC2, set_baud_cmd_2M}, {0, NULL}}, DEFAULT},
	{"SET_BAUD_CMD_3M", {{SETBAUD_CMD_LEN_CONNAC2, set_baud_cmd_3M}, {0, NULL}}, DEFAULT},
	{"SET_BAUD_CMD_4M", {{SETBAUD_CMD_LEN_CONNAC2, set_baud_cmd_4M}, {0, NULL}}, DEFAULT},
	{"SET_BAUD_EVT", {{SETBAUD_EVT_LEN, set_baud_evt}, {0, NULL}}, DEFAULT},
	{"GET_BAUD_CMD", {{GETBAUD_CMD_LEN, get_baud_cmd}, {0, NULL}}, DEFAULT},
	{"GET_BAUD_EVT", {{GETBAUD_EVT_LEN, get_baud_evt}, {0, NULL}}, DEFAULT},
	{"UART_WAKEUP_CMD", {{WAKEUP_CMD_LEN, uart_wakeup_cmd}, {0, NULL}}, DEFAULT},
	{"UART_WAKEUP_EVT", {{WAKEUP_EVT_LEN, uart_wakeup_evt}, {0, NULL}}, DEFAULT},
	{"UART_FW_OWN_CMD", {{FWOWN_TEST_CMD_LEN, uart_fw_own_cmd}, {0, NULL}}, DEFAULT},
	{"UART_FW_OWN_EVT", {{OWNTYPE_TEST_EVT_LEN, uart_fw_own_evt}, {0, NULL}}, DEFAULT},
	{"UART_DRIVER_OWN_CMD", {{DRVOWN_TEST_CMD_LEN, uart_driver_own_cmd}, {0, NULL}}, DEFAULT},
	{"UART_DRIVER_OWN_EVT", {{OWNTYPE_TEST_EVT_LEN, uart_driver_own_evt}, {0, NULL}}, DEFAULT},
	{"SET_ADDRESS_CMD", {{SET_ADDRESS_CMD_LEN, set_address_cmd}, {0, NULL}}, DEFAULT},
	{"SET_ADDRESS_EVT", {{SET_ADDRESS_EVT_LEN, set_address_evt}, {0, NULL}}, DEFAULT},
	{"SET_RADIO_CMD", {{SET_RADIO_CMD_LEN, set_radio_cmd}, {0, NULL}}, DEFAULT},
	{"SET_RADIO_EVT", {{SET_RADIO_EVT_LEN, set_radio_evt}, {0, NULL}}, DEFAULT},
	{"SET_GRP_CMD", {{SET_GRP_CMD_LEN, set_grp_cmd}, {0, NULL}}, DEFAULT},
	{"SET_GRP_EVT", {{SET_GRP_EVT_LEN, set_grp_evt}, {0, NULL}}, DEFAULT},
	{"SET_PWR_OFFSET_CMD", {{SET_PWR_OFFSET_CMD_LEN, set_pwr_offset_cmd}, {0, NULL}}, DEFAULT},
	{"SET_PWR_OFFSET_EVT", {{SET_PWR_OFFSET_EVT_LEN, set_pwr_offset_evt}, {0, NULL}}, DEFAULT},
	{"AUDIO_PINMUX_NUM", {{PINMUX_REG_NUM_7902, write_pinmux_pin_num}, {0, NULL}}, DEFAULT},
	{"AUDIO_PINMUX_MODE", {{PINMUX_REG_NUM_7902, write_pinmux_pin_mode}, {0, NULL}}, DEFAULT},
	{"SET_EFEM1_CMD", {{SET_EFEM_CMD_LEN, set_efem1_cmd}, {0, NULL}}, DEFAULT},
	{"SET_EFEM1_EVT", {{SET_EFEM_EVT_LEN, set_efem1_evt}, {0, NULL}}, DEFAULT},
	{"WIFI_PATCH_QUERY_CMD", {{WIFI_PATCH_QUERY_CMD_LEN, wifi_patch_query_cmd}, {0, NULL}}, DEFAULT},
	{"WIFI_PATCH_QUERY_EVT", {{WIFI_PATCH_QUERY_EVT_LEN, wifi_patch_query_evt}, {0, NULL}}, DEFAULT},
	{"WIFI_PATCH_ENABLE_CMD", {{WIFI_PATCH_ENABLE_CMD_LEN, wifi_patch_enable_cmd}, {0, NULL}}, DEFAULT},
	{"WIFI_PATCH_ENBALE_EVT", {{WIFI_PATCH_ENBALE_EVT_LEN, wifi_patch_enable_evt}, {0, NULL}}, DEFAULT},
	{"SET_RADIO_CMD_6639", {{SET_RADIO_CMD_LEN_6639, set_radio_cmd_6639}, {0, NULL}}, DEFAULT},
	{"SET_RADIO_EVT_6639", {{SET_RADIO_EVT_LEN_6639, set_radio_evt_6639}, {0, NULL}}, DEFAULT},
	{"SET_BT_LOSS_CMD", {{SET_BT_LOSS_CMD_LEN, set_bt_loss_cmd}, {0, NULL}}, DEFAULT},
	{"SET_BT_LOSS_EVT", {{SET_BT_LOSS_EVT_LEN, set_bt_loss_evt}, {0, NULL}}, DEFAULT},
	{"SET_COMPENSATION_CMD", {{SET_COMPENSATION_CMD_LEN, set_compensation_cmd}, {0, NULL}}, DEFAULT},
	{"SET_COMPENSATION_EVT", {{SET_COMPENSATION_EVT_LEN, set_compensation_evt}, {0, NULL}}, DEFAULT},
	{"SET_COMPENSATION_CMD_6639", {{SET_COMPENSATION_CMD_LEN_6639, set_compensation_cmd_6639}, {0, NULL}}, DEFAULT},
	{"SET_COMPENSATION_EVT_6639", {{SET_COMPENSATION_EVT_LEN_6639, set_compensation_evt_6639}, {0, NULL}}, DEFAULT},
	{"RHW_ASSERT_CMD", {{RHW_ASSERT_CMD_LEN, rhw_assert_cmd}, {0, NULL}}, DEFAULT}
};

struct bt_cmds_s command_raw_data[4][BTMTK_CMD_NUM];

int btmtk_common_data_initialize(struct btmtk_dev *bdev)
{
	if (bdev->dongle_index >= 4) {
		BTMTK_ERR("%s: dongle_index must smaller than 4 (%d)", __func__, bdev->dongle_index);
		return -1;
	}

	memcpy(command_raw_data[bdev->dongle_index], commands, sizeof(commands));
	bdev->bt_raw_data = command_raw_data[bdev->dongle_index];

	return 0;
}

void btmtk_free_chip_struct(	struct data_struct *fw_cfg, int count)
{
	int i = 0;

	for (i = 0; i < count; i++) {
		if (fw_cfg[i].content) {
			BTMTK_INFO("%s:kfree %d", __func__, i);
			kfree(fw_cfg[i].content);
			fw_cfg[i].content = NULL;
			fw_cfg[i].len = 0;
		} else
			fw_cfg[i].len = 0;
	}
}

void btmtk_free_chip_data(struct btmtk_dev *bdev)
{
	struct bt_cmds_s *bt_raw_cmds;
	int i;

	BTMTK_INFO("%s begin", __func__);
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev == NULL", __func__);
		return;
	}
	bt_raw_cmds = (struct bt_cmds_s *)bdev->bt_raw_data;

	for (i = 0; i < BTMTK_CMD_NUM; i++) {
		btmtk_free_chip_struct(&bt_raw_cmds[i].chip_data[FROM_BIN], 1);
		bt_raw_cmds[i].usage = DEFAULT;
	}
}

int btmtk_load_chip_data_item(struct bt_cmds_s *raw_cmds,
		int counter, u8 *searchcontent, enum fw_cfg_index_len index_length)
{
	char *block_name;
	struct data_struct *save_content;
	int ret = 0, i = 0;
	u16 temp_len = 0;
	u8 temp[TEMP_LEN]; /* save for total hex number */
	unsigned long parsing_result = 0;
	char *search_result = NULL, *ptr = NULL;
	char *search_end = NULL;
	char search[SEARCH_LEN];
	char *next_block = NULL;
	char number[CHAR2HEX_SIZE + 1];	/* 1 is for '\0' */

	memset(search, 0, SEARCH_LEN);
	memset(temp, 0, TEMP_LEN);
	memset(number, 0, CHAR2HEX_SIZE + 1);

	block_name = raw_cmds->name;
	save_content = &raw_cmds->chip_data[FROM_BIN];

	if (searchcontent == NULL) {
		BTMTK_ERR("%s: Searchcontent is NULL", __func__);
		return -1;
	}

	/* search block name */
	for (i = 0; i < counter; i++) {
		temp_len = 0;
		if (index_length == FW_CFG_INX_LEN_2) /* EX: APCF01 */
			(void)snprintf(search, SEARCH_LEN, "%s%02d:", block_name, i);
		else if (index_length == FW_CFG_INX_LEN_3) /* EX: APCF001 */
			(void)snprintf(search, SEARCH_LEN, "%s%03d:", block_name, i);
		else
			(void)snprintf(search, SEARCH_LEN, "%s:", block_name);

		ret = 0;

		ptr = search_result = strstr((char *)searchcontent, search);
		if (search_result) {
			/* Add # for comment in bt.cfg */
			if (ptr > (char *)searchcontent) {
				ptr--;
				while ((*ptr == ' ') && (ptr != (char *)searchcontent))
					ptr--;
				if (*ptr == '#') {
					BTMTK_WARN("%s: %s has been ignored", __func__, search);
					return -1;
				}
			}
			/* cmd found but may not have data */
			raw_cmds->usage = SEND_NULL;
			BTMTK_INFO("%s: %s has been found", __func__, search);

			memset(temp, 0, TEMP_LEN);
			search_end = search_result;
			search_result = strstr(search_result, "0x");

			BTMTK_INFO("%s: search is %ld", __func__, strlen(search));
			BTMTK_INFO("%s: len is %ld", __func__, (search_result - search_end));
			if (search_result == NULL ||
					(search_result - search_end) != strlen(search)) {
				BTMTK_ERR("%s: search_result is NULL", __func__);
				return ret;
			}

			/* find next line as end of this command line, if NULL means last line */
			next_block = strstr(search_result, ":");
			if (next_block == NULL)
				BTMTK_WARN("%s: if NULL means last line", __func__);

			do {
				search_end = strstr(search_result, ",");
				if (search_end == NULL) {
					BTMTK_ERR("%s: Search_end is NULL", __func__);
					break;
				}

				if (search_end - search_result != CHAR2HEX_SIZE) {
					BTMTK_ERR("%s: Incorrect Format in %s", __func__, search);
					break;
				}

				memset(number, 0, CHAR2HEX_SIZE + 1);
				memcpy(number, search_result, CHAR2HEX_SIZE);
				ret = kstrtoul(number, 0, &parsing_result);
				if (ret == 0) {
					if (temp_len >= TEMP_LEN) {
						BTMTK_ERR("%s: %s data over %d", __func__, search, TEMP_LEN);
						break;
					}
					temp[temp_len] = parsing_result;
					temp_len++;
				} else {
					BTMTK_WARN("%s: %s kstrtoul fail: %d", __func__, search, ret);
					break;
				}
				search_result = strstr(search_end, "0x");
				if (search_result == NULL) {
					BTMTK_ERR("%s: search_result is NULL", __func__);
					break;
				}
			} while (search_result < next_block || (search_result && next_block == NULL));
		} else
			BTMTK_DBG("%s: %s is not found in %d", __func__, search, i);

		if (temp_len && temp_len < TEMP_LEN) {
			BTMTK_INFO("%s: %s found & stored in %d", __func__, search, i);
			save_content[i].content = kzalloc(temp_len, GFP_KERNEL);
			if (save_content[i].content == NULL) {
				BTMTK_ERR("%s: Allocate memory fail(%d)", __func__, i);
				return -ENOMEM;
			}
			memcpy(save_content[i].content, temp, temp_len);
			save_content[i].len = temp_len;
			BTMTK_INFO_RAW(save_content[i].content, save_content[i].len, "%s", search);
			raw_cmds->usage = FROM_BIN;
			ret = 0; /* has found valid data */
		}
	}

	return ret;
}

int btmtk_load_raw_data(char *data_bin_name, struct btmtk_dev *bdev)
{
	int err = 0;
	u32 code_len = 0;
	int i;
	//struct bt_raw_data_struct *raw_data_content = bdev->bt_raw_data;
	struct bt_cmds_s *bt_raw_cmds = (struct bt_cmds_s *)bdev->bt_raw_data;

	btmtk_free_chip_data(bdev);
	err = btmtk_load_code_from_setting_files(data_bin_name, bdev->intf_dev, &code_len, bdev);
	if (err) {
		BTMTK_WARN("btmtk_load_code_from_setting_files failed!!");
		goto LOAD_END;
	} else {
	/* load from file */
		for (i = 0; i < BTMTK_CMD_NUM; i++) {
			err = btmtk_load_chip_data_item(&bt_raw_cmds[i],
						1, bdev->setting_file, FW_CFG_INX_LEN_NONE);
			if (err)
				BTMTK_WARN("%s: search item %s is invalid!", __func__, bt_raw_cmds[i].name);
		}
	}
LOAD_END:
	/* release setting file memory */
	kfree(bdev->setting_file);
	bdev->setting_file = NULL;

	if (err)
		BTMTK_ERR("%s: error return %d", __func__, err);

	return err;
}

void btmtk_get_cmd_or_event(struct btmtk_dev *bdev,
		enum cmd_index_s type, struct data_struct *default_cmd)
{
	struct bt_cmds_s *bt_raw_cmds = NULL;
	struct bt_cmds_s bt_cmd;
	if (bdev == NULL || bdev->bt_raw_data == NULL) {
		BTMTK_ERR("%s: Incorrect bdev", __func__);
		return;
	}

	if (type >= BTMTK_CMD_NUM) {
		BTMTK_ERR("%s: type %d is illegal!!!", __func__, type);
		return;
	}

	bt_raw_cmds = (struct bt_cmds_s *)bdev->bt_raw_data;
	bt_cmd = bt_raw_cmds[type];

	if (bt_cmd.usage == SEND_NULL) {
		default_cmd->content = NULL;
		default_cmd->len = 0;
		BTMTK_INFO("%s: type %s no data to send! ", __func__, bt_cmd.name);
	} else {
		BTMTK_DBG("%s: %s load from %d", __func__, bt_cmd.name, bt_cmd.usage);
		default_cmd->content = bt_cmd.chip_data[bt_cmd.usage].content;
		default_cmd->len = bt_cmd.chip_data[bt_cmd.usage].len;
	}
	BTMTK_DBG_RAW(default_cmd->content, default_cmd->len, "%s: get(%s) - ", __func__, bt_cmd.name);

	return;
}

int btmtk_send_cmd_to_fw(struct btmtk_dev *bdev,
		enum cmd_index_s cmd_type, enum cmd_index_s evt_type,
		int delay, int retry, int pkt_type, bool flag)
{
	int ret = 0;	/* if successful, 0 */
	struct data_struct cmd = {0}, event = {0};

	if (bdev == NULL) {
		BTMTK_ERR("%s: Incorrect bdev", __func__);
		ret = -1;
		return ret;
	}

	BTMTK_DBG("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, cmd_type, cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, evt_type, event);

	if (cmd.content == NULL || cmd.len <= 0) {
		BTMTK_WARN("%s: do not send cmd %d", __func__, cmd_type);
		return ret;
	}
	ret = btmtk_main_send_cmd(bdev,
			cmd.content, cmd.len,
			event.content, event.len,
			delay, retry, pkt_type, flag);
	if (ret < 0)
		BTMTK_ERR("%s: failed, type is %d", __func__, cmd_type);

	BTMTK_INFO("%s: end. ret = %d", __func__, ret);
	return ret;
}


void btmtk_recv_error_handler_common(struct hci_dev *hdev,
		const u8 *buf, u32 len, const u8 *dbg_buf, u32 dbg_len)
{
	BTMTK_INFO("%s: enter", __func__);

	btmtk_hci_snoop_print(dbg_buf, dbg_len);
	btmtk_hci_snoop_print(buf, len);
}

int btmtk_cif_rx_packet_handler_common(struct btmtk_dev *bdev,  struct sk_buff *skb)
{
	int state = 0;
	int err = 0;

	/* for bluetooth kpi */
	btmtk_dispatch_fwlog_bluetooth_kpi(bdev, skb->data, skb->len, hci_skb_pkt_type(skb));
	/* If reset stack enabled,
	 * driver should discard the frames
	 * when is in suspend/resume state
	 */
	state = btmtk_get_chip_state(bdev);
	if (bdev->bt_cfg.reset_stack_after_woble &&
		(state == BTMTK_STATE_SUSPEND || state == BTMTK_STATE_RESUME)) {
		kfree_skb(skb);
		err = CONTINUE_RET;
		//BTMTK_INFO("%s: skip", __func__);
		return err; /* 1 for continue */
	}

	err = hci_recv_frame(bdev->hdev, skb);

	return err;
}

int btmtk_cif_tx_frame_handler_common(struct btmtk_dev *bdev, struct sk_buff *skb, u8 *fw_dump)
{
	int ret = 0;
	/* parsing commands */
	struct data_struct reset_cmd = {0}, fw_coredump_cmd = {0};
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	BTMTK_DBG("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, HCI_RESET_CMD, reset_cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, FD5B_ASSERT_CMD, fw_coredump_cmd);

	/* +1 byte for not include pkt_type */
	btmtk_dispatch_fwlog_bluetooth_kpi(bdev, skb->data + 1, skb->len - 1, hci_skb_pkt_type(skb));

	if (memcmp(skb->data, bmain_info->read_iso_packet_size_cmd,
			READ_ISO_PACKET_SIZE_CMD_HDR_LEN) == 0) {
		bdev->iso_threshold = skb->data[READ_ISO_PACKET_SIZE_CMD_HDR_LEN] +
					(skb->data[READ_ISO_PACKET_SIZE_CMD_HDR_LEN + 1]  << 8);
		BTMTK_INFO("%s: Ble iso pkt size is %d", __func__, bdev->iso_threshold);
	}

	if (hci_skb_pkt_type(skb) == HCI_COMMAND_PKT) {
#if (USE_DEVICE_NODE == 0)
		if (bdev->get_hci_reset == 1) {
			ret = btmtk_set_audio_setting(bdev);
			bdev->get_hci_reset = 0;
			if (ret < 0) {
				BTMTK_ERR("%s btmtk_set_audio_setting failed!!", __func__);
				return ret;
			}
		}
#endif
		/* save hci cmd pkt for debug */
		btmtk_hci_snoop_save(HCI_SNOOP_TYPE_CMD_STACK, skb->data, skb->len);
		if (skb->len == FD5B_ASSERT_CMD_LEN &&
			!memcmp(skb->data, fw_coredump_cmd.content, fw_coredump_cmd.len)) {
			BTMTK_INFO("%s: Dongle FW Assert Triggered by BT Stack!", __func__);
			btmtk_assert_wake_lock();
			*fw_dump = 1;
			btmtk_reset_timer_update(bdev);
			btmtk_hci_snoop_print_to_log();
#if (USE_DEVICE_NODE == 1)
			if (bdev->assert_reason[0] == '\0') {
				memset(bdev->assert_reason, 0, ASSERT_REASON_SIZE);
				strncpy(bdev->assert_reason, "[BT_DRV assert] host trigger",
						strlen("[BT_DRV assert] host trigger") + 1);
				BTMTK_ERR("%s: [assert_reason] %s", __func__, bdev->assert_reason);
			}
			if(bmain_info->hif_hook.trigger_assert) {
				bmain_info->hif_hook.trigger_assert(bdev);
				/* return 1 means data already handled */
				return 1;
			}
#endif // (USE_DEVICE_NODE == 1)
		} else if (skb->len == HCI_RESET_CMD_LEN &&
				!memcmp(skb->data, reset_cmd.content, reset_cmd.len)) {
#if (USE_DEVICE_NODE == 1)
				u8 drv_own_retry = 10;
				// u8 evt[] = { 0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C };

				BTMTK_INFO("%s: got command: 0x03 0x0C 0x00 (HCI_RESET)", __func__);

				do {
					ret = bmain_info->hif_hook.send_and_recv(bdev, skb, skb->data, 3,
							DELAY_TIMES, RETRY_TIMES, BTMTK_TX_PKT_FROM_HOST, CMD_NEED_FILTER);
				} while (ret == -EAGAIN && drv_own_retry--);

				if (ret < 0)
					BTMTK_ERR("%s host call send_and_recv failed, ret[%d]", __func__, ret);
				else
					/* return 1 means data already handled */
					ret = 1;
				return ret;
#else
			BTMTK_INFO("%s: got command: 0x03 0C 00 (HCI_RESET)", __func__);
#endif
		}
#if (USE_DEVICE_NODE == 0)
		} else if (hci_skb_pkt_type(skb) == HCI_ACLDATA_PKT) {
			btmtk_hci_snoop_save(HCI_SNOOP_TYPE_TX_ACL_STACK, skb->data, skb->len);
		} else if (hci_skb_pkt_type(skb) == HCI_ISO_PKT) {
			btmtk_hci_snoop_save(HCI_SNOOP_TYPE_TX_ISO_STACK, skb->data, skb->len);
#else
		} else if (hci_skb_pkt_type(skb) == MTK_HCI_SCODATA_PKT) {
			/* for VTS SOC data loopback test workaround
			 * Uarthub and BT uart SOC format not sync
			 */
			u8 new_soc_hd[] = { 0x03, 0x00, 0x00, 0x00, 0x00};
			unsigned char *skb_tmp = NULL;

			BTMTK_INFO_RAW(skb->data, skb->len, "%s: VTS TX before rearrange[%d]:",
						__func__, skb->len);

			/* original:  03 AA BB	 LL 	 PP PP PP
			 * rearrange: 03 AA LL+2 LL+1 BB PP PP PP
			 */
			new_soc_hd[1] = skb->data[1];
			new_soc_hd[2] = skb->data[3] + 2;
			new_soc_hd[3] = skb->data[3] + 1;
			new_soc_hd[4] = skb->data[2];

			skb_tmp = skb_push(skb, 1);
			if (!skb_tmp) {
				BTMTK_ERR("%s, skb_put failed!", __func__);
				ret = -ENOMEM;
				return ret;
			}
			memcpy(skb_tmp, new_soc_hd, sizeof(new_soc_hd));
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: VTS TX after rearrange[%d]:",
					__func__, skb->len);
#endif
	}

	return ret;
}

int btmtk_cif_check_power_status_common(struct btmtk_dev *bdev, const uint8_t *cmd, int pkt_type)
{
	int ret = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (memcmp(cmd, bmain_info->wmt_over_hci_header, WMT_OVER_HCI_HEADER_SIZE) &&
			pkt_type == BTMTK_TX_PKT_FROM_HOST &&
			bdev->power_state != BTMTK_DONGLE_STATE_POWER_ON) {
		BTMTK_WARN("%s: chip power isn't on, ignore this command, state is %d",
				__func__, bdev->power_state);
		ret = -1;
	}
	return ret;
}

int btmtk_load_rom_patch_common(struct btmtk_dev *bdev)
{
	int err = 0;

	BTMTK_INFO("%s: enter", __func__);

	err = btmtk_load_rom_patch_connac3(bdev, BT_DOWNLOAD);
	if (err < 0) {
		BTMTK_ERR("%s: btmtk_load_rom_patch_connac3 bt patch failed!", __func__);
		return err;
	}

#if CFG_SUPPORT_BT_DL_WIFI_PATCH
	err = btmtk_load_rom_patch_connac3(bdev, WIFI_DOWNLOAD);
	if (err < 0) {
		BTMTK_WARN("%s: btmtk_load_rom_patch_connac3 wifi patch failed!", __func__);
		err = 0;
	}
#endif
	return err;
}

static int btmtk_set_audio_pin_mux_common(struct btmtk_dev *bdev)
{
	int ret = 0;
	unsigned int i = 0;
	struct fw_cfg_struct *audio_pinmux_num = &bdev->bt_cfg.audio_pinmux_num;
	struct fw_cfg_struct *audio_pinmux_mode = &bdev->bt_cfg.audio_pinmux_mode;
	struct data_struct write_pinmux_cmd = {0}, write_pinmux_event = {0};
	struct data_struct write_pinmux_pin_num = {0}, write_pinmux_pin_mode = {0};

	BTMTK_INFO("%s enter", __func__);

	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, WRITE_PINMUX_CMD, write_pinmux_cmd);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, WRITE_PINMUX_EVT, write_pinmux_event);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, AUDIO_PINMUX_NUM, write_pinmux_pin_num);
	BTMTK_GET_CMD_OR_EVENT_DATA(bdev, AUDIO_PINMUX_MODE, write_pinmux_pin_mode);

	if(!write_pinmux_cmd.len){
		BTMTK_WARN("%s write_pinmux_cmd is NULL, return", __func__);
		return ret;
	}

	if (audio_pinmux_num->content && audio_pinmux_num->length) {
		BTMTK_INFO("%s load audio pinmux num from bt.cfg", __func__);
		memcpy(write_pinmux_pin_num.content, audio_pinmux_num->content, audio_pinmux_num->length);
	} else {
		BTMTK_INFO("%s load default audio pinmux num", __func__);
	}
	BTMTK_INFO_RAW(write_pinmux_pin_num.content,
			write_pinmux_pin_num.len, "%s: Pin NUM:", __func__);

	if (audio_pinmux_mode->content && audio_pinmux_mode->length) {
		BTMTK_INFO("%s load audio pinmux mode from bt.cfg", __func__);
		memcpy(write_pinmux_pin_mode.content, audio_pinmux_mode->content, audio_pinmux_mode->length);
	} else {
		BTMTK_INFO("%s load default audio pinmux mode", __func__);
	}
	BTMTK_INFO_RAW(write_pinmux_pin_mode.content,
			write_pinmux_pin_mode.len, "%s: Pin MODE:", __func__);

	for (i = 0; i < PINMUX_REG_NUM_7902; i++) {
		write_pinmux_cmd.content[write_pinmux_cmd.len - 2] = write_pinmux_pin_num.content[i];
		write_pinmux_cmd.content[write_pinmux_cmd.len - 1] = write_pinmux_pin_mode.content[i];

		BTMTK_INFO_RAW(write_pinmux_cmd.content, write_pinmux_cmd.len, "%s: Send CMD:", __func__);
		ret = btmtk_main_send_cmd(bdev, write_pinmux_cmd.content, write_pinmux_cmd.len,
				write_pinmux_event.content, write_pinmux_event.len, 0, 0, BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);
		if (ret < 0) {
			BTMTK_ERR("%s: failed(%d)", __func__, ret);
			goto exit;
		}

		BTMTK_INFO("%s, confirm pinmux num : 0x%02x, mode :0x%02x", __func__,
				write_pinmux_pin_num.content[i], write_pinmux_pin_mode.content[i]);
	}

exit:
	return ret;
}

static int btmtk_get_fw_info_common(struct btmtk_dev *bdev)
{
	int ret = 0;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	ret = bmain_info->hif_hook.reg_read(bdev, FLAVOR, &bdev->flavor);
	if (ret < 0) {
		BTMTK_ERR("read flavor id failed");
		ret = -EIO;
		return ret;
	}
	ret = bmain_info->hif_hook.reg_read(bdev, FW_VERSION, &bdev->fw_version);
	if (ret < 0) {
		BTMTK_ERR("read fw version failed");
		ret = -EIO;
		return ret;
	}

	BTMTK_INFO("%s: Chip ID = 0x%x", __func__, bdev->chip_id);
	BTMTK_INFO("%s: flavor = 0x%x", __func__, bdev->flavor);
	BTMTK_INFO("%s: FW Ver = 0x%x", __func__, bdev->fw_version);

	memset(bdev->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
	if ((bdev->fw_version & 0xff) == 0xff) {
		BTMTK_ERR("%s: failed, wrong FW version : 0x%x !", __func__, bdev->fw_version);
		ret = -1;
		return ret;
	}

	bdev->dualBT = 0;

	/* Bin filename format : "BT_RAM_CODE_MT%04x_%x_%x_hdr.bin"
	 *	$$$$ : chip id
	 *	% : fw version & 0xFF + 1 (in HEX)
	 */

	bdev->proj = 0;
	bdev->flavor = (bdev->flavor & 0x00000080) >> 7;


	BTMTK_INFO("%s: proj = 0x%x", __func__, bdev->proj);
	BTMTK_INFO("%s: flavor1 = 0x%x", __func__, bdev->flavor);
	return ret;
}

#ifdef CHIP_IF_SDIO
#if 0
int btmtk_sdio_read_conn_infra_pc_common(void *func, u32 *val)
{
	btmtk_sdio_writel(0x44, 0, func);
	btmtk_sdio_writel(0x3C, 0x9F1E0000, func);
	btmtk_sdio_readl(0x38, val, func);

	return 0;
}
#endif
#endif

int btmtk_cif_chip_common_register(void)
{
	int retval = 0;
	struct hif_hook_chip_ptr hook_chip;

	BTMTK_INFO("%s", __func__);

	memset(&hook_chip, 0, sizeof(hook_chip));
	hook_chip.get_fw_info = btmtk_get_fw_info_common;
	hook_chip.load_patch = btmtk_load_rom_patch_common;
	hook_chip.err_handler = btmtk_recv_error_handler_common;
	hook_chip.rx_handler = btmtk_cif_rx_packet_handler_common;
	hook_chip.dispatch_fwlog = btmtk_dispatch_fwlog;
	hook_chip.bt_tx_frame_handler = btmtk_cif_tx_frame_handler_common;
	hook_chip.bt_check_power_status = btmtk_cif_check_power_status_common;
	hook_chip.bt_set_pinmux = btmtk_set_audio_pin_mux_common;

#ifdef CHIP_IF_SDIO
	//hook_chip.bt_conn_infra_pc = btmtk_sdio_read_conn_infra_pc_common;

#endif

	hook_chip.dl_delay_time = PATCH_DOWNLOAD_PHASE3_DELAY_TIME;
	hook_chip.support_woble = 1;
	hook_chip.patched = 0;

	btmtk_reg_hif_chip_hook(&hook_chip);

	return retval;
}

int btmtk_cif_chip_register(struct btmtk_dev *bdev)
{
	int ret = 0;
	u32 chip_id = bdev->chip_id & 0xFFFF;

	switch (chip_id) {
	case 0x7922:
		btmtk_cif_chip_7922_register();
		break;
	case 0x7902:
		btmtk_cif_chip_7902_register();
		break;
	case 0x7961:
		btmtk_cif_chip_7961_register();
		break;
	case 0x6639:
		(void)snprintf(bdev->chip_data_file_name, MAX_BIN_FILE_NAME_LEN, "bt_mt%x_data.bin",
				bdev->chip_id & 0xffff);
		ret = btmtk_load_raw_data(bdev->chip_data_file_name, bdev);
		if (ret) {
			BTMTK_ERR("%s: load raw data from %s failed", __func__, bdev->chip_data_file_name);
			goto register_end;
		}

		btmtk_cif_chip_6639_register();
		break;
	case 0x7925:
		(void)snprintf(bdev->chip_data_file_name, MAX_BIN_FILE_NAME_LEN, "bt_mt%x_data.bin",
				bdev->chip_id & 0xffff);
		ret = btmtk_load_raw_data(bdev->chip_data_file_name, bdev);
		if (ret) {
			BTMTK_ERR("%s: load raw data from %s failed", __func__, bdev->chip_data_file_name);
			goto register_end;
		}

		break;
	case 0x6631:
	case 0x6635:
	case 0x6653:
		btmtk_cif_chip_66xx_register();
		break;
	default:
		BTMTK_ERR("Unknown Mediatek device(%04X)", bdev->chip_id);
		ret = -1;
		break;
	}

register_end:
	return ret;
}

int btmtk_cif_chip_deregister(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s", __func__);

	btmtk_free_chip_data(bdev);

	BTMTK_INFO("%s: Done", __func__);
	return 0;
}
