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
#include <sys/msg.h>
#include <sys/types.h>

#define PIPE_NAME "TASK_PIPE"


typedef struct task {
    int id;
    int num_instr;
    double temp_max;
    int priority;
    double time;
} task;


typedef struct {
    long mtype;
    int number;
} mq_msg;

typedef struct {
	int queuepos;
	int maxwait;
	int edgeservers;
    int alt_receber_tarefa; //o que e isto?
    int nivel_perf;
    char name[64];
    int capac_proc1;
    int capac_proc2;
    int length;
    int tasks_executed;
    double total_time;  
} shared_mem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mq_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *semaphore , *sem_pipe , *sem_mq , *sem_mm;
pthread_cond_t  cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t maintenance = PTHREAD_COND_INITIALIZER;

shared_mem *my_sharedm;

time_t t;
struct tm *tm;

char s[64];
char queue_pos[20];
char max_wait[20];
char text_pipe[128];
int edge_servers = 0;
int finish_vcpumin = 0;
int finish_vcpumax = 0;
int fd;
int mq;
int shmid;
int end = 0;
int tasks_executed = 0;
double tempo_total = 0;
struct task *num_tasks;

task t2;
FILE *log_file;
FILE *config_file;



void write_file(char string[]){
	
	sem_wait(semaphore);

    log_file  = fopen("log_file.txt", "a");

    fprintf(log_file, string , s);
    printf(string , s);

    fclose(log_file);
    
    sem_post(semaphore);
}

void ignore_signal(){

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

}

void delete_task(int indice ){ //TASK MANAGER

    int task_priority = num_tasks[indice].priority;

    for (int i = 0; i < my_sharedm->length-1; i++)
    {   
        if(task_priority < num_tasks[i+1].priority)
            num_tasks[i].priority--;

        num_tasks[i] = num_tasks[i+1]; // assign arr[i+1] to arr[i]
    }
    my_sharedm->length--;
}

void reavaliar_prioridade(){ //TASK MANAGER


    //printf("reavaliating priority\n");
    int priority = 1;
    
    //printf("before reavaliating\n");
    for(int i = 0 ; i < my_sharedm->length ; i++){
    	//printf("MAX TIME %f , PRIORITY %d ||",num_tasks[i].temp_max , num_tasks[i].priority);
    }
	printf("\n");
    for(int i = 0 ; i < my_sharedm->length ; i++){

        priority = 1;

        for(int x = 0 ; x < my_sharedm->length ; x++){
            if(num_tasks[i].temp_max > num_tasks[x].temp_max)
                priority++;
            else if(num_tasks[i].temp_max == num_tasks[x].temp_max){
                //tem o mesmo tempo maximo mas a task atual foi inserida depois da task a que esta a comparar.
                if(i > x)
                    priority++;

            }
        }
        num_tasks[i].time = clock() - num_tasks[i].time;

        if(num_tasks[i].time > num_tasks[i].temp_max){
            write_file("%s:Max time has passed! Removing task...\n");
            delete_task(i);
        }

        num_tasks[i].priority = priority;
    }
	
	//printf("After reavaliating\n");
	for(int i = 0 ; i < my_sharedm->length ; i++){
    	//printf("TASK ID %f , PRIORITY %d ||",num_tasks[i].temp_max , num_tasks[i].priority);
    }
	printf("\n");
}



void add_task(task added_task){ //TASK MANAGER

    int maxwait = atoi(max_wait);
    int full = 0;

    //printf("Adding task %d\n" , added_task.id);

    if(my_sharedm->length == my_sharedm->queuepos){
        write_file("%s:Queue full! Removing task...\n");
        full = -1;
    }

    if(full == 0){
        num_tasks[my_sharedm->length] = added_task;
    	//printf("%d\n" , num_tasks[length].id); 
    	//printf("%d\n" , length);

        my_sharedm->length++;
        reavaliar_prioridade();
    }

}

