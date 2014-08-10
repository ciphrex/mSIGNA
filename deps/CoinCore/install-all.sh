#!/bin/bash

echo "Building..."
make

echo
echo "Installing locally..."
SYSROOT=../../sysroot make install

echo
echo "Installing globally..."
sudo make install
