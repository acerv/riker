// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#define TEST_CUSTOM_MAIN 1

#include "test_helper.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
	char log[512];
	size_t len;
	int run_called;
	int teardown_calls;
} trace_buf_t;

static trace_buf_t *trace;

static void trace_append(const char *tag)
{
	size_t n = strlen(tag);

	if (trace && trace->len + n < sizeof(trace->log)) {
		memcpy(trace->log + trace->len, tag, n);
		trace->len += n;
		trace->log[trace->len] = '\0';
	}
}

static void trace_reset(void)
{
	memset(trace, 0, sizeof(*trace));
}

static void hook_suite_setup(void) { trace_append("S+"); }
static void hook_suite_teardown(void) { trace_append("S-"); }
static void hook_t1_setup(void) { trace_append("1+"); }
static void hook_t1_run(void) { trace_append("1*"); rk_result(TPASS, "t1"); }
static void hook_t1_teardown(void) { trace_append("1-"); }
static void hook_t2_run(void) { trace_append("2*"); rk_result(TPASS, "t2"); }
static void hook_only_setup(void) { trace_append("O+"); rk_result(TPASS, "os"); }
static void hook_only_teardown(void) { trace_append("O-"); }

static void fn_pass(void) { rk_result(TPASS, "pass item"); }
static void fn_fail(void) { rk_result(TFAIL, "fail item"); }
static void fn_skip(void) { rk_result(TSKIP, "skip item"); }
static void fn_error(void) { rk_error("error item"); }

static void hook_setup_error(void)
{
	trace_append("E+");
	rk_error("setup failed");
}

static void hook_setup_fail(void)
{
	trace_append("F+");
	rk_check_eq(1, 2);
}

static void hook_guarded_run(void)
{
	trace->run_called++;
	trace_append("R*");
	rk_result(TPASS, "should not run");
}

static void hook_counted_teardown(void)
{
	trace->teardown_calls++;
	trace_append("T-");
}

static void hook_run_error(void)
{
	trace_append("RE*");
	rk_error("run failed");
}

static void hook_teardown_fail(void)
{
	rk_result(TFAIL, "suite teardown failed");
}

static void hook_teardown_error(void)
{
	rk_error("suite teardown error");
}

