# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gigaspark_m7)

target_sources(app PRIVATE
	m7/src/main.c
	m7/src/app_runner.c
	m7/src/fault_manager.c
	m7/src/nav_bar.c
	m7/src/power_manager.c
	m7/src/video_decoder.c
	m7/src/audio_i2s.c
	m7/src/net_manager.c
	m7/src/app_downloader.c
	sdk/gigaspark_api.c
)

target_include_directories(app PRIVATE
	m7/src
	common
	common/include
	sdk
)
