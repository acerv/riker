// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#ifndef RIKER_H
#define RIKER_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stddef.h>

/** @brief Latest test result. This is set all the times we call `rk_result`. */
static int RK_TST_RES;

/**
 * @brief Test result type.
 *
 * Input parameter for the @ref rk_result function, defining the result
 * of the test.
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
 * Return values provided by the runner and forwarded to the @ref main function
 * when a new test is executed.
 */
typedef enum 
{
	/** @brief An error occured during runner execution. */
	RK_ERROR = -1,
	/** @brief All tests passed. */
	RK_PASSED = 0,
	/** @brief Some tests failed. */
	RK_FAILED,
	/** @brief No tests have failed but some have been skipped. */
	RK_SKIPPED,
} rk_suite_result_t;

typedef void (*rk_test_func)(void);

/**
 * @brief Rapresent a test.
 *
 * This struct has to be initialized in order to declare a test.
 */
typedef struct __attribute__((packed))
{
	/** @brief Setup function executed before `run`. */
	rk_test_func setup;
	/** @brief Teardown function executed after `run`. */
	rk_test_func teardown;
	/** @brief Test to execute. */
	rk_test_func run;
	/** @brief Test timeout in seconds. */
	unsigned int timeout;
} rk_test_t;

/**
 * @brief Rapresent a testing suite.
 *
 * This struct has to be initialized in order to declare a testing suite.
 * Declaration of a testing suite can be done in the following way:
 *
 * @code
 * static struct rk_suite test_suite = {
 *	.tests = (struct rk_test []) {
 *		{
 *			.run = test2,
 *			.setup = setup1,
 *			.teardown = teardown1
 *			.timeout = 20,
 *		},
 *		{ .run = test2 },
 *		{ NULL },
 *	},
 *	.setup = setup_suite,
 *	.teardown = teardown_suite,
 * };
 * @endcode
 *
 * The object is going to be used automatically by the @ref main function at
 * compile time. Make sure that its name is correct.
 */
typedef struct
{
	/** @brief Setup function executed before all `tests`. */
	rk_test_func setup;
	/** @brief Setup function executed after all `tests`. */
	rk_test_func teardown;
	/** @brief List of the tests to execute. */
	rk_test_t *tests;
} rk_suite_t;

void rk_result_(const char *file, const int lineno, rk_test_result_t ttype,
		const char *fmt, ...)
		__attribute__ ((format (printf, 4, 5)));

#if __STDC_VERSION__ >= 201112L
#define RK_SNPRINTF_NUM_(ret, buf, buf_size, num) \
do { \
	if (buf_size > 0) { \
		ret = (size_t)_Generic((num), \
		    int: snprintf(buf, buf_size, "%d", (int)(num)), \
		    short: snprintf(buf, buf_size, "%hd", (short)(num)), \
		    long: snprintf(buf, buf_size, "%ld", (long)(num)), \
		    long long: snprintf(buf, buf_size, "%lld", (long long)(num)), \
		    float: snprintf(buf, buf_size, "%.6f", (float)(num)), \
		    double: snprintf(buf, buf_size, "%.6lf", (double)(num)), \
		    long double: snprintf(buf, buf_size, "%.6Lf", (long double)(num)), \
		    unsigned int: snprintf(buf, buf_size, "%u", (unsigned int)(num)), \
		    unsigned short: snprintf(buf, buf_size, "%hu", (unsigned short)(num)), \
		    unsigned long: snprintf(buf, buf_size, "%lu", (unsigned long)(num)), \
		    unsigned long long: snprintf(buf, buf_size, "%llu", (unsigned long long)(num)) \
		); \
	} \
} while(0)

