cmake_minimum_required (VERSION 3.19)

project(animgui)
set(CMAKE_CXX_STANDARD 17)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_definitions("UNICODE")
	add_compile_definitions("_UNICODE")
	add_compile_options("/utf-8")
endif()

include_directories(BEFORE include)

add_subdirectory(src)
add_subdirectory(examples)
