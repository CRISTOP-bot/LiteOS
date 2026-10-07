#include <assert.h>
#include <string.h>
#include "parser.h"

static void test_quoting_and_pipe(void)
{
    struct sh_command commands[SH_MAX_COMMANDS];
    char storage[256];
    int count = sh_parse("echo 'hello world' \"a|b\" | cat > out",
                         storage, sizeof(storage), commands);
    assert(count == 2);
    assert(commands[0].argc == 3);
    assert(strcmp(commands[0].argv[1], "hello world") == 0);
    assert(strcmp(commands[0].argv[2], "a|b") == 0);
    assert(strcmp(commands[1].argv[0], "cat") == 0);
    assert(strcmp(commands[1].output, "out") == 0);
    assert(commands[1].append == 0);
}

static void test_redirection_and_errors(void)
{
    struct sh_command commands[SH_MAX_COMMANDS];
    char storage[256];
    assert(sh_parse("cat < input >> result", storage, sizeof(storage), commands) == 1);
    assert(strcmp(commands[0].input, "input") == 0);
    assert(strcmp(commands[0].output, "result") == 0);
    assert(commands[0].append == 1);
    assert(sh_parse("", storage, sizeof(storage), commands) == 0);
    assert(sh_parse("echo 'unclosed", storage, sizeof(storage), commands) == -1);
    assert(sh_parse("echo |", storage, sizeof(storage), commands) == -1);
    assert(sh_parse("| cat", storage, sizeof(storage), commands) == -1);
    assert(sh_parse("echo hi", storage, 3, commands) == -1);
    assert(sh_parse("echo a\\ b \"\"", storage, sizeof(storage), commands) == 1);
    assert(strcmp(commands[0].argv[1], "a b") == 0);
    assert(strcmp(commands[0].argv[2], "") == 0);
    assert(sh_parse("echo 'a\\b'", storage, sizeof(storage), commands) == 1);
    assert(strcmp(commands[0].argv[1], "a\\b") == 0);
    assert(sh_parse("echo a > first > second", storage, sizeof(storage), commands) == -1);
    assert(sh_parse("a | b | c | d | e | f | g | h | i",
                    storage, sizeof(storage), commands) == -1);
}

int main(void)
{
    test_quoting_and_pipe();
    test_redirection_and_errors();
    return 0;
}
