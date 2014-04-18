ifndef SYSROOT
    SYSROOT = /usr/local
endif

all:

install:
	-mkdir -p $(SYSROOT)/include/Signals
	-rsync -u src/Signals.h $(SYSROOT)/include/Signals/

remove:
	-rm -rf $(SYSROOT)/include/Signals
