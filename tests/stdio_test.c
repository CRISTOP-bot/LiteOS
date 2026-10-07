#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    char buffer[64];
    char marker = 'X';
    int (*format_fn)(char *, size_t, const char *, ...) = snprintf;
    assert(snprintf(NULL, 0, "%s", "hello") == 5);
    assert(snprintf(&marker, 0, "%s", "hello") == 5);
    assert(marker == 'X');

    assert(snprintf(buffer, 5, "/root") == 5);
    assert(strcmp(buffer, "/roo") == 0);
    assert(snprintf(buffer, sizeof(buffer), "%d %u %x", INT_MIN, UINT_MAX, 0x2aU) == 25);
    assert(strcmp(buffer, "-2147483648 4294967295 2a") == 0);
    assert(format_fn(buffer, sizeof(buffer), "%c %% %", 'A') == 5);
    assert(strcmp(buffer, "A % %") == 0);

    assert(snprintf(buffer, sizeof(buffer), "%p", (void *)(uintptr_t)0x2a) == 18);
    assert(strcmp(buffer, "0x000000000000002a") == 0);
    assert(format_fn(buffer, sizeof(buffer), "%s", (const char *)NULL) == 6);
    assert(strcmp(buffer, "(null)") == 0);
    return 0;
}
