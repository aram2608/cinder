include(FetchContent)

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(cinder_unit_tests
  parser_qualified_types_test.cpp
  semantic_qualified_types_test.cpp
  lexer_test.cpp
)

target_link_libraries(cinder_unit_tests
  PRIVATE
    cinder_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(cinder_unit_tests)
