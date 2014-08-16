LOCAL_SYSROOT = ../../sysroot
include ../mk/sysroot.mk ../mk/os.mk

all:

install:
	-mkdir -p $(SYSROOT)/include/stdutils
	-rsync -u src/*.h $(SYSROOT)/include/stdutils/

remove:
	-rm -rf $(SYSROOT)/include/stdutils
