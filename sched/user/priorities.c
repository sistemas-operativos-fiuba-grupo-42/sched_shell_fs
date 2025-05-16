#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("Test start\n");

	cprintf("Priority of the current process is: %08x\n",
	        sys_get_priority(thisenv->env_id));
	int status = sys_set_priority(thisenv->env_id, 1);
	if (status == -1) {
		cprintf("Error");
	} else {
		cprintf("New process priority is: %08x\n",
		        sys_get_priority(thisenv->env_id));
	}


	int status_inc = sys_set_priority(thisenv->env_id, 0);
	if (status_inc == -1) {
		cprintf("It should not be possible to increase the priority of "
		        "a process.\n");
		cprintf("So priority is: %08x\n",
		        sys_get_priority(thisenv->env_id));
	}
}