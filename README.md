                               V A U L T (TM)
===============================================================================
Copyright (c) 2013 Ciphrex LLC, All Rights Reserved.


Vault is an interactive desktop application for transacting on the bitcoin network
supporting m-of-n signature policies and multiuser/multidevice account management.

It is built atop two custom libraries, CoinClasses and CoinQ, which provide
all core functionality for managing bitcoin data structures and connecting to peers.

    -CoinClasses is licensed under the MIT license.

    -CoinQ is licensed under the GPLv2 license.


In addition, Vault depends on the following:

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

    - OpenSSL (including EC module)             http://www.openssl.org/
        * crypto

    - ODB: C++ Object-Relational Mapping        http://www.codesynthesis.com/products/odb/download.xhtml
        * ODB Compiler (odb)
        * Common Runtime Library (libodb)
        * SQLite Database Runtime Library (libodb-sqlite)

Vault has been built in Linux using gcc 4.6.3 and greater for both 64-bit Linux
and 64-bit Windows (cross-build, mingw64). It has also been built using
clang (4.1 and greater) in OS X (10.7 and greater).

For instructions on setting up a build environment, look inside the docs directory.

- To build for linux:
    $ ./build-all.sh linux

- To build for windows:
    $ ./build-all.sh mingw64

- To build for os x:
    $ ./build-all.sh osx


An optional second parameter can be specified [debug|release]. Default is release.

===============================================================================

DISCLAIMER:

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

