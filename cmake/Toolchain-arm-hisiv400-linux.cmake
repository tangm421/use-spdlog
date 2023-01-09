
# crosscompiling settings for arm-hisiv400-linux

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
#set(CMAKE_SYSTEM_PROCESSOR cortex-a9)


# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# set(HOST "arm-hisiv400-linux" CACHE STRING "Similar to the toolchain name of option --host in configure")
# set(CMAKE_C_COMPILER ${HOST}-gcc)
# set(CMAKE_CXX_COMPILER ${HOST}-g++)
# set(CROSS_COMPILE_OPTIONS -fPIC -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations)


set(CMAKE_C_COMPILER "arm-hisiv400-linux-gcc")
set(CMAKE_CXX_COMPILER "arm-hisiv400-linux-g++")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations")
