# Copyright (c) 2024 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_INPUT_PROCESSOR_SPEED_CURVE)
  target_sources(app PRIVATE src/speed_curve.c)
  target_include_directories(app PRIVATE include)
endif()
