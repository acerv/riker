// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

/**
 * @file riker.h
 * @brief Public API for the Riker C testing framework.
 */

#ifndef RIKER_H
#define RIKER_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stddef.h>

/** @brief Latest test result, updated by @ref rk_result (except `TINFO`) and @ref rk_error. */
extern int RK_TST_RES;

/**
 * @brief Test result type.
 *
 * Input parameter for the @ref rk_result macro, defining the result
 * of a check or message.
 */
typedef enum
{
	/** @brief Error message. */
	TERROR = -1,
	/** @brief Test informative message. */
	TINFO = 0,
	/** @brief Test passed result message. */
	TPASS,
	/** @brief Test failed result message. */
	TFAIL,
	/** @brief Test skipped result message. */
	TSKIP,
} rk_test_result_t;

/**
 * @brief Testing suite runner result.
 *
 * Exit codes passed to `exit()` by @ref rk_run_suite after running a suite.
 */
typedef enum 
{
	/** @brief An error occurred during runner execution. */
	RK_ERROR = -1,
	/** @brief All tests passed. */
	RK_PASSED = 0,
	/** @brief One or more tests failed or reported errors. */
	RK_FAILED,
	/** @brief No tests failed or errored, and at least one was skipped. */
	RK_SKIPPED,
} rk_suite_result_t;

/** @brief Callback type for suite and test setup, run, and teardown hooks. */
typedef void (*rk_test_func)(void);

/**
 * @brief Represent a test.
 *
 * This struct has to be initialized in order to declare a test.
 */
typedef struct
{
	/** @brief Setup function executed before `run`. */
	rk_test_func setup;
	/** @brief Teardown function executed after `run`. */
	rk_test_func teardown;
	/** @brief Test to execute. */
	rk_test_func run;
	/** @brief Test timeout in seconds (0 uses default 600s). */
	size_t timeout;
} rk_test_t;

/**
 * @brief Represent a testing suite.
 *
 * This struct has to be initialized in order to declare a testing suite.
 * Declaration of a testing suite can be done in the following way:
 *
 * @code
 * rk_suite_t test_suite = {
 *	.tests = (rk_test_t []) {
 *		{
 *			.run = test1,
 *			.setup = setup1,
 *			.teardown = teardown1,
 *			.timeout = 20,
 *		},
 *		{ .run = test2 },
 *		{ .run = NULL },
 *	},
 *	.setup = setup_suite,
 *	.teardown = teardown_suite,
 * };
 * @endcode
 *
 * The object is used automatically by the default @ref main function when
 * `TEST_CUSTOM_MAIN` is not defined. Make sure that its name is `test_suite`.
 */
typedef struct
{
	/** @brief Setup function executed before all `tests`. */
	rk_test_func setup;
	/** @brief Teardown function executed after all `tests`. */
	rk_test_func teardown;
	/** @brief List of tests to execute, terminated by an all-NULL entry. */
	rk_test_t *tests;
} rk_suite_t;

/**
 * @brief Internal helper backing @ref rk_result and @ref rk_error.
 *
 * @param file Source file path where the result was reported.
 * @param lineno Source line number where the result was reported.
 * @param ttype Result classification.
 * @param fmt printf-style format string.
 * @param ... Arguments for `fmt`.
 */
void rk_result_(const char *file, const int lineno, rk_test_result_t ttype,
		const char *fmt, ...)
		__attribute__ ((format (printf, 4, 5)));

/**
 * @brief Internal helper backing @ref rk_result_errno.
 *
 * @param file Source file path where the result was reported.
 * @param lineno Source line number where the result was reported.
 * @param ttype Result classification.
 * @param errnum Captured `errno` value to append via `strerror()`.
 * @param fmt printf-style format string.
 * @param ... Arguments for `fmt`.
 */
void rk_result_errno_(const char *file, const int lineno,
		rk_test_result_t ttype, int errnum,
		const char *fmt, ...)
		__attribute__ ((format (printf, 5, 6)));

/**
 * @brief Internal helper backing @ref rk_brk and @ref rk_require.
 *
 * @param file Source file path where the break was reported.
 * @param lineno Source line number where the break was reported.
 * @param ttype Result classification.
 * @param fmt printf-style format string.
 * @param ... Arguments for `fmt`.
 */
void rk_brk_(const char *file, const int lineno, rk_test_result_t ttype,
		const char *fmt, ...)
		__attribute__ ((format (printf, 4, 5)));

