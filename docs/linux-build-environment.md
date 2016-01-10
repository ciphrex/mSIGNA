1) Create fresh ubuntu 14.04 (Trusty Tahr) VM
    http://cdimage.ubuntu.com/daily-live/current/trusty-desktop-amd64.iso

    (Note: ubuntu 13.xx can also be used, but gcc should be upgraded to v4.8 for full C++11 support)

2) Install packages

    sudo apt-get update && sudo apt-get upgrade
    sudo apt-get install ssh git build-essential libboost-all-dev libssl-dev libsqlite3-dev qtbase5-dev qt5-default

3) Install ODB compiler and library

    mkdir odb
    cd odb

Build and install libcutl for linux

    wget http://www.codesynthesis.com/download/libcutl/1.8/libcutl-1.8.0.tar.bz2
    tar -xjvf libcutl-1.8.0.tar.bz2
    cd libcutl-1.8.0
    ./configure
    make
    sudo make install
    sudo ldconfig
    cd ..

Build and install ODB compiler for linux

    sudo apt-get install gcc-4.8-plugin-dev
    wget http://www.codesynthesis.com/download/odb/2.3/odb-2.3.0.tar.bz2
    tar -xjvf odb-2.3.0.tar.bz2
    cd odb-2.3.0
    ./configure
    make
    sudo make install
    cd ..

4) Build ODB Common Runtime Library (libodb) for linux 

    wget http://www.codesynthesis.com/download/odb/2.3/libodb-2.3.0.tar.bz2
    tar -xjvf libodb-2.3.0.tar.bz2
    mkdir libodb-linux-build
    cd libodb-linux-build 
    ../libodb-2.3.0/configure
    make
    sudo make install
    cd ..

5) Build ODB Database Runtime Library for sqlite (libodb-sqlite) for linux 

    wget http://www.codesynthesis.com/download/odb/2.3/libodb-sqlite-2.3.0.tar.bz2
    tar -xjvf libodb-sqlite-2.3.0.tar.bz2
    mkdir libodb-sqlite-linux-build
    cd libodb-sqlite-linux-build
    ../libodb-sqlite-2.3.0/configure
    make
    sudo make install
    cd

6) Clone mSIGNA repository, in case you haven't done so already:

    git clone https://github.com/ciphrex/mSIGNA.git

7) Build qrencode QR Code C library (libqrencode) for linux

    cd mSIGNA/deps/qrencode-3.4.3
    ./configure --without-tools
    make
    sudo make install
    cd
