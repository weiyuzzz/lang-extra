cmake_minimum_required(VERSION 3.16)
project(langTextExtra VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Vendored third-party sources live in the original Visual Studio folder.
set(LANGTEXT_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/langTextExtra")

option(LANGTEXT_WITH_XLSX "Build vendored zlib + libxlsxwriter and enable .xlsx output" OFF)
option(LANGTEXT_BUILD_TESTS "Build the unit tests" ON)
option(LANGTEXT_BUILD_GUI "Build the Windows DuiLib GUI (MSVC only)" ON)

if(MSVC)
    set(LANGTEXT_WARNINGS /W4)
    set(LANGTEXT_UTF8 /utf-8)
else()
    set(LANGTEXT_WARNINGS -Wall -Wextra)
    set(LANGTEXT_UTF8 "")
endif()

# Only first-party sources are UTF-8. DuiLib is vendored GBK code and uses
# MSVC-only language extensions, so it is built as a separate target in
# gui/CMakeLists.txt WITHOUT /utf-8.
function(langtext_target_options target)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_compile_options(${target} PRIVATE ${LANGTEXT_WARNINGS})
    if(LANGTEXT_UTF8)
        target_compile_options(${target} PRIVATE ${LANGTEXT_UTF8})
    endif()
endfunction()

if(LANGTEXT_WITH_XLSX)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${LANGTEXT_VENDOR_DIR}/zlib-1.3.1" EXCLUDE_FROM_ALL)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${LANGTEXT_VENDOR_DIR}/libxlsxwriter" EXCLUDE_FROM_ALL)
    add_compile_definitions(LANGTEXT_HAS_XLSXWRITER=1)
endif()

add_subdirectory(core)
add_subdirectory(cli)

if(LANGTEXT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(WIN32 AND LANGTEXT_BUILD_GUI)
    add_subdirectory(gui)
endif()
