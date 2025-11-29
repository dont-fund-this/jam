DIST_DIR := ../../dist

.PHONY: all clean

all: $(DIST_DIR)/pjam

$(DIST_DIR)/pjam: main.py
	@mkdir -p $(DIST_DIR)
	@echo '#!/usr/bin/env python3' > $@
	@cat main.py >> $@
	@chmod +x $@

clean:
	rm -f $(DIST_DIR)/pjam
