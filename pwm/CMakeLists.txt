add_library(mat91lib_pwm INTERFACE)
add_library(mat91lib::pwm ALIAS mat91lib_pwm)
target_sources(mat91lib_pwm INTERFACE
    pwm.c
)
target_include_directories(
    mat91lib_pwm
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
