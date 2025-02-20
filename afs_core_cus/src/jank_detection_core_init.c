/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. and/or its licensors.
 * Except as otherwise provided in the applicable licensing terms with
 * MediaTek Inc. and/or its licensors, any reproduction, modification, use or
 * disclosure of MediaTek Software, and information contained herein, in whole
 * or in part, shall be strictly prohibited.
*/

#include <linux/module.h>

extern void model_init(void);
extern void model_exit(void);

static int __init jank_detection_init(void) {
  model_init();
  return 0;
}

static void __exit jank_detection_exit(void) { model_exit(); }

module_init(jank_detection_init);
module_exit(jank_detection_exit);

MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek Jank Detection");
MODULE_AUTHOR("MediaTek Inc.");
