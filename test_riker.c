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
}

static void teardown_error(void)
{
	rk_error("Teardown error");
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
}

static void test_fail(void)
{
	rk_result(TFAIL, "Test fail");
}

static void test_skip(void)
{
	rk_result(TSKIP, "Test skip");
}

static void test_error(void)
{
	rk_error("Test error");
}

static void test_rk_check_expr(void)
{
	rk_check_expr(10 < 12);
	rk_check_expr(10 > 12);
}

static void test_rk_check_eq(void)
{
	int a = 10;
	int b = 10;
	int c = 20;

	rk_check_eq(a, b);
	rk_check_eq(a, c);
}

static void test_rk_check_ne(void)
{
	int a = 10;
	int b = 10;
	int c = 20;

	rk_check_ne(a, c);
	rk_check_ne(a, b);
}

static void test_rk_check_gt(void)
{
	int a = 10;
	int b = 0;
	int c = 20;

	rk_check_gt(a, b);
	rk_check_gt(a, c);
}

static void test_rk_check_ge(void)
{
	int a = 10;
	int b = 10;
	int c = 20;

	rk_check_ge(a, b);
	rk_check_ge(a, c);
}

static void test_rk_check_lt(void)
{
	int a = 10;
	int b = 20;
	int c = 0;

	rk_check_lt(a, b);
	rk_check_lt(a, c);
}

static void test_rk_check_le(void)
{
	int a = 10;
	int b = 10;
	int c = 0;

	rk_check_le(a, b);
	rk_check_le(a, c);
}

static void test_rk_check_ptr_null(void)
{
	int *ptr1 = NULL;
	int *ptr2 = malloc(sizeof(int));

	rk_check_ptr_null(ptr1);
	rk_check_ptr_null(ptr2);

	free(ptr2);
}

static void test_rk_check_ptr_not_null(void)
{
	int *ptr1 = malloc(sizeof(int));
	int *ptr2 = NULL;

	rk_check_ptr_not_null(ptr1);
	rk_check_ptr_not_null(ptr2);

	free(ptr1);
}

static void test_rk_check_mem_eq(void)
{
	const char *s1 = "ciao";
	const char *s2 = "ciao";
	const char *s3 = "cia0";

	rk_check_mem_eq(s1, s2, 4);
	rk_check_mem_eq(s1, s3, 4);
}

static void test_rk_check_eq_ptr(void)
{
	const char *s1 = "ciao";
	char *s2 = s1;

	rk_check_eq_ptr(s1, s2);
}

static void test_rk_check_ne_ptr(void)
{
	const char *s1 = "ciao";
	const char *s2 = "ciao2";

	rk_check_ne_ptr(s1, s2);
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
		{ .run = test_rk_check_eq_ptr },
		{ .run = test_rk_check_ne_ptr },
		{
			.run = test_timeout,
			.timeout = 1,
		},
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
