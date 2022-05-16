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
    int nivel_perf;
    char name[64];
    int time_min;
    int capac_proc1;
    int capac_proc2;
    int length;
    int tasks_executed;
    double ready_time_min;
    double ready_time_max;
    double total_time;
    double sleep_min;
    double sleep_max;
    int maintenance_done;
    int wait;
    int busy;
    int id_monitor;
	int id_task_manager;
	int id_maintenance_manager;
    int id_edge;
    int free;
    int finish;
    pthread_cond_t ready;
    pthread_cond_t shm_finish_var;
    pthread_mutex_t shm_finish_mutex;
    pthread_mutex_t mm_mutex;
    pthread_mutex_t shm_mutex;
} shared_mem;

pthread_mutex_t tasks = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t min_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t max_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t edge_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *semaphore , *sem_pipe , *sem_mq , *sem_mm , *sem_array;
pthread_cond_t  cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t  edge_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t  min_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t  max_var = PTHREAD_COND_INITIALIZER;


shared_mem *my_sharedm;

time_t t;
struct tm *tm;

char s[64];
char queue_pos[20];
char max_wait[20];
char text_pipe[128];
int edge_servers = 0;
int fd;
int mq;
int shmid;
int end = 0;
int **un_pipe;
int finish_max = 0;
int finish_min = 0;
int ready_min = 0;
int ready_max = 0;
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
	
	sem_wait(sem_array);
    int task_priority = num_tasks[indice].priority;

    for (int i = 0; i < my_sharedm->length-1; i++)
    {   
        if(task_priority < num_tasks[i+1].priority)
            num_tasks[i].priority--;

        num_tasks[i] = num_tasks[i+1]; // assign arr[i+1] to arr[i]
    }
    my_sharedm->length--;
    sem_post(sem_array);
}

void reavaliar_prioridade(){ //TASK MANAGER
 
    //printf("reavaliating priority\n");
    int priority = 1;
    
    //printf("before reavaliating\n");
    //for(int i = 0 ; i < my_sharedm->length ; i++){
    	//printf("MAX TIME %f , PRIORITY %d ||",num_tasks[i].temp_max , num_tasks[i].priority);
    //}
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
        num_tasks[i].time = ((double) (clock()) / CLOCKS_PER_SEC) - num_tasks[i].time;

        num_tasks[i].priority = priority;

        if(num_tasks[i].time > num_tasks[i].temp_max){
            write_file("%s:Max time has passed! Removing task...\n");
            delete_task(i);
        }
    }
	
	sem_post(sem_array);
    //DEBUG FOR REAVALIATION OF TASKS
	//printf("After reavaliating\n");
	/*for(int i = 0 ; i < my_sharedm->length ; i++){
    	printf("TASK ID %f , PRIORITY %d ||",num_tasks[i].temp_max , num_tasks[i].priority);
    }
	printf("\n");*/
}



void add_task(task added_task){ //TASK MANAGER

    int full = 0;

    //printf("Adding task %d\n" , added_task.id);

    if(my_sharedm->length == my_sharedm->queuepos){
        write_file("%s:Queue full! Removing task...\n");
        full = -1;
    }

    if(full == 0){
		sem_wait(sem_array);
        num_tasks[my_sharedm->length] = added_task;
    	//printf("%d\n" , num_tasks[length].id); 
    	//printf("%d\n" , length);

        my_sharedm->length++;
        reavaliar_prioridade();
        sem_post(sem_array);
    }

}


