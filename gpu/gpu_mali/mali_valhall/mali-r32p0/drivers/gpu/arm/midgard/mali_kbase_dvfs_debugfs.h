/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 * (C) COPYRIGHT 2020-2021 ARM Limited. All rights reserved.
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

#ifndef _KBASE_DVFS_DEBUGFS_H_
#define _KBASE_DVFS_DEBUGFS_H_

/* Forward declaration */
struct kbase_device;

/**
 * kbase_dvfs_status_debugfs_init() - Create a debugfs entry for DVFS queries
 *
 * @kbdev: Pointer to the GPU device for which to create the debugfs entry
 */
void kbase_dvfs_status_debugfs_init(struct kbase_device *kbdev);

#endif /* _KBASE_DVFS_DEBUGFS_H_ */
