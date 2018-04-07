////////////////////////////////////////////////////////////////////////////////
//
// decrypt.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <aes.h>
#include <hash.h>
#include <stdutils/uchar_vector.h>

#include <iostream>

using namespace AES;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        cerr << "# Usage: " << argv[0] << " <passphrase> <hex ciphertext> <salt>" << endl;
        return -1;
    }

    try
    {
        secure_bytes_t passphrase((unsigned char*)&argv[1][0], (unsigned char*)&argv[1][0] + strlen(argv[1]));
        secure_bytes_t key = sha256_2(passphrase);

        bytes_t ciphertext = uchar_vector(argv[2]);

        uint64_t salt = strtoull(argv[3], NULL, 0);

        secure_bytes_t data = decrypt(key, ciphertext, true, salt);

        char* plaintext = (char*)malloc(data.size() + 1);
        memcpy((void*)plaintext, (const void*)&data[0], data.size());
        plaintext[data.size()] = 0;

        cout << "Hex key:        " << uchar_vector(key).getHex() << endl;
        cout << "Plaintext:      " << plaintext << endl;
        cout << "Salt:           " << salt << endl;

        free(plaintext);
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
