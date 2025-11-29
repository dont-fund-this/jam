CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O3 -fPIC -I. -Wno-unused-function
LDFLAGS := -dynamiclib

# llama.cpp with vision support (mtmd)
LLAMA_ROOT := ../../deps/llama.cpp/source
LLAMA_BUILD := $(LLAMA_ROOT)/build

CXXFLAGS += -I$(LLAMA_ROOT)/include -I$(LLAMA_ROOT)/ggml/include -I$(LLAMA_ROOT)/tools/mtmd

# Link static libraries
LDFLAGS += $(LLAMA_BUILD)/src/libllama.a \
           $(LLAMA_BUILD)/ggml/src/libggml.a \
           $(LLAMA_BUILD)/ggml/src/libggml-base.a \
           $(LLAMA_BUILD)/ggml/src/libggml-cpu.a \
           $(LLAMA_BUILD)/ggml/src/ggml-blas/libggml-blas.a \
           $(LLAMA_BUILD)/ggml/src/ggml-metal/libggml-metal.a \
           $(LLAMA_BUILD)/tools/mtmd/libmtmd.a \
           -framework Accelerate -framework Foundation -framework Metal -framework MetalKit

# Exclude test executable (it has main())
SRC := $(filter-out test_llm.cpp,$(wildcard *.cpp))

OBJ_DIR := build
OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/libllm.dylib

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
