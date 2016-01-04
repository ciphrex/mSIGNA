CXX = clang++
CXXFLAGS = -std=c++11 -stdlib=libc++ -Wall -g

ROOTDIR = ../..
SRCDIR = $(ROOTDIR)/src
INCPATH = -I$(SRCDIR) -I/opt/local/include
LIBPATH = -L/opt/local/lib

HEADERS = \
    $(SRCDIR)/hdkeys.h \
    $(SRCDIR)/hash.h \
    $(SRCDIR)/secp256k1_openssl.h \
    $(SRCDIR)/BigInt.h \
    $(SRCDIR)/uchar_vector.h

LIBS = \
    -lCoinCore \
    -lcrypto

build/hdkeychain: hdkeychain.cpp $(ROOTDIR)/obj/hdkeys.o $(SRCDIR)/Base58Check.h
	$(CXX) $(CXXFLAGS) -o $@ $< $(ROOTDIR)/obj/hdkeys.o $(INCPATH) $(LIBPATH) $(LIBS)

$(SRCDIR)/obj/hdkeys.o: $(SRCDIR)/hdkeys.cpp $(HEADERS) 
	$(CXX) $(CXXFLAGS) -o $@ -c $<


clean:
	-rm -rf build/* $(SRCDIR)/obj/hdkeys.o
