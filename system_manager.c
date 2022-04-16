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


typedef struct {
    int capac_proc1;
    int capac_proc2;
    int alt_receber_tarefa;
    int nivel_perf;
} shared_mem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

shared_mem *my_sharedm;
sem_t semaphore;

time_t t;
struct tm *tm;

char capac1[20];
char capac2[20];
char s[64];
char queue_pos[20];
char max_wait[20];
char edge_server_name[64];

FILE *log_file;
FILE *config_file;

void *vcpu(void *u) {
    pthread_mutex_lock(&mutex);

    //codigo do vcpu

    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int read_file() {
    char num_edge_servers[20];
    int num_servers;

    config_file = fopen("config_file.txt" , "r");

    if(config_file == NULL){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error opening config file.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Error opening config file.\n" , s);
        exit(1);
    }

    if(fscanf(config_file , "%s", queue_pos) != 1){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error reading file.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Error reading file.\n" , s);
    }

    if(fscanf(config_file , "%s", max_wait) != 1){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error reading file.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Error reading file.\n" , s);
    }

    if(fscanf(config_file , "%s", num_edge_servers) != 1){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error reading file.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Error reading file.\n" , s);
    }

    num_servers = atoi(num_edge_servers);

    if(num_servers == 0){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error converting to int.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Error converting to int (WTF) .\n" , s);
        exit(1);
    }

    if(num_servers < 2){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error in number of edge servers.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Error in number of edge servers.\n" , s);
        exit(1);
    }

    return num_servers;
}


void thread_scheduler(){


}



void edge_server() {

    //long capacity1 , capacity2;
    char server_name[64];

    //capacity1 = my_sharedm->capac_proc1;
    //capacity2 = my_sharedm->capac_proc2;

    strcpy(server_name ,edge_server_name);

    pthread_t thread_vcpu[2];
    int id[2];

    for (int i = 0; i < 2; i++) {
        id[i] = i;
        pthread_create(&thread_vcpu[i], NULL, vcpu, (void *) &id[i]);
    }

    for(int i = 0; i < 2; i++) {
        pthread_join(thread_vcpu[i],NULL);
    }

}



void task_manager() {

    int x = 0;
    int edge_servers;
    char line[64];
    char *tokens;

    edge_servers = read_file();


    for (int i = 0 ; i < edge_servers ; i++) {

        if (fscanf(config_file , "%s" , line) != 1) {
            pthread_mutex_lock(&mutex_log);
            fprintf(log_file, "%s:Error reading file.\n" , s);
            pthread_mutex_unlock(&mutex_log);

            printf("%s:Error reading file.\n" , s);
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
            pthread_mutex_lock(&mutex_log);
            fprintf(log_file, "%s:Error converting to int.\n" , s);
            pthread_mutex_unlock(&mutex_log);

            printf("%s:Error converting to int.\n" , s);
            exit(1);
        }
        if(my_sharedm->capac_proc2 == 0){
            pthread_mutex_lock(&mutex_log);
            fprintf(log_file, "%s:Error converting to int.\n" , s);
            pthread_mutex_unlock(&mutex_log);

            printf("%s:Error converting to int.\n" , s);
            exit(1);
        }

        if(fork() == 0) {
            pthread_mutex_lock(&mutex_log);
            fprintf(log_file, "%s:Process edge_server created.\n" , s);
            pthread_mutex_unlock(&mutex_log);

            printf("%s:Process edge_server created.\n" , s);

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

    log_file  = fopen("log_file.txt", "w");

    if(log_file == NULL){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error opening log file.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Error opening log file.\n" , s);

        exit(1);
    }

    int shmid;
    int status = 0;

    t = time(NULL);
    tm = localtime(&t);
    assert(strftime(s, sizeof(s), "%c", tm));

    // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Error na funcao shmget.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Erro na funcao shmget\n" , s);

        exit(1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Erro na funcao shmat.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Erro na funcao shmat\n" , s);

        exit(1);
    }

    if(fork() == 0){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Process Thread Scheduler created.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Process Thread Scheduler created.\n" , s);

        thread_scheduler();
        exit(0);
    }


    if(fork() == 0) {
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Process Task Manager created.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Process Task Manager created.\n" , s);

        task_manager();
        exit(0);
    }

    if(fork() == 0){
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Process Monitor created.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Process Monitor created.\n" , s);

        monitor();
        exit(0);
    }

    if(fork() == 0) {
        pthread_mutex_lock(&mutex_log);
        fprintf(log_file, "%s:Process Maintenance Manager created.\n" , s);
        pthread_mutex_unlock(&mutex_log);

        printf("%s:Process Maintenance Manager created.\n" , s);

        maintenance_manager();
        exit(0);
    }

    while ((wait(&status)) > 0);

    pthread_mutex_lock(&mutex_log);
    fprintf(log_file, "%s:Simulator closed.\n" , s);
    pthread_mutex_unlock(&mutex_log);

    printf("%s:Simulator closed.\n" , s);

    fclose(log_file);

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_log);

    return 0;
}
