cmake_minimum_required(VERSION 3.10)
project(Sippet VERSION 1.0.0)

# Configure the project languages
if(APPLE)
  enable_language(OBJC)
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required dependencies
find_package(PNG REQUIRED)
find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

# Collect the source files for the engine
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/source/*.c")
if(APPLE)
  file(GLOB_RECURSE OBJECTIVE_C_SOURCES "${CMAKE_CURRENT_LIST_DIR}/source/*.m")
  list(APPEND SOURCES ${OBJECTIVE_C_SOURCES})
endif()

# Create the engine library with the sources
message(STATUS "SOURCES: ${SOURCES}")
add_library(Sippet STATIC ${SOURCES})

# Setup the engine's include directory
target_include_directories(Sippet PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

# Link libraries from the dependencies
target_link_libraries(Sippet PRIVATE PNG::PNG)
target_link_libraries(Sippet PRIVATE SDL3::SDL3)
target_link_libraries(Sippet PRIVATE Vulkan::Vulkan)
if(APPLE)
  target_link_libraries(Sippet PRIVATE "-framework Cocoa")
  find_library(METAL_FRAMEWORK Metal)
  target_link_libraries(Sippet PRIVATE ${METAL_FRAMEWORK})
  find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)
  target_link_libraries(Sippet PRIVATE ${QUARTZ_CORE_FRAMEWORK})
endif()

# Find glslangvalidator
find_program(GLSLANG_VALIDATOR glslangValidator)
if (NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "glslangValidator not found, please install Vulkan SDK or provide a path to glslangValidator")
endif()

# Compile the shaders (if not compiled)
set(SHADER_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/resources/shaders" CACHE STRING "Shader Directory")
add_custom_command(
  OUTPUT ${SHADER_DIRECTORY}/shader.vert.spv
  COMMAND glslangValidator -V ${SHADER_DIRECTORY}/shader.vert -o ${SHADER_DIRECTORY}/shader.vert.spv
  DEPENDS ${SHADER_DIRECTORY}/shader.vert
  COMMENT "Compiling vertex shader")
add_custom_command(
  OUTPUT ${SHADER_DIRECTORY}/shader.frag.spv
  COMMAND glslangValidator -V ${SHADER_DIRECTORY}/shader.frag -o ${SHADER_DIRECTORY}/shader.frag.spv
  DEPENDS ${SHADER_DIRECTORY}/shader.frag
  COMMENT "Compiling fragment shader")

# Link the shaders to the engine
add_custom_target(Shaders DEPENDS ${SHADER_DIRECTORY}/shader.vert.spv ${SHADER_DIRECTORY}/shader.frag.spv)
add_dependencies(Sippet Shaders)

# Add the subdirectory for the build target, which is expected to be passed from a script via a
# CACHE variable
if (DEFINED LAUNCH_TARGET)
  message(STATUS "Building target: ${LAUNCH_TARGET}")
  add_subdirectory(${LAUNCH_TARGET})
endif()