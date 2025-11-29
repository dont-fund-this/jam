OCAMLOPT := ocamlopt

DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/ojam

STUBS := stubs.c
MAIN := main.ml

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(STUBS) $(MAIN)
	@mkdir -p $(DIST_DIR)
	$(OCAMLOPT) -c $(STUBS)
	$(OCAMLOPT) -o $@ stubs.o $(MAIN)

run: $(TARGET)
	cd $(DIST_DIR) && ./$(notdir $(TARGET))

clean:
	rm -f *.o *.cmi *.cmx *.cmo
	rm -f $(TARGET)
