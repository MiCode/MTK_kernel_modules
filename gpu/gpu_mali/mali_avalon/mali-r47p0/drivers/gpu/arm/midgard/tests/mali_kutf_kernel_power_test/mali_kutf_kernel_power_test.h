/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2021 ARM Limited. All rights reserved.
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

#ifndef _KUTF_KERNEL_DEFECT_TEST_H_
#define _KUTF_KERNEL_DEFECT_TEST_H_

#define KERNEL_POWER_TEST_APP_NAME "kernel_power_test"
#define KERNEL_POWER_TEST_DEFAULT_SUITE "mali_kutf_kernel_power_default"
#define KERNEL_POWER_TEST_INSTANT_POWEROFF_SUITE "mali_kutf_kernel_power_instant_poweroff"
#define KERNEL_POWER_TEST_POWER_CORE_MASK_SUITE "mali_kutf_kernel_power_core_mask"

#define KERNEL_POWER_TEST_POWER_CYCLE "Power Cycle"
#define KERNEL_POWER_TEST_POWER_CYCLE_ENQUEUED "Power Cycle whilst wait enqueued"
#define KERNEL_POWER_TEST_POWER_CORE_AVAILABILITY_CG0 "CG0 Power Core Availability"
#define KERNEL_POWER_TEST_POWER_POLICY_UNIT "Power Policy"
#define KERNEL_POWER_TEST_ACTIVE_GPU_POWER_CORE "Power Core Mask with active GPU"
#define KERNEL_POWER_TEST_IDLE_GPU_POWER_CORE "Power Core Mask with idle GPU"
#define KERNEL_POWER_TEST_NO_INTERSECT_POWER_CORE "Power Core Mask with no intersect"
#define KERNEL_POWER_TEST_GPU_RESET_POWER_CORE "Power Core Mask with GPU reset"

#endif /* _KUTF_KERNEL_DEFECT_TEST_H_ */
