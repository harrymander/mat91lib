set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(triple arm-none-eabi)

find_program(CMAKE_C_COMPILER ${triple}-gcc REQUIRED)
find_program(CMAKE_ASM_COMPILER ${triple}-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER ${triple}-g++ REQUIRED)
find_program(CMAKE_AR ${triple}-ar REQUIRED)
find_program(CMAKE_OBJCOPY ${triple}-objcopy REQUIRED)
find_program(CMAKE_OBJDUMP ${triple}-objdump REQUIRED)

set(
    common_flags
    "-mcpu=cortex-m4 \
    -mthumb \
    -specs=nano.specs \
    -specs=nosys.specs \
    -DARM_MATH_CM4=true \
    -mno-unaligned-access"
)

set(
    CMAKE_C_FLAGS_INIT
    "${common_flags} -Wstrict-prototypes"
    CACHE INTERNAL "Initial C compiler flags"
)
set(
    CMAKE_CXX_FLAGS_INIT
    "${common_flags}"
    CACHE INTERNAL "Initial C++ compiler flags"
)
