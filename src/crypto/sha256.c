/* SHA-256, implemented from FIPS 180-4. */
#include "crypto.h"

static const u32 k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static u32 ror32(u32 x, unsigned int n)
{
    return (x >> n) | (x << (32U - n));
}

static u32 load_be32(const u8 *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
           ((u32)p[2] << 8) | (u32)p[3];
}

static void store_be32(u8 *p, u32 x)
{
    p[0] = (u8)(x >> 24);
    p[1] = (u8)(x >> 16);
    p[2] = (u8)(x >> 8);
    p[3] = (u8)x;
}

static void sha256_block(u32 h[8], const u8 block[64])
{
    u32 w[64];
    u32 a, b, c, d, e, f, g, hh;

    for (unsigned int i = 0; i < 16; ++i)
        w[i] = load_be32(block + i * 4U);
    for (unsigned int i = 16; i < 64; ++i) {
        u32 s0 = ror32(w[i - 15], 7) ^ ror32(w[i - 15], 18) ^
                 (w[i - 15] >> 3);
        u32 s1 = ror32(w[i - 2], 17) ^ ror32(w[i - 2], 19) ^
                 (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = h[0]; b = h[1]; c = h[2]; d = h[3];
    e = h[4]; f = h[5]; g = h[6]; hh = h[7];
    for (unsigned int i = 0; i < 64; ++i) {
        u32 s1 = ror32(e, 6) ^ ror32(e, 11) ^ ror32(e, 25);
        u32 ch = (e & f) ^ (~e & g);
        u32 t1 = hh + s1 + ch + k[i] + w[i];
        u32 s0 = ror32(a, 2) ^ ror32(a, 13) ^ ror32(a, 22);
        u32 maj = (a & b) ^ (a & c) ^ (b & c);
        u32 t2 = s0 + maj;

        hh = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

#define SHA256_CONTEXT_ACTIVE 0x53483236U
#define SHA256_MAX_BYTES (~(u64)0 >> 3)

static void copy_bytes(u8 *dst, const u8 *src, usize len)
{
    for (usize i = 0; i < len; ++i)
        dst[i] = src[i];
}

int sha256_init(struct sha256_ctx *ctx)
{
    static const u32 initial[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    if (!ctx)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    (void)crypto_secure_zero(ctx, sizeof(*ctx));
    for (unsigned int i = 0; i < 8; ++i)
        ctx->state[i] = initial[i];
    ctx->lifecycle = SHA256_CONTEXT_ACTIVE;
    return CRYPTO_OK;
}

int sha256_update(struct sha256_ctx *ctx, const void *data, usize len)
{
    if (!ctx || ctx->lifecycle != SHA256_CONTEXT_ACTIVE ||
        (len && !data) || ctx->buffered >= SHA256_BLOCK_SIZE ||
        ctx->total_bytes > SHA256_MAX_BYTES ||
        (u64)len > SHA256_MAX_BYTES - ctx->total_bytes)
        return CRYPTO_ERR_INVALID_ARGUMENT;
    if (!len)
        return CRYPTO_OK;

    const u8 *input = (const u8 *)data;
    usize left = len;
    ctx->total_bytes += (u64)len;

    if (ctx->buffered) {
        usize take = SHA256_BLOCK_SIZE - ctx->buffered;
        if (take > left)
            take = left;
        copy_bytes(ctx->block + ctx->buffered, input, take);
        ctx->buffered += take;
        input += take;
        left -= take;
        if (ctx->buffered == SHA256_BLOCK_SIZE) {
            sha256_block(ctx->state, ctx->block);
            (void)crypto_secure_zero(ctx->block, sizeof(ctx->block));
            ctx->buffered = 0;
        }
    }

    while (left >= SHA256_BLOCK_SIZE) {
        sha256_block(ctx->state, input);
        input += SHA256_BLOCK_SIZE;
        left -= SHA256_BLOCK_SIZE;
    }
    if (left) {
        copy_bytes(ctx->block, input, left);
        ctx->buffered = left;
    }
    return CRYPTO_OK;
}

int sha256_final(struct sha256_ctx *ctx, u8 out[SHA256_DIGEST_SIZE])
{
    if (!ctx || !out || ctx->lifecycle != SHA256_CONTEXT_ACTIVE ||
        ctx->buffered >= SHA256_BLOCK_SIZE ||
        ctx->total_bytes > SHA256_MAX_BYTES)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    u8 tail[128];
    for (usize i = 0; i < sizeof(tail); ++i)
        tail[i] = 0;
    copy_bytes(tail, ctx->block, ctx->buffered);
    tail[ctx->buffered] = 0x80;

    u64 bit_length = ctx->total_bytes << 3;
    usize length_offset = ctx->buffered < 56 ? 56 : 120;
    for (unsigned int i = 0; i < 8; ++i)
        tail[length_offset + 7 - i] = (u8)(bit_length >> (i * 8U));

    sha256_block(ctx->state, tail);
    if (ctx->buffered >= 56)
        sha256_block(ctx->state, tail + SHA256_BLOCK_SIZE);

    for (unsigned int i = 0; i < 8; ++i)
        store_be32(out + i * 4U, ctx->state[i]);

    (void)crypto_secure_zero(tail, sizeof(tail));
    (void)crypto_secure_zero(ctx, sizeof(*ctx));
    return CRYPTO_OK;
}

int sha256(const void *data, usize len, u8 out[SHA256_DIGEST_SIZE])
{
    struct sha256_ctx ctx;
    int result;

    if (!out || (len && !data))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    result = sha256_init(&ctx);
    if (result != CRYPTO_OK)
        return result;
    result = sha256_update(&ctx, data, len);
    if (result != CRYPTO_OK) {
        (void)crypto_secure_zero(&ctx, sizeof(ctx));
        return result;
    }
    return sha256_final(&ctx, out);
}
