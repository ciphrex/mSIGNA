# Detect boost library filename suffix
ifneq ($(wildcard $(SYSROOT)/lib/libboost_system-mt.*),)
    BOOST_SUFFIX = -mt
else ifneq ($(wildcard $(SYSROOT)/lib/libboost_system-mt-s.*),)
    BOOST_SUFFIX = -mt-s
endif

