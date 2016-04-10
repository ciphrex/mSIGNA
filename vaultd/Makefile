ODB_DB = -DDATABASE_SQLITE

LOGGER_DIR = ../deps/logger
COINCLASSES_DIR = ../deps/CoinClasses
COINQ_DIR = ../deps/CoinQ
COINDB_DIR = ../deps/CoinDB
CLI_DIR = ../deps/cli

INCLUDE_PATH += \
    -I$(COINDB_DIR)/src \
    -I$(COINDB_DIR)/odb \
    -I$(COINDB_DIR)/tools/src \
    -I$(COINQ_DIR)/src \
    -I$(COINCLASSES_DIR)/src \
    -I$(LOGGER_DIR)/src \
    -I$(CLI_DIR)/src

LIB_PATH += \
    -L$(COINDB_DIR)/lib \
    -L$(COINQ_DIR)/lib \
    -L$(COINCLASSES_DIR)/lib \
    -L$(LOGGER_DIR)/lib

CXXFLAGS += -Wall -O3

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
    CXXFLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    ARCHIVER = ar

else ifeq ($(OS), mingw64)
    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXXFLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    MINGW64_ROOT = /usr/x86_64-w64-mingw32

    INCLUDE_PATH += -I$(MINGW64_ROOT)/include

    ARCHIVER = x86_64-w64-mingw32-ar

    BOOST_THREAD_SUFFIX = _win32
    BOOST_SUFFIX = -mt-s

    PLATFORM_LIBS += \
        -static-libgcc -static-libstdc++ \
        -lws2_32 \
        -lmswsock

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXXFLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    INCLUDE_PATH += -I/usr/local/include

    ARCHIVER = ar

    BOOST_SUFFIX = -mt

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

LIBS += \
    -lCoinDB \
    -lCoinQ \
    -lCoinClasses \
    -lWebSocketServer \
    -llogger \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_filesystem$(BOOST_SUFFIX) \
    -lboost_regex$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_THREAD_SUFFIX)$(BOOST_SUFFIX) \
    -lboost_serialization$(BOOST_SUFFIX) \
    -lcrypto \
    -lodb-sqlite \
    -lodb

all: build/vaultd${EXE_EXT}

build/vaultd${EXE_EXT}: src/main.cpp
	$(CXX) $(CXXFLAGS) $(ODB_DB) $(INCLUDE_PATH) $(LIB_PATH) $< -o $@ $(LIBS)

clean:
	-rm -f build/vaultd${EXE_EXT}

