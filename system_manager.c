#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>


typedef struct {
    int capacidade_processamento;
    int altura_processamento;
    float precoTotal; //METER AQUI VALORES QUE QUEREMOS QUE AINDA NAO SEI.
    int pesoTotal;
    int cliente;
} shared_mem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
shared_mem *my_sharedm;


void task_manager()
{

}

void monitor()
{

}

void mantinance_manager()
{

}

int main() {
    int shmid;
    printf("Hello, World!\n");

    // Criar o segmento de memória partilhada
	if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
		perror("Erro no shmget com IPC_CREAT\n");
		exit(1);
	}

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        perror("error in shmat");
        exit(1);
    }

    if(fork() == 0)
    {
        task_manager();
        exit(0);
    }
    if(fork() == 0)
    {
        monitor();
        exit(0);
    }
    if(fork() == 0)
    {
        mantinance_manager();
        exit(0);
    }



    return 0;
}
