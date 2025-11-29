APPS := $(wildcard *)

.PHONY: all clean

all:
	@for app in $(APPS); do $(MAKE) -C $$app; done

clean:
	@for app in $(APPS); do $(MAKE) -C $$app clean; done
