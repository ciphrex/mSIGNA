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

