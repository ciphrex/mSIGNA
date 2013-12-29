#!/bin/bash
set -x
CURRENT_DIR=$(pwd)

cd ../../../CoinClasses
make OS=$1

cd ../CoinQ
make OS=$1

cd ../sqlite3
make OS=$1

cd $CURRENT_DIR
make OS=$1
