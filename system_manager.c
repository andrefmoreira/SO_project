#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <string.h>


typedef struct {
    long capac_proc1;
    long capac_proc2;
    int alt_receber_tarefa;
    int nivel_perf;
} shared_mem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
shared_mem *my_sharedm;

time_t t;
struct tm *tm;

char s[64];
char queue_pos[20];
char max_wait[20];
char edge_server_name[64];

FILE *log_file;
FILE *config_file;

void *vcpu(void *u) {

}

long read_file(){

    char *pointer;
    char num_edge_servers[20];
    long num_servers;

    config_file = fopen("config_file.txt" , "r");

    if(config_file == NULL){
        fprintf(log_file, "%s:Error opening config file.\n" , s);
        printf("%s:Error opening config file.\n" , s);

        exit(1);
    }

    if(fscanf(config_file , "%s", queue_pos) != 1){
        fprintf(log_file, "%s:Error reading file.\n" , s);
        printf("%s:Error reading file.\n" , s);
    }

    if(fscanf(config_file , "%s", max_wait) != 1){
        fprintf(log_file, "%s:Error reading file.\n" , s);
        printf("%s:Error reading file.\n" , s);
    }

    if(fscanf(config_file , "%s", num_edge_servers) != 1){
        fprintf(log_file, "%s:Error reading file.\n" , s);
        printf("%s:Error reading file.\n" , s);
    }

    num_servers = strtol(num_edge_servers , &pointer , 10);

    if(num_servers == 0){
        fprintf(log_file, "%s:Error converting to long.\n" , s);
        printf("%s:Error converting to long.\n" , s);
        exit(1);
    }

    if(num_servers < 2){
        fprintf(log_file, "%s:Error in number of edge servers.\n" , s);
        printf("%s:Error in number of edge servers.\n" , s);
        exit(1);
    }

    return num_servers;
}

void edge_server() {
    long capacity1 , capacity2;
    char server_name[64];

    capacity1 = my_sharedm->capac_proc1;
    capacity2 = my_sharedm->capac_proc2;

    strcpy(server_name ,edge_server_name);

    pthread_t thread_vcpu[2];
    int id[2];

    for (int i = 0; i < 2; i++) {
        pthread_create(&thread_vcpu[i], NULL, vcpu, (void *) &id[i]);
    }
}



void task_manager() {

    char capac1[20];
    char capac2[20];
    char *ptr;
    long edge_servers;

    edge_servers = read_file();

    for(int i = 0 ; i < edge_servers ; i++){

        if(fscanf(config_file , "%s,%s,%s" , edge_server_name , capac1 , capac2) != 1){
            fprintf(log_file, "%s:Error reading file.\n" , s);
            printf("%s:Error reading file.\n" , s);
        }

        my_sharedm->capac_proc1 = strtol(capac1 , &ptr , 10);
        my_sharedm->capac_proc2 = strtol(capac2 , &ptr , 10);

        if(my_sharedm->capac_proc1 == 0){
            fprintf(log_file, "%s:Error converting to long.\n" , s);
            printf("%s:Error converting to long.\n" , s);
            exit(1);
        }
        if(my_sharedm->capac_proc2 == 0){
            fprintf(log_file, "%s:Error converting to long.\n" , s);
            printf("%s:Error converting to long.\n" , s);
            exit(1);
        }

        if(fork() == 0) {
            fprintf(log_file, "%s:Process Maintenance Manager created.\n" , s);
            printf("%s:Process edge_server created.\n" , s);

            edge_server();
            exit(0);
        }
    }
}


void monitor() {

}


void maintenance_manager() {

}


int main() {
    log_file  = fopen("log_file.txt", "w");

    if(log_file == NULL){
        fprintf(log_file, "%s:Error opening log file.\n" , s);
        printf("%s:Error opening log file.\n" , s);

        exit(1);
    }

    int shmid;
    int status;

    t = time(NULL);
    tm = localtime(&t);
    assert(strftime(s, sizeof(s), "%c", tm));

    // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        fprintf(log_file, "%s:Error na funcao shmget.\n" , s);
        printf("%s:Erro na funcao shmget\n" , s);

        exit(1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        fprintf(log_file, "%s:Erro na funcao shmat.\n" , s);
        printf("%s:Erro na funcao shmat\n" , s);

        exit(1);
    }

    if(fork() == 0) {
        fprintf(log_file, "%s:Process Task Manager created.\n" , s);
        printf("%s:Process Task Manager created.\n" , s);

        task_manager();
        exit(0);
    }

    if(fork() == 0){
        fprintf(log_file, "%s:Process Monitor created.\n" , s);
        printf("%s:Process Monitor created.\n" , s);

        monitor();
        exit(0);
    }

    if(fork() == 0) {
        fprintf(log_file, "%s:Process Maintenance Manager created.\n" , s);
        printf("%s:Process Maintenance Manager created.\n" , s);

        maintenance_manager();
        exit(0);
    }

    fprintf(log_file, "%s:Simulator waitting for last tasks to finish.\n" , s);
    printf("%s:Simulator waitting for last tasks to finish.\n" , s);

    while ((wait(&status)) > 0);

    fprintf(log_file, "%s:Simulator closed.\n" , s);
    printf("%s:Simulator closed.\n" , s);

    fclose(log_file);

    return 0;
}
