                                 m S I G N A (TM)
===============================================================================
Copyright (c) 2013-2017 Ciphrex Corporation


mSIGNA is an interactive desktop application for transacting on the bitcoin network
featuring m-of-n signature policies, multiuser/multidevice account management,
and multiple blockchain support.

Bitcoin mainnet is used as the default blockchain. To select a different supported
blockchain, start mSIGNA with the command line parameter "select". To add support
for an unsupported compatible blockchain, please add your parameters to the following
source files and submit a pull request:

    deps/CoinQ/src/CoinQ_coinparams.h
    deps/CoinQ/src/CoinQ_coinparams.cpp


New since 0.10.0: Segregated witness support. Only supported on blockchains that
currently support it.


mSIGNA depends on several libraries also included in this repository including:

    CoinCore
    CoinQ
    CoinDB

These libraries provide all core functionality for managing bitcoin data structures,
connecting to peers, and handling persistent storage.

They are licensed under the MIT license. See accompanying file LICENSE for details


In addition, mSIGNA depends on the following:

    - Qt5 application and UI framework          http://qt-project.org/
        * Qt5Core
        * Qt5Gui
        * Qt5Widgets
        * Qt5Network

    - Boost C++ Libraries (1.49 or greater)     http://www.boost.org/
        * boost_system
        * boost_filesystem
        * boost_regex
        * boost_thread
        * boost_serialization

    - OpenSSL (including EC module)             http://www.openssl.org/
        * crypto

    - ODB: C++ Object-Relational Mapping        http://www.codesynthesis.com/products/odb/download.xhtml
        * ODB Compiler (odb)
        * Common Runtime Library (libodb)
        * SQLite Database Runtime Library (libodb-sqlite)

    - SQLite transactional database engine      http://www.sqlite.org/
        * sqlite3

    - qrencode QR Code C library                http://fukuchi.org/works/qrencode/
        * qrencode (distribution provided in deps directory)

mSIGNA has been built in Linux using gcc 4.6.3 and greater for both 64-bit Linux
and 64-bit Windows (cross-build, mingw64). It has also been built using
clang (Xcode 4.1 and greater) in OS X (10.7 and greater).

Recommended compilers:
    - gcc4.8 or greater
    - clang with llvm v3.3 or greater

Earlier compilers will no longer be supported.

For instructions on setting up a build environment, look inside the docs directory.

- To build for linux:
    $ ./build-all.sh linux

- To build for windows:
    $ ./build-all.sh mingw64

- To build for os x:
    $ ./build-all.sh osx


An optional parameter can be specified [debug|release]. Default is release.
You may also specify additional parameters to be passed to make.

===============================================================================

DISCLAIMER:

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

