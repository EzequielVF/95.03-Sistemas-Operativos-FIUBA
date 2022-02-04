#ifndef EXEC_H
#define EXEC_H

#include "defs.h"
#include "types.h"
#include "utils.h"
#include "freecmd.h"

extern struct cmd *parsed_pipe;

void exec_cmd(struct cmd *c);
void redir_entrada(struct execcmd *r);
void redir_salida(struct execcmd *r);
void redir_error_salida(struct execcmd *r);
void redir_command(struct execcmd *r);
void redir_command(struct execcmd *r);
static void set_environ_vars(char **eargv, int eargc);
void pipe_command(struct pipecmd *p);
void exec_command(struct execcmd *e);
void exec_hijo_izq(struct cmd *cmd, int *fd_lr, int *pid_izq);
void exec_hijo_der(struct cmd *cmd, int *fd_id, int *pid_der);

#endif  // EXEC_H
