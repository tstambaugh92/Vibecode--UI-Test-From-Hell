APP_NAME := vibecode_ui_test_from_hell

SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
BUILD_DIR := .build
LINUX_BUILD_DIR := $(BUILD_DIR)/linux
WIN_BUILD_DIR := $(BUILD_DIR)/win
WIN_PKG_DIR := $(BIN_DIR)/windows-package

SRCS := $(wildcard $(SRC_DIR)/*.c)
LINUX_OBJS := $(patsubst $(SRC_DIR)/%.c,$(LINUX_BUILD_DIR)/%.o,$(SRCS))
WIN_OBJS := $(patsubst $(SRC_DIR)/%.c,$(WIN_BUILD_DIR)/%.o,$(SRCS))

LINUX_CC := gcc
LINUX_PKG_CONFIG ?= pkg-config
LINUX_SDL_CFLAGS := $(shell $(LINUX_PKG_CONFIG) --cflags sdl3 2>/dev/null)
LINUX_SDL_LIBS := $(shell $(LINUX_PKG_CONFIG) --libs sdl3 2>/dev/null)
LINUX_CFLAGS := -std=c11 -O2 -Wall -Wextra -I$(INC_DIR) $(LINUX_SDL_CFLAGS)
LINUX_LDFLAGS := -lm $(LINUX_SDL_LIBS)
LINUX_TARGET := $(BIN_DIR)/$(APP_NAME)

WIN_CC ?= x86_64-w64-mingw32-gcc
WIN_PKG_CONFIG ?= x86_64-w64-mingw32-pkg-config
WIN_SDL_CFLAGS := $(shell command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 && $(WIN_PKG_CONFIG) --cflags sdl3 2>/dev/null)
WIN_SDL_LIBS := $(shell command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 && $(WIN_PKG_CONFIG) --libs sdl3 2>/dev/null)
WIN_CFLAGS := -std=c11 -O2 -Wall -Wextra -I$(INC_DIR) $(WIN_SDL_CFLAGS)
WIN_LDFLAGS := -lm $(WIN_SDL_LIBS)
WIN_TARGET := $(BIN_DIR)/$(APP_NAME).exe

# Provide one of these when running `make win-package`
# 1) SDL3_DLL=/path/to/SDL3.dll
# 2) SDL3_DLL_DIR=/path/to/folder/with/SDL3.dll
SDL3_DLL ?=
SDL3_DLL_DIR ?=

ifeq ($(OS),Windows_NT)
RM_RF = if exist "$(1)" rmdir /s /q "$(1)" & if exist "$(1)" del /f /q "$(1)"
else
RM_RF = rm -rf $(1)
endif

.PHONY: all linux win run run-win clean win-package deps deps-linux deps-win

all: linux

linux: $(LINUX_TARGET)

win: $(WIN_TARGET)

$(LINUX_TARGET): $(LINUX_OBJS) | $(BIN_DIR)
	$(LINUX_CC) $(LINUX_OBJS) -o $@ $(LINUX_LDFLAGS)

$(WIN_TARGET): $(WIN_OBJS) | $(BIN_DIR)
	$(WIN_CC) $(WIN_OBJS) -o $@ $(WIN_LDFLAGS)

$(LINUX_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(LINUX_BUILD_DIR)
	$(LINUX_CC) $(LINUX_CFLAGS) -c $< -o $@

$(WIN_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(WIN_BUILD_DIR)
	$(WIN_CC) $(WIN_CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(LINUX_BUILD_DIR):
	mkdir -p $(LINUX_BUILD_DIR)

$(WIN_BUILD_DIR):
	mkdir -p $(WIN_BUILD_DIR)

run: linux
	./$(LINUX_TARGET)

run-win: win
	@echo "Built $(WIN_TARGET)"

win-package: win
	rm -rf $(WIN_PKG_DIR)
	mkdir -p $(WIN_PKG_DIR)
	cp $(WIN_TARGET) $(WIN_PKG_DIR)/
	@if [ -n "$(SDL3_DLL)" ] && [ -f "$(SDL3_DLL)" ]; then \
		cp "$(SDL3_DLL)" $(WIN_PKG_DIR)/SDL3.dll; \
	elif [ -n "$(SDL3_DLL_DIR)" ] && [ -f "$(SDL3_DLL_DIR)/SDL3.dll" ]; then \
		cp "$(SDL3_DLL_DIR)/SDL3.dll" $(WIN_PKG_DIR)/SDL3.dll; \
	else \
		echo "No SDL3.dll copied. Set SDL3_DLL=... or SDL3_DLL_DIR=... when running make win-package."; \
	fi
	@echo "Windows package directory ready at $(WIN_PKG_DIR)"

deps: deps-linux
	@echo "Tip: run 'make deps-win' if you plan to cross-build Windows binaries."

deps-linux:
	@echo "Checking Linux build dependencies..."
	@command -v $(LINUX_CC) >/dev/null 2>&1 || { echo "Missing: $(LINUX_CC)"; exit 1; }
	@command -v $(LINUX_PKG_CONFIG) >/dev/null 2>&1 || { echo "Missing: $(LINUX_PKG_CONFIG)"; exit 1; }
	@$(LINUX_PKG_CONFIG) --exists sdl3 || { echo "Missing pkg-config entry: sdl3"; exit 1; }
	@echo "OK: Linux deps found ($(LINUX_CC), $(LINUX_PKG_CONFIG), sdl3)."

deps-win:
	@echo "Checking Windows cross-build dependencies..."
	@command -v $(WIN_CC) >/dev/null 2>&1 || { echo "Missing: $(WIN_CC)"; exit 1; }
	@command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 || { echo "Missing: $(WIN_PKG_CONFIG)"; exit 1; }
	@$(WIN_PKG_CONFIG) --exists sdl3 || { echo "Missing pkg-config entry for cross target: sdl3"; exit 1; }
	@echo "OK: Windows cross deps found ($(WIN_CC), $(WIN_PKG_CONFIG), sdl3)."

clean:
	$(call RM_RF,$(BUILD_DIR))
	$(call RM_RF,$(LINUX_TARGET))
	$(call RM_RF,$(WIN_TARGET))
	$(call RM_RF,$(WIN_PKG_DIR))
