#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>

/// @brief Simple utility macro to error out at unreachable points in source
#define UNREACHABLE(x, y)                                     \
  do {                                                        \
    std::cout << "UNREACHABLE " << #x << " " << #y << "\n";   \
    std::cout << "at" << __FILE__ << " " << __LINE__ << "\n"; \
    exit(1);                                                  \
  } while (0)

/// @brief Simple utility macro to error out for unimplemented components
#define IMPLEMENT(x)                              \
  do {                                            \
    std::cout << "Implement me.\n";               \
    std::cout << "Feature: " << #x << "\n";       \
    std::cout << "In: " << __FILE__ << "\n";      \
    std::cout << "At line: " << __LINE__ << "\n"; \
    exit(1);                                      \
  } while (0);

#define UNUSED(x) (void)(x)

#endif
