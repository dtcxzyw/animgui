cmake_minimum_required (VERSION 3.19)

set(BACKEND_GLFW3 on)

if(BACKEND_GLFW3)
    find_package(glfw3 CONFIG REQUIRED)
    add_library(backend_glfw3 SHARED glfw3.cpp)
    target_link_libraries(backend_glfw3 PRIVATE glfw)
    target_compile_definitions(backend_glfw3 PRIVATE ANIMGUI_EXPORT)
endif()

set(BACKEND_STB_FONT on)
if(BACKEND_STB_FONT)
    find_path(STB_INCLUDE_DIRS "stb.h")
    add_library(backend_stb_font SHARED stbfont.cpp)
    target_include_directories(backend_stb_font PRIVATE ${STB_INCLUDE_DIRS})
    target_compile_definitions(backend_stb_font PRIVATE ANIMGUI_EXPORT)
endif()


set(BACKEND_OPENGL3 on)
if(BACKEND_OPENGL3)
    find_package(GLEW REQUIRED)
    add_library(backend_opengl3 SHARED opengl3.cpp)
    target_link_libraries(backend_opengl3 PRIVATE GLEW::GLEW)
    target_compile_definitions(backend_opengl3 PRIVATE ANIMGUI_EXPORT)
endif()

set(BACKEND_D3D11 on)
if(BACKEND_D3D11)
    add_library(backend_d3d11 SHARED d3d11.cpp)
    target_compile_definitions(backend_d3d11 PRIVATE ANIMGUI_EXPORT)
    target_link_libraries(backend_d3d11 PRIVATE d3dcompiler)
endif()
