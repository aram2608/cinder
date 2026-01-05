CC := gcc

APP_DIR := ./change_me
INCLUDE_DIR := $(APP_DIR)/include
SRC_DIR := $(APP_DIR)/src

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

BUILD_DIR := ./build
TARGET := main

# Flags
WFLAGS := -Wall -Wextra -pedantic
INCLUDE_FLAGS := -I$(INCLUDE_DIR)

$(BUILD_DIR)/$(TARGET):
	@mkdir -p $(@D)
	$(CC) $(INCLUDE_FLAGS) $(SRC_FILES) -o $@ $(WFLAGS)