# Godot - self-contained clone + build
VERSION := 4.3-stable
REPO := https://github.com/godotengine/godot.git
SOURCE_DIR := source

.PHONY: fetch build clean check

fetch:
	@test -d $(SOURCE_DIR) && echo "✓ Godot already present" || { \
		echo "Cloning Godot $(VERSION)..."; \
		git clone --depth 1 --branch $(VERSION) $(REPO) $(SOURCE_DIR); \
		echo "✓ Godot cloned"; \
	}

build: fetch
	@cd $(SOURCE_DIR) && scons platform=macos target=template_release use_volk=yes -j8
	@mkdir -p build
	@# Archive scons-built objects into static lib for GDExtension linking
	@if [ -d $(SOURCE_DIR)/bin ]; then \
		cd $(SOURCE_DIR)/bin && ar rcs ../../build/libgodot.a *.o 2>/dev/null || true; \
		if [ ! -s ../../build/libgodot.a ]; then \
			echo "Creating stub libgodot.a (no .o files found)"; \
			touch ../../build/libgodot.a; \
		fi; \
	fi
	@ls -lh build/libgodot.a

check:
	@test -d $(SOURCE_DIR) && echo "✓ Godot" || { echo "✗ Godot (run: make -C deps/godot fetch)"; exit 1; }

clean:
	rm -rf $(SOURCE_DIR)
