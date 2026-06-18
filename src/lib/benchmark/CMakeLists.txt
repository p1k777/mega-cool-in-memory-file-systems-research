add_library(
    benchmark
    bench.cpp
)

target_include_directories(
    benchmark
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(
    benchmark
    PUBLIC 
        rnd
        fs_generator
        pathgen
        filesystem_a
        filesystem_b
        filesystem_c
        operation_generation
)


add_executable(
    bench_run
    bench_runner.cpp
)

target_link_libraries(
    bench_run
    PUBLIC
        benchmark
        filesystem_a
        filesystem_b
        filesystem_c
)

target_compile_features(benchmark PUBLIC cxx_std_23)
target_compile_features(bench_run PUBLIC cxx_std_23)