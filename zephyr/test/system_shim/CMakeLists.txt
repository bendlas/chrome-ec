# Copyright 2020 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS "${ZEPHYR_BASE}")
project(system_shim_test)

target_sources(app PRIVATE test_system.c
                           ${PLATFORM_EC}/zephyr/shim/src/system.c)
