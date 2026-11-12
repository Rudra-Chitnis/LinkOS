cmake_minimum_required(VERSION 3.25)
project(LinkOS VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(LINKOS_BUILD_TESTS "Build LinkOS tests" ON)

add_library(linkos_net STATIC src/net/socket.c)
target_include_directories(linkos_net PUBLIC include)
if(WIN32)
    target_link_libraries(linkos_net PUBLIC ws2_32)
    target_compile_definitions(linkos_net PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0600)
endif()
if(MSVC)
    target_compile_options(linkos_net PRIVATE /W4 /WX)
else()
    target_compile_options(linkos_net PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

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
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
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

    add_executable(linkos_net_tests tests/test_net.c)
    target_link_libraries(linkos_net_tests PRIVATE linkos_net)
    if(MSVC)
        target_compile_options(linkos_net_tests PRIVATE /W4 /WX)
    else()
        target_compile_options(linkos_net_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME linkos_net_tests COMMAND linkos_net_tests)
    add_test(NAME linkos_cli_integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli_integration.py"
                "$<TARGET_FILE:linkos>")
    set_tests_properties(linkos_cli_tests linkos_net_tests PROPERTIES TIMEOUT 20)
    set_tests_properties(linkos_cli_integration PROPERTIES TIMEOUT 30)
endif()
