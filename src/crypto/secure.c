/* Small freestanding helpers shared by LiteOS cryptographic primitives. */
#include "crypto.h"

int crypto_secure_zero(void *ptr, usize len)
{
    if (len && !ptr)
        return CRYPTO_ERR_INVALID_ARGUMENT;

    volatile u8 *p = (volatile u8 *)ptr;
    while (len--)
        *p++ = 0;
    return CRYPTO_OK;
}

int crypto_ct_equal(const void *a, const void *b, usize len)
{
    if (len && (!a || !b))
        return CRYPTO_ERR_INVALID_ARGUMENT;

    const volatile u8 *x = (const volatile u8 *)a;
    const volatile u8 *y = (const volatile u8 *)b;
    u32 difference = 0;
    for (usize i = 0; i < len; ++i)
        difference |= (u32)(x[i] ^ y[i]);

    /* No early exit based on compared bytes. */
    return (int)((difference | (0U - difference)) >> 31);
}
