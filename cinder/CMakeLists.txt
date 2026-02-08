cmake_minimum_required(VERSION 3.21)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(cinder LANGUAGES C CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CINDER_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang CONFIG QUIET)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

if(Clang_FOUND)
  message(STATUS "Found Clang package in: ${Clang_DIR}")
else()
  message(STATUS "Clang package not found; clang driver support disabled")
endif()

add_subdirectory(src)
