add_library(cinder_core STATIC)
add_executable(cinder)

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config)

set(LLVM_EXTRA_CXXFLAGS "")
set(LLVM_CONFIG_LDFLAGS_LIST "")
set(LLVM_CONFIG_LIBS_LIST "")
set(LLVM_SYSTEM_LIBS_LIST "")
set(CLANG_LIBS_LIST "")

if(Clang_FOUND)
  if(TARGET clang-cpp)
    list(APPEND CLANG_LIBS_LIST clang-cpp)
  else()
    foreach(clang_component IN ITEMS
        clangDriver
        clangFrontend
        clangSerialization
        clangParse
        clangSema
        clangEdit
        clangAST
        clangLex
        clangBasic)
      if(TARGET ${clang_component})
        list(APPEND CLANG_LIBS_LIST ${clang_component})
      endif()
    endforeach()
  endif()
endif()

if(LLVM_CONFIG_EXECUTABLE)
  execute_process(
      COMMAND "${LLVM_CONFIG_EXECUTABLE}" --cxxflags
      OUTPUT_VARIABLE LLVM_CONFIG_CXXFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  separate_arguments(LLVM_CONFIG_CXXFLAGS_LIST NATIVE_COMMAND
                     "${LLVM_CONFIG_CXXFLAGS}")
  foreach(flag IN LISTS LLVM_CONFIG_CXXFLAGS_LIST)
    if(NOT flag MATCHES "^-std=")
      list(APPEND LLVM_EXTRA_CXXFLAGS "${flag}")
    endif()
  endforeach()

  execute_process(
      COMMAND "${LLVM_CONFIG_EXECUTABLE}" --ldflags
      OUTPUT_VARIABLE LLVM_CONFIG_LDFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  separate_arguments(LLVM_CONFIG_LDFLAGS_LIST NATIVE_COMMAND
                     "${LLVM_CONFIG_LDFLAGS}")

  execute_process(
      COMMAND "${LLVM_CONFIG_EXECUTABLE}" --libs
      OUTPUT_VARIABLE LLVM_CONFIG_LIBS
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  separate_arguments(LLVM_CONFIG_LIBS_LIST NATIVE_COMMAND
                     "${LLVM_CONFIG_LIBS}")

  execute_process(
      COMMAND "${LLVM_CONFIG_EXECUTABLE}" --system-libs
      OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  separate_arguments(LLVM_SYSTEM_LIBS_LIST NATIVE_COMMAND
                     "${LLVM_SYSTEM_LIBS}")
else()
  message(STATUS "llvm-config not found; using LLVM CMake package settings")
  if(DEFINED LLVM_DEFINITIONS)
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND
                       "${LLVM_DEFINITIONS}")
    list(APPEND LLVM_EXTRA_CXXFLAGS ${LLVM_DEFINITIONS_LIST})
  endif()
  if(DEFINED LLVM_LIBRARY_DIRS)
    target_link_directories(cinder_core PRIVATE ${LLVM_LIBRARY_DIRS})
  endif()
  if(TARGET LLVM)
    list(APPEND LLVM_CONFIG_LIBS_LIST LLVM)
  elseif(DEFINED LLVM_LIBRARIES)
    list(APPEND LLVM_CONFIG_LIBS_LIST ${LLVM_LIBRARIES})
  endif()
endif()

target_include_directories(cinder_core
    PUBLIC
      ${CINDER_ROOT_DIR}/include
      ${CINDER_ROOT_DIR}/vendor
    SYSTEM PUBLIC
      ${LLVM_INCLUDE_DIRS}
      ${CLANG_INCLUDE_DIRS}
)

target_compile_options(cinder_core
    PRIVATE
      ${LLVM_EXTRA_CXXFLAGS}
      -g
      -Wall
      -Wextra
      -pedantic
      -Wno-unused-parameter
      -fno-exceptions
)

target_compile_definitions(cinder_core
    PUBLIC
      CXXOPTS_NO_EXCEPTIONS
      $<$<CONFIG:Debug>:DEBUG_BUILD>
)

if(APPLE AND EXISTS "/opt/homebrew/opt/llvm/lib/c++")
  target_link_directories(cinder_core PRIVATE "/opt/homebrew/opt/llvm/lib/c++")
endif()

target_link_options(cinder_core PUBLIC ${LLVM_CONFIG_LDFLAGS_LIST})
target_link_libraries(cinder_core
    PUBLIC
      ${LLVM_CONFIG_LIBS_LIST}
      ${CLANG_LIBS_LIST}
      ${LLVM_SYSTEM_LIBS_LIST}
)

target_compile_options(cinder
    PRIVATE
      -g
      -Wall
      -Wextra
      -pedantic
      -Wno-unused-parameter
      -fno-exceptions
)

target_compile_definitions(cinder
    PRIVATE
      CXXOPTS_NO_EXCEPTIONS
      $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_link_libraries(cinder PRIVATE cinder_core)

add_subdirectory(ast)
add_subdirectory(codegen)
add_subdirectory(frontend)
add_subdirectory(semantic)
add_subdirectory(support)
add_subdirectory(cli)
add_subdirectory(driver)
