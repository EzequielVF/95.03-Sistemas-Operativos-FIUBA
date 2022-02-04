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
static void set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		char aux_key[ARGSIZE];
		char aux_value[ARGSIZE];
		get_environ_key(eargv[i], aux_key);
		int index = block_contains(eargv[i], '=');
		get_environ_value(eargv[i], aux_value, index);
		int overwrite = 1;  // Asi la remplaza si ya existia
		setenv(aux_key, aux_value, overwrite);
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
	if (flags == READ) {
		return open(file,
		            O_RDONLY,
		            O_CLOEXEC);  // Pongo el flag para que se cierre cuando se llega a un exec!
	} else if (flags == WRITE || flags == SALIDA_ERR) {
		return open(file,
		            O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
		            S_IRWXU);  //(archivo, flags, permisos)
	} else {
		return ERROR;
	}
}

void
redir_entrada(struct execcmd *r)
{
	int fd = open_redir_fd(r->in_file, READ);
	if (fd == ERROR) {
		perror("[REDIR] - Error al redireccionar la entrada.");
	} else {
		dup2(fd, 0);
		int error = execvp(r->argv[0], r->argv);
		if (error) {
			perror("[EXEC] - Error al usar execvp. (LN 100 - "
			       "exec.c)");
		}
	}
}

void
redir_salida(struct execcmd *r)
{
	int fd = open_redir_fd(r->out_file, WRITE);
	if (fd == ERROR) {
		perror("[REDIR] - Error al redireccionar la salida.");
	} else {
		dup2(fd, 1);
		int error = execvp(r->argv[0], r->argv);
		if (error) {
			perror("[EXEC] - Error al usar execvp. (LN 114 - "
			       "exec.c)");
		}
	}
}

void
redir_error_salida(struct execcmd *r)
{
	int fd_salida;
	if ((fd_salida = open_redir_fd(r->out_file, WRITE)) == ERROR) {
		perror("[REDIR] - Error al abrir el archivo para salida.");
		return;
	}
	dup2(fd_salida, 1);
	int fd_error;
	int out_index = block_contains(r->err_file, '&');
	if (out_index >=
	    0) {  // Si '&' esta, significa que entrada y salida tienen que apuntar al mismo "canal".
		dup2(fd_salida, 2);
	} else {  // Sino, redirijo salida  al fd que me digan.
		if ((fd_error = open_redir_fd(r->err_file, SALIDA_ERR)) == ERROR) {
			perror("[REDIR] - Error al abrir el archivo para "
			       "error.");
			return;
		}
		dup2(fd_error, 2);
	}
	int error = execvp(r->argv[0], r->argv);
	if (error) {
		perror("[EXEC] - Error al usar execvp. (LN 144 - exec.c)");
	}
}

void
redir_command(struct execcmd *r)
{
	bool red_ingreso = (strlen(r->in_file) > 0);
	bool red_salida = (strlen(r->out_file) > 0);
	bool red_error = (strlen(r->err_file) > 0);
	if (red_error && red_salida) {
		redir_error_salida(r);
	} else if (red_salida) {
		redir_salida(r);
	} else if (red_ingreso) {
		redir_entrada(r);
	} else {
		perror("[REDIR] - Error, no hay a que redireccionar.");  // Este
		                                                         // seria
		                                                         // el raro caso de un error en el parseo :?.
	}
}

void
exec_command(struct execcmd *e)
{
	set_environ_vars((char **) e->eargv, e->eargc);
	int error = execvp(e->argv[0], e->argv);
	if (error) {
		perror("[EXEC] - Error al usar execvp. (LN 84 - exec.c)");
	}
}

void
exec_hijo_izq(struct cmd *cmd, int *fd_lr, int *pid_izq)
{
	(*pid_izq) = fork();
	if (*pid_izq == ERROR) {
		perror("[PIPE] - Error con fork. (LN 169)");
		close(fd_lr[WRITE]);
		close(fd_lr[READ]);
		return;
	}

	if ((*pid_izq) == 0) {          // Soy hijo
		close(fd_lr[READ]);     // Si soy hijo izquierdo no voy a leer
		dup2(fd_lr[WRITE], 1);  // Redirecciono la salida
		close(fd_lr[WRITE]);    // Y cierro el fd
		exec_cmd(cmd);

		free_command(parsed_pipe);  // Si falla exec, libero memoria y exit.
		close(fd_lr[WRITE]);
		exit(ERROR);
	}
}

void
exec_hijo_der(struct cmd *cmd, int *fd_id, int *pid_der)
{
	(*pid_der) = fork();
	if (*pid_der == ERROR) {
		perror("[PIPE] - Error con fork. (LN 194)");
		close(fd_id[WRITE]);
		close(fd_id[READ]);
		return;
	}

	if ((*pid_der) == 0) {         // Soy hijo
		close(fd_id[WRITE]);   // Si soy hijo derecho no voy a escribir
		dup2(fd_id[READ], 0);  // Redirecciono la entrada
		close(fd_id[READ]);    // Y cierro el fd
		exec_cmd(cmd);

		free_command(parsed_pipe);  // Si falla exec, libero memoria y exit.
		close(fd_id[READ]);
		exit(ERROR);
	}
}

void
pipe_command(struct pipecmd *p)
{
	int fd_id[2];
	if (pipe(fd_id) == ERROR) {
		perror("[PIPE] - Error al crear el pipe. (LN 222)");
		return;
	}
	pid_t pid_izq = -1;
	pid_t pid_der = -1;
	exec_hijo_izq(p->leftcmd, fd_id, &pid_izq);
	exec_hijo_der(p->rightcmd, fd_id, &pid_der);
	close(fd_id[READ]);  // Ya pase los file_descriptors no los necesito mas.
	close(fd_id[WRITE]);  // Ademas si los dejo abiertos podria tener problemas.

	if (pid_izq != -1)
		waitpid(pid_izq,
		        NULL,
		        0);  // Espero a los hijos, pero primero al izquierdo para
		             // no dejarlo huerfano (si se extendio el hijo derecho).

	if (pid_der != -1)
		waitpid(pid_der, NULL, 0);

	free_command(parsed_pipe);  // Libero la memoria y exit para ahorrarme problemas.
	exit(0);
}

void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		// spawns a command
		//
		e = (struct execcmd *) cmd;
		exec_command(e);
		break;
	}

	case BACK: {
		// runs a command in background
		//
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//
		r = (struct execcmd *) cmd;
		redir_command(r);
		break;
	}

	case PIPE: {
		// pipes two commands
		//
		p = (struct pipecmd *) cmd;
		pipe_command(p);
	}
	}
}
