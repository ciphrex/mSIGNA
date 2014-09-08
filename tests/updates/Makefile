MKDIR = ../../deps/mk

include $(MKDIR)/os.mk $(MKDIR)/cxx_flags.mk 

SRCDIR = ../../src

build/urlparser$(EXE_EXT): src/urlparser.cpp $(SRCDIR)/updates.cpp
	$(CXX) $(CXX_FLAGS) -I$(SRCDIR) $^ -o $@

