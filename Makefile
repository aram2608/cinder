CXX := clang++

APP_DIR := ./change_me
INCLUDE_DIR := $(APP_DIR)/include
SRC_DIR := $(APP_DIR)/src

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)

BUILD_DIR := ./build
TARGET := main

# Flags
# optimization levels
# -01, -02, -03
WFLAGS := -g -Wall -Wextra -pedantic -Wno-unused-parameter
INCLUDE_FLAGS := -I$(INCLUDE_DIR)
LDFLAGS := -L/opt/homebrew/opt/llvm/lib/c++ \
	$(shell llvm-config --cxxflags --ldflags --libs) \

OPTS := -DCXXOPTS_NO_EXCEPTIONS

$(BUILD_DIR)/$(TARGET):
	@mkdir -p $(@D)
	$(CXX) $(INCLUDE_FLAGS) $(WFLAGS) $(LDFLAGS) $(OPTS) -o $@ $(SRC_FILES)

LLVM:
	./build/main --emit-llvm -o test.ll test.changeme
TEST:
	./build/main --compile -o test test.changeme