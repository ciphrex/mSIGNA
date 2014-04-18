////////////////////////////////////////////////////////////////////////////////
//
// BigInt.h
//
// Copyright (c) 2011 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef BIGINT_H_INCLUDED
#define BIGINT_H_INCLUDED

#include <openssl/crypto.h>
#include <openssl/bn.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <stdexcept>

class BigInt
{
protected:
    BIGNUM* bn;
    BN_CTX* ctx;
    bool autoclear;

    void allocate()
    {
        this->autoclear = false;
        if (!(this->bn = BN_new())) throw std::runtime_error("BIGNUM allocation error.");
        if (!(this->ctx = BN_CTX_new())) { BN_free(this->bn); throw std::runtime_error("BIGNUM allocation error."); }
    }

public:
    // Allocation & Assignment
    BigInt() { this->allocate(); }
    BigInt(const BigInt& bigint)
    {
        if (!(this->bn = BN_dup(bigint.bn))) throw std::runtime_error("BIGNUM allocation error.");
        if (!(this->ctx = BN_CTX_new())) { BN_free(this->bn); throw std::runtime_error("BIGNUM allocation error."); }
    }
    BigInt(BN_ULONG num)
    {
        this->allocate();
        this->setWord(num);
    }
    BigInt(const std::vector<unsigned char>& bytes, bool bigEndian = false)
    {
        this->allocate();
        this->setBytes(bytes, bigEndian);
    }
    BigInt(const std::string& inBase, unsigned int base = 16, const char* alphabet = "0123456789abcdef")
    {
        this->allocate();
        this->setInBase(inBase, base, alphabet);
    }

    ~BigInt()
    {
        if (this->bn) {
            if (this->autoclear) BN_clear_free(this->bn);
            else BN_free(this->bn);
        }
        if (this->ctx) BN_CTX_free(this->ctx);
    }

    void setAutoclear(bool autoclear = true) { this->autoclear = autoclear; }

    void clear() { if (this->bn) BN_clear(this->bn); }

    // Assignment operations
    //BigInt& operator=(BN_ULONG rhs) { if (!BN_set_word(this->bn, rhs)) throw std::runtime_error("BIGNUM Error."); return *this; }
    BigInt& operator=(const BigInt& bigint)
    {
        if (!(BN_copy(this->bn, bigint.bn))) throw std::runtime_error("BIGNUM allocation error.");
        //if (!(this->bn = BN_dup(bigint.bn))) throw std::runtime_error("BIGNUM allocation error.");
        //if (!(this->ctx = BN_CTX_new())) { BN_free(this->bn); throw std::runtime_error("BIGNUM allocation error."); }
        return *this;
    }

    // Arithmetic Operations
    BigInt& operator+=(const BigInt& rhs) { if (!BN_add(this->bn, this->bn, rhs.bn)) throw std::runtime_error("BN_add error."); return *this; }
    BigInt& operator-=(const BigInt& rhs) { if (!BN_sub(this->bn, this->bn, rhs.bn)) throw std::runtime_error("BN_sub error."); return *this; }
    BigInt& operator*=(const BigInt& rhs) { if (!BN_mul(this->bn, this->bn, rhs.bn, this->ctx)) throw std::runtime_error("BN_mul rror."); return *this; }
    BigInt& operator/=(const BigInt& rhs) { if (!BN_div(this->bn, NULL, this->bn, rhs.bn, this->ctx)) throw std::runtime_error("BN_div error."); return *this; }
    BigInt& operator%=(const BigInt& rhs) { if (!BN_div(NULL, this->bn, this->bn, rhs.bn, this->ctx)) throw std::runtime_error("BN_div error."); return *this; }

    BigInt& operator+=(BN_ULONG rhs) { if (!BN_add_word(this->bn, rhs)) throw std::runtime_error("BN_add_word error."); return *this; }
    BigInt& operator-=(BN_ULONG rhs) { if (!BN_sub_word(this->bn, rhs)) throw std::runtime_error("BN_sub_word error."); return *this; }
    BigInt& operator*=(BN_ULONG rhs) { if (!BN_mul_word(this->bn, rhs)) throw std::runtime_error("BN_mul_word error."); return *this; }
    BigInt& operator/=(BN_ULONG rhs) { BN_div_word(this->bn, rhs); return *this; }
    BigInt& operator%=(BN_ULONG rhs) { this->setWord(BN_mod_word(this->bn, rhs)); return *this; }

