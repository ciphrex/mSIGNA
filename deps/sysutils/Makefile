INCLUDE_PATH += -Isrc
LIB_PATH += -Llib
PROJECT_SYSROOT = ../../sysroot

include ../mk/os.mk ../mk/cxx_flags.mk ../mk/boost_suffix.mk

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

install: install_lib

install_lib:
	-mkdir -p $(SYSROOT)/include/sysutils
	-rsync -u src/*.h $(SYSROOT)/include/sysutils/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/libsysutils.a $(SYSROOT)/lib/

remove: remove_lib

remove_lib:
	-rm -rf $(SYSROOT)/include/sysutils
	-rm $(SYSROOT)/lib/libsysutils.a

clean:
	-rm -f obj/*.o lib/*.a tools/build/*
