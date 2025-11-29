CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O3

SRC := main.cpp control/boot.cpp control/bind.cpp control/attach.cpp control/invoke.cpp control/detach.cpp


OBJ_DIR := build
OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/cjam

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

run: $(TARGET)
	cd $(DIST_DIR) && ./$(notdir $(TARGET))

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET) *.o
