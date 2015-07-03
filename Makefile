CXX = g++
CXXFLAGS = -std=c++14 -Werror -Wall -Wextra

HEADERS = $(wildcard *.hpp)
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))

*.o: $(HEADERS)

sh: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

clean:
	rm *.o sh{,.exe}

.PHONY: clean
