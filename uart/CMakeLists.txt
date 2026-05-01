add_library(mat91lib_uart INTERFACE)
add_library(mat91lib::uart ALIAS mat91lib_uart)
target_sources(mat91lib_uart INTERFACE
    uart.c
    uart0.c
    uart1.c
)
target_include_directories(
    mat91lib_uart
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
