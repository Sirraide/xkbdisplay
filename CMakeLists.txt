cmake_minimum_required(VERSION 3.27)
project(xkbdisplay VERSION 0.1.0 LANGUAGES CXX)

## ============================================================================
##  Global CMake Variables.
## ============================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## ============================================================================
##  Global compiler options.
## ============================================================================
## Turn on diagnostics colours.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

## Use mold as the default linker, if it exists.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(MOLD_LINKER "mold")
    if (MOLD_LINKER)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-fuse-ld=mold)
        endif()
        add_link_options(-fuse-ld=mold)
    endif()
endif()

## ============================================================================
##  Compiler options.
## ============================================================================
add_library(options INTERFACE)

## Flags for Clang and GCC.
target_compile_options(options INTERFACE
    ## Warnings.
    -Wall -Wextra     # Enable ‘all’ warnings.
    -Wundef           # Invalid #undef or undefined macro in #if.
    -Wcast-align      # Casting that changes alignment.
    -Wconversion      # Implicit conversions.
    -Wsign-conversion # Implicit sign conversions.
    -Wformat=2        # Stricter format checking.

    ## Disabled warnings.
    -Wno-unused-function
    -Wno-unused-local-typedefs

    ## NULL Errors.
    -Werror=nonnull # Passing NULL to nonnull parameter.

    ## Memory Errors.
    -Werror=address              # Suspicious use of addresses.
    -Werror=init-self            # Initialization of a variable with itself.
    -Werror=uninitialized

    ## Return type.
    -Werror=return-type
    -Wmissing-noreturn

    ## C/C++.
    -Werror=implicit-fallthrough
    -Werror=missing-include-dirs # User-specified include dir does not exist.
    -Werror=pointer-arith        # Disallow void* and function pointer arithmetic.
    -Werror=string-compare       # Nonsensical string comparisons.
    -Werror=switch               # Missing switch cases.
    # -Werror=switch-enum          # Switch on enum (even if there is a default case).
    -Werror=write-strings        # Strings in C should be const char*.

    ## C++.
    -Werror=missing-field-initializers
    -Werror=non-virtual-dtor
    -Werror=pessimizing-move
)

## Additional flags for GCC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(options INTERFACE
        -Wlogical-op      # Duplicate or unintended logical operators.
        -Werror=invalid-memory-model # For atomics.
        -Werror=maybe-uninitialized
        -Werror=missing-requires
        -Werror=return-local-addr
    )
endif()

## Additional flags for Clang.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(options INTERFACE
        -Werror=dangling
        -Werror=return-stack-address
    )
endif()

## Address Sanitiser.
if (ENABLE_ASAN)
    target_compile_options(options INTERFACE -fsanitize=address)
    target_link_options(options INTERFACE -fsanitize=address)
endif()

## Debug/Release flags.
target_compile_options(options INTERFACE
    $<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3>
    $<$<CONFIG:RELEASE>:-O3 -march=native>
)
target_link_options(options INTERFACE
    $<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3 -rdynamic>
    $<$<CONFIG:RELEASE>:-O3 -march=native>
)

include(FetchContent)
FetchContent_Declare(clopts
    GIT_REPOSITORY https://github.com/Sirraide/clopts
    GIT_TAG        v2.2.1
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/clopts"
)

FetchContent_Declare(stream
    GIT_REPOSITORY https://github.com/Sirraide/libstream
    GIT_TAG        master
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/stream"
)

FetchContent_MakeAvailable(clopts)
FetchContent_MakeAvailable(stream)
target_include_directories(options INTERFACE
    "${clopts_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
)

## ============================================================================
##  Executables and libraries.
## ============================================================================
## Add the executable.
file(GLOB_RECURSE sources "src/*.cc" "src/*.hh")
add_executable(xkbdisplay ${sources})

## Apply our options.
target_link_libraries(xkbdisplay PRIVATE options X11 Xft libstream xkbcommon)