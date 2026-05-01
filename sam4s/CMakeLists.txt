# Chip-specific support for SAM4S family

add_library(mat91lib_sam4s INTERFACE)
add_library(mat91lib::sam4s ALIAS mat91lib_sam4s)
target_sources(mat91lib_sam4s INTERFACE
    ../syscalls.c
    crt0.c
    mcu.c
    mcu_sleep.c
)
target_include_directories(
    mat91lib_sam4s INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/atmel
)

macro(add_sam4s_periph_lib peripheral)
    set(library_name mat91lib_sam4s_${peripheral})
    add_library(${library_name} INTERFACE)
    add_library(mat91lib::sam4s::${peripheral} ALIAS ${library_name})
    target_sources(${library_name} INTERFACE ${peripheral}/${peripheral}.c)
    target_include_directories(
        ${library_name}
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${peripheral}
    )
    target_link_libraries(${library_name} INTERFACE mat91lib::sam4s)
endmacro()

add_sam4s_periph_lib(pit)
add_sam4s_periph_lib(sysclock)
add_sam4s_periph_lib(systick)
