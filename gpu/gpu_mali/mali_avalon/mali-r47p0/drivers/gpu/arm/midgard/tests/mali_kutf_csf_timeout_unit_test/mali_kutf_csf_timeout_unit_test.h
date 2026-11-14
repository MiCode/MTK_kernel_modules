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

#ifndef _KUTF_CSF_TIMEOUT_UNIT_TEST_H_
#define _KUTF_CSF_TIMEOUT_UNIT_TEST_H_

#define CSF_TIMEOUT_APP_NAME "csf_timeout_unit"
#define CSF_TIMEOUT_SUITE_NAME "timeout"
#define CSF_TIMEOUT_SUITE_FIXTURES (4)
#define UNIT_TEST_0 "timer_event"
#define UNIT_TEST_1 "timer_event_with_dof"

/* Input parameter names */
#define BASE_CTX_ID "BASE_CTX_ID"
#define GPU_QUEUE_VA "GPU_QUEUE_ADDRESS"

/* Output parameter names */
#define TIMER_EVENT_SYNC "TIMER_EVENT_SYNC"
#define DOF_CHECK_DONE "DOF_CHECK_DONE"

#endif /* _KUTF_CSF_TIMEOUT_UNIT_TEST_H_ */
