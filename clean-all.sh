#!/bin/bash
set -x

cd deps/logger
make clean OS=linux

cd ../CoinClasses
make clean OS=linux

cd ../CoinQ
make clean OS=linux

cd ../sqlite3
make clean OS=linux

cd ../..
make clean
