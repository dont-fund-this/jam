V := v

DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/vjam

MAIN := main.v

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(MAIN)
	@mkdir -p $(DIST_DIR)
	$(V) -prod -o $@ $(MAIN)

run: $(TARGET)
	cd $(DIST_DIR) && ./$(notdir $(TARGET))

clean:
	rm -f $(TARGET)
