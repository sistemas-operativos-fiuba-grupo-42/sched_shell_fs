#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	sys_set_priority(thisenv->env_id, 1);
	sys_yield();

	cprintf("hello, world\n");
	cprintf("i am environment %08x with priority %08x\n",
	        thisenv->env_id,
	        sys_get_priority(thisenv->env_id));
}
