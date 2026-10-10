#include "crypto.h"

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

static int matches_hex(const u8 *actual, usize len, const char *expected)
{
    for (usize i = 0; i < len; ++i) {
        int high = hex_value(expected[i * 2]);
        int low = hex_value(expected[i * 2 + 1]);
        if (high < 0 || low < 0 || actual[i] != (u8)((high << 4) | low))
            return 0;
    }
    return expected[len * 2] == '\0';
}

static void fill_a(u8 *buffer, usize len)
{
    for (usize i = 0; i < len; ++i)
        buffer[i] = 'a';
}

static int test_secure_helpers(void)
{
    u8 secret[16];
    const u8 equal_a[] = { 0x00, 0x52, 0xff, 0x17, 0xa0 };
    const u8 equal_b[] = { 0x00, 0x52, 0xff, 0x17, 0xa0 };
    u8 different[sizeof(equal_a)];

    for (usize i = 0; i < sizeof(secret); ++i)
        secret[i] = (u8)(0xa5U ^ i);
    if (crypto_secure_zero(secret, sizeof(secret)) != CRYPTO_OK)
        return 1;
    for (usize i = 0; i < sizeof(secret); ++i)
        if (secret[i] != 0)
            return 2;
    if (crypto_secure_zero(NULL, 0) != CRYPTO_OK ||
        crypto_secure_zero(NULL, 1) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 3;

    if (crypto_ct_equal(equal_a, equal_b, sizeof(equal_a)) != 0 ||
        crypto_ct_equal(NULL, NULL, 0) != 0 ||
        crypto_ct_equal(NULL, equal_b, sizeof(equal_b)) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 4;
    for (usize changed = 0; changed < sizeof(equal_a); ++changed) {
        for (usize i = 0; i < sizeof(different); ++i)
            different[i] = equal_a[i];
        different[changed] ^= 1;
        if (crypto_ct_equal(equal_a, different, sizeof(equal_a)) != 1)
            return 5;
    }
    return 0;
}

static int test_sha256_streaming(const u8 *a100, const char *expected)
{
    struct sha256_ctx ctx;
    u8 digest[SHA256_DIGEST_SIZE];
    u8 zero_ctx[sizeof(ctx)] = { 0 };

    if (sha256_init(&ctx) != CRYPTO_OK ||
        sha256_update(&ctx, NULL, 0) != CRYPTO_OK ||
        sha256_update(&ctx, a100, 1) != CRYPTO_OK ||
        sha256_update(&ctx, NULL, 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha256_update(&ctx, a100 + 1, 63) != CRYPTO_OK ||
        sha256_update(&ctx, a100 + 64, 36) != CRYPTO_OK ||
        sha256_final(&ctx, digest) != CRYPTO_OK ||
        !matches_hex(digest, sizeof(digest), expected))
        return 1;
    if (crypto_ct_equal(&ctx, zero_ctx, sizeof(ctx)) != 0 ||
        sha256_update(&ctx, a100, 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha256_final(&ctx, digest) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 2;
    if (sha256_init(&ctx) != CRYPTO_OK ||
        sha256_update(&ctx, "abc", 1) != CRYPTO_OK ||
        sha256_update(&ctx, "bc", 2) != CRYPTO_OK ||
        sha256_final(&ctx, digest) != CRYPTO_OK ||
        !matches_hex(digest, sizeof(digest),
                     "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"))
        return 3;
    return 0;
}

static int test_sha512_streaming(const u8 *a200, const char *expected)
{
    struct sha512_ctx ctx;
    u8 digest[SHA512_DIGEST_SIZE];
    u8 zero_ctx[sizeof(ctx)] = { 0 };

    if (sha512_init(&ctx) != CRYPTO_OK ||
        sha512_update(&ctx, NULL, 0) != CRYPTO_OK ||
        sha512_update(&ctx, a200, 127) != CRYPTO_OK ||
        sha512_update(&ctx, NULL, 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha512_update(&ctx, a200 + 127, 1) != CRYPTO_OK ||
        sha512_update(&ctx, a200 + 128, 72) != CRYPTO_OK ||
        sha512_final(&ctx, digest) != CRYPTO_OK ||
        !matches_hex(digest, sizeof(digest), expected))
        return 1;
    if (crypto_ct_equal(&ctx, zero_ctx, sizeof(ctx)) != 0 ||
        sha512_update(&ctx, a200, 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha512_final(&ctx, digest) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 2;
    if (sha512_init(&ctx) != CRYPTO_OK ||
        sha512_update(&ctx, "abc", 1) != CRYPTO_OK ||
        sha512_update(&ctx, "bc", 2) != CRYPTO_OK ||
        sha512_final(&ctx, digest) != CRYPTO_OK ||
        !matches_hex(digest, sizeof(digest),
                     "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                     "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"))
        return 3;
    return 0;
}

int main(void)
{
    static const char sha256_abc[] =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    static const char sha256_56a[] =
        "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a";
    static const char sha256_100a[] =
        "2816597888e4a0d3a36b82b83316ab32680eb8f00f8cd3b904d681246d285a0e";
    static const char sha512_abc[] =
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
        "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f";
    static const char sha512_112a[] =
        "c01d080efd492776a1c43bd23dd99d0a2e626d481e16782e75d54c2503b5dc32"
        "bd05f0f1ba33e568b88fd2d970929b719ecbb152f58f130a407c8830604b70ca";
    static const char sha512_200a[] =
        "4b11459c33f52a22ee8236782714c150a3b2c60994e9acee17fe68947a3e6789"
        "f31e7668394592da7bef827cddca88c4e6f86e4df7ed1ae6cba71f3e98faee9f";
    static const char sha256_55a[] =
        "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318";
    static const char sha256_63a[] =
        "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34";
    static const char sha256_64a[] =
        "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb";
    static const char sha256_65a[] =
        "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0";
    static const char sha512_111a[] =
        "fa9121c7b32b9e01733d034cfc78cbf67f926c7ed83e82200ef8681819692176"
        "0b4beff48404df811b953828274461673c68d04e297b0eb7b2b4d60fc6b566a2";
    static const char sha512_127a[] =
        "828613968b501dc00a97e08c73b118aa8876c26b8aac93df128502ab360f91ba"
        "b50a51e088769a5c1eff4782ace147dce3642554199876374291f5d921629502";
    static const char sha512_128a[] =
        "b73d1929aa615934e61a871596b3f3b33359f42b8175602e89f7e06e5f658a24"
        "3667807ed300314b95cacdd579f3e33abdfbe351909519a846d465c59582f321";
    static const char sha512_129a[] =
        "4f681e0bd53cda4b5a2041cc8a06f2eabde44fb16c951fbd5b87702f07aeab61"
        "1565b19c47fde30587177ebb852e3971bbd8d3fd30da18d71037dfbd98420429";
    static const char sha256_empty[] =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    static const char sha512_empty[] =
        "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
        "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e";
    u8 digest[SHA512_DIGEST_SIZE];
    u8 a257[257];
    u8 inplace[SHA512_DIGEST_SIZE];
    struct sha256_ctx sha256_overflow;
    struct sha512_ctx sha512_overflow;

    fill_a(a257, sizeof(a257));
    if (sha256_init(NULL) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha256_update(NULL, NULL, 0) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha256_final(NULL, digest) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha512_init(NULL) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha512_update(NULL, NULL, 0) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha512_final(NULL, digest) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 1;
    if (test_secure_helpers())
        return 1;

    if (sha256(NULL, 0, digest) || !matches_hex(digest, 32, sha256_empty))
        return 2;
    if (sha256("abc", 3, digest) || !matches_hex(digest, 32, sha256_abc))
        return 3;
    if (sha256(a257, 56, digest) || !matches_hex(digest, 32, sha256_56a))
        return 4;
    if (sha256(a257, 100, digest) || !matches_hex(digest, 32, sha256_100a))
        return 5;
    if (sha256(a257, 55, digest) || !matches_hex(digest, 32, sha256_55a) ||
        sha256(a257, 63, digest) || !matches_hex(digest, 32, sha256_63a) ||
        sha256(a257, 64, digest) || !matches_hex(digest, 32, sha256_64a) ||
        sha256(a257, 65, digest) || !matches_hex(digest, 32, sha256_65a))
        return 6;
    if (sha256(NULL, 1, digest) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha256("abc", 3, NULL) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 7;

    if (sha256_init(&sha256_overflow) != CRYPTO_OK)
        return 8;
    sha256_overflow.total_bytes = (~(u64)0 >> 3);
    if (sha256_update(&sha256_overflow, "x", 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha256_overflow.total_bytes != (~(u64)0 >> 3) ||
        sha256_overflow.buffered != 0)
        return 9;
    (void)crypto_secure_zero(&sha256_overflow, sizeof(sha256_overflow));

    if (sha512(NULL, 0, digest) || !matches_hex(digest, 64, sha512_empty))
        return 10;
    if (sha512("abc", 3, digest) || !matches_hex(digest, 64, sha512_abc))
        return 11;
    if (sha512(a257, 112, digest) || !matches_hex(digest, 64, sha512_112a))
        return 12;
    if (sha512(a257, 200, digest) || !matches_hex(digest, 64, sha512_200a))
        return 13;
    if (sha512(a257, 111, digest) || !matches_hex(digest, 64, sha512_111a) ||
        sha512(a257, 127, digest) || !matches_hex(digest, 64, sha512_127a) ||
        sha512(a257, 128, digest) || !matches_hex(digest, 64, sha512_128a) ||
        sha512(a257, 129, digest) || !matches_hex(digest, 64, sha512_129a))
        return 14;
    if (sha512(NULL, 1, digest) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha512("abc", 3, NULL) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 15;

    if (sha512_init(&sha512_overflow) != CRYPTO_OK)
        return 16;
    sha512_overflow.total_bytes_high = (~(u64)0 >> 3);
    sha512_overflow.total_bytes_low = ~(u64)0;
    if (sha512_update(&sha512_overflow, "x", 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        sha512_overflow.total_bytes_high != (~(u64)0 >> 3) ||
        sha512_overflow.total_bytes_low != ~(u64)0)
        return 17;
    (void)crypto_secure_zero(&sha512_overflow, sizeof(sha512_overflow));

    if (test_sha256_streaming(a257, sha256_100a))
        return 18;
    if (test_sha512_streaming(a257, sha512_200a))
        return 19;

    inplace[0] = 'a'; inplace[1] = 'b'; inplace[2] = 'c';
    if (sha256(inplace, 3, inplace) || !matches_hex(inplace, 32, sha256_abc))
        return 20;
    inplace[0] = 'a'; inplace[1] = 'b'; inplace[2] = 'c';
    if (sha512(inplace, 3, inplace) || !matches_hex(inplace, 64, sha512_abc))
        return 21;

    return 0;
}
