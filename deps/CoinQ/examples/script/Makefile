include mk/os.mk mk/cxx_flags.mk mk/boost_suffix.mk

INCLUDE_PATH += \
    -Isrc

LIBS = \
    -lCoinQ \
    -lCoinCore \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_regex$(BOOST_SUFFIX) \
    -lcrypto

all: build/templates${EXE_EXT} build/hdmofn${EXE_EXT}

clean:
	-rm -rf build/templates${EXE_EXT} build/hdmofn${EXE_EXT}

build/templates${EXE_EXT}: src/templates.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/hdmofn${EXE_EXT}: src/hdmofn.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)
