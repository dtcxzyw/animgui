cmake_minimum_required (VERSION 3.19)

if(BACKEND_GLFW3)
    find_package(glfw3 CONFIG REQUIRED)
    add_library(backend_glfw3 SHARED glfw3.cpp)
    target_link_libraries(backend_glfw3 PRIVATE glfw)
    target_compile_definitions(backend_glfw3 PRIVATE ANIMGUI_EXPORT)
    if(WIN32)
    target_link_libraries(backend_glfw3 PRIVATE Imm32)
    endif()
endif()

if(BACKEND_STB_FONT)
    find_path(STB_INCLUDE_DIRS "stb/stb_truetype.h")
    add_library(backend_stb_font SHARED stbfont.cpp)
    target_include_directories(backend_stb_font PRIVATE ${STB_INCLUDE_DIRS}/stb)
    target_compile_definitions(backend_stb_font PRIVATE ANIMGUI_EXPORT)
endif()

if(BACKEND_OPENGL3)
    find_package(GLEW REQUIRED)
    add_library(backend_opengl3 SHARED opengl3.cpp)
    target_link_libraries(backend_opengl3 PRIVATE GLEW::GLEW)
    target_compile_definitions(backend_opengl3 PRIVATE ANIMGUI_EXPORT)
endif()

if(BACKEND_D3D11)
    add_library(backend_d3d11 SHARED d3d11.cpp)
    target_compile_definitions(backend_d3d11 PRIVATE ANIMGUI_EXPORT)
    target_link_libraries(backend_d3d11 PRIVATE d3dcompiler)
endif()


if(BACKEND_D3D12)
    find_package(directx-headers CONFIG REQUIRED)
    add_library(backend_d3d12 SHARED d3d12.cpp)
    target_compile_definitions(backend_d3d12 PRIVATE ANIMGUI_EXPORT)
    target_link_libraries(backend_d3d12 PRIVATE d3dcompiler Microsoft::DirectX-Guids Microsoft::DirectX-Headers D3D12)
endif()

if(BACKEND_VULKAN)
    find_package(Vulkan REQUIRED)
    add_custom_target(vulkan_shader_module 
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -c ${CMAKE_CURRENT_SOURCE_DIR}/shader.vert --target-env=vulkan1.2 --target-spv=spv1.5 -O -mfmt=c -o vert.spv.hpp
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -c ${CMAKE_CURRENT_SOURCE_DIR}/shader.frag --target-env=vulkan1.2 --target-spv=spv1.5 -O -mfmt=c -o frag.spv.hpp
    BYPRODUCTS vert.spv.hpp frag.spv.hpp
    SOURCES shader.vert shader.frag
    )

    add_library(backend_vulkan SHARED vulkan.cpp)
    target_include_directories(backend_vulkan PRIVATE ${Vulkan_INCLUDE_DIR})
    target_include_directories(backend_vulkan PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_definitions(backend_vulkan PRIVATE ANIMGUI_EXPORT)
    target_link_libraries(backend_vulkan PRIVATE Vulkan::Vulkan)
    add_dependencies(backend_vulkan vulkan_shader_module)
endif()

if(BACKEND_METAL)
endif()
