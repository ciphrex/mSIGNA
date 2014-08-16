ifndef OS
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S), Linux)
        OS = linux
    else ifeq ($(UNAME_S), Darwin)
        OS = osx
    endif
endif

ifeq ($(OS), linux)
    ifndef SYSROOT
        SYSROOT = /usr/local
    endif

    CXX = g++
    CC = gcc
    CXX_FLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""
    ARCHIVER = ar

    PLATFORM_LIBS += \
        -lpthread

else ifeq ($(OS), mingw64)
    ifndef SYSROOT
        SYSROOT = /usr/x86_64-w64-mingw32
    endif

    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""
    ARCHIVER = x86_64-w64-mingw32-ar
    EXE_EXT = .exe
    BOOST_THREAD_SUFFIX = _win32

    PLATFORM_LIBS += \
        -static-libgcc -static-libstdc++ \
        -lgdi32 \
        -lws2_32 \
        -lmswsock

else ifeq ($(OS), osx)
    ifndef SYSROOT
        SYSROOT = /usr/local
    endif

    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7
    ARCHIVER = ar

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

