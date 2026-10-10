/* Internal cryptographic interfaces used by the LiteOS kernel. */
#ifndef LITEOS_CRYPTO_H
#define LITEOS_CRYPTO_H

#include "types.h"

#define CRYPTO_OK                    0
#define CRYPTO_ERR_INVALID_ARGUMENT (-1)

#define SHA256_DIGEST_SIZE 32U
#define SHA256_BLOCK_SIZE  64U
#define SHA512_DIGEST_SIZE 64U
#define SHA512_BLOCK_SIZE  128U

/* Caller-owned incremental contexts. Initialize before every use; do not
 * copy, pack, share concurrently, or modify their fields. A successful final
 * wipes the context, so final output must not overlap it. Update errors leave
 * an initialized context unchanged. */
struct sha256_ctx {
    u32 state[8];
    u64 total_bytes;
    usize buffered;
    u8 block[SHA256_BLOCK_SIZE];
    u32 lifecycle;
};

struct sha512_ctx {
    u64 state[8];
    u64 total_bytes_low;
    u64 total_bytes_high;
    usize buffered;
    u8 block[SHA512_BLOCK_SIZE];
    u32 lifecycle;
};

/* Zeroes len bytes using volatile stores. A nonzero len requires a valid
 * writable buffer. Returns CRYPTO_OK or CRYPTO_ERR_INVALID_ARGUMENT. */
int crypto_secure_zero(void *ptr, usize len);

/* Fixed-length comparison: 0 if equal, 1 if different, -1 on invalid input.
 * The requested length is public; this API makes no hardware-level timing
 * guarantee. */
int crypto_ct_equal(const void *a, const void *b, usize len);

int sha256_init(struct sha256_ctx *ctx);
int sha256_update(struct sha256_ctx *ctx, const void *data, usize len);
int sha256_final(struct sha256_ctx *ctx, u8 out[SHA256_DIGEST_SIZE]);

int sha512_init(struct sha512_ctx *ctx);
int sha512_update(struct sha512_ctx *ctx, const void *data, usize len);
int sha512_final(struct sha512_ctx *ctx, u8 out[SHA512_DIGEST_SIZE]);

/* One-shot SHA-2 helpers. Empty input is represented by data == NULL, len 0.
 * Output may overlap input because all input is consumed before final writes.
 * Return CRYPTO_OK or CRYPTO_ERR_INVALID_ARGUMENT. */
int sha256(const void *data, usize len, u8 out[SHA256_DIGEST_SIZE]);
int sha512(const void *data, usize len, u8 out[SHA512_DIGEST_SIZE]);

#define HMAC_SHA256_DIGEST_SIZE SHA256_DIGEST_SIZE
#define HMAC_SHA256_BLOCK_SIZE  SHA256_BLOCK_SIZE
#define HMAC_SHA512_DIGEST_SIZE SHA512_DIGEST_SIZE
#define HMAC_SHA512_BLOCK_SIZE  SHA512_BLOCK_SIZE
#define HKDF_SHA256_PRK_SIZE SHA256_DIGEST_SIZE
#define HKDF_SHA512_PRK_SIZE SHA512_DIGEST_SIZE
#define HKDF_SHA256_MAX_OUTPUT_SIZE (255U * SHA256_DIGEST_SIZE)
#define HKDF_SHA512_MAX_OUTPUT_SIZE (255U * SHA512_DIGEST_SIZE)

/* Caller-owned HMAC contexts. NULL with a zero length is a valid key or
 * message. A successful final wipes the context before writing the tag; final
 * output must not overlap the context. Update input must not overlap its active
 * context; do not copy/share active contexts. One-shot output may overlap its
 * key or message because both are consumed before the tag is written. */
struct hmac_sha256_ctx {
    struct sha256_ctx inner;
    u8 opad[HMAC_SHA256_BLOCK_SIZE];
    u32 lifecycle;
};

struct hmac_sha512_ctx {
    struct sha512_ctx inner;
    u8 opad[HMAC_SHA512_BLOCK_SIZE];
    u32 lifecycle;
};

int hmac_sha256_init(struct hmac_sha256_ctx *ctx, const void *key,
                     usize key_len);
int hmac_sha256_update(struct hmac_sha256_ctx *ctx, const void *data,
                       usize len);
int hmac_sha256_final(struct hmac_sha256_ctx *ctx,
                      u8 out[HMAC_SHA256_DIGEST_SIZE]);
int hmac_sha256(const void *key, usize key_len, const void *data,
                usize data_len, u8 out[HMAC_SHA256_DIGEST_SIZE]);

int hmac_sha512_init(struct hmac_sha512_ctx *ctx, const void *key,
                     usize key_len);
int hmac_sha512_update(struct hmac_sha512_ctx *ctx, const void *data,
                       usize len);
int hmac_sha512_final(struct hmac_sha512_ctx *ctx,
                      u8 out[HMAC_SHA512_DIGEST_SIZE]);
int hmac_sha512(const void *key, usize key_len, const void *data,
                usize data_len, u8 out[HMAC_SHA512_DIGEST_SIZE]);

/* RFC 5869. NULL/zero-length salt means the RFC default (HashLen zero
 * octets); a non-NULL empty salt is an empty HMAC key. Extract output may
 * overlap salt/IKM because inputs are consumed first. Expand output may
 * overlap PRK (copied before writing) but must not overlap info. Combined
 * output may overlap salt/IKM, but not info. */
int hkdf_sha256_extract(const void *salt, usize salt_len,
                        const void *ikm, usize ikm_len,
                        u8 prk[HKDF_SHA256_PRK_SIZE]);
int hkdf_sha256_expand(const u8 prk[HKDF_SHA256_PRK_SIZE],
                       const void *info, usize info_len,
                       void *okm, usize okm_len);
int hkdf_sha256(const void *salt, usize salt_len,
                const void *ikm, usize ikm_len,
                const void *info, usize info_len,
                void *okm, usize okm_len);

int hkdf_sha512_extract(const void *salt, usize salt_len,
                        const void *ikm, usize ikm_len,
                        u8 prk[HKDF_SHA512_PRK_SIZE]);
int hkdf_sha512_expand(const u8 prk[HKDF_SHA512_PRK_SIZE],
                       const void *info, usize info_len,
                       void *okm, usize okm_len);
int hkdf_sha512(const void *salt, usize salt_len,
                const void *ikm, usize ikm_len,
                const void *info, usize info_len,
                void *okm, usize okm_len);

/* RFC 8018 PBKDF2-HMAC-SHA-256. iterations must be nonzero and the output
 * is limited to (2^32-1) digest blocks. Output must not overlap password or
 * salt. An internal failure clears any output bytes already derived. */
int pbkdf2_hmac_sha256(const void *password, usize password_len,
                       const void *salt, usize salt_len,
                       u32 iterations, void *derived_key,
                       usize derived_key_len);

#endif
