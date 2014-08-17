ifndef SYSROOT
    ifneq ($(shell whoami), root)
        SYSROOT = $(PROJECT_SYSROOT)
    else
        SYSROOT = $(LOCAL_SYSROOT)
    endif
endif

ifneq ($(wildcard $(SYSROOT)/include),)
    INCLUDE_PATH += -I$(SYSROOT)/include
endif

ifneq ($(wildcard $(SYSROOT)/lib),)
    LIB_PATH += -L$(SYSROOT)/lib
endif

