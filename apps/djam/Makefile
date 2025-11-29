DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/djam.dll

.PHONY: all clean

all: $(TARGET)

$(TARGET):
	@mkdir -p $(DIST_DIR)
	dotnet publish -c Release -o $(DIST_DIR) --self-contained false

clean:
	dotnet clean
	rm -rf bin obj
	rm -f $(DIST_DIR)/djam.dll $(DIST_DIR)/djam.runtimeconfig.json $(DIST_DIR)/djam.deps.json $(DIST_DIR)/djam.pdb
