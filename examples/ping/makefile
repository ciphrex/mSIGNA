SHELL=/bin/bash

SRC_PATH=../../src

CXX_FLAGS = -Wall -g -std=c++0x

LIBS = \
    -l rt \
	-l pthread \
	-l ssl \
	-l crypto \
	-l boost_regex \
    -l boost_thread

OBJS = \
    obj/IPv6.o \
	obj/CoinNodeData.o \
	obj/CoinNodeSocket.o

all: pingnode

$(SRC_PATH)/obj/%.o: $(SRC_PATH)/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< \
	-I$(SRC_PATH)

pingnode: $(OBJS:obj/%=$(SRC_PATH)/obj/%)
	$(CXX) $(CXX_FLAGS) -o $@ ping.cpp $^ \
	-I$(SRC_PATH) \
	$(LIBS)

clean:
	-rm -f pingnode

clean-all:
	-rm -f pingnode
	-rm -f $(SRC_PATH)/obj/*.o