CXX_FLAGS = -Wall
ifdef DEBUG
    C_FLAGS += -g
    CXX_FLAGS += -g
else
    C_FLAGS += -03
    CXX_FLAGS += -O3
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
        obj/IPv6.o \
        obj/CoinNodeData.o \
        obj/CoinKey.o \
        obj/hdkeys.o \
        obj/BloomFilter.o \
        obj/MerkleTree.o \
        obj/StandardTransactions.o

SCRYPT_OBJS = \
	src/scrypt/obj/scrypt.o

HASH9_OBJS = \
	src/hashfunc/obj/blake.o \
	src/hashfunc/obj/bmw.o \
	src/hashfunc/obj/groestl.o \
	src/hashfunc/obj/jh.o \
	src/hashfunc/obj/keccak.o \
	src/hashfunc/obj/skein.o

all: lib/libCoinClasses.a $(OBJS) $(SCRYPT_OBJS) $(HASH9_OBJS)

lib/libCoinClasses.a: $(OBJS) $(SCRYPT_OBJS) $(HASH9_OBJS)
	$(ARCHIVER) rcs $@ $^

obj/%.o: src/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

src/scrypt/obj/scrypt.o: src/scrypt/scrypt.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

src/hashfunc/obj/%.o: src/hashfunc/%.c
	$(CC) $(C_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

clean:
	-rm -f obj/*.o lib/*.a
