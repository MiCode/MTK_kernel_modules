/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl_shm_mt7999.h
 */

#ifndef UNI_FW_DL_SHM_MT7999_H
#define UNI_FW_DL_SHM_MT7999_H

#define SHM_RW_BASE_ADDR			0x200B0000
#define SHM_RO_BASE_ADDR			0x200B0074

#define SHM_RW_SIZE				0x74
#define SHM_RO_SIZE				0xB8

#define MT7999_SHM_RW_SIZE			0x74
#define MT7999_SHM_RO_SIZE			0x78

#define SHM_SYNC_INFO_ADDR			0x00
#define SHM_SYNC_INFO_MASK			0xFFFFFFFF
#define SHM_SYNC_INFO_SHFT			0

#define SHM_TIMEOUT_DURATION_ADDR		0x04
#define SHM_TIMEOUT_DURATION_MASK		0xFFFFFFFF
#define SHM_TIMEOUT_DURATION_SHFT		0

#define SHM_MSG_IDX_ADDR			0x08
#define SHM_MSG_IDX_MASK			0xFFFF0000
#define SHM_MSG_IDX_SHFT			16

#define SHM_MSG_ID_ADDR				0x08
#define SHM_MSG_ID_MASK				0x0000FFFF
#define SHM_MSG_ID_SHFT				0

#define SHM_CALIBRATION_ACTION_ADDR		0x0C
#define SHM_CALIBRATION_ACTION_MASK		0x00FF0000
#define SHM_CALIBRATION_ACTION_SHFT		16

#define SHM_MSG_RADIO_TYPE_ADDR			0x0C
#define SHM_MSG_RADIO_TYPE_MASK			0x0000FFFF
#define SHM_MSG_RADIO_TYPE_SHFT			0

#define SHM_DL_ADDR_ADDR			0x10
#define SHM_DL_ADDR_MASK			0xFFFFFFFF
#define SHM_DL_ADDR_SHFT			0

#define SHM_DL_SIZE_ADDR			0x18
#define SHM_DL_SIZE_MASK			0xFFFFFFFF
#define SHM_DL_SIZE_SHFT			0

#define SHM_SLOT_TABLE_ADDR_ADDR		0x1C
#define SHM_SLOT_TABLE_ADDR_MASK		0xFFFFFFFF
#define SHM_SLOT_TABLE_ADDR_SHFT		0

#define SHM_SLOT_TABLE_SIZE_ADDR		0x24
#define SHM_SLOT_TABLE_SIZE_MASK		0xFFFFFFFF
#define SHM_SLOT_TABLE_SIZE_SHFT		0

#define SHM_SLOT_NUM_ADDR			0x28
#define SHM_SLOT_NUM_MASK			0xFFFF0000
#define SHM_SLOT_NUM_SHFT			16

#define SHM_SLOT_SIZE_ADDR			0x28
#define SHM_SLOT_SIZE_MASK			0x0000FFFF
#define SHM_SLOT_SIZE_SHFT			0

#define SHM_SLOT_RDY_IDX_ADDR			0x2C
#define SHM_SLOT_RDY_IDX_MASK			0xFFFFFFFF
#define SHM_SLOT_RDY_IDX_SHFT			0

#define SHM_NOTIF_DONE_IDX_ADDR			0x30
#define SHM_NOTIF_DONE_IDX_MASK			0xFFFF0000
#define SHM_NOTIF_DONE_IDX_SHFT			16

#define SHM_CTX_INFO_ADDR_A_ADDR		0x34
#define SHM_CTX_INFO_ADDR_A_MASK		0xFFFFFFFF
#define SHM_CTX_INFO_ADDR_A_SHFT		0

#define SHM_CTX_INFO_SIZE_A_ADDR		0x3C
#define SHM_CTX_INFO_SIZE_A_MASK		0xFFFFFFFF
#define SHM_CTX_INFO_SIZE_A_SHFT		0

