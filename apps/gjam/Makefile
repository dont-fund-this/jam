.PHONY: all clean

all:
	@mkdir -p ../../dist
	CGO_ENABLED=1 go build -ldflags="-s -w" -o ../../dist/gjam .

clean:
	rm -f ../../dist/gjam
