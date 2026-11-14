/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GL_FW_LOG_H
#define _GL_FW_LOG_H

#define FW_LOG_WIFI_INF_NAME "fw_log_wifi"

#define WIFI_FW_LOG_IOC_MAGIC         (0xfc)
#define WIFI_FW_LOG_IOCTL_ON_OFF      _IOW(WIFI_FW_LOG_IOC_MAGIC, 0, int)
#define WIFI_FW_LOG_IOCTL_SET_LEVEL   _IOW(WIFI_FW_LOG_IOC_MAGIC, 1, int)
#define WIFI_FW_LOG_IOCTL_GET_VERSION _IOR(WIFI_FW_LOG_IOC_MAGIC, 2, char*)

#define WIFI_FW_LOG_CMD_ON_OFF		0
#define WIFI_FW_LOG_CMD_SET_LEVEL	1

#define RING_BUFFER_SIZE_WF_MCU		0x20000
#define RING_BUFFER_SIZE_WF_FW		0x20000

#define MANIFEST_BUFFER_SIZE        256

struct fw_log_wifi_interface {};

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
void wifi_fwlog_event_func_register(void (*func)(int, int));
uint32_t fw_log_notify_rcv(enum ENUM_FW_LOG_CTRL_TYPE type,
	uint8_t *buffer,
	uint32_t size);
int fw_log_wifi_inf_init(void);
void fw_log_wifi_inf_deinit(void);
#else
static inline void wifi_fwlog_event_func_register(void (*func)(int, int)) {}
static inline uint32_t fw_log_notify_rcv(enum ENUM_FW_LOG_CTRL_TYPE type,
	uint8_t *buffer,
	uint32_t size) { return size; }
static inline int fw_log_wifi_inf_init(void) { return 0; }
static inline void fw_log_wifi_inf_deinit(void) {}
#endif

#endif /* _GL_FW_LOG_H */
