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

typedef struct task{
    int id;
    int n_instrucoes;
    double tempo_maximo;
    int prioridade;
    double tempo;
} task;


//task manager: criar uma fila que ira ter tamanho QUEUE_POS , se a fila ja estiver cheia apagamos a task recevida e escrevemos no log.
//thread scheduler vai ver as tarefas que estao na fila e dar-lhes prioridades. Se chegar um pedido novo as prioridades sao reavaliadas. Se o prazo maximo de execucao de uma tarefa ja tiver passado eliminamos essa tarefa.
//as prioridades sao baseadas no tempo maximo de execucao e , o tempo de chegada a fila.


typedef struct {
    int capac_proc1;
    int capac_proc2;
    int alt_receber_tarefa;
    int nivel_perf; //isto e uma flag?
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

void apagar_task(int indice ){ //TASK MANAGER

    int length = sizeof(&num_tasks) / sizeof(task);
    int queuepos = atoi(queue_pos);

    for (int i = indice - 1; i < length -1; i++)
    {
        num_tasks[i] = num_tasks[i+1]; // assign arr[i+1] to arr[i]
    }

    if(my_sharedm->nivel_perf == 1){
        if(queuepos * 0.2 >= length-1){
            my_sharedm->nivel_perf = 0;
        }
    }
}

void write_file(char string[]){

    log_file  = fopen("log_file.txt", "a");

    pthread_mutex_lock(&mutex_log);

    fprintf(log_file, string , s);
    printf(string , s);

    pthread_mutex_unlock(&mutex_log);
    fclose(log_file);
}

void reavaliar_prioridade(){ //TASK MANAGER

    int prioridade = 1;
    int length = sizeof(&num_tasks) / sizeof(task);

    for(int i = 0 ; i < length ; i++){

        prioridade = 1;

        for(int x = 0 ; x < length ; x++){
            if(num_tasks[i].tempo_maximo < num_tasks[x].tempo_maximo)
                prioridade++;
            else if(num_tasks[i].tempo_maximo == num_tasks[x].tempo_maximo){
                //tem o mesmo tempo maximo mas a task atual foi inserida depois da task a que esta a comparar.
                if(i > x)
                    prioridade++;

            }
        }
        num_tasks[i].tempo = clock() - num_tasks[i].tempo;

        if(num_tasks[i].tempo > num_tasks[i].tempo_maximo){
            write_file("Max time has passed! Removing task...\n");
            apagar_task(i);
        }

        num_tasks[i].prioridade = prioridade;
    }

}


void *vcpu(void *u){

    //vamos fazer um while ate nao ter mais tarefas.


    int capac_proc;
    int id = *((int*)u);
    int array_size = sizeof(&num_tasks) / sizeof(task);
    //determinar o tempo que demora, se for menor que o tempo maximo da task removemos a task.
    //realizar a task que tem prioridade 1
    double time = 0;
    int atual_task = 0;

    for(int i = 0 ; i < array_size-1 ; i++){
        if(num_tasks[i].prioridade == 1){
            atual_task = i;
            break;
        }
    }

    if(my_sharedm->nivel_perf == 0){
        if(my_sharedm->capac_proc1 > my_sharedm->capac_proc2)
            capac_proc = my_sharedm->capac_proc2;
        else
            capac_proc = my_sharedm->capac_proc1;
    }

    if(my_sharedm->nivel_perf == 1){
        if(id == 1)
            capac_proc = my_sharedm->capac_proc2;
        else
            capac_proc = my_sharedm->capac_proc1;
    }

    time = ((double)num_tasks[atual_task].n_instrucoes * 1000) / (capac_proc * 1000000);
    num_tasks[atual_task].tempo = clock() - num_tasks[atual_task].tempo;
    time = time + num_tasks[atual_task].tempo;

    if(time <= num_tasks[atual_task].tempo_maximo){
        pthread_mutex_lock(&mutex);

        //codigo do vcpu

        pthread_mutex_unlock(&mutex);
        //sempre que acaba uma tarefa esta livre e vai chamar o thread dispatcher.
        write_file("Task finished.\n");
    }
    else {
        write_file("Task doesn't meet the time limit! Removing task...\n");
        apagar_task(atual_task);
        reavaliar_prioridade();

    }
    pthread_exit(NULL);
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

    int shmid;

    // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        write_file("%s:Error na funcao shmget.\n");
        exit(1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        write_file("%s:Erro na funcao shmat\n");
        exit(1);
    }

    //informar ao maintenance managers que esta ativo.
    int capacity1 , capacity2;
    char server_name[64];

    strcpy(server_name ,edge_server_name);

    pthread_t thread_vcpu[2];
    int id[2];

    //Normal performance:
    if(my_sharedm->nivel_perf == 0) {
        id[0] = 0;
        pthread_create(&thread_vcpu[0], NULL, vcpu, (void *) &id[0]);
    }

    //High performance:
    if(my_sharedm->nivel_perf == 1){

        for (int i = 0; i < 2; i++) {
            id[i] = i;
            pthread_create(&thread_vcpu[i], NULL, vcpu, (void *) &id[i]);
        }

        for(int i = 0; i < 2; i++) {
            pthread_join(thread_vcpu[i],NULL);
        }

    }

}


void add_task(task added_task){ //TASK MANAGER

    int index = sizeof(&num_tasks) / sizeof(task);
    int queuepos = atoi(queue_pos);
    int maxwait = atoi(max_wait);
    int full = 0;

    if(index == queuepos){
        write_file("Queue full! Removing task...\n");
        full = -1;
    }

    if(full == 0){
        num_tasks[index] = added_task;
        added_task.tempo = clock(); // tem de mudar para momento em que chega do pipe
        if(queuepos * 0.8 <= index+1){
            if(tempmin > maxwait) { //Perguntar o que e o temp_min!
                my_sharedm->nivel_perf = 1;
            }
        }

        //aqui temos de ir para o thread scheduler?
        reavaliar_prioridade();
    }

}


void task_manager() { //TASK MANAGER

    int x = 0;
    int edge_servers;
    char line[64];
    char *tokens;
    int queuepos;

    edge_servers = read_file();
    queuepos = atoi(queue_pos);
    num_tasks = malloc(queuepos * sizeof(task));


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

    int status = 0;

    log_file  = fopen("log_file.txt", "w");
    fclose(log_file);


    t = time(NULL);
    tm = localtime(&t);
    assert(strftime(s, sizeof(s), "%c", tm));


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
