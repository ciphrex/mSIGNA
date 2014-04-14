CXX = g++
CXXFLAGS = -std=c++0x -Wall -g

SRCDIR = ../../src
INCPATH = -I$(SRCDIR)

HEADERS = \
    $(SRCDIR)/hdkeys.h \
    $(SRCDIR)/hash.h \
    $(SRCDIR)/secp256k1.h \
    $(SRCDIR)/BigInt.h \
    $(SRCDIR)/uchar_vector.h

build/hdwallets: hdwallets.cpp $(SRCDIR)/obj/hdkeys.o $(SRCDIR)/Base58Check.h
	$(CXX) $(CXXFLAGS)  -o $@ $< $(SRCDIR)/obj/hdkeys.o $(INCPATH) -lcrypto

$(SRCDIR)/obj/hdkeys.o: $(SRCDIR)/hdkeys.cpp $(HEADERS) 
	$(CXX) $(CXXFLAGS) -o $@ -c $<


clean:
	-rm -rf build/* $(SRCDIR)/obj/hdkeys.o
