CXX_FLAGS = -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

ifeq ($(OS), linux)
    CXX = g++
    ARCHIVER = ar
else ifeq ($(OS), mingw64)
    CXX = x86_64-w64-mingw32-g++
    ARCHIVER = x86_64-w64-mingw32-ar
else ifeq ($(OS), osx)
    CXX = clang++
    ARCHIVER = ar
    CXX_FLAGS += -std=c++11 -stdlib=libc++
else
    $(error OS must be set to linux, osx, or mingw64)
endif

all: lib/liblogger.a

lib/liblogger.a: obj/logger.o
	$(ARCHIVER) rcs $@ $^

obj/logger.o: src/logger.cpp src/logger.h
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

clean:
	rm -f obj/*.o
