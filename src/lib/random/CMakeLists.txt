add_library(rnd STATIC random.cpp)
target_compile_features(rnd PUBLIC cxx_std_23)

add_subdirectory(tests)