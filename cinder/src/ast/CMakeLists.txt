target_sources(cinder_core
    PRIVATE
      types.cpp
)

add_subdirectory(expr)
add_subdirectory(stmt)