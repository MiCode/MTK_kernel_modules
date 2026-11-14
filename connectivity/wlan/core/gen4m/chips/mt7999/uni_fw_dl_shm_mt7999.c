// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"
#include "uni_fw_dl_shm_mt7999.h"


#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
#define SHM_RD_FIELD_EMI(__ctx, __offset, __mask, _shift, __val)	\
do {									\
	uint32_t __v = 0;						\
	kalMemCopyFromIo(&__v, (__ctx)->rRoMem.va + (__offset),		\
			 sizeof(__v));					\
	__v &= (__mask);						\
	__v = (__v >> (_shift));					\
	kalMemCopy(__val, &__v, sizeof(__v));				\
} while (0)
#define SHM_WR_FIELD_EMI(__ctx, __offset, __mask, _shift, __val)	\
do {									\
	uint32_t __v = 0;						\
	kalMemCopyFromIo(&__v, (__ctx)->rRwMem.va + (__offset),		\
			 sizeof(__v));					\
	__v &= ~(__mask);						\
	__v |= (((__val) << (_shift)) & (__mask));			\
	kalMemCopyToIo((__ctx)->rRwMem.va, &__v, sizeof(__v));		\
} while (0)
#else
#define SHM_RD_FIELD_EMI
#define SHM_WR_FIELD_EMI
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */

#define SHM_RD_FIELD_MMIO(__ctx, __offset, __mask, _shift, __val)	\
do {									\
	uint32_t __v = 0;						\
	HAL_RMCR_RD(ONOFF_READ, (__ctx)->ad,				\
		    ((__offset) + SHM_RO_BASE_ADDR), &__v);		\
	__v &= (__mask);						\
	__v = (__v >> (_shift));					\
	kalMemCopy(__val, &__v, sizeof(__v));				\
} while (0)
#define SHM_WR_FIELD_MMIO(__ctx, __offset, __mask, _shift, __val)	\
do {									\
	uint32_t __v = 0;						\
	HAL_RMCR_RD(ONOFF_READ, (__ctx)->ad,				\
		    ((__offset) + SHM_RW_BASE_ADDR), &__v);		\
	__v &= ~(__mask);						\
	__v |= (((__val) << (_shift)) & (__mask));			\
	HAL_MCR_WR((__ctx)->ad, ((__offset) + SHM_RW_BASE_ADDR), __v);	\
} while (0)

#define SHM_RD_FIELD(__ctx, __offset, __mask, _shift, __val)		\
do {									\
	if ((__ctx)->fgEmiMode) {					\
		SHM_RD_FIELD_EMI(__ctx, __offset, __mask, _shift,	\
				 __val);				\
	} else {							\
		SHM_RD_FIELD_MMIO(__ctx, __offset, __mask, _shift,	\
				  __val);				\
	}								\
} while (0)
#define SHM_WR_FIELD(__ctx, __offset, __mask, _shift, __val)		\
do {									\
	if ((__ctx)->fgEmiMode) {					\
		SHM_WR_FIELD_EMI(__ctx, __offset, __mask, _shift,	\
				 __val);				\
	} else {							\
		SHM_WR_FIELD_MMIO(__ctx, __offset, __mask, _shift,	\
				  __val);				\
	}								\
} while (0)

