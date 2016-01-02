PROJECT_SYSROOT = ../../sysroot

include ../mk/os.mk ../mk/cxx_flags.mk ../mk/c_flags.mk

OBJS = \
        obj/IPv6.o \
        obj/CoinNodeData.o \
        obj/CoinKey.o \
        obj/hdkeys.o \
        obj/bip39.o \
        obj/BloomFilter.o \
        obj/MerkleTree.o \
        obj/secp256k1_openssl.o \
        obj/aes.o

OBJ_HEADERS = \
        src/Base58Check.h \
        src/BigInt.h \
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
