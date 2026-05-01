add_library(mat91lib_twi INTERFACE)
add_library(mat91lib::twi ALIAS mat91lib_twi)
target_sources(mat91lib_twi INTERFACE
    twi.c
)
target_include_directories(
    mat91lib_twi
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
