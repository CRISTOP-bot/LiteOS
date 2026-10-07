#include <sys/liteos.h>

int main(void)
{
    static const char message[] = "Hello from a user-space ELF process!\n";
    return sys_write(1, message, sizeof(message) - 1) < 0 ? 1 : 0;
}
