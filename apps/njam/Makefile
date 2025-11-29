DIST_DIR := ../../dist

.PHONY: all clean

all: $(DIST_DIR)/njam

npm-install:
	npm install

$(DIST_DIR)/njam: index.js npm-install
	@mkdir -p $(DIST_DIR)
	@echo '#!/usr/bin/env node' > $@
	@cat index.js >> $@
	@chmod +x $@

clean:
	rm -rf node_modules package-lock.json
	rm -f $(DIST_DIR)/njam
