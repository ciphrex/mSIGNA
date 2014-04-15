CXX_FLAGS += -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

LOCAL_SYSROOT = ../../sysroot

ifndef SYSROOT
    SYSROOT = /usr/local
endif

INCLUDE_PATH += \
    -Isrc

ifneq ($(wildcard $(LOCAL_SYSROOT)/include),)
    INCLUDE_PATH += -I$(LOCAL_SYSROOT)/include
endif

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

    ARCHIVER = x86_64-w64-mingw32-ar

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    INCLUDE_PATH += -I/usr/local/include

    ARCHIVER = ar

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

OBJS = \
    obj/CoinQ_script.o \
    obj/CoinQ_peer_io.o \
    obj/CoinQ_netsync.o \
    obj/CoinQ_blocks.o \
    obj/CoinQ_txs.o \
    obj/CoinQ_keys.o \
    obj/CoinQ_filter.o

all: lib/libCoinQ.a

lib/libCoinQ.a: $(OBJS)
	$(ARCHIVER) rcs $@ $^

obj/%.o: src/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

install:
	-mkdir -p $(SYSROOT)/include/CoinQ
	-cp src/*.h  $(SYSROOT)/include/CoinQ/
	-mkdir -p $(SYSROOT)/lib
	-cp lib/libCoinQ.a $(SYSROOT)/lib/

remove:
	-rm -rf $(SYSROOT)/include/CoinQ
	-rm $(SYSROOT)/lib/libCoinQ.a

clean:
	-rm -f obj/*.o lib/*.a
