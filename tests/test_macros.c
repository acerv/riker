// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#define TEST_CUSTOM_MAIN 1

#include "test_helper.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int eval_a_count;
static int eval_b_count;

static int next_a(void)
{
	eval_a_count++;
	return 10;
}

static int next_b_same(void)
{
	eval_b_count++;
	return 10;
}

static int next_b_diff(void)
{
	eval_b_count++;
	return 20;
}

static void fn_tinfo_preserves_res(void)
{
	rk_result(TPASS, "set pass");
	ASSERT_TRUE(RK_TST_RES == TPASS);

	rk_result(TINFO, "info only");
	ASSERT_TRUE(RK_TST_RES == TPASS);

	rk_result(TFAIL, "set fail");
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_result(TINFO, "info after fail");
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_result(TSKIP, "set skip");
	ASSERT_TRUE(RK_TST_RES == TSKIP);

	rk_error("set error");
	ASSERT_TRUE(RK_TST_RES == TERROR);
}

static void fn_check_expr_and_num(void)
{
	int a = 10;
	int b = 10;
	int c = 20;

	rk_check_expr(10 < 12);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_expr(10 > 12);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_eq(a, b);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_eq(a, c);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_ne(a, c);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_ne(a, b);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_gt(c, a);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_gt(a, c);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_ge(a, b);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_ge(a, c);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_lt(a, c);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_lt(c, a);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_le(a, b);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_le(c, a);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
}

