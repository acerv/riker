// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#include "riker.h"
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define RED(str) BOLD "\033[31m" str RESET
#define GREEN(str) BOLD "\033[32m" str RESET
#define YELLOW(str) BOLD "\033[33m" str RESET
#define BLUE(str) BOLD "\033[34m" str RESET
#define MAGENTA(str) BOLD "\033[35m" str RESET
#define COLORIZE(color, str, colorize) (colorize ? color(str) : str)

typedef enum
{
	SUITE_SETUP = 0,
	SUITE_TEARDOWN,
	TEST_RUN,
	TEST_SETUP,
	TEST_TEARDOWN,
} rk_session_state_t;

typedef struct
{
	size_t passed;
	size_t failed;
	size_t skipped;
	size_t errors;
	rk_suite_t *suite;
	rk_test_t *curr_test;
	rk_session_state_t state;
} rk_session_t;

static rk_session_t *session;

/* Forward declaration specifying gnu_printf attribute */
void show_test_result(const char *file, const int lineno, int res,
		const char *fmt, va_list va)
		__attribute__ ((format (printf, 4, 0)));

void show_test_result(const char *file, const int lineno, int res,
			const char *fmt, va_list va)
{
	char buf[1024];
	size_t sfx_size;
	size_t buf_size = sizeof(buf);
	const char *str_res;

	switch (res) {
	case TPASS:
		str_res = COLORIZE(GREEN, "PASS", 1);
		session->passed++;
		break;
	case TFAIL:
		str_res = COLORIZE(RED, "FAIL", 1);
		session->failed++;
		break;
	case TSKIP:
		str_res = COLORIZE(YELLOW, "SKIP", 1);
		session->skipped++;
		break;
	case TERROR:
		str_res = COLORIZE(MAGENTA, "ERROR", 1);
		session->errors++;
		break;
	default:
		str_res = COLORIZE(BLUE, "INFO", 1);
		break;
	}

	memset(buf, 0, buf_size);

	sfx_size = (size_t)snprintf(buf, buf_size, "%s:%i %s ",
			file, lineno, str_res);

	if (sfx_size < buf_size)
		vsnprintf(buf + sfx_size, buf_size - sfx_size, fmt, va);

	printf("%s\n", buf);
}

static void run_test(rk_test_t *test)
{
	assert(test);

	if (test->setup) {
		session->state = TEST_SETUP;
		test->setup();
	}

	if (test->run) {
		session->state = TEST_RUN;
		test->run();
	}

	if (test->teardown) {
		session->state = TEST_TEARDOWN;
		test->teardown();
	}
}

static void fork_run_test(rk_test_t *test)
{
	int ret;
	int status;
	pid_t test_pid;
	time_t start_time;
	bool killed = false;
	unsigned int timeout;

	assert(test);

	test_pid = fork();
	if (test_pid < 0)
		rk_error("fork() error: %s\n", strerror(errno));

	if (!test_pid) {
		run_test(test);
		exit(0);
	}

	timeout = test->timeout ? test->timeout: 600;
	start_time = time(NULL);

	while (1) {
		if (difftime(time(NULL), start_time) >= timeout) {
			rk_result(TINFO, "Test timed out. Kill the process.");
			kill(test_pid, SIGKILL);
			killed = true;
			break;
		}

		ret = waitpid(test_pid, &status, WNOHANG);
		if (ret > 0 && WIFEXITED(status))
			break;

		if (ret < -1 && errno != ECHILD) {
			rk_error("waitpid(%i) error: %s\n",
				test_pid, strerror(errno));
		}

		usleep(100);
	}

	if (waitpid(test_pid, &status, 0) < 0 && errno != ECHILD)
		rk_error("waitpid(%i) error: %s\n", test_pid, strerror(errno));

	if (!killed && WIFSIGNALED(status))
		rk_error("Test child killed with signal %d", WTERMSIG(status));
}

void rk_result_(const char *file, const int lineno, rk_test_result_t ttype,
		const char *fmt, ...)
{
	va_list va;
	rk_test_t *test;
	rk_suite_t *suite;

	va_start(va, fmt);
	show_test_result(file, lineno, ttype, fmt, va);
	va_end(va);

	if (ttype == TERROR) {
		switch (session->state) {
		case SUITE_SETUP:
			suite = session->suite;
			if (suite && suite->teardown)
				suite->teardown();
			break;
		case TEST_SETUP:
		case TEST_RUN:
			test = session->curr_test;
			if (test && test->teardown)
				test->teardown();
			break;
		case SUITE_TEARDOWN:
		case TEST_TEARDOWN:
		default:
			break;
		}

		exit(ttype);
	}
}

void rk_run_suite(rk_suite_t *suite)
{
	int result = RK_PASSED;
	int ret;

	assert(suite);

	session = mmap(NULL,
		sizeof(rk_session_t),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);

	if (session == MAP_FAILED) {
		rk_result(TERROR, "mmap() error: %s\n", strerror(errno));
		return;
	}

	session->suite = suite;

	if (suite->setup) {
		session->state = SUITE_SETUP;
		suite->setup();
	}

	if (suite->tests) {
		for (size_t i = 0; suite->tests[i].run; i++) {
			session->curr_test = suite->tests + i;
			fork_run_test(session->curr_test);
		}

		session->curr_test = NULL;
	}

	if (waitpid(-1, NULL, 0) < 0 && errno != ECHILD)
		rk_error("waitpid(-1) error: %s\n", strerror(errno));

	if (session->skipped)
		result = RK_SKIPPED;
	else if (session->failed || session->errors)
		result = RK_FAILED;

	if (suite->teardown) {
		session->state = SUITE_TEARDOWN;
		suite->teardown();
	}

	printf("\nSummary:\n"
		"%s:  %lu\n"
		"%s:  %lu\n"
		"%s: %lu\n"
		"%s:  %lu\n",
		COLORIZE(GREEN, "Passed", 1),
		session->passed,
		COLORIZE(RED, "Failed", 1),
		session->failed,
		COLORIZE(YELLOW, "Skipped", 1),
		session->skipped,
		COLORIZE(MAGENTA, "Errors", 1),
		session->errors
	);

	ret = munmap(session, sizeof(rk_session_t));
	if (ret == -1)
		rk_result(TERROR, "munmap() error: %s\n", strerror(errno));

	exit(result);
}
