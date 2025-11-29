DIST_DIR := ../../dist

.PHONY: all clean

all: $(DIST_DIR)/ljam

$(DIST_DIR)/ljam: main.lua
	@mkdir -p $(DIST_DIR)
	@echo '#!/usr/bin/env luajit' > $@
	@cat main.lua >> $@
	@chmod +x $@

clean:
	rm -f $(DIST_DIR)/ljam
