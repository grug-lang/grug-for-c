cmake_minimum_required(VERSION 3.28)

project(grug
    VERSION 1.0.0
    DESCRIPTION "The perfect modding language"
    HOMEPAGE_URL "https://github.com/grug-lang/grug-for-c"
    LANGUAGES C
)

add_library(grug src/grug.c)

# Cmake be Cmake, and wants 90 instead of 89 to get ANSI 89 C.
set_target_properties(grug PROPERTIES C_STANDARD 90)
target_compile_options(grug PRIVATE -Wall -Wextra -Werror -pedantic-errors -Wconversion -g -fsanitize=address,undefined)
target_link_options(grug PRIVATE -fsanitize=address,undefined)
target_include_directories(grug PUBLIC src)

add_executable(test
    test/test.c
)

set_target_properties(test PROPERTIES C_STANDARD 90)
target_compile_options(test PRIVATE -Wall -Wextra -Werror -pedantic-errors -Wconversion -g -fsanitize=address,undefined)
target_link_options(test PRIVATE -fsanitize=address,undefined)
target_link_libraries(test PRIVATE grug)
