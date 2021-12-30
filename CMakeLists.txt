cmake_minimum_required(VERSION 3.21)
project(xkbdisplay)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

file(GLOB SRC src/*.cc src/*.h)

add_executable(xkbdisplay ${SRC})
target_link_libraries(xkbdisplay PRIVATE X11 Xft xkbcommon xcb X11-xcb xkbcommon-x11)
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_compile_options(xkbdisplay PRIVATE
            -Wall -Wextra -Wundef -Werror=return-type -Wconversion -Wpedantic
            -fno-exceptions -Wno-gnu-zero-variadic-macro-arguments -Wno-dollar-in-identifier-extension
            -O3 -Xclang -fcolor-diagnostics)
else ()
    target_compile_options(xkbdisplay PRIVATE
            -Wall -Wextra -Wundef -Werror=return-type -Wconversion -Wpedantic
            -fno-exceptions -Wno-gnu-zero-variadic-macro-arguments -Wno-dollar-in-identifier-extension
            -O0 -ggdb -Xclang -fcolor-diagnostics)
endif ()

