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
 * wipes the context. Update errors leave an initialized context unchanged. */
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

#endif
