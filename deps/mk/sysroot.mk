ifndef SYSROOT
    ifneq ($(shell whoami), root)
        SYSROOT = $(LOCAL_SYSROOT)
    endif
endif

ifdef SYSROOT
    ifneq ($(wildcard $(SYSROOT)/include),)
        INCLUDE_PATH += -I$(SYSROOT)/include
    endif

    ifneq ($(wildcard $(SYSROOT)/lib),)
        LIB_PATH += -L$(SYSROOT)/lib
    endif
endif

