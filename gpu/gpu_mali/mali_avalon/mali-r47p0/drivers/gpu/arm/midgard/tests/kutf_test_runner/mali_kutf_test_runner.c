/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2014-2023 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include <tpi/mali_tpi.h>
#include <utf/include/mali_utf_suite.h>
#include <utf/include/mali_utf_mem.h>
#include <utf/include/mali_utf_resultset.h>
#include <utf/include/mali_utf_helpers.h>
#include <utf/include/mali_utf_main.h>

#include <kutf/kutf_resultset.h>
#include "mali_kutf_test_runner.h"
#include <cutils/linked_list/mali_cutils_dlist.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

/*
 * Maximum length of a single result that will be read from the kernel.
 * There is no limit imposed during the recording of a result in kutf
 * so there could be message truncation, but in this case the kernel
 * will discard the remaining part of the message and the next result
 * will be reported in the format we expect with the UTF result code
 * first which means at least the result code is _always_ obtained.
 */
#define KUTF_RESULT_LEN 1024

#define KUTF_KP_BUF_SIZE 4096

#define KUTF_BASE_DEBUGFS_DIR "/sys/kernel/debug/kutf_tests"
#define KUTF_KP_REG_FILE KUTF_BASE_DEBUGFS_DIR"/register_kprobe"
#define KUTF_KP_UNREG_FILE KUTF_BASE_DEBUGFS_DIR"/unregister_kprobe"

struct kutf_convert_table {
	char result_name[KUTF_ERROR_MAX_NAME_SIZE];
	mali_utf_result_status result;
};

struct kutf_convert_table kutf_convert[] = {
#define ADD_UTF_RESULT(name, value) \
	{                           \
		#name,              \
		value,              \
	},
	ADD_UTF_RESULT(KUTF_RESULT_BENCHMARK, MALI_UTF_RESULT_BENCHMARK) ADD_UTF_RESULT(
		KUTF_RESULT_SKIP, MALI_UTF_RESULT_SKIP) ADD_UTF_RESULT(KUTF_RESULT_UNKNOWN,
								       MALI_UTF_RESULT_UNKNOWN)
		ADD_UTF_RESULT(KUTF_RESULT_PASS, MALI_UTF_RESULT_PASS) ADD_UTF_RESULT(
			KUTF_RESULT_DEBUG, MALI_UTF_RESULT_DEBUG)
			ADD_UTF_RESULT(KUTF_RESULT_INFO, MALI_UTF_RESULT_INFO) ADD_UTF_RESULT(
				KUTF_RESULT_WARN, MALI_UTF_RESULT_WARN)
				ADD_UTF_RESULT(KUTF_RESULT_FAIL, MALI_UTF_RESULT_FAIL)
					ADD_UTF_RESULT(KUTF_RESULT_FATAL, MALI_UTF_RESULT_FATAL)
						ADD_UTF_RESULT(KUTF_RESULT_ABORT,
							       MALI_UTF_RESULT_ABORT)
};

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

struct app_list_node {
	char name[APP_NAME_LEN];
	cutils_dlist_item link;
};

cutils_dlist selected_apps_list = CUTILS_DLIST_STATIC_INITIALIZER;
cutils_dlist extra_funcs_list = CUTILS_DLIST_STATIC_INITIALIZER;

static char default_kutf_base_dir[] = KUTF_BASE_DEBUGFS_DIR;
char *kutf_base_dir = default_kutf_base_dir;

mali_utf_mempool global_mempool;

struct app_data {
	char dir_name[PATH_LIMIT];
	char name[APP_NAME_LEN];
	mali_utf_mempool result_mempool;
	mali_utf_mempool suite_mempool;
	mali_utf_suite *suite;
	mali_utf_result_set *results;
	struct app_data *next;
};

/* This could be optimised to insert into a tree of apps, referencing a tree of
 * suites, referencing a tree of tests. But we're not expecting many overrides
 * or many tests being iterated over in total, so we go with a simple
 * implementation for now
 */
struct extra_func_spec_item {
	cutils_dlist_item link;
	struct kutf_extra_func_spec func_spec;
};

/*
 * Initialize the kutf test runner
 */
