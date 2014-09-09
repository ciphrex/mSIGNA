CXX = g++

CXXFLAGS += -Wall -O3 -std=c++0x

INCLUDE_PATH = \
    -I../../src

OBJS = \
    ../../obj/secp256k1.o

LIBS = \
    -lcrypto

all: build/secp256k1_test build/secp256k1_rfc6979_test build/ascii2hex

build/secp256k1_test: src/secp256k1_test.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/secp256k1_rfc6979_test: src/secp256k1_rfc6979_test.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/ascii2hex: src/ascii2hex.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)
