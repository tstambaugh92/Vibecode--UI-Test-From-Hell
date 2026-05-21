APP_NAME := vibecode_ui_test_from_hell

SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
BUILD_DIR := .build
LINUX_BUILD_DIR := $(BUILD_DIR)/linux
WIN_BUILD_DIR := $(BUILD_DIR)/win
WIN_PKG_DIR := $(BIN_DIR)/windows-package
APPIMAGE_BUILD_DIR := $(BUILD_DIR)/appimage
APPDIR := $(APPIMAGE_BUILD_DIR)/AppDir
APP_ID := com.christhawk.vibecode_ui_test_from_hell
APP_DATA_DIR := usr/share/$(APP_NAME)
APPIMAGE_DESKTOP := $(APPDIR)/$(APP_ID).desktop
APPIMAGE_ICON := $(APPDIR)/$(APP_ID).svg
APPIMAGE_ARCH := $(shell uname -m)
APPIMAGE_TARGET := $(BIN_DIR)/$(APP_NAME)-$(APPIMAGE_ARCH).AppImage
APPIMAGE_TOOL_DIR := tools/appimage
LINUXDEPLOY_APPIMAGE := $(APPIMAGE_TOOL_DIR)/linuxdeploy-$(APPIMAGE_ARCH).AppImage
APPIMAGETOOL_APPIMAGE := $(APPIMAGE_TOOL_DIR)/appimagetool-$(APPIMAGE_ARCH).AppImage
APPIMAGE_RUNTIME := $(APPIMAGE_TOOL_DIR)/runtime-$(APPIMAGE_ARCH)
LINUXDEPLOY_URL ?= https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$(APPIMAGE_ARCH).AppImage
APPIMAGETOOL_URL ?= https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$(APPIMAGE_ARCH).AppImage
APPIMAGE_RUNTIME_URL ?= https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-$(APPIMAGE_ARCH)
SDL3_LICENSE ?= /usr/share/licenses/sdl3/LICENSE

