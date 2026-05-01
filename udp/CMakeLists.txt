add_library(mat91lib_udp INTERFACE)
add_library(mat91lib::udp ALIAS mat91lib_udp)
target_sources(mat91lib_udp INTERFACE
    udp.c
)
target_include_directories(
    mat91lib_udp
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
