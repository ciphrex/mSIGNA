#!/bin/bash
set -x

cd deps/logger
make clean

cd ../CoinClasses
make clean

cd ../CoinQ
make clean

cd ../CoinDB
make clean

cd ../..

if [ -f "Makefile" ]
then
    make clean
fi
