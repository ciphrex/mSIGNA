#######################################
# coinvault.pro
#
# Copyright (c) 2013 Eric Lombrozo
#
# All Rights Reserved.

# Application icons for windows
RC_FILE = res/coinvault.rc

# Application icons for mac
ICON = res/icons/app_icons/osx.icns

DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE
CONFIG += c++11 rtti thread

QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas

INCLUDEPATH += \
    /usr/local/include \
    deps/logger/src \
    deps/CoinQ/src \
    deps/CoinClasses/src

QT += widgets network

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
} else {
    DESTDIR = build/release
}

OBJECTS_DIR = $$DESTDIR/obj
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui

FORMS = \
    src/acceptlicensedialog.ui \
    src/requestpaymentdialog.ui

HEADERS = \
    src/settings.h \
    src/filesystem.h \
    src/versioninfo.h \
    src/copyrightinfo.h \
    src/coinparams.h \
    src/severitylogger.h \
    src/splashscreen.h \
    src/acceptlicensedialog.h \
    src/mainwindow.h \
    src/commandserver.h \
    src/paymentrequest.h \
    src/numberformats.h \
    src/accountmodel.h \
    src/accountview.h \
    src/keychainmodel.h \
    src/keychainview.h \
    src/quicknewaccountdialog.h \
    src/newaccountdialog.h \
    src/newkeychaindialog.h \
    src/rawtxdialog.h \
    src/createtxdialog.h \
    src/txmodel.h \
    src/txview.h \
    src/accounthistorydialog.h \
    src/txactions.h \
    src/scriptmodel.h \
    src/scriptview.h \
    src/scriptdialog.h \
    src/requestpaymentdialog.h \
    src/networksettingsdialog.h \
    src/keychainbackupdialog.h \
    src/resyncdialog.h
                
SOURCES = \
    src/settings.cpp \
    src/filesystem.cpp \
    src/coinparams.cpp \
    src/main.cpp \
    src/splashscreen.cpp \
    src/acceptlicensedialog.cpp \
    src/mainwindow.cpp \
    src/commandserver.cpp \
    src/paymentrequest.cpp \
    src/numberformats.cpp \
    src/accountmodel.cpp \
    src/accountview.cpp \
    src/keychainmodel.cpp \
    src/keychainview.cpp \
    src/quicknewaccountdialog.cpp \
    src/newaccountdialog.cpp \
    src/newkeychaindialog.cpp \
    src/rawtxdialog.cpp \
    src/createtxdialog.cpp \
    src/txmodel.cpp \
    src/txview.cpp \
    src/accounthistorydialog.cpp \
    src/txactions.cpp \
    src/scriptmodel.cpp \
    src/scriptview.cpp \
    src/scriptdialog.cpp \
    src/requestpaymentdialog.cpp \
    src/networksettingsdialog.cpp \
    src/keychainbackupdialog.cpp \
    src/resyncdialog.cpp

RESOURCES = \
    res/coinvault.qrc \
    docs/docs.qrc

# install
target.path = build 
INSTALLS += target

# logger objects
LIBS += \
    deps/logger/obj/logger.o

# CoinQ objects 
LIBS += \
    deps/CoinQ/obj/CoinQ_vault.o \
    deps/CoinQ/obj/CoinQ_vault_db-odb.o \
    deps/CoinQ/obj/CoinQ_script.o \
    deps/CoinQ/obj/CoinQ_peer_io.o \
    deps/CoinQ/obj/CoinQ_netsync.o \
    deps/CoinQ/obj/CoinQ_blocks.o \
    deps/CoinQ/obj/CoinQ_filter.o

# CoinClasses objects
LIBS += \
    deps/CoinClasses/obj/CoinKey.o \
    deps/CoinClasses/obj/hdkeys.o \
    deps/CoinClasses/obj/CoinNodeData.o \
    deps/CoinClasses/obj/MerkleTree.o \
    deps/CoinClasses/obj/BloomFilter.o \
    deps/CoinClasses/obj/IPv6.o

# Hash function object (for Litecoin)
LIBS += \
    deps/CoinClasses/src/scrypt/obj/scrypt.o

# Hash function objects (for QuarkCoin)
LIBS += \
    deps/CoinClasses/src/hashfunc/obj/blake.o \
    deps/CoinClasses/src/hashfunc/obj/bmw.o \
    deps/CoinClasses/src/hashfunc/obj/groestl.o \
    deps/CoinClasses/src/hashfunc/obj/jh.o \
    deps/CoinClasses/src/hashfunc/obj/keccak.o \
    deps/CoinClasses/src/hashfunc/obj/skein.o

win32 {
    LIBS += \
        -static-libgcc -static-libstdc++ \
        -lws2_32 \
        -lmswsock \
        -lboost_system-mt-s \
        -lboost_filesystem-mt-s \
        -lboost_regex-mt-s \
        -lboost_thread_win32-mt-s \
        -lcrypto \
        -lodb-sqlite \
        -lodb
}

unix {
    !macx {
        LIBS += \
            -lboost_system \
            -lboost_filesystem \
            -lboost_regex \
            -lboost_thread \
            -lcrypto \
            -lodb-sqlite \
            -lodb
    }
    else {
	isEmpty(BOOST_LIB_PATH) {
            BOOST_LIB_PATH = /usr/local/lib
        }

        exists($$BOOST_LIB_PATH/libboost_system-mt*) {
            BOOST_LIB_SUFFIX = -mt
        }

        LIBS += \
            -L$$BOOST_LIB_PATH \
            -lboost_system$$BOOST_LIB_SUFFIX \
            -lboost_filesystem$$BOOST_LIB_SUFFIX \
            -lboost_regex$$BOOST_LIB_SUFFIX \
            -lboost_thread$$BOOST_LIB_SUFFIX \
            -lcrypto \
            -lodb-sqlite \
            -lodb
    }
}
