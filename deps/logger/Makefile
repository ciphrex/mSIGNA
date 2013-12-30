CXX = g++

CXX_FLAGS = -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif


obj/logger.o: src/logger.cpp src/logger.h
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

clean:
	rm -f obj/*.o
