add_library(operation_generation
    operation_generator.cpp
)

target_include_directories(operation_generation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(operation_generation_tests
    tests/operation_gen_test.cpp
)

target_link_libraries(operation_generation_tests
    PRIVATE
        operation_generation
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(operation_generation_tests)