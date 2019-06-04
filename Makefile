APP_NAME = vlk

MODE = Debug
CC = gcc

CFLAGS = -g -Wall -Wextra -pedantic -Wno-unused-function # -Wno-unused-variable -Wno-unused-parameter -fsanitize=address -fsanitize=undefined -fsanitize=leak
BUILD_PATH = build/debug

ifeq ($(MODE), Release)
	CFLAGS = -O2
	BUILD_PATH = build/release
endif

all:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f "[TIME] %E" $(CC) $(CFLAGS) main.c -o $(BUILD_PATH)/$(APP_NAME).new
	@rm -f $(BUILD_PATH)/$(APP_NAME)
	@mv $(BUILD_PATH)/$(APP_NAME).new $(BUILD_PATH)/$(APP_NAME)

run:
	@./$(BUILD_PATH)/$(APP_NAME)