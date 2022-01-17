cmake_minimum_required(VERSION 3.0.0)
set (CMAKE_CXX_STANDARD 17)

# vcpkg Toolchain Link
set (CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/lib/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
set (VCPKG_MANIFEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

project(Rose VERSION 0.1.0)

# CPack Module for creating Installers
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# CTest Module
include(CTest)
enable_testing()

# Verify Dependencies are Installed
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_subdirectory(assets)
add_subdirectory(src/rendering/engine)

include(${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/shaders/compile_shaders.cmake)

add_library(VulkanEngine
    src/logging/logging.cpp
    src/rendering/engine/vk_engine.cpp
    src/rendering/engine/vk_mesh.cpp
    src/rendering/engine/vk_debug.cpp
    src/rendering/engine/vk_swapchain.cpp
    src/rendering/engine/ve_pipeline.cpp
    src/rendering/engine/output/vk_output.hpp
    src/rendering/engine/output/vk_glfw_output.cpp
    src/rendering/engine/ve_loader.hpp
    src/rendering/engine/models.cpp
)

target_link_libraries(VulkanEngine
PUBLIC Vulkan::Vulkan 
PRIVATE unofficial::vulkan-memory-allocator::vulkan-memory-allocator 
PRIVATE glm::glm
PRIVATE glfw
PRIVATE spdlog::spdlog spdlog::spdlog_header_only
PRIVATE assimp::assimp
)

target_include_directories(VulkanEngine
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/engine
)

add_executable(TestVulkanEngine
src/rendering/engine/test.cpp
src/logging/logging.cpp
)

target_link_libraries(TestVulkanEngine
PRIVATE spdlog::spdlog spdlog::spdlog_header_only
PRIVATE VulkanEngine
PRIVATE Vulkan::Vulkan 
PRIVATE glfw 
)

add_shaders(TestVulkanEngine
    triangle.frag
    basic_mesh.vert
)

# Add Executable File as Build target 
add_executable(Rose
    src/main.cpp
    src/logging/logging.cpp
)

target_include_directories(Rose 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Libraries
target_link_libraries(Rose 
    PRIVATE Vulkan::Vulkan 
    PRIVATE unofficial::vulkan-memory-allocator::vulkan-memory-allocator
    PRIVATE glfw 
    PRIVATE glm::glm
    PRIVATE spdlog::spdlog spdlog::spdlog_header_only
    PRIVATE Bullet3Common BulletDynamics
    PRIVATE imgui::imgui
)

# Vulkan Shader Dependencies
add_shaders(Rose 
    shader.vert 
    shader.frag
) 

