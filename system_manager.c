/*
Trabalho realizado por:
André Filipe de Oliveira Moreira Nº 2020239416
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
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PIPE_NAME "TASK_PIPE"


typedef struct task {
    int id;
    int num_instr;
    double temp_max;
    int prioridade;
    double time;
} task;


//task manager: criar uma fila que ira ter tamanho QUEUE_POS , se a fila ja estiver cheia apagamos a task recevida e escrevemos no log.
//thread scheduler vai ver as tarefas que estao na fila e dar-lhes prioridades. Se chegar um pedido novo as prioridades sao reavaliadas. Se o prazo maximo de execucao de uma tarefa ja tiver passado eliminamos essa tarefa.
//as prioridades sao baseadas no tempo maximo de execucao e , o tempo de chegada a fila.


typedef struct {
    int capac_proc1;
    int capac_proc2;
    int alt_receber_tarefa; //o que e isto?
    int nivel_perf; 
} shared_mem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond_var = PTHREAD_COND_INITIALIZER;

shared_mem *my_sharedm;

time_t t;
struct tm *tm;

char capac1[20];
char capac2[20];
char s[64];
char queue_pos[20];
char max_wait[20];
char edge_server_name[64];
int fd;
int shmid;
int length = 0;
int fim = 0;
int tasks_executed = 0;
double tempo_total = 0;
struct task *num_tasks;


task t2;
FILE *log_file;
FILE *config_file;

void write_file(char string[]){

    log_file  = fopen("log_file.txt", "a");

    pthread_mutex_lock(&mutex_log);

    fprintf(log_file, string , s);
    printf(string , s);

    pthread_mutex_unlock(&mutex_log);
    fclose(log_file);
}


void delete_task(int indice ){ //TASK MANAGER

    int queuepos = atoi(queue_pos);
    int prioridade_task = num_tasks[indice].prioridade;

    for (int i = length; i < length -1; i++)
    {   
        if(prioridade_task < num_tasks[i+1].prioridade)
            num_tasks[i].prioridade--;

        num_tasks[i] = num_tasks[i+1]; // assign arr[i+1] to arr[i]
    }
    length--;
}

void reavaliar_prioridade(){ //TASK MANAGER


    printf("reavaliating priority\n");
    int prioridade = 1;
    
    printf("before reavaliating\n");
    for(int i = 0 ; i < length ; i++){
    	printf("MAX TIME %f , PRIORITY %d ||",num_tasks[i].temp_max , num_tasks[i].prioridade);
    }
	printf("\n");
    for(int i = 0 ; i < length ; i++){

        prioridade = 1;

        for(int x = 0 ; x < length ; x++){
            if(num_tasks[i].temp_max > num_tasks[x].temp_max)
                prioridade++;
            else if(num_tasks[i].temp_max == num_tasks[x].temp_max){
                //tem o mesmo tempo maximo mas a task atual foi inserida depois da task a que esta a comparar.
                if(i > x)
                    prioridade++;

            }
        }
        num_tasks[i].time = clock() - num_tasks[i].time;

        if(num_tasks[i].time > num_tasks[i].temp_max){
            write_file("Max time has passed! Removing task...\n");
            delete_task(i);
        }

        num_tasks[i].prioridade = prioridade;
    }
	
	printf("After reavaliating\n");
	for(int i = 0 ; i < length ; i++){
    	printf("TASK ID %f , PRIORITY %d ||",num_tasks[i].temp_max , num_tasks[i].prioridade);
    }
	printf("\n");
}



void add_task(task added_task){ //TASK MANAGER

    int queuepos = atoi(queue_pos);
    int maxwait = atoi(max_wait);
    int full = 0;

    printf("Adding task %d\n" , added_task.id);

    if(length == queuepos){
        write_file("Queue full! Removing task...\n");
        full = -1;
    }

    if(full == 0){
        num_tasks[length] = added_task;
    	printf("%d\n" , num_tasks[length].id); 
    	printf("%d\n" , length);

        length++;
        reavaliar_prioridade();
    }

}

//TEMOS DE SEPARAR EM VCPU_MIN E VCPU_MAX PARA SER MAIS FACIL
void *vcpu(void *u){

    do{
        //pthread_cond_wait(&tarefa) fazer aqui uma variavel de condiçao ate receber a tarefa pelo unamed pipe.
        int capac_proc;
        int id = *((int*)u);

        //determinar o tempo que demora, se for menor que o tempo maximo da task removemos a task.
        //realizar a task que tem prioridade 1
        double time = 0;
        int atual_task = 0;

        for(int i = 0 ; i < length ; i++){   // a task vai ser enviada por isso tiramos isto depois
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
        //ESTA PARTE TAMBEM VAI SER REMOVIDA
        if(my_sharedm->nivel_perf == 1){
            if(id == 1)
                capac_proc = my_sharedm->capac_proc2;
            else
                capac_proc = my_sharedm->capac_proc1;
        }

        //tempo minimo e o tempo que num momento T o vcpu demora a ficar livre, ou seja vcpu ta ocupado, no melhor dos casos no momento T+X o vcpu esta livre, ou seja temp min = X.

        time = ((double)num_tasks[atual_task].num_instr * 1000) / (capac_proc * 1000000);
        num_tasks[atual_task].time = clock() - num_tasks[atual_task].time;
        time = time + num_tasks[atual_task].time;

        if(time <= num_tasks[atual_task].temp_max){
            pthread_mutex_lock(&mutex);

            //codigo do vcpu

            pthread_mutex_unlock(&mutex);
            //sempre que acaba uma tarefa esta livre e vai chamar o thread dispatcher.

            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond_var);
            pthread_mutex_unlock(&mutex);
            //perguntar como e que identificamos qual vcpu esta livre.
            write_file("Task finished successfully.\n");
            tasks_executed++;
            tempo_total += time;
        }
        else {
            write_file("Task doesn't meet the time limit! Removing task...\n");
            delete_task(atual_task);
            reavaliar_prioridade();

        }
    }while(fim == 0);
    pthread_exit(NULL);
}


void le_pipe(){

    if ((fd = open(PIPE_NAME, O_RDONLY)) < 0) {
        write_file("Error oppening pipe for reading.\n");
        exit(0);
    }
    

    if(read(fd, &t2, sizeof(task)) == -1)
    	printf("Erro a ler.");
    	
    t2.time = clock();
    
    
}
    

int read_file() {
    char num_edge_servers[20];
    int num_servers;

    config_file = fopen("config_file.txt" , "r");

    if(config_file == NULL){
        write_file("%s:Error opening config file!\n");
        exit(1);
    }

    if(fscanf(config_file , "%s", queue_pos) != 1){
        write_file("%s:Error reading file!\n");
        exit(1);
    }

    if(fscanf(config_file , "%s", max_wait) != 1){
        write_file("%s:Error reading file!\n");
        exit(1);
    }

    if(fscanf(config_file , "%s", num_edge_servers) != 1){
        write_file("%s:Error reading file!\n");
        exit(1);
    }

    num_servers = atoi(num_edge_servers);

    if(num_servers == 0){
        write_file("%s:Error converting to int!\n");
        exit(1);
    }

    if(num_servers < 2){
        write_file("%s:Wrong number of edge servers!\n" );
        exit(1);
    }

    return num_servers;
}

void * thread_dispatcher{

}


void * thread_scheduler(void *x){

    do{
    
		le_pipe();

        printf("task %d just arrived\n" , t2.id);
        add_task(t2); //esta a enviar a ultima task 2 vezes

    }while(fim == 0);

}



void edge_server() {

    // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        write_file("%s:Error on shmget function!\n");
        exit(1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        write_file("%s:Error on shmat function!\n");
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


void task_manager() { //TASK MANAGER
	
    int x = 0;
    int edge_servers;
    char line[64];
    char *tokens;
    int queuepos;
    int id_scheduler = 1;
    pthread_t thread_sched;

    edge_servers = read_file();
    queuepos = atoi(queue_pos);
    num_tasks = malloc(queuepos * sizeof(task));
    
    pthread_create(&thread_sched, NULL, thread_scheduler, (void *) &id_scheduler);
	

    for (int i = 0 ; i < edge_servers ; i++) {

        if (fscanf(config_file , "%s" , line) != 1) {
            write_file("%s:Error reading file!\n");
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
            write_file("%s:Error converting to int!\n");
            exit(1);
        }
        if(my_sharedm->capac_proc2 == 0){
            write_file("%s:Error converting to int!\n");
            exit(1);
        }

        if(fork() == 0) {
            write_file("%s:Process edge_server created.\n");

            edge_server();
            exit(0);
        }
    }

    while (wait(&x) > 0);
    pthread_join(thread_sched,NULL);
}


void monitor() 
{
    do{
        if(queuepos * 0.8 <= lenght){
            if(tempmin > maxwait) { //falta o tempmin...
                my_sharedm->nivel_perf = 1;
            }
        }

        if(my_sharedm->nivel_perf == 1){
            if(queuepos * 0.2 >= length){
                my_sharedm->nivel_perf = 0;
            }
        }
    }while(fim == 0);
}


void maintenance_manager() {

}


void end(){

    write_file("Signal SIGINT received ... waiting for last tasks to close simulator.");
    fim = 1;
    unlink(PIPE_NAME);
    
    write_file("Tasks that were not completed: \n");

    for(int i = 0; i < length ; i++){
        write_file("Task ID: %d, Task Priority: %d\n", num_tasks[i].id , num_tasks[i].priority);
    }


    write_file("%s:Simulator closed.\n");
    pthread_mutex_destroy(&mutex);    //falta por todos os que usamos aqui.
    pthread_mutex_destroy(&mutex_log);

    exit(0);
}

void stats(){
    write_file("Tasks executed: %d \n" , tasks_executed);

    double average_time = 0;
    average_time = tempo_total / tasks_executed;

    write_file("Average response time: %d \n", average_time);

    //FALTA COISAS AQUI

    write_file("Number of tasks that have not been executed: %d \n" , lenght);
}


int main() {

    int status = 0;

    signal(SIGINT, end);
    signal(SIGTSTP, stats);


    log_file  = fopen("log_file.txt", "w");
    fclose(log_file);
    
        // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        write_file("%s:Error on shmget function!\n");
        exit(1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        write_file("%s:Error on shmat function!\n");
        exit(1);
    }



    if (mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0 && (errno!= EEXIST)) {
        write_file("Error creating pipe.\n");
        exit(0);
    }


    t = time(NULL);
    tm = localtime(&t);
    assert(strftime(s, sizeof(s), "%c", tm));


    if(fork() == 0){
        write_file("%s:Process Monitor created.\n");

        monitor();
        exit(0);
    }
    
    if(fork() == 0) {
        write_file("%s:Process Task Manager created.\n");
        
        task_manager();
        exit(0);
    }


    if(fork() == 0) {
        write_file("%s:Process Maintenance Manager created.\n");
		
        maintenance_manager();
        exit(0);
    }

    return 0;
}
