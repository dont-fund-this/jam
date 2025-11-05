.PHONY: clean build dev run egg tui gui

clean:
	rm -rf bin/
	rm -rf .task/
	rm -rf refs/html/main/bindings/

build:
	wails3 task build

egg:
	$(MAKE) clean
	@rm -f refs/code/code.zip
	zip -r code.zip . -x "refs/code/code.zip" ".git/*" "bin/*" "build/*" "*.DS_Store" "refs/html/main/node_modules/*"
	@mkdir -p refs/code
	mv code.zip refs/code/code.zip

dev:
	$(MAKE) egg
	wails3 dev -config ./build/config.yml

run: build
	./bin/jam

tui: build
	./bin/jam tui show

gui: build
	./bin/jam gui show
