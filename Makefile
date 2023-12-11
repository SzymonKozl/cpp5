CXXFLAGS = -Wall -Wextra -std=c++20 -O2
BINARIES = stack

all: $(BINARIES)

stack.o: stack.h stack.cc
stack_example.o: stack_example.cc

stack: stack.o stack_example.o
	g++ $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o

.PHONY: clean all
