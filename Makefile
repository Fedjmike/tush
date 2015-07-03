CXX = g++
CXXFLAGS = -std=c++14 -Werror -Wall -Wextra

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))

sh: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@