//TEMOS DE SEPARAR EM VCPU_MIN E VCPU_MAX PARA SER MAIS FACIL
void *vcpu_min(void *u){
    
    int capac_proc = 0;

    if(my_sharedm->capac_proc1 > my_sharedm->capac_proc2)
        capac_proc = my_sharedm->capac_proc2;
    else
        capac_proc = my_sharedm->capac_proc1;

    while(finish_vcpumin == 0){
        //pthread_cond_wait(&tarefa) fazer aqui uma variavel de condiçao ate receber a tarefa pelo unamed pipe.
        int capac_proc;
        int id = *((int*)u);
        
        
        double time = 0;
        int atual_task = 0;

        //tempo minimo e o tempo que num momento T o vcpu demora a ficar livre, ou seja vcpu ta ocupado, no melhor dos casos no momento T+X o vcpu esta livre, ou seja temp min = X.

        time = ((double)num_tasks[atual_task].num_instr * 1000) / (capac_proc * 1000000);
        num_tasks[atual_task].time = clock() - num_tasks[atual_task].time;
        time = time + num_tasks[atual_task].time;

        if(time <= num_tasks[atual_task].temp_max){
            //sempre que acaba uma tarefa esta livre e vai chamar o thread dispatcher.

            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond_var);
            pthread_mutex_unlock(&mutex);
            //perguntar como e que identificamos qual vcpu esta livre.
            write_file("%s:Task finished successfully.\n");
            my_sharedm[id].tasks_executed++;
            my_sharedm[id].total_time += time;
        }
    }
    pthread_exit(NULL);
}


void * vcpu_max(void *m){

    printf("cheguei !!! \n");
    int capacproc = 0;

    if(my_sharedm->capac_proc1 < my_sharedm->capac_proc2)
        capacproc = my_sharedm->capac_proc2;
    else{
        capacproc = my_sharedm->capac_proc1;
	}
	
	while(finish_vcpumax == 0){
        //pthread_cond_wait(&tarefa) fazer aqui uma variavel de condiçao ate receber a tarefa pelo unamed pipe.
        int capac_proc;
        int id = *((int*)m);

        //determinar o tempo que demora, se for menor que o tempo maximo da task removemos a task.
        //realizar a task que tem priority 1
        double time = 0;
        int atual_task = 0;

        //tempo minimo e o tempo que num momento T o vcpu demora a ficar livre, ou seja vcpu ta ocupado, no melhor dos casos no momento T+X o vcpu esta livre, ou seja temp min = X.

        time = ((double)num_tasks[atual_task].num_instr * 1000) / (capac_proc * 1000000);
        num_tasks[atual_task].time = clock() - num_tasks[atual_task].time;            //ISTO NAO DEVE SER AQUI, ACHO QUE E NO DISPATCHER.
        time = time + num_tasks[atual_task].time;

        

        if(time <= num_tasks[atual_task].temp_max){ // ESTA CONFIRMACAO TBM VAI SAIR E VAI SER FEITA PELO DISPATCHER. 

            //VCPU SO VAI FICAR A DAR READ AO UNAMED PIPE ATE RECEBER UMA MENSAGEM, DEPOIS FAZ A TAREFA E VOLTA A ESPERAR POR UMA NOVA.
            //sempre que acaba uma tarefa esta livre e vai chamar o thread dispatcher.

            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond_var);
            pthread_mutex_unlock(&mutex);
            //perguntar como e que identificamos qual vcpu esta livre.
            write_file("%s:Task finished successfully.\n");
            my_sharedm[id].tasks_executed++;
            my_sharedm[id].total_time += time;
        }
    }
    pthread_exit(NULL);



}