//TEMOS DE SEPARAR EM VCPU_MIN E VCPU_MAX PARA SER MAIS FACIL
void *vcpu_min(void *u){
    
    //DEVEMOS TER O TEMPO EM QUE VAI ESTAR LIVRE, OU SEJA, TEMPO DE QUANDO RECEBES A TAREFA + TEMPO DE PROCESSAMENTO DO VCPU
    int id = *((int*)u);
    my_sharedm->free++;
   

    while(finish_min == 0){
    	
    	if(my_sharedm[id].wait == 1){
    		pthread_mutex_lock(&edge_mutex);
    		pthread_cond_signal(&edge_var);
    		pthread_mutex_unlock(&edge_mutex);

    		pthread_mutex_lock(&edge_mutex);
    		pthread_cond_wait(&edge_var , &edge_mutex);
    		pthread_mutex_unlock(&edge_mutex);
    	}
 			
 		
 		
    	pthread_mutex_lock(&min_mutex);
    	pthread_cond_signal(&min_var);
    	ready_min--;
        pthread_cond_wait(&min_var , &min_mutex);
        pthread_mutex_unlock(&min_mutex);
        ready_min++;
       		
        printf("%s : task was choosen !!!\n" , my_sharedm[id].name);
		
		my_sharedm[id].ready_time_min = my_sharedm[id].sleep_min + ((double) (clock()) / CLOCKS_PER_SEC);
		
				
		while(((double) (clock()) / CLOCKS_PER_SEC) < my_sharedm[id].ready_time_min){
			
		}
		
        write_file("%s:Task finished successfully.\n");
        my_sharedm[id].tasks_executed++;
        my_sharedm->total_time += my_sharedm[id].sleep_min;
        
        my_sharedm[id].busy--;
    	my_sharedm->free++;
    	
    	if(my_sharedm->finish == 1){
    	
    	    pthread_mutex_lock(&edge_mutex);
    		pthread_cond_signal(&edge_var);
    		pthread_mutex_unlock(&edge_mutex);
    		
    		pthread_exit(NULL);
    	
    	}
    	
    	pthread_mutex_lock(&my_sharedm->mm_mutex);
        pthread_cond_signal(&my_sharedm->ready);
        pthread_mutex_unlock(&my_sharedm->mm_mutex);
        
    }
    
    pthread_exit(NULL);
}


void * vcpu_max(void *m){

    //printf("VCPU_MAX was activated !!! \n");
 	int id = *((int*)m);
 	my_sharedm->free++;
	
    while(finish_max == 0){
    
    	if(my_sharedm[id].wait == 1){
    	
    		pthread_mutex_lock(&edge_mutex);
    		pthread_cond_signal(&edge_var);
    		pthread_mutex_unlock(&edge_mutex);

    		pthread_mutex_lock(&edge_mutex);
    		pthread_cond_wait(&edge_var , &edge_mutex);
    		pthread_mutex_unlock(&edge_mutex);
    		
    	}
 			
 		
    	pthread_mutex_lock(&max_mutex);
    	pthread_cond_signal(&max_var);
    	ready_max--;
        pthread_cond_wait(&max_var , &max_mutex);
        pthread_mutex_unlock(&max_mutex);

        ready_max++;
       		
        printf("%s : task was choosen !!!\n" , my_sharedm[id].name);
		
		my_sharedm[id].ready_time_max = my_sharedm[id].sleep_max + ((double) (clock()) / CLOCKS_PER_SEC);
	
				
		while(((double) (clock()) / CLOCKS_PER_SEC) < my_sharedm[id].ready_time_max){
		
		}

        write_file("%s:Task finished successfully.\n");
        my_sharedm[id].tasks_executed++;
        my_sharedm->total_time += my_sharedm[id].sleep_max;
        
        my_sharedm[id].busy--;
    	my_sharedm->free++;
    	
    	if(my_sharedm->finish == 1){
    	
    	    pthread_mutex_lock(&edge_mutex);
    		pthread_cond_signal(&edge_var);
    		pthread_mutex_unlock(&edge_mutex);
    		
    		pthread_exit(NULL);
    	
    	}
    	    	
    	pthread_mutex_lock(&my_sharedm->mm_mutex);
        pthread_cond_signal(&my_sharedm->ready);
        pthread_mutex_unlock(&my_sharedm->mm_mutex);
    }
    pthread_exit(NULL);
    
    return NULL;
}




void stats(){

	int total_tasks = 0;
    char phrase[128];
    

	
    for(int i = 0 ; i < edge_servers ; i++){
        sprintf(phrase,"%s:\n\nTasks executed: %d\nMaintenances done:%d\n\n", my_sharedm[i].name , my_sharedm[i].tasks_executed , my_sharedm[i].maintenance_done);
        total_tasks += my_sharedm[i].tasks_executed;
        write_file(phrase);
    }

    sprintf(phrase, "Tasks executed: %d \n" , total_tasks);
    write_file(phrase);

    double average_time = 0;

    average_time = my_sharedm->total_time / total_tasks;
    sprintf(phrase, "Average response time: %lf \n", average_time);
    write_file(phrase);

    sprintf(phrase, "Number of tasks that have not been executed: %d \n\n" , my_sharedm->length);
    write_file(phrase);

}



