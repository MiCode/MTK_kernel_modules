// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>

#include "mtk_platform_debug.h"

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

static void print_group_queues_data(struct kbase_queue_group *const group)
{
	struct kbase_queue *queue;
	struct page *page;
	size_t size_mask;
	const unsigned int instruction_size = sizeof(u64);
	unsigned int i;
	u64 insert[5], extract[5], ringbuff[5];
	u64 *input_addr;
	u64 *output_addr;
	u64 cs_insert;
	u64 cs_extract;
	u64 start, stop;
	u64 page_off;
	u64 offset;
	u64 *ringbuffer;
	u64 *ptr;

	if (!group)
		return;

	for (i = 0; i < 5; i++) {
		queue = group->bound_queues[i];
		if (queue && queue->user_io_addr) {
			input_addr = (u64 *)queue->user_io_addr;
			output_addr = (u64 *)(queue->user_io_addr + PAGE_SIZE / sizeof(u64));

			insert[i] = input_addr[CS_INSERT_LO / sizeof(u64)];
			extract[i] = output_addr[CS_EXTRACT_LO / sizeof(u64)];
			ringbuff[i] = queue->base_addr;
		} else {
			insert[i] = 0;
			extract[i] = 0;
			ringbuff[i] = 0;
		}
	}

	dev_warn(group->kctx->kbdev->dev,
		"R0 %llx I0 %llx E0 %llx, R1 %llx I1 %llx E1 %llx, R2 %llx I2 %llx E2 %llx, R3 %llx I3 %llx E3 %llx, R4 %llx I4 %llx E4 %llx",
		ringbuff[0], insert[0], extract[0],
		ringbuff[1], insert[1], extract[1],
		ringbuff[2], insert[2], extract[2],
		ringbuff[3], insert[3], extract[3],
		ringbuff[4], insert[4], extract[4]);

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(group->kctx->kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"R0 %llx I0 %llx E0 %llx, R1 %llx I1 %llx E1 %llx, R2 %llx I2 %llx E2 %llx, R3 %llx I3 %llx E3 %llx, R4 %llx I4 %llx E4 %llx\n",
		ringbuff[0], insert[0], extract[0],
		ringbuff[1], insert[1], extract[1],
		ringbuff[2], insert[2], extract[2],
		ringbuff[3], insert[3], extract[3],
		ringbuff[4], insert[4], extract[4]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	for (i = 0; i < 5; i++) {
		queue = group->bound_queues[i];

		if (!queue)
			continue;

		cs_insert = insert[i];
		cs_extract = extract[i];

		if (!queue->queue_reg || !queue->queue_reg->nr_pages)
			continue;

		size_mask = (queue->queue_reg->nr_pages << PAGE_SHIFT) - 1;

		if (cs_insert == cs_extract)
			continue;

		cs_extract = ALIGN_DOWN(cs_extract, 8 * instruction_size);

		/* Go 32 instructions back */
		if (cs_extract > (32 * instruction_size))
			start = cs_extract - (32 * instruction_size);
		else
			start = 0;

		/* Print upto 64 instructions */
		stop = start + (64 * instruction_size);
		if (stop > cs_insert)
			stop = cs_insert;

		pr_err("\nQueue %u: Instructions from Extract offset %llx\n", i, start);

		while (start != stop) {
			page_off = (start & size_mask) >> PAGE_SHIFT;
			offset = (start & size_mask) & ~PAGE_MASK;

			if (!queue->queue_reg->gpu_alloc)
				break;

			page = as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
			ringbuffer = vmap(&page, 1, VM_MAP, pgprot_noncached(PAGE_KERNEL));

			if (!ringbuffer)
				break;

			ptr = &ringbuffer[offset/8];

			pr_err("%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);

			vunmap(ringbuffer);
			start += (8 * instruction_size);
		}
	}
}

void mtk_debug_csf_dump_queue_data(struct kbase_queue_group *group)
{
	if (!group)
		return;
	dev_err(group->kctx->kbdev->dev, "Dumping data of queues of group %d on slot %d in run_state %d",
				group->handle, group->csg_nr, group->run_state);
	print_group_queues_data(group);
}
