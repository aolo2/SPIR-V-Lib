THIS = $(HOME)/Documents/code/study/crs-vlk+

APP_NAME = vlk

DATA_DIR = data
RELEASE_BUILD_PATH = build/release
DEBUG_BUILD_PATH = build/debug

CC = gcc
# INPUT = $(DATA_DIR)/out.spv

DEBUG_CFLAGS = -g -Wall -Wextra -pedantic -Wno-unused-function # -Wno-unused-variable -Wno-unused-parameter # -fsanitize=address -fsanitize=undefined -fsanitize=leak

RELEASE_CFLAGS = -DRELEASE -O2

CFLAGS = $(DEBUG_CFLAGS)
BUILD_PATH = $(DEBUG_BUILD_PATH)

all:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f"[TIME] %E" $(CC) $(CFLAGS) newinst.c -o $(BUILD_PATH)/$(APP_NAME).new
	@rm -f $(BUILD_PATH)/$(APP_NAME)
	@mv $(BUILD_PATH)/$(APP_NAME).new $(BUILD_PATH)/$(APP_NAME)

run:
	@/usr/bin/time -f"[TIME] %E" ./$(BUILD_PATH)/$(APP_NAME)

debug:
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BUILD_PATH)/$(APP_NAME)