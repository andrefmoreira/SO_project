/* 
Trabalho realizado por:
André Filipe de Oliveira Moreira Nº 2020239416
Pedro Miguel Pereira Catorze Nº 2020222916
*/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define PIPE_NAME "TASK_PIPE"


typedef struct task {
    int id;
    int num_instr;
    double temp_max;
    int priority;
    double time;
} task;


int main(int argc, char *argv[]) {

    int num_requests, interval;
    task t;

    if (argc != 5) {
        printf("Wrong number of parameters!\n");
        exit(-1);
    }

    if (atoi(argv[1]) == 0) {
        printf("Error on the first parameter!\n");
        exit(-1);
    }
    num_requests = atoi(argv[1]);


    if (atoi(argv[2]) == 0) {
        printf("Error on the second parameter!\n");
        exit(-1);
    }
    interval = atoi(argv[2]);


    if (atoi(argv[3]) == 0) {
        printf("Error on the third parameter!\n");
        exit(-1);
    }
    t.num_instr = atoi(argv[3]);


    if (atoi(argv[4]) == 0) {
        printf("Error on the fourth parameter!\n");
        exit(-1);
    }
    t.temp_max = atof(argv[4]);


    int fd;
    if ((fd = open(PIPE_NAME, O_WRONLY)) < 0) {
        perror("Can't open pipe for writting!\n");
        exit(-1);
    }


    t.priority = 1;
    t.time = 0;


    int interval_micro = interval * 1000;

    char *string = NULL;
    sprintf(string, "%d %d %lf %d %lf", num_requests, interval, t.temp_max , t.priority, t.time);

    for (int i = 0 ; i < num_requests ; i++) {
        t.id = i;
        write(fd, &t, sizeof(task));
		printf("task %d enviada.\n",i);
        usleep(interval_micro);
    }

    return 0;
}
