// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2023 ARM Limited. All rights reserved.
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
#ifdef CONFIG_KPROBES

#include <kutf/kutf_suite.h>
#include <kutf/kutf_kprobe.h>
#include "kutf_test_kprobe.h"

#define KPROBE_APP_NAME "kprobe"
#define KPROBE_SUITE_NAME "kprobe_suite"
#define KPROBE_TEST_NAME "kprobe_test"

static struct kutf_application *kprobe_app;

static void kprobe_test(struct kutf_context *context)
{
	kutf_kp_sample_kernel_function();
	kutf_test_pass(context, NULL);
}

void kutf_test_kprobe_exit(void)
{
	kutf_destroy_application(kprobe_app);

}

void kutf_test_kprobe_init(void)
{
	struct kutf_suite *kprobe_suite;

	kprobe_app = kutf_create_application(KPROBE_APP_NAME);
	if (!kprobe_app) {
		pr_warn("Creation of test_kprobe failed!\n");
		return;
	}

	kprobe_suite = kutf_create_suite(kprobe_app, KPROBE_SUITE_NAME, 1,
					 NULL, NULL);
	if (!kprobe_suite) {
		pr_warn("Creation of suite for test_kprobe failed!\n");
		kutf_destroy_application(kprobe_app);
	}

	kutf_add_test(kprobe_suite, 100, KPROBE_TEST_NAME,
		      kprobe_test);
}

#endif
