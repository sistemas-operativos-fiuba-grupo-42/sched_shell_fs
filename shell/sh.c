#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"
#include <signal.h>

char prompt[PRMTLEN] = { 0 };
stack_t alt_stack;

void
int_to_str(int num, char *str)
{
	int i = 0;
	int temp = num;

	do {
		i++;
		temp /= 10;
	} while (temp != 0);

	str[i] = '\0';

	while (i > 0) {
		i--;
		str[i] = '0' + (num % 10);
		num /= 10;
	}
}

void
sigchild_handler(int signum)
{
	pid_t pid;
	int status;

	pid = waitpid(0, &status, WNOHANG);
	if (pid > 0) {
		char str[] = "==> terminado: PID=";
		if (write(STDOUT_FILENO, str, sizeof(str)) < 0) {
			perror("Error en write");
			_exit(-1);
		}

		char pid_str[12] = { 0 };
		int_to_str(pid, pid_str);
		if (write(STDOUT_FILENO, pid_str, sizeof(pid_str)) < 0) {
			perror("Error en write");
			_exit(-1);
		}
		if (write(STDOUT_FILENO, "\n", 2) < 0) {
			perror("Error en write");
			_exit(-1);
		}
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

	alt_stack.ss_sp = malloc(
	        SIGSTKSZ);  // Asignar memoria para la pila (SIGSTKSZ es el tamaño recomendado)
	if (alt_stack.ss_sp == NULL) {
		perror("Error al asignar memoria para la pila alternativa");
		exit(-1);
	}
	alt_stack.ss_size = SIGSTKSZ;
	alt_stack.ss_flags = 0;

	// sigaltstack(new_ss, old_ss)
	if (sigaltstack(&alt_stack, NULL) < 0) {
		perror("Error al ejecutar sigalstack");
		free(alt_stack.ss_sp);
		exit(-1);
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigchild_handler;
	sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("Error al ejectutar sigaction");
		free(alt_stack.ss_sp);
		exit(-1);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	free(alt_stack.ss_sp);
	return 0;
}
