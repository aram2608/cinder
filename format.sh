#!/bin/bash

git ls-files "*.cpp" | xargs clang-format -i
git ls-files "*.hpp" | xargs clang-format -i