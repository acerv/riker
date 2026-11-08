// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#define TEST_CUSTOM_MAIN 1

#include "riker.h"
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
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
	int pad;
} rk_session_t;

int RK_TST_RES;
static rk_session_t *session;

/* Forward declaration specifying gnu_printf attribute */
static void show_test_result(const char *file, const int lineno, int res,
		const char *fmt, va_list va)
		__attribute__ ((format (printf, 4, 0)));

static bool use_color(void)
{
	const char *no_color = getenv("NO_COLOR");

	if (no_color && no_color[0] != '\0')
		return false;

	return isatty(STDOUT_FILENO) != 0;
}

static void show_test_result(const char *file, const int lineno, int res,
			const char *fmt, va_list va)
{
	char buf[1024];
	size_t sfx_size;
	size_t buf_size = sizeof(buf);
	const char *str_res;
	bool color = use_color();

	switch (res) {
	case TPASS:
		str_res = COLORIZE(GREEN, "PASS", color);
		if (session)
			session->passed++;
		break;
	case TFAIL:
		str_res = COLORIZE(RED, "FAIL", color);
		if (session)
			session->failed++;
		break;
	case TSKIP:
		str_res = COLORIZE(YELLOW, "SKIP", color);
		if (session)
			session->skipped++;
		break;
	case TERROR:
		str_res = COLORIZE(MAGENTA, "ERROR", color);
		if (session)
			session->errors++;
		break;
	default:
		str_res = COLORIZE(BLUE, "INFO", color);
		break;
	}

	memset(buf, 0, buf_size);

	sfx_size = (size_t)snprintf(buf, buf_size, "%s:%i %s ",
			file, lineno, str_res);

	if (sfx_size < buf_size)
		vsnprintf(buf + sfx_size, buf_size - sfx_size, fmt, va);

	printf("%s\n", buf);
	fflush(stdout);
}

static inline bool is_test_empty(const rk_test_t *test)
{
	return !test->run && !test->setup && !test->teardown;
}

static void run_test(rk_test_t *test)
{
	assert(test);

	size_t prev_err = session->errors;
	size_t prev_fail = session->failed;

	if (test->setup) {
		session->state = TEST_SETUP;
		test->setup();
	}

	if (session->errors == prev_err &&
	    session->failed == prev_fail &&
	    test->run) {
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
	int status = 0;
	pid_t test_pid;
	struct timespec start;
	struct timespec now;
	struct timespec step = { .tv_sec = 0, .tv_nsec = 1000000L };
	bool killed = false;
	size_t timeout;

	assert(test);

	fflush(NULL);
	test_pid = fork();
	if (test_pid < 0) {
		rk_error("fork() error: %s", strerror(errno));
		return;
	}

	if (!test_pid) {
		run_test(test);
		fflush(NULL);
		_exit(0);
	}

	timeout = test->timeout ? test->timeout : 600;
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
		rk_error("clock_gettime() error: %s", strerror(errno));
		kill(test_pid, SIGKILL);
		while (waitpid(test_pid, &status, 0) == -1 && errno == EINTR)
			;
		return;
	}

	for (;;) {
		ret = waitpid(test_pid, &status, WNOHANG);
		if (ret == test_pid)
			break;

		if (ret == -1) {
			if (errno == EINTR)
				continue;
			rk_error("waitpid(%i) error: %s",
				test_pid, strerror(errno));
			return;
		}

		if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
			long long sec = (long long)(now.tv_sec - start.tv_sec);

			if (now.tv_nsec < start.tv_nsec)
				sec--;

			if (sec >= 0 && (size_t)sec >= timeout) {
				kill(test_pid, SIGKILL);
				while (waitpid(test_pid, &status, 0) == -1 &&
				       errno == EINTR)
					;
				rk_error("Test timed out after %zu s", timeout);
				killed = true;
				break;
			}
		}

		nanosleep(&step, NULL);
	}

	if (!killed) {
		if (WIFSIGNALED(status)) {
			rk_error("Test child killed with signal %d",
				WTERMSIG(status));
		} else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			rk_error("Test child exited with status %d",
				WEXITSTATUS(status));
		}
	}
}

void rk_result_(const char *file, const int lineno, rk_test_result_t ttype,
		const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	show_test_result(file, lineno, ttype, fmt, va);
	va_end(va);
}

void rk_run_suite(rk_suite_t *suite)
{
	int result = RK_PASSED;
	int ret;

	void *mem;

	assert(suite);

	mem = mmap(NULL,
		sizeof(rk_session_t),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);

	if (mem == MAP_FAILED) {
		rk_result(TERROR, "mmap() error: %s", strerror(errno));
		exit(RK_ERROR);
	}

	session = mem;
	session->suite = suite;

	if (suite->setup) {
		session->state = SUITE_SETUP;
		suite->setup();
	}

	if (suite->tests) {
		for (size_t i = 0; !is_test_empty(&suite->tests[i]); i++) {
			session->curr_test = &suite->tests[i];
			fork_run_test(session->curr_test);
		}

		session->curr_test = NULL;
	}

	if (suite->teardown) {
		session->state = SUITE_TEARDOWN;
		suite->teardown();
	}

	if (session->failed || session->errors)
		result = RK_FAILED;
	else if (session->skipped)
		result = RK_SKIPPED;

	bool color = use_color();

	printf("\nSummary:\n"
		"%s:  %zu\n"
		"%s:  %zu\n"
		"%s: %zu\n"
		"%s:  %zu\n",
		COLORIZE(GREEN, "Passed", color),
		session->passed,
		COLORIZE(RED, "Failed", color),
		session->failed,
		COLORIZE(YELLOW, "Skipped", color),
		session->skipped,
		COLORIZE(MAGENTA, "Errors", color),
		session->errors
	);

	ret = munmap(session, sizeof(rk_session_t));
	session = NULL;
	if (ret == -1)
		rk_result(TERROR, "munmap() error: %s", strerror(errno));

	exit(result);
}