/** @cond INTERNAL */

#if __STDC_VERSION__ >= 201112L

#define RK_PRINT_FMT_(...) \
	_Generic((__VA_ARGS__), \
		_Bool: "%d", \
		char*: "%s", \
		const char*: "%s", \
		char: "%c", \
		signed char: "%hhi", \
		short: "%hi", \
		int: "%i", \
		long: "%li", \
		long long: "%lli", \
		unsigned char: "%hhu", \
		unsigned short: "%hu", \
		unsigned int: "%u", \
		unsigned long: "%lu", \
		unsigned long long: "%llu", \
		float: "%g", \
		double: "%g", \
		long double: "%Lg", \
		default: "%p")

static inline int rk_snprintf_float_(char *buf, size_t size, const char *fmt,
				     float val)
{
	(void)fmt;
	return snprintf(buf, size, "%g", (double)val);
}

#define RK_SNPRINTF_NUM_(ret, buf, buf_size, num) \
do { \
	if (buf_size > 0) { \
		int sn_ret__ = _Generic((num), \
			float: rk_snprintf_float_, \
			default: snprintf)(buf, buf_size, \
					   RK_PRINT_FMT_((num)), (num)); \
		(ret) = sn_ret__ > 0 ? (size_t)sn_ret__ : (size_t)0; \
	} \
} while(0)

#define RK_SNPRINTF_NUM_ARGS_DEBUG_(buf, buf_size, str_a, val_a, str_b, val_b) \
do { \
	char a_buf__[1024] = {0}; \
	char b_buf__[1024] = {0}; \
	size_t len_a__ = 0; \
	size_t len_b__ = 0; \
	RK_SNPRINTF_NUM_(len_a__, a_buf__, sizeof(a_buf__), (val_a)); \
	RK_SNPRINTF_NUM_(len_b__, b_buf__, sizeof(b_buf__), (val_b)); \
	(void)len_a__; \
	(void)len_b__; \
	snprintf(buf, buf_size, "(%s = %s, %s = %s)", \
		 (str_a), a_buf__, (str_b), b_buf__); \
} while(0)
#else
#define RK_SNPRINTF_NUM_(ret, buf, buf_size, num) (ret = 0)
#define RK_SNPRINTF_NUM_ARGS_DEBUG_(buf, buf_size, str_a, val_a, str_b, val_b) \
do { \
	size_t s__ = buf_size; \
	if (s__ == buf_size) {} \
} while(0)
#endif

