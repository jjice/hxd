cmake_minimum_required(VERSION 3.10)

project(hxed LANGUAGES C VERSION 1.4.1)

configure_file(
    "${PROJECT_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(hxed 
    src/hxed.c
    src/Args.c
    src/Display.c
    src/DisplayUtils.c
    src/MagicBytes.c
    src/File.c
    src/Utils.c
    ${HEADERS}
)

target_include_directories(hxed PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

if(UNIX AND NOT APPLE)
    target_link_libraries(hxed PRIVATE m)
endif()

if (NOT MSVC)
    target_compile_options(hxed PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g -O0 -Wall>
    )
else()
    target_compile_options(hxed PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Zi /Od>
    )
endif()

include(GNUInstallDirs)

install(TARGETS hxed
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
