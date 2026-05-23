cmake_minimum_required(VERSION 3.10.0)
project(toy-gpu-compiler-frontend VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(program parser.cpp program.cpp)
target_include_directories(program PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(program PROPERTIES OUTPUT_NAME "program")

option(ENABLE_TESTS "Build and run unit tests" ON)

if (ENABLE_TESTS) 
    enable_testing()
    find_package(Catch2 CONFIG REQUIRED)

    add_executable(compiler_tests
        test/test_parser.cpp
        parser.cpp
    )
    target_link_libraries(compiler_tests PRIVATE Catch2::Catch2WithMain)

    target_include_directories(compiler_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    include(CTest)
    include(Catch)
    catch_discover_tests(compiler_tests)
endif()

