cmake_minimum_required(VERSION 3.10)
project(hxd LANGUAGES C VERSION 1.0.0)
configure_file(include/version.h.in include/version.h)

# Clangd-Support für IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 1. TARGET DEKLARIEREN
add_executable(hxd 
    src/hxd.c
    src/Args.c
    src/MagicBytes.c
)

# 2. HEADER PFADE
target_include_directories(hxd PRIVATE ${PROJECT_SOURCE_DIR}/include)

if(UNIX AND NOT APPLE)
    target_link_libraries(hxd PRIVATE m)
endif()

# 3. OPTIMIERUNGEN
if (NOT MSVC)
    target_compile_options(hxd PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native>
        $<$<CONFIG:Debug>:-g -O0 -Wall>
    )
else()
    # Hier war noch ein Tippfehler ("mein_tool" statt "hxd")
    target_compile_options(hxd PRIVATE
        $<$<CONFIG:Release>:/O2 /arch:AVX2>
        $<$<CONFIG:Debug>:/Zi /Od>
    )
endif()

# 4. INSTALLATION
include(GNUInstallDirs)

install(TARGETS hxd
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
