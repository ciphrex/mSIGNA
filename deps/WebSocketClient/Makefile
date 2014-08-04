CXXFLAGS += -Wall -O3

ifndef OS
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S), Linux)
        OS = linux
    else ifeq ($(UNAME_S), Darwin)
        OS = osx
    endif
endif

LOCAL_SYSROOT = ../../sysroot

WEBSOCKETPP = ../websocketpp

INCLUDE_PATH += \
    -Isrc \
    -I$(WEBSOCKETPP)

ifneq ($(wildcard $(LOCAL_SYSROOT)/include),)
    INCLUDE_PATH += -I$(LOCAL_SYSROOT)/include
endif

LIB_PATH += \
    -Llib

ifneq ($(wildcard $(LOCAL_SYSROOT)/lib),)
    LIB_PATH += -L$(LOCAL_SYSROOT)/lib
endif

ifeq ($(OS), linux)
    ifndef SYSROOT
        SYSROOT = /usr/local
    endif

    CXX = g++
    CC = gcc
    CXXFLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    ARCHIVER = ar

    PLATFORM_LIBS += \
        -lpthread

else ifeq ($(OS), mingw64)
    ifndef SYSROOT
        SYSROOT = /usr/x86_64-w64-mingw32
    endif

    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXXFLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""


    INCLUDE_PATH += -I$(SYSROOT)/include

    ARCHIVER = x86_64-w64-mingw32-ar

    BOOST_THREAD_SUFFIX = _win32
    BOOST_SUFFIX = -mt-s

    PLATFORM_LIBS += \
        -static-libgcc -static-libstdc++ \
        -lws2_32 \
        -lmswsock

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    ifndef SYSROOT
        SYSROOT = /usr/local
    endif

    CXX = clang++
    CC = clang
    CXXFLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    INCLUDE_PATH += -I$(SYSROOT)/include

    ARCHIVER = ar

    BOOST_SUFFIX = -mt

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

# libboost_random is needed by client
LIBS += \
    -lWebSocketClient \
    -lJsonRpc \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_random$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_THREAD_SUFFIX)$(BOOST_SUFFIX)

all: libs tests

libs: jsonrpc client

# JSON-RPC
jsonrpc: lib/libJsonRpc.a

lib/libJsonRpc.a: obj/JsonRpc.o
	$(ARCHIVER) rcs $@ $^

obj/JsonRpc.o: src/JsonRpc.cpp src/JsonRpc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) -c $< -o $@

# Client
client: jsonrpc lib/libWebSocketClient.a

lib/libWebSocketClient.a: obj/Client.o
	$(ARCHIVER) rcs $@ $^

obj/Client.o: src/Client.cpp src/Client.h
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) -c $< -o $@

tests: client_tests

# Client Tests
client_tests: tests/build/CoinSocketClientTest$(EXE_EXT) tests/build/RippleClientTest$(EXE_EXT)

tests/build/CoinSocketClientTest$(EXE_EXT): tests/src/CoinSocketClientTest.cpp lib/libWebSocketClient.a
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $(LIB_PATH) $< -o $@ $(LIBS) $(PLATFORM_LIBS)

tests/build/RippleClientTest$(EXE_EXT): tests/src/RippleClientTest.cpp lib/libWebSocketClient.a
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $(LIB_PATH) $< -o $@ $(LIBS) $(PLATFORM_LIBS)

install: install_jsonrpc install_client

install_jsonrpc:
	-mkdir -p $(SYSROOT)/include/WebSocketClient
	-rsync -u src/JsonRpc.h $(SYSROOT)/include/WebSocketClient/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/libJsonRpc.a $(SYSROOT)/lib/

install_client: install_jsonrpc
	-mkdir -p $(SYSROOT)/include/WebSocketClient
	-rsync -u src/Client.h $(SYSROOT)/include/WebSocketClient/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/libWebSocketClient.a $(SYSROOT)/lib/

remove:
	-rm -rf $(SYSROOT)/include/WebSocketClient
	-rm $(SYSROOT)/lib/libJsonRpc.a
	-rm $(SYSROOT)/lib/libWebSocketClient.a
	
clean:
	-rm -f obj/*.o lib/*.a tests/build/*Test
