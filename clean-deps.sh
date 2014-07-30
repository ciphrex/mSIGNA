#!/bin/bash
set -x

cd deps/logger
make clean

cd ../sysutils
make clean

cd ../CoinCore
make clean

cd ../CoinQ
make clean

cd ../CoinDB
make clean

cd ../WebSocketClient
make clean

cd ../../sysroot
rm -rf include
rm -rf lib
