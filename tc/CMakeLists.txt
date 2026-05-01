add_library(mat91lib_tc INTERFACE)
add_library(mat91lib::tc ALIAS mat91lib_tc)
target_sources(mat91lib_tc INTERFACE
    tc.c
)
target_include_directories(
    mat91lib_tc
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
