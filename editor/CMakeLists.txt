cmake_minimum_required(VERSION 3.10)
project(SippetBox VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/source/*.c")
add_executable(SippetBox ${SOURCES})

target_link_libraries(SippetBox PRIVATE Sippet)

target_include_directories(SippetBox PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")