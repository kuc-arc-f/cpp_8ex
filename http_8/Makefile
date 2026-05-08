CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -I ./include/laserpants/dotenv
LIBS     = -lsqlite3
TARGET   = server
SRC = ./src
.PHONY: all clean install

all: $(TARGET)

$(TARGET): server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

install: $(TARGET)
	install -m 755 $(TARGET) $(HOME)/.local/bin/

clean:
	rm -f $(TARGET)
rm:
	rm -f $(TARGET)