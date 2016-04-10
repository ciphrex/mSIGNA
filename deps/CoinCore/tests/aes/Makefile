CXX = g++
CXXFLAGS = -std=c++0x -Wall -g

ROOTDIR = ../..
INCPATH = -I$(ROOTDIR)/src

LIBS = \
    -lcrypto

OBJ = \
    $(ROOTDIR)/obj/aes.o

TARGETS = \
    build/encrypt \
    build/decrypt

all: $(TARGETS)

build/%: %.cpp $(OBJ)
	$(CXX) $(CXXFLAGS)  -o $@ $< $(OBJ) $(INCPATH) $(LIBS)

$(ROOTDIR)/obj/%.o: $(ROOTDIR)/src/%.cpp $(ROOTDIR)/src/%.h
	$(CXX) $(CXXFLAGS) -o $@ -c $< $(INCPATH)


clean:
	-rm -rf build/*

clean-all:
	-rm -rf build/* $(OBJ)
