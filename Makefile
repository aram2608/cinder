CXX := clang++

APP_DIR := ./change_me
INCLUDE_DIR := $(APP_DIR)/include
SRC_DIR := $(APP_DIR)/src

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)

BUILD_DIR := ./build
TARGET := main

# Flags
WFLAGS := -g -Wall -Wextra -pedantic
INCLUDE_FLAGS := -I$(INCLUDE_DIR)
LDFLAGS := -L/opt/homebrew/opt/llvm/lib/c++ -L/opt/homebrew/opt/llvm/lib/unwind -lunwind

$(BUILD_DIR)/$(TARGET):
	@mkdir -p $(@D)
	$(CXX) $(INCLUDE_FLAGS) $(SRC_FILES) $(LDFLAGS) -o $@ $(WFLAGS)