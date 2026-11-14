/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl_shm_ops.h
 */

#ifndef _UNI_FW_DL_SHM_OPS_H
#define _UNI_FW_DL_SHM_OPS_H


static void
uniFwdlShmWrSyncInfo(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_sync_info)
		return;

	prShmOps->write_sync_info(ctx, value);
}

static void
uniFwdlShmWrTODuration(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_timeout_duration)
		return;

	prShmOps->write_timeout_duration(ctx, value);
}

static void
uniFwdlShmWrMsgIdx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_msg_idx)
		return;

	prShmOps->write_msg_idx(ctx, value);
}

static void
uniFwdlShmWrMsgId(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_msg_id)
		return;

	prShmOps->write_msg_id(ctx, value);
}

static void
uniFwdlShmWrCalibrationAction(struct UNI_FWLD_SHM_CTX *ctx, uint8_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_calibration_action)
		return;

	prShmOps->write_calibration_action(ctx, value);
}

static void
uniFwdlShmWrRadioType(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_radio_type)
		return;

	prShmOps->write_radio_type(ctx, value);
}

static void
uniFwdlShmWrDlAddr(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_dl_addr)
		return;

	prShmOps->write_dl_addr(ctx, value);
}

static void
uniFwdlShmWrDlSize(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_dl_size)
		return;

	prShmOps->write_dl_size(ctx, value);
}

static void
uniFwdlShmWrSecSlotTblAddr(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_slot_tbl_addr)
		return;

	prShmOps->write_slot_tbl_addr(ctx, value);
}

static void
uniFwdlShmWrSecSlotTblSize(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_slot_tbl_size)
		return;

	prShmOps->write_slot_tbl_size(ctx, value);
}

static void
uniFwdlShmWrSlotNum(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_slot_num)
		return;

	prShmOps->write_slot_num(ctx, value);
}

static void
uniFwdlShmWrSlotSize(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_slot_size)
		return;

	prShmOps->write_slot_size(ctx, value);
}

static void
uniFwdlShmWrSlotRdyIdx(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_slot_rdy_idx)
		return;

	prShmOps->write_slot_rdy_idx(ctx, value);
}

static void
uniFwdlShmWrNotifDoneIdx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_notif_done_idx)
		return;

	prShmOps->write_notif_done_idx(ctx, value);
}

static void
uniFwdlShmWrCtxInfoAddr_A(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_ctx_info_A_addr)
		return;

	prShmOps->write_ctx_info_A_addr(ctx, value);
}

static void
uniFwdlShmWrCtxInfoSize_A(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_ctx_info_A_size)
		return;

	prShmOps->write_ctx_info_A_size(ctx, value);
}

static void
uniFwdlShmWrCtxInfoAddr_B(struct UNI_FWLD_SHM_CTX *ctx, uint64_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_ctx_info_B_addr)
		return;

	prShmOps->write_ctx_info_B_addr(ctx, value);
}

static void
uniFwdlShmWrCtxInfoSize_B(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_ctx_info_B_size)
		return;

	prShmOps->write_ctx_info_B_size(ctx, value);
}

static void
uniFwdlShmWrCoDlRadio(struct UNI_FWLD_SHM_CTX *ctx, uint8_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_co_dl_radio)
		return;

	prShmOps->write_co_dl_radio(ctx, value);
}

static void
uniFwdlShmWrCoDlSecMapSize_WF(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_co_dl_sec_map_size_wf)
		return;

	prShmOps->write_co_dl_sec_map_size_wf(ctx, value);
}

static void
uniFwdlShmWrCoDlSecMapSize_BT(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_co_dl_sec_map_size_bt)
		return;

	prShmOps->write_co_dl_sec_map_size_bt(ctx, value);
}

static void
uniFwdlShmWrCoDlSecMapSize_ZB(struct UNI_FWLD_SHM_CTX *ctx, uint32_t value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->write_co_dl_sec_map_size_zb)
		return;

	prShmOps->write_co_dl_sec_map_size_zb(ctx, value);
}

