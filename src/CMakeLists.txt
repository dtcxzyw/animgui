cmake_minimum_required (VERSION 3.19)

find_package(utf8cpp CONFIG REQUIRED)

aux_source_directory(core CoreSrc)
aux_source_directory(builtins BuiltinsSrc)
add_library(animgui SHARED ${CoreSrc} ${BuiltinsSrc})
target_link_libraries(animgui PRIVATE utf8cpp)

add_subdirectory(backends)
