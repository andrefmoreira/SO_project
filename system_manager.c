/*
Trabalho realizado por:
André Filipe de Oliveira Nº 2020239416
Pedro Miguel Pereira Catorze Nº 2020222916
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>

typedef struct{
    int id;
    int n_instrucoes;
    int tempo_maximo;
    int prioridade;
}task;


//task manager: criar uma fila que ira ter tamanho QUEUE_POS , se a fila ja estiver cheia apagamos a task recevida e escrevemos no log.
//thread scheduler vai ver as tarefas que estao na fila e dar-lhes prioridades. Se chegar um pedido novo as prioridades sao reavaliadas. Se o prazo maximo de execucao de uma tarefa ja tiver passado eliminamos essa tarefa.
//as prioridades sao baseadas no tempo maximo de execucao e , o tempo de chegada a fila.


typedef struct {
    int capac_proc1;
    int capac_proc2;
    int alt_receber_tarefa;
    int nivel_perf;
} shared_mem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

shared_mem *my_sharedm;

time_t t;
struct tm *tm;

char capac1[20];
char capac2[20];
char s[64];
char queue_pos[20];
char max_wait[20];
char edge_server_name[64];
struct task *num_tasks;

FILE *log_file;
FILE *config_file;


void *vcpu(void *u) {

    //determinar o tempo que demora, se for menor que o tempo maximo da task removemos a task.
    //realizar a task que tem prioridade 1


    pthread_mutex_lock(&mutex);

    //codigo do vcpu

    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}


void write_file(char string[]){

    log_file  = fopen("log_file.txt", "a");


    if(log_file == NULL){
        write_file("%s:Error opening log file.\n");
        exit(1);
    }

    pthread_mutex_lock(&mutex_log);

    fprintf(log_file, string , s);
    printf(string , s);

    pthread_mutex_unlock(&mutex_log);
    fclose(log_file);
}


int read_file() {
    char num_edge_servers[20];
    int num_servers;

    config_file = fopen("config_file.txt" , "r");

    if(config_file == NULL){
        write_file("%s:Error opening config file.\n");
        exit(1);
    }

    if(fscanf(config_file , "%s", queue_pos) != 1){
        write_file("%s:Error reading file.\n");
        exit(1);
    }

    if(fscanf(config_file , "%s", max_wait) != 1){
        write_file("%s:Error reading file.\n");
        exit(1);
    }

    if(fscanf(config_file , "%s", num_edge_servers) != 1){
        write_file("%s:Error reading file.\n");
        exit(1);
    }

    num_servers = atoi(num_edge_servers);

    if(num_servers == 0){
        write_file("%s:Error converting to int.\n");
        exit(1);
    }

    if(num_servers < 2){
        write_file("%s:Error in number of edge servers.\n" );
        exit(1);
    }

    return num_servers;
}


void thread_scheduler(){


}



void edge_server() {

    int capacity1 , capacity2;
    char server_name[64];

    capacity1 = my_sharedm->capac_proc1;
    capacity2 = my_sharedm->capac_proc2;

    strcpy(server_name ,edge_server_name);

    pthread_t thread_vcpu[2];
    int id[2];


    //se estiver num nivel de so utilizar um , tem o nivel de so usar o segundo ou so usar o primeiro.
    id[i] = i;
    pthread_create(&thread_vcpu[i], NULL, vcpu, (void *) &id[i]);

    //se for um dos niveis ativa mos os dois.
    for (int i = 0; i < 2; i++) {
        id[i] = i;
        pthread_create(&thread_vcpu[i], NULL, vcpu, (void *) &id[i]);
    }

    for(int i = 0; i < 2; i++) {
        pthread_join(thread_vcpu[i],NULL);
    }

}



void reavaliar_prioridade(){

    int prioridade = 1;
    int length = sizeof(num_tasks) / sizeof(task);

    for(int i = 0 ; i < length ; i++){

        prioridade = 1;

        for(int x = 0 ; x < length ; i++){
            
            if(num_tasks[i].tempo_maximo < num_tasks[x].tempo_maximo)
                prioridade++; 
            else if(num_tasks[i].tempo_maximo == num_tasks[x].tempo_maximo){
                //tem o mesmo tempo maximo mas a task atual foi inserida depois da task a que esta a comparar.
                if(i > x)
                    prioridade++;

            }
        }

        num_tasks[i].prioridade = prioridade;
    }

}

void apagar_task(int indice ){

    int length = sizeof(num_tasks) / sizeof(task);

    for (i = indice - 1; i < length -1; i++)  
    {  
        num_tasks[i] = num_tasks[i+1]; // assign arr[i+1] to arr[i]  
    }  
    
}


void task_manager() {

    int x = 0;
    int edge_servers;
    char line[64];
    char *tokens;

    edge_servers = read_file();

    num_tasks = malloc(queue_pos * sizeof(task));


    for (int i = 0 ; i < edge_servers ; i++) {

        if (fscanf(config_file , "%s" , line) != 1) {
            write_file("%s:Error reading file.\n");
            exit(1);
        }

        tokens = strtok(line, ",");
        strcpy(edge_server_name, tokens);

        tokens = strtok(NULL, ",");
        strcpy(capac1, tokens);

        tokens = strtok(NULL, ",");
        strcpy(capac2, tokens);

        my_sharedm->capac_proc1 = atoi(capac1);
        my_sharedm->capac_proc2 = atoi(capac2);


        if(my_sharedm->capac_proc1 == 0){
            write_file("%s:Error converting to int.\n");
            exit(1);
        }
        if(my_sharedm->capac_proc2 == 0){
            write_file("%s:Error converting to int.\n");
            exit(1);
        }

        if(fork() == 0) {
            write_file("%s:Process edge_server created.\n");

            edge_server();
            exit(0);
        }
    }

    while (wait(&x) > 0);
}


void monitor() {

}


void maintenance_manager() {

}


int main() {

    int shmid;
    int status = 0;

    log_file  = fopen("log_file.txt", "w");
    fclose(log_file);


    t = time(NULL);
    tm = localtime(&t);
    assert(strftime(s, sizeof(s), "%c", tm));

    // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        write_file("%s:Error na funcao shmget.\n");
        exit(1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        write_file("%s:Erro na funcao shmat\n");
        exit(1);
    }

    if(fork() == 0){
        write_file("%s:Process Thread Scheduler created.\n");

        thread_scheduler();
        exit(0);
    }


    if(fork() == 0) {
        write_file("%s:Process Task Manager created.\n");

        task_manager();
        exit(0);
    }

    if(fork() == 0){
        write_file("%s:Process Monitor created.\n");

        monitor();
        exit(0);
    }

    if(fork() == 0) {
        write_file("%s:Process Maintenance Manager created.\n");

        maintenance_manager();
        exit(0);
    }

    while ((wait(&status)) > 0);

    write_file("%s:Simulator closed.\n");

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_log);

    return 0;
}
