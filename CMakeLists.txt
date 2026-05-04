cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(mat91lib LANGUAGES C)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/utils.cmake)

add_subdirectory(sam4s)
if(DEFINED MCU)
    get_mcu_family(${MCU} family)
    set(FAMILY ${family} PARENT_SCOPE)
    message(STATUS "Configuring mat91lib for ${MCU} (family: ${family})")

    string(TOLOWER ${family} family_lower)
    set(family_dir ${CMAKE_CURRENT_SOURCE_DIR}/${family_lower})
    set(MAT91LIB_FAMILY_DIR ${family_dir} PARENT_SCOPE)

    add_library(mat91lib INTERFACE)
    target_include_directories(
        mat91lib INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${family_lower}
    )
    target_compile_definitions(mat91lib INTERFACE __${MCU}__ __${family}__)
    target_link_libraries(mat91lib INTERFACE mat91lib::${family_lower})
    target_link_options(mat91lib INTERFACE -L${family_dir} -T${MCU}-ROM.ld)
    target_compile_options(mat91lib INTERFACE
        # Prevent "ISO C++17 does not allow 'register' storage class specifier"
        # warning from causing error:
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=register>
    )

    # Chip-specific peripheral libraries:
    add_library(mat91lib::pit ALIAS mat91lib_${family_lower}_pit)
    add_library(mat91lib::sysclock ALIAS mat91lib_${family_lower}_sysclock)
    add_library(mat91lib::systick ALIAS mat91lib_${family_lower}_systick)
else()
    message(
        WARNING
        "MCU not defined - mat91lib core libraries won't be available."
    )
endif()

add_subdirectory(ac EXCLUDE_FROM_ALL)
add_subdirectory(adc EXCLUDE_FROM_ALL)
add_subdirectory(cadc EXCLUDE_FROM_ALL)
add_subdirectory(dac EXCLUDE_FROM_ALL)
add_subdirectory(extint EXCLUDE_FROM_ALL)
add_subdirectory(pdc EXCLUDE_FROM_ALL)
add_subdirectory(pwm EXCLUDE_FROM_ALL)
add_subdirectory(spi EXCLUDE_FROM_ALL)
add_subdirectory(ssc EXCLUDE_FROM_ALL)
add_subdirectory(tc EXCLUDE_FROM_ALL)
add_subdirectory(twi EXCLUDE_FROM_ALL)
add_subdirectory(uart EXCLUDE_FROM_ALL)
add_subdirectory(udp EXCLUDE_FROM_ALL)
add_subdirectory(usart EXCLUDE_FROM_ALL)
