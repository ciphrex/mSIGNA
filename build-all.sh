#!/bin/bash

# Set target platform
case $1 in
linux)
    SPEC=""
;;

mingw64)
    SPEC="-spec win32-g++"
;;

osx)
    SPEC=""
;;

*)
    echo "You must specify between linux, mingw64, and osx."
    exit
esac

# Set build type
case $2 in
debug)
    BUILD_TYPE=debug
    OPTIONS="$OPTIONS DEBUG=1"
;;

*)
    BUILD_TYPE=release
esac


CURRENT_DIR=$(pwd)

set -x

cd deps/logger
make OS=$1 $OPTIONS

cd ../CoinClasses
make OS=$1 $OPTIONS

cd ../CoinQ
make OS=$1 $OPTIONS

cd ../CoinDB
make OS=$1 $OPTIONS

cd $CURRENT_DIR
qmake $SPEC CONFIG+=$BUILD_TYPE && make

case $1 in
osx)
    macdeployqt $(find ./build/$BUILD_TYPE -name *.app)
;;

esac
