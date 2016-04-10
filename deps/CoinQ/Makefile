INCLUDE_PATH += -Isrc
PROJECT_SYSROOT = ../../sysroot

include ../mk/os.mk ../mk/cxx_flags.mk ../mk/boost_suffix.mk

OBJS = \
    obj/CoinQ_coinparams.o \
    obj/CoinQ_script.o \
    obj/CoinQ_peer_io.o \
    obj/CoinQ_netsync.o \
    obj/CoinQ_blocks.o \
    obj/CoinQ_txs.o \
    obj/CoinQ_keys.o \
    obj/CoinQ_filter.o \
    obj/BlockchainDownload.o

LIBS = \
    -lCoinQ \
    -lCoinCore \
    -llogger \
    -lsysutils \
    -lboost_regex$(BOOST_SUFFIX) \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_filesystem$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_THREAD_SUFFIX)$(BOOST_SUFFIX) \
    -lboost_program_options$(BOOST_SUFFIX) \
    -lcrypto

EXAMPLES = \
    examples/build/peer$(EXE_EXT) \
    examples/build/netsync$(EXE_EXT) \
    examples/build/blockchain$(EXE_EXT)

lib: lib/libCoinQ.a

all: lib/libCoinQ.a examples

lib/libCoinQ.a: $(OBJS)
	$(ARCHIVER) rcs $@ $^

obj/%.o: src/%.cpp src/%.h
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

examples: $(EXAMPLES)

examples/build/%$(EXE_EXT): examples/%/src/main.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $< -o $@ $(LIBS) $(PLATFORM_LIBS)

install: install-lib

install-lib:
	-mkdir -p $(SYSROOT)/include/CoinQ
	-rsync -u src/*.h  $(SYSROOT)/include/CoinQ/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/libCoinQ.a $(SYSROOT)/lib/

remove: remove-lib

remove-lib:
	-rm -rf $(SYSROOT)/include/CoinQ
	-rm $(SYSROOT)/lib/libCoinQ.a

clean: clean-lib

clean-all: clean-lib clean-examples

clean-lib:
	-rm -f obj/*.o lib/*.a

clean-examples:
	-rm -f $(EXAMPLES)

