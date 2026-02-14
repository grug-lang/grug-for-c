cmake_minimum_required(VERSION 3.28)

project(grug
    VERSION 1.0.0
    DESCRIPTION "The perfect modding language"
    HOMEPAGE_URL "https://github.com/grug-lang/grug-for-c"
    LANGUAGES C
)

# removed pedantic errors because it doesn't allow unnamed structs and unions
set(GRUG_COMPILE_OPTIONS "-Wall" "-Wextra" "-Werror" "-Wconversion" "-g" "-fsanitize=address,undefined")
set(GRUG_LINK_OPTIONS "-fsanitize=address,undefined")

add_library(grug src/grug.c)

set_target_properties(grug PROPERTIES C_STANDARD 99)
target_compile_options(grug PRIVATE ${GRUG_COMPILE_OPTIONS})
target_link_options(grug PRIVATE ${GRUG_LINK_OPTIONS})
target_include_directories(grug PUBLIC src)

add_executable(test
    test/test.c
)

set_target_properties(test PROPERTIES C_STANDARD 99)
target_compile_options(test PRIVATE ${GRUG_COMPILE_OPTIONS})
target_link_options(test PRIVATE ${GRUG_LINK_OPTIONS})
target_link_libraries(test PRIVATE grug)
