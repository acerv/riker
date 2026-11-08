# Riker

Riker is a simple and lean C testing framework, inspired by the Linux
Test Project (LTP) core library, providing macros to check and debug
your program.

## Installation

Build and install with Make:

```sh
make
make install [PREFIX=/usr/local] [DESTDIR=...]
```

To build and run the test suite:

```sh
make check
```

Run tests with AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
make check-asan
```

Run static analysis with Clang Static Analyzer:

```sh
make check-analyze
```

To generate the API documentation (requires Doxygen):

```sh
make doc
```

Link test binaries with `-lriker` (or use
`pkg-config --cflags --libs riker`).

## Example

```c
#include <riker.h>
#include <stdlib.h>

static char *buf;

static void suite_setup(void)
{
	rk_result(TINFO, "suite setup");
}

static void suite_teardown(void)
{
	rk_result(TINFO, "suite teardown");
}

static void test_setup(void)
{
	buf = malloc(64);
}

static void test_teardown(void)
{
	free(buf);
	buf = NULL;
}

static void test_buffer(void)
{
	rk_require(buf != NULL);
	rk_check_ptr_not_null(buf);
}

static void test_math(void)
{
	int a = 2 + 2;

	rk_check_eq(a, 4);
	rk_check_gt(a, 0);
}

rk_suite_t test_suite = {
	.setup = suite_setup,
	.teardown = suite_teardown,
	.tests = (rk_test_t []) {
		{
			.setup = test_setup,
			.run = test_buffer,
			.teardown = test_teardown,
		},
		{ .run = test_math },
		{ .run = NULL },
	},
};
```

Compile and run:

```sh
gcc -std=gnu11 test.c -lriker -o test
./test
```

Define `TEST_CUSTOM_MAIN` before `#include <riker.h>` if you provide your
own `main()` or include `riker.h` across multiple translation units.

## Reference

- Reporting: `rk_result()`, `rk_result_errno()`, `rk_error()`, `rk_brk()`,
  `rk_require()`
- Checks: `rk_check_expr()`, `rk_check_{eq,ne,gt,ge,lt,le}()`,
  `rk_check_near()`, `rk_check_ptr_{null,not_null,eq,ne}()`,
  `rk_check_mem_{eq,ne}()`, `rk_check_str_{eq,ne}()`,
  `rk_check_cstr_{eq,ne}()`, `rk_check_errno()`
- Isolation: each `rk_test_t` runs in a forked child with `.timeout`
  seconds (default 600s)
- Environment: `RIKER_NO_FORK=1` (in-process debugging), `NO_COLOR=1`
  (disable ANSI colors)
