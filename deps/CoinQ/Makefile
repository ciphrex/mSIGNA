ODB_DB = \
    -DDATABASE_SQLITE

CXX_FLAGS = -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

INCLUDE_PATH = \
    -Isrc \
    -I../CoinClasses/src \
    -I../logger/src

ifeq ($(OS), linux)
    CXX = g++
    CC = gcc
    CXX_FLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    ARCHIVER = ar

else ifeq ($(OS), mingw64)
    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    MINGW64_ROOT = /opt/mingw64

    INCLUDE_PATH += -I$(MINGW64_ROOT)/include

    ARCHIVER = x86_64-w64-mingw32-ar

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    INCLUDE_PATH += -I/usr/local/include

    ARCHIVER = ar

else ifeq ($(OS), cygwin)
    CXX_FLAGS += -Wno-unused-local-typedefs -std=c++0x

else
    $(error OS must be set to linux, osx, mingw64, or cygwin)
endif

OBJS = \
    obj/CoinQ_vault_db-odb.o \
    obj/CoinQ_vault.o \
    obj/CoinQ_script.o \
    obj/CoinQ_peer_io.o \
    obj/CoinQ_netsync.o \
    obj/CoinQ_blocks.o \
    obj/CoinQ_txs.o \
    obj/CoinQ_keys.o \
    obj/CoinQ_filter.o


all: $(OBJS)

#
# odb compiled dependencies
#
odb/CoinQ_vault_db-odb.hxx: src/CoinQ_vault_db.hxx
	odb --output-dir odb/ --std c++11 -d sqlite --generate-query --generate-schema src/CoinQ_vault_db.hxx $(INCLUDE_PATH)

odb/CoinQ_vault_db-odb.cxx: src/CoinQ_vault_db.hxx
	odb --output-dir odb/ --std c++11 -d sqlite --generate-query --generate-schema src/CoinQ_vault_db.hxx $(INCLUDE_PATH)

#
# vault dependencies
#
obj/CoinQ_vault_db-odb.o: odb/CoinQ_vault_db-odb.cxx odb/CoinQ_vault_db-odb.hxx src/CoinQ_vault_db.hxx
	$(CXX) $(CXX_FLAGS) -o obj/CoinQ_vault_db-odb.o -c odb/CoinQ_vault_db-odb.cxx $(INCLUDE_PATH)

#
# vault class
#
obj/CoinQ_vault.o: src/CoinQ_vault.cpp src/CoinQ_vault.h odb/CoinQ_vault_db-odb.hxx
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(ODB_DB) $(INCLUDE_PATH)

#
# the remaining targets
#
obj/%.o: src/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

clean:
	-rm -f obj/*.o odb/*-odb.*
