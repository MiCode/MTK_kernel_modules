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

#ifndef _KUTF_KERNEL_DEFECT_TEST_GPUCORE_37465_H_
#define _KUTF_KERNEL_DEFECT_TEST_GPUCORE_37465_H_

/* User space to kernel space parameter names */
#define GPUCORE37465_USERSPACE_READY "GPUCORE37465_USERSPACE_READY"
#define GPUCORE37465_USERSPACE_GPU_HEAP_VA "GPUCORE37465_USERSPACE_GPU_HEAP_VA"
#define GPUCORE37465_USERSPACE_HEAP_CHUNK_SIZE "GPUCORE37465_USERSPACE_HEAP_CHUNK_SIZE"
#define GPUCORE37465_USERSPACE_GPU_QUEUE_VA "GPUCORE37465_USERSPACE_GPU_QUEUE_VA"
#define GPUCORE37465_USERSPACE_HEAP_VT_START "GPUCORE37465_USERSPACE_HEAP_VT_START"
#define GPUCORE37465_USERSPACE_HEAP_VT_END "GPUCORE37465_USERSPACE_HEAP_VT_END"
#define GPUCORE37465_USERSPACE_HEAP_FRAG_END "GPUCORE37465_USERSPACE_HEAP_FRAG_END"
#define GPUCORE37465_USERSPACE_CTX_ID "GPUCORE37465_USERSPACE_CTX_ID"
#define GPUCORE37465_USERSPACE_DO_CLEANUP "GPUCORE37465_USERSPACE_DO_CLEANUP"
#define GPUCORE37465_USERSPACE_TERM_DONE "GPUCORE37465_USERSPACE_TERM_DONE"

/* Kernel space to user space parameter names */
#define GPUCORE37465_KERNEL_TEST_DONE "GPUCORE37465_KERNEL_TEST_DONE"
#define GPUCORE37465_KERNEL_CLEANUP_DONE "GPUCORE37465_KERNEL_CLEANUP_DONE"
#define GPUCORE37465_KERNEL_READY "GPUCORE37465_KERNEL_READY"

/* Generic parameter value to notify about success */
#define GPUCORE37465_OK 1
/* Generic parameter value to notify about failure */
#define GPUCORE37465_NOK 0

#endif /* _KUTF_KERNEL_DEFECT_TEST_GPUCORE_37465_H_ */
