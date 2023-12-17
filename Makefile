CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -O2 -g
BINARIES = stack_example

all: $(BINARIES)

stack_example.o: stack_example.cc stack.h

stack_example: stack_example.o
	g++ $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o

.PHONY: clean all
