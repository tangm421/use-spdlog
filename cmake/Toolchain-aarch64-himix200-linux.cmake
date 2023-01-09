
# crosscompiling settings for aarch64-himix200-linux

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
#set(CMAKE_SYSTEM_PROCESSOR cortex-a53)


# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# set(HOST "aarch64-himix200-linux" CACHE STRING "Similar to the toolchain name of option --host in configure")
# set(CMAKE_C_COMPILER ${HOST}-gcc)
# set(CMAKE_CXX_COMPILER ${HOST}-g++)
# set(CROSS_COMPILE_OPTIONS -fPIC -mcpu=cortex-a53)


set(CMAKE_C_COMPILER "aarch64-himix200-linux-gcc")
set(CMAKE_CXX_COMPILER "aarch64-himix200-linux-g++")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -mcpu=cortex-a53")
