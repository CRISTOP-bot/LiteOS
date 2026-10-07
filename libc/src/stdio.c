#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

struct format_output {
    char *buffer;
    size_t capacity;
    size_t length;
};

static int append_char(struct format_output *out, char ch)
{
    if (out->length == (size_t)__INT_MAX__)
        return -1;
    if (out->length + 1 < out->capacity)
        out->buffer[out->length] = ch;
    out->length++;
    return 0;
}

static int append_string(struct format_output *out, const char *text)
{
    if (!text)
        text = "(null)";
    while (*text) {
        if (append_char(out, *text++) < 0)
            return -1;
    }
    return 0;
}

static int append_number(struct format_output *out, uint64_t value,
                         unsigned int base, int width)
{
    static const char digits[] = "0123456789abcdef";
    char reversed[sizeof(value) * 8];
    int length = 0;
    do {
        reversed[length++] = digits[value % base];
        value /= base;
    } while (value);
    while (length < width)
        reversed[length++] = '0';
    while (length) {
        if (append_char(out, reversed[--length]) < 0)
            return -1;
    }
    return 0;
}

int snprintf(char *buffer, size_t capacity, const char *format, ...)
{
    if (!format || (capacity && !buffer))
        return -1;

    struct format_output out = { buffer, capacity, 0 };
    int result = 0;
    va_list args;
    va_start(args, format);
    for (const char *p = format; *p && result == 0; p++) {
        if (*p != '%') {
            result = append_char(&out, *p);
            continue;
        }
        p++;
        switch (*p) {
        case 's':
            result = append_string(&out, va_arg(args, const char *));
            break;
        case 'd': {
            int value = va_arg(args, int);
            if (value < 0) {
                result = append_char(&out, '-');
                if (result < 0)
                    break;
            }
            uint64_t magnitude = value < 0 ? (uint64_t)-(int64_t)value : (uint64_t)value;
            result = append_number(&out, magnitude, 10, 0);
            break;
        }
        case 'u':
            result = append_number(&out, va_arg(args, unsigned int), 10, 0);
            break;
        case 'x':
            result = append_number(&out, va_arg(args, unsigned int), 16, 0);
            break;
        case 'p':
            result = append_string(&out, "0x");
            if (result == 0)
                result = append_number(&out, (uintptr_t)va_arg(args, void *),
                                       16, (int)(sizeof(uintptr_t) * 2));
            break;
        case 'c':
            result = append_char(&out, (char)va_arg(args, int));
            break;
        case '%':
            result = append_char(&out, '%');
            break;
        case '\0':
            result = append_char(&out, '%');
            p--;
            break;
        default:
            result = append_char(&out, '%');
            if (result == 0)
                result = append_char(&out, *p);
            break;
        }
    }
    va_end(args);
    if (capacity)
        buffer[out.length < capacity ? out.length : capacity - 1] = '\0';
    return result < 0 ? -1 : (int)out.length;
}