static void
uniFwdlShmRdChipInfo(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_chip_info)
		return;

	prShmOps->read_chip_info(ctx, value);
}

static void
uniFwdlShmRdChipId(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_chip_id)
		return;

	prShmOps->read_chip_id(ctx, value);
}

static void
uniFwdlShmRdHwVer(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_hw_ver)
		return;

	prShmOps->read_hw_ver(ctx, value);
}

static void
uniFwdlShmRdFwVer(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_fw_ver)
		return;

	prShmOps->read_fw_ver(ctx, value);
}

static void
uniFwdlShmRdEfuseInfo(struct UNI_FWLD_SHM_CTX *ctx, uint8_t *buff,
		      uint32_t buff_size)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_efuse_info)
		return;

	prShmOps->read_efuse_info(ctx, buff, buff_size);
}

static void
uniFwdlShmRdBootStage(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_boot_stage)
		return;

	prShmOps->read_boot_stage(ctx, value);
}

static void
uniFwdlShmRdSlotDoneIdx(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_slot_done_idx)
		return;

	prShmOps->read_slot_done_idx(ctx, value);
}

static void
uniFwdlShmRdMsgDoneIdx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_msg_done_idx)
		return;

	prShmOps->read_msg_done_idx(ctx, value);
}

static void
uniFwdlShmRdNotifIdx(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_notif_idx)
		return;

	prShmOps->read_notif_idx(ctx, value);
}

static void
uniFwdlShmRdNotifId(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_notif_id)
		return;

	prShmOps->read_notif_id(ctx, value);
}

static void
uniFwdlShmRdDlResp(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_dl_resp)
		return;

	prShmOps->read_dl_resp(ctx, value);
}

static void
uniFwdlShmRdBlockRadio(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_block_radio)
		return;

	prShmOps->read_block_radio(ctx, value);
}

static void
uniFwdlShmRdBlockId(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_block_id)
		return;

	prShmOps->read_block_id(ctx, value);
}

static void
uniFwdlShmRdErrorCode(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_error_code)
		return;

	prShmOps->read_error_code(ctx, value);
}

static void
uniFwdlShmRdSubErrorCode(struct UNI_FWLD_SHM_CTX *ctx, uint16_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_sub_error_code)
		return;

	prShmOps->read_sub_error_code(ctx, value);
}

static void
uniFwdlShmRdCommState(struct UNI_FWLD_SHM_CTX *ctx, uint32_t *value)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ctx || !ctx->ad || !ctx->ad->chip_info ||
	    !ctx->ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ctx->ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->read_comm_state)
		return;

	prShmOps->read_comm_state(ctx, value);
}

static void
uniFwdlShmChangeEmiMode(struct UNI_FWLD_SHM_CTX *ctx)
{
	if (!ctx)
		return;

	DBGLOG(UNI_FWDL, INFO, "SHM change to EMI mode.\n");

	ctx->fgEmiMode = TRUE;
}

static void
uniFwdlShmDump(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->dump)
		return;

	prShmOps->dump(ad, ctx);
}

static uint32_t
uniFwdlShmInit(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	ctx->ad = ad;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return WLAN_STATUS_NOT_SUPPORTED;

	prShmOps = &ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->init)
		return WLAN_STATUS_NOT_SUPPORTED;

	return prShmOps->init(ad, ctx);
}

static void
uniFwdlShmDeInit(struct ADAPTER *ad, struct UNI_FWLD_SHM_CTX *ctx)
{
	struct UNI_FWDL_SHM_OPS *prShmOps;

	ctx->ad = NULL;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return;

	prShmOps = &ad->chip_info->uni_fwdl_info->rShmOps;
	if (!prShmOps->deinit)
		return;

	prShmOps->deinit(ad, ctx);
}
#endif /* _UNI_FW_DL_SHM_OPS_H */