void finish(){

	int status1 = 0;
    char phrase[64];

    //vamos ter de dar kill aos threads e processos que ja nao sao precisos, parar de receber tarefas e depois esperar que as tarefas do vcpu acabem.
    write_file("\n%s:Signal SIGINT received ... waiting for last tasks to close simulator.\n"); 
    end = 1;
    
    while ((wait(&status1)) > 0);
    unlink(PIPE_NAME);
    
    write_file("%s:Tasks that were not completed: \n");

    for(int i = 0; i < my_sharedm->length ; i++){
        sprintf(phrase, "Task ID: %d, Task Priority: %d\n", num_tasks[i].id , num_tasks[i].priority);
        write_file(phrase);
    }

    write_file("%s:Simulator closed.\n");

    pthread_mutex_destroy(&mutex);    //falta por todos os que usamos aqui.
    free(num_tasks);
	
    exit(0);
}

void stats(){

	//meter tudo na shared memory??? nao sei muito bem como ter acesso aos valores que precisamos aqui.
    char phrase[64];
    
	write_file("\n%s:Signal SIGTSTP received ... showing stats! \n");
	
    sprintf(phrase, "Tasks executed: %d \n" , tasks_executed);
    write_file(phrase);

    char string[64];
    double average_time = 0;

    average_time = tempo_total / tasks_executed;
    sprintf(string, "Average response time: %lf \n", average_time);
    write_file(string);

    //FALTA COISAS AQUI
    sprintf(string, "Number of tasks that have not been executed: %d \n" , my_sharedm->length);
    write_file(string);

}



