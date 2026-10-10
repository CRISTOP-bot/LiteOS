#include <stdint.h>
#include <string.h>

int main(void)
{
    char forward[] = "abcdef";
    char backward[] = "abcdef";
    char separated[16] = "abc";
    char reverse_separated[] = "abcdefghijklmno";
    unsigned char bytes[] = { 'a', 0xff, 'z', 0 };
    char text[] = "LiteOS";

    if (memmove(forward + 2, forward, 4) != forward + 2 ||
        memcmp(forward, "ababcd", 6) != 0)
        return 1;
    if (memmove(backward, backward + 2, 4) != backward ||
        memcmp(backward, "cdefef", 6) != 0)
        return 2;
    if (memmove(separated + 8, separated, 3) != separated + 8 ||
        memcmp(separated + 8, "abc", 3) != 0)
        return 3;
    if (memmove(reverse_separated, reverse_separated + 8, 3) !=
            reverse_separated ||
        memcmp(reverse_separated, "ijk", 3) != 0)
        return 4;
    if (memmove(separated, separated, sizeof(separated)) != separated)
        return 5;
    if (memmove(0, 0, 0) != 0)
        return 6;

    if (strchr(text, 'O') != text + 4 ||
        strchr(text, '\0') != text + 6 ||
        strchr(text, 'x') != 0)
        return 7;
    if (strchr((const char *)bytes, 0xff) != (char *)&bytes[1] ||
        strchr((const char *)bytes, 0x1ff) != (char *)&bytes[1])
        return 8;

    return 0;
}
