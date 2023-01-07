SRCS = sim.cpp pipeline.cpp bpred.cpp
OBJS = $(SRCS:.cpp=.o)

CXX = g++
CXXFLAGS = -g -std=c++0x -Wall

all: sim

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

sim: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	-rm -f sim $(OBJS)
