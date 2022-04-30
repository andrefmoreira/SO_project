/* 
Trabalho realizado por:
André Filipe de Oliveira Moreira Nº 2020239416
Pedro Miguel Pereira Catorze Nº 2020222916
*/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define PIPE_NAME "TASK_PIPE"


typedef struct task {
    int id;
    int num_instr;
    double temp_max;
    int prioridade;
    double time;
} task;


int main(int argc, char *argv[]) {
    int num_ped, intervalo;
    task t;

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
        t.num_instr = atoi(argv[3]);
    } else {
        printf("Error on the third parameter!\n");
        exit(-1);
    }


    if (atoi(argv[4]) != 0) {
        t.temp_max = atof(argv[4]);
    } else {
        printf("Error on the fourth parameter!\n");
        exit(-1);
    }



    int fd;

    if ((fd = open(PIPE_NAME, O_WRONLY)) < 0) {
        perror("Cannot open pipe for writing: ");
        exit(0);
    }


    t.prioridade = 1;
    t.time = 0;


    int intervalo_micro = intervalo * 1000;

    for(int i = 0 ; i < num_ped ; i++){
    
        t.id = i;
        write(fd, &t, sizeof(task));
		printf("task %d enviada.\n",i);
        usleep(intervalo_micro);
    }

    return 0;
}
