#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>

#define ERROR -1
#define EXITO 0
#define FLAG "-i"
#define PROB_ACTUAL "."
#define PROB_SUPERIOR ".."

bool comparar_strings(char *string_1, char *string_2);
void buscar(int file_descriptor, char *ruta, char *busqueda, bool flag);
void preparar_y_mostrar_ruta(
        char *nombre, char *ruta, char *ruta_actual, char *busqueda, bool flag);


bool
comparar_strings(char *string_1, char *string_2)
{
	return (strcmp(string_1, string_2) == 0);
}

void
preparar_y_mostrar_ruta(
        char *nombre, char *ruta, char *ruta_actual, char *busqueda, bool flag)
{
	bool estoy_en_otro_directorio = true;
	if (ruta[0] == '\0')
		estoy_en_otro_directorio = false;
	strcpy(ruta_actual,
	       ruta);  // Si ya estaba dentro de otro directorio, lo agrego a la ruta actual
	if (estoy_en_otro_directorio)
		strcat(ruta_actual, "/");
	strcat(ruta_actual, nombre);
	if (flag) {
		if (strcasestr(nombre, busqueda)) {
			printf("%s\n", ruta_actual);
		}
	} else {
		if (strstr(nombre, busqueda)) {
			printf("%s\n", ruta_actual);
		}
	}
}

void
buscar(int file_descriptor, char *ruta, char *busqueda, bool flag)
{
	char ruta_actual[PATH_MAX];
	memset(ruta_actual, 0, PATH_MAX);  // Inicializo todo en 0
	DIR *directorio = fdopendir(file_descriptor);
	struct dirent *registro;
	registro = readdir(directorio);
	while (registro != NULL) {
		if (!comparar_strings(registro->d_name, PROB_ACTUAL) &&
		    !comparar_strings(registro->d_name, PROB_SUPERIOR)) {
			char *nombre = registro->d_name;
			preparar_y_mostrar_ruta(
			        nombre, ruta, ruta_actual, busqueda, flag);
			int nuevo_file_descriptor =
			        openat(file_descriptor, nombre, O_DIRECTORY);
			if (nuevo_file_descriptor !=
			    -1) {  // Si se abre significa que es un directorio al cual deberia acceder.
				buscar(nuevo_file_descriptor,
				       ruta_actual,
				       busqueda,
				       flag);
			}
		}
		registro = readdir(directorio);
	}
	closedir(directorio);
}

int
main(int argc, char *argv[])
{
	if (argc <= 1) {
		perror("[MAIN] - Falta el nombre a buscar.");
		return ERROR;
	}
	DIR *base = opendir(".");
	if (!base) {
		perror("[MAIN] - Problema al abrir el directorio '.'.");
		return ERROR;
	}
	int file_descriptor = dirfd(base);
	if (comparar_strings(FLAG, argv[1])) {
		buscar(file_descriptor, "", argv[2], true);
	} else {
		buscar(file_descriptor, "", argv[1], false);
	}
	closedir(base);
	return EXITO;
}