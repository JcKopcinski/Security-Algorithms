CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -pedantic -Wunused -ggdb -std=c++20

#static code analyzer
CPPCHECK = cppcheck --enable=all --inconclusive --std=c++20 --quiet --suppress=missingIncludeSystem

TARGET = DES_original
SRCS = DES_original.cpp
OBJS = $(SRCS:.cpp=.o)

all: static-check $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

static-check:
	@echo "Running Static Analysis..."
	$(CPPCHECK) $(SRCS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean static-check
