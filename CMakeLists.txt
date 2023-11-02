#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project("Zigbee coordinator")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)

target_include_directories(app PRIVATE include)
target_include_directories(app PRIVATE src/util)
target_include_directories(app PRIVATE src/library)
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE src/module)
target_include_directories(app PRIVATE src/bsp)
target_include_directories(app PRIVATE src/config)

# Application directories
add_subdirectory(src/library)
add_subdirectory(src/config)
add_subdirectory(src/module)
add_subdirectory(src/bsp)

# NORDIC SDK APP END
