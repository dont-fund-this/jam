# ========================================
# Unified Makefile Template for JAM Apps
# ========================================
# All 14 language implementations follow this pattern:
# 1. Build the language-specific binary/artifact
# 2. Copy to ../../dist/ with consistent naming (e.g., cjam, pjam, ljam)
# 3. All binaries/scripts are in dist/ and ready to run
# 4. libControl and plugins are automatically discovered at runtime
#
# Pattern for Compiled Languages (C++, Go, Rust, etc.):
#   all: [compile to ../../dist/LANGNAME]
#   clean: [remove binary and build artifacts]
#
# Pattern for Script-Based Languages (Python, Node.js, Lua, etc.):
#   all: [copy main script to ../../dist/LANGNAME with shebang]
#   clean: [remove copied script]
#
# ========================================

DIST_DIR := ../../dist

# ========================================
# C++ Example (cjam)
# ========================================
# CXX := clang++
# CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O3
# SRC := main.cpp control/boot.cpp control/bind.cpp control/attach.cpp control/invoke.cpp control/detach.cpp
# OBJ_DIR := build
# OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))
# TARGET := $(DIST_DIR)/cjam
# 
# .PHONY: all clean
# 
# all: $(TARGET)
# 
# $(TARGET): $(OBJ)
# 	@mkdir -p $(DIST_DIR)
# 	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)
# 
# $(OBJ_DIR)/%.o: %.cpp
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
# 
# clean:
# 	rm -rf $(OBJ_DIR) $(TARGET)

# ========================================
# Go Example (gjam)
# ========================================
# .PHONY: all clean
# 
# all:
# 	@mkdir -p $(DIST_DIR)
# 	CGO_ENABLED=1 go build -ldflags="-s -w" -o $(DIST_DIR)/gjam .
# 
# clean:
# 	go clean
# 	rm -f $(DIST_DIR)/gjam

# ========================================
# Rust Example (rjam)
# ========================================
# .PHONY: all clean
# 
# all:
# 	@mkdir -p $(DIST_DIR)
# 	cargo build --release
# 	cp target/release/rjam $(DIST_DIR)/rjam
# 
# clean:
# 	cargo clean
# 	rm -f $(DIST_DIR)/rjam

# ========================================
# Python Example (pjam) - SCRIPT-BASED
# ========================================
# .PHONY: all clean
# 
# all: $(DIST_DIR)/pjam
# 
# $(DIST_DIR)/pjam: main.py
# 	@mkdir -p $(DIST_DIR)
# 	@echo '#!/usr/bin/env python3' > $@
# 	@cat main.py >> $@
# 	@chmod +x $@
# 
# clean:
# 	rm -f $(DIST_DIR)/pjam

# ========================================
# Node.js Example (njam) - SCRIPT-BASED
# ========================================
# .PHONY: all clean
# 
# all: $(DIST_DIR)/njam
# 	npm install
# 
# $(DIST_DIR)/njam: index.js
# 	@mkdir -p $(DIST_DIR)
# 	@echo '#!/usr/bin/env node' > $@
# 	@cat index.js >> $@
# 	@chmod +x $@
# 
# clean:
# 	rm -rf node_modules package-lock.json
# 	rm -f $(DIST_DIR)/njam

# ========================================
# Lua Example (ljam) - SCRIPT-BASED
# ========================================
# .PHONY: all clean
# 
# all: $(DIST_DIR)/ljam
# 
# $(DIST_DIR)/ljam: main.lua
# 	@mkdir -p $(DIST_DIR)
# 	@echo '#!/usr/bin/env luajit' > $@
# 	@cat main.lua >> $@
# 	@chmod +x $@
# 
# clean:
# 	rm -f $(DIST_DIR)/ljam

# ========================================
# Key Points:
# ========================================
# 1. ALL apps compile/build to $(DIST_DIR)/LANGNAME
# 2. Script-based languages get shebang + chmod +x
# 3. All clean targets remove from $(DIST_DIR)
# 4. Root Makefile calls each app's Makefile
# 5. Runtime discovery of libControl + plugins is automatic
# 6. No language-specific logic needed in root Makefile
