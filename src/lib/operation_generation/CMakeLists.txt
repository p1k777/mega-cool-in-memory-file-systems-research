add_library(operation_generation
    operation_generator.cpp
)

target_include_directories(operation_generation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
