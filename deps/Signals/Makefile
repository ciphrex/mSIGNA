LOCAL_SYSROOT = ../../sysroot
 
ifeq ($(OS), mingw64)
    ifndef SYSROOT
        SYSROOT = /usr/x86_64-w64-mingw32
    endif
else
    ifndef SYSROOT
        ifeq ($(shell whoami), root)
            SYSROOT = /usr/local
        else
            SYSROOT = $(LOCAL_SYSROOT)
        endif
    endif
endif

all:

install:
	-mkdir -p $(SYSROOT)/include/Signals
	-rsync -u src/Signals.h $(SYSROOT)/include/Signals/
	-rsync -u src/SignalQueue.h $(SYSROOT)/include/Signals/

remove:
	-rm -rf $(SYSROOT)/include/Signals
