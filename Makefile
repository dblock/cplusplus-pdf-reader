CXX = clang++
CPPFLAGS ?=
CXXFLAGS ?= -O2 -g
WARNINGS = -Wall -Wextra -Wpedantic -Wconversion -Wshadow
LDLIBS = -lz
SOURCES = src/main.cpp src/PDFFile.cpp src/PDFObjects.cpp
HEADERS = src/PDFFile.h src/PDFObjects.h
.PHONY: all clean
all: pdf-reader

pdf-reader: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(WARNINGS) -std=c++17 $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -f pdf-reader
