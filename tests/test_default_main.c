// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#include "riker.h"
#include "test_default_main.h"

static void test_cross_tu_clean(void)
{
	RK_TST_RES = TERROR;
	aux_check_pass();
	rk_check_eq(RK_TST_RES, TPASS);

	aux_check_tinfo_preserves();
	rk_check_eq(RK_TST_RES, TPASS);
}

rk_suite_t test_suite = {
	.tests = (rk_test_t []) {
		{ .run = test_cross_tu_clean },
		{ .run = NULL },
	},
};
