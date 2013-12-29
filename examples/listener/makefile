SHELL=/bin/bash

SRC_PATH=../../src

CXX_FLAGS = -Wall -g -std=c++0x

LIBS = \
    -l rt \
	-l pthread \
	-l ssl \
	-l crypto \
	-l boost_regex

OBJS = \
    obj/IPv6.o \
	obj/CoinNodeData.o \
	obj/CoinNodeSocket.o \
	obj/CoinNodeListener.o

all: listener

$(SRC_PATH)/obj/%.o: $(SRC_PATH)/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< \
	-I$(SRC_PATH)

listener: $(OBJS:obj/%=$(SRC_PATH)/obj/%)
	$(CXX) $(CXX_FLAGS) -o $@ listener.cpp $^ \
	-I$(SRC_PATH) \
	$(LIBS)

clean:
	-rm -f listener

clean-all:
	-rm -f listener
	-rm -f $(SRC_PATH)/obj/*.o