int kutf_test_runner_init(void)
{
	mali_utf_memerr utf_err;

	utf_err = mali_utf_mempool_init(&global_mempool, 1024);
	if (utf_err != MALI_UTF_MEM_OK) {
		mali_utf_logerr("Failed to create mempool for querying kutf\n");
		return MALI_UTF_RESULT_FATAL;
	}
	return 0;
}

/*
 * Terminate the kutf test runner
 */
void kutf_test_runner_term(void)
{
	mali_utf_mempool_destroy(&global_mempool);
}

static void app_node_dtor(struct app_list_node *app_node)
{
	/* Nothing to do, theses items are on a mempool */
	CSTD_UNUSED(app_node);
}

void kutf_test_runner_filter_app_clear(void)
{
	CUTILS_DLIST_EMPTY_LIST(&selected_apps_list, struct app_list_node, link, app_node_dtor);
}

int kutf_test_runner_filter_app_add(char *app)
{
	struct app_list_node *app_node;

	app_node = mali_utf_mempool_alloc(&global_mempool, sizeof(*app_node));
	if (app_node == NULL) {
		mali_utf_logerr("Failed to allocate memory in kutf_test_runner_filter_add_app\n");
		return MALI_UTF_RESULT_FATAL;
	}

	strncpy(app_node->name, app, APP_NAME_LEN - 1);
	app_node->name[APP_NAME_LEN - 1] = '\0';

	CUTILS_DLIST_PUSH_BACK(&selected_apps_list, app_node, link);

	return 0;
}

/*
 * Check for apps in the selected list
 */
static bool is_app_selected(char *app_name)
{
	struct app_list_node *app_node;

	/* No filter means all apps selected */
	if (CUTILS_DLIST_IS_EMPTY(&selected_apps_list))
		return true;

	CUTILS_DLIST_FOREACH(&selected_apps_list, struct app_list_node, link, app_node)
	{
		if (strcmp(app_name, app_node->name) == 0)
			return true;
	}

	/* Filter was present but app not found */
	return false;
}

int kutf_test_runner_test_extras_add(const struct kutf_extra_func_spec *extra_func_spec)
{
	const char *app_name = extra_func_spec->app_name;
	const char *suite_name = extra_func_spec->suite_name;
	const char *test_name = extra_func_spec->test_name;
	const struct kutf_test_runner_cbs *extra_funcs = &extra_func_spec->extra_funcs;

	struct extra_func_spec_item *func_spec_item;
	int suite_name_sz = strlen(extra_func_spec->suite_name) + 1;
	int test_name_sz = strlen(extra_func_spec->test_name) + 1;

	int mem_sz = sizeof(*func_spec_item) + suite_name_sz + test_name_sz;
	char *memp;

	/* All parts allocated together to make error path code more simple */
	memp = mali_utf_mempool_alloc(&global_mempool, mem_sz);
	func_spec_item = (struct extra_func_spec_item *)memp;
	if (func_spec_item == NULL) {
		mali_utf_logerr("Failed to allocate memory in kutf_test_runner_test_extras_add\n");
		return MALI_UTF_RESULT_FATAL;
	}

	memp += sizeof(*func_spec_item);

	strncpy(func_spec_item->func_spec.app_name, app_name, APP_NAME_LEN - 1);
	func_spec_item->func_spec.app_name[APP_NAME_LEN - 1] = '\0';

	func_spec_item->func_spec.suite_name = memp;
	strncpy(func_spec_item->func_spec.suite_name, suite_name, suite_name_sz);
	memp += suite_name_sz;

	func_spec_item->func_spec.test_name = memp;
	strncpy(func_spec_item->func_spec.test_name, test_name, test_name_sz);

	func_spec_item->func_spec.extra_funcs = *extra_funcs;

	CUTILS_DLIST_PUSH_FRONT(&extra_funcs_list, func_spec_item, link);

	return 0;
}

static struct kutf_test_runner_cbs *find_test_extra_funcs(struct mali_utf_suite *suite)
{
	struct app_data *app = suite->user_state;
	char *app_name = app->name;
	const char *suite_name = suite->name;
	const char *test_name = suite->this_test->name;
	struct extra_func_spec_item *func_spec_item;

