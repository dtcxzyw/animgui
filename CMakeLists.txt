cmake_minimum_required (VERSION 3.19)

project(animgui)
set(CMAKE_CXX_STANDARD 17)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_definitions("UNICODE")
	add_compile_definitions("_UNICODE")
	add_compile_options("/utf-8")
endif()

include_directories(BEFORE include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(examples)

install(FILES LICENSE DESTINATION ./)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include DESTINATION ./)
install(DIRECTORY ${CMAKE_BINARY_DIR}/lib DESTINATION ./)
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin DESTINATION ./)
