cmake_minimum_required(VERSION 3.10)
project(json_srv)

set(CMAKE_CXX_STANDARD 11)

set(AERON_C_CLIENT_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/main/c/aeron")

SET(AERON_C_CLIENT_SOURCE
    ${AERON_C_CLIENT_SOURCE_PATH}/concurrent/aeron_atomic.c
    ${AERON_C_CLIENT_SOURCE_PATH}/concurrent/aeron_spsc_rb.c
    ${AERON_C_CLIENT_SOURCE_PATH}/util/aeron_bitutil.c
    ${AERON_C_CLIENT_SOURCE_PATH}/aeron_alloc.c)

set(AERON_C_CLIENT_HEADERS
    ${AERON_C_CLIENT_SOURCE_PATH}/concurrent/aeron_atomic.h
    ${AERON_C_CLIENT_SOURCE_PATH}/concurrent/aeron_atomic64_gcc_x86_64.h
    ${AERON_C_CLIENT_SOURCE_PATH}/concurrent/aeron_rb.h
    ${AERON_C_CLIENT_SOURCE_PATH}/concurrent/aeron_spsc_rb.h
    ${AERON_C_CLIENT_SOURCE_PATH}/util/aeron_bitutil.h
    ${AERON_C_CLIENT_SOURCE_PATH}/aeron_alloc.h)

include_directories(src/main/c/aeron)

add_executable(json_srv ${AERON_C_CLIENT_HEADERS} ${AERON_C_CLIENT_SOURCE} src/main/c/main.cpp)
