ifndef DB
    # Detect db library filename suffix
    ifneq ($(wildcard $(LOCAL_SYSROOT)/lib/libodb-sqlite.*),)
        DB = sqlite
    else ifneq ($(wildcard $(GLOBAL_SYSROOT)/lib/libodb-sqlite.*),)
        DB = sqlite
    else ifneq ($(wildcard $(LOCAL_SYSROOT)/lib/libodb-mysql.*),)
        DB = mysql
    else ifneq ($(wildcard $(GLOBAL_SYSROOT)/lib/libodb-mysql.*),)
        DB = mysql
    endif
endif

ifeq ($(DB), mysql)
    ODB_DB = -DDATABASE_MYSQL
else ifeq ($(DB), sqlite)
    ODB_DB = -DDATABASE_SQLITE
    DB_LIBS = -lsqlite3
else
    $(error DB must be set to sqlite or mysql)
endif

ifdef SYSROOT
    ODB_INCLUDE_PATH += -I$(SYSROOT)/include
endif

ifdef LOCAL_SYSROOT
    ODB_INCLUDE_PATH += -I$(LOCAL_SYSROOT)/include
endif

#ifdef GLOBAL_SYSROOT
#    ifeq ($(OS), mingw64)
#        # ODB include path cannot include windows system headers when doing crossbuilds
#        ODB_INCLUDE_PATH += -I$(GLOBAL_SYSROOT)/local/include
#    else
#        ODB_INCLUDE_PATH += -I$(GLOBAL_SYSROOT)/include
#    endif
#endif

