/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 *
 * Author: ChenHung Yang <chenhung.yang@mediatek.com>
 */

#ifndef MTK_AOV_MTEE_H
#define MTK_AOV_MTEE_H

/* from aov_fr_result.h */
struct aov_fr_offset {
	uint32_t token;
	uint64_t result_offset;
} __packed;

/* from aov_fr_cmd.h */
#define AOV_FR_CMD_INITIALIZE           (0)
#define AOV_FR_CMD_RELEASE              (1)
#define AOV_FR_CMD_SET_ACTIVE_USER      (2)
#define AOV_FR_CMD_CLEAR_ACTIVE_USER    (3)
#define AOV_FR_CMD_ENROLL               (4)
#define AOV_FR_CMD_REMOVE               (5)
#define AOV_FR_CMD_AUTHENTICATE         (6)
#define AOV_FR_CMD_CANCEL               (7)

// Forward declaration
struct mtk_aov;

int aov_mtee_init(struct mtk_aov *device);

int aov_mtee_notify(struct mtk_aov *device, void *buffer);

int aov_mtee_uninit(struct mtk_aov *aov_dev);

#endif  // MTK_AOV_MTEE_H
