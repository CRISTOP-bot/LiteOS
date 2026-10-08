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
    static const char sha256_empty[] =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    static const char sha512_empty[] =
        "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
        "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e";
    u8 digest[64];
    char a56[56];
    char a100[100];
    char a112[112];
    char a200[200];

    for (usize i = 0; i < sizeof(a56); ++i)
        a56[i] = 'a';
    for (usize i = 0; i < sizeof(a112); ++i)
        a112[i] = 'a';
    for (usize i = 0; i < sizeof(a100); ++i)
        a100[i] = 'a';
    for (usize i = 0; i < sizeof(a200); ++i)
        a200[i] = 'a';

    if (sha256(NULL, 0, digest) || !matches_hex(digest, 32, sha256_empty))
        return 1;
    if (sha256("abc", 3, digest) || !matches_hex(digest, 32, sha256_abc))
        return 2;
    if (sha256(a56, sizeof(a56), digest) || !matches_hex(digest, 32, sha256_56a))
        return 3;
    if (sha256(a100, sizeof(a100), digest) || !matches_hex(digest, 32, sha256_100a))
        return 4;
    if (!sha256(NULL, 1, digest) || !sha256("abc", 3, NULL))
        return 5;

    if (sha512(NULL, 0, digest) || !matches_hex(digest, 64, sha512_empty))
        return 6;
    if (sha512("abc", 3, digest) || !matches_hex(digest, 64, sha512_abc))
        return 7;
    if (sha512(a112, sizeof(a112), digest) || !matches_hex(digest, 64, sha512_112a))
        return 8;
    if (sha512(a200, sizeof(a200), digest) || !matches_hex(digest, 64, sha512_200a))
        return 9;
    if (!sha512(NULL, 1, digest) || !sha512("abc", 3, NULL))
        return 10;

    return 0;
}
