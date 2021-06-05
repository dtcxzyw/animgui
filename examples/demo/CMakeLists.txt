cmake_minimum_required (VERSION 3.19)

find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
add_executable(demo demo.cpp)
target_link_libraries(demo PRIVATE animgui backend_glfw3 backend_stb_font backend_opengl3)
target_link_libraries(demo PRIVATE glfw GLEW::GLEW)
