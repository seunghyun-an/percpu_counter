# Compiler and flags
CXX      := gcc
CXXFLAGS := -std=c11 -O2 -Wall -Wextra -pthread
LDFLAGS  := -lnuma

# Targets
TARGET   := counter_benchmark
SRCS     := main.cpp
OBJS     := $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp funcs.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)