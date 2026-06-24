add_executable(test_fs_generator
    test_fs_generator.cpp
)

target_compile_features(test_fs_generator PRIVATE cxx_std_23)

target_link_libraries(test_fs_generator PRIVATE
    fs_generator
    GTest::gtest_main
)

target_include_directories(test_fs_generator PRIVATE ../../)

gtest_discover_tests(test_fs_generator)
