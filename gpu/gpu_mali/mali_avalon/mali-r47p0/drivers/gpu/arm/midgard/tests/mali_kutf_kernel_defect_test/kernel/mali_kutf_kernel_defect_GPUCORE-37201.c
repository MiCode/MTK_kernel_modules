// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2022-2023 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#include <linux/delay.h>
#include <device/mali_kbase_device.h>
#include <mali_kbase.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers_user.h>
#include "mali_kutf_kernel_defect_test_main.h"


/**
 * mali_kutf_kernel_defect_GPUCORE_37201 - Entry point for the test
 * @context: KUTF context
 *
 */
void mali_kutf_kernel_defect_GPUCORE_37201(struct kutf_context *context)
{
	kutf_test_skip_msg(context,
			   "The kernel_defect_GPUCORE_37201 test is only applicable to JM GPUs");
}
