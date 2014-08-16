LOCAL_SYSROOT = ../../sysroot
include ../mk/sysroot.mk ../mk/os.mk

all:

install:
	-mkdir -p $(SYSROOT)/include/Signals
	-rsync -u src/Signals.h $(SYSROOT)/include/Signals/
	-rsync -u src/SignalQueue.h $(SYSROOT)/include/Signals/

remove:
	-rm -rf $(SYSROOT)/include/Signals
