#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("Test start\n");

    cprintf("Priority of the current process is: %d\n", sys_env_get_priority(0));

    int status = sys_env_set_priority(1);
    if (status < 0) {
    }
}