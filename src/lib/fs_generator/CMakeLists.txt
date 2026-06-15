add_library(fs_generator STATIC
    generated_fs.cpp
    fs_generator.cpp
)

target_compile_features(fs_generator PUBLIC cxx_std_23)
target_link_libraries(fs_generator
    PUBLIC rnd
)

add_subdirectory(tests)