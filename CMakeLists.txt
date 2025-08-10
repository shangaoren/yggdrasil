project(yggdrasil)
cmake_minimum_required(VERSION 3.6)
enable_language(ASM C CXX)

file(GLOB ${PROJECT_NAME}_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/kernel/Task.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/kernel/Scheduler.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/kernel/Mutex.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/kernel/Event.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/kernel/CriticalSection.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/framework/assert.cpp"
        "core/cortex_m/CortexM.cpp"
        )
add_library(${PROJECT_NAME}  ${${PROJECT_NAME}_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/framework
        ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel
        ${CMAKE_CURRENT_SOURCE_DIR}/
        )