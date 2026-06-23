add_executable(B_fs_tests
    B_fs_test.cpp
)

target_compile_features(B_fs_tests PRIVATE cxx_std_23)

target_link_libraries(B_fs_tests
    PRIVATE
        B_fs
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(B_fs_tests)
