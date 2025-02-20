/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2022-2024 ARM Limited. All rights reserved.
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

#ifndef _VERSION_COMPAT_DEFS_H_
#define _VERSION_COMPAT_DEFS_H_

#include <linux/version.h>

#if KERNEL_VERSION(6, 0, 0) > LINUX_VERSION_CODE
#define KBASE_REGISTER_SHRINKER(reclaim, name, priv_data) register_shrinker(reclaim)

#elif (KERNEL_VERSION(6, 7, 0) > LINUX_VERSION_CODE)
#define KBASE_REGISTER_SHRINKER(reclaim, name, priv_data) register_shrinker(reclaim, name)

#else
#define KBASE_REGISTER_SHRINKER(reclaim, name, priv_data) \
	do {                                              \
		reclaim->private_data = priv_data;        \
		shrinker_register(reclaim);               \
	} while (0)

#endif /* KERNEL_VERSION(6, 0, 0) > LINUX_VERSION_CODE */

#if (KERNEL_VERSION(6, 7, 0) > LINUX_VERSION_CODE)
#define KBASE_UNREGISTER_SHRINKER(reclaim) unregister_shrinker(&reclaim)
#define KBASE_GET_KBASE_DATA_FROM_SHRINKER(s, type, var) container_of(s, type, var)
#define DEFINE_KBASE_SHRINKER struct shrinker
#define KBASE_INIT_RECLAIM(var, attr, name) (&((var)->attr))
#define KBASE_SET_RECLAIM(var, attr, reclaim) ((var)->attr = (*reclaim))

#else
#define KBASE_UNREGISTER_SHRINKER(reclaim) shrinker_free(reclaim)
#define KBASE_GET_KBASE_DATA_FROM_SHRINKER(s, type, var) s->private_data
#define DEFINE_KBASE_SHRINKER struct shrinker *
#define KBASE_SHRINKER_ALLOC(name) shrinker_alloc(0, name)
#define KBASE_INIT_RECLAIM(var, attr, name) (KBASE_SHRINKER_ALLOC(name))
#define KBASE_SET_RECLAIM(var, attr, reclaim) ((var)->attr = reclaim)

#endif

#endif /* _VERSION_COMPAT_DEFS_H_ */
