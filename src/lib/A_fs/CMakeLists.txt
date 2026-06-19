add_library(filesystem_a STATIC
    A_fs.cpp
)

target_compile_features(filesystem_a PUBLIC cxx_std_23)

target_include_directories(filesystem_a
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
