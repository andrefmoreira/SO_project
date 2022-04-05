#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
    long numero_pedidos, intervalo , instrucoes , tempo_max;

    if(argc != 5)
    {
        printf("Numero errado de parametros!\n");
        exit(-1);
    }

    if(strtol(argv[1], argv, 10) != 0)
        numero_pedidos = strtol(argv[1], argv, 10);
    else{
        printf("erro no parametro 1\n");
        exit(-1);
    }


    if(strtol(argv[2], argv, 10) != 0)
        intervalo = strtol(argv[2], argv, 10);
    else{
        printf("erro no parametro 2\n");
        exit(-1);
    }


    if(strtol(argv[3], argv, 10) != 0)
        instrucoes = strtol(argv[3], argv, 10);
    else{
        printf("erro no parametro 3\n");
        exit(-1);
    }


    if(strtol(argv[4], argv, 10) != 0)
        tempo_max = strtol(argv[4], argv, 10);
    else{
        printf("erro no parametro 4\n");
        exit(-1);
    }

    printf("%d \n %d\n %d\n %d\n" , numero_pedidos,intervalo,instrucoes,tempo_max);
    return 0;
}