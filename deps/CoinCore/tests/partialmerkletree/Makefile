CXX = g++
CXXFLAGS = -std=c++0x -Wall -g

ROOTDIR = ../..
INCPATH = -I$(ROOTDIR)/src

LIBS = \
    -lcrypto \
    -lboost_regex

OBJ = \
    $(ROOTDIR)/obj/MerkleTree.o

TARGETS = \
    build/set \
    build/merge \
    build/random

all: $(TARGETS)

build/%: %.cpp $(OBJ)
	$(CXX) $(CXXFLAGS)  -o $@ $< $(OBJ) $(INCPATH) $(LIBS)

$(ROOTDIR)/obj/%.o: $(ROOTDIR)/src/%.cpp $(ROOTDIR)/src/%.h
	$(CXX) $(CXXFLAGS) -o $@ -c $< $(INCPATH)


clean:
	-rm -rf build/*

clean-all:
	-rm -rf build/* $(OBJ)
