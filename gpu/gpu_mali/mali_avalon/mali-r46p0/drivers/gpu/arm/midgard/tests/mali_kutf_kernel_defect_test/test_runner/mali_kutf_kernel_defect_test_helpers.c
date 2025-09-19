/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2021-2023 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */

#include <utf/mali_utf.h>
#include <kutf/kutf_resultset.h>
#include "mali_kutf_test_runner.h"
#include "mali_kutf_kernel_defect_test_helpers.h"

bool buf_descr_array_init(base_context *const ctx, size_t buf_cnt,
			  struct basep_test_buf_descr *buf_descr)
{
	size_t i = 0;
	bool success = true;
	const unsigned int flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
				   BASE_MEM_PROT_GPU_WR | BASE_MEM_PROT_GPU_RD | BASE_MEM_SAME_VA;

	for (; i < buf_cnt; i++) {
		buf_descr[i].hdl =
			base_mem_alloc(ctx, ALLOC_SIZE_PAGES, ALLOC_SIZE_PAGES, 0, flags);
		if (base_mem_handle_is_invalid(buf_descr[i].hdl)) {
			success = false;
			break;
		}

		buf_descr[i].buf_descr_cpu_va = base_mem_cpu_address(buf_descr[i].hdl, 0);
		/* Clear the full array contents to 0 */
		memset(buf_descr[i].buf_descr_cpu_va, 0,
		       ALLOC_SIZE_PAGES * OSU_CONFIG_CPU_PAGE_SIZE);
	}

	while ((i-- > 0) && !success)
		base_mem_free(ctx, buf_descr[i].hdl, ALLOC_SIZE_PAGES);

	return success;
}

void buf_descr_array_term(base_context *const ctx, size_t buf_cnt,
			  struct basep_test_buf_descr *buf_descr)
{
	for (size_t i = 0; i < buf_cnt; i++)
		base_mem_free(ctx, buf_descr[i].hdl, ALLOC_SIZE_PAGES);
}
