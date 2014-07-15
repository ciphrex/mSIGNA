CXX = g++
CXXFLAGS += -Wall -O3 --std=c++0x

SRCDIR = \
    ../../src

OBJ = \
    obj/numberformats.o

build/regexp: src/regexp.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $^ -o $@

obj/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/%.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
