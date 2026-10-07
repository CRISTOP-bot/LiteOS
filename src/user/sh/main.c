#include <sys/liteos.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include "parser.h"

#define LINE_SIZE 1024

static void output(int fd, const char *s)
{
    size_t left = strlen(s);
    while (left) {
        long written = sys_write(fd, s, left);
        if (written <= 0)
            return;
        s += written;
        left -= (size_t)written;
    }
}

static void error(const char *message, const char *path)
{
    output(2, "sh: ");
    output(2, message);
    if (path) {
        output(2, ": ");
        output(2, path);
    }
    output(2, "\n");
}

static int copy_file(int fd)
{
    char buf[256];
    long count;
    while ((count = sys_read(fd, buf, sizeof(buf))) > 0) {
        long off = 0;
        while (off < count) {
            long n = sys_write(1, buf + off, (unsigned long)(count - off));
            if (n <= 0)
                return 1;
            off += n;
        }
    }
    return count < 0;
}

static int exit_status(struct sh_command *cmd)
{
    if (cmd->argc == 1)
        return 0;
    if (cmd->argc != 2 || !cmd->argv[1][0])
        return -1;
    int code = 0;
    for (const char *p = cmd->argv[1]; *p; p++) {
        if (*p < '0' || *p > '9' || code > (255 - (*p - '0')) / 10)
            return -1;
        code = code * 10 + (*p - '0');
    }
    return code;
}

static int builtin(struct sh_command *cmd)
{
    const char *name = cmd->argv[0];
    if (strcmp(name, "echo") == 0) {
        for (int i = 1; i < cmd->argc; i++) {
            if (i > 1)
                output(1, " ");
            output(1, cmd->argv[i]);
        }
        output(1, "\n");
    } else if (strcmp(name, "pwd") == 0) {
        char cwd[256];
        if (sys_getcwd(cwd, sizeof(cwd)) < 0)
            error("pwd failed", 0);
        else {
            output(1, cwd);
            output(1, "\n");
        }
    } else if (strcmp(name, "cd") == 0) {
        if (sys_chdir(cmd->argc > 1 ? cmd->argv[1] : "/") < 0)
            error("cannot change directory", cmd->argc > 1 ? cmd->argv[1] : "/");
    } else if (strcmp(name, "ls") == 0) {
        const char *path = cmd->argc > 1 ? cmd->argv[1] : ".";
        long fd = sys_open(path, O_RDONLY, 0);
        if (fd < 0) {
            error("cannot open", path);
            return 1;
        }
        char buf[512];
        long count;
        while ((count = sys_getdents64((int)fd, buf, sizeof(buf))) > 0) {
            size_t off = 0;
            while (off < (size_t)count) {
                struct liteos_dirent64 *entry = (void *)(buf + off);
                if (entry->d_reclen < offsetof(struct liteos_dirent64, d_name) + 1 ||
                    entry->d_reclen > (size_t)count - off) {
                    error("invalid directory entry", path);
                    sys_close((int)fd);
                    return 1;
                }
                output(1, entry->d_name);
                output(1, "\n");
                off += entry->d_reclen;
            }
        }
        sys_close((int)fd);
        if (count < 0)
            error("cannot read directory", path);
    } else if (strcmp(name, "cat") == 0) {
        if (cmd->argc == 1)
            return copy_file(0);
        for (int i = 1; i < cmd->argc; i++) {
            long fd = sys_open(cmd->argv[i], O_RDONLY, 0);
            if (fd < 0) {
                error("cannot open", cmd->argv[i]);
                return 1;
            }
            int result = copy_file((int)fd);
            sys_close((int)fd);
            if (result)
                return result;
        }
    } else if (strcmp(name, "mkdir") == 0) {
        if (cmd->argc != 2 || sys_mkdir(cmd->argv[1], 0755) < 0)
            error("mkdir failed", cmd->argc == 2 ? cmd->argv[1] : 0);
    } else if (strcmp(name, "rm") == 0) {
        if (cmd->argc != 2 || sys_unlink(cmd->argv[1]) < 0)
            error("rm failed", cmd->argc == 2 ? cmd->argv[1] : 0);
    } else if (strcmp(name, "help") == 0) {
        output(1, "Builtins: cd pwd ls cat echo mkdir rm exit help\n"
                  "External programs: /bin/name or name (searched in /bin).\n"
                  "Supports quotes, pipes, <, > and >>.\n");
    } else if (strcmp(name, "exit") == 0) {
        int code = exit_status(cmd);
        if (code < 0) {
            error("usage: exit [0..255]", 0);
            return 1;
        }
        return code;
    } else {
        return -1;
    }
    return 0;
}

