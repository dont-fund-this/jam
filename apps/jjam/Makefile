DIST_DIR := ../../dist
TARGET := $(DIST_DIR)/jjam.jar
JNA_JAR := jna-5.14.0.jar

.PHONY: all clean

all: $(TARGET)

$(JNA_JAR):
	@if [ ! -f $@ ]; then \
		echo "Downloading JNA..."; \
		curl -L -o $@ https://repo1.maven.org/maven2/net/java/dev/jna/jna/5.14.0/jna-5.14.0.jar; \
	fi

$(TARGET): Main.java $(JNA_JAR)
	@mkdir -p $(DIST_DIR)
	javac -cp $(JNA_JAR) -O Main.java
	jar cfe $@ Main Main*.class
	cp $(JNA_JAR) $(DIST_DIR)/
	rm -f Main*.class

clean:
	rm -f Main*.class $(JNA_JAR)
	rm -f $(TARGET) $(DIST_DIR)/$(JNA_JAR)
