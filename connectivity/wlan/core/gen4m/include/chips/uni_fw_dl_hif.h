/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl_hif.h
 */

#ifndef _UNI_FW_DL_HIF_H
#define _UNI_FW_DL_HIF_H

enum UNI_FWDL_MSG_ID {
	UNI_FWDL_MSG_ID_START_DL = 0,
	UNI_FWDL_MSG_ID_QUERY_HW_INFO,
	UNI_FWDL_MSG_ID_RESUME_DL,

	UNI_FWDL_MSG_ID_SET_SKU_CONFIG = 7,

	/* PCIE: 16 ~ 31 */
	UNI_FWDL_MSG_ID_SLOT_IDX_UPDATE = 16,
	UNI_FWDL_MSG_ID_HOST_EMI_ADDR_UPDATE,

	/* UART: 31 ~ 47 */
	/* USB: 48 ~ 63 */
	UNI_FWDL_MSG_ID_NUM,
};

enum UNI_FWDL_NOTIF_ID {
	UNI_FWDL_NOTIF_ID_QUERY_HW_INFO = 0,
	UNI_FWDL_NOTIF_ID_BROM_PATCH_DONE,
	UNI_FWDL_NOTIF_ID_DL_DONE,
	UNI_FWDL_NOTIF_ID_FWDL_RESP,
	UNI_FWDL_NOTIF_ID_BOOT_STAGE,

	UNI_FWDL_NOTIF_ID_SKU_CONFIG_RESP = 7,

	/* PCIE: 16 ~ 31 */
	UNI_FWDL_NOTIF_ID_SLOT_DONE_IDX = 16,
	UNI_FWDL_NOTIF_ID_EMI_ADDR_UPDATE,

	/* UART: 31 ~ 47 */
	/* USB: 48 ~ 63 */
	UNI_FWDL_NOTIF_ID_NUM,
};

enum UNI_FWDL_RESP_STATUS {
	UNI_FWDL_RESP_STATUS_DEFAULT,
	UNI_FWDL_RESP_STATUS_SUCCESS,
	UNI_FWDL_RESP_STATUS_FAIL,
	UNI_FWDL_RESP_STATUS_REQ_DL,
};

enum UNI_FWDL_COMM_STATE {
	UNI_FWDL_COMM_STATE_MSG_NOTIF,
	UNI_FWDL_COMM_STATE_SLOT_DL,
	UNI_FWDL_COMM_STATE_FEATURE_INIT,
};

enum BOOT_STAGE {
	BOOT_STAGE_ROM,
	BOOT_STAGE_SECOND_BOOT_IMAGE,
	BOOT_STAGE_OS,
	BOOT_STAGE_ABORT_HANDLER,
	BOOT_STAGE_COREDUMP,
	BOOT_STAGE_TRAPPED,
};

enum UNI_FWDL_NOTIF_HW_INFO_TAG {
	UNI_FWDL_NOTIF_HW_INFO_TAG_CHIP_ID,
	UNI_FWDL_NOTIF_HW_INFO_TAG_HW_VER,
	UNI_FWDL_NOTIF_HW_INFO_TAG_FW_VER,
	UNI_FWDL_NOTIF_HW_INFO_TAG_NUM,
};

struct UNI_FWDL_MSG_HOST_EMI_ADDR_UPDATE {
	uint64_t u8RwShmAddr;
	uint32_t u4RwShmSize;
	uint64_t u8RoShmAddr;
	uint32_t u4RoShmSize;
};

struct UNI_FWDL_NOTIF_HW_INFO_ENTRY {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucData[];
};

struct UNI_FWDL_NOTIF_HW_INFO {
	uint32_t u4EntriesNum;
	struct UNI_FWDL_NOTIF_HW_INFO_ENTRY aucEntries[];
};

enum UNI_FWDL_HIF_DL_RADIO_TYPE {
	UNI_FWDL_HIF_DL_RADIO_TYPE_RESERVED,
	UNI_FWDL_HIF_DL_RADIO_TYPE_WF,
	UNI_FWDL_HIF_DL_RADIO_TYPE_BT,
	UNI_FWDL_HIF_DL_RADIO_TYPE_ZB,
};

