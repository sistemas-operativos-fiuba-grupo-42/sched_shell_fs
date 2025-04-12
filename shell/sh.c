#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };

// sigaltstack(2) Inicializa stack alternativo
// sigaction(2) ¿inicializa handler?
// Ejecuta codigo async-signal-safe :(
void sigchild_handler(int signum) {
    pid_t pid; int status;

	pid = waitpid(0, &status, WNOHANG);
	if (pid == -1) {
		perror("Error en waitpid");
		_exit(-1);
	}
	if (pid > 0) {
		char str[] = "==> terminado: PID=";
        // write(1, str, sizeof(str)); 
    
        // write(1, &pid, sizeof(pid));
		// write(1, "\n", 1);

		printf("==> terminado: PID=%d\n", pid);

	}
}



// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
	
	stack_t ss;
    ss.ss_sp = malloc(SIGSTKSZ); // Asignar memoria para la pila (SIGSTKSZ es el tamaño recomendado)
    if (ss.ss_sp == NULL) {
		perror("Error al asignar memoria para la pila alternativa");
        exit(-1);
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
	
	// sigaltstack(new_ss, old_ss)
	if (sigaltstack(&ss, NULL) < 0) {
		perror("Error al ejecutar sigalstack");
		exit(-1);
	}

	struct sigaction a = {
		sigchild_handler, 
		0, 
		0, 
		SA_SIGINFO | SA_RESTART | SA_ONSTACK, 
		0
	};
	if (sigaction(SIGCHLD, &a, NULL) < 0) {
		perror("Error al ejectutar sigaction");
		exit(-1);
	}
} 

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
