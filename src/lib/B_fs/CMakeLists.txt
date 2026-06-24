add_library(B_fs
    B_fs.cpp
)

target_include_directories(B_fs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(B_fs PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