uint32_t mt7999_shm_init(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx)
{
#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
	struct GL_HIF_INFO *prHif = &ad->prGlueInfo->rHifInfo;
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */
	uint32_t u4Status = WLAN_STATUS_FAILURE;

#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
	ctx->rRwMem.align_size = MT7999_SHM_RW_SIZE;
	ctx->rRwMem.va =
		KAL_DMA_ALLOC_COHERENT(prHif->pdev,
				       ctx->rRwMem.align_size,
				       &ctx->rRwMem.pa);
	if (!ctx->rRwMem.va) {
		DBGLOG(UNI_FWDL, ERROR,
			"Alloc RW mem failed, size=%u\n",
			ctx->rRwMem.align_size);
		goto error;
	}

	ctx->rRoMem.align_size = MT7999_SHM_RO_SIZE;
	ctx->rRoMem.va =
		KAL_DMA_ALLOC_COHERENT(prHif->pdev,
				       ctx->rRoMem.align_size,
				       &ctx->rRoMem.pa);
	if (!ctx->rRoMem.va) {
		DBGLOG(UNI_FWDL, ERROR,
			"Alloc RO mem failed, size=%u\n",
			ctx->rRoMem.align_size);
		goto error;
	}
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */

	u4Status = WLAN_STATUS_SUCCESS;
	goto exit;

#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
error:
	if (ctx->rRoMem.va)
		KAL_DMA_FREE_COHERENT(prHif->pdev,
				      ctx->rRoMem.align_size,
				      ctx->rRoMem.va,
				      ctx->rRoMem.pa);
	if (ctx->rRwMem.va)
		KAL_DMA_FREE_COHERENT(prHif->pdev,
				      ctx->rRwMem.align_size,
				      ctx->rRwMem.va,
				      ctx->rRwMem.pa);
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */

exit:
	return u4Status;
}

void mt7999_shm_deinit(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx)
{
#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
	struct GL_HIF_INFO *prHif = &ad->prGlueInfo->rHifInfo;
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */

#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
	KAL_DMA_FREE_COHERENT(prHif->pdev,
			      ctx->rRoMem.align_size,
			      ctx->rRoMem.va,
			      ctx->rRoMem.pa);
	KAL_DMA_FREE_COHERENT(prHif->pdev,
			      ctx->rRwMem.align_size,
			      ctx->rRwMem.va,
			      ctx->rRwMem.pa);
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */
}

void mt7999_shm_dump(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx)
{
	if (ctx->fgEmiMode) {
#if (CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM == 1)
		DBGLOG(UNI_FWDL, INFO, "Dump RW mem\n");
		DBGLOG_MEM8(UNI_FWDL, INFO, ctx->rRwMem.va,
			    ctx->rRwMem.align_size);

		DBGLOG(UNI_FWDL, INFO, "Dump RO mem\n");
		DBGLOG_MEM8(UNI_FWDL, INFO, ctx->rRoMem.va,
			    ctx->rRoMem.align_size);
#endif /* CFG_SUPPORT_UNI_FWDL_EMI_SHARED_MEM */
	} else {
		uint8_t *prDbgBuffer = NULL;
		u_int8_t fgRet;

		prDbgBuffer = kalMemZAlloc(SHM_RW_SIZE, VIR_MEM_TYPE);
		if (prDbgBuffer) {
			HAL_RMCR_RD_RANGE(ONOFF_DBG,
					  ad,
					  SHM_RW_BASE_ADDR,
					  prDbgBuffer,
					  SHM_RW_SIZE,
					  fgRet);

			DBGLOG(UNI_FWDL, INFO, "Dump RW mem\n");
			DBGLOG_MEM8(UNI_FWDL, INFO, prDbgBuffer, SHM_RW_SIZE);

			prDbgBuffer = NULL;
			kalMemFree(prDbgBuffer, VIR_MEM_TYPE, SHM_RW_SIZE);
		}

		prDbgBuffer = kalMemZAlloc(SHM_RO_SIZE, VIR_MEM_TYPE);
		if (prDbgBuffer) {
			HAL_RMCR_RD_RANGE(ONOFF_DBG,
					  ad,
					  SHM_RO_BASE_ADDR,
					  prDbgBuffer,
					  SHM_RO_SIZE,
					  fgRet);

			DBGLOG(UNI_FWDL, INFO, "Dump RO mem\n");
			DBGLOG_MEM8(UNI_FWDL, INFO, prDbgBuffer, SHM_RO_SIZE);

			prDbgBuffer = NULL;
			kalMemFree(prDbgBuffer, VIR_MEM_TYPE, SHM_RO_SIZE);
		}
	}
}

