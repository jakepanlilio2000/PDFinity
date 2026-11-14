# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)

cmake_minimum_required(VERSION 3.25)

project(PDFinity
    VERSION 0.1.0
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
