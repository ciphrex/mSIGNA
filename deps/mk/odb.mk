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

ifdef SYSROOT
    ifeq ($(OS), mingw64)
        # ODB include path cannot include windows system headers when doing crossbuilds
        ODB_INCLUDE_PATH += -I$(SYSROOT)/local/include
    endif
endif