void mt7999_write_sync_info(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_SYNC_INFO_ADDR,
		     SHM_SYNC_INFO_MASK,
		     SHM_SYNC_INFO_SHFT,
		     value);
}

void mt7999_write_timeout_duration(struct UNI_FWLD_SHM_CTX *ctx,
				   uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_TIMEOUT_DURATION_ADDR,
		     SHM_TIMEOUT_DURATION_MASK,
		     SHM_TIMEOUT_DURATION_SHFT,
		     value);
}

void mt7999_write_msg_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_MSG_IDX_ADDR,
		     SHM_MSG_IDX_MASK,
		     SHM_MSG_IDX_SHFT,
		     value);
}

void mt7999_write_msg_id(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_MSG_ID_ADDR,
		     SHM_MSG_ID_MASK,
		     SHM_MSG_ID_SHFT,
		     value);
}

void mt7999_write_calibration_action(struct UNI_FWLD_SHM_CTX *ctx,
				     uint8_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_CALIBRATION_ACTION_ADDR,
		     SHM_CALIBRATION_ACTION_MASK,
		     SHM_CALIBRATION_ACTION_SHFT,
		     value);
}

void mt7999_write_radio_type(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_MSG_RADIO_TYPE_ADDR,
		     SHM_MSG_RADIO_TYPE_MASK,
		     SHM_MSG_RADIO_TYPE_SHFT,
		     value);
}

void mt7999_write_dl_addr(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value)
{
	uint32_t addr;

	addr = (value >> DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
	SHM_WR_FIELD(ctx,
		     SHM_DL_ADDR_ADDR + 0x4,
		     SHM_DL_ADDR_MASK,
		     SHM_DL_ADDR_SHFT,
		     addr);

	addr = (value & DMA_LOWER_32BITS_MASK);
	SHM_WR_FIELD(ctx,
		     SHM_DL_ADDR_ADDR,
		     SHM_DL_ADDR_MASK,
		     SHM_DL_ADDR_SHFT,
		     addr);
}

void mt7999_write_dl_size(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_DL_SIZE_ADDR,
		     SHM_DL_SIZE_MASK,
		     SHM_DL_SIZE_SHFT,
		     value);
}

void mt7999_write_slot_tbl_addr(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value)
{
	uint32_t addr;

	addr = (value >> DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
	SHM_WR_FIELD(ctx,
		     SHM_SLOT_TABLE_ADDR_ADDR + 0x4,
		     SHM_SLOT_TABLE_ADDR_MASK,
		     SHM_SLOT_TABLE_ADDR_SHFT,
		     addr);

	addr = (value & DMA_LOWER_32BITS_MASK);
	SHM_WR_FIELD(ctx,
		     SHM_SLOT_TABLE_ADDR_ADDR,
		     SHM_SLOT_TABLE_ADDR_MASK,
		     SHM_SLOT_TABLE_ADDR_SHFT,
		     addr);
}

void mt7999_write_slot_tbl_size(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_SLOT_TABLE_SIZE_ADDR,
		     SHM_SLOT_TABLE_SIZE_MASK,
		     SHM_SLOT_TABLE_SIZE_SHFT,
		     value);
}

void mt7999_write_slot_num(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_SLOT_NUM_ADDR,
		     SHM_SLOT_NUM_MASK,
		     SHM_SLOT_NUM_SHFT,
		     value);
}

void mt7999_write_slot_size(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_SLOT_SIZE_ADDR,
		     SHM_SLOT_SIZE_MASK,
		     SHM_SLOT_SIZE_SHFT,
		     value);
}

void mt7999_write_slot_rdy_idx(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_SLOT_RDY_IDX_ADDR,
		     SHM_SLOT_RDY_IDX_MASK,
		     SHM_SLOT_RDY_IDX_SHFT,
		     value);
}

