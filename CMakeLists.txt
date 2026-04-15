cmake_minimum_required(VERSION 3.10)

project(hxed LANGUAGES C VERSION 1.4.3)

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
    src/Config.c
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
        -Wall -Wextra
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g -O0>
    )
else()
    target_compile_options(hxed PRIVATE
        /W4
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Zi /Od>
    )
endif()

include(GNUInstallDirs)

install(TARGETS hxed
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (UNIX)
    install(FILES man/hxed.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )

    install(
        FILES completions/hxed.bash
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/bash-completion/completions
        RENAME hxed
    )

    install(
        FILES completions/hxed.fish
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/fish/vendor_completions.d
    )

    install(
        FILES completions/hxed.zsh
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/zsh/site-functions
        RENAME _hxed
    )

    install(CODE "
        if(NOT \"\${CMAKE_INSTALL_PREFIX}\" MATCHES \"^/usr\")
            message(STATUS \"--- Note ---\")
            message(STATUS \"Zsh completion installed to \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/zsh/site-functions\")
            message(STATUS \"If it's not working, add this to your ~/.zshrc:\")
            message(STATUS \"  fpath=(\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/zsh/site-functions \\\$fpath)\")
            message(STATUS \"  autoload -Uz compinit && compinit\")
        endif()
    ")
endif()

if (WIN32)
    set(PS_DEST "${CMAKE_INSTALL_DATAROOTDIR}/powershell/hxed.ps1")
    
    install(
        FILES completions/hxed.ps1
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/powershell
    )

    # Hinweis für Windows-Nutzer
    install(CODE "
        message(STATUS \"--- PowerShell Completion Note ---\")
        message(STATUS \"To enable autocompletion, add the following line to your \$PROFILE:\")
        message(STATUS \"  . \\\"\${CMAKE_INSTALL_PREFIX}/${PS_DEST}\\\"\")
    ")
endif()

enable_testing()
add_test(NAME usage_test COMMAND hxed --version)

set(CPACK_PACKAGE_NAME "hxed")
set(CPACK_PACKAGE_VENDOR "jjice")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern cross-platform hex viewer")
set(CPACK_PACKAGE_CONTACT "pm:github@jjice")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "hxed")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP
        "${CMAKE_CURRENT_SOURCE_DIR}/.docs/normal.bmp")

elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;DragNDrop")

else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")

    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
    set(CPACK_RPM_PACKAGE_RELEASE 1)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
endif()

include(CPack)