DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/hjam

.PHONY: all clean

all: $(TARGET)

$(TARGET):
	@mkdir -p $(DIST_DIR)
	ghc -O2 -threaded -dynamic Main.hs -o $@

clean:
	rm -f *.o *.hi Main
	rm -f $(TARGET)
