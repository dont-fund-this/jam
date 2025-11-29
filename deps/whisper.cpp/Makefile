# whisper.cpp - self-contained clone + build
VERSION := master
REPO := https://github.com/ggerganov/whisper.cpp.git
SOURCE_DIR := source

.PHONY: fetch build clean check

fetch:
	@test -d $(SOURCE_DIR) && echo "✓ Whisper already present" || { \
		echo "Cloning whisper.cpp $(VERSION)..."; \
		git clone --depth 1 --branch $(VERSION) $(REPO) $(SOURCE_DIR); \
		echo "✓ Whisper cloned"; \
	}

build: fetch
	@cd $(SOURCE_DIR) && $(MAKE) -j8

check:
	@test -d $(SOURCE_DIR) && echo "✓ Whisper" || { echo "✗ Whisper (run: make -C deps/whisper.cpp fetch)"; exit 1; }

clean:
	rm -rf $(SOURCE_DIR)
