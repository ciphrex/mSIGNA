PROJECT_SYSROOT = ../../../../sysroot

include ../../../mk/os.mk ../../../mk/cxx_flags.mk

INCLUDE_PATH += \
    -I../../src

OBJS = \
    ../../obj/secp256k1_openssl.o

LIBS = \
    -lcrypto

EXES = \
    build/secp256k1_keygen${EXE_EXT} \
    build/secp256k1_test${EXE_EXT} \
    build/secp256k1_rfc6979_test${EXE_EXT} \
    build/secp256k1_verify${EXE_EXT} \
    build/ascii2hex${EXE_EXT}

all: $(EXES) 

build/secp256k1_keygen${EXE_EXT}: src/secp256k1_keygen.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/secp256k1_test${EXE_EXT}: src/secp256k1_test.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/secp256k1_rfc6979_test${EXE_EXT}: src/secp256k1_rfc6979_test.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/secp256k1_verify${EXE_EXT}: src/secp256k1_verify.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

build/ascii2hex${EXE_EXT}: src/ascii2hex.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $^ -o $@ $(LIBS)

../../obj/secp256k1_openssl.o: ../../src/secp256k1_openssl.cpp ../../src/secp256k1_openssl.h
	$(CXX) $(CXX_FLAGS) -DTRACE_RFC6979 $(INCLUDE_PATH) -c $< -o $@

clean:
	-rm -f build/*
