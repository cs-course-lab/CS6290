SRCS = rat.cpp rob.cpp pipeline.cpp sim.cpp exeq.cpp
OBJS = $(SRCS:.cpp=.o)

CXX = g++
CXXFLAGS = -g -std=c++03 -Wall

all: sim

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

sim: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	-rm -f sim $(OBJS)
