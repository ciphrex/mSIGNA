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
	-mkdir -p $(SYSROOT)/include/stdutils
	-rsync -u src/*.h $(SYSROOT)/include/stdutils/

remove:
	-rm -rf $(SYSROOT)/include/stdutils
