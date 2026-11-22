cmake_minimum_required(VERSION 3.22.1)
project("shawarmaui_demo")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..")

add_library(shawarmaui_demo SHARED
    main.cpp
    "${REPO_ROOT}/third_party/nanovg/nanovg.c"
)

target_include_directories(shawarmaui_demo PRIVATE
    "${REPO_ROOT}/include"
    "${REPO_ROOT}/third_party/nanovg"
)

target_compile_options(shawarmaui_demo PRIVATE -O3 -fvisibility=hidden)

target_link_libraries(shawarmaui_demo
    GLESv3
    EGL
    android
    log
)