	CUTILS_DLIST_FOREACH(&extra_funcs_list, struct extra_func_spec_item, link, func_spec_item)
	{
		if (strcmp(func_spec_item->func_spec.app_name, app_name) != 0)
			continue;

		if (strcmp(func_spec_item->func_spec.suite_name, suite_name) != 0)
			continue;

		if (strcmp(func_spec_item->func_spec.test_name, test_name) != 0)
			continue;

		return &func_spec_item->func_spec.extra_funcs;
	}

	return NULL;
}

/*
 * Helper function which takes a KUTF string and converts it
 * into a UTF result status code.
 */
static mali_utf_result_status convert_kutf_result(char *status_string)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(kutf_convert); i++)
		if (strcmp(kutf_convert[i].result_name, status_string) == 0)
			return kutf_convert[i].result;

	/* If the result can't be converted report a fatal message */
	return MALI_UTF_RESULT_FATAL;
}

/*
 * Helper function which takes the result string that is
 * reported by KUTF and translates it into a UTF result
 * with result status and message string.
 */
void kutf_test_runner_add_result_external(char *result)
{
	char *result_string = result;
	char *message = NULL;
	char *tmp;
	mali_utf_result_status status;

	while ((*result != ':') && (*result != '\n') && (*result != '\0'))
		result++;

	if (*result == ':') {
		message = result + 1;
		*result = '\0';
	}

	/* Convert the KUTF result string into a UTF result code */
	status = convert_kutf_result(result_string);

	/* Drop the '\n' as UTF will add one for us */
	if (message != NULL) {
		tmp = message;
		while ((*tmp != '\n') && (*tmp != '\0'))
			tmp++;
		*tmp = '\0';
	} else {
		mali_utf_test_fatal("No data reported by the test!");
		return;
	}

	mali_utfp_test_log_result_external(message, status);
}

/*
 * UTF fixture run callback.
 *
 * This function is called for every fixture of every test to
 * run the test.
 */
static void kutf_test_runner_test_func(struct mali_utf_suite *suite)
{
	char tmp[KUTF_RESULT_LEN];
	struct kutf_fixture_data *const fix = suite->fixture;
	int fd;
	int read_bytes;
	int err;

	if (fix->extra_funcs && fix->extra_funcs->pretest) {
		err = fix->extra_funcs->pretest(suite);
		if (err) {
			mali_utf_test_fail("extra_funcs pretest failed");
			goto out_pretest_fail;
		}
	}

	fd = open(fix->file_name, O_RDWR);
	if (fd == -1) {
		mali_utf_test_fatal("failed to open test file");
		goto out_open_fail;
	}

	fix->fd = fd;

	if (fix->extra_funcs && fix->extra_funcs->midtest)
		fix->extra_funcs->midtest(suite);

	/*
	 * Read from the file reporting one result back at a time.
	 * Any user data should have already been read by the midtest function,
	 * including any results produced during the test
	 */
	while ((read_bytes = read(fd, tmp, sizeof(tmp) - 1)) > 0) {
		tmp[read_bytes] = '\0';
		kutf_test_runner_add_result_external(tmp);
	}

	close(fd);

out_open_fail:
	if (fix->extra_funcs && fix->extra_funcs->posttest)
		fix->extra_funcs->posttest(suite);

out_pretest_fail:
	return;
}

/*
 * UTF fixture creation callback.
 *
 * This function is called for every fixture of every test to
 * create the resources before the test is run
 */
static void *kutf_test_runner_create_func(struct mali_utf_suite *suite)
{
	struct app_data *app = suite->user_state;
	struct kutf_fixture_data *fix;
	int str_len;

	fix = mali_utf_test_alloc(sizeof(*fix));

	if (fix == NULL) {
		mali_utf_logerr("Failed to allocate test resources!\n");
		return NULL;
	}

	/*
	 * Store the filename, don't open the file until the test is executed as
	 * that will trigger kutf to run the fixture
	 */
	str_len = snprintf(fix->file_name, sizeof(fix->file_name), "%s/%s/%s/%d/run", app->dir_name,
			   suite->name, suite->this_test->name, suite->fixture_index);
	if ((str_len < 0) || ((size_t)str_len >= sizeof(fix->file_name))) {
		mali_utf_logerr("Path truncation in kutf_test_runner_create_func for 'run' file\n"
				"when processing app %s, suite %s, test %s, fixture %d\n",
				app->dir_name, suite->this_test->name, suite->this_test->name,
				suite->fixture_index);
		return NULL;
	}

	/* Setup and pre-test/post-test callbacks */
	fix->extra_funcs = find_test_extra_funcs(suite);
	fix->extra_funcs_data = NULL;

	return fix;
}

