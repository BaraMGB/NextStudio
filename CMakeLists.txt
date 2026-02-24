cmake_minimum_required(VERSION 3.15)

set(NEXTSTUDIO_VERSION "0.01")

project(NextStudio VERSION ${NEXTSTUDIO_VERSION})
option(JUCE_BUILD_EXTRAS "Build JUCE Examples" OFF)
option(JUCE_BUILD_EXAMPLES "Build JUCE Extras" OFF)

set(TargetName ${PROJECT_NAME})

add_subdirectory(App)

include(GNUInstallDirs)

# define App Paths cross plattform (User Config)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(APP_DATA_DIR "$ENV{HOME}/.config/${PROJECT_NAME}")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(APP_DATA_DIR "$ENV{HOME}/Library/Application Support/${PROJECT_NAME}")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}" LOCALAPPDATA_PATH)
    set(APP_DATA_DIR "${LOCALAPPDATA_PATH}/${PROJECT_NAME}")
endif()

# Install binary and language file
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    install(TARGETS ${TargetName}
            RUNTIME DESTINATION bin
            BUNDLE DESTINATION .
            COMPONENT Applications
    )

    # install(DIRECTORY ${CMAKE_SOURCE_DIR}/language
    #         DESTINATION ${APP_DATA_DIR}
    #         PATTERN ".git" EXCLUDE
    #         PATTERN "*.md" EXCLUDE
    # )

endif()

# =========================================================================
# CPACK configuration
# =========================================================================

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${NEXTSTUDIO_VERSION}")
set(CPACK_PACKAGE_VENDOR "NextStudio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Your NextStudio Application")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENCE.md")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

# Linux
if(UNIX AND NOT APPLE)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Steffen Baranowsky <baramgb@nextstudio-daw.org>")
    set(CPACK_RPM_PACKAGE_RELEASE 1)
    set(CPACK_RPM_PACKAGE_LICENSE "AGPL")

    install(FILES ${CMAKE_SOURCE_DIR}/resources/nextstudio.desktop
            DESTINATION share/applications
            RENAME com.nextstudio.NextStudio.desktop)

    install(FILES ${CMAKE_SOURCE_DIR}/resources/logo.svg
            DESTINATION share/icons/hicolor/scalable/apps
            RENAME com.nextstudio.NextStudio.svg)

    install(FILES ${CMAKE_SOURCE_DIR}/resources/nextstudio.png
            DESTINATION share/icons/hicolor/256x256/apps
            RENAME com.nextstudio.NextStudio.png)
endif()

# Windows NSIS
if(WIN32)
    set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME}")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/app_icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/app_icon.ico")
    
    set(CPACK_PACKAGE_EXECUTABLES "${TargetName}" "${PROJECT_NAME}")
    set(CPACK_CREATE_DESKTOP_LINKS "${TargetName}")
endif()

include(CPack)
