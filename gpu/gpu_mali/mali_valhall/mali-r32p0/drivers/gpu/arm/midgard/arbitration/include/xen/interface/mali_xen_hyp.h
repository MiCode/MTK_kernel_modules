/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2019-2021 ARM Limited. All rights reserved.
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

/*
 * Part of the Mali reference arbiter
 */

#ifndef _MALI_XEN_HYP_H_
#define _MALI_XEN_HYP_H_

#include <xen/interface/xen.h>

#define XENARBGPU_INTERFACE_VERSION 0x01000001

/**
 * struct xen_arb_gpu_op - xen gpu operation
 * @cmd: Arbiter to Hypervisor command
 * @domain: VM domain to use
 * @interface_version: Interface version number (XENARBGPU_INTERFACE_VERSION)
 */
struct xen_arb_gpu_op {
	uint32_t cmd;
	domid_t  domain;
	uint32_t interface_version;
};

typedef struct xen_arb_gpu_op xen_arb_gpu_op_t;
DEFINE_GUEST_HANDLE_STRUCT(xen_arb_gpu_op_t);

#define ARB_HYP_ASSIGN_VM_GPU           8

#endif /* _MALI_XEN_HYP_H_ */