void kutf_test_runner_remove_func(struct mali_utf_suite *suite)
{
	/* Nothing to do */
}

/*
 * Helper function which reads the specified entry_name from a KUTF
 * app/suite/test/fixture and returns the value in the provided buffer.
 */
static int kutf_test_runner_read_entry(char *dir_name, char *entry_name, char *buf, int buf_size)
{
	char tmp[PATH_LIMIT];
	int fd;
	int count;
	int str_len;

	str_len = snprintf(tmp, sizeof(tmp), "%s/%s", dir_name, entry_name);
	if ((str_len < 0) || ((size_t)str_len >= sizeof(tmp))) {
		mali_utf_logerr(
			"Path truncation in kutf_test_runner_read_entry when processing %s, entry %s\n",
			dir_name, entry_name);
		goto error;
	}

	fd = open(tmp, S_IRUSR);
	if (fd == -1) {
		mali_utf_logerr("Failed to open %s in kutf_test_runner_read_entry\n", tmp);
		goto error;
	}

	count = read(fd, buf, buf_size - 1);
	buf[count] = '\0';
	close(fd);
	return 0;

error:
	return -1;
}

/*
 * Helper function which counts the number of fixtures within a
 * KUTF test and returns either that number or -1 on error.
 */
static int kutf_test_runner_count_fixtures_in_test(char *test_dir)
{
	DIR *dir;
	struct dirent *walker;
	int count = 0;

	dir = opendir(test_dir);
	if (!dir) {
		mali_utf_logerr("Failed to open %s in\n"
				"kutf_test_runner_count_fixtures_in_test\n",
				test_dir);
		goto error_dir;
	}

	/* Walk all entries */
	while (walker = readdir(dir), walker != NULL) {
		struct stat file_stat;
		char path[PATH_LIMIT];
		int ret;
		int str_len;

		str_len = snprintf(path, sizeof(path), "%s/%s", test_dir, walker->d_name);
		if ((str_len < 0) || ((size_t)str_len >= sizeof(path))) {
			mali_utf_logerr(
				"Path truncation in kutf_test_runner_count_fixtures_in_test when processing test %s, file %s\n",
				test_dir, walker->d_name);
			goto error_walking;
		}

		/* The only directory entries should be fixtures */
		ret = stat(path, &file_stat);
		if ((ret == 0) && (S_ISDIR(file_stat.st_mode)) && (walker->d_name[0] != '.'))
			count++;
	}

	closedir(dir);
	return count;

error_walking:
	closedir(dir);
error_dir:
	return -1;
}

/*
 * Scan a KUTF suite directory for fixtures.
 */
int kutf_test_runner_count_fixtures(char *suite_dir)
{
	DIR *dir;
	struct dirent *walker;
	int count = 0;

	dir = opendir(suite_dir);
	if (!dir) {
		mali_utf_logerr("Failed to open %s in kutf_test_runner_count_fixtures\n",
				suite_dir);
		goto error_dir;
	}

	/* Walk all enteries */
	while (walker = readdir(dir), walker != NULL) {
		struct stat file_stat;
		char path[PATH_LIMIT];
		int ret;
		int str_len;

		str_len = snprintf(path, sizeof(path), "%s/%s", suite_dir, walker->d_name);
		if ((str_len < 0) || ((size_t)str_len >= sizeof(path))) {
			mali_utf_logerr(
				"Path truncation in kutf_test_runner_count_fixtures when processing suite %s, file %s\n",
				suite_dir, walker->d_name);
			goto error_walking;
		}

		ret = stat(path, &file_stat);
		/* The only directory enteries should be tests */
		if ((ret == 0) && (S_ISDIR(file_stat.st_mode)) && (walker->d_name[0] != '.')) {
			int fix_count = kutf_test_runner_count_fixtures_in_test(path);

			/*
			 * Check that all tests in the suite have the same number of
			 * fixtures and report an error if they don't
			 */
			if (count == 0) {
				count = fix_count;
			} else {
				if (count != fix_count) {
					mali_utf_logerr(
						"Found a different number of fixtures in different tests in the same suite (%s)!\n",
						path);
					goto error_walking;
				}
			}
		}
	}

	closedir(dir);
	return count;

error_walking:
	closedir(dir);
error_dir:
	return -1;
}

