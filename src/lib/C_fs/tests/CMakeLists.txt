add_executable(filesystem_c_tests
    C_fs_test.cpp
)

target_compile_features(filesystem_c_tests PRIVATE cxx_std_23)

target_link_libraries(filesystem_c_tests
    PRIVATE
        filesystem_c
        GTest::gtest_main
)

gtest_discover_tests(filesystem_c_tests)
