// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>
 */

#define TEST_CUSTOM_MAIN 1

#include "riker.h"
#include "test_default_main.h"

void aux_check_pass(void)
{
	rk_check_eq(42, 42);
}

void aux_check_tinfo_preserves(void)
{
	rk_result(TINFO, "aux info message");
}