void finish(){

	int status1 = 0;
	write_file("\n%s:Signal SIGINT received ... waiting for last tasks to close simulator.\n"); 
	
	
    my_sharedm->finish = 1;

    kill(my_sharedm->id_monitor,SIGKILL);
    kill(my_sharedm->id_maintenance_manager,SIGKILL);
        

	pthread_mutex_lock(&my_sharedm->shm_finish_mutex);
    pthread_cond_signal(&my_sharedm->shm_finish_var); 
    pthread_mutex_unlock(&my_sharedm->shm_finish_mutex);

    end = 1;
    
    while ((wait(&status1)) > 0);
    
    printf("QUASE\n");

    stats();

    write_file("%s:Simulator closed.\n");

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&edge_mutex);
    pthread_mutex_destroy(&max_mutex);
    pthread_mutex_destroy(&min_mutex);
    pthread_mutex_destroy(&tasks);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&my_sharedm->mm_mutex);
    pthread_mutex_destroy(&my_sharedm->shm_mutex);
    pthread_cond_destroy(&cond_var);
    pthread_cond_destroy(&max_var);
    pthread_cond_destroy(&min_var);
    pthread_cond_destroy(&edge_var);
    pthread_cond_destroy(&my_sharedm->ready);
    msgctl(mq, IPC_RMID, 0);
    sem_close(semaphore);
    sem_close(sem_pipe);
    sem_close(sem_mq);
    sem_close(sem_mm);
    sem_close(sem_array);
    shmdt(my_sharedm);
	shmctl(shmid, IPC_RMID, NULL);
    free(num_tasks);
    unlink(PIPE_NAME);
	
    exit(0);
}



void stats_signal(){

	write_file("\n%s:Signal SIGTSTP received ... showing stats! \n");
	
	stats();

}



