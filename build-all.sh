#!/bin/bash

case $1 in
linux)
    SPEC=""
;;

mingw64)
    SPEC="-spec win32-g++"
;;

*)
    echo "You must specify between linux and mingw64."
    exit
esac

CURRENT_DIR=$(pwd)

set -x

cd deps/logger
make OS=$1

cd ../CoinClasses
make OS=$1

cd ../CoinQ
make OS=$1

cd ../sqlite3
make OS=$1

cd $CURRENT_DIR
qmake $SPEC && make