static void test_exit_codes_and_summary(void)
{
	char out[4096];
	int rc;

	rk_suite_t s_pass = {
		.tests = (rk_test_t []) {
			{ .run = fn_pass },
			{ .run = fn_pass },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_pass, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_PASSED);
	strip_ansi(out);
	ASSERT_TRUE(strstr(out, "Passed:  2") != NULL);
	ASSERT_TRUE(strstr(out, "Failed:  0") != NULL);
	ASSERT_TRUE(strstr(out, "Skipped: 0") != NULL);
	ASSERT_TRUE(strstr(out, "Errors:  0") != NULL);

	rk_suite_t s_skip = {
		.tests = (rk_test_t []) {
			{ .run = fn_pass },
			{ .run = fn_skip },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_skip, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_SKIPPED);
	strip_ansi(out);
	ASSERT_TRUE(strstr(out, "Passed:  1") != NULL);
	ASSERT_TRUE(strstr(out, "Skipped: 1") != NULL);

	rk_suite_t s_skip_fail = {
		.tests = (rk_test_t []) {
			{ .run = fn_skip },
			{ .run = fn_fail },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_skip_fail, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);

	rk_suite_t s_skip_err = {
		.tests = (rk_test_t []) {
			{ .run = fn_skip },
			{ .run = fn_error },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_skip_err, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
}

static void test_suite_teardown_affects_result(void)
{
	char out[4096];
	int rc;

	rk_suite_t s_td_fail = {
		.tests = (rk_test_t []) {
			{ .run = fn_pass },
			{ .run = NULL },
		},
		.teardown = hook_teardown_fail,
	};
	ASSERT_TRUE(run_suite_proc(&s_td_fail, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);

	rk_suite_t s_td_err = {
		.tests = (rk_test_t []) {
			{ .run = fn_pass },
			{ .run = NULL },
		},
		.teardown = hook_teardown_error,
	};
	ASSERT_TRUE(run_suite_proc(&s_td_err, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);

	rk_suite_t s_both_err = {
		.tests = (rk_test_t []) {
			{ .run = fn_pass },
			{ .run = NULL },
		},
		.setup = hook_setup_error,
		.teardown = hook_teardown_error,
	};
	ASSERT_TRUE(run_suite_proc(&s_both_err, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
}

static void test_lifecycle_and_isolation(void)
{
	char out[4096];
	int rc;

	trace_reset();
	rk_suite_t s_order = {
		.setup = hook_suite_setup,
		.teardown = hook_suite_teardown,
		.tests = (rk_test_t []) {
			{
				.setup = hook_t1_setup,
				.run = hook_t1_run,
				.teardown = hook_t1_teardown,
			},
			{ .run = hook_t2_run },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_order, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_PASSED);
	ASSERT_TRUE(strcmp(trace->log, "S+1+1*1-2*S-") == 0);

	trace_reset();
	rk_suite_t s_setup_err = {
		.tests = (rk_test_t []) {
			{
				.setup = hook_setup_error,
				.run = hook_guarded_run,
				.teardown = hook_counted_teardown,
			},
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_setup_err, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	ASSERT_TRUE(trace->run_called == 0);
	ASSERT_TRUE(trace->teardown_calls == 1);
	ASSERT_TRUE(strcmp(trace->log, "E+T-") == 0);

	trace_reset();
	rk_suite_t s_setup_fail = {
		.tests = (rk_test_t []) {
			{
				.setup = hook_setup_fail,
				.run = hook_guarded_run,
				.teardown = hook_counted_teardown,
			},
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_setup_fail, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	ASSERT_TRUE(trace->run_called == 0);
	ASSERT_TRUE(trace->teardown_calls == 1);
	ASSERT_TRUE(strcmp(trace->log, "F+T-") == 0);

	trace_reset();
	rk_suite_t s_run_err = {
		.tests = (rk_test_t []) {
			{
				.run = hook_run_error,
				.teardown = hook_counted_teardown,
			},
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_run_err, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	ASSERT_TRUE(trace->teardown_calls == 1);
	ASSERT_TRUE(strcmp(trace->log, "RE*T-") == 0);
}

static void test_structural_edge_cases(void)
{
	char out[4096];
	int rc;

	trace_reset();
	rk_suite_t s_empty_run = {
		.tests = (rk_test_t []) {
			{
				.setup = hook_only_setup,
				.teardown = hook_only_teardown,
			},
			{ .run = hook_t2_run },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_empty_run, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_PASSED);
	ASSERT_TRUE(strcmp(trace->log, "O+O-2*") == 0);

	trace_reset();
	rk_suite_t s_empty_arr = {
		.setup = hook_suite_setup,
		.teardown = hook_suite_teardown,
		.tests = (rk_test_t []) {
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_empty_arr, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_PASSED);
	ASSERT_TRUE(strcmp(trace->log, "S+S-") == 0);

	trace_reset();
	rk_suite_t s_null_tests = {
		.setup = hook_suite_setup,
		.teardown = hook_suite_teardown,
		.tests = NULL,
	};
	ASSERT_TRUE(run_suite_proc(&s_null_tests, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_PASSED);
	ASSERT_TRUE(strcmp(trace->log, "S+S-") == 0);
}

static void test_null_session_safety(void)
{
	pid_t pid;
	int status;

	fflush(NULL);
	pid = fork();
	ASSERT_TRUE(pid != -1);

	if (!pid) {
		rk_result(TPASS, "standalone pass");
		rk_result(TFAIL, "standalone fail");
		rk_result(TSKIP, "standalone skip");
		rk_error("standalone error");
		rk_check_eq(1, 1);
		_exit(0);
	}

	ASSERT_TRUE(waitpid(pid, &status, 0) != -1);
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_TRUE(WEXITSTATUS(status) == 0);
}

int main(void)
{
	void *mem = mmap(NULL, sizeof(*trace), PROT_READ | PROT_WRITE,
			 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	ASSERT_TRUE(mem != MAP_FAILED);
	trace = mem;

	test_exit_codes_and_summary();
	test_suite_teardown_affects_result();
	test_lifecycle_and_isolation();
	test_structural_edge_cases();
	test_null_session_safety();

	ASSERT_TRUE(munmap(trace, sizeof(*trace)) == 0);
	return 0;
}
