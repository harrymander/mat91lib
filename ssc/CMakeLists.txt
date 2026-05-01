add_library(mat91lib_ssc INTERFACE)
add_library(mat91lib::ssc ALIAS mat91lib_ssc)
target_sources(mat91lib_ssc INTERFACE
    ssc.c
    ssc_dma.c
)
target_include_directories(
    mat91lib_ssc
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
