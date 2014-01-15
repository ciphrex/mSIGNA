CXX_FLAGS = -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
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

    MINGW64_ROOT = /opt/mingw64

    INCLUDE_PATH += -I$(MINGW64_ROOT)/include

    ARCHIVER = x86_64-w64-mingw32-ar

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    GLOBAL_LIB = /opt/local/lib
    INCLUDE_PATH += -I/opt/local/include

    ARCHIVER = ar

else ifeq ($(OS), cygwin)
    CXX_FLAGS += -Wno-unused-local-typedefs -std=c++0x

else
    $(error OS must be set to linux, osx, mingw64, or cygwin)
endif

OBJS = \
        obj/IPv6.o \
        obj/CoinNodeData.o \
        obj/CoinKey.o \
        obj/hdkeys.o \
        obj/BloomFilter.o \
        obj/MerkleTree.o \
        obj/StandardTransactions.o


obj/%.o: src/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

src/scrypt/obj/scrypt.o: src/scrypt/scrypt.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE_PATH)

all: $(OBJS) src/scrypt/obj/scrypt.o

clean:
	-rm -f obj/*.o