void read_pipe(){
	char aux[128];
	
	while(1){

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
        	kill(getppid() , SIGINT);
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
        		t2.time = ((double) (clock()) / CLOCKS_PER_SEC);
        		sem_post(sem_pipe);
        		break;
        	}
        	
    	}
    sem_post(sem_pipe);
    }
}
    
    
void init(int n_servers){
	
	pthread_mutexattr_t maintenance_mutex;
    pthread_mutexattr_t vcpu_min_mutex;
	pthread_mutexattr_t shm;
    pthread_mutexattr_t finish_mutex;
	pthread_condattr_t maintenance_var;
    pthread_condattr_t finish_var;
    pthread_condattr_t min_var;

    sem_unlink("SEMAPHORE");
    semaphore = sem_open("SEMAPHORE" , O_CREAT|O_EXCL , 0700 , 1);

    sem_unlink("PIPE_SEM");
    sem_pipe = sem_open("PIPE_SEM", O_CREAT|O_EXCL , 0700 , 1);
    
    sem_unlink("MQ_SEM");
    sem_mq = sem_open("MQ_SEM" , O_CREAT|O_EXCL , 0700 , 1);
    
    sem_unlink("MQ_MM");
    sem_mm = sem_open("MQ_MM" , O_CREAT|O_EXCL , 0700 , n_servers - 1);
    
    sem_unlink("ARRAY_SEM");
    sem_array = sem_open("ARRAY_SEM" , O_CREAT|O_EXCL , 0700 , 1);

    /* Initialize attribute of mutex. */
    pthread_mutexattr_init(&maintenance_mutex);
    pthread_mutexattr_setpshared(&maintenance_mutex, PTHREAD_PROCESS_SHARED);

    pthread_mutexattr_init(&vcpu_min_mutex);
    pthread_mutexattr_setpshared(&vcpu_min_mutex, PTHREAD_PROCESS_SHARED);

    pthread_mutexattr_init(&finish_mutex);
    pthread_mutexattr_setpshared(&finish_mutex, PTHREAD_PROCESS_SHARED);
    
    pthread_mutexattr_init(&shm);
    pthread_mutexattr_setpshared(&shm, PTHREAD_PROCESS_SHARED);

    /* Initialize attribute of condition variable. */
    pthread_condattr_init(&maintenance_var);
    pthread_condattr_setpshared(&maintenance_var, PTHREAD_PROCESS_SHARED);

    pthread_condattr_init(&finish_var);
    pthread_condattr_setpshared(&finish_var, PTHREAD_PROCESS_SHARED);

    pthread_condattr_init(&min_var);
    pthread_condattr_setpshared(&min_var, PTHREAD_PROCESS_SHARED);

    /* Initialize mutex. */
    //ver isto
    pthread_mutex_init(&my_sharedm->mm_mutex, &maintenance_mutex);
    pthread_mutex_init(&my_sharedm->shm_finish_mutex, &finish_mutex);
    pthread_mutex_init(&my_sharedm->shm_mutex, &shm);


    /* Initialize condition variables. */
    pthread_cond_init(&my_sharedm->ready, &maintenance_var);
    pthread_cond_init(&my_sharedm->shm_finish_var, &finish_var);     

    
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

int check_time(int index , task t2){
	
	double time = 0;
	int capac = 0;
	int capacmax = 0;
	
	if(my_sharedm[index].capac_proc1 < my_sharedm[index].capac_proc2){
		capac = my_sharedm[index].capac_proc1;
		capacmax = my_sharedm[index].capac_proc2;
	}
	else{
		capac = my_sharedm[index].capac_proc2;
		capacmax = my_sharedm[index].capac_proc1;
	}

	if(my_sharedm[index].nivel_perf == 0){
		
			
		time = ((double)t2.num_instr * 1000) / (capac * 1000000);
		time = time + ((double) (clock()) / CLOCKS_PER_SEC);
	
		if(time < t2.temp_max){
			my_sharedm[index].sleep_min = time;
			return 1;
		}
	}
	
	else{
	
		time = ((double)t2.num_instr * 1000) / (capac * 1000000);
		time = time + ((double) (clock()) / CLOCKS_PER_SEC);
	
		if(time < t2.temp_max){
			my_sharedm[index].sleep_min = time;
			return 1;
		}
		
		time = ((double)t2.num_instr * 1000) / (capacmax * 1000000);
		time = time + ((double) (clock()) / CLOCKS_PER_SEC);	
		
		if(time < t2.temp_max){
			my_sharedm[index].sleep_max = time;
			return 1;
		}
	
	}
	
	return 0;
}


void next_task(){

    int min = 0;
    int task_sent = 0;
    int task_index = 0;
    task t4;
   
    while(1){
    	if(my_sharedm->length > 0){
    		if(my_sharedm->free <= 0){
    			pthread_mutex_lock(&my_sharedm->mm_mutex);
        		pthread_cond_wait(&my_sharedm->ready , &my_sharedm->mm_mutex);
        		pthread_mutex_unlock(&my_sharedm->mm_mutex);
    		}
    		
    		min = num_tasks[0].priority;
    		t4 = num_tasks[0];
        	task_index = 0;


			//meter semaforo aqui para mexer no array.
    		for(int i = 0 ; i < my_sharedm->length ; i++){
        		
        		if(num_tasks[i].priority < min && num_tasks[i].priority > 0){
            		min = num_tasks[i].priority;
            		t4 = num_tasks[i];
            		task_index = i;
        		}
		
    		}
    		
    		for(int i = 0 ; i < edge_servers ; i++){
        		if(my_sharedm[i].nivel_perf == 0 && my_sharedm[i].busy == 0 && task_sent == 0){
        		
        			if(check_time(i , t4)){
        			
        				write(un_pipe[i][1] , &t4 , sizeof(task));
        				
        				if(((double) (clock()) / CLOCKS_PER_SEC) - t4.time < my_sharedm->time_min){
        					my_sharedm->time_min = ((double) (clock()) / CLOCKS_PER_SEC) - t4.time;
        				} 
        				       				
        				my_sharedm->free--;
						my_sharedm[i].busy++;
        				task_sent++;
        				delete_task(task_index);
        			}
        		}
        		
        		else if(my_sharedm[i].nivel_perf == 1 && (my_sharedm[i].busy == 1 || my_sharedm[i].busy == 0) && task_sent == 0){
        		
        			if(check_time(i , t4)){
        			
        				write(un_pipe[i][1] , &t4 , sizeof(task));
        				
        				if(((double) (clock()) / CLOCKS_PER_SEC) - t4.time < my_sharedm->time_min){
        					my_sharedm->time_min = ((double) (clock()) / CLOCKS_PER_SEC) - t4.time;
        				}

        				my_sharedm->free--;
						my_sharedm[i].busy++;
        				task_sent++;
        				delete_task(task_index);
        			}
        		}
    		}
			
			if(task_sent == 0){
				int in_time = 0;
				
				for(int i = 0 ; i < edge_servers ; i ++ ){
					if(check_time(i , t4))
						in_time++;
				}
				
				if(in_time == 0){
					write_file("Task can't be done before time limit... deleting task...!\n");
					delete_task(task_index);
				}
			}
		
			task_sent = 0;
  	}
  }
}

void * thread_dispatcher(void *l) {

while(1){

    next_task();    

} 
pthread_exit(NULL);

}


void * thread_scheduler(void *x){
	
	
	if ((fd = open(PIPE_NAME, O_RDWR)) < 0) {
        write_file("Error oppening pipe for reading.\n");
        exit(0);
    }

    while(1){
    
		read_pipe();
        printf("task %d just arrived\n" , t2.id);
        add_task(t2);

    }
    
	pthread_exit(NULL);
}


void *receive_task(void *k){
	
	int index1 = *((int*)k);
	task task1;
	
	while(1){
		
		read(un_pipe[index1][0] , &task1 , sizeof(task));
		
    	if(my_sharedm[index1].ready_time_min <= ((double) (clock()) / CLOCKS_PER_SEC) && my_sharedm[index1].nivel_perf != -1){
			
			if(ready_min == 0){
			
			pthread_mutex_lock(&min_mutex);
        	pthread_cond_wait(&min_var , &min_mutex); 
        	pthread_mutex_unlock(&min_mutex);
			
			}

			pthread_mutex_lock(&min_mutex);
        	pthread_cond_signal(&min_var); 
        	pthread_mutex_unlock(&min_mutex);
        	
    	}
    	else if(my_sharedm[index1].ready_time_max <= ((double) (clock()) / CLOCKS_PER_SEC) && my_sharedm[index1].nivel_perf == 1){
			if(ready_max == 0){
			
			pthread_mutex_lock(&max_mutex);
        	pthread_cond_wait(&max_var , &min_mutex); 
        	pthread_mutex_unlock(&min_mutex);
			
			}
			pthread_mutex_lock(&max_mutex);
        	pthread_cond_signal(&max_var); 
        	pthread_mutex_unlock(&max_mutex);
	
    	}
	}
}


void edge_server(int edge_id) {

	ignore_signal();
	
	pthread_mutex_lock(&my_sharedm->mm_mutex);
    pthread_cond_signal(&my_sharedm->ready); 
    pthread_mutex_unlock(&my_sharedm->mm_mutex);
    
    my_sharedm[edge_id].wait = 0;
    
    int received_msg = 0;
    int flag_change = 0;
	
    mq_msg msg1;
    msg1.mtype = 3;
    msg1.number = 0;
 
    pthread_t thread_vcpu[2];
    pthread_t thread_task;
    pthread_create(&thread_task , NULL , receive_task , (void *) &edge_id);
    pthread_create(&thread_vcpu[0], NULL, vcpu_min , (void *) &edge_id); 
    int current_flag = my_sharedm[edge_id].nivel_perf;
    
    while(my_sharedm->finish == 0){ 
        sem_wait(sem_mq);
        
        if(msgrcv(mq, &msg1, sizeof(msg1), 1, IPC_NOWAIT) != -1){
        	
			sem_post(sem_mq);
			my_sharedm[edge_id].nivel_perf = -1;
            printf("%s was choosen for maintenance!...\n", my_sharedm[edge_id].name);
            received_msg++; 
            msg1.mtype = 3;
			
		
			if(my_sharedm[edge_id].busy == 2){
			
				my_sharedm[edge_id].wait = 1;
				
				for(int y = 0 ; y < 2 ; y++){
				
					pthread_mutex_lock(&edge_mutex);
					pthread_cond_wait(&edge_var , &edge_mutex);
					pthread_mutex_unlock(&edge_mutex);
					
				}
			}
			else if(my_sharedm[edge_id].busy == 1){
			
				my_sharedm[edge_id].wait = 1;
				
				pthread_mutex_lock(&edge_mutex);
				pthread_cond_wait(&edge_var , &edge_mutex);
				pthread_mutex_unlock(&edge_mutex);
			}
			
			if(my_sharedm->finish == 1){
				return;
			}
			
            msgsnd(mq, &msg1, sizeof(msg1), 0);
            msgrcv(mq, &msg1, sizeof(msg1), 2, 0);
            
            my_sharedm[edge_id].maintenance_done++;
            printf("%s just finished maintenance! ... \n" , my_sharedm[edge_id].name);
            
            pthread_mutex_lock(&edge_mutex);
			pthread_cond_broadcast(&edge_var);
			pthread_mutex_unlock(&edge_mutex);
			
			my_sharedm[edge_id].wait = 0;
			
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
		}
    	
    	if(current_flag == 1 && flag_change == 1){
            pthread_create(&thread_vcpu[1], NULL, vcpu_max, (void *) &edge_id);
            current_flag = my_sharedm[edge_id].nivel_perf; 
            flag_change = 0; 
       	}else if(current_flag == 0 && flag_change == 1){
            finish_max = 1;

            if(my_sharedm[edge_id].ready_time_max <= ((double) (clock()) / CLOCKS_PER_SEC)){
                pthread_join(thread_vcpu[1],NULL);
            }
            else
                pthread_cancel(thread_vcpu[1]);

            finish_max = 0;
        	current_flag = my_sharedm[edge_id].nivel_perf;
        	flag_change = 0;	 
        }	
    }
	
	finish_min = 1;
	finish_max = 1;
    pthread_cancel(thread_task);
    
    if(my_sharedm[edge_id].ready_time_min > ((double) (clock()) / CLOCKS_PER_SEC) && my_sharedm[edge_id].ready_time_max > ((double) (clock()) / CLOCKS_PER_SEC)){
        for(int i = 0 ; i < 2 ; i++){
            pthread_join(thread_vcpu[i], NULL);
        }
    }
    else if(my_sharedm[edge_id].ready_time_min > ((double) (clock()) / CLOCKS_PER_SEC)){
        pthread_cancel(thread_vcpu[1]);        
        pthread_join(thread_vcpu[0] , NULL);

    }
    else if(my_sharedm[edge_id].ready_time_max > ((double) (clock()) / CLOCKS_PER_SEC)){
        pthread_cancel(thread_vcpu[0]);
        
    	pthread_mutex_lock(&my_sharedm->mm_mutex);
        pthread_cond_wait(&my_sharedm->ready , &my_sharedm->mm_mutex);
        pthread_mutex_unlock(&my_sharedm->mm_mutex);
        
        pthread_join(thread_vcpu[1] , NULL);

    }
    else{
        pthread_cancel(thread_vcpu[0]);
        pthread_cancel(thread_vcpu[1]);
    }
}


void task_manager() { //TASK MANAGER
	
    int x = 0;
    int id_scheduler = 1;
    int id_dispatcher = 2;
    pthread_t thread_sched;
    pthread_t thread_disp;
    
    un_pipe = (int**) malloc(edge_servers * sizeof(int*));

	for (int i = 0; i < edge_servers; i++) {
    	un_pipe[i] = (int*) malloc(2*sizeof(int));
	}
	
	for (int i = 0; i < edge_servers; i++) {
    	pipe(un_pipe[i]);
	}
		
	ignore_signal();
    num_tasks = malloc(my_sharedm->queuepos * sizeof(task));
    
    pthread_create(&thread_disp, NULL, thread_dispatcher, (void *) &id_dispatcher);
    pthread_create(&thread_sched, NULL, thread_scheduler, (void *) &id_scheduler);

    for (int i = 0 ; i < my_sharedm->edgeservers ; i++) {

        if(fork() == 0) {
            my_sharedm[i].id_edge = getpid();
            write_file("%s:Process edge_server created.\n");

            edge_server(i);
            exit(0);
        } 
    
    }

    pthread_mutex_lock(&my_sharedm->shm_finish_mutex);
    pthread_cond_wait(&my_sharedm->shm_finish_var , &my_sharedm->shm_finish_mutex); 
    pthread_mutex_unlock(&my_sharedm->shm_finish_mutex);

    pthread_cancel(thread_sched);
    pthread_cancel(thread_disp);

    for(int i = 0 ; i < edge_servers ; i++){
        if(my_sharedm[i].ready_time_min > ((double) (clock()) / CLOCKS_PER_SEC) || my_sharedm[i].ready_time_max > ((double) (clock()) / CLOCKS_PER_SEC)){
        	waitpid(my_sharedm[i].id_edge , &x, 0);
        	//printf("%s just finished\n" , my_sharedm[i].name);
         }
         else{
         	//printf("%s just finished\n" , my_sharedm[i].name);
         	kill(my_sharedm[i].id_edge,SIGKILL);
         }
    }
}

void monitor() {
	
	ignore_signal();
	int change_level = 0;
	
    while(end == 0){
        if(my_sharedm->queuepos * 0.8 <= my_sharedm->length && change_level == 0){
            if(my_sharedm->time_min > my_sharedm->maxwait) {
                my_sharedm->nivel_perf = 1;
            }
            for(int i = 0; i < edge_servers ; i++)
                my_sharedm[i].nivel_perf = 1;
            change_level++;
            //printf("FOI TROCADO O NIVEL !!!!!!! \n");
        }

        if(my_sharedm->nivel_perf == 1){
            if(my_sharedm->queuepos * 0.2 >= my_sharedm->length){
           		//printf("NIVEL FOI TROCADO OUTRAVEZ \n");
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



void maintenance_manager() {

ignore_signal();

pthread_mutex_lock(&my_sharedm->mm_mutex);
pthread_cond_signal(&my_sharedm->ready);
pthread_mutex_unlock(&my_sharedm->mm_mutex); 

for(int i = 0 ; i < edge_servers ; i++){
	pthread_mutex_lock(&my_sharedm->mm_mutex);
	pthread_cond_wait(&my_sharedm->ready , &my_sharedm->mm_mutex);
	pthread_mutex_unlock(&my_sharedm->mm_mutex);
}

pthread_t thread_maintenance[edge_servers - 1];
    

for(int i = 0 ; i < edge_servers - 1 ; i++ ){
	pthread_create(&thread_maintenance[i], NULL, maintenance_thread , NULL);
}

for(int i = 0 ; i < edge_servers - 1 ; i++)
 	pthread_join(thread_maintenance[i],NULL);
}


int main() {

    int status = 0;
    char capac1[20];
	char capac2[20];
    char line[64];
    char *tokens;   

    signal(SIGINT, finish);
    signal(SIGTSTP, stats_signal);
    
    
	
    log_file  = fopen("log_file.txt", "w");
    fclose(log_file);
    
    edge_servers = read_file();
    assert((mq = msgget(IPC_PRIVATE, IPC_CREAT|0700)) != -1 );

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
    
    
    init(edge_servers);
    
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
		my_sharedm->id_maintenance_manager = getpid();
        write_file("%s:Process Maintenance Manager created.\n");

        maintenance_manager();
        exit(0);
    }	

	pthread_mutex_lock(&my_sharedm->mm_mutex);
	pthread_cond_wait(&my_sharedm->ready , &my_sharedm->mm_mutex);
	pthread_mutex_unlock(&my_sharedm->mm_mutex);
	
    if(fork() == 0){
    	my_sharedm->id_monitor = getpid();
        write_file("%s:Process Monitor created.\n");

        monitor();
        exit(0);
    }
    
    if(fork() == 0) {
    	my_sharedm->id_task_manager = getpid();
        write_file("%s:Process Task Manager created.\n");
        
        task_manager();
        exit(0);
    }
   

    while ((wait(&status)) > 0);
    

    return 0;
}
