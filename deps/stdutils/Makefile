PROJECT_SYSROOT = ../../sysroot

include ../mk/os.mk

all:

install:
	-mkdir -p $(SYSROOT)/include/stdutils
	-rsync -u src/*.h $(SYSROOT)/include/stdutils/

remove:
	-rm -rf $(SYSROOT)/include/stdutils
