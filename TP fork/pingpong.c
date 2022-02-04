#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define ERROR -1
#define EXITO 0
#define ESCRITURA 1
#define LECTURA 0

int inicializo_pipes(int *pipe_1, int *pipe_2);
int hago_de_padre(int *pipe_padre_hijo, int *pipe_hijo_padre, int valor);
int hago_de_hijo(int *pipe_padre_hijo, int *pipe_hijo_padre, int valor);

int
inicializo_pipes(int *pipe_1, int *pipe_2)
{
	if (pipe(pipe_1) == ERROR || pipe(pipe_2) == ERROR) {
		return ERROR;
	}
	printf("  - pipe me devuelve: [%i, %i]\n", pipe_1[0], pipe_1[1]);
	printf("  - pipe me devuelve: [%i, %i]\n", pipe_2[0], pipe_2[1]);
	return EXITO;
}

int
hago_de_padre(int *pipe_padre_hijo, int *pipe_hijo_padre, int valor)
{
	if (close(pipe_padre_hijo[LECTURA]) == ERROR ||
	    close(pipe_hijo_padre[ESCRITURA]) == ERROR) {
		perror("[PADRE] - Error al cerrar las direcciones de los "
		       "canales que no usare.");
		return ERROR;
	}
	srandom(time(NULL));  // Seteo la semilla para conseguir los numeros 'casi' aleatorios.
	int valor_aleatorio = random();

	printf("\nDonde fork me devuelve %i:\n", valor);
	printf("  - getpid me devuelve: %i\n", getpid());
	printf("  - getppid me devuelve: %i\n", getppid());
	printf("  - random me devuelve: %i\n", valor_aleatorio);
	printf("  - envío valor %i a través de fd=%i\n",
	       valor_aleatorio,
	       pipe_padre_hijo[ESCRITURA]);

	if (write(pipe_padre_hijo[ESCRITURA], &valor_aleatorio, sizeof(int)) <
	    0) {  // Escribo en el pipeline
		perror("[PADRE] - Error al escribir en el pipe - padre");
	}

	if (read(pipe_hijo_padre[LECTURA], &valor_aleatorio, sizeof(int)) ==
	    -1) {  // Leo del pipeline, si no hay nada que leer...espero
		perror("[PADRE] - Error al leer del pipe");
	}
	printf("\nHola, de nuevo PID %d\n", getpid());
	printf("  - recibi valor %d via fd= %d \n",
	       valor_aleatorio,
	       pipe_hijo_padre[LECTURA]);

	if (close(pipe_padre_hijo[ESCRITURA]) == ERROR ||
	    close(pipe_hijo_padre[LECTURA]) ==
	            ERROR) {  // Una vez usados cierro los canales que deje abiertos
		perror("[PADRE] - Error al cerrar las direcciones de los "
		       "canales que use para comunicarme");
		return ERROR;
	}
    wait(NULL);
	return EXITO;
}

int
hago_de_hijo(int *pipe_padre_hijo, int *pipe_hijo_padre, int valor)
{
	if (close(pipe_padre_hijo[ESCRITURA]) == ERROR ||
	    close(pipe_hijo_padre[LECTURA]) == ERROR) {
		perror("[HIJO] - Error al cerrar las direcciones de los "
		       "canales que no usare. (Soy el proceso hijo)");
		return ERROR;
	}
	int valor_aleatorio;
	if (read(pipe_padre_hijo[LECTURA], &valor_aleatorio, sizeof(int)) ==
	    -1) {  // Leo del pipeline, si no hay nada que leer...espero
		perror("[HIJO] - Error al leer del pipe");
	}

	printf("\nDonde fork me devuelve %i:\n", valor);
	printf("  - getpid me devuelve: %i\n", getpid());
	printf("  - getppid me devuelve: %i\n", getppid());
	printf("  - recibo valor %i vía fd=%i\n",
	       valor_aleatorio,
	       pipe_padre_hijo[LECTURA]);
	printf("  - reenvío valor en fd=%i y termino\n", pipe_padre_hijo[LECTURA]);

	if (write(pipe_hijo_padre[ESCRITURA], &valor_aleatorio, sizeof(int)) ==
	    -1) {  // Escribo en el pipeline
		perror("[HIJO] - Error al escribir en el pipe");
	}

	if (close(pipe_padre_hijo[LECTURA]) == ERROR ||
	    close(pipe_hijo_padre[ESCRITURA]) ==
	            ERROR) {  // Una vez usados cierro los canales que deje abiertos
		perror("[HIJO] - Error al cerrar las direcciones de los "
		       "canales que use para comunicarme");
		return ERROR;
	}
	return EXITO;
}

int
main()
{
	printf("Hola, soy PID %i:\n", getpid());
	int pipe_padre_hijo[2];
	int pipe_hijo_padre[2];

	if (inicializo_pipes(pipe_padre_hijo, pipe_hijo_padre) ==
	    ERROR) {  // Inicializo los pipeline y si hay un error resuelvo.
		perror("[MAIN] - Error al inicializar los pipelines");
		return ERROR;
	}

	int valor = fork();  // Realizo el fork
	if (valor < 0) {
		perror("[MAIN] - Error al hacer el fork");
		return ERROR;
	} else if (valor == 0) {
		if (hago_de_hijo(pipe_padre_hijo, pipe_hijo_padre, valor) ==
		    ERROR) {
			return ERROR;
		}
	} else {
		if (hago_de_padre(pipe_padre_hijo, pipe_hijo_padre, valor) ==
		    ERROR) {
			return ERROR;
		}
	}
	return EXITO;
}