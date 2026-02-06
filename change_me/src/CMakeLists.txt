add_library(no_name_lang_core STATIC)
add_executable(main)

# TODO: Fix this so it works even if llvm-config is not found
execute_process(
    COMMAND llvm-config --cxxflags
    OUTPUT_VARIABLE LLVM_CONFIG_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_CONFIG_CXXFLAGS_LIST NATIVE_COMMAND
                   "${LLVM_CONFIG_CXXFLAGS}")
set(LLVM_EXTRA_CXXFLAGS "")
foreach(flag IN LISTS LLVM_CONFIG_CXXFLAGS_LIST)
  if(NOT flag MATCHES "^-std=")
    list(APPEND LLVM_EXTRA_CXXFLAGS "${flag}")
  endif()
endforeach()

execute_process(
    COMMAND llvm-config --ldflags
    OUTPUT_VARIABLE LLVM_CONFIG_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_CONFIG_LDFLAGS_LIST NATIVE_COMMAND
                   "${LLVM_CONFIG_LDFLAGS}")

execute_process(
    COMMAND llvm-config --libs
    OUTPUT_VARIABLE LLVM_CONFIG_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_CONFIG_LIBS_LIST NATIVE_COMMAND
                   "${LLVM_CONFIG_LIBS}")

execute_process(
    COMMAND llvm-config --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_SYSTEM_LIBS_LIST NATIVE_COMMAND
                   "${LLVM_SYSTEM_LIBS}")

target_include_directories(no_name_lang_core
    PUBLIC
      ${NO_NAME_LANG_ROOT_DIR}/include
      ${NO_NAME_LANG_ROOT_DIR}/vendor
    SYSTEM PUBLIC
      ${LLVM_INCLUDE_DIRS}
)

target_compile_options(no_name_lang_core
    PRIVATE
      ${LLVM_EXTRA_CXXFLAGS}
      -g
      -Wall
      -Wextra
      -pedantic
      -Wno-unused-parameter
      -fno-exceptions
)

target_compile_definitions(no_name_lang_core PUBLIC CXXOPTS_NO_EXCEPTIONS)

if(APPLE AND EXISTS "/opt/homebrew/opt/llvm/lib/c++")
  target_link_directories(no_name_lang_core PRIVATE "/opt/homebrew/opt/llvm/lib/c++")
endif()

target_link_options(no_name_lang_core PUBLIC ${LLVM_CONFIG_LDFLAGS_LIST})
target_link_libraries(no_name_lang_core
    PUBLIC
      ${LLVM_CONFIG_LIBS_LIST}
      ${LLVM_SYSTEM_LIBS_LIST}
)

target_compile_options(main
    PRIVATE
      -g
      -Wall
      -Wextra
      -pedantic
      -Wno-unused-parameter
      -fno-exceptions
)

target_compile_definitions(main PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_link_libraries(main PRIVATE no_name_lang_core)

add_subdirectory(ast)
add_subdirectory(codegen)
add_subdirectory(frontend)
add_subdirectory(semantic)
add_subdirectory(support)
add_subdirectory(cli)
