add_library(fs_generator STATIC
    generated_fs.cpp
    fs_generator.cpp
)

target_compile_features(fs_generator PUBLIC cxx_std_23)

target_link_libraries(fs_generator
    PUBLIC rnd
)

target_include_directories(fs_generator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_subdirectory(tests)