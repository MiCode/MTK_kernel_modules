/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2017-2023 ARM Limited. All rights reserved.
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

#ifndef _KERNEL_UTF_TEST_USERDATA_H_
#define _KERNEL_UTF_TEST_USERDATA_H_

#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>

#define USERDATA_APP_NAME "test_app_userdata"
#define USERDATA_SUITE_NAME "simple_suite"

void userdata_add_two_values_test(struct kutf_context *context);
void userdata_increasing_str_sizes_test(struct kutf_context *context);
void userdata_random_str_sizes_test(struct kutf_context *context);

#endif /* _KERNEL_UTF_TEST_USERDATA_H_ */
