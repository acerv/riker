// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#define TEST_CUSTOM_MAIN 1

#include "test_helper.h"
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int run_suite_proc(rk_suite_t *suite, int *exit_code, char *out, size_t out_sz)
{
	int pipefd[2];
	int status;
	pid_t pid;
	size_t total = 0;

	if (!suite || !exit_code)
		return -1;

	if (pipe(pipefd) == -1)
		return -1;

	fflush(NULL);
	pid = fork();
	if (pid == -1) {
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}

	if (!pid) {
		close(pipefd[0]);
		if (dup2(pipefd[1], STDOUT_FILENO) == -1 ||
		    dup2(pipefd[1], STDERR_FILENO) == -1) {
			_exit(127);
		}
		close(pipefd[1]);
		rk_run_suite(suite);
	}

	close(pipefd[1]);

	for (;;) {
		char drain[256];
		ssize_t n;

		if (out && out_sz > 1 && total < out_sz - 1) {
			n = read(pipefd[0], out + total, out_sz - 1 - total);
		} else {
			n = read(pipefd[0], drain, sizeof(drain));
		}

		if (n > 0) {
			if (out && out_sz > 1 && total < out_sz - 1)
				total += (size_t)n;
		} else if (n == -1 && errno == EINTR) {
			continue;
		} else {
			break;
		}
	}

	if (out && out_sz > 0)
		out[total] = '\0';

	close(pipefd[0]);

	while (waitpid(pid, &status, 0) == -1) {
		if (errno != EINTR)
			return -1;
	}

	if (!WIFEXITED(status))
		return -1;

	*exit_code = WEXITSTATUS(status);
	return 0;
}

void strip_ansi(char *str)
{
	char *src = str;
	char *dst = str;

	if (!str)
		return;

	while (*src) {
		if (src[0] == '\033' && src[1] == '[') {
			src += 2;
			while (*src && *src != 'm')
				src++;
			if (*src == 'm')
				src++;
			continue;
		}
		*dst++ = *src++;
	}

	*dst = '\0';
}