/*
 * Scan a KUTF suite directory for tests.
 */
int kutf_test_runner_add_tests(char *dir_name, mali_utf_suite *suite, mali_utf_mempool *mempool)
{
	char file_name[PATH_LIMIT];
	char tmp[PATH_LIMIT];
	DIR *dir;
	struct dirent *walker;

	dir = opendir(dir_name);
	if (!dir) {
		mali_utf_logerr("Failed to open directory %s in kutf_test_runner_add_tests\n",
				dir_name);
		goto error_dir;
	}

	while (walker = readdir(dir), walker != NULL) {
		int ret;
		unsigned int test_id;
		unsigned int test_flags;
		char *test_name;
		struct stat file_stat;
		int str_len;
		int sscanf_ret;

		/* Skip files that aren't of interest */
		if (walker->d_name[0] == '.')
			continue;

		str_len = snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, walker->d_name);
		if ((str_len < 0) || ((size_t)str_len >= sizeof(file_name))) {
			/*
			 * The path we're building has overflowed the scratch buffer
			 * so report a failure
			 */
			mali_utf_logerr(
				"Path truncation in kutf_test_runner_add_tests when processing suite %s, file %s\n",
				dir_name, walker->d_name);
			goto error_walking;
		}

		/* Check if this is a directory */
		ret = stat(file_name, &file_stat);
		if (ret != 0) {
			mali_utf_logerr("Failed to stat %s in kutf_test_runner_add_tests\n",
					file_name);
			goto error_walking;
		}

		if (!S_ISDIR(file_stat.st_mode))
			continue;

		/* Directories that don't contain the expected entries are skipped over */
		ret = kutf_test_runner_read_entry(file_name, "filters", tmp, sizeof(tmp));
		if (ret != 0)
			continue;
		sscanf_ret = sscanf(tmp, "0x%08x", &test_flags);
		if ((sscanf_ret == 0) || (sscanf_ret == EOF)) {
			mali_utf_logerr("Failed to read test flags\n");
			goto error_walking;
		}

		ret = kutf_test_runner_read_entry(file_name, "test_id", tmp, sizeof(tmp));
		if (ret != 0)
			continue;
		sscanf_ret = sscanf(tmp, "%ud", &test_id);
		if ((sscanf_ret == 0) || (sscanf_ret == EOF)) {
			mali_utf_logerr("Failed to read test id\n");
			goto error_walking;
		}

		test_name = mali_utf_mempool_alloc(mempool, strlen(walker->d_name) + 1);
		if (test_name == NULL) {
			mali_utf_logerr("Failed to allocate memory for test\n");
			continue;
		}

		memcpy(test_name, walker->d_name, strlen(walker->d_name) + 1);

		mali_utf_add_test_with_filters(suite, test_id, test_name,
					       kutf_test_runner_test_func, test_flags);
	}
	closedir(dir);
	return 0;

error_walking:
	closedir(dir);
error_dir:
	return -1;
}

static int kutf_test_runner_add_suite(char const *const dir_name, struct dirent const *const walker,
				      struct app_data *const app)
{
	int ret;
	int fixture_count;
	char path[PATH_LIMIT];
	char *suite_name;
	mali_utf_suite *suite;
	struct stat file_stat;
	int str_len;
	char tmp[PATH_LIMIT];

	if (walker->d_name[0] == '.')
		return 0;

	str_len = snprintf(path, sizeof(path), "%s/%s", dir_name, walker->d_name);
	if ((str_len < 0) || ((size_t)str_len >= sizeof(path))) {
		mali_utf_logerr(
			"Path truncation in kutf_test_runner_add_app when processing app %s\n",
			dir_name);
		return -1;
	}

	/* Check if this is a directory */
	ret = stat(path, &file_stat);
	if (ret != 0) {
		mali_utf_logerr("Failed to stat %s in kutf_test_runner_add_app\n", path);
		return -1;
	}

	if (!S_ISDIR(file_stat.st_mode))
		return 0;

	/* Check if this directory is a suite */
	ret = kutf_test_runner_read_entry(path, "type", tmp, sizeof(tmp));
	if (ret != 0)
		return 0;

