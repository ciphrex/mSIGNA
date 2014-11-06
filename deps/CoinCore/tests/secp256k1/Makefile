PROJECT_SYSROOT = ../../../../sysroot

include ../../../mk/os.mk ../../../mk/cxx_flags.mk

INCLUDE_PATH += \
    -I../../src

OBJS = \
    ../../obj/secp256k1.o

LIBS = \
    -lcrypto

all: build/secp256k1_test${EXE_EXT} build/secp256k1_rfc6979_test${EXE_EXT} build/ascii2hex${EXE_EXT}

build/secp256k1_test${EXE_EXT}: src/secp256k1_test.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/secp256k1_rfc6979_test${EXE_EXT}: src/secp256k1_rfc6979_test.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/ascii2hex${EXE_EXT}: src/ascii2hex.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)
