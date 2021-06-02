cmake_minimum_required (VERSION 3.19)

aux_source_directory(core CoreSrc)
aux_source_directory(components ComponentsSrc)
add_library(aimgui SHARED ${CoreSrc} ${ComponentsSrc})

add_subdirectory(backends)
