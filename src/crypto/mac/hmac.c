/* HMAC-SHA-256 and HMAC-SHA-512 (RFC 2104). */
#include "crypto.h"

#define HMAC256_CONTEXT_ACTIVE 0x484d3236U
#define HMAC512_CONTEXT_ACTIVE 0x484d3531U

static void copy_bytes(u8 *dst, const u8 *src, usize len)
{
    for (usize i = 0; i < len; ++i)
        dst[i] = src[i];
}

static void hmac256_clear(struct hmac_sha256_ctx *ctx)
{
    (void)crypto_secure_zero(ctx, sizeof(*ctx));
}

static void hmac512_clear(struct hmac_sha512_ctx *ctx)
{
    (void)crypto_secure_zero(ctx, sizeof(*ctx));
}

int hmac_sha256_init(struct hmac_sha256_ctx *ctx, const void *key,
                     usize key_len)
{
    struct hmac_sha256_ctx next = { 0 };
    u8 key_block[SHA256_BLOCK_SIZE] = { 0 };
    u8 inner_pad[SHA256_BLOCK_SIZE];
    u8 key_digest[SHA256_DIGEST_SIZE] = { 0 };
    int result = CRYPTO_OK;

    if (!ctx || (key_len && !key))
        return CRYPTO_ERR_INVALID_ARGUMENT;

    if (key_len > SHA256_BLOCK_SIZE) {
        result = sha256(key, key_len, key_digest);
        if (result != CRYPTO_OK)
            goto done;
        copy_bytes(key_block, key_digest, sizeof(key_digest));
    } else if (key_len) {
        copy_bytes(key_block, (const u8 *)key, key_len);
    }

    for (usize i = 0; i < SHA256_BLOCK_SIZE; ++i) {
        inner_pad[i] = key_block[i] ^ 0x36U;
        next.opad[i] = key_block[i] ^ 0x5cU;
    }
    result = sha256_init(&next.inner);
    if (result == CRYPTO_OK)
        result = sha256_update(&next.inner, inner_pad, sizeof(inner_pad));
    if (result == CRYPTO_OK) {
        next.lifecycle = HMAC256_CONTEXT_ACTIVE;
        *ctx = next;
    } else {
        hmac256_clear(ctx);
    }

done:
    (void)crypto_secure_zero(&next, sizeof(next));
    (void)crypto_secure_zero(key_block, sizeof(key_block));
    (void)crypto_secure_zero(inner_pad, sizeof(inner_pad));
    (void)crypto_secure_zero(key_digest, sizeof(key_digest));
    return result;
}

int hmac_sha256_update(struct hmac_sha256_ctx *ctx, const void *data,
                       usize len)
{
    if (!ctx || ctx->lifecycle != HMAC256_CONTEXT_ACTIVE ||
        (len && !data))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    return sha256_update(&ctx->inner, data, len);
}

int hmac_sha256_final(struct hmac_sha256_ctx *ctx,
                      u8 out[HMAC_SHA256_DIGEST_SIZE])
{
    struct sha256_ctx outer = { 0 };
    u8 inner_digest[SHA256_DIGEST_SIZE] = { 0 };
    u8 result_digest[SHA256_DIGEST_SIZE] = { 0 };
    int result;

    if (!ctx || !out || ctx->lifecycle != HMAC256_CONTEXT_ACTIVE)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    result = sha256_final(&ctx->inner, inner_digest);
    if (result == CRYPTO_OK)
        result = sha256_init(&outer);
    if (result == CRYPTO_OK)
        result = sha256_update(&outer, ctx->opad, sizeof(ctx->opad));
    if (result == CRYPTO_OK)
        result = sha256_update(&outer, inner_digest, sizeof(inner_digest));
    if (result == CRYPTO_OK)
        result = sha256_final(&outer, result_digest);

    hmac256_clear(ctx);
    if (result == CRYPTO_OK)
        copy_bytes(out, result_digest, sizeof(result_digest));
    else
        (void)crypto_secure_zero(out, HMAC_SHA256_DIGEST_SIZE);

    (void)crypto_secure_zero(&outer, sizeof(outer));
    (void)crypto_secure_zero(inner_digest, sizeof(inner_digest));
    (void)crypto_secure_zero(result_digest, sizeof(result_digest));
    return result;
}

