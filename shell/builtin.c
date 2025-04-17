#include "builtin.h"
extern int status;

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (strcmp(cmd, "exit") == 0) {
		status = 0;
		return 1;
	}
	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strncmp(cmd, "cd", 2) != 0) {
		return 0;
	}
	if (cmd == NULL) {
		perror("Error: cmd es NULL");
		status = -1;
		return 0;
	}

	char *dir;
	if (strlen(cmd + 2) == 0 || strlen(cmd + 3) == 0) {  // "cd \0" "cd\0"
		dir = getenv("HOME");
		if (dir == NULL) {
			perror("Error: $HOME no está definido");
			status = -1;
			return 0;
		}
	} else {
		dir = cmd + 3;
	}
	if (chdir(dir) < 0) {
		perror("Error al cambiar de directorio");
		status = -1;
		return 0;
	}

	// Update the prompt with the new directory
	char buf[BUFLEN - 2];
	if (getcwd(buf, sizeof(buf)) == NULL) {
		perror("Error al obtener el directorio actual");
		status = -1;
		return 0;
	}
	snprintf(prompt, PRMTLEN, "(%s)", buf);
	status = 0;
	return 1;  // return true
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		char buf[BUFLEN];
		if (getcwd(buf, BUFLEN) == NULL) {
			perror("Error al obtener el directorio actual");
			status = -1;
			return 0;
		}
		printf("%s\n", buf);
		status = 0;
		return 1;
	}
	return 0;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
