SHELL=/bin/bash

SRC_PATH=../../src

LIBS = \
	-l pthread \
	-l ssl \
	-l crypto \
	-l boost_regex

OBJS = \
	obj/CoinKey.o \
	obj/CoinNodeData.o \
	obj/TransactionSigner.o

all: rawtx

$(SRC_PATH)/obj/%.o: $(SRC_PATH)/%.cpp
	$(CXX) -c -o $@ $< \
	-I$(SRC_PATH) 

rawtx: $(OBJS:obj/%=$(SRC_PATH)/obj/%)
	$(CXX) -o $@ rawtx.cpp $^ \
	-I$(SRC_PATH) \
	$(LIBS)

clean:
	-rm -f rawtx

clean-all:
	-rm -f rawtx
	-rm -f $(SRC_PATH)/obj/*.o
