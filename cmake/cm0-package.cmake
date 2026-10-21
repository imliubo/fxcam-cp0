#
# SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
#
# SPDX-License-Identifier: MIT
#

include(GNUInstallDirs)

set(APP_DISPLAY_NAME "FXCam")
set(APP_DEBIAN_REVISION "m5stack1" CACHE STRING "Debian package revision/vendor suffix")
set(APP_DEBIAN_ARCHITECTURE "arm64" CACHE STRING "Debian package architecture")
set(APP_MAINTAINER "imliubo <imliubo@makingfun.xyz>" CACHE STRING "Debian package maintainer")
set(APP_PACKAGE_DESCRIPTION "Camera with retro effects for M5CardputerZero APPLaunch")

set(APP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/package")
set(APP_LAUNCH_BINDIR "${CMAKE_INSTALL_DATADIR}/APPLaunch/bin")

string(TOLOWER "${APP_DISPLAY_NAME}" APP_DEBIAN_PACKAGE_NAME)
string(REGEX REPLACE "[^a-z0-9+.-]" "-" APP_DEBIAN_PACKAGE_NAME "${APP_DEBIAN_PACKAGE_NAME}")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${APP_LAUNCH_BINDIR}")

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${APP_NAME}/audio"
    PATTERN ".DS_Store" EXCLUDE
    PATTERN "._*" EXCLUDE
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${APP_NAME}/fonts"
    PATTERN ".DS_Store" EXCLUDE
    PATTERN "._*" EXCLUDE
)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/fxcam.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/share/images"
)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/fxcam.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/applications"
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/License.txt"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    RENAME "third-party-assets-license.txt"
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/dist")
set(CPACK_PACKAGE_NAME "${APP_DISPLAY_NAME}")
set(CPACK_PACKAGE_VENDOR "CardputerZero Community")
set(CPACK_PACKAGE_CONTACT "${APP_MAINTAINER}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_PACKAGE_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
    "${APP_DEBIAN_PACKAGE_NAME}_${PROJECT_VERSION}-${APP_DEBIAN_REVISION}_${APP_DEBIAN_ARCHITECTURE}"
)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_DEBIAN_PACKAGE_NAME "${APP_DEBIAN_PACKAGE_NAME}")
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "${APP_DEBIAN_REVISION}")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${APP_DEBIAN_ARCHITECTURE}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${APP_MAINTAINER}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
# Let package managers replace releases installed under the two previous names.
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "pocket-pixel, filter-camera")
set(CPACK_DEBIAN_PACKAGE_REPLACES "pocket-pixel, filter-camera")

set(APP_DEBIAN_PACKAGE_DEPENDS
    libc6
    libstdc++6
    libgcc-s1
    libfreetype6
    libpng16-16
    libjpeg62-turbo
    zlib1g
    libfmt10
    "libcamera0.7 | libcamera0.6 | libcamera0.5 | libcamera0.4 | libcamera0.3 | libcamera0.2 | libcamera0"
    libcamera-ipa
    libasound2
    libcjson1
    libv4l-0
)
if(APP_USE_DRM)
    list(APPEND APP_DEBIAN_PACKAGE_DEPENDS libdrm2)
endif()
list(REMOVE_DUPLICATES APP_DEBIAN_PACKAGE_DEPENDS)
string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_DEPENDS "${APP_DEBIAN_PACKAGE_DEPENDS}")

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/templates/debian-control.in"
    "${APP_GENERATED_DIR}/control"
    @ONLY
)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)

include(CPack)
