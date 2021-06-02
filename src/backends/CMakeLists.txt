cmake_minimum_required (VERSION 3.19)

option(BACKEND_GLFW3 on)

if(BACKEND_GLFW3)
find_package(glfw3 CONFIG REQUIRED)
#target_link_libraries(main PRIVATE glfw)
endif()
