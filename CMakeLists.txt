cmake_minimum_required(VERSION 3.30)

# Set VCPKG to correct triplet for Linux
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_SOURCE_DIR}/vcpkg-config/my-triplets" CACHE STRING "VCPKG overlay triplet")
    set(VCPKG_TARGET_TRIPLET "x64-linux-clang" CACHE STRING "VCPKG target triplet")
endif()

project(aras-fs LANGUAGES CXX)

set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Apply compiler flags
if (MSVC)
    add_compile_options(
        /W4
        /permissive-
        /EHsc
        /Zi
        /sdl
    )

    add_link_options(
        /DEBUG
    )

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC
    # Not working with modules right now. Will need checking later
    message(FATAL_ERROR "GCC not supported. Use Clang.")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang

    # Throw error if linking against libcxx
    if (CMAKE_CXX_COMPILER_LINKER_ID STREQUAL "LLD")
        message(FATAL_ERROR "Linking with libcxx not supported. Use libstdc++.")
    endif()

    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wno-unknown-pragmas
        -fno-omit-frame-pointer
        -fexceptions
        "-fsanitize=address,undefined,leak"
        -fcolor-diagnostics
    )

    add_link_options(
        "-fsanitize=address,undefined,leak"
    )

endif()

unset(BUILD_TESTS CACHE)
option(BUILD_TESTS "Build the tests" ON)

if(BUILD_TESTS)
    add_compile_definitions(TESTING)
endif()

add_subdirectory(src)

if(BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
