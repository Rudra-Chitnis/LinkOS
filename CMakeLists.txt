cmake_minimum_required(VERSION 3.20)
project(LinkOS VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(LINKOS_BUILD_TESTS "Build LinkOS tests" ON)

add_executable(linkos
    src/main.c
    src/cli/commands.c
    src/core/log.c
    src/platform/platform.c
)
target_include_directories(linkos PRIVATE include)
target_compile_definitions(linkos PRIVATE
    LINKOS_VERSION="${PROJECT_VERSION}"
    LINKOS_BUILD_TYPE="$<CONFIG>"
)

if(MSVC)
    target_compile_options(linkos PRIVATE /W4 /WX)
else()
    target_compile_options(linkos PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(LINKOS_BUILD_TESTS)
    enable_testing()
    add_executable(linkos_tests
        tests/test_cli.c
        src/cli/commands.c
        src/core/log.c
        src/platform/platform.c
    )
    target_include_directories(linkos_tests PRIVATE include)
    target_compile_definitions(linkos_tests PRIVATE LINKOS_VERSION="${PROJECT_VERSION}")
    if(MSVC)
        target_compile_options(linkos_tests PRIVATE /W4 /WX)
    else()
        target_compile_options(linkos_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME linkos_cli_tests COMMAND linkos_tests)
endif()
