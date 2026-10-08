/* SHA-512, implemented from FIPS 180-4. */
#include "crypto.h"

static const u64 k[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
    0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

static u64 ror64(u64 x, unsigned int n)
{
    return (x >> n) | (x << (64U - n));
}

static u64 load_be64(const u8 *p)
{
    u64 x = 0;
    for (unsigned int i = 0; i < 8; ++i)
        x = (x << 8) | p[i];
    return x;
}

static void store_be64(u8 *p, u64 x)
{
    for (unsigned int i = 0; i < 8; ++i)
        p[7 - i] = (u8)(x >> (i * 8U));
}

static void sha512_block(u64 h[8], const u8 block[128])
{
    u64 w[80];
    u64 a, b, c, d, e, f, g, hh;

    for (unsigned int i = 0; i < 16; ++i)
        w[i] = load_be64(block + i * 8U);
    for (unsigned int i = 16; i < 80; ++i) {
        u64 s0 = ror64(w[i - 15], 1) ^ ror64(w[i - 15], 8) ^
                 (w[i - 15] >> 7);
        u64 s1 = ror64(w[i - 2], 19) ^ ror64(w[i - 2], 61) ^
                 (w[i - 2] >> 6);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = h[0]; b = h[1]; c = h[2]; d = h[3];
    e = h[4]; f = h[5]; g = h[6]; hh = h[7];
    for (unsigned int i = 0; i < 80; ++i) {
        u64 s1 = ror64(e, 14) ^ ror64(e, 18) ^ ror64(e, 41);
        u64 ch = (e & f) ^ (~e & g);
        u64 t1 = hh + s1 + ch + k[i] + w[i];
        u64 s0 = ror64(a, 28) ^ ror64(a, 34) ^ ror64(a, 39);
        u64 maj = (a & b) ^ (a & c) ^ (b & c);
        u64 t2 = s0 + maj;

        hh = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

int sha512(const void *data, usize len, u8 out[64])
{
    static const u64 initial[8] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
    };
    const u8 *input = (const u8 *)data;
    u64 h[8];
    u8 tail[256];
    usize full = len / 128;
    usize rem = len % 128;
    u64 bit_hi = (u64)len >> 61;
    u64 bit_lo = (u64)len << 3;

    if (!out || (len != 0 && !data))
        return -1;

    for (unsigned int i = 0; i < 8; ++i)
        h[i] = initial[i];
    for (usize i = 0; i < full; ++i)
        sha512_block(h, input + i * 128);

    for (usize i = 0; i < rem; ++i)
        tail[i] = input[full * 128 + i];
    tail[rem] = 0x80;
    for (usize i = rem + 1; i < sizeof(tail); ++i)
        tail[i] = 0;
    if (rem >= 112) {
        store_be64(tail + 240, bit_hi);
        store_be64(tail + 248, bit_lo);
        sha512_block(h, tail);
        sha512_block(h, tail + 128);
    } else {
        store_be64(tail + 112, bit_hi);
        store_be64(tail + 120, bit_lo);
        sha512_block(h, tail);
    }

    for (unsigned int i = 0; i < 8; ++i)
        store_be64(out + i * 8U, h[i]);
    return 0;
}
