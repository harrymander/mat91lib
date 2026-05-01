add_library(mat91lib_cadc INTERFACE)
add_library(mat91lib::cadc ALIAS mat91lib_cadc)
target_sources(mat91lib_cadc INTERFACE
    cadc.c
)
target_include_directories(
    mat91lib_cadc
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(mat91lib_cadc INTERFACE
    mat91lib::adc
    mat91lib::pdc
    mat91lib::tc
)
