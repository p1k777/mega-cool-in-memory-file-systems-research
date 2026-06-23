add_executable(test_path_generator test_path_generator.cpp)

target_compile_features(test_path_generator PRIVATE cxx_std_23)

target_link_libraries(
    test_path_generator
    PRIVATE path_generator
    PRIVATE GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_path_generator)
