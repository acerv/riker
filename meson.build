# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2025 Andrea Cervesato <andrea.cervesato@mailbox.org>

project(
    'riker',
    'c',
    license : 'GPL-2.0+',
    version : '1.0',
    default_options : [
        'c_std=gnu11',
        'warning_level=everything',
        'buildtype=debugoptimized',
    ],
    meson_version: '>=1.3.1'
)

add_project_arguments([
        '-Wno-c++-compat',
    ], language : 'c'
)

riker_api = include_directories('.')

install_headers('riker.h')

my_library = library(
    'riker',
    'riker.c',
    install : true,
    install_dir : 'lib',
    include_directories: riker_api
)

riker = declare_dependency(
    include_directories: riker_api,
    link_with: [my_library]
)

if get_option('build_tests')
    test_exec = executable(
        'test_riker',
        'test_riker.c',
        link_with : my_library,
    )

    test('test_riker', test_exec)
endif
