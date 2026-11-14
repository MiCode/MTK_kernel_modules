/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2019-2023 ARM Limited. All rights reserved.
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

#ifndef _KUTF_CSF_TILER_HEAP_UNIT_TEST_H_
#define _KUTF_CSF_TILER_HEAP_UNIT_TEST_H_

#define CSF_TILER_HEAP_APP_NAME "csf_tiler_heap_unit"
#define CSF_TILER_HEAP_SUITE_NAME "tiler_heap"
#define CSF_TILER_HEAP_SUITE_FIXTURES 26
#define UNIT_TEST_0 "oom_event"
#define UNIT_TEST_1 "oom_event_with_dof"

/* Input parameter names */
#define BASE_CTX_ID "BASE_CTX_ID"
#define GPU_HEAP_VA "HEAP_CTX_ADDRESS"
#define HEAP_CHUNK_SIZE "HEAP_CHUNK_SIZE"
#define GPU_QUEUE_VA "GPU_QUEUE_ADDRESS"
#define HEAP_VT_START "HEAP_VT_START"
#define HEAP_VT_END "HEAP_VT_END"
#define HEAP_FRAG_END "HEAP_FRAG_END"

/* Output parameter names */
#define OOM_EVENT_ACTION "OOM_EVENT_ACTION"

enum oom_event_action {
	OOM_EVENT_NEW_CHUNK_ALLOCATED,
	OOM_EVENT_NULL_CHUNK,
	OOM_EVENT_INCREMENTAL_RENDER,
	OOM_EVENT_CSG_TERMINATED,
	OOM_EVENT_MULTIPLE_NEW_CHUNKS,
	OOM_EVENT_INVALID_CHUNK,
	OOM_EVENT_INCONSISTENT_CS_STATE,
};

#endif /* _KUTF_CSF_TILER_HEAP_UNIT_TEST_H_ */
