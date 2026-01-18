cmake_minimum_required(VERSION 3.16)

project(scripts)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(SCRIPT_FILE ${SCRIPTS})
    get_filename_component(SCRIPT_NAME ${SCRIPT_FILE} NAME_WE)
    
    add_library(${SCRIPT_NAME} SHARED ${SCRIPT_FILE})

    target_link_libraries(${SCRIPT_NAME} PRIVATE
        Engine
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        nlohmann_json::nlohmann_json
        fastgltf::fastgltf
        glm::glm-header-only
    )
endforeach()