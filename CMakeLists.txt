cmake_minimum_required(VERSION 3.15)

project(Isolyx VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(isolyx
    src/main.cpp
    src/Parser.cpp
    src/Executor.cpp
    src/LinuxNamespaceIsolator.cpp
    src/PerLanguageRootfsProvider.cpp
    src/CgroupV2Limiter.cpp
    src/TimeoutWatchdog.cpp
    src/FCFSScheduler.cpp
    src/InMemoryJobQueue.cpp
    src/WorkerPool.cpp
    src/SeccompDenylistPolicy.cpp
)

target_link_libraries(isolyx seccomp)

target_include_directories(isolyx PRIVATE ${CMAKE_SOURCE_DIR}/include)