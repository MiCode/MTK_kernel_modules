/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2021-2023 ARM Limited. All rights reserved.
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

#ifndef _KUTF_KERNEL_DEFECT_TEST_HELPERS_H_
#define _KUTF_KERNEL_DEFECT_TEST_HELPERS_H_
#include <base/mali_base_context.h>

#define BYTES_TO_PAGES(x) (((x) + OSU_CONFIG_CPU_PAGE_SIZE - 1u) >> OSU_CONFIG_CPU_PAGE_SIZE_LOG2)

/** Number of pages per allocated region. */
#define ALLOC_SIZE_PAGES 1u

/** Number of allocated regions in this test. */
#define NUM_ALLOC_REGIONS 4u

struct basep_test_buf_descr {
	base_mem_handle hdl; /* Memory handle when mapping the buf_descr[] */
	void *buf_descr_cpu_va; /* buffer descriptor array base */
};

bool buf_descr_array_init(base_context *const ctx, size_t buf_cnt,
			  struct basep_test_buf_descr *buf_descr);
void buf_descr_array_term(base_context *const ctx, size_t buf_cnt,
			  struct basep_test_buf_descr *buf_descr);

#endif /* _KUTF_KERNEL_DEFECT_TEST_HELPERS_H_ */
