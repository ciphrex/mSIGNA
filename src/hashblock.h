#ifndef HASHBLOCK_H
#define HASHBLOCK_H

#include "uint256.h"
#include "hashfunc/sph_blake.h"
#include "hashfunc/sph_bmw.h"
#include "hashfunc/sph_groestl.h"
#include "hashfunc/sph_jh.h"
#include "hashfunc/sph_keccak.h"
#include "hashfunc/sph_skein.h"

#ifndef QT_NO_DEBUG
#include <string>
#endif

#ifdef GLOBALDEFINED
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL sph_blake512_context     z_blake;
GLOBAL sph_bmw512_context       z_bmw;
GLOBAL sph_groestl512_context   z_groestl;
GLOBAL sph_jh512_context        z_jh;
GLOBAL sph_keccak512_context    z_keccak;
GLOBAL sph_skein512_context     z_skein;

#define fillz() do { \
    sph_blake512_init(&z_blake); \
    sph_bmw512_init(&z_bmw); \
    sph_groestl512_init(&z_groestl); \
    sph_jh512_init(&z_jh); \
    sph_keccak512_init(&z_keccak); \
    sph_skein512_init(&z_skein); \
} while (0) 

#define ZBLAKE (memcpy(&ctx_blake, &z_blake, sizeof(z_blake)))
#define ZBMW (memcpy(&ctx_bmw, &z_bmw, sizeof(z_bmw)))
#define ZGROESTL (memcpy(&ctx_groestl, &z_groestl, sizeof(z_groestl)))
#define ZJH (memcpy(&ctx_jh, &z_jh, sizeof(z_jh)))
#define ZKECCAK (memcpy(&ctx_keccak, &z_keccak, sizeof(z_keccak)))
#define ZSKEIN (memcpy(&ctx_skein, &z_skein, sizeof(z_skein)))

template<typename T1>
inline uint256 Hash9(const T1 pbegin, const T1 pend)

{
    sph_blake512_context     ctx_blake;
    sph_bmw512_context       ctx_bmw;
    sph_groestl512_context   ctx_groestl;
    sph_jh512_context        ctx_jh;
    sph_keccak512_context    ctx_keccak;
    sph_skein512_context     ctx_skein;
    static unsigned char pblank[1];

#ifndef QT_NO_DEBUG
    //std::string strhash;
    //strhash = "";
#endif

    uint512 mask = 8;
    uint512 zero = 0;
    
    uint512 hash[9];

    sph_blake512_init(&ctx_blake);
    // ZBLAKE;
    sph_blake512 (&ctx_blake, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_blake512_close(&ctx_blake, static_cast<void*>(&hash[0]));
    
    sph_bmw512_init(&ctx_bmw);
    // ZBMW;
    sph_bmw512 (&ctx_bmw, static_cast<const void*>(&hash[0]), 64);
    sph_bmw512_close(&ctx_bmw, static_cast<void*>(&hash[1]));

    if ((hash[1] & mask) != zero)
    {
        sph_groestl512_init(&ctx_groestl);
        // ZGROESTL;
        sph_groestl512 (&ctx_groestl, static_cast<const void*>(&hash[1]), 64);
        sph_groestl512_close(&ctx_groestl, static_cast<void*>(&hash[2]));
    }
    else
    {
        sph_skein512_init(&ctx_skein);
        // ZSKEIN;
        sph_skein512 (&ctx_skein, static_cast<const void*>(&hash[1]), 64);
        sph_skein512_close(&ctx_skein, static_cast<void*>(&hash[2]));
    }
    
    sph_groestl512_init(&ctx_groestl);
    // ZGROESTL;
    sph_groestl512 (&ctx_groestl, static_cast<const void*>(&hash[2]), 64);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(&hash[3]));

    sph_jh512_init(&ctx_jh);
    // ZJH;
    sph_jh512 (&ctx_jh, static_cast<const void*>(&hash[3]), 64);
    sph_jh512_close(&ctx_jh, static_cast<void*>(&hash[4]));

    if ((hash[4] & mask) != zero)
    {
        sph_blake512_init(&ctx_blake);
        // ZBLAKE;
        sph_blake512 (&ctx_blake, static_cast<const void*>(&hash[4]), 64);
        sph_blake512_close(&ctx_blake, static_cast<void*>(&hash[5]));
    }
    else
    {
        sph_bmw512_init(&ctx_bmw);
        // ZBMW;
        sph_bmw512 (&ctx_bmw, static_cast<const void*>(&hash[4]), 64);
        sph_bmw512_close(&ctx_bmw, static_cast<void*>(&hash[5]));
    }
    
    sph_keccak512_init(&ctx_keccak);
    // ZKECCAK;
    sph_keccak512 (&ctx_keccak, static_cast<const void*>(&hash[5]), 64);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(&hash[6]));

    sph_skein512_init(&ctx_skein);
    // SKEIN;
    sph_skein512 (&ctx_skein, static_cast<const void*>(&hash[6]), 64);
    sph_skein512_close(&ctx_skein, static_cast<void*>(&hash[7]));

    if ((hash[7] & mask) != zero)
    {
        sph_keccak512_init(&ctx_keccak);
        // ZKECCAK;
        sph_keccak512 (&ctx_keccak, static_cast<const void*>(&hash[7]), 64);
        sph_keccak512_close(&ctx_keccak, static_cast<void*>(&hash[8]));
    }
    else
    {
        sph_jh512_init(&ctx_jh);
        // ZJH;
        sph_jh512 (&ctx_jh, static_cast<const void*>(&hash[7]), 64);
        sph_jh512_close(&ctx_jh, static_cast<void*>(&hash[8]));
    }

    return hash[8].trim256();
}






#endif // HASHBLOCK_H
