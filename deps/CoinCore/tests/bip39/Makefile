CXX = g++
CXXFLAGS = -std=c++0x -Wall -g

ROOTDIR = ../..
INCPATH = -I$(ROOTDIR)/src -I$(ROOTDIR)/../stdutils/src

LIBS = \
    -lcrypto

OBJ = \
    $(ROOTDIR)/obj/bip39.o

TARGETS = \
    build/towordlist \
    build/fromwordlist

all: $(TARGETS)

build/%: %.cpp $(OBJ)
	$(CXX) $(CXXFLAGS)  -o $@ $< $(OBJ) $(INCPATH) $(LIBS)

$(ROOTDIR)/obj/%.o: $(ROOTDIR)/src/%.cpp $(ROOTDIR)/src/%.h
	$(CXX) $(CXXFLAGS) -o $@ -c $< $(INCPATH)


clean:
	-rm -rf build/*

clean-all:
	-rm -rf build/* $(OBJ)
