# llama.cpp - self-contained clone + build
VERSION := master
REPO := https://github.com/ggerganov/llama.cpp.git
SOURCE_DIR := source

.PHONY: fetch build clean check

fetch:
	@test -d $(SOURCE_DIR) && echo "✓ llama.cpp already present" || { \
		echo "Cloning llama.cpp $(VERSION)..."; \
		git clone --depth 1 --branch $(VERSION) $(REPO) $(SOURCE_DIR); \
		echo "✓ llama.cpp cloned"; \
	}

build: fetch
	@cd $(SOURCE_DIR) && cmake -B build -DBUILD_SHARED_LIBS=OFF && cmake --build build -j8
	
check:
	@test -d $(SOURCE_DIR) && echo "✓ llama.cpp" || { echo "✗ llama.cpp (run: make -C deps/llama.cpp fetch)"; exit 1; }

clean:
	rm -rf $(SOURCE_DIR)
