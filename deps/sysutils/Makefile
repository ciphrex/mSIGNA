CXX_FLAGS += -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

LOCAL_SYSROOT = ../../sysroot

INCLUDE_PATH += \
    -Isrc

ifneq ($(wildcard $(LOCAL_SYSROOT)/include),)
    INCLUDE_PATH += -I$(LOCAL_SYSROOT)/include
endif

LIB_PATH += \
    -Llib

ifneq ($(wildcard $(LOCAL_SYSROOT)/lib),)
    LIB_PATH += -L$(LOCAL_SYSROOT)/lib
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
    ifndef SYSROOT
        SYSROOT = /usr/local
    endif

    CXX = g++
    CC = gcc
    CXX_FLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""
    ARCHIVER = ar

else ifeq ($(OS), mingw64)
    ifndef SYSROOT
        SYSROOT = /usr/x86_64-w64-mingw32
    endif

    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""
    ARCHIVER = x86_64-w64-mingw32-ar

    INCLUDE_PATH += -I$(SYSROOT)/include

    EXE_EXT = .exe

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

    INCLUDE_PATH += -I$(SYSROOT)/include

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

# Detect boost library filename suffix
ifneq ($(wildcard $(SYSROOT)/lib/libboost_system-mt.*),)
    BOOST_SUFFIX = -mt
else ifneq ($(wildcard $(SYSROOT)/lib/libboost_system-mt-s.*),)
    BOOST_SUFFIX = -mt-s
endif

LIBS = \
    -lsysutils \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_filesystem$(BOOST_SUFFIX) \

OBJS = \
    obj/filesystem.o

TESTS = \
    tests/build/filesystem$(EXE_EXT)

all: lib tests

lib: lib/libsysutils.a

obj/%.o: src/%.cpp src/%.h
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) -c $< -o $@

tests: $(TESTS)

tests/build/%$(EXE_EXT): tests/src/%test.cpp lib/libsysutils.a
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $< -o $@ $(LIB_PATH) $(LIBS) $(PLATFORM_LIBS)

lib/libsysutils.a: $(OBJS)
	$(ARCHIVER) rcs $@ $^

install: install_lib install_tools

install_lib:
	-mkdir -p $(SYSROOT)/include/sysutils
	-rsync -u src/*.h $(SYSROOT)/include/sysutils/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/libsysutils.a $(SYSROOT)/lib/

install_tools:
	-mkdir -p $(SYSROOT)/bin
	-rsync -u $(TOOLS) $(SYSROOT)/bin/

remove: remove_lib remove_tools

remove_lib:
	-rm -rf $(SYSROOT)/include/sysutils
	-rm $(SYSROOT)/lib/libsysutils.a

clean:
	-rm -f obj/*.o lib/*.a tools/build/*
