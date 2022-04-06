#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {
    long num_pedidos, intervalo, instr, temp_max;

    if (argc != 5) {
        printf("Número errado de parâmetros!\n");
        exit(-1);
    }

    if (strtol(argv[1], argv, 10) != 0) {
        num_pedidos = strtol(argv[1], argv, 10);
    } else {
        printf("Erro no 1º parâmetro!\n");
        exit(-1);
    }

    if (strtol(argv[2], argv, 10) != 0) {
        intervalo = strtol(argv[2], argv, 10);
    } else {
        printf("Erro no 2º parâmetro!\n");
        exit(-1);
    }

    if (strtol(argv[3], argv, 10) != 0) {
        instr = strtol(argv[3], argv, 10);
    } else {
        printf("Erro no 3º parâmetro!\n");
        exit(-1);
    }


    if (strtol(argv[4], argv, 10) != 0) {
        temp_max = strtol(argv[4], argv, 10);
    } else {
        printf("Erro no 4º parâmetro!\n");
        exit(-1);
    }

    printf("%ld\n %ld\n %ld\n %ld\n" , num_pedidos, intervalo, instr, temp_max);

    return 0;
}
