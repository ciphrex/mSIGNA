ifndef SYSROOT
    ifneq ($(shell whoami), root)
        SYSROOT = $(LOCAL_SYSROOT)
    endif
endif

