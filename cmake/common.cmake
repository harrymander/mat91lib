include(${CMAKE_CURRENT_LIST_DIR}/utils.cmake)

if(DEFINED MCU)
    get_mcu_family(${MCU} family)
    set(FAMILY ${family} PARENT_SCOPE)
    message(STATUS "Configuring mat91lib for ${MCU} (family: ${family})")

    string(TOLOWER ${family} family_lower)
    set(family_dir ${CMAKE_CURRENT_LIST_DIR}/../${family_lower})

    add_library(mat91lib INTERFACE)
    target_include_directories(
        mat91lib INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${family_lower}
    )
    target_compile_definitions(mat91lib INTERFACE __${MCU}__ __${family}__)
    target_link_libraries(mat91lib INTERFACE mat91lib::${family_lower})
    target_link_options(mat91lib INTERFACE -L${family_dir} -T${MCU}-ROM.ld)

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
