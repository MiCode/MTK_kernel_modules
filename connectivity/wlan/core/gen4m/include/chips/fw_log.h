/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_H
#define _FW_LOG_H

enum ENUM_FW_LOG_CTRL_TYPE {
	ENUM_FW_LOG_CTRL_TYPE_MCU,
	ENUM_FW_LOG_CTRL_TYPE_WIFI,
	ENUM_FW_LOG_CTRL_TYPE_NUM,
};

__KAL_ATTRIB_PACKED_FRONT__
struct FW_LOG_COMMON_HEADER {
	uint32_t wifi_log_base_addr;
	uint32_t wifi_log_length;
	uint32_t mcu_log_base_addr;
	uint32_t mcu_log_length;
	uint32_t bt_log_base_addr;
	uint32_t bt_log_length;
	uint32_t gps_log_base_addr;
	uint32_t gps_log_length;
	uint8_t reserved[24];
	uint8_t key[8];
	uint8_t reserved2[32];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct FW_LOG_SUB_HEADER {
	uint32_t rp;
	uint32_t wp;
	uint32_t internal_wp;
	uint8_t reserved[20];
} __KAL_ATTRIB_PACKED__;

enum ENUM_LOG_READ_POINTER_PATH {
	ENUM_LOG_READ_POINTER_PATH_CMD,
	ENUM_LOG_READ_POINTER_PATH_CCIF,
	ENUM_LOG_READ_POINTER_PATH_NUM,
};

struct FW_LOG_OPS {
	uint32_t (*init)(struct ADAPTER *ad);
	void (*deinit)(struct ADAPTER *ad);
	uint32_t (*start)(struct ADAPTER *ad);
	void (*stop)(struct ADAPTER *ad);
	void (*set_enabled)(struct ADAPTER *ad, u_int8_t enabled);
	int32_t (*handler)(void);
};

struct FW_LOG_INFO {
	uint32_t base;
	enum ENUM_LOG_READ_POINTER_PATH path;
	struct FW_LOG_OPS *ops;
};

uint8_t *fw_log_type_to_str(enum ENUM_FW_LOG_CTRL_TYPE eType);
uint32_t fw_log_init(struct ADAPTER *ad);
void fw_log_deinit(struct ADAPTER *ad);
uint32_t fw_log_start(struct ADAPTER *ad);
void fw_log_stop(struct ADAPTER *ad);
void fw_log_set_enabled(struct ADAPTER *ad, u_int8_t enabled);
int32_t fw_log_handler(void);

#endif /* _FW_LOG_H */

