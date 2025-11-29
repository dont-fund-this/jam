DEPS := cef godot llama.cpp whisper.cpp

.PHONY: fetch build check clean

fetch:
	@for dep in $(DEPS); do $(MAKE) -C $$dep fetch; done

build:
	@for dep in $(DEPS); do $(MAKE) -C $$dep build; done

check:
	@for dep in $(DEPS); do $(MAKE) -C $$dep check; done

clean:
	@for dep in $(DEPS); do $(MAKE) -C $$dep clean; done
