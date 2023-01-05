cmake_policy(PUSH)

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(example VERSION 0.1.1226.1 DESCRIPTION "example pf using spdlog" LANGUAGES C CXX)

option(ENABLE_SPDLOG "Whether to use spdlog" ON)
option(ENABLE_DEBUG "Whether to display the detailed configuration process of cmake" OFF)
if(ENABLE_DEBUG)
  set(FETCHCONTENT_QUIET OFF)
else()
  set(FETCHCONTENT_QUIET ON)
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

set(EXAMPLE_HEADERS example/logger_wrapper.h)
set(EXAMPLE_SOURCES example/logger_wrapper.cpp example/main.cpp)

source_group("" FILES ${EXAMPLE_HEADERS} ${EXAMPLE_SOURCES})
add_executable(${PROJECT_NAME} ${EXAMPLE_HEADERS} ${EXAMPLE_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE $<$<BOOL:${ENABLE_SPDLOG}>:spdlog_header_only>)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${ENABLE_SPDLOG}>:ENABLE_SPDLOG>)
if(ENABLE_SPDLOG)
## avoid excessive warnings about '#pragma GCC target'
target_compile_options(${PROJECT_NAME} PRIVATE $<$<AND:$<BOOL:${COMPILER_IS_GNU}>,$<BOOL:${ENABLE_SPDLOG}>>:-Wno-pragmas>)
endif()

set(VS_STARTUP_PROJECT ${PROJECT_NAME})

cmake_policy(POP)