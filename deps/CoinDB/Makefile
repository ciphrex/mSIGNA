CLI = ../cli

INCLUDE_PATH += \
    -Isrc \
    -I$(CLI)/src

ODB_INCLUDE_PATH += \
    -Isrc

LIB_PATH += \
    -Llib

PROJECT_SYSROOT = ../../sysroot

include ../mk/os.mk ../mk/cxx_flags.mk ../mk/boost_suffix.mk ../mk/odb.mk

ifeq ($(OS), mingw64)
    CXX_FLAGS += -DLIBODB_STATIC_LIB
endif

LIBS = \
    -lCoinDB \
    -lCoinQ \
    -lCoinCore \
    -lsysutils \
    -llogger \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_filesystem$(BOOST_SUFFIX) \
    -lboost_program_options$(BOOST_SUFFIX) \
    -lboost_regex$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_THREAD_SUFFIX)$(BOOST_SUFFIX) \
    -lboost_serialization$(BOOST_SUFFIX) \
    -lcrypto \
    -lodb-$(DB) \
    -lodb \
    $(DB_LIBS)

OBJS = \
    obj/Schema-odb-$(DB).o \
    obj/Schema.o \
    obj/Vault.o \
    obj/SynchedVault.o

TOOLS = \
    tools/coindb/build/coindb$(EXE_EXT) \
    tools/syncdb/build/syncdb$(EXE_EXT) \
    tools/multibip32/build/multibip32$(EXE_EXT) \
    tools/signbip32/build/signbip32$(EXE_EXT)

all: lib tools

lib: lib/libCoinDB.a

tools: coindb syncdb multibip32 signbip32

lib/libCoinDB.a: $(OBJS)
	$(ARCHIVER) rcs $@ $^

#
# odb compiled dependencies
#
odb/Schema-odb-$(DB).hxx: src/Schema.h
	odb --output-dir odb/ --std c++11 $(ODB_DB) --database $(DB) --odb-file-suffix -odb-$(DB) --changelog-file-suffix -$(DB) --generate-query --generate-schema --schema-format embedded $(ODB_INCLUDE_PATH) $<

odb/Schema-odb-$(DB).cxx: odb/Schema-odb-$(DB).hxx

#
# odb generated schema object
#
obj/Schema-odb-$(DB).o: odb/Schema-odb-$(DB).cxx odb/Schema-odb-$(DB).hxx src/Schema.h
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

#
# schema classes
#
obj/Schema.o: src/Schema.cpp src/Schema.h
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

#
# vault class
#
obj/Vault.o: src/Vault.cpp src/Vault.h src/VaultExceptions.h src/SigningRequest.h src/SignatureInfo.h src/Schema.h src/Database.h odb/Schema-odb-$(DB).hxx
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

#
# synched vault class
#
obj/SynchedVault.o: src/SynchedVault.cpp src/SynchedVault.h src/VaultExceptions.h src/SigningRequest.h src/Schema.h src/Database.h odb/Schema-odb-$(DB).hxx
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

#
# coindb command line tool
#
coindb: lib tools/coindb/build/coindb$(EXE_EXT)

tools/coindb/build/coindb$(EXE_EXT): tools/coindb/src/coindb.cpp tools/coindb/src/formatting.h src/CoinDBConfig.h lib/libCoinDB.a
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) $< -o $@ $(LIB_PATH) $(LIBS) $(PLATFORM_LIBS)

#
# syncdb command line tool
#
syncdb: lib tools/syncdb/build/syncdb$(EXE_EXT)

tools/syncdb/build/syncdb$(EXE_EXT): tools/syncdb/src/syncdb.cpp src/CoinDBConfig.h lib/libCoinDB.a
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) $< -o $@ $(LIB_PATH) $(LIBS) $(PLATFORM_LIBS)

#
# multibip32 command line tool
#
multibip32: tools/multibip32/build/multibip32$(EXE_EXT)

tools/multibip32/build/multibip32$(EXE_EXT): tools/multibip32/src/multibip32.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $< -o $@ $(LIB_PATH) $(LIBS) $(PLATFORM_LIBS)

#
# signbip32 command line tool
#
signbip32: tools/signbip32/build/signbip32$(EXE_EXT)

tools/signbip32/build/signbip32$(EXE_EXT): tools/signbip32/src/signbip32.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) $< -o $@ $(LIB_PATH) $(LIBS) $(PLATFORM_LIBS)

install: install_lib install_tools

install_lib:
	-mkdir -p $(SYSROOT)/include/CoinDB
	-rsync -u src/*.h $(SYSROOT)/include/CoinDB/
	-mkdir -p $(SYSROOT)/lib
	-rsync -u lib/libCoinDB.a $(SYSROOT)/lib/

install_tools:
	-mkdir -p $(SYSROOT)/bin
	-rsync -u $(TOOLS) $(SYSROOT)/bin/

remove: remove_lib remove_tools

remove_lib:
	-rm -rf $(SYSROOT)/include/CoinDB
	-rm $(SYSROOT)/lib/libCoinDB.a

remove_tools:
	-rm $(SYSROOT)/bin/coindb$(EXE_EXT)
	-rm $(SYSROOT)/bin/syncdb$(EXE_EXT)
	-rm $(SYSROOT)/bin/multibip32$(EXE_EXT)
	-rm $(SYSROOT)/bin/signbip32$(EXE_EXT)

clean: clean_lib

clean_all: clean_lib clean_tools

clean_lib:
	-rm -f obj/*.o odb/*-odb*.* lib/*.a

clean_tools:
	-rm -f $(TOOLS)
