/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2017-2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _KUTF_TEST_RUNNER_H_
#define _KUTF_TEST_RUNNER_H_

#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>

/*
 * During the probing of the sysfs entries paths need to be built up
 * which requires the use of scratch buffers. As there is no true maximum
 * path + filename define a "reasonable" size and throw an error if we ever
 * exceed it
 */
#define PATH_LIMIT 1024

/*
 * Define a "reasonable" size for application name, this limit is not
 * enforced and applications with longer names will have their name
 * truncated
 */
#define APP_NAME_LEN 128

/*
 * Extra functions to run to allow user-side to take part in the kernel
 * test. These will apply to all fixtures for that test.
 *
 * Extra functions should use the standard utf mechanisms for reporting
 * results.
 *
 * Any pretest results will appear before the kernel's test results.
 *
 * Any midtest results generally appear before the kernel's test results, but
 * could appear mixed in with kernel test results when the kernel test reaches
 * the point where it no longer needs any userspace data.
 *
 * Any posttest results will appear after the kernel's test results.
 *
 * Any or all of these can be NULL.
 */
struct kutf_test_runner_cbs {
	/*
	 * Function pointer to call before a kernel test is run on a fixture
	 *
	 * Returns non-zero if an error was found that prevents the rest of the
	 * test being run
	 */
	int (*pretest)(struct mali_utf_suite *suite);

	/*
	 * Function pointer to call during a test in parallel with the
	 * kernel thread.
	 *
	 * This function must return normally (i.e. it must not call any
	 * MALI_UTF_ASSERT_xxx_EX function). Otherwise the kernel test results
	 * will not be read and the posttest callback will not be made.
	 */
	void (*midtest)(struct mali_utf_suite *suite);

	/*
	 * Function pointer to call after a kernel test is run on a fixture.
	 *
	 * If the pretest returned no error, then this will be run, e.g. to
	 * perform cleanup actions should the midtest fail.
	 */
	void (*posttest)(struct mali_utf_suite *suite);
};

struct kutf_extra_func_spec {
	char app_name[APP_NAME_LEN];
	char *suite_name;
	char *test_name;
	struct kutf_test_runner_cbs extra_funcs;
};

struct kutf_fixture_data {
	char file_name[PATH_LIMIT];
	struct kutf_test_runner_cbs *extra_funcs;
	void *extra_funcs_data;
	int fd;
};

/*
 * Initialize the kutf test runner
 */
int kutf_test_runner_init(void);

/*
 * Terminate the kutf test runner
 */
void kutf_test_runner_term(void);

/*
 * Clear the filter of apps.
 *
 * This will revert the effect of all previous calls to
 * kutf_test_runner_filter_app_add(), allowing all discovered apps to be run.
 */
void kutf_test_runner_filter_app_clear(void);

/*
 * Add an app string to a filter of selected apps for the run function helper
 *
 * A copy of the app string is made, so the memory pointed to by app can be
 * freed afterwards if so desired.
 *
 * Return zero on success, UTF result value otherwise.
 */
int kutf_test_runner_filter_app_add(char *app);

/*
 * Specify the name of a test within a suite and app to run some extra
 * functions from userspace
 *
 * This can be used so that userspace can take part in the test, and will often
 * require communication to kernel space via the test's 'run' file.
 *
 * Copies of the extra_test_spec members are made, so they may be safely freed by the
 * caller if so desired.
 *
 * If this is called with the same combination of (app_name, suite_name,
 * test_name), only the last call will take effect.
 *
 * Return zero on success, UTF result value otherwise.
 */
int kutf_test_runner_test_extras_add(const struct kutf_extra_func_spec *extra_func_spec);

/*
 * Helper function to find tests exposed by KUTF and run them.
 */
int kutf_test_runner_helper_run_func(mali_utf_test_specifier *test_spec);

/*
 * Helper function to parse the parameters that are specific to our test.
 */
int kutf_test_runner_helper_parse_func(mali_utf_test_specifier *test_spec, mali_utf_mempool *pool,
				       const char *arg);

/*
 * Helper function to provide information about the CLI options defined in kutf_test_runner_helper_parse_func()
 */
void kutf_test_runner_helper_cli_info_func(void);

/*
 * Helper function which takes the result string that is
 * reported by KUTF and translates it into a UTF result (which is logged)
 * with result status and message string.
 *
 * Note: result is modified by this function
 */
void kutf_test_runner_add_result_external(char *result);

/*
 * Helper function which provides information on kprobe availability.
 * Kprobes enables to dynamically break into any kernel routine
 * and collect debugging and performance information.
 * Refer https://docs.kernel.org/trace/kprobes.html for more info.
 *
 * Returns true if kprobe functionality is enabled in kernel.
 * Ensure kutf.ko is loaded before calling this function.
 */
bool kutf_test_runner_helper_is_kprobe_available(void);

/*
 * Helper function to register kprobe.
 * @probe_func_name:    kernel function name to probe.
 * @entry:              True if we need to insert probe at function entry.
 *                      False if we need to insert probe at function exit.
 * @probe_handler_name: kernel function name which should be called once
 *                      probe is triggered by kernel
 * @format:             format string for variable arguments.
 * @...                 variable number of arguments to @probe_handler_name
 */
int kutf_test_runner_helper_register_kprobe(char *probe_func_name,
					    bool  entry,
					    char *probe_handler_name,
					    char *format,
					    ...);
/*
 * Helper function to unregister kprobe.
 * @probe_func_name: kernel function name to unregister probe.
 * @entry:           True if we need to remove probe at function entry.
 *                   False if we need to remove probe at function exit.
 */
int kutf_test_runner_helper_unregister_kprobe(char *probe_func_name,
					      bool  entry);

#endif /* _KUTF_TEST_RUNNER_H_ */
