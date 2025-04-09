#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++){
		int pos = block_contains(eargv[i], '=');
		char *key = malloc(pos + 1);
		char *value = malloc(strlen(eargv[i]) - pos + 1);
		if (key == NULL || value == NULL){
			perror("Error en malloc");
			_exit(-1);
		}
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, pos);
		if (setenv(key, value, 1) == -1) {
			perror("Error en setenv");
			_exit(-1);
		}
		free(key);
		free(value);
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd;
	if (flags & O_CREAT){
		fd = open(file, flags, S_IRUSR | S_IWUSR);
	} else{
		fd = open(file, flags);
	}
	if (fd < 0){
		perror("Error al abrir archivo");
		_exit(-1);
	}

	return fd;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		
		e = (struct execcmd *) cmd;

		e->argv[e->argc] = NULL;
		set_environ_vars(e->eargv, e->eargc);
		if (execvp(e->argv[0], e->argv) == -1){
			perror("Error en el exec");
			_exit(-1);
		}
		_exit(0);
		break;

	case BACK: {
		// runs a command in background
		//
		// Your code here
		printf("Background process are not yet implemented\n");
		_exit(-1);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		
		r = (struct execcmd *) cmd;
		if (strlen(r->in_file) > 0){
			int fd_in = open_redir_fd(r->in_file, O_RDONLY | O_CLOEXEC);
			if (dup2(fd_in, STDIN_FILENO) < 0) {
				perror("Error en dup2");
				_exit(-1);
			} 
			if (close(fd_in) < 0) {
				perror("Error en close");
				_exit(-1);
			}
		}
		if (strlen(r->out_file) > 0){			
			int fd_out = open_redir_fd(r->out_file, O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC);
			if (dup2(fd_out, STDOUT_FILENO) < 0) {
				perror("Error en dup2");
				_exit(-1);
			} 
			if (close(fd_out) < 0) {
				perror("Error en close");
				_exit(-1);
			}
		}
		if (strlen(r->err_file) > 0){
			if (strcmp(r->err_file, "&1") == 0){
				if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
					perror("Error en dup2");
					_exit(-1);
				} 
			} else{
				int fd_err = open_redir_fd(r->err_file, O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC);
				if (dup2(fd_err, STDERR_FILENO) < 0) {
					perror("Error en dup2");
					_exit(-1);
				} 
				if (close(fd_err) < 0) {
					perror("Error en close");
					_exit(-1);
				}
			}
		}

		r->type = EXEC;
		exec_cmd((struct cmd *) r);
		_exit(0);
		break;
	}

	case PIPE: {
		// pipes two commands
		//

		p = (struct pipecmd *) cmd;
		int pipes[2];

		if (pipe(pipes) < 0){
			perror("Error en el pipe");
			free_command((struct cmd *) p);
			_exit(-1);
		}

		int pid_i = fork();
		if (pid_i < 0){
			perror("Error en fork");
			free_command((struct cmd *) p);
			close(pipes[READ]);
			close(pipes[WRITE]);
			_exit(-1);
		} else if (pid_i == 0) {
			free_command(p->rightcmd);
			close(pipes[READ]);
			if (dup2(pipes[WRITE], STDOUT_FILENO) < 0) {
				perror("Error en dup2");
				free_command((struct cmd *) p);
				_exit(-1);
			}
			close(pipes[WRITE]);
			struct cmd *left = p->leftcmd;
			free(p);
			exec_cmd(left);
			_exit(0);
		}

		int pid_d = fork();
		if (pid_d < 0) {
			perror("Error en fork");
			free_command((struct cmd *) p);
			close(pipes[READ]);
			close(pipes[WRITE]);
			_exit(-1);
		} else if (pid_d == 0) {
			free_command(p->leftcmd);
			close(pipes[WRITE]);
			if (dup2(pipes[READ], STDIN_FILENO) < 0) {
				perror("Error en dup2");
				free_command((struct cmd *) p);
				_exit(-1);
			}
			close(pipes[READ]);
			struct cmd *right = p->rightcmd;
			free(p);
			exec_cmd(right);
			_exit(0);
		}
		
		close(pipes[READ]);
		close(pipes[WRITE]);
		int status_left, status_right;
		waitpid(pid_i, &status_left, 0);
		waitpid(pid_d, &status_right, 0);
		free_command((struct cmd *) p);

		if (WIFEXITED(status_right)) {
			_exit(WEXITSTATUS(status_right));
		} else {
			_exit(-1);
		}
	}
	}
}
