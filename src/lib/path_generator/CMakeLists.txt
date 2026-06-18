add_library(pathgen STATIC path_generator.cpp)

target_compile_features(pathgen PUBLIC cxx_std_23)

target_link_libraries(pathgen
    PUBLIC fs_generator
)

add_subdirectory(tests)