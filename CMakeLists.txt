cmake_minimum_required(VERSION 3.16)

project(in_memory_file_systems LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(filesystem_a
    src/lib/A_fs.cpp
)

target_include_directories(filesystem_a
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
)

target_compile_options(filesystem_a
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

include(CTest)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(filesystem_a_tests
        tests/A_fs_test.cpp
    )

    target_link_libraries(filesystem_a_tests
        PRIVATE
            filesystem_a
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(filesystem_a_tests)
endif()
