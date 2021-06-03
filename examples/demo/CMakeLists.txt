cmake_minimum_required (VERSION 3.19)

find_package(glfw3 CONFIG REQUIRED)
add_executable(demo demo.cpp)
target_link_libraries(demo PRIVATE animgui backend_glfw3 backend_stb_font)
target_link_libraries(demo PRIVATE glfw)
