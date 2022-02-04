#include "builtin.h"
#include "utils.h"

extern int status;

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return (strcmp(cmd, "exit") == 0);
}

char *
remover_espacios(char *string)
{
	int i = 0;
	while (string[i] == SPACE && string[i] != END_STRING)
		i++;
	return &string[i];
}

static void
actualizar_dir(const char *nueva_ruta)
{
	if (chdir(nueva_ruta) == ERROR) {
		perror("[BUILT-IN] - Error al cambiar de directorio");
		status = 1;
		return;
	}

	char dir[PRMTLEN - 3];
	if (!getcwd(dir, PRMTLEN - 3)) {
		perror("[BUILT-IN] - Error al intentar obtener el nuevo "
		       "directorio.");
		status = 1;
		return;
	}
	snprintf(promt, PRMTLEN, "(%s)", dir);  // Actualizo el promt
	status = 0;
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
	if (strlen(cmd) < 2) {
		return 0;
	}

	char *dir = NULL;
	if (strcmp(cmd, "cd") == 0) {
		dir = getenv("HOME");
	}

	if (strncmp(cmd, "cd ", 3) == 0) {
		dir = split_line(cmd, SPACE);
		dir = remover_espacios(dir);
	}

	if (dir) {
		actualizar_dir(dir);
		return 1;
	}
	return 0;
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
		void *ruta = getcwd(cmd, ARGSIZE);
		if (ruta != NULL) {
			printf("%s\n", cmd);
			status = 0;
			return 1;
		} else {
			status = 1;
			return ERROR;
		}
	}
	return 0;
}
