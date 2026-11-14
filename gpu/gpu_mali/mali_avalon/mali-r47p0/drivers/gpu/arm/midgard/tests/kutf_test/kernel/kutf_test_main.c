// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2014-2023 ARM Limited. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>

#include "kutf_test_userdata.h"
#ifdef CONFIG_KPROBES
#include "kutf_test_kprobe.h"
#endif

static struct kutf_application *app1;
static struct kutf_application *app2;
static struct kutf_application *app_userdata;

static const char *const names[] = { "Fixture 1", "Fixture 2", "Fixture 3" };

static void simple_test(struct kutf_context *context)
{
	kutf_test_info(context, "simple_test says hello");
	kutf_test_pass(context, NULL);
}

static void simple_test2(struct kutf_context *context)
{
	kutf_test_info(context, "simple_test2 says hello");
	kutf_test_pass(context, NULL);
}

static void simple_test3(struct kutf_context *context)
{
	kutf_test_info(context, "simple_test3 says hello");
	kutf_test_pass(context, NULL);
}

static void *fixture_create(struct kutf_context *context)
{
	struct kutf_suite *suite = context->suite;
	char *name;

	name = kutf_mempool_alloc(&context->fixture_pool, 50);
	if (!name)
		return NULL;

	snprintf(name, 50, "%s-%s", suite->name, names[context->fixture_index]);
	context->fixture_name = name;

	return (void *)1;
}

static void fixture_destroy(struct kutf_context *context)
{
}

static void complex_test(struct kutf_context *context)
{
	const char *message;

	message =
		kutf_dsprintf(&context->fixture_pool, "complex_test on %s", context->fixture_name);

	kutf_test_debug(context, message);
	kutf_test_pass(context, message);
	kutf_test_info(context, message);
	kutf_test_warn(context, message);
}

static void complex_test2(struct kutf_context *context)
{
	const char *message;

	message =
		kutf_dsprintf(&context->fixture_pool, "complex_test2 on %s", context->fixture_name);

	kutf_test_warn(context, message);
	kutf_test_info(context, message);
	kutf_test_pass(context, message);
	kutf_test_debug(context, message);
}

static int __init init_test(void)
{
	struct kutf_suite *app1_suite1;
	struct kutf_suite *app2_suite1;
	struct kutf_suite *app2_suite2;
	struct kutf_suite *app_userdata_suite;

	unsigned int filters;

	app1 = kutf_create_application("test_app");
	if (!app1) {
		pr_warn("Creation of test_app failed!\n");
		goto fail_no_apps_present;
	}

	app1_suite1 = kutf_create_suite(app1, "simple_suite", 1, NULL, NULL);
	if (!app1_suite1) {
		pr_warn("Creation of simple_suite for test_app failed!\n");
		goto fail_app1_present;
	}

	kutf_add_test(app1_suite1, 100, "simple_test", simple_test);
	kutf_add_test(app1_suite1, 101, "simple_test2", simple_test2);
	kutf_add_test(app1_suite1, 102, "simple_test3", simple_test3);

	app2 = kutf_create_application("test_app2");
	if (!app2) {
		pr_warn("Creation of test_app2 failed!\n");
		goto fail_app1_present;
	}

	app2_suite1 = kutf_create_suite(app2, "foo", 2, fixture_create, fixture_destroy);
	if (!app2_suite1) {
		pr_warn("Creation of test suite foo for test_app2 failed!\n");
		goto fail_app1_app2_present;
	}

	kutf_add_test(app2_suite1, 100, "complex_test", complex_test);
	kutf_add_test(app2_suite1, 101, "complex_test2", complex_test2);

	app2_suite2 = kutf_create_suite(app2, "simple_suite", 1, NULL, NULL);
	if (!app2_suite2) {
		pr_warn("Creation of simple_suite for test_app2 failed!\n");
		goto fail_app1_app2_present;
	}

	kutf_add_test(app2_suite2, 100, "simple_test", simple_test);

	app_userdata = kutf_create_application(USERDATA_APP_NAME);
	if (!app_userdata) {
		pr_warn("Creation of " USERDATA_APP_NAME " failed!\n");
		goto fail_app1_app2_present;
	}
	app_userdata_suite = kutf_create_suite(app_userdata, USERDATA_SUITE_NAME, 1, NULL, NULL);
	if (!app_userdata_suite) {
		pr_warn("Creation of " USERDATA_SUITE_NAME " for " USERDATA_APP_NAME " failed!\n");
		goto fail_app1_app2_appuserdata_present;
	}

	filters = app_userdata_suite->suite_default_flags;
	kutf_add_test_with_filters(app_userdata_suite, 100, "add_two_values",
				   userdata_add_two_values_test, filters);
	kutf_add_test_with_filters(app_userdata_suite, 101, "increasing_str_sizes",
				   userdata_increasing_str_sizes_test, filters);
	kutf_add_test_with_filters(app_userdata_suite, 102, "random_str_sizes",
				   userdata_random_str_sizes_test, filters);
#ifdef CONFIG_KPROBES
	kutf_test_kprobe_init();
#endif
	return 0;

fail_app1_app2_appuserdata_present:
	kutf_destroy_application(app_userdata);
fail_app1_app2_present:
	kutf_destroy_application(app2);
fail_app1_present:
	kutf_destroy_application(app1);
fail_no_apps_present:
	return -ENOMEM;
}

static void __exit exit_test(void)
{
	kutf_destroy_application(app1);
	kutf_destroy_application(app2);
	kutf_destroy_application(app_userdata);
#ifdef CONFIG_KPROBES
	kutf_test_kprobe_exit();
#endif
}

MODULE_LICENSE("GPL");

module_init(init_test);
module_exit(exit_test);
