/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl_shm.h
 */

#ifndef _UNI_FW_DL_SHM_H
#define _UNI_FW_DL_SHM_H


struct UNI_FWLD_SHM_CTX {
	struct ADAPTER *ad;
	u_int8_t fgEmiMode;
	struct HIF_MEM rRwMem;
	struct HIF_MEM rRoMem;
};

struct UNI_FWDL_SHM_OPS {
	uint32_t (*init)(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx);
	void (*deinit)(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx);
	void (*dump)(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx);

	void (*write_sync_info)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value);
	void (*write_timeout_duration)(struct UNI_FWLD_SHM_CTX *ctx,
				       uint32_t value);
	void (*write_msg_idx)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
	void (*write_msg_id)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
	void (*write_calibration_action)(struct UNI_FWLD_SHM_CTX *ctx,
					 uint8_t value);
	void (*write_radio_type)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
	void (*write_dl_addr)(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value);
	void (*write_dl_size)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value);
	void (*write_slot_tbl_addr)(struct UNI_FWLD_SHM_CTX *ctx,
				    uint64_t value);
	void (*write_slot_tbl_size)(struct UNI_FWLD_SHM_CTX *ctx,
				    uint32_t value);
	void (*write_slot_num)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
	void (*write_slot_size)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value);
	void (*write_slot_rdy_idx)(struct UNI_FWLD_SHM_CTX *ctx,
				   uint32_t value);
	void (*write_notif_done_idx)(struct UNI_FWLD_SHM_CTX *ctx,
				     uint16_t value);
	void (*write_ctx_info_A_addr)(struct UNI_FWLD_SHM_CTX *ctx,
				      uint64_t value);
	void (*write_ctx_info_A_size)(struct UNI_FWLD_SHM_CTX *ctx,
				      uint32_t value);
	void (*write_ctx_info_B_addr)(struct UNI_FWLD_SHM_CTX *ctx,
				      uint64_t value);
	void (*write_ctx_info_B_size)(struct UNI_FWLD_SHM_CTX *ctx,
				      uint32_t value);
	void (*write_co_dl_radio)(struct UNI_FWLD_SHM_CTX *ctx, uint8_t value);
	void (*write_co_dl_sec_map_size_wf)(struct UNI_FWLD_SHM_CTX *ctx,
					    uint32_t value);
	void (*write_co_dl_sec_map_size_bt)(struct UNI_FWLD_SHM_CTX *ctx,
					    uint32_t value);
	void (*write_co_dl_sec_map_size_zb)(struct UNI_FWLD_SHM_CTX *ctx,
					    uint32_t value);

	void (*read_chip_info)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
	void (*read_chip_id)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
	void (*read_hw_ver)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
	void (*read_fw_ver)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
	void (*read_efuse_info)(struct UNI_FWLD_SHM_CTX *ctx, uint8_t *buff,
				uint32_t buff_size);
	void (*read_boot_stage)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
	void (*read_slot_done_idx)(struct UNI_FWLD_SHM_CTX *ctx,
				   uint32_t *value);
	void (*read_msg_done_idx)(struct UNI_FWLD_SHM_CTX *ctx,
				  uint16_t *value);
	void (*read_notif_idx)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
	void (*read_notif_id)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
	void (*read_dl_resp)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
	void (*read_block_radio)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
	void (*read_block_id)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
	void (*read_error_code)(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value);
	void (*read_sub_error_code)(struct UNI_FWLD_SHM_CTX *ctx,
				    uint16_t *value);
	void (*read_comm_state)(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value);
};
#endif /* _UNI_FW_DL_SHM_H */