int hmac_sha256(const void *key, usize key_len, const void *data,
                usize data_len, u8 out[HMAC_SHA256_DIGEST_SIZE])
{
    struct hmac_sha256_ctx ctx = { 0 };
    int result;

    if (!out || (key_len && !key) || (data_len && !data))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    result = hmac_sha256_init(&ctx, key, key_len);
    if (result == CRYPTO_OK)
        result = hmac_sha256_update(&ctx, data, data_len);
    if (result == CRYPTO_OK)
        return hmac_sha256_final(&ctx, out);

    hmac256_clear(&ctx);
    return result;
}

int hmac_sha512_init(struct hmac_sha512_ctx *ctx, const void *key,
                     usize key_len)
{
    struct hmac_sha512_ctx next = { 0 };
    u8 key_block[SHA512_BLOCK_SIZE] = { 0 };
    u8 inner_pad[SHA512_BLOCK_SIZE];
    u8 key_digest[SHA512_DIGEST_SIZE] = { 0 };
    int result = CRYPTO_OK;

    if (!ctx || (key_len && !key))
        return CRYPTO_ERR_INVALID_ARGUMENT;

    if (key_len > SHA512_BLOCK_SIZE) {
        result = sha512(key, key_len, key_digest);
        if (result != CRYPTO_OK)
            goto done;
        copy_bytes(key_block, key_digest, sizeof(key_digest));
    } else if (key_len) {
        copy_bytes(key_block, (const u8 *)key, key_len);
    }

    for (usize i = 0; i < SHA512_BLOCK_SIZE; ++i) {
        inner_pad[i] = key_block[i] ^ 0x36U;
        next.opad[i] = key_block[i] ^ 0x5cU;
    }
    result = sha512_init(&next.inner);
    if (result == CRYPTO_OK)
        result = sha512_update(&next.inner, inner_pad, sizeof(inner_pad));
    if (result == CRYPTO_OK) {
        next.lifecycle = HMAC512_CONTEXT_ACTIVE;
        *ctx = next;
    } else {
        hmac512_clear(ctx);
    }

done:
    (void)crypto_secure_zero(&next, sizeof(next));
    (void)crypto_secure_zero(key_block, sizeof(key_block));
    (void)crypto_secure_zero(inner_pad, sizeof(inner_pad));
    (void)crypto_secure_zero(key_digest, sizeof(key_digest));
    return result;
}

int hmac_sha512_update(struct hmac_sha512_ctx *ctx, const void *data,
                       usize len)
{
    if (!ctx || ctx->lifecycle != HMAC512_CONTEXT_ACTIVE ||
        (len && !data))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    return sha512_update(&ctx->inner, data, len);
}

int hmac_sha512_final(struct hmac_sha512_ctx *ctx,
                      u8 out[HMAC_SHA512_DIGEST_SIZE])
{
    struct sha512_ctx outer = { 0 };
    u8 inner_digest[SHA512_DIGEST_SIZE] = { 0 };
    u8 result_digest[SHA512_DIGEST_SIZE] = { 0 };
    int result;

    if (!ctx || !out || ctx->lifecycle != HMAC512_CONTEXT_ACTIVE)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    result = sha512_final(&ctx->inner, inner_digest);
    if (result == CRYPTO_OK)
        result = sha512_init(&outer);
    if (result == CRYPTO_OK)
        result = sha512_update(&outer, ctx->opad, sizeof(ctx->opad));
    if (result == CRYPTO_OK)
        result = sha512_update(&outer, inner_digest, sizeof(inner_digest));
    if (result == CRYPTO_OK)
        result = sha512_final(&outer, result_digest);

    hmac512_clear(ctx);
    if (result == CRYPTO_OK)
        copy_bytes(out, result_digest, sizeof(result_digest));
    else
        (void)crypto_secure_zero(out, HMAC_SHA512_DIGEST_SIZE);

    (void)crypto_secure_zero(&outer, sizeof(outer));
    (void)crypto_secure_zero(inner_digest, sizeof(inner_digest));
    (void)crypto_secure_zero(result_digest, sizeof(result_digest));
    return result;
}

int hmac_sha512(const void *key, usize key_len, const void *data,
                usize data_len, u8 out[HMAC_SHA512_DIGEST_SIZE])
{
    struct hmac_sha512_ctx ctx = { 0 };
    int result;

    if (!out || (key_len && !key) || (data_len && !data))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    result = hmac_sha512_init(&ctx, key, key_len);
    if (result == CRYPTO_OK)
        result = hmac_sha512_update(&ctx, data, data_len);
    if (result == CRYPTO_OK)
        return hmac_sha512_final(&ctx, out);

    hmac512_clear(&ctx);
    return result;
}
