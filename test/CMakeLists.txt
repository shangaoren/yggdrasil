cmake_minimum_required(VERSION 3.14)
project(yggdrasil_unit_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)
include_directories(
        framework
        kernel
        mock
)
add_compile_definitions(USE_DUMMY_CORE)
add_compile_definitions(USE_DUMMY_VECTOR)
enable_testing()
add_executable(
        yggdrasil_test
        framework/YList_test.cpp
)
target_link_libraries(yggdrasil_test PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(yggdrasil_test)
