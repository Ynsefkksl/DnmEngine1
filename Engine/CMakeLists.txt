cmake_minimum_required(VERSION 3.16)

project(Engine)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED) 
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Window/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utility/*.cpp
    ${CMAKE_SOURCE_DIR}/Vendor/*.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    ENGINE_EXPORT
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
                        Vulkan::Vulkan
                        GPUOpen::VulkanMemoryAllocator
                        SDL3::SDL3
                        nlohmann_json::nlohmann_json 
                        fastgltf::fastgltf
                        glm::glm-header-only)