    const BigInt operator+(const BigInt& rightOperand) const { return BigInt(*this) += rightOperand; }
    const BigInt operator-(const BigInt& rightOperand) const { return BigInt(*this) -= rightOperand; }
    const BigInt operator*(const BigInt& rightOperand) const { return BigInt(*this) *= rightOperand; }
    const BigInt operator/(const BigInt& rightOperand) const { return BigInt(*this) /= rightOperand; }
    const BigInt operator%(const BigInt& rightOperand) const { return BigInt(*this) %= rightOperand; }

    const BigInt operator+(BN_ULONG rightOperand) const { return BigInt(*this) += rightOperand; }
    const BigInt operator-(BN_ULONG rightOperand) const { return BigInt(*this) -= rightOperand; }
    const BigInt operator*(BN_ULONG rightOperand) const { return BigInt(*this) *= rightOperand; }
    const BigInt operator/(BN_ULONG rightOperand) const { return BigInt(*this) /= rightOperand; }
    BN_LONG operator%(BN_ULONG rightOperand) const { return BN_mod_word(this->bn, rightOperand); }

    // Bitshift Operators
    BigInt& operator<<=(int rhs) { if (!BN_lshift(this->bn, this->bn, rhs)) throw std::runtime_error("BN_lshift error."); return *this; }
    BigInt& operator>>=(int rhs) { if (!BN_rshift(this->bn, this->bn, rhs)) throw std::runtime_error("BN_rshift error."); return *this; }

    const BigInt operator<<(int rhs) const { return BigInt(*this) <<= rhs; }
    const BigInt operator>>(int rhs) const { return BigInt(*this) >>= rhs; }

    // Comparison Operators
    bool operator==(const BigInt& rhs) const { return (BN_cmp(this->bn, rhs.bn) == 0); }
    bool operator!=(const BigInt& rhs) const { return (BN_cmp(this->bn, rhs.bn) != 0); }
    bool operator<(const BigInt& rhs) const { return (BN_cmp(this->bn, rhs.bn) < 0); }
    bool operator>(const BigInt& rhs) const { return (BN_cmp(this->bn, rhs.bn) > 0); }
    bool operator<=(const BigInt& rhs) const { return (BN_cmp(this->bn, rhs.bn) <= 0); }
    bool operator>=(const BigInt& rhs) const { return (BN_cmp(this->bn, rhs.bn) >= 0); }
    bool isZero() const { return BN_is_zero(this->bn); }

    // Accessor Methods
    BN_ULONG getWord() const { return BN_get_word(this->bn); }
    void setWord(BN_ULONG num) { if (!BN_set_word(this->bn, num)) throw std::runtime_error("BN_set_word error."); }

    std::vector<unsigned char> getBytes(bool bigEndian = false) const
    {
        std::vector<unsigned char> bytes;
        bytes.resize(BN_num_bytes(this->bn));
        BN_bn2bin(this->bn, &bytes[0]);
        if (bigEndian) reverse(bytes.begin(), bytes.end());
        return bytes;
    }
    void setBytes(std::vector<unsigned char> bytes, bool bigEndian = false)
    {
        if (bigEndian) reverse(bytes.begin(), bytes.end());
        BN_bin2bn(&bytes[0], bytes.size(), this->bn);
    }

    int numBytes() const { return BN_num_bytes(this->bn); }

    std::string getHex() const
    {
        char* hex = BN_bn2hex(this->bn);
        if (!hex) throw std::runtime_error("BN_bn2hex error.");
        std::string rval(hex);
        OPENSSL_free(hex);
        return rval;
    }
    void setHex(const std::string& hex) { BN_hex2bn(&this->bn, hex.c_str()); }
    void SetHex(const std::string& hex) { setHex(hex); }

    std::string getDec() const
    {
        char* dec = BN_bn2dec(this->bn);
        if (!dec) throw std::runtime_error("BN_bn2dec error.");
        std::string rval(dec);
        OPENSSL_free(dec);
        return rval;
    }
    void setDec(const std::string& dec) { BN_dec2bn(&this->bn, dec.c_str()); }

    std::string getInBase(unsigned int base, const char* alphabet) const
    {
        BigInt num = *this;
        std::string inBase;
        do {
            inBase = alphabet[num % base] + inBase; // TODO: check whether this is most efficient structure manipulation
            num /= base;
        } while (!num.isZero());
        return inBase;
    }

    void setInBase(const std::string& inBase, unsigned int base, const char* alphabet)
    {
        this->setWord(0);
        for (unsigned int i = 0; i < inBase.size(); i++) {
            const char* pPos = strchr(alphabet, inBase[i]);
            if (!pPos) continue;
            *this *= base;
            *this += (pPos - alphabet);
        }
    }
};

#endif // BIGINT_H_INCLUDED
