TARGET = libnrf24l01_analyzer.so

CC ?= g++
LDFLAGS = -lAnalyzer64 -LAnalyzerSDK/lib/
CFLAGS = -fPIC -Wall -Iinclude -IAnalyzerSDK/include/ 

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
	$(CC) -shared $(OBJECTS) -Wall $(LDFLAGS) -o $@

clean:
	-rm -rf $(OBJ)
	-rm -f $(TARGET)
