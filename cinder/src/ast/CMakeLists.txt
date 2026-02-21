target_sources(cinder_core
    PRIVATE
      types.cpp
      ast_dumper.cpp
)

add_subdirectory(expr)
add_subdirectory(stmt)