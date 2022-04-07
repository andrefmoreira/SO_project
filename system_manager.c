#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <pthread.h>


typedef struct {
    int capac_proc;
    int alt_receber_tarefa;
    int nivel_perf;
} shared_mem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
shared_mem *my_sharedm;


void *vcpu(void *t) {

}


void edge_server() {
    pthread_t thread_vcpu[2];
    int id[2];
    for (int i = 0; i < 2; i++) {
        pthread_create(&thread_vcpu[i], NULL, vcpu, (void *) &id[i]);
    }
}



void task_manager() {

    if(fork() == 0) {
        edge_server();
        exit(0);
    }
}


void monitor() {

}


void maintenance_manager() {

}


int main() {
    int shmid;

    // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        perror("Erro na função shmget\n");
        exit(1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        perror("Erro na função shmat\n");
        exit(1);
    }
    if(fork() == 0) {
        task_manager();
        exit(0);
    }
    if(fork() == 0){
        monitor();
        exit(0);
    }
    if(fork() == 0) {
        maintenance_manager();
        exit(0);
    }

    return 0;
}
