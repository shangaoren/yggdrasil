cmake_minimum_required(VERSION 3.6)
project(yggdrasil)
enable_language(ASM C CXX)
message("-------------- YGGDRASIL KERNEL ----------------")

message("building kernel")
file(GLOB ${PROJECT_NAME}_SOURCES
        "src/kernel/Task.cpp"
        "src/kernel/Scheduler.cpp"
        "src/kernel/Mutex.cpp"
        "src/kernel/Event.cpp"
        "src/kernel/CriticalSection.cpp"
        "src/framework/assert.cpp"
        "src/core/cortex_m/CortexM.cpp"
)
add_library(${PROJECT_NAME} OBJECT ${${PROJECT_NAME}_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
        src/framework
        ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
        src/kernel
)