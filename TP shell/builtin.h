#ifndef BUILTIN_H
#define BUILTIN_H

#include "defs.h"

extern char promt[PRMTLEN];

int cd(char *cmd);

int exit_shell(char *cmd);

int pwd(char *cmd);

char *remover_espacios(char *string);

#endif  // BUILTIN_H
