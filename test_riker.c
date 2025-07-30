// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#define TEST_CUSTOM_MAIN 1

#include "riker.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void setup_error(void)
{
	rk_error("Setup error");
	rk_check_eq(RK_TST_RES, TERROR);
}

static void teardown_error(void)
{
	rk_error("Teardown error");
	rk_check_eq(RK_TST_RES, TERROR);
}

static void setup_test(void)
{
	rk_result(TINFO, "Setup test");
}

static void teardown_test(void)
{
	rk_result(TINFO, "Teardown test");
}

static void setup_suite(void)
{
	rk_result(TINFO, "Setup suite");
}

static void teardown_suite(void)
{
	rk_result(TINFO, "Teardown suite");
}

static void test_pass(void)
{
	rk_result(TPASS, "Test passed");
	rk_check_eq(RK_TST_RES, TPASS);
}

static void test_fail(void)
{
	rk_result(TFAIL, "Test fail");
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_skip(void)
{
	rk_result(TSKIP, "Test skip");
	rk_check_eq(RK_TST_RES, TSKIP);
}

static void test_error(void)
{
	rk_error("Test error");
	rk_check_eq(RK_TST_RES, TERROR);
}

static void test_rk_check_expr(void)
{
	rk_check_expr(10 < 12);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_expr(10 > 12);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_eq(void)
{
	int a = 10;
	int b = 10;
	int c = 20;

	rk_check_eq(a, b);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_eq(a, c);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_ne(void)
{
	int a = 10;
	int b = 10;
	int c = 20;

	rk_check_ne(a, c);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_ne(a, b);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_gt(void)
{
	int a = 10;
	int b = 0;
	int c = 20;

	rk_check_gt(a, b);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_gt(a, c);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_ge(void)
{
	int a = 10;
	int b = 10;
	int c = 20;

	rk_check_ge(a, b);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_ge(a, c);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_lt(void)
{
	int a = 10;
	int b = 20;
	int c = 0;

	rk_check_lt(a, b);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_lt(a, c);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_le(void)
{
	int a = 10;
	int b = 10;
	int c = 0;

	rk_check_le(a, b);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_le(a, c);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_ptr_null(void)
{
	int ptr[] = {1};

	rk_check_ptr_null(NULL);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_ptr_null(ptr);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_ptr_not_null(void)
{
	int ptr[] = {1};

	rk_check_ptr_not_null(ptr);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_ptr_not_null(NULL);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_mem_eq(void)
{
	const char *s1 = "ciao";
	const char *s2 = "ciao";
	const char *s3 = "cia0";

	rk_check_mem_eq(s1, s2, 4);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_mem_eq(s1, s3, 4);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_mem_not_eq(void)
{
	const char *s1 = "ciao";
	const char *s2 = "cia0";
	const char *s3 = "ciao";

	rk_check_mem_not_eq(s1, s2, 4);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_mem_not_eq(s1, s3, 4);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_str_eq(void)
{
	const char *s1 = "ciao";
	const char *s2 = "ciao";
	const char *s3 = "cia0";

	rk_check_str_eq(s1, s2, 4);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_str_eq(s1, s3, 4);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_str_not_eq(void)
{
	const char *s1 = "ciao";
	const char *s2 = "cia0";
	const char *s3 = "ciao";

	rk_check_str_not_eq(s1, s2, 4);
	rk_check_eq(RK_TST_RES, TPASS);

	rk_check_str_not_eq(s1, s3, 4);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_rk_check_eq_ptr(void)
{
	const char *s1 = "ciao";
	const char *s2 = s1;

	rk_check_eq_ptr(s1, s2);
	rk_check_eq(RK_TST_RES, TPASS);
}

static void test_rk_check_ne_ptr(void)
{
	const char *s1 = "ciao";
	const char *s2 = "ciao1";

	rk_check_ne_ptr(s1, s2);
	rk_check_eq(RK_TST_RES, TPASS);
}

static void test_rk_check_assignment(void)
{
	int a = 10;
	int b;

	rk_check_eq(a, b = 11);
	rk_check_eq(RK_TST_RES, TFAIL);
}

static void test_timeout(void)
{
	rk_result(TINFO, "Waiting for timeout..");
	sleep(3);
}

static rk_suite_t test_suite = {
	.tests = (rk_test_t []) {
		{
			.setup = setup_test,
			.run = test_pass,
			.teardown = teardown_test
		},
		{
			.setup = setup_error,
			.run = test_pass,
			.teardown = teardown_test,
		},
		{
			.setup = setup_test,
			.run = test_error,
			.teardown = teardown_test,
		},
		{
			.setup = setup_test,
			.run = test_pass,
			.teardown = teardown_error,
		},
		{ .run = test_pass },
		{ .run = test_fail },
		{ .run = test_skip },
		{ .run = test_rk_check_expr },
		{ .run = test_rk_check_eq },
		{ .run = test_rk_check_ne },
		{ .run = test_rk_check_gt },
		{ .run = test_rk_check_ge },
		{ .run = test_rk_check_lt },
		{ .run = test_rk_check_le },
		{ .run = test_rk_check_ptr_null },
		{ .run = test_rk_check_ptr_not_null },
		{ .run = test_rk_check_mem_eq },
		{ .run = test_rk_check_mem_not_eq },
		{ .run = test_rk_check_str_eq },
		{ .run = test_rk_check_str_not_eq },
		{ .run = test_rk_check_eq_ptr },
		{ .run = test_rk_check_ne_ptr },
		{
			.run = test_timeout,
			.timeout = 1,
		},
		{ .run = test_rk_check_assignment },
		{ .run = NULL },
	},
	.setup = setup_suite,
	.teardown = teardown_suite,
};

int main(void)
{
	pid_t pid;
	int status;

	pid = fork();
	assert(pid != -1);

	if (!pid) {
		rk_run_suite(&test_suite);
		exit(0);
	}

	assert(waitpid(pid, &status, 0) != -1);
	assert(WIFEXITED(status));

	return 0;
}
