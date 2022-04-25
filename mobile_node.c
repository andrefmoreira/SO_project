/* 
Trabalho realizado por:
André Filipe de Oliveira Nº 2020239416
Pedro Miguel Pereira Catorze Nº 2020222916
*/


#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int num_ped, intervalo, instr, temp_max;

    if (argc != 5) {
        printf("Wrong number of parameters!\n");
        exit(-1);
    }

    if (atoi(argv[1]) != 0) {
        num_ped = atoi(argv[1]);
    } else {
        printf("Error on the first parameter!\n");
        exit(-1);
    }

    if (atoi(argv[2]) != 0) {
        intervalo = atoi(argv[2]);
    } else {
        printf("Error on the second parameter!\n");
        exit(-1);
    }

    if (atoi(argv[3]) != 0) {
        instr = atoi(argv[3]);
    } else {
        printf("Error on the third parameter!\n");
        exit(-1);
    }


    if (atoi(argv[4]) != 0) {
        temp_max = atoi(argv[4]);
    } else {
        printf("Error on the fourth parameter!\n");
        exit(-1);
    }

    //debug printf: parametros lidos
    printf("Valores lidos:\n%d\n%d\n%d\n%d\n", num_ped, intervalo, instr, temp_max);

    return 0;
}
