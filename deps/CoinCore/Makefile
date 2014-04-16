CXX_FLAGS += -Wall
ifdef DEBUG
    C_FLAGS += -g
    CXX_FLAGS += -g
else
    C_FLAGS += -O3
    CXX_FLAGS += -O3
endif

LOCAL_SYSROOT = ../../sysroot

ifndef SYSROOT
    SYSROOT = /usr/local
endif

INCLUDE_PATH += \
    -I$(LOCAL_SYSROOT)/include

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
        obj/secp256k1.o \
        obj/StandardTransactions.o

OBJ_HEADERS = \
        src/Base58Check.h \
        src/BigInt.h \
        src/aes.h \
        src/encodings.h \
        src/hash.h \
        src/hashblock.h \
        src/jsonResult.h \
        src/numericdata.h \
        src/random.h \
        src/typedefs.h \
        src/uint256.h

SCRYPT_OBJS = \
	src/scrypt/obj/scrypt.o

HASH9_OBJS = \
	src/hashfunc/obj/blake.o \
	src/hashfunc/obj/bmw.o \
	src/hashfunc/obj/groestl.o \
	src/hashfunc/obj/jh.o \
	src/hashfunc/obj/keccak.o \
	src/hashfunc/obj/skein.o

all: lib/libCoinCore.a $(OBJS) $(SCRYPT_OBJS) $(HASH9_OBJS)

lib/libCoinCore.a: $(OBJS) $(SCRYPT_OBJS) $(HASH9_OBJS)
	$(ARCHIVER) rcs $@ $^

obj/%.o: src/%.cpp src/%.h $(OBJ_HEADERS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) -c $< -o $@

src/scrypt/obj/scrypt.o: src/scrypt/scrypt.cpp src/scrypt/scrypt.h
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) -c $< -o $@

src/hashfunc/obj/%.o: src/hashfunc/%.c src/hashfunc/sph_%.h src/hashfunc/sph_types.h
	$(CC) $(C_FLAGS) $(INCLUDE_PATH) -c $< -o $@

install:
	-mkdir -p $(SYSROOT)/include/CoinCore
	-rsync -u src/*.h $(SYSROOT)/include/CoinCore/
	-mkdir -p $(SYSROOT)/include/CoinCore/hashfunc
	-rsync -u src/hashfunc/*.h $(SYSROOT)/include/CoinCore/hashfunc/
	-mkdir -p $(SYSROOT)/include/CoinCore/scrypt
	-rsync -u src/scrypt/*.h $(SYSROOT)/include/CoinCore/scrypt/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/libCoinCore.a $(SYSROOT)/lib/

remove:
	-rm -rf $(SYSROOT)/include/CoinCore
	-rm $(SYSROOT)/lib/libCoinCore.a

clean:
	-rm -f obj/*.o lib/*.a src/scrypt/obj/*.o src/hashfunc/obj/*.o
