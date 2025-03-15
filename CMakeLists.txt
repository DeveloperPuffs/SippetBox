cmake_minimum_required(VERSION 3.10)
project(SippetBox VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/source/*.c")
add_executable(SippetBox ${SOURCES})

target_include_directories(SippetBox PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(SippetBox PRIVATE SDL3::SDL3)
target_link_libraries(SippetBox PRIVATE Vulkan::Vulkan)