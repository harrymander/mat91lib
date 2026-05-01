add_library(mat91lib_ac INTERFACE)
add_library(mat91lib::ac ALIAS mat91lib_ac)
target_sources(mat91lib_ac INTERFACE
    ac.c
)
target_include_directories(
    mat91lib_ac
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
