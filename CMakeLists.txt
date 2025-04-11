# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/scd4x
)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensorflv)
target_sources(app PRIVATE src/main.c
                           src/sensor.c
                           src/storage_nvs.c
                           src/coap_client_utils.c)
                           