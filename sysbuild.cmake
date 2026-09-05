# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Gigaspark OS

if("${SB_CONFIG_GIGASPARK_REMOTE_BOARD}" STREQUAL "")
  message(
    "Target ${BOARD} not supported for this sample. "
    "There is no remote board selected in Kconfig.sysbuild"
  )
endif()

set(REMOTE_APP gigaspark_m4)

ExternalZephyrProject_Add(
  APPLICATION ${REMOTE_APP}
  SOURCE_DIR  ${APP_DIR}/m4
  BOARD       ${SB_CONFIG_GIGASPARK_REMOTE_BOARD}
)

add_dependencies(${DEFAULT_IMAGE} ${REMOTE_APP})
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${REMOTE_APP})
