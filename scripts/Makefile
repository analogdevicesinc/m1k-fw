CXX=g++
CXXFLAGS=-g -std=c++11 -Wall -pedantic -O3
LINKFLAGS=-lusb-1.0 -lm
BIN=testusb

SRC=testusb.cpp
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) -o $(BIN) $^ $(LINKFLAGS)

%.o: %.c
	$(CXX) $@ -c $<

clean:
	rm -f *.o
	rm $(BIN)