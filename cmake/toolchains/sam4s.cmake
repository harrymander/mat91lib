set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc REQUIRED)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++ REQUIRED)
find_program(CMAKE_AS arm-none-eabi-as REQUIRED)
find_program(CMAKE_AR arm-none-eabi-ar REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(CMAKE_OBJDUMP arm-none-eabi-objdump REQUIRED)
find_program(CMAKE_SIZE arm-none-eabi-size REQUIRED)

set(
    CMAKE_C_FLAGS
    "-mcpu=cortex-m4 \
    -mthumb \
    -specs=nano.specs \
    -specs=nosys.specs \
    -DARM_MATH_CM4=true \
    -Wstrict-prototypes \
    -mno-unaligned-access"
    CACHE INTERNAL "C compiler flags"
)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE INTERNAL "C++ compiler flags")
