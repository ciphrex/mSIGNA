SHELL=/bin/bash

SRC_PATH=../../src

CXX_FLAGS = -Wall -std=c++0x
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

LIBS = \
    -l rt \
	-l pthread \
	-l ssl \
	-l crypto \
	-l boost_system \
	-l boost_regex \
	-l boost_thread

OBJS = \
    obj/IPv6.o \
	obj/CoinNodeData.o \
	obj/CoinNodeSocket.o \
	obj/CoinNodeAbstractListener.o

all: listener2

$(SRC_PATH)/obj/%.o: $(SRC_PATH)/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< \
	-I$(SRC_PATH)

listener2: listener2.cpp $(OBJS:obj/%=$(SRC_PATH)/obj/%)
	$(CXX) $(CXX_FLAGS) -o $@ $^ \
	-I$(SRC_PATH) \
	$(LIBS)

clean:
	-rm -f listener2

clean-all:
	-rm -f listener2
	-rm -f $(SRC_PATH)/obj/*.o