#define SHM_CTX_INFO_ADDR_B_ADDR		0x40
#define SHM_CTX_INFO_ADDR_B_MASK		0xFFFFFFFF
#define SHM_CTX_INFO_ADDR_B_SHFT		0

#define SHM_CTX_INFO_SIZE_B_ADDR		0x48
#define SHM_CTX_INFO_SIZE_B_MASK		0xFFFFFFFF
#define SHM_CTX_INFO_SIZE_B_SHFT		0

#define SHM_CO_DL_RADIO_ADDR			0x58
#define SHM_CO_DL_RADIO_MASK			0x000000FF
#define SHM_CO_DL_RADIO_SHFT			0

#define SHM_CO_DL_WF_HEADER_SZ_ADDR		0x5C
#define SHM_CO_DL_WF_HEADER_SZ_MASK		0xFFFFFFFF
#define SHM_CO_DL_WF_HEADER_SZ_SHFT		0

#define SHM_CO_DL_BT_HEADER_SZ_ADDR		0x60
#define SHM_CO_DL_BT_HEADER_SZ_MASK		0xFFFFFFFF
#define SHM_CO_DL_BT_HEADER_SZ_SHFT		0

#define SHM_CO_DL_ZB_HEADER_SZ_ADDR		0x64
#define SHM_CO_DL_ZB_HEADER_SZ_MASK		0xFFFFFFFF
#define SHM_CO_DL_ZB_HEADER_SZ_SHFT		0

#define SHM_CHIP_INFO_ADDR			0x00
#define SHM_CHIP_INFO_MASK			0xFFFFFFFF
#define SHM_CHIP_INFO_SHFT			0

#define SHM_CHIP_ID_ADDR			0x04
#define SHM_CHIP_ID_MASK			0xFFFFFFFF
#define SHM_CHIP_ID_SHFT			0

#define SHM_HW_VER_ADDR				0x08
#define SHM_HW_VER_MASK				0xFFFFFFFF
#define SHM_HW_VER_SHFT				0

#define SHM_FW_VER_ADDR				0x0C
#define SHM_FW_VER_MASK				0xFFFFFFFF
#define SHM_FW_VER_SHFT				0

#define SHM_EFUSE_INFO_ADDR			0x10
#define SHM_EFUSE_INFO_SIZE			0x40

#define SHM_BOOT_STAGE_ADDR			0x8C
#define SHM_BOOT_STAGE_MASK			0xFFFFFFFF
#define SHM_BOOT_STAGE_SHFT			0

#define SHM_SLOT_DONE_IDX_ADDR			0x90
#define SHM_SLOT_DONE_IDX_MASK			0xFFFFFFFF
#define SHM_SLOT_DONE_IDX_SHFT			0

#define SHM_MSG_DONE_IDX_ADDR			0x94
#define SHM_MSG_DONE_IDX_MASK			0xFFFF0000
#define SHM_MSG_DONE_IDX_SHFT			16

#define SHM_NOTIF_IDX_ADDR			0x98
#define SHM_NOTIF_IDX_MASK			0xFFFF0000
#define SHM_NOTIF_IDX_SHFT			16

#define SHM_NOTIF_ID_ADDR			0x98
#define SHM_NOTIF_ID_MASK			0x0000FFFF
#define SHM_NOTIF_ID_SHFT			0

#define SHM_DL_RESP_ADDR			0x9C
#define SHM_DL_RESP_MASK			0xFFFFFFFF
#define SHM_DL_RESP_SHFT			0

#define SHM_DL_BLOCK_RADIO_ADDR			0xA0
#define SHM_DL_BLOCK_RADIO_MASK			0xFFFF0000
#define SHM_DL_BLOCK_RADIO_SHFT			16

#define SHM_DL_BLOCK_CODE_ADDR			0xA0
#define SHM_DL_BLOCK_CODE_MASK			0x0000FFFF
#define SHM_DL_BLOCK_CODE_SHFT			0

