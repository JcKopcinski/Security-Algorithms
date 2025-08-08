CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -pedantic -Wunused -ggdb -std=c++20

TARGET = DES_original
SRCS = DES_original.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
