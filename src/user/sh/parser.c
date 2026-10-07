#include "parser.h"
#include <string.h>

static int special(char c)
{
    return c == '|' || c == '<' || c == '>';
}

static int space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static char *word(const char **cursor, char **output, size_t *remaining)
{
    const char *p = *cursor;
    char *start = *output;
    char quote = 0;
    int seen = 0;

    while (*p && (quote || (!space(*p) && !special(*p)))) {
        char c = *p++;
        if (c == '\\' && *p && quote != '\'') {
            c = *p++;
        } else if (c == '\'' || c == '"') {
            if (quote == c) {
                quote = 0;
                seen = 1;
                continue;
            }
            if (!quote) {
                quote = c;
                seen = 1;
                continue;
            }
        }
        if (*remaining < 2)
            return 0;
        *(*output)++ = c;
        --*remaining;
        seen = 1;
    }
    if (quote || !seen || !*remaining)
        return 0;
    *(*output)++ = '\0';
    --*remaining;
    *cursor = p;
    return start;
}

int sh_parse(const char *line, char *storage, size_t capacity,
             struct sh_command commands[SH_MAX_COMMANDS])
{
    if (!line || !storage || !capacity || !commands)
        return -1;
    memset(commands, 0, sizeof(*commands) * SH_MAX_COMMANDS);
    char *out = storage;
    int count = 1;
    while (*line) {
        while (space(*line))
            line++;
        if (!*line)
            break;
        struct sh_command *cmd = &commands[count - 1];
        if (*line == '|') {
            if (!cmd->argc || count == SH_MAX_COMMANDS)
                return -1;
            count++;
            line++;
            continue;
        }
        if (*line == '<' || *line == '>') {
            char op = *line++;
            int append = op == '>' && *line == '>';
            if (append)
                line++;
            while (space(*line))
                line++;
            if (!*line || special(*line))
                return -1;
            char *name = word(&line, &out, &capacity);
            if (!name)
                return -1;
            if (op == '<') {
                if (cmd->input)
                    return -1;
                cmd->input = name;
            } else {
                if (cmd->output)
                    return -1;
                cmd->output = name;
                cmd->append = append;
            }
            continue;
        }
        if (cmd->argc == SH_MAX_ARGS)
            return -1;
        char *arg = word(&line, &out, &capacity);
        if (!arg)
            return -1;
        cmd->argv[cmd->argc++] = arg;
    }
    return commands[count - 1].argc ? count : (count == 1 ? 0 : -1);
}
