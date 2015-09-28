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
    CXX_FLAGS += -Wno-unknown-pragmas -std=c++0x -fvisibility=hidden -fvisibility-inlines-hidden -DBOOST_SYSTEM_NOEXCEPT=""
    ARCHIVER = ar

    LOCAL_SYSROOT = /usr/local
    GLOBAL_SYSROOT = /usr

    PLATFORM_LIBS += \
        -lpthread

else ifeq ($(OS), mingw64)
    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -fvisibility=hidden -fvisibility-inlines-hidden -DBOOST_SYSTEM_NOEXCEPT="" -DLIBODB_SQLITE_STATIC_LIB -DLIBODB_STATIC_LIB
    ARCHIVER = x86_64-w64-mingw32-ar
    EXE_EXT = .exe

    LOCAL_SYSROOT = /usr/x86_64-w64-mingw32/local
    GLOBAL_SYSROOT = /usr/x86_64-w64-mingw32

    PLATFORM_LIBS += \
        -L/usr/x86_64-w64-mingw32/lib \
        -L/usr/x86_64-w64-mingw32/plugins/platforms \
        -L/usr/x86_64-w64-mingw32/plugins/imageformats \
        -L/usr/x86_64-w64-mingw32/plugins/bearer \
        -static-libgcc -static-libstdc++ -static \
        -lgdi32 \
        -lws2_32 \
        -lmswsock \
        -lpthread

else ifeq ($(OS), mingw32)
    CXX =  i686-w64-mingw32-g++
    CC =  i686-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -fvisibility=hidden -fvisibility-inlines-hidden -DBOOST_SYSTEM_NOEXCEPT="" -DLIBODB_SQLITE_STATIC_LIB -DLIBODB_STATIC_LIB
    ARCHIVER = i686-w64-mingw32-ar
    EXE_EXT = .exe

    LOCAL_SYSROOT = /usr/i686-w64-mingw32/local
    GLOBAL_SYSROOT = /usr/i686-w64-mingw32

    PLATFORM_LIBS += \
        -L/usr/i686-w64-mingw32/lib \
        -L/usr/i686-w64-mingw32/plugins/platforms \
        -L/usr/i686-w64-mingw32/plugins/imageformats \
        -L/usr/i686-w64-mingw32/plugins/bearer \
        -static-libgcc -static-libstdc++ -static \
        -lgdi32 \
        -lws2_32 \
        -lmswsock \
        -lpthread

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7
    ARCHIVER = ar

    LOCAL_SYSROOT = /usr/local
    GLOBAL_SYSROOT = /usr

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, mingw32, or osx)
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

