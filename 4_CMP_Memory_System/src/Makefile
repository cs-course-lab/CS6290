SRCS = cache.cpp core.cpp dram.cpp memsys.cpp sim.cpp
OBJS = $(SRCS:.cpp=.o)

CXX = g++
CXXFLAGS = -g -std=c++11 -Wall

all: sim

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

sim: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	-rm -f sim $(OBJS)
