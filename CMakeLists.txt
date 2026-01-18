cmake_minimum_required(VERSION 3.16)

project(game)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_definitions(
	-DIMGUI_USER_CONFIG="my_imgui_config.h"
	-DSPIRV_REFLECT_USE_SYSTEM_SPIRV_H=1
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/Engine
	${CMAKE_CURRENT_SOURCE_DIR}/Vendor
	${Stb_INCLUDE_DIR}
)

add_custom_target(RunScript 
	ALL COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/compile.bat
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
)

add_subdirectory(Engine)

add_subdirectory(Scripts)

add_executable(${PROJECT_NAME} Main.cpp)