#define RK_SNPRINTF_NUM_ARGS_DEBUG_(buf, buf_size, a, b) \
do { \
	size_t pos__; \
	char *str__ = buf; \
	size_t size__ = buf_size; \
	pos__ = (size_t)snprintf(str__, size__, "(%s = ", #a); \
	size__ -= pos__; \
	if (buf_size > size__) { \
		str__ += pos__; \
		RK_SNPRINTF_NUM_(pos__, str__, size__, a); \
		size__ -= pos__; \
	} \
	if (buf_size > size__) { \
		str__ += pos__; \
		pos__ = (size_t)snprintf(str__, size__, ", %s = ", #b); \
		size__ -= pos__; \
	} \
	if (buf_size > size__) { \
		str__ += pos__; \
		RK_SNPRINTF_NUM_(pos__, str__, size__, b); \
		size__ -= pos__; \
	} \
	if (buf_size > size__) { \
		str__ += pos__; \
		pos__ = (size_t)snprintf(str__, size__, ")"); \
	} \
} while(0)
#else
#define RK_SNPRINTF_NUM_(ret, buf, buf_size, num) (ret = 0)
#define RK_SNPRINTF_NUM_ARGS_DEBUG_(buf, buf_size, a, b) \
do { \
	size_t s__ = buf_size; \
	if (s__ == buf_size) {} \
} while(0)
#endif

#define RK_CHECK_NUM_(a, b, op) \
do { \
	int ttype__ = (((a) op (b)) ? TPASS : TFAIL); \
	if (ttype__ == TFAIL) { \
		char buf__[4096] = {0}; \
		size_t buf_size__ = 4096; \
		RK_SNPRINTF_NUM_ARGS_DEBUG_(buf__, buf_size__, a, b); \
		rk_result(ttype__, "%s %s %s %s", #a, #op, #b, buf__); \
	} else { \
		rk_result(ttype__, "%s %s %s", #a, #op, #b); \
	} \
} while(0)

/**
 * @brief Send a test result to stdout.
 *
 * Send a test result to stdout and save the test status in the suite results
 * table.
 *
 * @param ttype Message type.
 * @param arg_fmt String to print, including string formatters.
 * @param va_args Arguments for the printf().
 */
#define rk_result(ttype, arg_fmt, ...) do { \
	if (ttype != TINFO) \
		RK_TST_RES = ttype; \
	rk_result_(__FILE__, __LINE__, (ttype), (arg_fmt), ##__VA_ARGS__); \
} while (0)

/**
 * @brief Send an error message to stderr and close the current session.
 *
 * Send an error message to the stderr, save the session status in the suite
 * results table and exit() the calling process.
 *
 * @param arg_fmt String to print, including string formatters.
 * @param va_args Arguments for the printf().
 */
#define rk_error(arg_fmt, ...) do { \
	RK_TST_RES = TERROR; \
	rk_result_(__FILE__, __LINE__, TERROR, (arg_fmt), ##__VA_ARGS__); \
} while (0)

/**
 * @brief Verify that `expr` is satisfied.
 *
 * Verify that `expr` is satisfied and return a TPASS message. TFAIL otherwise.
 *
 * @param expr An expression that has to be satisfied.
 */
#define rk_check_expr(expr) \
	rk_result(((expr) ? TPASS : TFAIL), "%s", #expr)

/**
 * @brief Verify that two numbers are equal.
 *
 * Recognize the numeric type of the input values and verify if they have the
 * same value.
 *
 * @param a First number.
 * @param b Second number.
 */
#define rk_check_eq(a, b) RK_CHECK_NUM_(a, b, ==)

/**
 * @brief Verify that two numbers are different.
 *
 * Recognize the numeric type of the input values and verify if they don' have
 * the same value.
 *
 * @param a First number.
 * @param b Second number.
 */
#define rk_check_ne(a, b) RK_CHECK_NUM_(a, b, !=)

/**
 * @brief Verify that the first number is greater than the second.
 *
 * Recognize the numeric type of the input values and verify that first
 * number is greater than the second.
 *
 * @param a First number.
 * @param b Second number.
 */
#define rk_check_gt(a, b) RK_CHECK_NUM_(a, b, >)

/**
 * @brief Verify that the first number is greater or equal than the second.
 *
 * Recognize the numeric type of the input values and verify that first
 * number is greater or equal than the second.
 *
 * @param a First number.
 * @param b Second number.
 */
#define rk_check_ge(a, b) RK_CHECK_NUM_(a, b, >=)

/**
 * @brief Verify that the first number is lower than the second.
 *
 * Recognize the numeric type of the input values and verify that first
 * number is lower than the second.
 *
 * @param a First number.
 * @param b Second number.
 */
#define rk_check_lt(a, b) RK_CHECK_NUM_(a, b, <)

/**
 * @brief Verify that the first number is lower or equal than the second.
 *
 * Recognize the numeric type of the input values and verify that first
 * number is lower or equal than the second.
 *
 * @param a First number.
 * @param b Second number.
 */
#define rk_check_le(a, b) RK_CHECK_NUM_(a, b, <=)

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
		rk_result(TFAIL, "%s == NULL (0x%p)", #ptr, _ck_ptr); \
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
		rk_result(TPASS, "%s != NULL (0x%p)", #ptr, _ck_ptr); \
	else \
		rk_result(TFAIL, "%s != NULL", #ptr); \
} while(0)

/**
 * @brief Verify that two memories contain the same data. 
 *
 * Verify that `m1` and `m2` contains the same data.
 *
 * @param m1 First pointer to some memory data.
 * @param m2 Second pointer to some memory data.
 * @param n Length of data.
 */
#define rk_check_mem_eq(m1, m2, n) \
do { \
	const void *_ck_m1 = (m1); \
	const void *_ck_m2 = (m2); \
	size_t _ck_n = (size_t)(n); \
	if (!memcmp(_ck_m1, _ck_m2, _ck_n)) \
		rk_result(TPASS, "%s == %s", #m1, #m2); \
	else \
		rk_result(TFAIL, "%s != %s", #m1, #m2); \
} while(0)

/**
 * @brief Verify that two memories doesn't contain the same data. 
 *
 * Verify that `m1` and `m2` doesn't contains the same data.
 *
 * @param m1 First pointer to some memory data.
 * @param m2 Second pointer to some memory data.
 * @param n Length of data.
 */
#define rk_check_mem_ne(m1, m2, n) \
do { \
	const void *_ck_m1 = (m1); \
	const void *_ck_m2 = (m2); \
	size_t _ck_n = (size_t)(n); \
	if (memcmp(_ck_m1, _ck_m2, _ck_n)) \
		rk_result(TPASS, "%s != %s", #m1, #m2); \
	else \
		rk_result(TFAIL, "%s == %s", #m1, #m2); \
} while(0)

/**
 * @brief Verify that two strings contain the same data. 
 *
 * Verify that `s1` and `s2` contains the same data.
 *
 * @param s1 First pointer to some string data.
 * @param s2 Second pointer to some string data.
 * @param n Length of data.
 */
#define rk_check_str_eq(s1, s2, n) \
do { \
	const char *_ck_s1 = (s1); \
	const char *_ck_s2 = (s2); \
	size_t _ck_n = (size_t)(n); \
	if (!memcmp(_ck_s1, _ck_s2, _ck_n)) { \
		rk_result(TPASS, "%s == %s (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1, #s2, _ck_s2); \
	} else { \
		rk_result(TFAIL, "%s != %s (%s = %s, %s = %s)", \
			#s1, #s2, #s1, _ck_s1, #s2, _ck_s2); \
	} \
} while(0)

/**
 * @brief Verify that two strings don't contain the same data. 
 *
 * Verify that `s1` and `s2` don't contains the same data.
 *
 * @param s1 First pointer to some string data.
 * @param s2 Second pointer to some string data.
 * @param n Length of data.
 */
#define rk_check_str_ne(s1, s2, n) \
do { \
	const char *_ck_s1 = (s1); \
	const char *_ck_s2 = (s2); \
	size_t _ck_n = (size_t)(n); \
	if (memcmp(_ck_s1, _ck_s2, _ck_n)) { \
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
 * Recognize that `ptr1` is equal to `ptr2 and return a TPASS message if true.
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
		rk_result(TPASS, "%s (0x%p) == %s (0x%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} else { \
		rk_result(TFAIL, "%s (0x%p) != %s (0x%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} \
} while(0)

/**
 * @brief Verify that pointers are not equal.
 *
 * Recognize that `ptr1` is equal to `ptr2 and return a TFAIL message if true.
 * TPASS otherwise.
 *
 * @param ptr1 A pointer.
 * @param ptr2 A pointer.
 */
#define rk_check_ptr_ne(ptr1, ptr2) \
do { \
	const void *_rk_ptr1 = (ptr1); \
	const void *_rk_ptr2 = (ptr2); \
	if (_rk_ptr1 != _rk_ptr2) { \
		rk_result(TPASS, "%s (0x%p) != %s (0x%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} else { \
		rk_result(TFAIL, "%s (0x%p) == %s (0x%p)", \
			#ptr1, _rk_ptr1, \
			#ptr2, _rk_ptr2); \
	} \
} while(0)

/**
 * @brief Testing suite declaration.
 *
 * Override this object in order to declare your own testing suite.
 */
static rk_suite_t test_suite;

/**
 * @brief Run a testing suite.
 *
 * Run all tests inside a testing suite and return the ending result.
 *
 * @param suite Testing suite object.
 */
void rk_run_suite(rk_suite_t *suite);

#ifndef TEST_CUSTOM_MAIN

int main(void)
{
	rk_run_suite(&test_suite);
}

#endif

#endif
