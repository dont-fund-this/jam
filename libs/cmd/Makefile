CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O3 -fPIC -I.
LDFLAGS := -dynamiclib

SRC := $(wildcard *.cpp)

OBJ_DIR := build
OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/libcmd.dylib

.PHONY: all clean pre-build

all: $(TARGET)

pre-build:

$(TARGET): $(OBJ)
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
