CXX_FLAGS += -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

ifndef SYSROOT
    SYSROOT = /usr/local
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

    PLATFORM_LIBS += \
        -lpthread

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

    BOOST_SUFFIX = -mt

    ARCHIVER = ar

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

LIBS = \
    -lCoinQ \
    -lCoinCore \
    -llogger \
    -lsysutils \
    -lboost_regex$(BOOST_SUFFIX) \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_SUFFIX) \
    -lboost_program_options$(BOOST_SUFFIX) \
    -lcrypto

build/peer: src/main.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $< -o $@ $(LIBS) $(PLATFORM_LIBS)

clean:
	-rm build/*
