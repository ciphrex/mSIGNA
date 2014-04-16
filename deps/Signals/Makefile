ifndef SYSROOT
    SYSROOT = /usr/local
endif

all:

install:
	-mkdir $(SYSROOT)/include/Signals
	-rsync -u src/Signals.h $(SYSROOT)/include/Signals/

remove:
	-rm -rf $(SYSROOT)/include/Signals
