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
	obj/CoinNodeSocket.o \
	obj/CoinNodeAbstractListener.o

all: nodecrawler

$(SRC_PATH)/obj/%.o: $(SRC_PATH)/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< \
	-I$(SRC_PATH)

nodecrawler: $(OBJS:obj/%=$(SRC_PATH)/obj/%)
	$(CXX) $(CXX_FLAGS) -o $@ nodecrawler.cpp $^ \
	-I$(SRC_PATH) \
	$(LIBS)

clean:
	-rm -f nodecrawler

clean-all:
	-rm -f nodecrawler
	-rm -f $(SRC_PATH)/obj/*.o
