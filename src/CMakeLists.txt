project(yggdrasil)
cmake_minimum_required(VERSION 3.6)
enable_language(ASM C CXX)

file(GLOB ${PROJECT_NAME}_SOURCES
        "kernel/Task.cpp"
        "kernel/Scheduler.cpp"
        "kernel/Mutex.cpp"
        "kernel/Event.cpp"
        "kernel/CriticalSection.cpp"
        "framework/assert.cpp"
        "core/cortex_m/CortexM.cpp"
        )
add_library(${PROJECT_NAME}  ${${PROJECT_NAME}_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
        framework
        ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
        kernel
        ..
        )