#include <stdio.h>
#include <zconf.h>
#include <sys/wait.h>
#include "string.h"
#include <stdlib.h>
#include <stdbool.h>

#define ERROR -1
#define EXITO 0
#define MAX_LINEA 512
#ifndef NARGS
#define NARGS 4
#endif

int leer_ngars_lineas(bool *termine, char *linea_aux);
void parsear_lineas(char **cosas, char *linea_aux);
void iniciar_procesos(char **cosas);
void limpiar(char **cosas, char *linea_aux);

int
leer_ngars_lineas(bool *termine, char *linea_aux)
{
	char linea[MAX_LINEA];
	char *puntero_linea = linea;
	memset(linea, 0, MAX_LINEA);
	size_t tamanio_linea = MAX_LINEA;
	int leidos = 0;
	ssize_t aux = getline(&puntero_linea, &tamanio_linea, stdin);
	if (aux == -1)
		*termine = true;

	while (leidos < NARGS && !(*termine)) {
		strcat(linea_aux, puntero_linea);
		leidos++;
		if (leidos < NARGS)
			aux = getline(&puntero_linea, &tamanio_linea, stdin);
		if (aux == ERROR)
			*termine = true;
	}
	return leidos;
}

void
parsear_lineas(char **cosas, char *linea_aux)
{
	char *puntero;
	puntero = strtok(linea_aux, "\n");
	int i = 0;
	while (puntero != NULL) {
		i++;
		cosas[i] = puntero;
		puntero = strtok(NULL, "\n");
	}
}

void
iniciar_procesos(char **cosas)
{
	int id = fork();
	if (id == 0) {
		int cod = execvp(cosas[0], cosas);
		if (cod) {
			perror("[MAIN] - Error al usar execvp.");
		}
	}
	wait(NULL);
}

void
limpiar(char **cosas, char *linea_aux)
{
	for (int i = 0; i < NARGS + 2; i++) {
		cosas[i] = NULL;
	}
	memset(linea_aux, 0, MAX_LINEA * NARGS);
}

int
main(int argc, char *argv[])
{
	if (argc <= 1) {
		perror("[MAIN] - Falta el comando a ejecutar.");
		return ERROR;
	}
	char *cosas[NARGS + 2] = { NULL };
	cosas[0] = argv[1];
	char linea_aux[MAX_LINEA * NARGS];  // Ya que voy a meter NGARS lineas para despues separarlas fijandome en /n
	memset(linea_aux, 0, MAX_LINEA * NARGS);
	bool termine = false;
	int aux;
	while (!termine) {
		aux = leer_ngars_lineas(&termine, linea_aux);
		if (aux == 0)  // Si no lei nada, salgo del while
			break;
		parsear_lineas(cosas, linea_aux);
		iniciar_procesos(cosas);
		limpiar(cosas, linea_aux);
		cosas[0] = argv[1];
	}
	return 0;
}