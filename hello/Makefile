CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
#CXXFLAGS = -Wall -O2 -I./include
LIBS     = ""
TARGET   = main
SRC = ./src
.PHONY: all clean install

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

install: $(TARGET)
	install -m 755 $(TARGET) $(HOME)/.local/bin/

clean:
	rm -f $(TARGET)
rm:
	rm -f $(TARGET)