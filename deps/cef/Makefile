# CEF - self-contained clone
REPO := https://bitbucket.org/chromiumembedded/cef.git
SOURCE_DIR := source

.PHONY: fetch build clean check

fetch:
	@test -d $(SOURCE_DIR) && echo "✓ CEF already present" || { \
		echo "Cloning CEF (this may take a while)..."; \
		git clone --depth 1 $(REPO) $(SOURCE_DIR); \
		echo "✓ CEF cloned"; \
	}

build: fetch
	@echo "✓ CEF (prebuilt binaries)"

check:
	@test -d $(SOURCE_DIR) && echo "✓ CEF" || { echo "✗ CEF (run: make -C deps/cef fetch)"; exit 1; }

clean:
	rm -rf $(SOURCE_DIR) $(ARCHIVE) *.tar.bz2
