cmake_minimum_required(VERSION 3.0.0)
project(Rose VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

set(CMAKE_TOOLCHAIN_FILE "C:/Projects/Rose/Rose/src/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

message(${CMAKE_TOOLCHAIN_FILE})

find_package(Vulkan REQUIRED)

add_executable(Rose src/main.cpp)

target_link_libraries(Rose Vulkan::Vulkan)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
