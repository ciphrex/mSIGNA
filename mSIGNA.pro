#######################################
# mSIGNA.pro
#
# Copyright (c) 2013-2015 Eric Lombrozo
#
# All Rights Reserved.

# Application icons for windows
RC_FILE = res/coinvault.rc

# Application icons for mac
ICON = res/icons/app_icons/osx.icns

DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE DATABASE_SQLITE
CONFIG += c++11 rtti thread

QT += widgets network

QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas


INCLUDEPATH += \
    sysroot/include \
    /usr/local/include

LIBS += \
    -Lsysroot/lib \
    -lCoinDB \
    -lCoinQ \
    -lCoinCore \
    -llogger \
    -lqrencode

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
    src/stylesheets.h \
    src/aboutdialog.h \
    src/coinparams.h \
    src/docdir.h \
    src/severitylogger.h \
    src/entropysource.h \
    src/currencyunitdialog.h \
    src/networkselectiondialog.h \
    src/splashscreen.h \
    src/acceptlicensedialog.h \
    src/mainwindow.h \
    src/commandserver.h \
    src/paymentrequest.h \
    src/numberformats.h \
    src/currencyvalidator.h \
    src/hexvalidator.h \
    src/accountmodel.h \
    src/accountview.h \
    src/keychainmodel.h \
    src/keychainview.h \
    src/entropydialog.h \
    src/quicknewaccountdialog.h \
    src/newaccountdialog.h \
    src/newkeychaindialog.h \
    src/txsearchdialog.h \
    src/rawtxdialog.h \
    src/createtxdialog.h \
    src/unspenttxoutmodel.h \
    src/unspenttxoutview.h \
    src/txmodel.h \
    src/txview.h \
    src/accounthistorydialog.h \
    src/txactions.h \
    src/scriptmodel.h \
    src/scriptview.h \
    src/scriptdialog.h \
    src/signaturemodel.h \
    src/signatureview.h \
    src/signaturedialog.h \
    src/signatureactions.h \
    src/requestpaymentdialog.h \
    src/networksettingsdialog.h \
    src/wordlistvalidator.h \
    src/keychainbackupwizard.h \
    src/keychainbackupdialog.h \
    src/viewbip32dialog.h \
    src/importbip32dialog.h \
    src/viewbip39dialog.h \
    src/importbip39dialog.h \
    src/passphrasedialog.h \
    src/setpassphrasedialog.h
                
SOURCES = \
    src/settings.cpp \
    src/filesystem.cpp \
    src/versioninfo.cpp \
    src/copyrightinfo.cpp \
    src/stylesheets.cpp \
    src/aboutdialog.cpp \
    src/coinparams.cpp \
    src/docdir.cpp \
    src/entropysource.cpp \
    src/main.cpp \
    src/currencyunitdialog.cpp \
    src/networkselectiondialog.cpp \
    src/splashscreen.cpp \
    src/acceptlicensedialog.cpp \
    src/mainwindow.cpp \
    src/commandserver.cpp \
    src/paymentrequest.cpp \
    src/numberformats.cpp \
    src/currencyvalidator.cpp \
    src/hexvalidator.cpp \
    src/accountmodel.cpp \
    src/accountview.cpp \
    src/keychainmodel.cpp \
    src/keychainview.cpp \
    src/entropydialog.cpp \
    src/quicknewaccountdialog.cpp \
    src/newaccountdialog.cpp \
    src/newkeychaindialog.cpp \
    src/txsearchdialog.cpp \
    src/rawtxdialog.cpp \
    src/createtxdialog.cpp \
    src/unspenttxoutmodel.cpp \
    src/unspenttxoutview.cpp \
    src/txmodel.cpp \
    src/txview.cpp \
    src/accounthistorydialog.cpp \
    src/txactions.cpp \
    src/scriptmodel.cpp \
    src/scriptview.cpp \
    src/scriptdialog.cpp \
    src/signaturemodel.cpp \
    src/signatureview.cpp \
    src/signaturedialog.cpp \
    src/signatureactions.cpp \
    src/requestpaymentdialog.cpp \
    src/networksettingsdialog.cpp \
    src/wordlistvalidator.cpp \
    src/keychainbackupwizard.cpp \
    src/keychainbackupdialog.cpp \
    src/viewbip32dialog.cpp \
    src/importbip32dialog.cpp \
    src/viewbip39dialog.cpp \
    src/importbip39dialog.cpp \
    src/passphrasedialog.cpp \
    src/setpassphrasedialog.cpp

RESOURCES = \
    res/coinvault.qrc \
    docs/docs.qrc

# install
target.path = build 
INSTALLS += target

win32 {
    BOOST_LIB_SUFFIX = -mt-s
    BOOST_THREAD_LIB_SUFFIX = _win32
    DEFINES += LIBODB_SQLITE_STATIC_LIB LIBODB_STATIC_LIB
    CONFIG += static

    contains(OS, mingw64){
     LIBS += \
        -L/usr/x86_64-w64-mingw32/lib \
        -L/usr/x86_64-w64-mingw32/plugins/platforms \
        -L/usr/x86_64-w64-mingw32/plugins/imageformats \
        -L/usr/x86_64-w64-mingw32/plugins/bearer \

    }

    contains(OS, mingw32){    
     LIBS += \
        -L/usr/i686-w64-mingw32/lib \
        -L/usr/i686-w64-mingw32/plugins/platforms \
        -L/usr/i686-w64-mingw32/plugins/imageformats \
        -L/usr/i686-w64-mingw32/plugins/bearer \
    }
   
   LIBS += \
        -static-libgcc -static-libstdc++ -static \
        -lgdi32 \
        -lws2_32 \
        -lmswsock \
        -lpthread
}

macx {
    isEmpty(BOOST_LIB_PATH) {
        BOOST_LIB_PATH = /usr/local/lib
    }

    exists($$BOOST_LIB_PATH/libboost_system-mt*) {
        BOOST_LIB_SUFFIX = -mt
    }

    LIBS += \
        -L$$BOOST_LIB_PATH
}

LIBS += \
    -lboost_system$$BOOST_LIB_SUFFIX \
    -lboost_filesystem$$BOOST_LIB_SUFFIX \
    -lboost_regex$$BOOST_LIB_SUFFIX \
    -lboost_thread$$BOOST_THREAD_LIB_SUFFIX$$BOOST_LIB_SUFFIX \
    -lboost_serialization$$BOOST_LIB_SUFFIX \
    -lcrypto \
    -lodb-sqlite \
    -lodb \
    -lsqlite3
