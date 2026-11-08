# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>

VERSION      = 1.0
SOVERSION    = 1

CC          ?= cc
AR          ?= ar
DOXYGEN     ?= doxygen
CFLAGS      ?= -O2 -g -Wall -Wextra

PREFIX      ?= /usr/local
LIBDIR      ?= $(PREFIX)/lib
INCLUDEDIR  ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig

RK_CFLAGS    = -std=gnu11 -I. $(CFLAGS)

STATIC_LIB   = libriker.a
SHARED_LIB   = libriker.so.$(VERSION)
SONAME       = libriker.so.$(SOVERSION)
DEV_LINK     = libriker.so

TESTS        = tests/test_runner tests/test_macros \
	       tests/test_default_main tests/test_c99
TEST_CFLAGS  = -Itests

.PHONY: all install uninstall clean check doc

all: $(STATIC_LIB) $(SHARED_LIB) $(SONAME) $(DEV_LINK) riker.pc

riker.o: riker.c riker.h
	$(CC) $(CPPFLAGS) $(RK_CFLAGS) -c -o $@ $<

riker.pic.o: riker.c riker.h
	$(CC) $(CPPFLAGS) $(RK_CFLAGS) -fPIC -c -o $@ $<

$(STATIC_LIB): riker.o
	$(AR) rcs $@ $^

$(SHARED_LIB): riker.pic.o
	$(CC) $(RK_CFLAGS) $(LDFLAGS) -shared \
		-Wl,-soname,$(SONAME) -o $@ $^

$(SONAME) $(DEV_LINK): $(SHARED_LIB)
	ln -sf $(SHARED_LIB) $@

riker.pc: riker.pc.in Makefile
	sed -e 's|@PREFIX@|$(PREFIX)|g' \
	    -e 's|@LIBDIR@|$(LIBDIR)|g' \
	    -e 's|@INCLUDEDIR@|$(INCLUDEDIR)|g' \
	    -e 's|@VERSION@|$(VERSION)|g' \
	    $< > $@

install: all
	install -d $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(LIBDIR) \
		$(DESTDIR)$(PKGCONFIGDIR)
	install -m 644 riker.h $(DESTDIR)$(INCLUDEDIR)/riker.h
	install -m 644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/$(STATIC_LIB)
	install -m 755 $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/$(SHARED_LIB)
	ln -sf $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/$(DEV_LINK)
	install -m 644 riker.pc $(DESTDIR)$(PKGCONFIGDIR)/riker.pc

uninstall:
	rm -f $(DESTDIR)$(INCLUDEDIR)/riker.h \
	      $(DESTDIR)$(LIBDIR)/$(STATIC_LIB) \
	      $(DESTDIR)$(LIBDIR)/$(SHARED_LIB) \
	      $(DESTDIR)$(LIBDIR)/$(SONAME) \
	      $(DESTDIR)$(LIBDIR)/$(DEV_LINK) \
	      $(DESTDIR)$(PKGCONFIGDIR)/riker.pc

tests/test_helper.o: tests/test_helper.c tests/test_helper.h riker.h
	$(CC) $(CPPFLAGS) $(RK_CFLAGS) $(TEST_CFLAGS) -c -o $@ $<

tests/test_runner: tests/test_runner.c tests/test_helper.o $(STATIC_LIB)
	$(CC) $(CPPFLAGS) $(RK_CFLAGS) $(TEST_CFLAGS) $(LDFLAGS) -o $@ $^

tests/test_macros: tests/test_macros.c tests/test_helper.o $(STATIC_LIB)
	$(CC) $(CPPFLAGS) $(RK_CFLAGS) $(TEST_CFLAGS) $(LDFLAGS) -o $@ $^

tests/test_default_main: tests/test_default_main.c \
		tests/test_default_main_aux.c tests/test_default_main.h \
		$(STATIC_LIB)
	$(CC) $(CPPFLAGS) $(RK_CFLAGS) $(TEST_CFLAGS) $(LDFLAGS) -o $@ \
		$(filter %.c %.a,$^)

tests/test_c99: tests/test_c99.c tests/test_helper.o $(STATIC_LIB)
	$(CC) $(CPPFLAGS) $(RK_CFLAGS) -std=gnu99 $(TEST_CFLAGS) \
		$(LDFLAGS) -o $@ $^

check: $(TESTS)
	@for t in $(TESTS); do \
		echo "Running $$t"; \
		./$$t || { echo "FAIL: $$t"; exit 1; }; \
	done
	@echo "All tests passed"

Doxyfile: Doxyfile.in Makefile
	sed -e 's|@PROJECT_NAME@|riker|g' \
	    -e 's|@PROJECT_VERSION@|$(VERSION)|g' \
	    -e 's|@INPUT@|$(CURDIR)/riker.h|g' \
	    -e 's|@MAINPAGE@|$(CURDIR)/README.md|g' \
	    -e 's|@OUTPUT@|$(CURDIR)/doc|g' \
	    $< > $@

doc: Doxyfile
	@command -v $(DOXYGEN) >/dev/null 2>&1 || \
		{ echo "error: $(DOXYGEN) not found" >&2; exit 1; }
	$(DOXYGEN) Doxyfile

clean:
	rm -f riker.o riker.pic.o $(STATIC_LIB) $(SHARED_LIB) \
	      $(SONAME) $(DEV_LINK) riker.pc
	rm -f tests/test_helper.o $(TESTS)
	rm -rf Doxyfile doc
