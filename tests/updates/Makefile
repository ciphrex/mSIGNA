MKDIR = ../../deps/mk

include $(MKDIR)/os.mk $(MKDIR)/cxx_flags.mk $(MKDIR)/boost_suffix.mk

SRCDIR = ../../src

LIBS = -lboost_system$(BOOST_SUFFIX)

all: urlparser latestversioninfo
urlparser: build/urlparser$(EXE_EXT)
latestversioninfo: build/latestversioninfo$(EXE_EXT)

build/urlparser$(EXE_EXT): src/urlparser.cpp $(SRCDIR)/updates.cpp
	$(CXX) $(CXX_FLAGS) -I$(SRCDIR) $^ -o $@ $(LIBS)

build/latestversioninfo$(EXE_EXT): src/latestversioninfo.cpp $(SRCDIR)/updates.cpp
	$(CXX) $(CXX_FLAGS) -I$(SRCDIR) $^ -o $@ $(LIBS)
