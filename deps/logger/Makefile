PROJECT_SYSROOT = ../../sysroot

include ../mk/os.mk ../mk/cxx_flags.mk

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
