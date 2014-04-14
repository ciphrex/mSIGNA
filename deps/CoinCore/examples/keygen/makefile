SHELL=/bin/bash

SRC_PATH=../../src

CXX_FLAGS = -Wall -g -std=c++0x

LIBS = \
	-l ssl \
	-l crypto

all: keygen

$(SRC_PATH)/obj/CoinKey.o: $(SRC_PATH)/CoinKey.cpp
	$(CXX) $(CXX_FLAGS) -c -o $(SRC_PATH)/obj/CoinKey.o \
	$(SRC_PATH)/CoinKey.cpp \
	-I$(SRC_PATH)

keygen: $(SRC_PATH)/obj/CoinKey.o
	$(CXX) $(CXX_FLAGS) -o keygen \
	keygen.cpp $(SRC_PATH)/obj/CoinKey.o \
	-I$(SRC_PATH) \
	$(LIBS)

clean:
	-rm -f keygen

clean-all:
	-rm -f keygen
	-rm -f $(SRC_PATH)/obj/*.o
