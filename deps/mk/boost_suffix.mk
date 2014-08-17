# Detect boost library filename suffix
ifneq ($(wildcard $(LOCAL_SYSROOT)/lib/libboost_system-mt.*),)
    BOOST_SUFFIX = -mt
else ifneq ($(wildcard $(GLOBAL_SYSROOT)/lib/libboost_system-mt.*),)
    BOOST_SUFFIX = -mt
else ifneq ($(wildcard $(LOCAL_SYSROOT)/lib/libboost_system-mt-s.*),)
    BOOST_SUFFIX = -mt-s
else ifneq ($(wildcard $(GLOBAL_SYSROOT)/lib/libboost_system-mt-s.*),)
    BOOST_SUFFIX = -mt-s
endif

ifeq ($(OS), mingw64)
    BOOST_THREAD_SUFFIX = _win32
endif

