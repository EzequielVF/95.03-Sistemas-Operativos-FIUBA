#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define ERROR -1
#define EXITO 0
#define ESCRITURA 1
#define LECTURA 0

int obtener_numero(int argc, char *argv[]);
void hago_de_padre(int *pipe_der, int numero_max);
void filtro(int *pipe_der, int *pipe_izq, int numero_base);
void primes(int *pipe_izq, int numero_max);
void hago_de_hijo(int *pipe_izq, int numero_max);

int
obtener_numero(int argc, char *argv[])
{
	if (argc <= 1) {
		perror("[MAIN] - Falta un argumento mas, el N MAX.");
		return ERROR;
	}
	int aux = strtol(argv[1], NULL /*No chequeo errores*/, 10);
	return aux;
}

void
hago_de_padre(int *pipe_der, int numero_max)
{
	for (int i = 2; i <= numero_max; i++) {
		int cod = write(pipe_der[ESCRITURA], &i, sizeof(int));
		if (cod == -1) {
			perror("[PADRE] - Error al escribir en el pipe");
		}
	}
	close(pipe_der[ESCRITURA]);
	close(pipe_der[LECTURA]);
	wait(NULL);
}

void
filtro(int *pipe_der, int *pipe_izq, int numero_base)
{
	int numero_aux;
	while (read(pipe_izq[LECTURA], &numero_aux, sizeof(int)) > 0) {
		if ((numero_aux % numero_base) != 0) {
			int cod = write(pipe_der[ESCRITURA],
			                &numero_aux,
			                sizeof(int));
			if (cod == ERROR)
				perror("[FILTRO] - Error al escribir en el "
				       "pipe");
		}
	}
	close(pipe_izq[LECTURA]);
	close(pipe_der[ESCRITURA]);
	close(pipe_der[LECTURA]);
	wait(NULL);
}

void
primes(int *pipe_izq, int numero_max)
{
	int numero_base;
	if (read(pipe_izq[LECTURA], &numero_base, sizeof(int)) <= 0) {
		printf("[PRIMES] - No hay mas PRIMOS en el rango pedido.\n");  // Linea extra no pedida en la consigna.
		close(pipe_izq[LECTURA]);
		exit(0);
	}
	printf("primo: %d\n", numero_base);
	int pipe_der[2];
	if (pipe(pipe_der)) {
		perror("[PRIMES] - Error al crear el pipe");
	}
	int pid = fork();
	if (pid < 0) {
		perror("[PRIMES] - Error al hacer el fork");
	} else if (pid == 0) {  // Soy hijo
		close(pipe_izq[LECTURA]);
		hago_de_hijo(pipe_der, numero_max);
	} else {  // Soy padre
		filtro(pipe_der, pipe_izq, numero_base);
	}
}

void
hago_de_hijo(int *pipe_izq, int numero_max)
{
	if (close(pipe_izq[ESCRITURA]) ==
	    ERROR) {  // No deberia escribirle nada nunca al que esta en la izquierda
		perror("[HIJO] - Error al cerrar el pipe");
	}
	primes(pipe_izq, numero_max);
}

int
main(int argc, char *argv[])
{
	int numero_max = obtener_numero(argc, argv);
	if (numero_max < 2) {
		perror("[MAIN] - Numero invalido");
		return ERROR;
	}
	int pipe_der[2];
	if (pipe(pipe_der)) {
		perror("[MAIN] - Error al crear el pipe");
		return ERROR;
	}
	int pid = fork();
	if (pid < 0) {
		perror("[MAIN] - Error al hacer el fork");
		return ERROR;
	} else if (pid == 0) {  // Soy hijo
		hago_de_hijo(pipe_der, numero_max);
	} else {  // Soy padre
		hago_de_padre(pipe_der, numero_max);
	}
	return EXITO;
}