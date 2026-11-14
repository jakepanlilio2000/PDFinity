# PDFium Package Configuration for CMake
#
# To use PDFium in your CMake project:
#
#   1. set the environment variable PDFium_DIR to the folder containing this file.
#   2. in your CMakeLists.txt, add
#       find_package(PDFium)
#   3. then link your executable with PDFium
#       target_link_libraries(my_exe pdfium)

include(FindPackageHandleStandardArgs)

find_path(PDFium_INCLUDE_DIR
    NAMES "fpdfview.h"
    PATHS "${CMAKE_CURRENT_LIST_DIR}"
    PATH_SUFFIXES "include"
)

set(PDFium_VERSION "159.0.8152.0")
set(PDFIUM_BUILD_VER "8152")

if(WIN32)
  if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/bin/pdfium.dll" OR NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/pdfium.dll.lib")
    message(STATUS "PDFium Windows binaries not found. Downloading chromium/${PDFIUM_BUILD_VER}...")
    set(_PDFIUM_URL "https://github.com/bblanchon/pdfium-binaries/releases/download/chromium%2F${PDFIUM_BUILD_VER}/pdfium-win-x64.tgz")
    set(_ARCHIVE "${CMAKE_CURRENT_LIST_DIR}/pdfium-temp.tgz")
    file(DOWNLOAD "${_PDFIUM_URL}" "${_ARCHIVE}" SHOW_PROGRESS STATUS _status)
    list(GET _status 0 _status_code)
    if(_status_code EQUAL 0)
      execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${_ARCHIVE}" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
      file(REMOVE "${_ARCHIVE}")
    else()
      message(WARNING "Failed to download PDFium from ${_PDFIUM_URL}")
    endif()
  endif()

  find_file(PDFium_LIBRARY
        NAMES "pdfium.dll"
        PATHS "${CMAKE_CURRENT_LIST_DIR}"
        PATH_SUFFIXES "bin")

  find_file(PDFium_IMPLIB
        NAMES "pdfium.dll.lib"
        PATHS "${CMAKE_CURRENT_LIST_DIR}"
        PATH_SUFFIXES "lib")

  add_library(pdfium SHARED IMPORTED)
  set_target_properties(pdfium
    PROPERTIES
    IMPORTED_LOCATION             "${PDFium_LIBRARY}"
    IMPORTED_IMPLIB               "${PDFium_IMPLIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${PDFium_INCLUDE_DIR};${PDFium_INCLUDE_DIR}/cpp"
  )

  find_package_handle_standard_args(PDFium
    REQUIRED_VARS PDFium_LIBRARY PDFium_IMPLIB PDFium_INCLUDE_DIR
    VERSION_VAR PDFium_VERSION
  )
else()
  if(APPLE)
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/libpdfium.dylib")
      message(STATUS "PDFium macOS binaries not found. Downloading chromium/${PDFIUM_BUILD_VER}...")
      set(_PDFIUM_URL "https://github.com/bblanchon/pdfium-binaries/releases/download/chromium%2F${PDFIUM_BUILD_VER}/pdfium-mac-univ.tgz")
      set(_ARCHIVE "${CMAKE_CURRENT_LIST_DIR}/pdfium-temp.tgz")
      file(DOWNLOAD "${_PDFIUM_URL}" "${_ARCHIVE}" SHOW_PROGRESS STATUS _status)
      list(GET _status 0 _status_code)
      if(_status_code EQUAL 0)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${_ARCHIVE}" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
        file(REMOVE "${_ARCHIVE}")
      else()
        message(WARNING "Failed to download PDFium from ${_PDFIUM_URL}")
      endif()
    endif()
  else()
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/libpdfium.so")
      message(STATUS "PDFium Linux binaries not found. Downloading chromium/${PDFIUM_BUILD_VER}...")
      set(_PDFIUM_URL "https://github.com/bblanchon/pdfium-binaries/releases/download/chromium%2F${PDFIUM_BUILD_VER}/pdfium-linux-x64.tgz")
      set(_ARCHIVE "${CMAKE_CURRENT_LIST_DIR}/pdfium-temp.tgz")
      file(DOWNLOAD "${_PDFIUM_URL}" "${_ARCHIVE}" SHOW_PROGRESS STATUS _status)
      list(GET _status 0 _status_code)
      if(_status_code EQUAL 0)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${_ARCHIVE}" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
        file(REMOVE "${_ARCHIVE}")
      else()
        message(WARNING "Failed to download PDFium from ${_PDFIUM_URL}")
      endif()
    endif()
  endif()

  find_library(PDFium_LIBRARY
        NAMES "pdfium"
        PATHS "${CMAKE_CURRENT_LIST_DIR}"
        PATH_SUFFIXES "lib")

  add_library(pdfium SHARED IMPORTED)
  set_target_properties(pdfium
    PROPERTIES
    IMPORTED_LOCATION             "${PDFium_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PDFium_INCLUDE_DIR};${PDFium_INCLUDE_DIR}/cpp"
  )

  find_package_handle_standard_args(PDFium
    REQUIRED_VARS PDFium_LIBRARY PDFium_INCLUDE_DIR
    VERSION_VAR PDFium_VERSION
  )
endif()
