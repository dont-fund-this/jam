CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O3 -fPIC -I.
LDFLAGS := -dynamiclib

# Godot GDExtension
GODOT_ROOT := ../../deps/godot/source
ifneq ($(wildcard $(GODOT_ROOT)/core),)
    CXXFLAGS += -I$(GODOT_ROOT)/core -I$(GODOT_ROOT)/modules
endif

SRC := $(wildcard *.cpp)

OBJ_DIR := build
OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/libege.dylib

.PHONY: all clean pre-build

all: $(TARGET)

pre-build:

$(TARGET): $(OBJ) ../../deps/godot/build/libgodot.a
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ) ../../deps/godot/build/libgodot.a -lSystem
	@filesize=$$(stat -f%z $@ 2>/dev/null); \
	  test "$$filesize" -gt 40960 || (echo "$@ too small (<40KB), size=$$filesize" && false)
	@echo "✓ $@ (size: $$(stat -f%z $@) bytes)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
