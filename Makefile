# Compiler and flags
CXX      := gcc
CXXFLAGS := -std=c11 -O2 -Wall -Wextra -pthread -g
LDFLAGS  := -lnuma

# Targets
TARGET   := counter_benchmark
SRCS     := main.c percpu_counter.c spinlock.c
OBJS     := $(SRCS:.c=.o)
HDRS	 := *.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c  $(HDRS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)