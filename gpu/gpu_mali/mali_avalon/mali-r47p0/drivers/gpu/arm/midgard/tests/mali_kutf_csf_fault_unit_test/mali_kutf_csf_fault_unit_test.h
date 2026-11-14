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

#ifndef _KUTF_CSF_FAULT_UNIT_TEST_H_
#define _KUTF_CSF_FAULT_UNIT_TEST_H_

#define CSF_FAULT_APP_NAME "csf_fault_unit"
#define CS_FAULT_SUITE_NAME "cs_fault"
#define GPU_FAULT_SUITE_NAME "gpu_fault"
#define FW_CS_FAULT_SUITE_NAME "fw_cs_fault"
#define TRACE_INFO_FAULT_SUITE_NAME "trace_info_fault"

#define CS_FAULT_SUITE_FIXTURES 14
#define TRACE_INFO_FAULT_SUITE_FIXTURES 2
#define GPU_FAULT_SUITE_FIXTURES 8
#define FW_CS_FAULT_SUITE_FIXTURES 1
#define FW_CS_FAULT_CS_UNRECOVERABLE_SUITE_FIXTURES 1

#define CS_FAULT_UNIT_TEST_0 "fault_event"
#define CS_FAULT_UNIT_TEST_1 "fault_event_with_suspend"
#define CS_FAULT_UNIT_TEST_2 "fault_event_with_dof"
#define GPU_FAULT_UNIT_TEST_0 "gpu_fault"
#define FW_CS_FAULT_UNIT_TEST_0 "firmware_cs_fault_event_test"
#define FW_CS_FAULT_UNIT_TEST_1 "firmware_cs_fault_cs_unrecoverable_event_test"
#define FW_CS_FAULT_UNIT_TEST_2 "firmware_cs_fault_event_test_with_dof"
#define TRACE_INFO_FAULT_UNIT_TEST_0 "trace_info_fault_event"

/* Input parameter names */
#define BASE_CTX_ID "BASE_CTX_ID"
#define GPU_QUEUE_VA "GPU_QUEUE_ADDRESS"
#define FAULT_EXCEPTION_TYPE "FAULT_EXCEPTION_TYPE"
#define FAULT_EXCEPTION_DATA "FAULT_EXCEPTION_DATA"
#define FAULT_INFO_EXCEPTION_DATA "FAULT_INFO_EXCEPTION_DATA"
#define FAULT_TRACE_ID0 "FAULT_TRACE_ID0"
#define FAULT_TRACE_ID1 "FAULT_TRACE_ID1"
#define FAULT_TRACE_TASK "FAULT_TRACE_TASK"

/* Output parameter names */
#define FAULT_EVENT_ACTION "FAULT_EVENT_ACTION"
#define DOF_CHECK_DONE "DOF_CHECK_DONE"
#define SKIP_TRACE_INFO_TEST "SKIP_TRACE_INFO_TEST"

enum fault_event_action {
	FAULT_EVENT_NO_ACTION,
	FAULT_EVENT_CSG_TERMINATED,
	FAULT_EVENT_ERROR_RECOVERY_REQUESTED,
	FAULT_EVENT_INCONSISTENT_CS_STATE
};

enum fault_test_mode { FAULT_TEST_DEFAULT, FAULT_TEST_SUSPEND, FAULT_TEST_DOF };

#endif /* _KUTF_CSF_FAULT_UNIT_TEST_H_ */