static void fn_check_ptr_mem_str(void)
{
	int arr[] = {1, 2};
	const char *s1 = "ciao";
	const char *s2 = "ciao";
	const char *s3 = "cia0";

	rk_check_ptr_null(NULL);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_ptr_null(arr);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_ptr_not_null(arr);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_ptr_not_null(NULL);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_ptr_eq(s1, s2);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_ptr_eq(s1, s3);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_ptr_ne(s1, s3);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_ptr_ne(s1, s2);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_mem_eq(s1, s2, 4);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_mem_eq(s1, s3, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_mem_ne(s1, s3, 4);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_mem_ne(s1, s2, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_mem_eq(NULL, s1, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_mem_ne(s1, NULL, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_mem_eq(NULL, NULL, 0);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_str_eq(s1, s2, 4);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_str_eq(s1, s3, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_str_ne(s1, s3, 4);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_str_ne(s1, s2, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	char *raw1 = malloc(4);
	char *raw2 = malloc(4);
	ASSERT_TRUE(raw1 != NULL && raw2 != NULL);
	memcpy(raw1, "abcd", 4);
	memcpy(raw2, "abce", 4);

	rk_check_str_eq(raw1, raw1, 4);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_str_ne(raw1, raw2, 4);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_str_eq(NULL, raw1, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_str_ne(raw1, NULL, 4);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	rk_check_cstr_eq(s1, s2);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_cstr_eq(s1, "cia");
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_cstr_ne(s1, "cia");
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_cstr_ne(s1, s2);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_cstr_eq(NULL, s1);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_cstr_ne(s1, NULL);
	ASSERT_TRUE(RK_TST_RES == TFAIL);

	free(raw1);
	free(raw2);
}

static void fn_single_evaluation(void)
{
	int a = 10;
	int b = 0;

	eval_a_count = 0;
	eval_b_count = 0;
	rk_check_eq(next_a(), next_b_same());
	ASSERT_TRUE(RK_TST_RES == TPASS);
	ASSERT_TRUE(eval_a_count == 1);
	ASSERT_TRUE(eval_b_count == 1);

	eval_a_count = 0;
	eval_b_count = 0;
	rk_check_eq(next_a(), next_b_diff());
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	ASSERT_TRUE(eval_a_count == 1);
	ASSERT_TRUE(eval_b_count == 1);

	rk_check_eq(a, b = 11);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	ASSERT_TRUE(b == 11);
}

static void fn_type_matrix(void)
{
	_Bool b1 = 1, b2 = 0;
	char c1 = 'A', c2 = 'B';
	signed char sc1 = -42, sc2 = 12;
	unsigned char uc1 = 250, uc2 = 1;
	short sh1 = -1234, sh2 = 567;
	unsigned short us1 = 65535, us2 = 1;
	int i1 = -100, i2 = 200;
	unsigned int ui1 = 4000000000U, ui2 = 1U;
	long l1 = -1000000L, l2 = 2L;
	unsigned long ul1 = 4000000000UL, ul2 = 3UL;
	long long ll1 = -900000000000LL, ll2 = 4LL;
	unsigned long long ull1 = 18000000000000ULL, ull2 = 5ULL;
	float f1 = 1.5f, f2 = 2.5f;
	double d1 = 1.5, d2 = 2.5;
	long double ld1 = 1.5L, ld2 = 2.5L;
	char ms1[] = "foo";
	char ms2[] = "bar";
	const char *cs1 = "abc";
	const char *cs2 = "def";
	int x = 1, y = 2;
	void *p1 = &x;
	void *p2 = &y;

	rk_check_eq(b1, b2);
	rk_check_eq(c1, c2);
	rk_check_eq(sc1, sc2);
	rk_check_eq(uc1, uc2);
	rk_check_eq(sh1, sh2);
	rk_check_eq(us1, us2);
	rk_check_eq(i1, i2);
	rk_check_eq(ui1, ui2);
	rk_check_eq(l1, l2);
	rk_check_eq(ul1, ul2);
	rk_check_eq(ll1, ll2);
	rk_check_eq(ull1, ull2);
	rk_check_gt(f1, f2);
	rk_check_gt(d1, d2);
	rk_check_gt(ld1, ld2);

	volatile double z = 0.0;
	double nan_v = z / z;

	rk_check_near(1.005f, 1.0f, 0.01f);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_near(1.05, 1.0, 0.01);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_near(1.0001L, 1.0L, 0.001L);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	rk_check_near(1.0, 1.0, -0.1);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_near(nan_v, 1.0, 1.0);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	rk_check_eq(ms1, ms2);
	rk_check_eq(cs1, cs2);
	rk_check_eq(p1, p2);
}

static void fn_truncation_bounds(void)
{
	char *s1 = malloc(2500);
	char *s2 = malloc(2500);
	char long_msg[1500];

	ASSERT_TRUE(s1 != NULL);
	ASSERT_TRUE(s2 != NULL);
	memset(s1, 'a', 2499);
	s1[2499] = '\0';
	memset(s2, 'b', 2499);
	s2[2499] = '\0';

	/* Combined debug string exceeds 4096 bytes in RK_CHECK_NUM_ */
	rk_check_eq(s1, s2);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	free(s1);
	free(s2);

	memset(long_msg, 'Z', sizeof(long_msg) - 1);
	long_msg[sizeof(long_msg) - 1] = '\0';
	rk_result(TPASS, "%s", long_msg);
	ASSERT_TRUE(RK_TST_RES == TPASS);
}

static void test_tinfo_behavior(void)
{
	char out[2048];
	int rc;

	rk_suite_t s = {
		.tests = (rk_test_t []) {
			{ .run = fn_tinfo_preserves_res },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
}

static void test_checks_and_output(void)
{
	char out[8192];
	int rc;

	rk_suite_t s = {
		.tests = (rk_test_t []) {
			{ .run = fn_check_expr_and_num },
			{ .run = fn_check_ptr_mem_str },
			{ .run = fn_single_evaluation },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	strip_ansi(out);

	ASSERT_TRUE(strstr(out, "FAIL a == c (a = 10, c = 20)") != NULL);
	ASSERT_TRUE(strstr(out, "FAIL s1 != s3 (s1 = ciao, s3 = cia0)") != NULL);
	ASSERT_TRUE(strstr(out, "0x0x") == NULL);
	ASSERT_TRUE(strstr(out, "(0x") != NULL);
}

static void test_type_formatting(void)
{
	char out[8192];
	int rc;

	rk_suite_t s = {
		.tests = (rk_test_t []) {
			{ .run = fn_type_matrix },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	strip_ansi(out);

	ASSERT_TRUE(strstr(out, "(b1 = 1, b2 = 0)") != NULL);
	ASSERT_TRUE(strstr(out, "(c1 = A, c2 = B)") != NULL);
	ASSERT_TRUE(strstr(out, "(sc1 = -42, sc2 = 12)") != NULL);
	ASSERT_TRUE(strstr(out, "(uc1 = 250, uc2 = 1)") != NULL);
	ASSERT_TRUE(strstr(out, "(sh1 = -1234, sh2 = 567)") != NULL);
	ASSERT_TRUE(strstr(out, "(us1 = 65535, us2 = 1)") != NULL);
	ASSERT_TRUE(strstr(out, "(i1 = -100, i2 = 200)") != NULL);
	ASSERT_TRUE(strstr(out, "(ui1 = 4000000000, ui2 = 1)") != NULL);
	ASSERT_TRUE(strstr(out, "(l1 = -1000000, l2 = 2)") != NULL);
	ASSERT_TRUE(strstr(out, "(ul1 = 4000000000, ul2 = 3)") != NULL);
	ASSERT_TRUE(strstr(out, "(ll1 = -900000000000, ll2 = 4)") != NULL);
	ASSERT_TRUE(strstr(out, "(ull1 = 18000000000000, ull2 = 5)") != NULL);
	ASSERT_TRUE(strstr(out, "(f1 = 1.5, f2 = 2.5)") != NULL);
	ASSERT_TRUE(strstr(out, "(d1 = 1.5, d2 = 2.5)") != NULL);
	ASSERT_TRUE(strstr(out, "(ld1 = 1.5, ld2 = 2.5)") != NULL);
	ASSERT_TRUE(strstr(out, "(ms1 = foo, ms2 = bar)") != NULL);
	ASSERT_TRUE(strstr(out, "(cs1 = abc, cs2 = def)") != NULL);
	ASSERT_TRUE(strstr(out, "(p1 = 0x") != NULL);
}

static void test_truncation(void)
{
	char out[16384];
	int rc;

	rk_suite_t s = {
		.tests = (rk_test_t []) {
			{ .run = fn_truncation_bounds },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	strip_ansi(out);
	ASSERT_TRUE(strstr(out, "Passed:  1") != NULL);
	ASSERT_TRUE(strstr(out, "Failed:  1") != NULL);
}

static void fn_errno_checks(void)
{
	errno = ENOENT;
	rk_check_errno(ENOENT);
	ASSERT_TRUE(RK_TST_RES == TPASS);
	ASSERT_TRUE(errno == ENOENT);

	rk_check_errno(EACCES);
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	ASSERT_TRUE(errno == ENOENT);

	rk_result_errno(TFAIL, "open %s", "/missing");
	ASSERT_TRUE(RK_TST_RES == TFAIL);
	ASSERT_TRUE(errno == ENOENT);
}

static void test_errno_helpers(void)
{
	char out[4096];
	int rc;

	rk_suite_t s = {
		.tests = (rk_test_t []) {
			{ .run = fn_errno_checks },
			{ .run = NULL },
		},
	};
	ASSERT_TRUE(run_suite_proc(&s, &rc, out, sizeof(out)) == 0);
	ASSERT_TRUE(rc == RK_FAILED);
	strip_ansi(out);
	ASSERT_TRUE(strstr(out, "PASS errno == ENOENT") != NULL);
	ASSERT_TRUE(strstr(out, "FAIL errno != EACCES") != NULL);
	ASSERT_TRUE(strstr(out, "FAIL open /missing:") != NULL);
}

int main(void)
{
	test_tinfo_behavior();
	test_checks_and_output();
	test_type_formatting();
	test_truncation();
	test_errno_helpers();
	return 0;
}
