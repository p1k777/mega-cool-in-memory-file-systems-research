add_library(pathgen STATIC path_generator.cpp)

target_compile_features(pathgen PUBLIC cxx_std_23)