add_library(path_generator STATIC path_generator.cpp)

target_compile_features(path_generator PUBLIC cxx_std_23)

target_link_libraries(path_generator
    PUBLIC fs_generator
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
