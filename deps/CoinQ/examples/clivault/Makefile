COINCLASSES = ../../../CoinClasses
COINQ = ../../../CoinQ
SQLITE3 = ../../../sqlite3
CLI = ../../../cli
LOGGER = ../../../logger

ODB_DB = -DDATABASE_SQLITE

CXX_FLAGS = -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

INCLUDE_PATH = -I$(COINCLASSES)/src -I$(COINQ)/src -I$(COINQ)/odb -I$(CLI)/src -I$(LOGGER)/src
LIB_PATH =

$(warning $(OS))

ifeq ($(OS), linux)
    CXX = g++
    CC = gcc
    CXX_FLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    LIBS = \
        -l pthread \
        -l ssl \
        -l crypto \
        -l boost_system \
        -l boost_filesystem \
        -l boost_program_options \
        -l boost_regex \
        -l boost_thread \
        $(SQLITE3)/lib/libsqlite3.a \
        -l stdc++ \
	-l odb \
	-l odb-sqlite

    ARCHIVER = ar

else ifeq ($(OS), mingw64)
    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    MINGW64_ROOT = /opt/mingw64

    INCLUDE_PATH += -I$(MINGW64_ROOT)/include
    LIB_PATH += -L$(MINGW64_ROOT)/lib

    LIBS += \
	-static \
	-l gcc \
	-l stdc++ \
        -l ssl \
        -l crypto \
	-l gdi32 \
	-l ws2_32 \
	-l mswsock \
	-l boost_system-mt-s \
	-l boost_filesystem-mt-s \
	-l boost_program_options-mt-s \
	-l boost_regex-mt-s \
	-l boost_thread_win32-mt-s \
	$(SQLITE3)/lib/libsqlite3.a \
	-Wl,-Bdynamic \
	-l odb \
	-l odb-sqlite

    ARCHIVER = x86_64-w64-mingw32-ar

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    GLOBAL_LIB = /opt/local/lib
    INCLUDE_PATH += -I/opt/local/include

#    INCLUDE_PATH += -I$(BOOST_HOME)
#    LIB_PATH += -L$(BOOST_LIB)

    ifdef DYNAMIC
        LIBS = \
            -l boost_system \
            -l boost_filesystem \
            -l boost_thread \
            -l boost_regex \
            -l ssl \
            -l crypto \
            -l z \
            -l odb \
            -l odb-sqlite \
            -l pthread
    else
        LIBS = \
            /Users/Rey/dev/boost_1_54_0/stage/lib/libboost_system.a \
            /Users/Rey/dev/boost_1_54_0/stage/lib/libboost_filesystem.a \
            /Users/Rey/dev/boost_1_54_0/stage/lib/libboost_regex.a \
            /Users/Rey/dev/boost_1_54_0/stage/lib/libboost_thread.a \
            $(GLOBAL_LIB)/libssl.a \
            $(GLOBAL_LIB)/libcrypto.a \
            $(GLOBAL_LIB)/libz.a \
	    -l odb \
	    -l odb-sqlite \
            -l pthread
    endif

    ARCHIVER = ar

else ifeq ($(OS), cygwin)
    CXX_FLAGS += -Wno-unused-local-typedefs -std=c++0x

    ifdef DYNAMIC
        LIBS = \
            -l pthread \
            -l ssl \
            -l crypto \
            libboost_system-mt.dll.a \
            libboost_filesystem-mt.dll.a \
            libboost_program_options-mt.dll.a \
            libboost_regex-mt.dll.a \
            libboost_thread-mt.dll.a \
            libboost_date_time-mt.dll.a \
            -l sqlite3
            -l stdc++ 
    else
        LIBS = \
            -l pthread \
            -l ssl \
            -l crypto \
            -l boost_system-mt \
            -l boost_filesystem-mt \
            -l boost_program_options-mt \
            -l boost_regex-mt \
            -l boost_thread-mt \
            -l boost_date_time-mt \
            -l sqlite3 \
            -l stdc++
    endif
else
    $(error OS must be set to linux, osx, mingw64, or cygwin)
endif

LOGGER_OBJS = \
    $(LOGGER)/obj/logger.o

COINCLASSES_OBJS = \
    $(COINCLASSES)/obj/IPv6.o \
    $(COINCLASSES)/obj/CoinNodeData.o \
    $(COINCLASSES)/obj/CoinKey.o \
    $(COINCLASSES)/obj/hdkeys.o \
    $(COINCLASSES)/obj/BloomFilter.o \
    $(COINCLASSES)/obj/MerkleTree.o

COINQ_OBJS = \
    $(COINQ)/obj/CoinQ_blocks.o \
    $(COINQ)/obj/CoinQ_txs.o \
    $(COINQ)/obj/CoinQ_peer_io.o \
    $(COINQ)/obj/CoinQ_netsync.o \
    $(COINQ)/obj/CoinQ_keys.o \
    $(COINQ)/obj/CoinQ_script.o \
    $(COINQ)/obj/CoinQ_filter.o \
    $(COINQ)/obj/CoinQ_vault.o \
    $(COINQ)/obj/CoinQ_vault_db-odb.o

ALL_OBJS = $(LOGGER_OBJS) $(COINCLASSES_OBJS) $(COINQ_OBJS)

all: build/vault$(EXE_EXT)

obj/vault.o: src/vault.cpp
	$(CXX) $(INCLUDE_PATH) $(CXX_FLAGS) -o obj/vault.o -c src/vault.cpp $(ODB_DB)

build/vault$(EXE_EXT): obj/vault.o
	$(CXX) $(INCLUDE_PATH) $(CXX_FLAGS) -o $@ obj/vault.o $(ALL_OBJS) $(LIB_PATH) $(LIBS) $(ODB_DB)

clean:
	rm -f obj/*.o build/vault*
