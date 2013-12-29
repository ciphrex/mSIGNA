#!/bin/bash
set -x

cd deps/CoinClasses
make clean OS=linux

cd ../CoinQ
make clean OS=linux

cd ../sqlite3
make clean OS=linux