void read_pipe(){
	char aux[128];
	
	while(1){
	//read first 4 bytes and check if it's EXIT
	sem_wait(sem_pipe);
    	if(read(fd, &text_pipe, sizeof(text_pipe)) == -1){
    		write_file("Error reading pipe.\n");
   		}
		
		for(int i = 0 ; i < 4 ; i++){
			aux[i] = text_pipe[i];
		}
		aux[4] = '\0';
		
		//verificamos se a palavra que recebemos e EXIT e, se nao tem nada a seguir dela.
    	if(strcmp(aux , "EXIT") == 0 && text_pipe[4] == '\n'){
        	finish();
		}
		
		if(text_pipe[5] == '\n'){
			aux[4] = text_pipe[4];
		}
		aux[5] = '\0';
		//verificar se a palavra que recebemos e EXIT
		if(strcmp(aux , "STATS") == 0){
			stats();
		}
		
    	else {
    	//It's neither of them let's check if it was a task that was sent
    		//not a task, we complain and then keep waitting for a task.
        	if(sscanf(text_pipe,"%d %d %lf" , &t2.id , &t2.num_instr , &t2.temp_max) != 3)
            	write_file("Wrong values received from pipe! ... \n");
            //it's a task , start task values and leave the function.
        	else{
        		t2.priority = -1;
        		t2.time = clock();
        		sem_post(sem_pipe);
        		break;
        	}
        	
    	}
    sem_post(sem_pipe);
    }
}
    
    
void init(int n_servers){

    sem_unlink("SEMAPHORE");
    semaphore = sem_open("SEMAPHORE" , O_CREAT|O_EXCL , 0700 , 1);
    sem_unlink("PIPE_SEM");
    sem_pipe = sem_open("PIPE_SEM", O_CREAT|O_EXCL , 0700 , 1);
    sem_unlink("MQ_SEM");
    sem_mq = sem_open("MQ_SEM" , O_CREAT|O_EXCL , 0700 , 1);
    sem_unlink("MQ_MM");
    sem_mm = sem_open("MQ_MM" , O_CREAT|O_EXCL , 0700 , n_servers - 1);
    
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
    
    if(atoi(queue_pos) == 0){
        write_file("%s:Error converting to int!\n");
        exit(1);    
    }
    
    if(atoi(max_wait) == 0){
    	write_file("%s:Error converting to int!\n");
        exit(1);
    }

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

void * thread_dispatcher() {

pthread_exit(NULL);

}


void * thread_scheduler(void *x){
	
	if ((fd = open(PIPE_NAME, O_RDWR)) < 0) {
        write_file("Error oppening pipe for reading.\n");
        exit(0);
    }
	

    while(end == 0){
    
		read_pipe();

        printf("task %d just arrived\n" , t2.id);
        add_task(t2);

    }
    
	pthread_exit(NULL);
}


void edge_server(int edge_id) {

	ignore_signal();
    //pthread_cond_signal(&maintenance); //ver isto
    int received_msg = 0;
    int flag_change = 0;

    mq_msg msg1;
    msg1.mtype = 3;
    msg1.number = 0;
	
    //informar ao maintenance managers que esta ativo.
 
    pthread_t thread_vcpu[2];
    
    pthread_create(&thread_vcpu[0], NULL, vcpu_min , (void *) &edge_id); 
    
    int current_flag = my_sharedm[edge_id].alt_receber_tarefa;
    
    while(1){ 

        sem_wait(sem_mq);

        if(msgrcv(mq, &msg1, sizeof(msg1), 1, IPC_NOWAIT) != -1){

			sem_post(sem_mq);
            printf("%s was choosen for maintenance!...\n", my_sharedm[edge_id].name);
            received_msg++; 
            msg1.mtype = 3;

            //falta terminar tarefas , ainda nao sei como fazer isso.
            msgsnd(mq, &msg1, sizeof(msg1), 0);
            
            my_sharedm[edge_id].nivel_perf = -1;

            /*if(current_flag == 1){
                for(int i = 0 ; i < 2 ; i++)
                    pthread_join(thread_vcpu[i],NULL);
            }
            else{
                pthread_join(thread_vcpu[0],NULL);
            }*/ //ESTA CORRETO MAS SE ESTIVER ISTO AQUI FICA PRESO PORQUE NAO TERMINAMOS OS VCPUS.

            msgrcv(mq, &msg1, sizeof(msg1), 2, 0);
            printf("%s just finished maintenance! ... \n" , my_sharedm[edge_id].name);

		if(current_flag != my_sharedm[edge_id].nivel_perf && my_sharedm[edge_id].nivel_perf != -1){
			current_flag = my_sharedm[edge_id].nivel_perf;
			flag_change++;
		}
		else            
            my_sharedm[edge_id].nivel_perf = current_flag;

        }
        
        if(received_msg == 0)
            sem_post(sem_mq);
        else
            received_msg = 0;
           
		if(current_flag != my_sharedm[edge_id].nivel_perf){
			current_flag = my_sharedm[edge_id].nivel_perf;
			flag_change++;
		} //NAO TESTEI.
    	
    	if(current_flag == 1 && flag_change == 1){
            pthread_create(&thread_vcpu[1], NULL, vcpu_max, (void *) &edge_id);
            current_flag = my_sharedm[edge_id].nivel_perf; 
            flag_change = 0; 
       	}else if(current_flag == 0 && flag_change == 1){
        	 pthread_join(thread_vcpu[1],NULL);
        	 current_flag = my_sharedm[edge_id].nivel_perf;
        	 flag_change = 0;	 
        }	 
    }

}


void task_manager() { //TASK MANAGER
	
    int x = 0;
    int id_scheduler = 1;
    pthread_t thread_sched;
	
	ignore_signal();
    num_tasks = malloc(my_sharedm->queuepos * sizeof(task));
    
    pthread_create(&thread_sched, NULL, thread_scheduler, (void *) &id_scheduler);
	

    for (int i = 0 ; i < my_sharedm->edgeservers ; i++) {

        if(fork() == 0) {
            write_file("%s:Process edge_server created.\n");

            edge_server(i);
            exit(0);
        } 
    
    }

    while (wait(&x) > 0);
    pthread_join(thread_sched,NULL);
}


void monitor() {
	
	ignore_signal();
	int change_level = 0;
	
    while(end == 0){
        if(my_sharedm->queuepos * 0.8 <= my_sharedm->length && change_level == 0){
            /*if(tempmin > max_wait) { //falta o tempmin...
                my_sharedm->nivel_perf = 1;
            }*/
            for(int i = 0; i < edge_servers ; i++)
                my_sharedm[i].nivel_perf = 1;
            change_level++;
            printf("FOI TROCADO O NIVEL !!!!!!! \n");
        }

        if(my_sharedm->nivel_perf == 1){
            if(my_sharedm->queuepos * 0.2 >= my_sharedm->length){
            	change_level = 0;
                for(int i = 0; i < edge_servers ; i++)
                    my_sharedm[i].nivel_perf = 0;
            }
        }
    }
}

void *maintenance_thread(){

mq_msg msg;


while(1){

	sem_wait(sem_mm);
	
	int sleep_time = (rand() % (5 - 1 + 1)) + 1;

    msg.mtype = 1;
    msg.number = 1;

    msgsnd(mq, &msg, sizeof(msg), 0);
    msgrcv(mq, &msg, sizeof(msg), 3, 0);

    sleep(sleep_time);
    printf("Server sleeping for %d seconds...\n",sleep_time);

    msg.mtype = 2;
    msgsnd(mq, &msg, sizeof(msg) , 0);
    sleep(sleep_time);
    
    sem_post(sem_mm);
}

pthread_exit(NULL);
}



void maintenance_manager(int num_servers) {

ignore_signal();

//pthread_cond_wait(&maintenance , &mq_mutex); //ver isto
pthread_t thread_maintenance[num_servers - 1];
    

for(int i = 0 ; i < num_servers - 1 ; i++ ){
	pthread_create(&thread_maintenance[i], NULL, maintenance_thread , NULL);
}

for(int i = 0 ; i < num_servers - 1 ; i++)
 	pthread_join(thread_maintenance[i],NULL);
}


int main() {

    int status = 0;
    char capac1[20];
	char capac2[20];
    char line[64];
    char *tokens;   

    signal(SIGINT, finish);
    signal(SIGTSTP, stats);
	
    log_file  = fopen("log_file.txt", "w");
    fclose(log_file);
    
    edge_servers = read_file();
    assert((mq = msgget(IPC_PRIVATE, IPC_CREAT|0700)) != -1 );
    
    init(edge_servers);

    int shm_users = 3 + edge_servers;

    
    // Criar o segmento de memória partilhada
    if ((shmid = shmget(IPC_PRIVATE, shm_users * sizeof(shared_mem), IPC_CREAT | 0777)) < 0){
        write_file("%s:Error on shmget function!\n");
        exit(-1);
    }

    if ((my_sharedm = shmat(shmid, NULL, 0)) == (shared_mem *) -1) {
        write_file("%s:Error on shmat function!\n");
        exit(-1);
    }
    
    for(int index = 0 ; index < edge_servers ; index++){

        if (fscanf(config_file , "%s" , line) != 1) {
            write_file("%s:Error reading file!\n");
            exit(-1);
        }

        tokens = strtok(line, ",");
        strcpy(my_sharedm[index].name, tokens);

        tokens = strtok(NULL, ",");
        strcpy(capac1, tokens);

        tokens = strtok(NULL, ",");
        strcpy(capac2, tokens);
        
        
        my_sharedm[index].capac_proc1 = atoi(capac1);
        my_sharedm[index].capac_proc2 = atoi(capac2);
        

        if(my_sharedm[index].capac_proc1 == 0){
            write_file("%s:Error converting to int!\n");
            exit(-1);
        }
        if(my_sharedm[index].capac_proc2 == 0){
            write_file("%s:Error converting to int!\n");
            exit(-1);
        }

        my_sharedm[index].queuepos = atoi(queue_pos);
        my_sharedm[index].maxwait = atoi(max_wait);
        my_sharedm[index].edgeservers = edge_servers;
        
    }

    if (mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0 && (errno!= EEXIST)) {
        write_file("Error creating pipe.\n");
        exit(-1);
    }


    t = time(NULL);
    tm = localtime(&t);
    assert(strftime(s, sizeof(s), "%c", tm));
	
	if(fork() == 0) {
        write_file("%s:Process Maintenance Manager created.\n");

        maintenance_manager(edge_servers);
        exit(0);
    }	

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


    while ((wait(&status)) > 0);

    return 0;
}
