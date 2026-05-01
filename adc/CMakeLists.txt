add_library(mat91lib_adc INTERFACE)
add_library(mat91lib::adc ALIAS mat91lib_adc)
target_sources(mat91lib_adc INTERFACE
    adc.c
)
target_include_directories(
    mat91lib_adc
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
