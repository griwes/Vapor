cmake_minimum_required(VERSION 3.5)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "Don't configure this project in-source!")
endif()

project(Vapor C CXX)

find_package(Threads REQUIRED)

# Boost 1.70 breaks the FindBoost world rather spectacularly
# do NOT delegate to boost-specific cmake config files
# and force the old-and-proven FindBoost behavior
set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost 1.64 REQUIRED
    COMPONENTS system program_options filesystem iostreams
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

include(FindPkgConfig)

pkg_check_modules(SODIUM REQUIRED libsodium>=1.0)
include_directories(${SODIUM_INCLUDEDIR})
link_directories(${SODIUM_LIBRARY_DIRS})

find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM tools dir: ${LLVM_TOOLS_BINARY_DIR}")

find_program(LLC llc ${LLVM_TOOLS_BINARY_DIR})
if(NOT LLC)
    message(FATAL_ERROR "Couldn't find llc!")
endif()

find_program(OPT opt ${LLVM_TOOLS_BINARY_DIR})
if(NOT OPT)
    message(FATAL_ERROR "Couldn't find opt!")
endif()

message(STATUS "LLC path: ${LLC}")
message(STATUS "OPT path: ${OPT}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")

add_subdirectory(vendor EXCLUDE_FROM_ALL)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wpedantic -Weffc++ -Werror -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ferror-limit=1 -ftemplate-backtrace-limit=0 -fno-limit-debug-info")
endif()

include_directories(include/reaver)
include_directories(vendor/protobuf/src SYSTEM)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(runtime)
add_subdirectory(tests EXCLUDE_FROM_ALL)

