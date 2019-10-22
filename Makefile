TARGET = libnrf24l01_analyzer.so

CC ?= g++
LDFLAGS = -lAnalyzer
CFLAGS = -Wall -Iinclude -IAnalyzerSDK/include/

CFLAGS += `pkg-config --cflags libusb-1.0`
LDFLAGS += `pkg-config --libs libusb-1.0`

.PHONY: default all clean

default: $(TARGET)
all: default

OBJ = obj
SOURCES = src
OBJECTS = $(patsubst %.cpp, $(OBJ)/%.o, $(wildcard $(SOURCES)/*.cpp))
HEADERS = $(wildcard include/*.h)

$(OBJ)/%.o: %.cpp $(HEADERS)
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS) -o $@

clean:
	-rm -rf $(OBJ)
	-rm -f $(TARGET)
	-rm -f config.mk
	-rm -f config.status
