# Riker

Riker is a simple and lean C testing framework, inspired by the Linux
Test Project (LTP) core library, providing macros to check and debug
your program.

## Installation

Build and install with Meson:

```sh
meson setup builddir
meson compile -C builddir
meson install -C builddir
```

To build and run the test suite:

```sh
meson setup builddir -Dbuild_tests=true
meson test -C builddir
```

Link test binaries with `-lriker` (or use the `riker` Meson
dependency).

## Example

```c
#include <riker.h>

static void test_math(void)
{
	int a = 2 + 2;

	rk_check_eq(a, 4);
	rk_check_gt(a, 0);
}

static void test_string(void)
{
	const char *s = "riker";

	rk_check_ptr_not_null(s);
	rk_check_str_eq(s, "riker", 5);
}

rk_suite_t test_suite = {
	.tests = (rk_test_t []) {
		{ .run = test_math },
		{ .run = test_string },
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
Set `RIKER_NO_FORK=1` to run tests in-process when debugging.
