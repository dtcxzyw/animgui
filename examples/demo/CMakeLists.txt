cmake_minimum_required (VERSION 3.19)

message(STATUS "entrys for examples:")
foreach(ENTRY IN LISTS ENTRYS)
message(STATUS ${ENTRY})
endforeach()

foreach(ENTRY IN LISTS ENTRYS)
set(TARGET_NAME "demo_${ENTRY}")
add_executable(${TARGET_NAME} demo.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE "entry_${ENTRY}")
endforeach()
