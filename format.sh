#!/bin/bash

git ls-files "*.c" | xargs clang-format -i
git ls-files "*.h" | xargs clang-format -i