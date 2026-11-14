/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _FW_LOG_MMIO_H
#define _FW_LOG_MMIO_H

struct FW_LOG_MMIO_SUB_CTRL {
	enum ENUM_FW_LOG_CTRL_TYPE type;
	uint32_t base_addr;
	uint32_t buf_base_addr;
	uint32_t length;
	uint32_t rp; /* read pointer */
	uint32_t irp; /* internal read pointer, used by driver */
	uint32_t wp; /* write pointer */
	uint32_t iwp; /* internal write pointer, used by fw */
	uint8_t *buffer;
};

struct FW_LOG_MMIO_SUB_STATS {
	uint32_t handle_size;
};

struct FW_LOG_MMIO_STATS {
	unsigned long update_period; /* in ms */
	uint32_t request;
	uint32_t skipped;
	uint32_t handled;
	struct FW_LOG_MMIO_SUB_STATS sub_stats[ENUM_FW_LOG_CTRL_TYPE_NUM];
};

struct FW_LOG_MMIO_CTRL {
	enum ENUM_LOG_READ_POINTER_PATH ePath;
	u_int8_t initialized;
	u_int8_t started;
	u_int8_t defered;
	uint32_t base_addr;
	struct FW_LOG_MMIO_SUB_CTRL sub_ctrls[ENUM_FW_LOG_CTRL_TYPE_NUM];
	void *priv;
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *prWakeLock;
#endif
	struct FW_LOG_MMIO_STATS stats;
	struct workqueue_struct *wq;
	struct work_struct work;
};

int32_t fwLogMmioHandler(void);
uint32_t fwLogMmioStart(struct ADAPTER *prAdapter);
void fwLogMmioStop(struct ADAPTER *prAdapter);
uint32_t fwLogMmioInitMcu(struct ADAPTER *prAdapter);
void fwLogMmioDeInitMcu(struct ADAPTER *prAdapter);

#endif /* _FW_LOG_MMIO_H */

