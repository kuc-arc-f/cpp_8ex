CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -I/usr/include/postgresql -I ./include/laserpants/dotenv -pthread
LIBS     = -lpq
TARGET   = server
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