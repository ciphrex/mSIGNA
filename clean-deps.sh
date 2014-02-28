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
