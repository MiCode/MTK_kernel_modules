/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2024. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER\'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER\'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER\'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK\'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK\'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver\'s
 * applicable license agreements with MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include "game_ec_private.h"
#include "game_lc_private.h"

#define HWCODE_IDX		20
#define SEGMENT_IDX		30

static int dev_initialized;
struct devinfo_tag {
	unsigned int data_size;
	unsigned int data[300];
};

static const struct devinfo_tag* get_devinfo_tag(void)
{
	static const struct devinfo_tag *tags;

	if (likely(dev_initialized))
		return tags;

	if (!dev_initialized) {
		struct device_node *np;

		np = of_find_node_by_path("/chosen");
		if (np) {
			tags = (struct devinfo_tag*) of_get_property(np, "atag,devinfo", NULL);
			of_node_put(np);
		}
		dev_initialized = 1;
	}

	return tags;
}

void get_devinfo_hwcode_segment(unsigned int *chip_code, unsigned int *segment_mask)
{
	const struct devinfo_tag *tags;

	tags = get_devinfo_tag();
	if (!tags) {
		pr_debug("%s: devinfo property missing\n", __func__);
		*chip_code = 0; // Set default or error value
		*segment_mask = 0; // Set default or error value
		return;
	}

	*chip_code = tags->data[HWCODE_IDX];
	*segment_mask = tags->data[SEGMENT_IDX];

	pr_debug("[%s] hw_code:0x%x seg:0x%x\n", __func__, *chip_code, *segment_mask);
}

static void __exit game_exit(void)
{
	engine_cooler_exit();
	lc_exit();
}

static int __init game_init(void)
{
	unsigned int chip_code = 0, segment_mask = 0;

	dev_initialized = 0;

	engine_cooler_init();
	get_devinfo_hwcode_segment(&chip_code, &segment_mask);
	lc_init(chip_code, segment_mask);
	return 0;
}

module_init(game_init);
module_exit(game_exit);

MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek GAME");
MODULE_AUTHOR("MediaTek Inc.");
