#!/bin/bash

for OPTION in $@
do
    case $OPTION in
    linux)
        if [[ ! -z "$OS" ]]; then echo "OS cannot be set twice"; exit 1; fi
        OS=linux
    ;;

    mingw64)
        if [[ ! -z "$OS" ]]; then echo "OS cannot be set twice"; exit 1; fi
        OS=mingw64
    ;;

    osx)
        if [[ ! -z "$OS" ]]; then echo "OS cannot be set twice"; exit 1; fi
        OS=osx
    ;;

    sqlite)
        if [[ ! -z "$DB" ]]; then echo "DB cannot be set twice"; exit 1; fi
        DB=sqlite
    ;;

    mysql)
        if [[ ! -z "$DB" ]]; then echo "DB cannot be set twice"; exit 1; fi
        DB=mysql
    ;;

    debug)
        if [[ ! -z "$BUILD_TYPE" ]]; then echo "Build type cannot be set twice"; exit 2; fi
        BUILD_TYPE=debug
        OPTIONS="DEBUG=1 $OPTIONS"
    ;;

    release)
        if [[ ! -z "$BUILD_TYPE" ]]; then echo "Build type cannot be set twice"; exit 2; fi
        BUILD_TYPE=release
    ;;

    tools_only)
        tools_only=true
    ;;

    local)
        SYSROOT=../../sysroot
    ;;

    *)
        OPTIONS="$OPTIONS $OPTION"
    esac
done

if [[ -z "$OS" ]]
then
    UNAME_S=$(uname -s)
    if [[ "$UNAME_S" == "Linux" ]]
    then
        OS=linux
    elif [[ "$UNAME_S" == "Darwin" ]]
    then
        OS=osx
    fi
fi

# Set target platform parameters
case $OS in
linux)
;;

mingw64)
    if [[ -z "$QMAKE_PATH" ]]; then QMAKE_PATH="/usr/x86_64-w64-mingw32/host/bin/"; fi
    SPEC="-spec win32-g++"
;;

osx)
;;

*)
    echo "You must specify between linux, mingw64, and osx."
    exit
esac

if [[ -z "$DB" ]]
then
    DB=sqlite
fi

# Use release as default build type
if [[ -z "$BUILD_TYPE" ]]
then
    BUILD_TYPE=release
fi

if [[ -z "$SYSROOT" ]]
then
    SYSROOT=/usr/local
fi

CURRENT_DIR=$(pwd)

set -x
set -e

cd deps/logger
make OS=$OS $OPTIONS
make install

cd ../Signals
make install

cd ../stdutils
make install

cd ../sysutils
make OS=$OS $OPTIONS
make install

cd ../CoinCore
make OS=$OS $OPTIONS
make install

cd ../CoinQ
make OS=$OS $OPTIONS
make install

cd ../CoinDB
make lib OS=$OS DB=$DB $OPTIONS
make install_lib

