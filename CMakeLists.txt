cmake_policy(PUSH)

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

list(APPEND _SUPPORT_TARGET_PLATFORM 3531a 3531d)
set(BUILD_TARGET_PLATFORM "" CACHE STRING "Specify the build target platform, choice one from supported target platform: ${_SUPPORT_TARGET_PLATFORM}")

if(CMAKE_HOST_UNIX)
    if(BUILD_TARGET_PLATFORM STREQUAL "3531a")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/Toolchain-arm-hisiv400-linux.cmake")
    elseif(BUILD_TARGET_PLATFORM STREQUAL "3531d")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/Toolchain-aarch64-himix200-linux.cmake")
    endif()
endif()


project(example VERSION 0.1.109.1 DESCRIPTION "example pf using spdlog" LANGUAGES C CXX)


option(ENABLE_SPDLOG "Whether to use spdlog" ON)
option(ENABLE_DEBUG "Whether to display the detailed configuration process of cmake" OFF)
if(ENABLE_DEBUG)
  set(FETCHCONTENT_QUIET OFF)
else()
  set(FETCHCONTENT_QUIET ON)
endif()


## set flags for Cross-Compiling
## `CMAKE_CROSSCOMPILING` will be set to `TRUE` if set `CMAKE_TOOLCHAIN_FILE`
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-Compiling for ${CMAKE_SYSTEM_NAME}[${CMAKE_SYSTEM_PROCESSOR}], CMAKE_TOOLCHAIN_FILE:${CMAKE_TOOLCHAIN_FILE}")
    if(BUILD_TARGET_PLATFORM STREQUAL "3531a")
        add_compile_options(-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations)
        add_link_options(-mcpu=cortex-a9)
    elseif(BUILD_TARGET_PLATFORM STREQUAL "3531d")
        add_compile_options(-mcpu=cortex-a53)
        add_link_options(-mcpu=cortex-a53)
    endif()
endif()

## ----------------------------------------------------------- handle dependent libraries ...
message(STATUS "################################# Import dependent libraries... #################################")
## import spdlog
if(ENABLE_SPDLOG)
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  set(__spdlog_version "v1.9.2")
  include(FetchContent)
  # FetchContent_Declare(
  #   spdlog
  #   GIT_REPOSITORY https://github.com/gabime/spdlog.git
  #   GIT_TAG ${__spdlog_version}
  #   GIT_PROGRESS TRUE
  # )
  FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/${__spdlog_version}.tar.gz
    DOWNLOAD_NO_PROGRESS FALSE
    TIMEOUT 120
  )
  FetchContent_GetProperties(spdlog)
  if(NOT spdlog_POPULATED)
    message(STATUS "================================= Import spdlog... =================================")
    FetchContent_MakeAvailable(spdlog)
    if(NOT TARGET spdlog_header_only)
      message(FATAL_ERROR "Failed to import spdlog, target spdlog::spdlog not found!")
    elseif(ENABLE_DEBUG)
      message(STATUS "import_target_name      = spdlog")
      message(STATUS "import_target_version   = ${__spdlog_version}")
      message(STATUS "import_target_dir       = ${spdlog_SOURCE_DIR}")
    endif()
    message(STATUS "================================= Import spdlog...done =================================\n")
  endif()

  ## IDE support for sources classify
  if(MSVC)
    set_target_properties(spdlog PROPERTIES FOLDER "deps")
  endif()
  list(POP_BACK CMAKE_MESSAGE_INDENT)
endif()

message(STATUS "################################# Import dependent libraries...done #################################")
## ----------------------------------------------------------- handle dependent libraries done

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXAMPLE_HEADERS example/logger_wrapper.h example/ZkMSFifo.h)
set(EXAMPLE_SOURCES example/logger_wrapper.cpp example/main.cpp)

source_group("" FILES ${EXAMPLE_HEADERS} ${EXAMPLE_SOURCES})
add_executable(${PROJECT_NAME} ${EXAMPLE_HEADERS} ${EXAMPLE_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
   $<$<BOOL:${ENABLE_SPDLOG}>:spdlog_header_only> ## use head-only library of spdlog when spdlog enable
   $<$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${ENABLE_SPDLOG}>>>:pthread> ## add link flag `-lpthread` under unix when spdlog disabled
)
## add preprocessing macro `ENABLE_SPDLOG` when spdlog enable
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${ENABLE_SPDLOG}>:ENABLE_SPDLOG>)
## avoid excessive warnings about '#pragma GCC target'
target_compile_options(${PROJECT_NAME} PRIVATE $<$<AND:$<BOOL:${UNIX}>,$<BOOL:${ENABLE_SPDLOG}>>:-Wno-pragmas>)

set(VS_STARTUP_PROJECT ${PROJECT_NAME})

cmake_policy(POP)