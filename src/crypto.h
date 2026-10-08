/* Internal cryptographic interfaces used by the LiteOS kernel. */
#ifndef LITEOS_CRYPTO_H
#define LITEOS_CRYPTO_H

#include "types.h"

/* One-shot SHA-2 digests. Return 0 on success, -1 for invalid arguments. */
int sha256(const void *data, usize len, u8 out[32]);
int sha512(const void *data, usize len, u8 out[64]);

#endif
