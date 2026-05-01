add_library(mat91lib_spi INTERFACE)
add_library(mat91lib::spi ALIAS mat91lib_spi)
target_sources(mat91lib_spi INTERFACE
    spi.c
)
target_include_directories(
    mat91lib_spi
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