#define SHM_ERROR_CODE_ADDR			0xA4
#define SHM_ERROR_CODE_MASK			0xFFFF0000
#define SHM_ERROR_CODE_SHFT			16

#define SHM_SUB_ERROR_CODE_ADDR			0xA4
#define SHM_SUB_ERROR_CODE_MASK			0x0000FFFF
#define SHM_SUB_ERROR_CODE_SHFT			0

#define SHM_COMM_STATE_ADDR			0xA8
#define SHM_COMM_STATE_MASK			0xFFFFFFFF
#define SHM_COMM_STATE_SHFT			0

#define SHM_PREV_NOTIF_IDX_ADDR			0xAC
#define SHM_PREV_NOTIF_IDX_MASK			0xFFFF0000
#define SHM_PREV_NOTIF_IDX_SHFT			16

#define SHM_PREV_NOTIF_ID_ADDR			0xAC
#define SHM_PREV_NOTIF_ID_MASK			0x0000FFFF
#define SHM_PREV_NOTIF_ID_SHFT			0

uint32_t mt7999_shm_init(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx);
void mt7999_shm_deinit(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx);
void mt7999_shm_dump(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx);
void mt7999_write_sync_info(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value);
void mt7999_write_timeout_duration(struct UNI_FWLD_SHM_CTX *ctx,
				   uint32_t value);
void mt7999_write_msg_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
void mt7999_write_msg_id(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
void mt7999_write_calibration_action(struct UNI_FWLD_SHM_CTX *ctx,
				     uint8_t value);
void mt7999_write_radio_type(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
void mt7999_write_dl_addr(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value);
void mt7999_write_dl_size(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value);
void mt7999_write_slot_tbl_addr(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value);
void mt7999_write_slot_tbl_size(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value);
void mt7999_write_slot_num(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
void mt7999_write_slot_size(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
void mt7999_write_slot_rdy_idx(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value);
void mt7999_write_notif_done_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
void mt7999_write_ctx_info_A_addr(struct UNI_FWLD_SHM_CTX *ctx,
				  uint64_t value);
void mt7999_write_ctx_info_A_size(struct UNI_FWLD_SHM_CTX *ctx,
				  uint32_t value);
void mt7999_write_ctx_info_B_addr(struct UNI_FWLD_SHM_CTX *ctx,
				  uint64_t value);
void mt7999_write_ctx_info_B_size(struct UNI_FWLD_SHM_CTX *ctx,
				  uint32_t value);
void mt7999_write_co_dl_radio(struct UNI_FWLD_SHM_CTX *ctx, uint8_t value);
void mt7999_write_co_dl_sec_map_size_wf(struct UNI_FWLD_SHM_CTX *ctx,
					uint32_t value);
void mt7999_write_co_dl_sec_map_size_bt(struct UNI_FWLD_SHM_CTX *ctx,
					uint32_t value);
void mt7999_write_co_dl_sec_map_size_zb(struct UNI_FWLD_SHM_CTX *ctx,
					uint32_t value);
void mt7999_read_chip_info(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
void mt7999_read_chip_id(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
void mt7999_read_hw_ver(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
void mt7999_read_fw_ver(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
void mt7999_read_efuse_info(struct UNI_FWLD_SHM_CTX *ctx, uint8_t *buff,
			    uint32_t buff_size);
void mt7999_read_boot_stage(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
void mt7999_read_slot_done_idx(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
void mt7999_read_msg_done_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
void mt7999_read_notif_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
void mt7999_read_notif_id(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
void mt7999_read_dl_resp(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
void mt7999_read_block_radio(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
void mt7999_read_block_id(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
void mt7999_read_error_code(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
void mt7999_read_sub_error_code(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
void mt7999_read_comm_state(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);

#endif /* UNI_FW_DL_SHM_MT7999_H */
