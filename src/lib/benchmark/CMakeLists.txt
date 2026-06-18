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
    bench
    PUBLIC 
        rnd
        fs_generator
        pathgen
        filesystem_a
        filesystem_b
        filesystem_c
)

target_compile_features(bench PUBLIC cxx_std_23)