	if (strcmp(tmp, "suite\n") != 0)
		return 0;

	/* Check the number of fixtures required by this suite */
	fixture_count = kutf_test_runner_count_fixtures(path);
	if (fixture_count <= 0)
		return 0;

	suite_name = mali_utf_mempool_alloc(&app->suite_mempool, strlen(walker->d_name) + 1);
	if (suite_name == NULL)
		return 0;

	strcpy(suite_name, walker->d_name);

	/* Create the suite based on the information we're gathered */
	suite = mali_utf_create_suite(&app->suite_mempool, suite_name, fixture_count,
				      kutf_test_runner_create_func, kutf_test_runner_remove_func,
				      app->results, app->suite);

	if (!suite) {
		mali_utf_logerr("Failed to create suite for %s", suite_name);
		return -1;
	}

	/* Store the root suite in the app structure */
	if (app->suite == NULL)
		app->suite = suite;

	/* Store the app data in the suite for back reference */
	mali_utf_set_suite_user_data(suite, app, NULL);

	/* Recurse into each test directory adding tests as we go */
	return kutf_test_runner_add_tests(path, suite, &app->suite_mempool);
}

/*
 * Scan a directory that contains a KUTF app and
 * enumerate the suites, tests and fixtures within it.
 */
struct app_data *kutf_test_runner_add_app(char *dir_name)
{
	/* Allocate a structure to store our per application data */
	mali_utf_memerr err;
	struct app_data *const app = mali_utf_mempool_alloc(&global_mempool, sizeof(*app));

	if (app == NULL) {
		mali_utf_logerr("Failed to allocate memory for app %s\n", dir_name);
		return NULL;
	}

	*app = (struct app_data){ .suite = NULL, .name = "", .dir_name = "" };
	strncat(app->name, basename(dir_name), APP_NAME_LEN - 1);
	strncat(app->dir_name, dir_name, PATH_LIMIT - 1);

	/* Initialize the resources required by UTF */
	err = mali_utf_mempool_init(&app->result_mempool, 1024);
	if (err != MALI_UTF_MEM_OK) {
		mali_utf_logerr("Failed to initialize results memory pool when adding app %s\n",
				dir_name);
		return NULL;
	}

	err = mali_utf_mempool_init(&app->suite_mempool, 1024);
	if (err != MALI_UTF_MEM_OK) {
		mali_utf_logerr("Failed to initialize suite memory pool when adding app %s\n",
				dir_name);
	} else {
		app->results = mali_utf_create_result_set(&app->result_mempool);
		if (app->results == NULL) {
			mali_utf_logerr("Failed to create results set when adding app %s\n",
					dir_name);
		} else {
			int errnum = 0;
			DIR *dir = opendir(dir_name);

			if (dir) {
				/* Scan each directory which should be a suite */
				for (struct dirent *walker = readdir(dir);
				     walker != NULL && !errnum; walker = readdir(dir))
					errnum = kutf_test_runner_add_suite(dir_name, walker, app);

				closedir(dir);
			}

			if (!errnum)
				return app;

			/* Only error clean-up code is below */
			mali_utf_destroy_result_set(app->results);
		}
		mali_utf_mempool_destroy(&app->suite_mempool);
	}
	mali_utf_mempool_destroy(&app->result_mempool);
	return NULL;
}

/*
 * Scan the specified directory looking for KUTF apps
 * within it. If non-KUTF directories are found that recurse
 * into the next level and try again.
 */
struct app_data *kutf_test_runner_scan_directory_app(char *dir_name, struct app_data *prev)
{
	char tmp[PATH_LIMIT];
	int fd;
	int ret;
	/* If no app is found in this directory pass back the last found app */
	struct app_data *app = prev;
	struct stat file_stat;
	DIR *dir;
	int str_len;

	/* Check if this is a directory */
	ret = stat(dir_name, &file_stat);
	if (ret != 0) {
		mali_utf_logerr("Failed to stat %s in kutf_test_runner_scan_directory_app\n",
				dir_name);
		goto error_stat;
	}

	if (!S_ISDIR(file_stat.st_mode))
		return app;

