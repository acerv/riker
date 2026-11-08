// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#define TEST_CUSTOM_MAIN 1

#include "test_helper.h"
#include <errno.h>
#include <string.h>

static int eval_count;

static int next_val(void)
{
	eval_count++;
	return 10;
}

static void fn_c99_checks(void)
{
	int a = 10;
	int b = 10;
	int c = 20;
	const char *s1 = "abc";
	const char *s2 = "abd";

	rk_check_eq(a, b);
	ASSERT_TRUE(RK_TST_RES == TPASS);

	rk_check_eq(a, c);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_ne(a, c);
	ASSERT_TRUE(RK_TST_RES == TPASS);

	rk_check_lt(a, c);
	ASSERT_TRUE(RK_TST_RES == TPASS);

	rk_check_str_eq(s1, s2, 3);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_cstr_eq(s1, "abc");
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_cstr_ne(s1, s2);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_cstr_eq(NULL, s1);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_mem_eq(NULL, s1, 3);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_mem_ne(s1, NULL, 3);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_near(1.005, 1.0, 0.01);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_near(1.05, 1.0, 0.01);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	errno = ENOENT;
	rk_check_errno(ENOENT);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_result_errno(TFAIL, "c99 open");
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	eval_count = 0;
	rk_check_eq(next_val(), 20);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	ASSERT_TRUE(eval_count == 1);

	eval_count = 0;
	rk_check_near(next_val(), 20.0, 1.0);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	ASSERT_TRUE(eval_count == 1);
}

static void fn_c99_brk(void)
{
	rk_require(1 == 1);
	rk_result(TINFO, "after require pass");
	rk_require(1 == 2);
	rk_result(TPASS, "unreachable after require fail");
}

static void fn_c99_pass(void)
{
	rk_check_eq(5, 5);
	ASSERT_TRUE(RK_TST_RES == TPASS);
}

int main(void)
{
	char out[4096];
	int rc;

	rk_suite_t s_pass = {
		.tests = (rk_test_t []) {
			{ .run = fn_c99_pass },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_pass, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_PASSED);
	strip_ansi(out);
	ASSERT_TRUE(strstr(out, "PASS 5 == 5") != NULL);

	rk_suite_t s_checks = {
		.tests = (rk_test_t []) {
			{ .run = fn_c99_checks },
			{ .run = fn_c99_brk },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s_checks, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	strip_ansi(out);
	ASSERT_TRUE(strstr(out, "PASS a == b") != NULL);
	ASSERT_TRUE(strstr(out, "FAIL a == c") != NULL);
	ASSERT_TRUE(strstr(out, "FAIL s1 != s2 (s1 = abc, s2 = abd)") != NULL);
	ASSERT_TRUE(strstr(out, "FAIL |1.05 - 1.0| <= 0.01") != NULL);
	ASSERT_TRUE(strstr(out, "PASS errno == ENOENT") != NULL);
	ASSERT_TRUE(strstr(out, "FAIL c99 open:") != NULL);
	ASSERT_TRUE(strstr(out, "after require pass") != NULL);
	ASSERT_TRUE(strstr(out, "unreachable after require fail") == NULL);

	return 0;
}
