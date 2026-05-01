add_library(mat91lib_usart INTERFACE)
add_library(mat91lib::usart ALIAS mat91lib_usart)
target_sources(mat91lib_usart INTERFACE
    usart.c
    usart0.c
    usart1.c
)
target_include_directories(
    mat91lib_usart
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