	str_len = snprintf(tmp, sizeof(tmp), "%s/type", dir_name);
	if ((str_len < 0) || ((size_t)str_len >= sizeof(tmp))) {
		mali_utf_logerr(
			"Path truncation in kutf_test_runner_scan_directory_app when processing app %s\n",
			dir_name);
		goto error_stat;
	}

	fd = open(tmp, S_IRUSR);
	if (fd != -1) {
		int count;

		/* Looks like we're in a kutf directory, check the level */
		count = read(fd, tmp, sizeof(tmp) - 1);
		tmp[MAX(count, 0)] = '\0';

		if (strcmp(tmp, "application\n") == 0) {
			/* At application level, should this application be added? */
			if (is_app_selected(basename(dir_name))) {
				app = kutf_test_runner_add_app(dir_name);
				if (app == NULL) {
					mali_utf_logerr("Failed to add application!\n");
					close(fd);
					goto error_stat;
				}
				app->next = prev;
			}
			close(fd);
		} else {
			close(fd);
			/* Clear down fd to show that this directory wasn't what we're looking for */
			fd = -1;
		}
	}

	/* Note: Not else due to the clear down above */
	if (fd == -1) {
		/* This isn't a kutf application directory, check all sub directories */
		struct dirent *walker;
		struct stat file_stat;

		dir = opendir(dir_name);
		if (!dir)
			return app;

		while (walker = readdir(dir), walker != NULL) {
			int ret;

			ret = stat(kutf_base_dir, &file_stat);
			if ((ret == 0) && S_ISDIR(file_stat.st_mode) &&
			    (walker->d_name[0] != '.')) {
				char path[PATH_LIMIT];
				int str_len;

				str_len = snprintf(path, sizeof(path), "%s/%s", dir_name,
						   walker->d_name);
				if ((str_len < 0) || ((size_t)str_len >= sizeof(path))) {
					mali_utf_logerr(
						"Path truncation in kutf_test_runner_scan_directory_app when processing %s\n",
						dir_name);
					goto error_walking;
				}
				app = kutf_test_runner_scan_directory_app(path, app);
			}
		}
		closedir(dir);
	}
	return app;

error_walking:
	closedir(dir);
error_stat:
	return NULL;
}

/*
 * Run function Helper shared across most test apps
 *
 * First, based on either a user provided or the default path scan all the
 * sub directories and build up test apps, suite, tests and fixtures.
 * Once these structures have been created run each test app, or just the
 * selected test app (depending on what the user requested) through UTF and
 * report back the results per app.
 */
int kutf_test_runner_helper_run_func(mali_utf_test_specifier *test_spec)
{
	struct stat file_stat;
	int ret;
	struct app_data *apps;

	ret = stat(kutf_base_dir, &file_stat);
	if (ret != 0) {
		mali_utf_logerr(
			"Invalid path specified for kutf base directory (did you remember to insmod kutf.ko?)\n");
		goto error_stat;
	}

	if (!S_ISDIR(file_stat.st_mode)) {
		mali_utf_logerr("Path specified is not a directory:\n");
		goto error_stat;
	}

	apps = kutf_test_runner_scan_directory_app(kutf_base_dir, NULL);
	if (apps == NULL)
		mali_utf_logerr("Failed to find test apps or failure building test apps\n");

	while (apps) {
		int worst_error;

		mali_tpi_puts(MALI_UTF_SEP_THICK);
		mali_tpi_puts(apps->name);
		mali_tpi_puts("\n" MALI_UTF_SEP_THICK);
		worst_error = mali_utf_test_default_exec_and_teardown(test_spec, apps->suite,
								      &apps->result_mempool,
								      &apps->suite_mempool,
								      &apps->results);
		apps = apps->next;
		if (worst_error > ret)
			ret = worst_error;
	}

	return ret;

error_stat:
	return MALI_UTF_RESULT_FATAL;
}

