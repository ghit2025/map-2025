// CONVIERTE MAYÚSCULAS EN MINÚSCULAS Y VICEVERSA EN LOS DATOS RECIBIDOS
// POR LA ENTRADA ESTÁNDAR Y ESCRITOS POR LA SALIDA ESTÁNDAR
// No válido para UTF-8
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>

int main(int  argc, char **argv) {
	char car;

	while ((car = fgetc(stdin))!=EOF) {
		if (islower(car)) car=toupper(car);
		else if (isupper(car)) car=tolower(car);
		fputc(car, stdout);
	}
	return(0);
}
