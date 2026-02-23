cmake_minimum_required(VERSION 3.10)
project(hxd LANGUAGES C VERSION 0.2)

# Clangd-Support für IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. TARGET DEKLARIEREN
add_executable(hxd 
    src/hxd.c
    src/Args.c
)

# 2. HEADER PFADE
target_include_directories(hxd PRIVATE ${PROJECT_SOURCE_DIR}/include)

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
