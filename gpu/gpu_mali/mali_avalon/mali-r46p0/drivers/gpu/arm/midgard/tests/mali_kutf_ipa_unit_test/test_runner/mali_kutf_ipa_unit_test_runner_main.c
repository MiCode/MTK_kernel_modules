// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2017-2023 ARM Limited. All rights reserved.
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

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_main.h>
#include "mali_kutf_test_runner.h"
#include "mali_kutf_ipa_unit_test_extra.h"

#include "../mali_kutf_ipa_unit_test.h"

static int kutf_test_ipa_unit_test_runner_run_func(struct mali_utf_test_specifier *test_spec)
{
	int i = 0;

	kutf_test_runner_filter_app_clear();
	kutf_test_runner_filter_app_add(IPA_APP_NAME);

	/* Add extra testing functions to the test */
	while (kutf_ipa_unit_test_extra_funcs[i].app_name[0] != 0) {
		int err = kutf_test_runner_test_extras_add(&kutf_ipa_unit_test_extra_funcs[i]);
		if (err)
			return err;
		i++;
	}

	return kutf_test_runner_helper_run_func(test_spec);
}

static void kutf_test_ipa_unit_test_runner_cli_info_func(void)
{
	/* --app is a hidden option that is accepted but ignored */
	mali_tpi_puts("    --kutf_base_dir=  Specify the base directory of kutf.\n");
}

/* Main entry point. */
int main(int argc, char **argv)
{
	int ret;

	if (!mali_tpi_init())
		return MALI_UTF_RESULT_FATAL;

	ret = kutf_test_runner_init();

	if (!ret) {
		/* The runner and cli_info functions are overridden.
		 * The parse function is the stock helper for the moment,
		 * and will silently accept (but ignore) the --app
		 * argument. In future we could improve that to ensure we
		 * only accept the arguments supported by the core of the
		 * test runner
		 */
		ret = mali_utf_test_main(kutf_test_ipa_unit_test_runner_run_func,
					 kutf_test_runner_helper_parse_func,
					 kutf_test_ipa_unit_test_runner_cli_info_func, argc, argv);

		kutf_test_runner_term();
	}

	mali_tpi_shutdown();

	return ret;
}
