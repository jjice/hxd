cmake_minimum_required(VERSION 3.10)
project(hxed LANGUAGES C VERSION 1.5.0)

include(GNUInstallDirs)

if(WIN32)
    set(GLOBAL_CONFIG_DIR ".") 
else()
    # Using /etc/hxed oder <prefix>/etc/hxed
    set(GLOBAL_CONFIG_DIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/hxed")
endif()

configure_file(
    "${PROJECT_SOURCE_DIR}/include/hxed_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/hxed_config.h"
    @ONLY
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

install(TARGETS hxed
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (UNIX)
    # Manpages
    install(FILES man/hxed.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )

    # Bash
    install(
        FILES completions/hxed.bash
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/bash-completion/completions
        RENAME hxed
    )

    # Fish
    install(
        FILES completions/hxed.fish
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/fish/vendor_completions.d
    )

    # Zsh
    install(
        FILES completions/hxed.zsh
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/zsh/site-functions
        RENAME _hxed
    )

    # Config-File (Global)
    install(
        FILES misc/hxed.conf
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/hxed
    )

    # ZSH Hints
    install(CODE "
        if(NOT \"\${CMAKE_INSTALL_PREFIX}\" MATCHES \"^/usr$\")
            message(STATUS \"--- Zsh Completion Note ---\")
            message(STATUS \"Add this to your ~/.zshrc if completion fails:\")
            message(STATUS \"  fpath=(\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/zsh/site-functions \\\$fpath)\")
            message(STATUS \"  autoload -Uz compinit && compinit\")
        endif()
    ")
endif()

if (WIN32)
    set(PS_REL_PATH "${CMAKE_INSTALL_DATAROOTDIR}/powershell/hxed.ps1")
    
    install(
        FILES completions/hxed.ps1
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/powershell
    )

    install(
        FILES misc/hxed.conf
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/hxed
    )

    install(CODE "
        file(TO_NATIVE_PATH \"\${CMAKE_INSTALL_PREFIX}/${PS_REL_PATH}\" PS_NATIVE_PATH)
        # Backslashes für PowerShell escapen
        string(REPLACE \"\\\\\" \"/\" PS_NATIVE_PATH \"\${PS_NATIVE_PATH}\")
        message(STATUS \"--- PowerShell Completion Note ---\")
        message(STATUS \"To enable autocompletion, add this to your \$PROFILE:\")
        message(STATUS \"  . \\\"\${PS_NATIVE_PATH}\\\"\")
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

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")

include(CPack)