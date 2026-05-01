add_library(mat91lib_extint INTERFACE)
add_library(mat91lib::extint ALIAS mat91lib_extint)
target_sources(mat91lib_extint INTERFACE
    extint.c
)
target_include_directories(
    mat91lib_extint
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