struct UNI_FWDL_NOTIF_NROM_PATCH_DONE {
	enum UNI_FWDL_HIF_DL_RADIO_TYPE eRadioType;
};

struct UNI_FWDL_NOTIF_DL_DONE {
	enum UNI_FWDL_HIF_DL_RADIO_TYPE eRadioType;
};

struct UNI_FWDL_NOTIF_FWDL_RESP {
	enum UNI_FWDL_HIF_DL_RADIO_TYPE eDlRadio;
	enum UNI_FWDL_RESP_STATUS eResp;
	uint16_t u2DlBlockId;
	uint16_t u2ErrorCode;
	enum UNI_FWDL_COMM_STATE eCommState;
};

struct UNI_FWDL_NOTIF_BOOT_STAGE {
	enum BOOT_STAGE eBootStage;
};

enum UNI_FWDL_HIF_CO_DL_RADIO {
	UNI_FWDL_HIF_CO_DL_RADIO_CBMCU,
	UNI_FWDL_HIF_CO_DL_RADIO_WF,
	UNI_FWDL_HIF_CO_DL_RADIO_BT,
	UNI_FWDL_HIF_CO_DL_RADIO_ZB,
	UNI_FWDL_HIF_CO_DL_RADIO_NUM,
};

enum UNI_FWDL_HIF_CALIBRATION_ACTION {
	UNI_FWDL_HIF_CALIBRATION_ACTION_NONE,
	UNI_FWDL_HIF_CALIBRATION_ACTION_CONTINUE_DL,
	UNI_FWDL_HIF_CALIBRATION_ACTION_ABORT_DL,
	UNI_FWDL_HIF_CALIBRATION_ACTION_SKIP_K_ABORT_DL,
	UNI_FWDL_HIF_CALIBRATION_ACTION_SKIP_K_CONTINUE_DL,
	UNI_FWDL_HIF_CALIBRATION_ACTION_NUM,
};

struct START_DL_PARAMS {
	enum UNI_FWDL_HIF_DL_RADIO_TYPE eRadio;
	uint8_t *prFwBuffer;
	uint32_t u4FwSize;
	uint32_t u4HeaderSz;
	uint32_t u4SyncInfo;
	uint8_t eCoDlRadioBitmap;
	uint32_t u4CoDlWfHeaderSz;
	uint32_t u4CoDlBtHeaderSz;
	uint32_t u4CoDlZbHeaderSz;
	enum UNI_FWDL_HIF_CALIBRATION_ACTION eAction;
	uint32_t u4FwTimeoutDuration;
};

typedef void(*UNI_FWDL_NOTIF_CB) (struct ADAPTER *prAdapter,
				  enum UNI_FWDL_NOTIF_ID eNotifId,
				  void *prBuf, uint32_t u4Size);

struct UNI_FWDL_HIF_OPS {
	uint32_t (*init)(struct ADAPTER *ad, UNI_FWDL_NOTIF_CB cb,
			 uint32_t size);
	void (*deinit)(struct ADAPTER *ad);
	void (*dump)(struct ADAPTER *ad);
	uint32_t (*query_hw_info)(struct ADAPTER *ad, uint8_t *buff,
				  uint32_t buff_size);
	uint32_t (*start_dl)(struct ADAPTER *ad,
			     struct START_DL_PARAMS *params);
	uint32_t (*dl_block)(struct ADAPTER *ad, uint8_t *buff,
			     uint32_t buff_size);
	uint32_t (*send_msg)(struct ADAPTER *ad,
			     enum UNI_FWDL_HIF_DL_RADIO_TYPE radio,
			     uint8_t msg_id,
			     uint8_t *msg_data,
			     uint32_t msg_data_size);
	void (*rcv_irq)(struct ADAPTER *ad);
};
#endif /* _UNI_FW_DL_HIF_H */
