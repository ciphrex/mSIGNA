LOCAL_SYSROOT = ../../sysroot

CXX_FLAGS += -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
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
        ifeq ($(shell whoami), root)
            SYSROOT = /usr/local
        else
            SYSROOT = $(LOCAL_SYSROOT)
        endif
    endif

    CXX = g++
    ARCHIVER = ar
else ifeq ($(OS), mingw64)
    ifndef SYSROOT
        SYSROOT = /usr/x86_64-w64-mingw32
    endif

    CXX = x86_64-w64-mingw32-g++
    ARCHIVER = x86_64-w64-mingw32-ar
else ifeq ($(OS), osx)
    ifndef SYSROOT
        ifeq ($(shell whoami), root)
            SYSROOT = /usr/local
        else
            SYSROOT = $(LOCAL_SYSROOT)
        endif
    endif

    CXX = clang++
    ARCHIVER = ar
    CXX_FLAGS += -std=c++11 -stdlib=libc++
else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

all: lib/liblogger.a

lib/liblogger.a: obj/logger.o
	$(ARCHIVER) rcs $@ $^

obj/logger.o: src/logger.cpp src/logger.h
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

install:
	-mkdir -p $(SYSROOT)/include/logger
	-rsync -u src/logger.h $(SYSROOT)/include/logger/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/liblogger.a $(SYSROOT)/lib/

remove:
	-rm -rf $(SYSROOT)/include/logger
	-rm $(SYSROOT)/lib/liblogger.a

clean:
	rm -f obj/*.o lib/*.a/
