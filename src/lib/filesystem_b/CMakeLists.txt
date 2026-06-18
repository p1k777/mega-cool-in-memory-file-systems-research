add_library(filesystem_b
    filesystem_b.cpp
)

target_include_directories(filesystem_b
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(filesystem_b PUBLIC cxx_std_23)

add_executable(filesystem_b_tests
    tests/filesystem_b_test.cpp
)

target_link_libraries(filesystem_b_tests
    PRIVATE
        filesystem_b
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(filesystem_b_tests)