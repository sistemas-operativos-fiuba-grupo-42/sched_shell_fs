#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

#define MAX_RUNS_PRIORITY_0 15
#define MAX_RUNS_PRIORITY_1 7
#define MAX_RUNS_PRIORITY_2 2

#define LEN_HISTORY 10


int priority_0_runs = 0;
int priority_1_runs = 0;
int priority_2_runs = 0;

struct Stats {
	int history[LEN_HISTORY];
	int process_executions[NENV];
	int sched_calls;
};

struct Stats stats = {0};

void update_stats(int i) {
	stats.process_executions[i]++;
	for (int i = 1; i < LEN_HISTORY; i++){
		stats.history[i-1] = stats.history[i];
	}
	stats.history[LEN_HISTORY-1] = i;
}

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{
#ifdef SCHED_ROUND_ROBIN
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running. Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here - Round robin
	
	stats.sched_calls++;
	struct Env *e = curenv;
	int i_curenv = e != NULL ? ENVX(e->env_id) : 0;

	for (int i = 1; i < NENV + 1; i++) {
		int idx = (i_curenv + i) % NENV;
		struct Env *env = envs+idx;
		if (env->env_status == ENV_RUNNABLE) {
			update_stats(ENVX(env->env_id));
			env_run(env);
		}
	}
	
	if (e != NULL && e->env_status == ENV_RUNNING) {
		update_stats(ENVX(e->env_id));
		env_run(e);
	}

#endif

#ifdef SCHED_PRIORITIES
	// Implement simple priorities scheduling.
	//
	// Environments now have a "priority" so it must be consider
	// when the selection is performed.
	//
	// Be careful to not fall in "starvation" such that only one
	// environment is selected and run every time.

	// Your code here - Priorities
	
	stats.sched_calls++;
	if (priority_0_runs == MAX_RUNS_PRIORITY_0 &&
	    priority_1_runs == MAX_RUNS_PRIORITY_1 &&
	    priority_2_runs == MAX_RUNS_PRIORITY_2) {
	    priority_0_runs = 0;
	    priority_1_runs = 0;
	    priority_2_runs = 0;
	}

	struct Env *e = curenv;
	int i_curenv = e ? ENVX(e->env_id) : 0;

	struct Env *next[3] = { NULL, NULL, NULL };
	int min_runs[3] = { __INT32_MAX__, __INT32_MAX__, __INT32_MAX__ };

	for (int i = 1; i < NENV + 1; i++) {
		int idx = (i_curenv + i) % NENV;
		struct Env *env = &envs[idx];

		if (env->env_status != ENV_RUNNABLE && !(env->env_status == ENV_RUNNING && env == curenv)) continue;

		int pr = env->priority;
		if (pr >= 0 && pr <= 2 && env->env_runs < min_runs[pr]) {
			min_runs[pr] = env->env_runs;
			next[pr] = env;
		}
	}

	if (next[0] && priority_0_runs < MAX_RUNS_PRIORITY_0) {
		priority_0_runs++;
		update_stats(ENVX(next[0]->env_id));
		env_run(next[0]);
	} else if (next[1] && priority_1_runs < MAX_RUNS_PRIORITY_1) {
		priority_1_runs++;
		update_stats(ENVX(next[1]->env_id));
		env_run(next[1]);
	} else if (next[2] && priority_2_runs < MAX_RUNS_PRIORITY_2) {
		priority_2_runs++;
		update_stats(ENVX(next[2]->env_id));
		env_run(next[2]);
	} else if (next[0]){
		priority_0_runs = 0;
		priority_1_runs = 0;
		priority_2_runs = 0;
		priority_0_runs++;
		update_stats(ENVX(next[0]->env_id));
		env_run(next[0]);
	} else if (next[1] && !next[0]){
		priority_0_runs = 0;
		priority_1_runs = 0;
		priority_2_runs = 0;
		priority_1_runs++;
		update_stats(ENVX(next[1]->env_id));		
		env_run(next[1]);
	}  else if (next[2] && !next[0] && !next[1]){
		priority_0_runs = 0;
		priority_1_runs = 0;
		priority_2_runs = 0;
		priority_2_runs++;
		update_stats(ENVX(next[2]->env_id));
		env_run(next[2]);
	}

	if (e != NULL && e->env_status == ENV_RUNNING) {
		update_stats(ENVX(e->env_id));
		env_run(e);
	}
	
#endif
		// Without scheduler, keep runing the last environment while it exists 
		/*
		if (curenv) {
			env_run(curenv);
			}
			*/
		
		// sched_halt never returns
		sched_halt();
	}

void print_stats() {
	cprintf("scheduler calls: %d\n", stats.sched_calls);
	cprintf("LAST %d PROCESS EXECUTIONS:\n", LEN_HISTORY);

	for (int i = 0; i < LEN_HISTORY; i++) {
		cprintf("env: %d ", stats.history[i]);
	}
	cprintf("\n");

	cprintf("PROCESS EXECUTIONS\n");
	for (int i = 0; i < NENV; i++) {
		cprintf("env: %d | runs: %d\n", i, stats.process_executions[i]);
	}
}

	// Halt this CPU when there is nothing to do. Wait until the
	// timer interrupt wakes it up. This function never returns.
	//
	void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		print_stats();
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
