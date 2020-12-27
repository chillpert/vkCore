cmake_minimum_required(VERSION 3.16.2)
project(vkCore VERSION 0.0.1)

message("Using compiler: " + ${CMAKE_CXX_COMPILER_ID})

option(BUILD_EXAMPLES "Build examples" ON)
option(CLANG_TIDY_LINUX_DEBUG "Use clang-tidy for a Linux debug build" OFF)

if(NOT CMAKE_BUILD_TYPE)
  message("MX: Build type not set. Continuing with: Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "No build type was set" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(WIN32 OR MSVC)
  set(OS ${OS} "Windows")
  message("Renderer: Target platform Windows (x64)")
elseif(UNIX)
  set(OS ${OS} "Linux")
  message("vkCore: Target platform Linux (x64)")
else()
  message(FATAL_ERROR "vkCore: Target platform is not supported")
endif()

# VULKAN
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(vkCore SHARED source/vkCore.cpp)

set_target_properties(vkCore PROPERTIES COMPILE_DEFINITIONS "VK_CORE_BUILD_DLL")

target_link_libraries(vkCore
  ${Vulkan_LIBRARY}
)

set_target_properties(vkCore PROPERTIES
  CXX_STANDARD 17
)