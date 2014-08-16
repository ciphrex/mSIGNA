#use sqlite by default
ifndef DB
    DB = sqlite
endif

ifeq ($(DB), mysql)
    ODB_DB = -DDATABASE_MYSQL
else ifeq ($(DB), sqlite)
    ODB_DB = -DDATABASE_SQLITE
else
    $(error DB must be set to sqlite or mysql)
endif

ifdef LOCAL_SYSROOT
    ODB_INCLUDE_PATH += -I$(LOCAL_SYSROOT)/include
endif

ifdef GLOBAL_SYSROOT
    ifeq ($(OS), mingw64)
        # ODB include path cannot include windows system headers when doing crossbuilds
        ODB_INCLUDE_PATH += -I$(GLOBAL_SYSROOT)/local/include
    else
        ODB_INCLUDE_PATH += -I$(GLOBAL_SYSROOT)/include
    endif
endif

