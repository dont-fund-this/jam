CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O3 -fPIC -I.
LDFLAGS := -dynamiclib

# Exclude test and generator executables (they have main())
SRC := $(filter-out test_embed.cpp generate_embedded.cpp,$(wildcard *.cpp))

OBJ_DIR := build
OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/libefs.dylib

GEN_SRC := generate_embedded.cpp
GEN_BIN := $(OBJ_DIR)/generate_embedded
TEST_SRC := test_embed.cpp
TEST_BIN := $(OBJ_DIR)/test_embed
EMBED_FILE := embedded_refs.cpp

.PHONY: all clean test regen-embedded

all: $(EMBED_FILE) $(TARGET)

# Generate embedded_refs.cpp if missing
$(EMBED_FILE): $(GEN_BIN)
	@if [ ! -f $(EMBED_FILE) ]; then \
		echo "Generating $(EMBED_FILE)..."; \
		$(GEN_BIN); \
	fi

$(GEN_BIN): $(GEN_SRC)
	@mkdir -p $(dir $@)
	$(CXX) -std=c++20 -Wall -Wextra -o $@ $<

$(TARGET): $(OBJ)
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build and run test
test: $(TARGET) $(TEST_BIN)
	@echo "Running embedded filesystem test..."
	@$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	@mkdir -p $(dir $@)
	$(CXX) -std=c++20 -Wall -Wextra -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Manual target to regenerate embedded_refs.cpp
regen-embedded:
	@$(OBJ_DIR)/generate_embedded