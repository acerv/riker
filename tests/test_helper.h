// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include "riker.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSERT_TRUE(cond) do { \
	if (!(cond)) { \
		fprintf(stderr, "%s:%d: assertion failed: %s\n", \
			__FILE__, __LINE__, #cond); \
		abort(); \
	} \
} while (0)

int run_suite_proc(rk_suite_t *suite, int *exit_code, char *out, size_t out_sz);
void strip_ansi(char *str);

#endif
