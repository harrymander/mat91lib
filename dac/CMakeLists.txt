add_library(mat91lib_dac INTERFACE)
add_library(mat91lib::dac ALIAS mat91lib_dac)
target_sources(mat91lib_dac INTERFACE
    dac.c
)
target_include_directories(
    mat91lib_dac
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
