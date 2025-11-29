LIBS := $(filter-out libs/TEMPLATE.mk,$(wildcard libs/*))
APPS := $(filter-out apps/Makefile,$(wildcard apps/*))
CODE_ZIP := refs/code/code.zip

.PHONY: all all-hosts clean clean-hosts run run-cjam run-gjam run-rjam run-njam run-pjam run-jjam run-djam run-kjam run-sjam run-ljam run-zjam run-hjam run-ojam run-vjam
.PHONY: libs apps $(APPS)
.PHONY: test-none test-control-only test-all
.PHONY: bench-cjam bench-gjam bench-rjam bench-njam bench-pjam bench-jjam bench-djam bench-kjam bench-sjam bench-ljam bench-zjam bench-hjam bench-ojam bench-vjam bench-all
.PHONY: deps-fetch deps-build deps-check deps-clean
.PHONY: egg-lay egg-test egg-clean
.PHONY: oobe

# Default: build all plugins and C++ host
all: libs cjam

# Build all hosts (all languages)
all-hosts: libs apps

apps: libs
	$(MAKE) -C apps all

# Pattern rule: any target matching an app directory builds that app
$(APPS): libs
	$(MAKE) -C apps/$@

# Shorthand targets: map cjam -> apps/cjam, gjam -> apps/gjam, etc.
cjam gjam rjam njam pjam jjam djam kjam sjam ljam zjam hjam ojam vjam: libs
	$(MAKE) -C apps/$@

libs:
	@for dir in $(LIBS); do $(MAKE) -C $$dir pre-build; done
	@for dir in $(LIBS); do $(MAKE) -C $$dir; done



clean-hosts:
	rm -rf dist/

clean:
	@for dir in $(LIBS); do $(MAKE) -C $$dir clean 2>/dev/null || true; done
	@for app in $(APPS); do $(MAKE) -C $$app clean 2>/dev/null || true; done
	rm -f dist/*.dylib dist/*.so dist/*.dll dist/cjam dist/gjam dist/rjam dist/djam* dist/*.js dist/*.py dist/*.json dist/*.jar
	rm -rf dist/node_modules dist/*-node_modules dist/*-package.json
	$(MAKE) deps-clean

# Run targets - symmetrical naming
run-cjam: cjam
	cd dist && ./cjam

run-gjam: gjam
	cd dist && ./gjam

run-rjam: rjam
	cd dist && ./rjam

run-njam: njam
	cd apps/njam && node index.js

run-pjam: pjam
	cd apps/pjam && python3 main.py

run-jjam: jjam
	cd dist && export PATH="/opt/homebrew/opt/openjdk/bin:$$PATH" && java -cp jna-5.14.0.jar:jjam.jar Main

run-djam: djam
	cd dist && dotnet djam.dll

run-kjam: kjam
	cd dist && export PATH="/opt/homebrew/opt/openjdk/bin:$$PATH" && java -cp jna-5.14.0.jar:kjam.jar MainKt

run-sjam: sjam
	cd dist && ./sjam

run-ljam: ljam
	cd apps/ljam && luajit main.lua

run-zjam: zjam
	cd dist && ./zjam

run-hjam: hjam
	cd dist && ./hjam

run-ojam: ojam
	cd dist && ./ojam

run-vjam: vjam
	cd dist && ./vjam

#
# Default run target
run: run-cjam

# Test targets - use C++ host by default
test-none: clean
	@echo "=== TEST 1: No libraries at all ==="
	$(MAKE) cjam
	$(MAKE) run || true

test-control-only: clean
	@echo "=== TEST 2: Only control plugin ==="
	$(MAKE) -C libs/control
	$(MAKE) cjam
	$(MAKE) run

test-all: clean all
	@echo "=== TEST 3: All plugins ==="
	$(MAKE) run

#
# LLM targets
test-llm-vision:
	@echo "=== Testing LLM Library (test_llm_vision.cpp) ==="
	@cd libs/llm && \
	clang++ -std=c++20 -o build/test_llm_vision test_llm_vision.cpp && \
	cd ../.. && \
	./libs/llm/build/test_llm_vision

test-llm-15b:
	@echo "=== Testing LLM Library (test_llm_15b.cpp) ==="
	@cd libs/llm && \
	mkdir -p build && \
	clang++ -std=c++20 -o build/test_llm_15b test_llm_15b.cpp && \
	cd ../.. && \
	./libs/llm/build/test_llm_15b

test-llm-32:
	@echo "=== Testing LLM Library (test_llm_32b.cpp) ==="
	@cd libs/llm && \
	clang++ -std=c++20 -o build/test_llm_32b test_llm_32b.cpp && \
	cd ../.. && \
	./libs/llm/build/test_llm_32b

#
# Dependency management targets
deps-fetch:
	$(MAKE) -C deps fetch

deps-build:
	$(MAKE) -C deps build

deps-check:
	$(MAKE) -C deps check

deps-clean:
	$(MAKE) -C deps clean

#
# Benchmark targets - template for all hosts
define BENCH_TEMPLATE
bench-$(1):
	@$$(MAKE) clean-hosts > /dev/null 2>&1
	@$$(MAKE) $(1) > /dev/null 2>&1
	@echo "=== Benchmarking $(1) (50 runs) ==="
	@total=0; min=999999; max=0; \
	for i in $$$$(seq 1 50); do \
		start=$$$$(perl -MTime::HiRes=time -e 'printf "%.6f", time'); \
		$$(MAKE) run-$(1) > /dev/null 2>&1; \
		end=$$$$(perl -MTime::HiRes=time -e 'printf "%.6f", time'); \
		elapsed=$$$$(echo "$$$$end - $$$$start" | bc); \
		total=$$$$(echo "$$$$total + $$$$elapsed" | bc); \
		test $$$$(echo "$$$$elapsed < $$$$min" | bc) -eq 1 && min=$$$$elapsed; \
		test $$$$(echo "$$$$elapsed > $$$$max" | bc) -eq 1 && max=$$$$elapsed; \
	done; \
	avg=$$$$(echo "scale=6; $$$$total / 50" | bc); \
	avg_ms=$$$$(echo "$$$$avg * 1000" | bc); \
	min_ms=$$$$(echo "$$$$min * 1000" | bc); \
	max_ms=$$$$(echo "$$$$max * 1000" | bc); \
	printf "Average: %.2fms  Min: %.2fms  Max: %.2fms\n" $$$$avg_ms $$$$min_ms $$$$max_ms
endef

bench-all:
	@for app in $(APPS); do $(MAKE) bench-$$app; done

#
# Egg targets - package clean source code
egg-lay:
	@echo "=== Creating mono egg ==="
	$(MAKE) clean
	@rm -rf dist/
	@rm -f $(CODE_ZIP)
	@echo "Packaging clean source code..."
	@zip -r code.zip \
		Makefile read.md \
		apps/ libs/ deps/ refs/ \
		-x "dist/*" \
		-x "*/build/*" \
		-x "*/.git/*" \
		-x "deps/*/source/*" \
		-x "*.o" "*.dylib" "*.so" "*.dll" \
		-x "*/node_modules/*" \
		-x "*.jar" "*.class" \
		-x "*/.task/*" \
		-x "*.DS_Store" \
		-x ".gitignore" \
		-x "$(CODE_ZIP)"
	@mkdir -p $$(dirname $(CODE_ZIP))
	@mv code.zip $(CODE_ZIP)
	@ls -lh $(CODE_ZIP)
	@echo "✓ Egg created: $(CODE_ZIP)"

egg-test:
	@echo "=== Testing mono egg workflow ==="
	@TEST_DIR=/tmp/mono-egg-test && \
	rm -rf $$TEST_DIR && \
	mkdir -p $$TEST_DIR && \
	cd $$TEST_DIR && unzip -q $(shell pwd)/$(CODE_ZIP) && \
	echo "Verifying clean source (no dist/)..." && \
	test ! -d $$TEST_DIR/dist && echo "✓ No artifacts in egg" || (echo "✗ Found artifacts"; exit 1) && \
	echo "" && \
	echo "Running make oobe in extracted egg..." && \
	$(MAKE) -C $$TEST_DIR oobe && \
	echo "" && \
	echo "✓ Egg test passed!"

egg-clean:
	rm -f $(CODE_ZIP)

#
# Out-of-box experience - complete workflow from scratch
oobe:
	@echo "=== OOBE: Complete Clean Build 4 minutes===" 
	$(MAKE) clean
	@echo ""
	@echo "=== OOBE: Fetching Dependencies ==="
	$(MAKE) deps-fetch
	@echo ""
	@echo "=== OOBE: Building Dependencies ==="
	$(MAKE) deps-build
	@echo ""
	@echo "=== OOBE: Building All Plugins + Hosts ==="
	$(MAKE) all-hosts
	@echo ""
	@echo "=== OOBE: Testing cjam (C++) ==="
	$(MAKE) run-cjam
	@echo ""
	@echo "=== OOBE: Testing gjam (Go) ==="
	$(MAKE) run-gjam
	@echo ""
	@echo "=== OOBE: Testing rjam (Rust) ==="
	$(MAKE) run-rjam
	@echo ""
	@echo "=== OOBE: Testing njam (Node.js) ==="
	$(MAKE) run-njam
	@echo ""
	@echo "=== OOBE: Testing pjam (Python) ==="
	$(MAKE) run-pjam
	@echo ""
	@echo "=== OOBE: Testing jjam (Java) ==="
	$(MAKE) run-jjam
	@echo ""
	@echo "=== OOBE: Testing djam (C#/.NET) ==="
	$(MAKE) run-djam
	@echo ""
	@echo "=== OOBE: Testing kjam (Kotlin) ==="
	$(MAKE) run-kjam
	@echo ""
	@echo "=== OOBE: Testing sjam (Swift) ==="
	$(MAKE) run-sjam
	@echo ""
	@echo "=== OOBE: Testing ljam (Lua) ==="
	$(MAKE) run-ljam
	@echo ""
	@echo "=== OOBE: Testing zjam (Zig) ==="
	$(MAKE) run-zjam
	@echo ""
	@echo "=== OOBE: Testing hjam (Haskell) ==="
	$(MAKE) run-hjam
	@echo ""
	@echo "=== OOBE: Testing ojam (OCaml) ==="
	$(MAKE) run-ojam