void mt7999_write_notif_done_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_NOTIF_DONE_IDX_ADDR,
		     SHM_NOTIF_DONE_IDX_MASK,
		     SHM_NOTIF_DONE_IDX_SHFT,
		     value);
}

void mt7999_write_ctx_info_A_addr(struct UNI_FWLD_SHM_CTX *ctx,
				  uint64_t value)
{
	uint32_t addr;

	addr = (value >> DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
	SHM_WR_FIELD(ctx,
		     SHM_CTX_INFO_ADDR_A_ADDR + 0x4,
		     SHM_CTX_INFO_ADDR_A_MASK,
		     SHM_CTX_INFO_ADDR_A_SHFT,
		     addr);

	addr = (value & DMA_LOWER_32BITS_MASK);
	SHM_WR_FIELD(ctx,
		     SHM_CTX_INFO_ADDR_A_ADDR,
		     SHM_CTX_INFO_ADDR_A_MASK,
		     SHM_CTX_INFO_ADDR_A_SHFT,
		     addr);
}

void mt7999_write_ctx_info_A_size(struct UNI_FWLD_SHM_CTX *ctx,
				  uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_CTX_INFO_SIZE_A_ADDR,
		     SHM_CTX_INFO_SIZE_A_MASK,
		     SHM_CTX_INFO_SIZE_A_SHFT,
		     value);
}

void mt7999_write_ctx_info_B_addr(struct UNI_FWLD_SHM_CTX *ctx,
				  uint64_t value)
{
	uint32_t addr;

	addr = (value >> DMA_BITS_OFFSET) & DMA_HIGHER_4BITS_MASK;
	SHM_WR_FIELD(ctx,
		     SHM_CTX_INFO_ADDR_B_ADDR + 0x4,
		     SHM_CTX_INFO_ADDR_B_MASK,
		     SHM_CTX_INFO_ADDR_B_SHFT,
		     addr);

	addr = (value & DMA_LOWER_32BITS_MASK);
	SHM_WR_FIELD(ctx,
		     SHM_CTX_INFO_ADDR_B_ADDR,
		     SHM_CTX_INFO_ADDR_B_MASK,
		     SHM_CTX_INFO_ADDR_B_SHFT,
		     addr);
}

void mt7999_write_ctx_info_B_size(struct UNI_FWLD_SHM_CTX *ctx,
				  uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_CTX_INFO_SIZE_B_ADDR,
		     SHM_CTX_INFO_SIZE_B_MASK,
		     SHM_CTX_INFO_SIZE_B_SHFT,
		     value);
}

void mt7999_write_co_dl_radio(struct UNI_FWLD_SHM_CTX *ctx, uint8_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_CO_DL_RADIO_ADDR,
		     SHM_CO_DL_RADIO_MASK,
		     SHM_CO_DL_RADIO_SHFT,
		     value);
}

void mt7999_write_co_dl_sec_map_size_wf(struct UNI_FWLD_SHM_CTX *ctx,
					uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_CO_DL_WF_HEADER_SZ_ADDR,
		     SHM_CO_DL_WF_HEADER_SZ_MASK,
		     SHM_CO_DL_WF_HEADER_SZ_SHFT,
		     value);
}

void mt7999_write_co_dl_sec_map_size_bt(struct UNI_FWLD_SHM_CTX *ctx,
					uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_CO_DL_BT_HEADER_SZ_ADDR,
		     SHM_CO_DL_BT_HEADER_SZ_MASK,
		     SHM_CO_DL_BT_HEADER_SZ_SHFT,
		     value);
}

void mt7999_write_co_dl_sec_map_size_zb(struct UNI_FWLD_SHM_CTX *ctx,
					uint32_t value)
{
	SHM_WR_FIELD(ctx,
		     SHM_CO_DL_ZB_HEADER_SZ_ADDR,
		     SHM_CO_DL_ZB_HEADER_SZ_MASK,
		     SHM_CO_DL_ZB_HEADER_SZ_SHFT,
		     value);
}

