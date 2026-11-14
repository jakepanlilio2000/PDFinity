# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)

cmake_minimum_required(VERSION 3.25)

project(PDFinity
    VERSION 1.0.11
    DESCRIPTION "Professional, modular, offline-first PDF productivity suite"
    HOMEPAGE_URL "https://www.stacksyncsolutions.com/"
    LANGUAGES C CXX
)

set(PDFINITY_ORGANIZATION "StackSync Solutions")
set(PDFINITY_ORGANIZATION_URL "https://www.stacksyncsolutions.com/")
set(PDFINITY_LEAD_PROGRAMMER "Jake Ashley Panlilio")
set(PDFINITY_LEAD_PROGRAMMER_URL "https://github.com/jakepanlilio2000")
set(PDFINITY_LICENSE "Mozilla Public License Version 2.0 (MPL-2.0)")

# Enforce C++20 across all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable automatic Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt for Android Platform Configuration
if(ANDROID)
    if(EXISTS "${PROJECT_SOURCE_DIR}/packaging/android")
        set(QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/packaging/android")
    else()
        set(QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/android")
    endif()
    set(QT_ANDROID_ABIS "arm64-v8a;armeabi-v7a;x86_64" CACHE STRING "Target Android ABIs")
    set(QT_ANDROID_MIN_SDK_VERSION 28)
    set(QT_ANDROID_TARGET_SDK_VERSION 35)
    set(ANDROID_NATIVE_API_LEVEL 28)
endif()

# Default to Release if no configuration specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Strict compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
    # Enable multi-processor compilation
    add_compile_options(/MP)
    # Disable secure CRT warnings for standard portable functions
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Locate Qt 6 components
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    PrintSupport
    Test
    Network
)

# Locate Google PDFium
set(PDFium_DIR "${PROJECT_SOURCE_DIR}/third_party/pdfium")
find_package(PDFium REQUIRED)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

# Subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)

# ==============================================================================
# CPack Packaging Configuration (.deb, .rpm, .tar.gz, .zip)
# ==============================================================================
set(CPACK_PACKAGE_NAME "pdfinity")
set(CPACK_PACKAGE_VENDOR "StackSync Solutions")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional, modular, offline-first PDF productivity suite")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.stacksyncsolutions.com/")
set(CPACK_PACKAGE_CONTACT "StackSync Solutions <support@stacksyncsolutions.com>")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

# Debian Generator
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "StackSync Solutions <support@stacksyncsolutions.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "text")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.stacksyncsolutions.com/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.31), libstdc++6 (>= 11), libqt6core6 (>= 6.5.0), libqt6gui6 (>= 6.5.0), libqt6widgets6 (>= 6.5.0), libqt6network6 (>= 6.5.0), libqt6printsupport6 (>= 6.5.0)")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/packaging/linux/deb/postinst;${PROJECT_SOURCE_DIR}/packaging/linux/deb/postrm")

# RPM Generator
set(CPACK_RPM_PACKAGE_LICENSE "MPL-2.0")
set(CPACK_RPM_PACKAGE_URL "https://www.stacksyncsolutions.com/")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Publishing")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.5.0, qt6-qtbase-gui >= 6.5.0, hicolor-icon-theme")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/packaging/linux/rpm/postinst")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/packaging/linux/rpm/postrm")

include(CPack)
