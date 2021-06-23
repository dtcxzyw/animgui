cmake_minimum_required (VERSION 3.19)

if(BACKEND_OPENGL3 AND BACKEND_GLFW3 AND BACKEND_STB_FONT)
list(APPEND ENTRYS opengl3_glfw3)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
add_library(entry_opengl3_glfw3 opengl3_glfw3.cpp)
target_link_libraries(entry_opengl3_glfw3 PRIVATE animgui backend_glfw3 backend_stb_font backend_opengl3)
target_link_libraries(entry_opengl3_glfw3 PRIVATE glfw GLEW::GLEW)
endif()

if(BACKEND_D3D11 AND BACKEND_GLFW3 AND BACKEND_STB_FONT)
list(APPEND ENTRYS d3d11_glfw3)
add_library(entry_d3d11_glfw3 d3d11_glfw3.cpp)
target_link_libraries(entry_d3d11_glfw3 PRIVATE animgui backend_glfw3 backend_stb_font backend_d3d11)
target_link_libraries(entry_d3d11_glfw3 PRIVATE glfw D3D11)
endif()

set(ENTRYS ${ENTRYS} PARENT_SCOPE)
