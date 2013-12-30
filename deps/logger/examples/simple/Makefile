CXX = g++
LOGGER_PATH = ../..

build/simple: src/main.cpp $(LOGGER_PATH)/obj/logger.o
	$(CXX) src/main.cpp $(LOGGER_PATH)/obj/logger.o -o build/simple -I$(LOGGER_PATH)/src

$(LOGGER_PATH)/obj/logger.o: $(LOGGER_PATH)/src/logger.cpp $(LOGGER_PATH)/src/logger.h
	$(CXX) -c -o $@ $< -I$(LOGGER_PATH)/src

clean:
	rm -f build/simple

clean-all:
	rm -f build/simple $(LOGGER_PATH)/obj/*.o
