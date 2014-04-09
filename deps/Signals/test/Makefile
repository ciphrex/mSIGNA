SIGNALS_ROOT = ..
INCLUDEPATH = -I${SIGNALS_ROOT}/src

CXX = clang++
CXXFLAGS += -O2 -std=c++11 -stdlib=libc++

build/test: test.cpp ${SIGNALS_ROOT}/src/Signals.h
	$(CXX) ${CXXFLAGS} ${INCLUDEPATH} $< -o $@

clean:
	-rm build/test

