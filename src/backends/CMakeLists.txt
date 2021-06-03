cmake_minimum_required (VERSION 3.19)

set(BACKEND_GLFW3 on)

if(BACKEND_GLFW3)
    find_package(glfw3 CONFIG REQUIRED)
    add_library(backend_glfw3 SHARED glfw3.cpp)
    target_link_libraries(backend_glfw3 PRIVATE glfw)
endif()

set(BACKEND_STB_FONT on)
if(BACKEND_STB_FONT)
    find_path(STB_INCLUDE_DIRS "stb.h")
    add_library(backend_stb_font SHARED stbfont.cpp)
    target_include_directories(backend_stb_font PRIVATE ${STB_INCLUDE_DIRS})
endif()
