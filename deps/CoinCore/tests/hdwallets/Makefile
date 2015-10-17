SYSROOT = ../../../../sysroot
CXX = g++
CXXFLAGS = -std=c++0x -Wall -g

SRCDIR = ../../src
OBJDIR = ../../obj
INCPATH += \
    -I$(SRCDIR) \
    -I$(SYSROOT)/include

OBJS = \
    $(OBJDIR)/hdkeys.o \
    $(OBJDIR)/secp256k1_openssl.o

HEADERS = \
    $(SRCDIR)/hdkeys.h \
    $(SRCDIR)/hash.h \
    $(SRCDIR)/secp256k1_openssl.h \
    $(SRCDIR)/BigInt.h

build/hdwallets: hdwallets.cpp $(OBJS) $(SRCDIR)/Base58Check.h
	$(CXX) $(CXXFLAGS) $(INCPATH) -o $@ $< $(OBJS) -lcrypto

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/%.h $(HEADERS) 
	$(CXX) $(CXXFLAGS) $(INCPATH) -o $@ -c $<

clean:
	-rm -rf build/* $(SRCDIR)/obj/hdkeys.o
