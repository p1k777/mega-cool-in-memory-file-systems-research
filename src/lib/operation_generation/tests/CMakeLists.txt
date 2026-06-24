add_executable(operation_generation_tests
    operation_gen_test.cpp
)

target_compile_features(operation_generation_tests PRIVATE cxx_std_23)

target_link_libraries(operation_generation_tests
    PRIVATE
        operation_generation
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(operation_generation_tests)
