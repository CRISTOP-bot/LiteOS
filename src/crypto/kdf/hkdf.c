/* HKDF-SHA-256 and HKDF-SHA-512 (RFC 5869). */
#include "crypto.h"

static void copy_bytes(u8 *dst, const u8 *src, usize len)
{
    for (usize i = 0; i < len; ++i)
        dst[i] = src[i];
}

int hkdf_sha256_extract(const void *salt, usize salt_len,
                        const void *ikm, usize ikm_len,
                        u8 prk[HKDF_SHA256_PRK_SIZE])
{
    u8 default_salt[SHA256_DIGEST_SIZE] = { 0 };
    const void *key = salt;
    usize key_len = salt_len;
    int result;

    if (!prk || (salt_len && !salt) || (ikm_len && !ikm))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    if (!salt && !salt_len) {
        key = default_salt;
        key_len = sizeof(default_salt);
    }

    result = hmac_sha256(key, key_len, ikm, ikm_len, prk);
    (void)crypto_secure_zero(default_salt, sizeof(default_salt));
    return result;
}

int hkdf_sha256_expand(const u8 prk[HKDF_SHA256_PRK_SIZE],
                       const void *info, usize info_len,
                       void *okm, usize okm_len)
{
    struct hmac_sha256_ctx ctx = { 0 };
    u8 key[SHA256_DIGEST_SIZE] = { 0 };
    u8 previous[SHA256_DIGEST_SIZE] = { 0 };
    u8 counter = 1;
    usize previous_len = 0;
    usize written = 0;
    int result = CRYPTO_OK;

    if (!prk || (info_len && !info) || (okm_len && !okm) ||
        okm_len > HKDF_SHA256_MAX_OUTPUT_SIZE)
        return CRYPTO_ERR_INVALID_ARGUMENT;
    if (!okm_len)
        return CRYPTO_OK;

    copy_bytes(key, prk, sizeof(key));
    while (written < okm_len) {
        result = hmac_sha256_init(&ctx, key, sizeof(key));
        if (result == CRYPTO_OK && previous_len)
            result = hmac_sha256_update(&ctx, previous, previous_len);
        if (result == CRYPTO_OK && info_len)
            result = hmac_sha256_update(&ctx, info, info_len);
        if (result == CRYPTO_OK)
            result = hmac_sha256_update(&ctx, &counter, sizeof(counter));
        if (result == CRYPTO_OK)
            result = hmac_sha256_final(&ctx, previous);
        if (result != CRYPTO_OK) {
            (void)crypto_secure_zero(&ctx, sizeof(ctx));
            (void)crypto_secure_zero(okm, okm_len);
            goto done;
        }

        previous_len = sizeof(previous);
        usize take = okm_len - written;
        if (take > sizeof(previous))
            take = sizeof(previous);
        copy_bytes((u8 *)okm + written, previous, take);
        written += take;
        ++counter;
    }

done:
    (void)crypto_secure_zero(&ctx, sizeof(ctx));
    (void)crypto_secure_zero(key, sizeof(key));
    (void)crypto_secure_zero(previous, sizeof(previous));
    return result;
}

int hkdf_sha256(const void *salt, usize salt_len,
                const void *ikm, usize ikm_len,
                const void *info, usize info_len,
                void *okm, usize okm_len)
{
    u8 prk[HKDF_SHA256_PRK_SIZE] = { 0 };
    int result;

    if ((salt_len && !salt) || (ikm_len && !ikm) ||
        (info_len && !info) || (okm_len && !okm) ||
        okm_len > HKDF_SHA256_MAX_OUTPUT_SIZE)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    result = hkdf_sha256_extract(salt, salt_len, ikm, ikm_len, prk);
    if (result == CRYPTO_OK)
        result = hkdf_sha256_expand(prk, info, info_len, okm, okm_len);
    (void)crypto_secure_zero(prk, sizeof(prk));
    return result;
}

int hkdf_sha512_extract(const void *salt, usize salt_len,
                        const void *ikm, usize ikm_len,
                        u8 prk[HKDF_SHA512_PRK_SIZE])
{
    u8 default_salt[SHA512_DIGEST_SIZE] = { 0 };
    const void *key = salt;
    usize key_len = salt_len;
    int result;

    if (!prk || (salt_len && !salt) || (ikm_len && !ikm))
        return CRYPTO_ERR_INVALID_ARGUMENT;
    if (!salt && !salt_len) {
        key = default_salt;
        key_len = sizeof(default_salt);
    }

    result = hmac_sha512(key, key_len, ikm, ikm_len, prk);
    (void)crypto_secure_zero(default_salt, sizeof(default_salt));
    return result;
}

int hkdf_sha512_expand(const u8 prk[HKDF_SHA512_PRK_SIZE],
                       const void *info, usize info_len,
                       void *okm, usize okm_len)
{
    struct hmac_sha512_ctx ctx = { 0 };
    u8 key[SHA512_DIGEST_SIZE] = { 0 };
    u8 previous[SHA512_DIGEST_SIZE] = { 0 };
    u8 counter = 1;
    usize previous_len = 0;
    usize written = 0;
    int result = CRYPTO_OK;

    if (!prk || (info_len && !info) || (okm_len && !okm) ||
        okm_len > HKDF_SHA512_MAX_OUTPUT_SIZE)
        return CRYPTO_ERR_INVALID_ARGUMENT;
    if (!okm_len)
        return CRYPTO_OK;

    copy_bytes(key, prk, sizeof(key));
    while (written < okm_len) {
        result = hmac_sha512_init(&ctx, key, sizeof(key));
        if (result == CRYPTO_OK && previous_len)
            result = hmac_sha512_update(&ctx, previous, previous_len);
        if (result == CRYPTO_OK && info_len)
            result = hmac_sha512_update(&ctx, info, info_len);
        if (result == CRYPTO_OK)
            result = hmac_sha512_update(&ctx, &counter, sizeof(counter));
        if (result == CRYPTO_OK)
            result = hmac_sha512_final(&ctx, previous);
        if (result != CRYPTO_OK) {
            (void)crypto_secure_zero(&ctx, sizeof(ctx));
            (void)crypto_secure_zero(okm, okm_len);
            goto done;
        }

        previous_len = sizeof(previous);
        usize take = okm_len - written;
        if (take > sizeof(previous))
            take = sizeof(previous);
        copy_bytes((u8 *)okm + written, previous, take);
        written += take;
        ++counter;
    }

done:
    (void)crypto_secure_zero(&ctx, sizeof(ctx));
    (void)crypto_secure_zero(key, sizeof(key));
    (void)crypto_secure_zero(previous, sizeof(previous));
    return result;
}

int hkdf_sha512(const void *salt, usize salt_len,
                const void *ikm, usize ikm_len,
                const void *info, usize info_len,
                void *okm, usize okm_len)
{
    u8 prk[HKDF_SHA512_PRK_SIZE] = { 0 };
    int result;

    if ((salt_len && !salt) || (ikm_len && !ikm) ||
        (info_len && !info) || (okm_len && !okm) ||
        okm_len > HKDF_SHA512_MAX_OUTPUT_SIZE)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    result = hkdf_sha512_extract(salt, salt_len, ikm, ikm_len, prk);
    if (result == CRYPTO_OK)
        result = hkdf_sha512_expand(prk, info, info_len, okm, okm_len);
    (void)crypto_secure_zero(prk, sizeof(prk));
    return result;
}
