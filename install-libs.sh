#!/bin/bash

for OPTION in $@
do
    case $OPTION in
    linux)
        if [[ ! -z "$OS" ]]; then echo "OS cannot be set twice"; exit 1; fi
        OS=linux
        OPTIONS="OS=linux $OPTIONS"
    ;;

    mingw64)
        if [[ ! -z "$OS" ]]; then echo "OS cannot be set twice"; exit 1; fi
        OS=mingw64
        OPTIONS="OS=mingw64 $OPTIONS"
    ;;

    osx)
        if [[ ! -z "$OS" ]]; then echo "OS cannot be set twice"; exit 1; fi
        OS=osx
        OPTIONS="OS=osx $OPTIONS"
    ;;

    sqlite)
        if [[ ! -z "$DB" ]]; then echo "DB cannot be set twice"; exit 1; fi
        DB=sqlite
        OPTIONS="DB=sqlite $OPTIONS"
    ;;

    mysql)
        if [[ ! -z "$DB" ]]; then echo "DB cannot be set twice"; exit 1; fi
        DB=mysql
        OPTIONS="DB=mysql $OPTIONS"
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

    *)
        OPTIONS="$OPTIONS $OPTION"
    esac
done

# Use release as default build type
if [[ -z "$BUILD_TYPE" ]]
then
    BUILD_TYPE=release
fi

CURRENT_DIR=$(pwd)

set -x
set -e

cd deps/logger
make $OPTIONS
make install

cd ../Signals
make install

cd ../stdutils
make install

cd ../sysutils
make $OPTIONS
make install

cd ../CoinCore
make $OPTIONS
make install

cd ../CoinQ
make $OPTIONS
make install

cd ../CoinDB
make lib $OPTIONS
make install_lib

