// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _KBASE_CSF_IPA_CONTROL_EX_H
#define _KBASE_CSF_IPA_CONTROL_EX_H


/**
 * mtk_common_rate_change_notify_fp -  Function Pointer to
 *                                     Notify GPU rate change
 *                                     (MTK implementation)
 *
 * @kbdev:       Pointer to kbase device.
 * @clk_index:   Index of the clock for which the change has occurred.
 * @clk_rate_hz: Clock frequency(Hz).
 *
 * Notify the IPA Control component about a GPU rate change.
 */
extern int (*mtk_common_rate_change_notify_fp)(struct kbase_device *kbdev,
					       u32 clk_index, u32 clk_rate_hz);

#endif /* _KBASE_CSF_IPA_CONTROL_EX_H */
