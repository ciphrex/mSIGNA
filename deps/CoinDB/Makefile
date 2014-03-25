ODB_DB = \
    -DDATABASE_SQLITE

CXX_FLAGS += -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

LOGGER = ../logger
COINCLASSES = ../CoinClasses
COINQ = ../CoinQ
CLI = ../cli

INCLUDE_PATH = \
    -Isrc \
    -I$(COINQ)/src \
    -I$(COINCLASSES)/src \
    -I$(LOGGER)/src \
    -I$(CLI)/src

ODB_INCLUDE_PATH = \
    -Isrc \
    -I$(COINQ)/src \
    -I$(COINCLASSES)/src \
    -I$(LOGGER)/src

TOOLS_INCLUDE_PATH = \
    $(INCLUDE_PATH) \
    -Iodb


ifndef OS
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S), Linux)
        OS = linux
    else ifeq ($(UNAME_S), Darwin)
        OS = osx
    endif
endif

ifeq ($(OS), linux)
    CXX = g++
    CC = gcc
    CXX_FLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    ARCHIVER = ar

else ifeq ($(OS), mingw64)
    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    MINGW64_ROOT = /usr/x86_64-w64-mingw32

    INCLUDE_PATH += -I$(MINGW64_ROOT)/include

    # ODB_INCLUDE_PATH must not contain windows system headers
    ODB_INCLUDE_PATH += -I$(MINGW64_ROOT)/local/include

    ARCHIVER = x86_64-w64-mingw32-ar

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    INCLUDE_PATH += -I/usr/local/include
    ODB_INCLUDE_PATH += -I/usr/local/include

    BOOST_SUFFIX = -mt

    ARCHIVER = ar

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

TOOLS_LIBPATH = \
    -Llib -lCoinDB \
    -L$(COINQ)/lib -lCoinQ \
    -L$(COINCLASSES)/lib -lCoinClasses \
    -L$(LOGGER)/lib -llogger \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_filesystem$(BOOST_SUFFIX) \
    -lboost_regex$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_SUFFIX) \
    -lcrypto \
    -lodb-sqlite \
    -lodb

OBJS = \
    obj/Schema-odb.o \
    obj/Vault.o


all: lib tools

lib: lib/libCoinDB.a

tools: tools/build/coindb$(EXE_EXT)

lib/libCoinDB.a: $(OBJS)
	$(ARCHIVER) rcs $@ $^

#
# odb compiled dependencies
#
odb/Schema-odb.hxx: src/Schema.hxx
	odb --output-dir odb/ --std c++11 -d sqlite --generate-query --generate-schema $(ODB_INCLUDE_PATH) $<

#odb/Schema-odb.cxx: src/Schema.hxx
#	odb --output-dir odb/ --std c++11 -d sqlite --generate-query --generate-schema $< $(ODB_INCLUDE_PATH)
odb/Schema-odb.cxx: odb/Schema-odb.hxx

#
# schema
#
obj/Schema-odb.o: odb/Schema-odb.cxx odb/Schema-odb.hxx src/Schema.hxx
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

#
# vault class
#
obj/Vault.o: src/Vault.cpp src/Vault.h src/VaultExceptions.h src/SigningRequest.h src/Schema.hxx odb/Schema-odb.hxx src/Database.hxx
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

#
# coindb command line tool
#
tools/build/coindb$(EXE_EXT): tools/src/coindb.cpp lib/libCoinDB.a
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(TOOLS_INCLUDE_PATH) $< $(TOOLS_LIBPATH) -o $@

clean:
	-rm -f obj/*.o odb/*-odb.* lib/*.a tools/build/*
