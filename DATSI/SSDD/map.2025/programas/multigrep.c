// IMPRIME POR LA SALIDA ESTÁNDAR LAS LÍNEAS DE LA ENTRADA ESTÁNDAR
// QUE CONTIENEN ALGUNAS DE LAS PALABRAS RECIBIDAS COMO ARGUMENTOS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc<2) {
        fprintf(stderr, "usage: %s patterns...\n", argv[0]); return 1;
    }
    char *line=NULL;
    size_t size;
    int found;
    while (getline(&line, &size, stdin)!=-1) {
        found=0;
        for (int i=1; !found && i<argc; i++)
            if (strstr(line, argv[i])) found=1;
        if (found) printf("%s", line);
    }
    free(line);
    return 0;
}