void mt7999_read_chip_info(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_CHIP_INFO_ADDR,
		     SHM_CHIP_INFO_MASK,
		     SHM_CHIP_INFO_SHFT,
		     value);
}

void mt7999_read_chip_id(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_CHIP_ID_ADDR,
		     SHM_CHIP_ID_MASK,
		     SHM_CHIP_ID_SHFT,
		     value);
}

void mt7999_read_hw_ver(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_HW_VER_ADDR,
		     SHM_HW_VER_MASK,
		     SHM_HW_VER_SHFT,
		     value);
}

void mt7999_read_fw_ver(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_FW_VER_ADDR,
		     SHM_FW_VER_MASK,
		     SHM_FW_VER_SHFT,
		     value);
}

void mt7999_read_efuse_info(struct UNI_FWLD_SHM_CTX *ctx, uint8_t *buff,
			    uint32_t buff_size)
{
	uint32_t base, size, read = 0;

	base = SHM_EFUSE_INFO_ADDR;
	size = kal_min_t(uint32_t,
			 ALIGN_4(buff_size),
			 ALIGN_4(SHM_EFUSE_INFO_SIZE));
	while (read < size) {
		SHM_RD_FIELD(ctx, base, 0xFFFFFFFF, 0, (buff + read));

		base + 4;
		read += 4;
	}
}

void mt7999_read_boot_stage(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_BOOT_STAGE_ADDR,
		     SHM_BOOT_STAGE_MASK,
		     SHM_BOOT_STAGE_SHFT,
		     value);
}

void mt7999_read_slot_done_idx(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_SLOT_DONE_IDX_ADDR,
		     SHM_SLOT_DONE_IDX_MASK,
		     SHM_SLOT_DONE_IDX_SHFT,
		     value);
}

void mt7999_read_msg_done_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_MSG_DONE_IDX_ADDR,
		     SHM_MSG_DONE_IDX_MASK,
		     SHM_MSG_DONE_IDX_SHFT,
		     value);
}

void mt7999_read_notif_idx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_NOTIF_IDX_ADDR,
		     SHM_NOTIF_IDX_MASK,
		     SHM_NOTIF_IDX_SHFT,
		     value);
}

void mt7999_read_notif_id(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_NOTIF_ID_ADDR,
		     SHM_NOTIF_ID_MASK,
		     SHM_NOTIF_ID_SHFT,
		     value);
}

void mt7999_read_dl_resp(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_DL_RESP_ADDR,
		     SHM_DL_RESP_MASK,
		     SHM_DL_RESP_SHFT,
		     value);
}

void mt7999_read_block_radio(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_DL_BLOCK_RADIO_ADDR,
		     SHM_DL_BLOCK_RADIO_MASK,
		     SHM_DL_BLOCK_RADIO_SHFT,
		     value);
}

void mt7999_read_block_id(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_DL_BLOCK_CODE_ADDR,
		     SHM_DL_BLOCK_CODE_MASK,
		     SHM_DL_BLOCK_CODE_SHFT,
		     value);
}

void mt7999_read_error_code(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_ERROR_CODE_ADDR,
		     SHM_ERROR_CODE_MASK,
		     SHM_ERROR_CODE_SHFT,
		     value);
}

void mt7999_read_sub_error_code(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_SUB_ERROR_CODE_ADDR,
		     SHM_SUB_ERROR_CODE_MASK,
		     SHM_SUB_ERROR_CODE_SHFT,
		     value);
}

void mt7999_read_comm_state(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	SHM_RD_FIELD(ctx,
		     SHM_COMM_STATE_ADDR,
		     SHM_COMM_STATE_MASK,
		     SHM_COMM_STATE_SHFT,
		     value);
}
