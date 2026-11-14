/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl_hif_ops.h
 */

#ifndef _UNI_FW_DL_HIF_OPS_H
#define _UNI_FW_DL_HIF_OPS_H

static inline uint32_t
uniFwdlHifInit(struct ADAPTER *ad, UNI_FWDL_NOTIF_CB cb, uint32_t size)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return WLAN_STATUS_NOT_SUPPORTED;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->init)
		return WLAN_STATUS_NOT_SUPPORTED;

	return prHifOps->init(ad, cb, size);
}

static inline void
uniFwdlHifDeInit(struct ADAPTER *ad)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->deinit)
		return;

	prHifOps->deinit(ad);
}

static inline void
uniFwdlHifDump(struct ADAPTER *ad)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->dump)
		return;

	prHifOps->dump(ad);
}

static inline uint32_t
uniFwdlHifQueryHwInfo(struct ADAPTER *ad, uint8_t *buff,
		      uint32_t buff_size)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return WLAN_STATUS_NOT_SUPPORTED;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->query_hw_info)
		return WLAN_STATUS_NOT_SUPPORTED;

	return prHifOps->query_hw_info(ad, buff, buff_size);
}

static inline uint32_t
uniFwdlHifStartDl(struct ADAPTER *ad,
		  struct START_DL_PARAMS *params)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return WLAN_STATUS_NOT_SUPPORTED;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->start_dl)
		return WLAN_STATUS_NOT_SUPPORTED;

	return prHifOps->start_dl(ad, params);
}

static inline uint32_t
uniFwdlHifDlBlock(struct ADAPTER *ad, uint8_t *buff,
		  uint32_t buff_size)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return WLAN_STATUS_NOT_SUPPORTED;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->dl_block)
		return WLAN_STATUS_NOT_SUPPORTED;

	return prHifOps->dl_block(ad, buff, buff_size);
}

static inline uint32_t
uniFwdlHifSendMsg(struct ADAPTER *ad,
		  enum UNI_FWDL_HIF_DL_RADIO_TYPE radio,
		  uint8_t msg_id,
		  uint8_t *msg_data,
		  uint32_t msg_data_size)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return WLAN_STATUS_NOT_SUPPORTED;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->send_msg)
		return WLAN_STATUS_NOT_SUPPORTED;

	return prHifOps->send_msg(ad, radio, msg_id, msg_data, msg_data_size);
}

static inline void
uniFwdlHifRcvInterrupt(struct ADAPTER *ad)
{
	struct UNI_FWDL_HIF_OPS *prHifOps;

	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info)
		return;

	prHifOps = &ad->chip_info->uni_fwdl_info->rHifOps;
	if (!prHifOps->rcv_irq)
		return;

	return prHifOps->rcv_irq(ad);
}
#endif /* _UNI_FW_DL_HIF_OPS_H */
