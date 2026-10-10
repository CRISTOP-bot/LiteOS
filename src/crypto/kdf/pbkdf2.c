/* PBKDF2-HMAC-SHA-256 (RFC 8018). */
#include "crypto.h"

#define PBKDF2_SHA256_MAX_BLOCKS 0xffffffffULL

static void copy_bytes(u8 *dst, const u8 *src, usize len)
{
    for (usize i = 0; i < len; ++i)
        dst[i] = src[i];
}

int pbkdf2_hmac_sha256(const void *password, usize password_len,
                       const void *salt, usize salt_len,
                       u32 iterations, void *derived_key,
                       usize derived_key_len)
{
    struct hmac_sha256_ctx ctx = { 0 };
    u8 u[SHA256_DIGEST_SIZE] = { 0 };
    u8 accumulator[SHA256_DIGEST_SIZE] = { 0 };
    u8 block_index[4];
    u64 block_count = (u64)(derived_key_len / SHA256_DIGEST_SIZE) +
                       (derived_key_len % SHA256_DIGEST_SIZE != 0);
    usize written = 0;
    int result = CRYPTO_OK;

    if ((password_len && !password) || (salt_len && !salt) ||
        (derived_key_len && !derived_key) || !iterations ||
        block_count > PBKDF2_SHA256_MAX_BLOCKS)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    for (u64 block = 1; block <= block_count; ++block) {
        block_index[0] = (u8)(block >> 24);
        block_index[1] = (u8)(block >> 16);
        block_index[2] = (u8)(block >> 8);
        block_index[3] = (u8)block;

        result = hmac_sha256_init(&ctx, password, password_len);
        if (result == CRYPTO_OK && salt_len)
            result = hmac_sha256_update(&ctx, salt, salt_len);
        if (result == CRYPTO_OK)
            result = hmac_sha256_update(&ctx, block_index, sizeof(block_index));
        if (result == CRYPTO_OK)
            result = hmac_sha256_final(&ctx, u);
        if (result != CRYPTO_OK)
            goto failed;

        copy_bytes(accumulator, u, sizeof(accumulator));
        for (u32 iteration = 1; iteration < iterations; ++iteration) {
            result = hmac_sha256(password, password_len,
                                 u, sizeof(u), u);
            if (result != CRYPTO_OK)
                goto failed;
            for (usize i = 0; i < sizeof(accumulator); ++i)
                accumulator[i] ^= u[i];
        }

        usize take = derived_key_len - written;
        if (take > sizeof(accumulator))
            take = sizeof(accumulator);
        copy_bytes((u8 *)derived_key + written, accumulator, take);
        written += take;
        (void)crypto_secure_zero(u, sizeof(u));
        (void)crypto_secure_zero(accumulator, sizeof(accumulator));
    }

    goto done;

failed:
    (void)crypto_secure_zero(&ctx, sizeof(ctx));
    if (written)
        (void)crypto_secure_zero(derived_key, written);

done:
    (void)crypto_secure_zero(&ctx, sizeof(ctx));
    (void)crypto_secure_zero(u, sizeof(u));
    (void)crypto_secure_zero(accumulator, sizeof(accumulator));
    (void)crypto_secure_zero(block_index, sizeof(block_index));
    return result;
}
