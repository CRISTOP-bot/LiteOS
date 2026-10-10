#include <stdint.h>
#include <string.h>

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    if (n == 0) return 0;
    while (n > 1 && *a && *a == *b) { a++; b++; n--; }
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++))
        ;
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    uintptr_t dst_addr, src_addr;

    if (n == 0 || dst == src)
        return dst;

    /* LiteOS is x86_64: compare flat addresses, not unrelated C pointers. */
    dst_addr = (uintptr_t)dst;
    src_addr = (uintptr_t)src;
    if (dst_addr < src_addr || dst_addr - src_addr >= n) {
        while (n--)
            *d++ = *s++;
    } else {
        while (n) {
            --n;
            d[n] = s[n];
        }
    }
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = dst;
    while (n--)
        *d++ = (unsigned char)c;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = a, *y = b;
    while (n--) {
        if (*x != *y) return *x - *y;
        x++; y++;
    }
    return 0;
}

char *strchr(const char *s, int c)
{
    unsigned char target = (unsigned char)c;

    do {
        if ((unsigned char)*s == target)
            return (char *)s;
    } while (*s++);
    return 0;
}