/* Parse function Helper shared across most test apps */
int kutf_test_runner_helper_parse_func(mali_utf_test_specifier *test_spec, mali_utf_mempool *pool,
				       const char *arg)
{
	char *arg_copy;

	CSTD_UNUSED(test_spec);

	/* If arg is NULL this is the first call to initialize the parse pool. */
	if (arg == NULL) {
		/* Return non-zero for successful init. */
		return 1;
	} else {
		/* handle additional command line arg. */
		int result = 0;
		int len = strlen(arg);
		/* copy arg to avoid const-qualifier discards */
		arg_copy = mali_utf_mempool_alloc(pool, (len + 1) * sizeof(char));
		if (arg_copy == NULL) {
			mali_utf_logerr("Failed to allocate memory during options parsing\n");
			return result;
		}

		memcpy(arg_copy, arg, len * sizeof(char));
		arg_copy[len] = '\0';
		/* long option form --option */
		if (mali_utf_is_long_opt(arg_copy)) {
			char *value = NULL;

			/* check for supported options */
			if (mali_utf_get_long_opt("--app=", arg_copy, &value)) {
				int err = kutf_test_runner_filter_app_add(value);

				if (err) {
					mali_utf_logerr("Failed to add app string to filter\n");
					return result;
				}
				result = 1;
			} else if (mali_utf_get_long_opt("--kutf_base_dir=", arg_copy, &value)) {
				kutf_base_dir = value;
				result = 1;
			}
		}

		if (result == 0)
			mali_utf_logerr("Unknown argument '%s'\n", arg);

		/* Return non-zero for successful token parse. */
		return result;
	}
}

/* CLI Info function Helper shared across most test apps */
void kutf_test_runner_helper_cli_info_func(void)
{
	mali_tpi_puts("    --app=            Only run selected test app in the kernel.\n"
		      "                      Multiple --app options may be specified to\n"
		      "                      select several apps\n"
		      "    --kutf_base_dir=  Specify the base directory of kutf.\n");
}

static int kutf_test_runner_write_to_debugfs(char *filename, char *buf,
					     int count)
{
	int fd;
	int ret;

	fd = open(filename, O_WRONLY);
	if (fd == -1)
		return errno;

	ret = write(fd, buf, count);

	close(fd);

	if (ret == -1)
		return errno;

	return 0;
}

bool kutf_test_runner_helper_is_kprobe_available(void)
{
	bool kp_available  = false;
	char *debugfs_file = KUTF_KP_REG_FILE;

	if (access(debugfs_file, F_OK) != -1)
		kp_available = true;

	return kp_available;
}

int kutf_test_runner_helper_register_kprobe(char *probe_func_name,
					    bool  entry,
					    char *probe_handler_name,
					    char *format_str,
					    ...)
{
	char buf[KUTF_KP_BUF_SIZE];
	char *p = buf;
	int n;
	int ret;
	int count;
	va_list args;
	char *debugfs_kp_path = KUTF_KP_REG_FILE;
	char *kp_kind_str;

	if ((NULL == probe_func_name) || (NULL == probe_handler_name))
		return -EINVAL;

	if (!kutf_test_runner_helper_is_kprobe_available())
		return -EPERM;

	if (entry)
		kp_kind_str = "entry";
	else
		kp_kind_str = "exit";

	n = snprintf(p, KUTF_KP_BUF_SIZE, "%s %s %s ", probe_func_name,
		     kp_kind_str, probe_handler_name);
	count = n;

	if (n >= KUTF_KP_BUF_SIZE)
		return -EINVAL;
	p += n;

	va_start(args, format_str);
	n = vsnprintf(p, KUTF_KP_BUF_SIZE-n, format_str, args);
	va_end(args);
	count += n;
	/* add extra 1 byte for null char as vsnprintf doesn't count null */
	count += 1;

	ret = kutf_test_runner_write_to_debugfs(debugfs_kp_path,
						buf, count);
	return ret;
}

int kutf_test_runner_helper_unregister_kprobe(char *probe_func_name,
					      bool  entry)
{
	char buf[KUTF_KP_BUF_SIZE];
	int ret;
	int count;
	char *kp_kind_str;
	char *debugfs_kp_path = KUTF_KP_UNREG_FILE;

	if (probe_func_name == NULL)
		return -EINVAL;

	if (!kutf_test_runner_helper_is_kprobe_available())
		return -EPERM;

	if (entry)
		kp_kind_str = "entry";
	else
		kp_kind_str = "exit";

	count = snprintf(buf, KUTF_KP_BUF_SIZE, "%s %s", probe_func_name,
		     kp_kind_str);
	/* add extra 1 byte for null char as snprintf doesn't count null */
	count += 1;

	ret = kutf_test_runner_write_to_debugfs(debugfs_kp_path,
						buf, count);
	return ret;
}
