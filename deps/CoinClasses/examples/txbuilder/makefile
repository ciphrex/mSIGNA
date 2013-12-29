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
    obj/CoinNodeData.o \
    obj/IPv6.o \
    obj/CoinKey.o \
    obj/StandardTransactions.o

all: txbuilder

$(SRC_PATH)/obj/%.o: $(SRC_PATH)/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< \
	-I$(SRC_PATH)

txbuilder: $(OBJS:obj/%=$(SRC_PATH)/obj/%) txbuilder.cpp txbuilder_commands.cpp
	$(CXX) $(CXX_FLAGS) -o $@ $^ \
	-I$(SRC_PATH) \
	$(LIBS)

clean:
	-rm -f txbuilder

clean-all:
	-rm -f txbuilder
	-rm -f $(SRC_PATH)/obj/*.o
