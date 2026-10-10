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

#define SHA512_CONTEXT_ACTIVE 0x53483531U
#define SHA512_MAX_BYTES_HIGH (~(u64)0 >> 3)

static void copy_bytes(u8 *dst, const u8 *src, usize len)
{
    for (usize i = 0; i < len; ++i)
        dst[i] = src[i];
}

int sha512_init(struct sha512_ctx *ctx)
{
    static const u64 initial[8] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
    };
    if (!ctx)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    (void)crypto_secure_zero(ctx, sizeof(*ctx));
    for (unsigned int i = 0; i < 8; ++i)
        ctx->state[i] = initial[i];
    ctx->lifecycle = SHA512_CONTEXT_ACTIVE;
    return CRYPTO_OK;
}

int sha512_update(struct sha512_ctx *ctx, const void *data, usize len)
{
    if (!ctx || ctx->lifecycle != SHA512_CONTEXT_ACTIVE ||
        (len && !data) || ctx->buffered >= SHA512_BLOCK_SIZE ||
        ctx->total_bytes_high > SHA512_MAX_BYTES_HIGH)
        return CRYPTO_ERR_INVALID_ARGUMENT;
    if (!len)
        return CRYPTO_OK;

    u64 added = (u64)len;
    u64 new_low = ctx->total_bytes_low + added;
    u64 carry = new_low < ctx->total_bytes_low;
    if (ctx->total_bytes_high > SHA512_MAX_BYTES_HIGH - carry)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    const u8 *input = (const u8 *)data;
    usize left = len;
    ctx->total_bytes_low = new_low;
    ctx->total_bytes_high += carry;

    if (ctx->buffered) {
        usize take = SHA512_BLOCK_SIZE - ctx->buffered;
        if (take > left)
            take = left;
        copy_bytes(ctx->block + ctx->buffered, input, take);
        ctx->buffered += take;
        input += take;
        left -= take;
        if (ctx->buffered == SHA512_BLOCK_SIZE) {
            sha512_block(ctx->state, ctx->block);
            (void)crypto_secure_zero(ctx->block, sizeof(ctx->block));
            ctx->buffered = 0;
        }
    }

    while (left >= SHA512_BLOCK_SIZE) {
        sha512_block(ctx->state, input);
        input += SHA512_BLOCK_SIZE;
        left -= SHA512_BLOCK_SIZE;
    }
    if (left) {
        copy_bytes(ctx->block, input, left);
        ctx->buffered = left;
    }
    return CRYPTO_OK;
}

int sha512_final(struct sha512_ctx *ctx, u8 out[SHA512_DIGEST_SIZE])
{
    if (!ctx || !out || ctx->lifecycle != SHA512_CONTEXT_ACTIVE ||
        ctx->buffered >= SHA512_BLOCK_SIZE ||
        ctx->total_bytes_high > SHA512_MAX_BYTES_HIGH)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    u8 tail[256];
    for (usize i = 0; i < sizeof(tail); ++i)
        tail[i] = 0;
    copy_bytes(tail, ctx->block, ctx->buffered);
    tail[ctx->buffered] = 0x80;

    u64 bit_hi = (ctx->total_bytes_high << 3) |
                 (ctx->total_bytes_low >> 61);
    u64 bit_lo = ctx->total_bytes_low << 3;
    usize length_offset = ctx->buffered < 112 ? 112 : 240;
    store_be64(tail + length_offset, bit_hi);
    store_be64(tail + length_offset + 8, bit_lo);

    sha512_block(ctx->state, tail);
    if (ctx->buffered >= 112)
        sha512_block(ctx->state, tail + SHA512_BLOCK_SIZE);

    for (unsigned int i = 0; i < 8; ++i)
        store_be64(out + i * 8U, ctx->state[i]);

    (void)crypto_secure_zero(tail, sizeof(tail));
    (void)crypto_secure_zero(ctx, sizeof(*ctx));
    return CRYPTO_OK;
}

int sha512(const void *data, usize len, u8 out[SHA512_DIGEST_SIZE])
{
    struct sha512_ctx ctx;
    int result;

    if (!out || (len && !data))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    result = sha512_init(&ctx);
    if (result != CRYPTO_OK)
        return result;
    result = sha512_update(&ctx, data, len);
    if (result != CRYPTO_OK) {
        (void)crypto_secure_zero(&ctx, sizeof(ctx));
        return result;
    }
    return sha512_final(&ctx, out);
}
