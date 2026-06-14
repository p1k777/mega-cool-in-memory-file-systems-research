add_executable(test_random test_random.cpp)

target_compile_features(test_random PRIVATE cxx_std_23)

target_include_directories(test_random PRIVATE ../)
target_link_libraries(test_random PRIVATE
    rnd
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_random)
