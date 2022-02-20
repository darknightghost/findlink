cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(findlink)

###################################System#####################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Build type
if (NOT CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE "Debug")

endif ()

if (NOT UNIX)
    message (FATAL_ERROR        "Operation system is not supported.")

endif ()

set (CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"  CACHE STRING "${CMAKE_BUILD_TYPE}" FORCE)

if (GENERATOR_IS_MULTI_CONFIG OR CMAKE_CONFIGURATION_TYPES)
    set (CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "${CMAKE_BUILD_TYPE}" FORCE)

endif()

message (STATUS "Build Type - ${CMAKE_BUILD_TYPE}.")
message (STATUS "Host System - ${CMAKE_HOST_SYSTEM_NAME}.")
message (STATUS "Host Processor - ${CMAKE_HOST_SYSTEM_PROCESSOR}.")
message (STATUS "Target System - ${CMAKE_SYSTEM_NAME}.")
message (STATUS "Target Processor - ${CMAKE_SYSTEM_PROCESSOR}.")

# Environment.
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Options
add_compile_options("-fPIC" "-fexceptions" "-g")

# Output path.
set (OUTPUT_SUB_DIR "${CMAKE_BUILD_TYPE}/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY	"${CMAKE_CURRENT_SOURCE_DIR}/lib/${OUTPUT_SUB_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY	"${CMAKE_CURRENT_SOURCE_DIR}/lib/${OUTPUT_SUB_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUT_SUB_DIR}")

link_directories ("${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

# Platform options.
# Debug/Release.
# Enable address sanitizer in debug mode if possible.
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
			OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang"
			OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		# Enable address sanitizer.
		add_compile_options (-fno-omit-frame-pointer -fsanitize=address)
		add_link_options (-fno-omit-frame-pointer -fsanitize=address)
		message (STATUS "Address sanitizer is enabled.")

	endif()

endif ()


file (GLOB_RECURSE SRC
	"source/*.cc"
	"source/*.c"
	)

add_executable(${PROJECT_NAME}    
    ${SRC})

target_link_libraries(${PROJECT_NAME}    
    pthread)
