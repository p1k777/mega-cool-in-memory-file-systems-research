add_library(filesystem_c STATIC
    C_fs.cpp
)

target_compile_features(filesystem_c PUBLIC cxx_std_23)

target_include_directories(filesystem_c
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_subdirectory(tests)