static int redirect(struct sh_command *cmd)
{
    if (cmd->input) {
        long fd = sys_open(cmd->input, O_RDONLY, 0);
        if (fd < 0) {
            error("cannot open", cmd->input);
            return -1;
        }
        long ok = sys_dup2((int)fd, 0);
        sys_close((int)fd);
        if (ok < 0)
            return -1;
    }
    if (cmd->output) {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        long fd = sys_open(cmd->output, flags, 0644);
        if (fd < 0) {
            error("cannot open", cmd->output);
            return -1;
        }
        long ok = sys_dup2((int)fd, 1);
        sys_close((int)fd);
        if (ok < 0)
            return -1;
    }
    return 0;
}

static void run_child(struct sh_command *cmd)
{
    if (redirect(cmd) < 0)
        sys_exit(1);
    int result = builtin(cmd);
    if (result >= 0)
        sys_exit(result);

    char path[128];
    const char *executable = cmd->argv[0];
    if (!strchr(executable, '/')) {
        if (strlen(executable) + sizeof("/bin/") > sizeof(path)) {
            error("name too long", executable);
            sys_exit(127);
        }
        strcpy(path, "/bin/");
        strcpy(path + 5, executable);
        executable = path;
    }
    char *env[] = { "PATH=/bin", 0 };
    sys_execve(executable, cmd->argv, env);
    error("command not found", cmd->argv[0]);
    sys_exit(127);
}

static void execute(struct sh_command *commands, int count)
{
    if (count == 1 && !commands[0].input && !commands[0].output) {
        if (builtin(&commands[0]) >= 0)
            return;
    }

    int previous = -1;
    int children[SH_MAX_COMMANDS];
    int started = 0;
    for (int i = 0; i < count; i++) {
        int fds[2] = { -1, -1 };
        if (i + 1 < count && sys_pipe(fds) < 0) {
            error("cannot create pipe", 0);
            break;
        }
        long pid = sys_fork();
        if (pid == 0) {
            if (previous >= 0) {
                if (sys_dup2(previous, 0) < 0)
                    sys_exit(1);
                sys_close(previous);
            }
            if (fds[1] >= 0) {
                if (sys_dup2(fds[1], 1) < 0)
                    sys_exit(1);
                sys_close(fds[1]);
                sys_close(fds[0]);
            }
            run_child(&commands[i]);
        }
        if (previous >= 0)
            sys_close(previous);
        if (fds[1] >= 0)
            sys_close(fds[1]);
        previous = fds[0];
        if (pid < 0) {
            error("cannot fork", 0);
            break;
        }
        children[started++] = (int)pid;
    }
    if (previous >= 0)
        sys_close(previous);
    for (int i = 0; i < started; i++) {
        int status;
        if (sys_wait4(children[i], &status) < 0)
            error("wait failed", 0);
    }
}

int main(void)
{
    output(1, "LiteOS /bin/sh - type help for commands\n");
    char line[LINE_SIZE];
    char storage[LINE_SIZE];
    for (;;) {
        output(1, "liteos$ ");
        size_t used = 0;
        int overflow = 0;
        for (;;) {
            char c;
            long result = sys_read(0, &c, 1);
            if (result < 0) {
                error("read failed", 0);
                return 1;
            }
            if (result == 0) {
                if (!used)
                    return 0;
                break;
            }
            if (c == '\n')
                break;
            if (used + 1 < sizeof(line))
                line[used++] = c;
            else
                overflow = 1;
        }
        if (overflow) {
            error("line too long", 0);
            continue;
        }
        line[used] = '\0';
        struct sh_command commands[SH_MAX_COMMANDS];
        int count = sh_parse(line, storage, sizeof(storage), commands);
        if (count < 0) {
            error("syntax error", 0);
            continue;
        }
        if (!count)
            continue;
        if (count == 1 && strcmp(commands[0].argv[0], "exit") == 0) {
            int code = exit_status(&commands[0]);
            if (code < 0) {
                error("usage: exit [0..255]", 0);
                continue;
            }
            return code;
        }
        execute(commands, count);
    }
}