#define RK_CHECK_NUM_(a, b, op) \
do { \
	__extension__ __auto_type _rk_a = (a); \
	__extension__ __auto_type _rk_b = (b); \
	int ttype__ = (((_rk_a) op (_rk_b)) ? TPASS : TFAIL); \
	if (ttype__ == TFAIL) { \
		char buf__[4096] = {0}; \
		size_t buf_size__ = 4096; \
		RK_SNPRINTF_NUM_ARGS_DEBUG_(buf__, buf_size__, #a, _rk_a, #b, _rk_b); \
		rk_result(ttype__, "%s %s %s %s", #a, #op, #b, buf__); \
	} else { \
		rk_result(ttype__, "%s %s %s", #a, #op, #b); \
	} \
} while(0)

/** @endcond */

/**
 * @brief Send a test result to stdout.
 *
 * Print a formatted test result to stdout, update session counters, and store
 * `ttype` in @ref RK_TST_RES when `ttype` is not `TINFO`.
 *
 * @param ttype Message type.
 * @param arg_fmt printf-style format string.
 * @param ... Arguments for `arg_fmt`.
 */
#define rk_result(ttype, arg_fmt, ...) do { \
	if (ttype != TINFO) \
		RK_TST_RES = ttype; \
	rk_result_(__FILE__, __LINE__, (ttype), (arg_fmt), ##__VA_ARGS__); \
} while (0)

/**
 * @brief Send a test result with captured `errno` description to stdout.
 *
 * Captures `errno` before formatting, appends `: <strerror(errno)> (errno)`
 * to the message, updates @ref RK_TST_RES when `ttype` is not `TINFO`, and
 * preserves `errno`.
 *
 * @param ttype Message type.
 * @param arg_fmt printf-style format string.
 * @param ... Arguments for `arg_fmt`.
 */
#define rk_result_errno(ttype, arg_fmt, ...) do { \
	int _rk_err = errno; \
	if (ttype != TINFO) \
		RK_TST_RES = ttype; \
	rk_result_errno_(__FILE__, __LINE__, (ttype), _rk_err, \
		(arg_fmt), ##__VA_ARGS__); \
} while (0)

/**
 * @brief Verify that `errno` matches `expected`.
 *
 * Captures `errno` immediately, reports `TPASS` if it equals `expected` or
 * `TFAIL` with descriptions otherwise, and preserves `errno`.
 *
 * @param expected Expected `errno` value.
 */
#define rk_check_errno(expected) do { \
	int _rk_err = errno; \
	int _rk_exp = (expected); \
	if (_rk_err == _rk_exp) { \
		rk_result(TPASS, "errno == %s (%d: %s)", \
			#expected, _rk_err, strerror(_rk_err)); \
	} else { \
		rk_result(TFAIL, "errno != %s (got %d: %s, expected %d: %s)", \
			#expected, _rk_err, strerror(_rk_err), \
			_rk_exp, strerror(_rk_exp)); \
	} \
	errno = _rk_err; \
} while (0)

/**
 * @brief Report a test error to stdout.
 *
 * Print an error message to stdout, increment the session error counter, and
 * set @ref RK_TST_RES to `TERROR`. If called from a test setup hook, the
 * test `run` hook is skipped while `teardown` still executes.
 *
 * @param arg_fmt printf-style format string.
 * @param ... Arguments for `arg_fmt`.
 */
#define rk_error(arg_fmt, ...) do { \
	RK_TST_RES = TERROR; \
	rk_result_(__FILE__, __LINE__, TERROR, (arg_fmt), ##__VA_ARGS__); \
} while (0)

/**
 * @brief Report a result and abort the active test hook.
 *
 * Behaves like @ref rk_result, then aborts the current `setup` or `run`
 * hook when executed inside a test. Teardown still executes normally.
 *
 * @param ttype Message type.
 * @param arg_fmt printf-style format string.
 * @param ... Arguments for `arg_fmt`.
 */
#define rk_brk(ttype, arg_fmt, ...) do { \
	if (ttype != TINFO) \
		RK_TST_RES = ttype; \
	rk_brk_(__FILE__, __LINE__, (ttype), (arg_fmt), ##__VA_ARGS__); \
} while (0)

/**
 * @brief Verify that `expr` is satisfied.
 *
 * Verify that `expr` evaluates to non-zero and report TPASS, or TFAIL otherwise.
 *
 * @param expr An expression that has to be satisfied.
 */
#define rk_check_expr(expr) \
	rk_result(((expr) ? TPASS : TFAIL), "%s", #expr)

/**
 * @brief Require `expr` to be satisfied or abort the current test.
 *
 * Reports `TPASS` if `expr` evaluates to non-zero; otherwise reports `TFAIL`
 * and aborts the active test hook via @ref rk_brk.
 *
 * @param expr Condition that must hold to continue the test.
 */
#define rk_require(expr) do { \
	if (expr) \
		rk_result(TPASS, "%s", #expr); \
	else \
		rk_brk(TFAIL, "%s", #expr); \
} while (0)

/**
 * @brief Verify that two numbers are equal.
 *
 * Evaluate `a` and `b` once and verify that they are equal. Under C11 or newer,
 * failed checks include formatted operand values.
 *
 * @param a First value.
 * @param b Second value.
 */
#define rk_check_eq(a, b) RK_CHECK_NUM_(a, b, ==)

/**
 * @brief Verify that two numbers are different.
 *
 * Evaluate `a` and `b` once and verify that they do not have the same value.
 * Under C11 or newer, failed checks include formatted operand values.
 *
 * @param a First value.
 * @param b Second value.
 */
#define rk_check_ne(a, b) RK_CHECK_NUM_(a, b, !=)

/**
 * @brief Verify that the first number is greater than the second.
 *
 * Evaluate `a` and `b` once and verify that `a` is greater than `b`. Under C11
 * or newer, failed checks include formatted operand values.
 *
 * @param a First value.
 * @param b Second value.
 */
#define rk_check_gt(a, b) RK_CHECK_NUM_(a, b, >)

/**
 * @brief Verify that the first number is greater than or equal to the second.
 *
 * Evaluate `a` and `b` once and verify that `a` is greater than or equal to `b`.
 * Under C11 or newer, failed checks include formatted operand values.
 *
 * @param a First value.
 * @param b Second value.
 */
#define rk_check_ge(a, b) RK_CHECK_NUM_(a, b, >=)

/**
 * @brief Verify that the first number is less than the second.
 *
 * Evaluate `a` and `b` once and verify that `a` is less than `b`. Under C11 or
 * newer, failed checks include formatted operand values.
 *
 * @param a First value.
 * @param b Second value.
 */
#define rk_check_lt(a, b) RK_CHECK_NUM_(a, b, <)

/**
 * @brief Verify that the first number is less than or equal to the second.
 *
 * Evaluate `a` and `b` once and verify that `a` is less than or equal to `b`.
 * Under C11 or newer, failed checks include formatted operand values.
 *
 * @param a First value.
 * @param b Second value.
 */
#define rk_check_le(a, b) RK_CHECK_NUM_(a, b, <=)

/**
 * @brief Verify that two values are within an absolute tolerance.
 *
 * Evaluate `a`, `b`, and `eps` once and verify that `|a - b| <= eps` with
 * `eps >= 0`. NaN inputs fail the check.
 *
 * @param a First value.
 * @param b Second value.
 * @param eps Maximum allowed absolute difference.
 */
#define rk_check_near(a, b, eps) \
do { \
	__extension__ __auto_type _rk_a = (a); \
	__extension__ __auto_type _rk_b = (b); \
	long double _rk_e = (long double)(eps); \
	long double _rk_diff = (long double)_rk_a - (long double)_rk_b; \
	if (_rk_diff < 0.0L) \
		_rk_diff = -_rk_diff; \
	if (_rk_e >= 0.0L && _rk_diff <= _rk_e) { \
		rk_result(TPASS, "|%s - %s| <= %s", #a, #b, #eps); \
	} else { \
		char buf__[4096] = {0}; \
		size_t buf_size__ = 4096; \
		RK_SNPRINTF_NUM_ARGS_DEBUG_(buf__, buf_size__, \
			#a, _rk_a, #b, _rk_b); \
		rk_result(TFAIL, "|%s - %s| <= %s %s (eps = %Lg)", \
			#a, #b, #eps, buf__, _rk_e); \
	} \
} while(0)

/**
 * @brief Verify that pointer is NULL.
 *
 * Recognize if `ptr` is NULL and return a TPASS message. TFAIL otherwise.
 *
 * @param ptr A pointer.
 */
#define rk_check_ptr_null(ptr) \
do { \
	const void *_ck_ptr = (ptr); \
	if (!_ck_ptr) \
		rk_result(TPASS, "%s == NULL", #ptr); \
	else \
		rk_result(TFAIL, "%s == NULL (%p)", #ptr, _ck_ptr); \
} while(0)

/**
 * @brief Verify that pointer is not NULL.
 *
 * Recognize if `ptr` is not NULL and return a TPASS message. TFAIL otherwise.
 *
 * @param ptr A pointer.
 */
#define rk_check_ptr_not_null(ptr) \
do { \
	const void *_ck_ptr = (ptr); \
	if (_ck_ptr) \
		rk_result(TPASS, "%s != NULL (%p)", #ptr, _ck_ptr); \
	else \
		rk_result(TFAIL, "%s != NULL", #ptr); \
} while(0)

/**
 * @brief Verify that two memory regions contain the same data.
 *
 * Verify that `m1` and `m2` contain the same `n` bytes.
 *
 * @param m1 First pointer to memory data.
 * @param m2 Second pointer to memory data.
 * @param n Length of data in bytes.
 */
#define rk_check_mem_eq(m1, m2, n) \
do { \
	const void *_ck_m1 = (m1); \
	const void *_ck_m2 = (m2); \
	size_t _ck_n = (size_t)(n); \
	if (!_ck_m1 || !_ck_m2) \
		rk_result(TFAIL, "%s / %s NULL (%s = %p, %s = %p)", \
			#m1, #m2, #m1, _ck_m1, #m2, _ck_m2); \
	else if (!memcmp(_ck_m1, _ck_m2, _ck_n)) \
		rk_result(TPASS, "%s == %s", #m1, #m2); \
	else \
		rk_result(TFAIL, "%s != %s", #m1, #m2); \
} while(0)

/**
 * @brief Verify that two memory regions do not contain the same data.
 *
 * Verify that `m1` and `m2` differ within `n` bytes.
 *
 * @param m1 First pointer to memory data.
 * @param m2 Second pointer to memory data.
 * @param n Length of data in bytes.
 */
#define rk_check_mem_ne(m1, m2, n) \
do { \
	const void *_ck_m1 = (m1); \
	const void *_ck_m2 = (m2); \
	size_t _ck_n = (size_t)(n); \
	if (!_ck_m1 || !_ck_m2) \
		rk_result(TFAIL, "%s / %s NULL (%s = %p, %s = %p)", \
			#m1, #m2, #m1, _ck_m1, #m2, _ck_m2); \
	else if (memcmp(_ck_m1, _ck_m2, _ck_n)) \
		rk_result(TPASS, "%s != %s", #m1, #m2); \
	else \
		rk_result(TFAIL, "%s == %s", #m1, #m2); \
} while(0)

/**
 * @brief Verify that two strings contain the same data.
 *
 * Verify that `s1` and `s2` contain the same `n` bytes.
 *
 * @param s1 First pointer to string data.
 * @param s2 Second pointer to string data.
 * @param n Length of data in bytes.
 */
#define rk_check_str_eq(s1, s2, n) \
do { \
	const char *_ck_s1 = (s1); \
	const char *_ck_s2 = (s2); \
	size_t _ck_n = (size_t)(n); \
	int _ck_len = (int)(_ck_n > 1024 ? 1024 : _ck_n); \
	const char *_ck_p1 = _ck_s1 ? _ck_s1 : "(null)"; \
	const char *_ck_p2 = _ck_s2 ? _ck_s2 : "(null)"; \
	int _ck_l1 = _ck_s1 ? _ck_len : 6; \
	int _ck_l2 = _ck_s2 ? _ck_len : 6; \
	if (!_ck_s1 || !_ck_s2) { \
		rk_result(TFAIL, "%s != %s (%s = %.*s, %s = %.*s)", \
			#s1, #s2, #s1, _ck_l1, _ck_p1, #s2, _ck_l2, _ck_p2); \
	} else if (!memcmp(_ck_s1, _ck_s2, _ck_n)) { \
		rk_result(TPASS, "%s == %s (%s = %.*s, %s = %.*s)", \
			#s1, #s2, #s1, _ck_l1, _ck_p1, #s2, _ck_l2, _ck_p2); \
	} else { \
		rk_result(TFAIL, "%s != %s (%s = %.*s, %s = %.*s)", \
			#s1, #s2, #s1, _ck_l1, _ck_p1, #s2, _ck_l2, _ck_p2); \
	} \
} while(0)

/**
 * @brief Verify that two strings do not contain the same data.
 *
 * Verify that `s1` and `s2` differ within `n` bytes.
 *
 * @param s1 First pointer to string data.
 * @param s2 Second pointer to string data.
 * @param n Length of data in bytes.
 */
#define rk_check_str_ne(s1, s2, n) \
do { \
	const char *_ck_s1 = (s1); \
	const char *_ck_s2 = (s2); \
	size_t _ck_n = (size_t)(n); \
	int _ck_len = (int)(_ck_n > 1024 ? 1024 : _ck_n); \
	const char *_ck_p1 = _ck_s1 ? _ck_s1 : "(null)"; \
	const char *_ck_p2 = _ck_s2 ? _ck_s2 : "(null)"; \
	int _ck_l1 = _ck_s1 ? _ck_len : 6; \
	int _ck_l2 = _ck_s2 ? _ck_len : 6; \
	if (!_ck_s1 || !_ck_s2) { \
		rk_result(TFAIL, "%s / %s NULL (%s = %.*s, %s = %.*s)", \
			#s1, #s2, #s1, _ck_l1, _ck_p1, #s2, _ck_l2, _ck_p2); \
	} else if (memcmp(_ck_s1, _ck_s2, _ck_n)) { \
		rk_result(TPASS, "%s != %s (%s = %.*s, %s = %.*s)", \
			#s1, #s2, #s1, _ck_l1, _ck_p1, #s2, _ck_l2, _ck_p2); \
	} else { \
		rk_result(TFAIL, "%s == %s (%s = %.*s, %s = %.*s)", \
			#s1, #s2, #s1, _ck_l1, _ck_p1, #s2, _ck_l2, _ck_p2); \
	} \
} while(0)

/**
 * @brief Verify that two NUL-terminated strings are equal.
 *
 * Compare `s1` and `s2` with `strcmp()` and report TPASS if equal, TFAIL
 * otherwise (including when either pointer is NULL).
 *
 * @param s1 First NUL-terminated string.
 * @param s2 Second NUL-terminated string.
 */
#define rk_check_cstr_eq(s1, s2) \
do { \
	const char *_ck_s1 = (s1); \
	const char *_ck_s2 = (s2); \
	if (!_ck_s1 || !_ck_s2) { \
		rk_result(TFAIL, "%s / %s NULL (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1 ? _ck_s1 : "(null)", \
			#s2, _ck_s2 ? _ck_s2 : "(null)"); \
	} else if (!strcmp(_ck_s1, _ck_s2)) { \
		rk_result(TPASS, "%s == %s (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1, #s2, _ck_s2); \
	} else { \
		rk_result(TFAIL, "%s != %s (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1, #s2, _ck_s2); \
	} \
} while(0)

/**
 * @brief Verify that two NUL-terminated strings are different.
 *
 * Compare `s1` and `s2` with `strcmp()` and report TPASS if different, TFAIL
 * if equal or when either pointer is NULL.
 *
 * @param s1 First NUL-terminated string.
 * @param s2 Second NUL-terminated string.
 */
#define rk_check_cstr_ne(s1, s2) \
do { \
	const char *_ck_s1 = (s1); \
	const char *_ck_s2 = (s2); \
	if (!_ck_s1 || !_ck_s2) { \
		rk_result(TFAIL, "%s / %s NULL (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1 ? _ck_s1 : "(null)", \
			#s2, _ck_s2 ? _ck_s2 : "(null)"); \
	} else if (strcmp(_ck_s1, _ck_s2)) { \
		rk_result(TPASS, "%s != %s (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1, #s2, _ck_s2); \
	} else { \
		rk_result(TFAIL, "%s == %s (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1, #s2, _ck_s2); \
	} \
} while(0)

/**
 * @brief Verify that pointers are in the same address.
 *
 * Recognize that `ptr1` is equal to `ptr2` and return a TPASS message if true.
 * TFAIL otherwise.
 *
 * @param ptr1 A pointer.
 * @param ptr2 A pointer.
 */
#define rk_check_ptr_eq(ptr1, ptr2) \
do { \
	const void *_rk_ptr1 = (ptr1); \
	const void *_rk_ptr2 = (ptr2); \
	if (_rk_ptr1 == _rk_ptr2) { \
		rk_result(TPASS, "%s (%p) == %s (%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} else { \
		rk_result(TFAIL, "%s (%p) != %s (%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} \
} while(0)

/**
 * @brief Verify that pointers are not equal.
 *
 * Recognize that `ptr1` is different from `ptr2` and return a TPASS message
 * if true. TFAIL otherwise.
 *
 * @param ptr1 A pointer.
 * @param ptr2 A pointer.
 */
#define rk_check_ptr_ne(ptr1, ptr2) \
do { \
	const void *_rk_ptr1 = (ptr1); \
	const void *_rk_ptr2 = (ptr2); \
	if (_rk_ptr1 != _rk_ptr2) { \
		rk_result(TPASS, "%s (%p) != %s (%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} else { \
		rk_result(TFAIL, "%s (%p) == %s (%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} \
} while(0)

/**
 * @brief Default testing suite declaration.
 *
 * Define this symbol in your test program when using the default @ref main
 * provided when `TEST_CUSTOM_MAIN` is not defined.
 */
extern rk_suite_t test_suite;

/**
 * @brief Run a testing suite.
 *
 * Run suite setup, all tests, and suite teardown, print a summary, and exit
 * with a @ref rk_suite_result_t status (`RK_FAILED` takes precedence over
 * `RK_SKIPPED`).
 *
 * @param suite Testing suite object.
 */
void rk_run_suite(rk_suite_t *suite) __attribute__ ((noreturn));

#ifndef TEST_CUSTOM_MAIN

/**
 * @brief Default test runner entry point.
 *
 * Provided automatically unless `TEST_CUSTOM_MAIN` is defined before including
 * this header. Programs spanning multiple translation units must define
 * `TEST_CUSTOM_MAIN` in auxiliary files to avoid duplicate `main` symbols.
 */
int main(void)
{
	rk_run_suite(&test_suite);
	return 0;
}

#endif

#endif
