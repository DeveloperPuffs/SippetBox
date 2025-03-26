cmake_minimum_required(VERSION 3.10)
project(SippetBox VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PNG REQUIRED)
find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/source/*.c")
add_executable(SippetBox ${SOURCES})

target_include_directories(SippetBox PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(SippetBox PRIVATE PNG::PNG)
target_link_libraries(SippetBox PRIVATE SDL3::SDL3)
target_link_libraries(SippetBox PRIVATE Vulkan::Vulkan)

find_program(GLSLANG_VALIDATOR glslangValidator)
if (NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "glslangValidator not found, please install Vulkan SDK or provide a path to glslangValidator")
endif()

set(SHADER_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/shaders" CACHE STRING "Shader Directory")

add_custom_command(
  OUTPUT ${SHADER_DIRECTORY}/shader.vert.spv
  COMMAND glslangValidator -V ${SHADER_DIRECTORY}/shader.vert -o ${SHADER_DIRECTORY}/shader.vert.spv
  DEPENDS ${SHADER_DIRECTORY}/shader.vert
  COMMENT "Compiling vertex shader"
)

add_custom_command(
  OUTPUT ${SHADER_DIRECTORY}/shader.frag.spv
  COMMAND glslangValidator -V ${SHADER_DIRECTORY}/shader.frag -o ${SHADER_DIRECTORY}/shader.frag.spv
  DEPENDS ${SHADER_DIRECTORY}/shader.frag
  COMMENT "Compiling fragment shader"
)

add_custom_target(Shaders DEPENDS ${SHADER_DIRECTORY}/shader.vert.spv ${SHADER_DIRECTORY}/shader.frag.spv)
add_dependencies(SippetBox Shaders)