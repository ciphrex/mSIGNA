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

    LOCAL_SYSROOT = /usr/local
    GLOBAL_SYSROOT = /usr

    PLATFORM_LIBS += \
        -lpthread

else ifeq ($(OS), mingw64)
    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""
    ARCHIVER = x86_64-w64-mingw32-ar
    EXE_EXT = .exe

    LOCAL_SYSROOT = /usr/x86_64-w64-mingw32/local
    GLOBAL_SYSROOT = /usr/x86_64-w64-mingw32

    PLATFORM_LIBS += \
        -static-libgcc -static-libstdc++ \
        -lgdi32 \
        -lws2_32 \
        -lmswsock

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7
    ARCHIVER = ar

    LOCAL_SYSROOT = /usr/local
    GLOBAL_SYSROOT = /usr

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

ifndef SYSROOT
    ifneq ($(shell whoami), root)
        ifdef PROJECT_SYSROOT
            SYSROOT = $(PROJECT_SYSROOT)
        else
            SYSROOT = $(LOCAL_SYSROOT)
        endif 
    else
        SYSROOT = $(LOCAL_SYSROOT)
    endif
endif

ifneq ($(wildcard $(SYSROOT)/include),)
    INCLUDE_PATH += -I$(SYSROOT)/include
endif

ifneq ($(wildcard $(SYSROOT)/lib),)
    LIB_PATH += -L$(SYSROOT)/lib
endif

