////////////////////////////////////////////////////////////////////////////////
//
// encrypt.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <openssl/aes.h>

#include <aes.h>
#include <hash.h>
#include <random.h>
#include <stdutils/uchar_vector.h>

#include <iostream>

using namespace AES;
using namespace std;

int main(int argc, char* argv[])
{

    if (argc < 3 || argc > 4)
    {
        cerr << "# Usage: " << argv[0] << " <passphrase> <plaintext> [salt]" << endl;
        return -1;
    }

    try
    {
        secure_bytes_t passphrase((unsigned char*)&argv[1][0], (unsigned char*)&argv[1][0] + strlen(argv[1]));
        secure_bytes_t key = sha256_2(passphrase);

        secure_bytes_t data((unsigned char*)&argv[2][0], (unsigned char*)&argv[2][0] + strlen(argv[2]));

        uint64_t salt = argc > 3 ? strtoull(argv[3], NULL, 0) : random_salt();
        bytes_t cipherdata = encrypt(key, data, true, salt);

        cout << "Hex key:        " << uchar_vector(key).getHex() << endl;
        cout << "Hex ciphertext: " << uchar_vector(cipherdata).getHex() << endl;
        cout << "Salt:           " << salt << endl;
    }
    catch (const AESException& e)
    {
        cerr << "AES Exception: " << e.what() << endl;
        return e.code();
    }
    catch (const exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -2;
    }    

    return 0;
}
