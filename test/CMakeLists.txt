project(yggdrasil_test)
cmake_minimum_required(VERSION 3.6)
enable_language(ASM C CXX)

set(CMAKE_CXX_STANDARD  20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "EXTERNAL: Clone Google Test Framework from Git repository...")
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
enable_testing()

add_subdirectory(../src)
add_subdirectory(tests)
