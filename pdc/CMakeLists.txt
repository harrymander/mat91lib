add_library(mat91lib_pdc INTERFACE)
add_library(mat91lib::pdc ALIAS mat91lib_pdc)
target_sources(mat91lib_pdc INTERFACE
    pdc.c
)
target_include_directories(
    mat91lib_pdc
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
