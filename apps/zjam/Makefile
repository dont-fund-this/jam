DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/zjam

.PHONY: all clean

all: $(TARGET)

$(TARGET):
	@mkdir -p $(DIST_DIR)
	zig build -Doptimize=ReleaseFast
	@cp zig-out/bin/zjam $@

clean:
	rm -rf zig-out .zig-cache
	rm -f $(TARGET)