SRCS := $(wildcard $(SRC_DIR)/*.c)
LINUX_OBJS := $(patsubst $(SRC_DIR)/%.c,$(LINUX_BUILD_DIR)/%.o,$(SRCS))
WIN_OBJS := $(patsubst $(SRC_DIR)/%.c,$(WIN_BUILD_DIR)/%.o,$(SRCS))

LINUX_CC := gcc
LINUX_PKG_CONFIG ?= pkg-config
LINUX_SDL_CFLAGS := $(shell $(LINUX_PKG_CONFIG) --cflags sdl3 2>/dev/null)
LINUX_SDL_LIBS := $(shell $(LINUX_PKG_CONFIG) --libs sdl3 2>/dev/null)
LINUX_FREETYPE_CFLAGS := $(shell $(LINUX_PKG_CONFIG) --exists freetype2 2>/dev/null && $(LINUX_PKG_CONFIG) --cflags freetype2 2>/dev/null)
LINUX_FREETYPE_LIBS := $(shell $(LINUX_PKG_CONFIG) --exists freetype2 2>/dev/null && $(LINUX_PKG_CONFIG) --libs freetype2 2>/dev/null)
LINUX_FREETYPE_DEFINE := $(shell $(LINUX_PKG_CONFIG) --exists freetype2 2>/dev/null && echo -DHAVE_FREETYPE=1)
LINUX_CFLAGS := -std=c11 -O2 -Wall -Wextra -I$(INC_DIR) $(LINUX_FREETYPE_DEFINE) $(LINUX_SDL_CFLAGS) $(LINUX_FREETYPE_CFLAGS)
LINUX_LDFLAGS := -lm $(LINUX_SDL_LIBS) $(LINUX_FREETYPE_LIBS)
LINUX_TARGET := $(BIN_DIR)/$(APP_NAME)

WIN_CC ?= x86_64-w64-mingw32-gcc
WIN_PKG_CONFIG ?= x86_64-w64-mingw32-pkg-config
WIN_SDL_CFLAGS := $(shell command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 && $(WIN_PKG_CONFIG) --cflags sdl3 2>/dev/null)
WIN_SDL_LIBS := $(shell command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 && $(WIN_PKG_CONFIG) --libs sdl3 2>/dev/null)
WIN_FREETYPE_CFLAGS := $(shell command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 && $(WIN_PKG_CONFIG) --exists freetype2 2>/dev/null && $(WIN_PKG_CONFIG) --cflags freetype2 2>/dev/null)
WIN_FREETYPE_LIBS := $(shell command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 && $(WIN_PKG_CONFIG) --exists freetype2 2>/dev/null && $(WIN_PKG_CONFIG) --libs freetype2 2>/dev/null)
WIN_FREETYPE_DEFINE := $(shell command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 && $(WIN_PKG_CONFIG) --exists freetype2 2>/dev/null && echo -DHAVE_FREETYPE=1)
WIN_CFLAGS := -std=c11 -O2 -Wall -Wextra -I$(INC_DIR) $(WIN_FREETYPE_DEFINE) $(WIN_SDL_CFLAGS) $(WIN_FREETYPE_CFLAGS)
WIN_LDFLAGS := -lm $(WIN_SDL_LIBS) $(WIN_FREETYPE_LIBS)
WIN_TARGET := $(BIN_DIR)/$(APP_NAME).exe
LINUXDEPLOY ?= $(APPIMAGE_TOOL_DIR)/linuxdeploy.AppDir/AppRun
APPIMAGETOOL ?= $(APPIMAGE_TOOL_DIR)/appimagetool.AppDir/AppRun

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

.PHONY: all linux win run run-win clean win-package appdir appimage appimage-tools deps deps-linux deps-win deps-appimage

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

$(APPIMAGE_TOOL_DIR):
	mkdir -p $(APPIMAGE_TOOL_DIR)

$(LINUXDEPLOY_APPIMAGE): | $(APPIMAGE_TOOL_DIR)
	curl --fail --location --output $@ $(LINUXDEPLOY_URL)
	chmod +x $@

$(APPIMAGETOOL_APPIMAGE): | $(APPIMAGE_TOOL_DIR)
	curl --fail --location --output $@ $(APPIMAGETOOL_URL)
	chmod +x $@

$(APPIMAGE_RUNTIME): | $(APPIMAGE_TOOL_DIR)
	curl --fail --location --output $@ $(APPIMAGE_RUNTIME_URL)
	chmod +x $@

$(APPIMAGE_TOOL_DIR)/linuxdeploy.AppDir/AppRun: $(LINUXDEPLOY_APPIMAGE)
	rm -rf $(APPIMAGE_TOOL_DIR)/linuxdeploy.AppDir
	cd $(APPIMAGE_TOOL_DIR) && ./linuxdeploy-$(APPIMAGE_ARCH).AppImage --appimage-extract >/dev/null
	mv $(APPIMAGE_TOOL_DIR)/squashfs-root $(APPIMAGE_TOOL_DIR)/linuxdeploy.AppDir
	@if command -v strip >/dev/null 2>&1; then ln -sf "$$(command -v strip)" $(APPIMAGE_TOOL_DIR)/linuxdeploy.AppDir/usr/bin/strip; fi
	touch $@

$(APPIMAGE_TOOL_DIR)/appimagetool.AppDir/AppRun: $(APPIMAGETOOL_APPIMAGE)
	rm -rf $(APPIMAGE_TOOL_DIR)/appimagetool.AppDir
	cd $(APPIMAGE_TOOL_DIR) && ./appimagetool-$(APPIMAGE_ARCH).AppImage --appimage-extract >/dev/null
	mv $(APPIMAGE_TOOL_DIR)/squashfs-root $(APPIMAGE_TOOL_DIR)/appimagetool.AppDir
	touch $@

run: linux
	./$(LINUX_TARGET)

run-win: win
	@echo "Built $(WIN_TARGET)"

win-package: win
	rm -rf $(WIN_PKG_DIR)
	mkdir -p $(WIN_PKG_DIR)
	cp $(WIN_TARGET) $(WIN_PKG_DIR)/
	@if [ -d audio ]; then mkdir -p $(WIN_PKG_DIR)/audio; cp -R audio/. $(WIN_PKG_DIR)/audio/; fi
	@if [ -d images ]; then mkdir -p $(WIN_PKG_DIR)/images; cp -R images/. $(WIN_PKG_DIR)/images/; fi
	@if [ -d font ]; then mkdir -p $(WIN_PKG_DIR)/font; cp -R font/. $(WIN_PKG_DIR)/font/; fi
	@if [ -n "$(SDL3_DLL)" ] && [ -f "$(SDL3_DLL)" ]; then \
		cp "$(SDL3_DLL)" $(WIN_PKG_DIR)/SDL3.dll; \
	elif [ -n "$(SDL3_DLL_DIR)" ] && [ -f "$(SDL3_DLL_DIR)/SDL3.dll" ]; then \
		cp "$(SDL3_DLL_DIR)/SDL3.dll" $(WIN_PKG_DIR)/SDL3.dll; \
	else \
		echo "No SDL3.dll copied. Set SDL3_DLL=... or SDL3_DLL_DIR=... when running make win-package."; \
	fi
	@echo "Windows package directory ready at $(WIN_PKG_DIR)"

appdir: linux
	rm -rf $(APPDIR)
	mkdir -p $(APPDIR)/usr/bin
	mkdir -p $(APPDIR)/usr/share/applications
	mkdir -p $(APPDIR)/usr/share/icons/hicolor/scalable/apps
	mkdir -p $(APPDIR)/usr/share/licenses/$(APP_ID)
	mkdir -p $(APPDIR)/$(APP_DATA_DIR)
	cp $(LINUX_TARGET) $(APPDIR)/usr/bin/$(APP_NAME)
	printf '%s\n' '#!/bin/sh' 'HERE="$$(dirname "$$(readlink -f "$$0")")"' '' 'export LD_LIBRARY_PATH="$$HERE/usr/lib:$$HERE/usr/lib64:$$LD_LIBRARY_PATH"' 'export VIBECODE_DATA_DIR="$$HERE/$(APP_DATA_DIR)"' '' 'exec "$$HERE/usr/bin/$(APP_NAME)" "$$@"' > $(APPDIR)/AppRun
	printf '%s\n' '[Desktop Entry]' 'Type=Application' 'Name=Vibecode UI Test From Hell' 'Comment=SDL3 sorting visualizer and secret snake toy' 'Exec=$(APP_NAME)' 'Icon=$(APP_ID)' 'Terminal=false' 'Categories=Game;' > $(APPIMAGE_DESKTOP)
	printf '%s\n' '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128">' '<rect width="128" height="128" rx="18" fill="#09101f"/>' '<rect x="18" y="26" width="10" height="76" fill="#009b48"/>' '<rect x="34" y="46" width="10" height="56" fill="#c41e3a"/>' '<rect x="50" y="18" width="10" height="84" fill="#0046ad"/>' '<rect x="66" y="62" width="10" height="40" fill="#ffcf00"/>' '<rect x="82" y="36" width="10" height="66" fill="#ff5800"/>' '<path d="M23 109h82" stroke="#edf2ff" stroke-width="6" stroke-linecap="round"/>' '<circle cx="101" cy="30" r="7" fill="#78f58c"/>' '</svg>' > $(APPIMAGE_ICON)
	cp $(APPIMAGE_DESKTOP) $(APPDIR)/usr/share/applications/$(APP_ID).desktop
	cp $(APPIMAGE_ICON) $(APPDIR)/usr/share/icons/hicolor/scalable/apps/$(APP_ID).svg
	@if [ -d audio ]; then mkdir -p $(APPDIR)/$(APP_DATA_DIR)/audio; cp -R audio/. $(APPDIR)/$(APP_DATA_DIR)/audio/; fi
	@if [ -d images ]; then mkdir -p $(APPDIR)/$(APP_DATA_DIR)/images; cp -R images/. $(APPDIR)/$(APP_DATA_DIR)/images/; fi
	@if [ -d font ]; then mkdir -p $(APPDIR)/$(APP_DATA_DIR)/font; cp -R font/. $(APPDIR)/$(APP_DATA_DIR)/font/; fi
	@if [ -f "$(SDL3_LICENSE)" ]; then cp "$(SDL3_LICENSE)" $(APPDIR)/usr/share/licenses/$(APP_ID)/SDL3-LICENSE.txt; else echo "Warning: SDL3 license not found at $(SDL3_LICENSE)"; fi
	chmod +x $(APPDIR)/AppRun $(APPDIR)/usr/bin/$(APP_NAME)
	@echo "AppDir staged at $(APPDIR)"

appimage: appimage-tools deps-appimage appdir | $(BIN_DIR)
	$(LINUXDEPLOY) --appdir $(APPDIR) --executable $(APPDIR)/usr/bin/$(APP_NAME) --desktop-file $(APPIMAGE_DESKTOP) --icon-file $(APPIMAGE_ICON)
	ARCH=$(APPIMAGE_ARCH) $(APPIMAGETOOL) --runtime-file $(APPIMAGE_RUNTIME) $(APPDIR) $(APPIMAGE_TARGET)
	chmod +x $(APPIMAGE_TARGET)
	@echo "AppImage ready at $(APPIMAGE_TARGET)"

appimage-tools: $(LINUXDEPLOY) $(APPIMAGETOOL) $(APPIMAGE_RUNTIME)
	@echo "AppImage tools downloaded to $(APPIMAGE_TOOL_DIR)"

deps: deps-linux
	@echo "Tip: run 'make deps-win' if you plan to cross-build Windows binaries."
	@echo "     or 'make appimage-tools' before making an AppImage."

deps-linux:
	@echo "Checking Linux build dependencies..."
	@command -v $(LINUX_CC) >/dev/null 2>&1 || { echo "Missing: $(LINUX_CC)"; exit 1; }
	@command -v $(LINUX_PKG_CONFIG) >/dev/null 2>&1 || { echo "Missing: $(LINUX_PKG_CONFIG)"; exit 1; }
	@$(LINUX_PKG_CONFIG) --exists sdl3 || { echo "Missing pkg-config entry: sdl3"; exit 1; }
	@$(LINUX_PKG_CONFIG) --exists freetype2 && echo "OK: freetype2 found; Daniel Bold error text enabled." || echo "Optional: freetype2 missing; error text falls back to SDL debug text."
	@echo "OK: Linux deps found ($(LINUX_CC), $(LINUX_PKG_CONFIG), sdl3)."

deps-win:
	@echo "Checking Windows cross-build dependencies..."
	@command -v $(WIN_CC) >/dev/null 2>&1 || { echo "Missing: $(WIN_CC)"; exit 1; }
	@command -v $(WIN_PKG_CONFIG) >/dev/null 2>&1 || { echo "Missing: $(WIN_PKG_CONFIG)"; exit 1; }
	@$(WIN_PKG_CONFIG) --exists sdl3 || { echo "Missing pkg-config entry for cross target: sdl3"; exit 1; }
	@$(WIN_PKG_CONFIG) --exists freetype2 && echo "OK: cross freetype2 found; Daniel Bold error text enabled." || echo "Optional: cross freetype2 missing; error text falls back to SDL debug text."
	@echo "OK: Windows cross deps found ($(WIN_CC), $(WIN_PKG_CONFIG), sdl3)."

deps-appimage:
	@echo "Checking AppImage packaging tools..."
	@{ [ -x "$(LINUXDEPLOY)" ] || command -v "$(LINUXDEPLOY)" >/dev/null 2>&1; } || { echo "Missing: $(LINUXDEPLOY)"; echo "Run 'make appimage-tools' to download linuxdeploy and appimagetool."; exit 1; }
	@{ [ -x "$(APPIMAGETOOL)" ] || command -v "$(APPIMAGETOOL)" >/dev/null 2>&1; } || { echo "Missing: $(APPIMAGETOOL)"; echo "Run 'make appimage-tools' to download linuxdeploy and appimagetool."; exit 1; }
	@[ -x "$(APPIMAGE_RUNTIME)" ] || { echo "Missing: $(APPIMAGE_RUNTIME)"; echo "Run 'make appimage-tools' to download the AppImage runtime."; exit 1; }
	@echo "OK: AppImage tools found ($(LINUXDEPLOY), $(APPIMAGETOOL))."

clean:
	$(call RM_RF,$(BUILD_DIR))
	$(call RM_RF,$(LINUX_TARGET))
	$(call RM_RF,$(WIN_TARGET))
	$(call RM_RF,$(WIN_PKG_DIR))
