.PHONY: all clean

all:
	@mkdir -p ../../dist
	cargo build --release
	cp target/release/rjam ../../dist/rjam

clean:
	cargo clean
	rm -f ../../dist/rjam
