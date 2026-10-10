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

static void fill(u8 *out, usize len, u8 value)
{
    for (usize i = 0; i < len; ++i)
        out[i] = value;
}

static int test_hmac(void)
{
    static const char sha256_case1[] =
        "b0344c61d8db38535ca8afceaf0bf12b"
        "881dc200c9833da726e9376c2e32cff7";
    static const char sha512_case1[] =
        "87aa7cdea5ef619d4ff0b4241a1d6cb0"
        "2379f4e2ce4ec2787ad0b30545e17cde"
        "daa833b7d6b8a702038b274eaea3f4e4"
        "be9d914eeb61f1702e696c203a126854";
    static const char sha256_long_key[] =
        "60e431591ee0b67f0d8a26aacbf5b77f"
        "8e0bc6213728c5140546040f0ee37f54";
    static const char sha512_long_key[] =
        "80b24263c7c1a3ebb71493c1dd7be8b4"
        "9b46d1f41b4aeec1121b013783f8f352"
        "6b56d037e05f2598bd0fd2215d6a1e52"
        "95e64f73f63f0aec8b915a985d786598";
    u8 key20[20];
    u8 key131[131];
    u8 digest[SHA512_DIGEST_SIZE];
    u8 overlap[SHA512_DIGEST_SIZE] = { 'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e' };
    struct hmac_sha256_ctx ctx;
    struct hmac_sha512_ctx ctx512;
    u8 cleared[sizeof(ctx)] = { 0 };
    u8 cleared512[sizeof(ctx512)] = { 0 };
    static const char long_message[] =
        "Test Using Larger Than Block-Size Key - Hash Key First";

    fill(key20, sizeof(key20), 0x0b);
    fill(key131, sizeof(key131), 0xaa);
    if (hmac_sha256(key20, sizeof(key20), "Hi There", 8, digest) != CRYPTO_OK ||
        !matches_hex(digest, SHA256_DIGEST_SIZE, sha256_case1))
        return 1;
    if (hmac_sha512(key20, sizeof(key20), "Hi There", 8, digest) != CRYPTO_OK ||
        !matches_hex(digest, SHA512_DIGEST_SIZE, sha512_case1))
        return 2;
    if (hmac_sha256(key131, sizeof(key131), long_message,
                    sizeof(long_message) - 1, digest) != CRYPTO_OK ||
        !matches_hex(digest, SHA256_DIGEST_SIZE, sha256_long_key))
        return 3;
    if (hmac_sha512(key131, sizeof(key131), long_message,
                    sizeof(long_message) - 1, digest) != CRYPTO_OK ||
        !matches_hex(digest, SHA512_DIGEST_SIZE, sha512_long_key))
        return 4;
    if (hmac_sha256(key20, sizeof(key20), overlap, 8, overlap) != CRYPTO_OK ||
        !matches_hex(overlap, SHA256_DIGEST_SIZE, sha256_case1))
        return 9;

    if (hmac_sha256_init(&ctx, key20, sizeof(key20)) != CRYPTO_OK ||
        hmac_sha256_update(&ctx, "Hi ", 3) != CRYPTO_OK ||
        hmac_sha256_update(&ctx, NULL, 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        hmac_sha256_update(&ctx, "There", 5) != CRYPTO_OK ||
        hmac_sha256_final(&ctx, digest) != CRYPTO_OK ||
        !matches_hex(digest, SHA256_DIGEST_SIZE, sha256_case1))
        return 5;
    if (crypto_ct_equal(&ctx, cleared, sizeof(ctx)) != 0 ||
        hmac_sha256_update(&ctx, "x", 1) != CRYPTO_ERR_INVALID_ARGUMENT ||
        hmac_sha256_final(&ctx, digest) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 6;

    if (hmac_sha512_init(&ctx512, key20, sizeof(key20)) != CRYPTO_OK ||
        hmac_sha512_update(&ctx512, "Hi", 2) != CRYPTO_OK ||
        hmac_sha512_update(&ctx512, " There", 6) != CRYPTO_OK ||
        hmac_sha512_final(&ctx512, digest) != CRYPTO_OK ||
        !matches_hex(digest, SHA512_DIGEST_SIZE, sha512_case1) ||
        crypto_ct_equal(&ctx512, cleared512, sizeof(ctx512)) != 0)
        return 8;

    if (hmac_sha256(NULL, 1, "x", 1, digest) != CRYPTO_ERR_INVALID_ARGUMENT ||
        hmac_sha512("k", 1, NULL, 1, digest) != CRYPTO_ERR_INVALID_ARGUMENT ||
        hmac_sha256("k", 1, "", 0, NULL) != CRYPTO_ERR_INVALID_ARGUMENT ||
        hmac_sha512_init(NULL, NULL, 0) != CRYPTO_ERR_INVALID_ARGUMENT)
        return 7;
    (void)crypto_secure_zero(key20, sizeof(key20));
    (void)crypto_secure_zero(key131, sizeof(key131));
    (void)crypto_secure_zero(digest, sizeof(digest));
    return 0;
}

static int test_hkdf(void)
{
    static const char case1_prk[] =
        "077709362c2e32df0ddc3f0dc47bba63"
        "90b6c73bb50f9c3122ec844ad7c2b3e5";
    static const char case1_okm[] =
        "3cb25f25faacd57a90434f64d0362f2a"
        "2d2d0a90cf1a5a4c5db02d56ecc4c5bf"
        "34007208d5b887185865";
    static const char case2_okm[] =
        "b11e398dc80327a1c8e7f78c596a4934"
        "4f012eda2d4efad8a050cc4c19afa97c"
        "59045a99cac7827271cb41c65e590e09"
        "da3275600c2f09b8367793a9aca3db71"
        "cc30c58179ec3e87c14c01d5c1f3434f"
        "1d87";
    static const char case3_okm[] =
        "8da4e775a563c18f715f802a063c5a31"
        "b8a11f5c5ee1879ec3454e5f3c738d2d"
        "9d201395faa4b61a96c8";
    static const char sha512_prk[] =
        "665799823737ded04a88e47e54a5890b"
        "b2c3d247c7a4254a8e61350723590a26"
        "c36238127d8661b88cf80ef802d57e2f"
        "7cebcf1e00e083848be19929c61b4237";
    static const char sha512_okm[] =
        "832390086cda71fb47625bb5ceb168e4"
        "c8e26a1a16ed34d9fc7fe92c1481579"
        "338da362cb8d9f925d7cbcce0dff7098"
        "769cf15959867d571c1715450cb53013"
        "7be3fb62f3cf32b84feba8f1eb1b563e"
        "20d97";
    u8 ikm[80];
    u8 salt[80];
    u8 info[80];
    u8 prk[SHA512_DIGEST_SIZE];
    u8 okm[82];
    u8 too_short[1] = { 0xa5 };

    fill(ikm, 22, 0x0b);
    for (usize i = 0; i < sizeof(salt); ++i) {
        salt[i] = (u8)i;
        info[i] = (u8)(0xf0U + i);
        ikm[i] = (u8)i;
    }
    fill(ikm, 22, 0x0b);

    if (hkdf_sha256_extract(salt, 13, ikm, 22, prk) != CRYPTO_OK ||
        !matches_hex(prk, SHA256_DIGEST_SIZE, case1_prk))
        return 1;
    if (hkdf_sha256_expand(prk, info, 10, okm, 42) != CRYPTO_OK ||
        !matches_hex(okm, 42, case1_okm))
        return 2;
    if (hkdf_sha256(salt, 13, ikm, 22, info, 10, okm, 42) != CRYPTO_OK ||
        !matches_hex(okm, 42, case1_okm))
        return 3;

    for (usize i = 0; i < sizeof(ikm); ++i) {
        ikm[i] = (u8)i;
        salt[i] = (u8)(0x60U + i);
        info[i] = (u8)(0xb0U + i);
    }
    if (hkdf_sha256(salt, 80, ikm, 80, info, 80, okm, 82) != CRYPTO_OK ||
        !matches_hex(okm, 82, case2_okm))
        return 4;

    fill(ikm, 22, 0x0b);
    if (hkdf_sha256(NULL, 0, ikm, 22, NULL, 0, okm, 42) != CRYPTO_OK ||
        !matches_hex(okm, 42, case3_okm))
        return 5;

    for (usize i = 0; i < 13; ++i)
        salt[i] = (u8)i;
    for (usize i = 0; i < 10; ++i)
        info[i] = (u8)(0xf0U + i);
    if (hkdf_sha512_extract(salt, 13, ikm, 22, prk) != CRYPTO_OK ||
        !matches_hex(prk, SHA512_DIGEST_SIZE, sha512_prk))
        return 6;
    if (hkdf_sha512(salt, 13, ikm, 22, info, 10, okm, 82) != CRYPTO_OK ||
        !matches_hex(okm, 82, sha512_okm))
        return 7;

    too_short[0] = 0xa5;
    if (hkdf_sha256(NULL, 1, ikm, 22, NULL, 0, okm, 42) !=
            CRYPTO_ERR_INVALID_ARGUMENT ||
        hkdf_sha512(salt, 13, ikm, 22, NULL, 1, okm, 42) !=
            CRYPTO_ERR_INVALID_ARGUMENT ||
        hkdf_sha256_expand(NULL, NULL, 0, okm, 1) !=
            CRYPTO_ERR_INVALID_ARGUMENT ||
        hkdf_sha256_expand(prk, NULL, 0, too_short,
                           HKDF_SHA256_MAX_OUTPUT_SIZE + 1U) !=
            CRYPTO_ERR_INVALID_ARGUMENT ||
        too_short[0] != 0xa5)
        return 8;
    if (hkdf_sha256(NULL, 0, NULL, 0, NULL, 0, NULL, 0) != CRYPTO_OK ||
        hkdf_sha512(NULL, 0, NULL, 0, NULL, 0, NULL, 0) != CRYPTO_OK)
        return 9;

    (void)crypto_secure_zero(ikm, sizeof(ikm));
    (void)crypto_secure_zero(salt, sizeof(salt));
    (void)crypto_secure_zero(info, sizeof(info));
    (void)crypto_secure_zero(prk, sizeof(prk));
    (void)crypto_secure_zero(okm, sizeof(okm));
    return 0;
}

static int test_pbkdf2(void)
{
    static const char rfc7914_case1[] =
        "55ac046e56e3089fec1691c22544b605"
        "f94185216dde0465e68b9d57c20dacbc"
        "49ca9cccf179b645991664b39d77ef31"
        "7c71b845b1e30bd509112041d3a19783";
    static const char rfc7914_case2[] =
        "4ddcd8f60b98be21830cee5ef22701f9"
        "641a4418d04c0414aeff08876b34ab56"
        "a1d425a1225833549adb841b51c9b317"
        "6a272bdebba1d078478f62b397f33c8d";
    u8 derived[64];
    u8 sentinel[1] = { 0xa5 };

    if (pbkdf2_hmac_sha256("passwd", 6, "salt", 4, 1,
                           derived, sizeof(derived)) != CRYPTO_OK ||
        !matches_hex(derived, sizeof(derived), rfc7914_case1))
        return 1;
    if (pbkdf2_hmac_sha256("Password", 8, "NaCl", 4, 80000,
                           derived, sizeof(derived)) != CRYPTO_OK ||
        !matches_hex(derived, sizeof(derived), rfc7914_case2))
        return 2;
    if (pbkdf2_hmac_sha256(NULL, 0, NULL, 0, 0, sentinel, 1) !=
            CRYPTO_ERR_INVALID_ARGUMENT ||
        pbkdf2_hmac_sha256(NULL, 0, NULL, 0, 1, NULL, 1) !=
            CRYPTO_ERR_INVALID_ARGUMENT ||
        pbkdf2_hmac_sha256(NULL, 0, NULL, 0, 1, NULL, 0) != CRYPTO_OK ||
        pbkdf2_hmac_sha256(NULL, 0, NULL, 0, 1, sentinel,
                           (usize)0x2000000000ULL) !=
            CRYPTO_ERR_INVALID_ARGUMENT ||
        sentinel[0] != 0xa5)
        return 3;
    (void)crypto_secure_zero(derived, sizeof(derived));
    return 0;
}

int main(void)
{
    int result = test_hmac();
    if (result)
        return 10 + result;
    result = test_hkdf();
    if (result)
        return 30 + result;
    result = test_pbkdf2();
    if (result)
        return 50 + result;
    return 0;
}
