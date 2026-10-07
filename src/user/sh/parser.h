#ifndef LITEOS_SHELL_PARSER_H
#define LITEOS_SHELL_PARSER_H

#include <stddef.h>

#define SH_MAX_COMMANDS 8
#define SH_MAX_ARGS 16

struct sh_command {
    char *argv[SH_MAX_ARGS + 1];
    int argc;
    char *input;
    char *output;
    int append;
};

/* Token pointers refer to storage. Returns command count, or -1 on syntax error. */
int sh_parse(const char *line, char *storage, size_t capacity,
             struct sh_command commands[SH_MAX_COMMANDS]);

#endif
