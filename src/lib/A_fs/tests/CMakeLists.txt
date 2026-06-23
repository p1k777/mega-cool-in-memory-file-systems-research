add_executable(filesystem_a_tests
    A_fs_test.cpp
)

target_compile_features(filesystem_a_tests PRIVATE cxx_std_23)

target_link_libraries(filesystem_a_tests
    PRIVATE
        filesystem_a
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(filesystem_a_tests)
