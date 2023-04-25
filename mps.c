#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <getopt.h>
#include <unistd.h>

// Global variables
int num_processors = 2;
char scheduling_approach = 'S';
char queue_selection_method[] = "RM";
char scheduling_algorithm[] = "FCFS";
int time_quantum = 20;



struct timeval starttime, currenttime, endtime;

// struct for a process
typedef struct process {
    int pid;                // process ID
    long burst_length;       // length of the CPU burst
    long arrival_time;       // arrival time of the process
    long remaining_time;     // remaining time of the process (used for RR algorithm)
    long finish_time;        // finish time of the process
    long turnaround_time;    // turnaround time of the process
    long waiting_time;       // waiting time of the process
    int processor_id;       // ID of the processor in which the process has executed
} process_t;

// struct for a node in the ready queue
typedef struct node {
    process_t *process; // pointer to a process
    struct node *next;  // pointer to the next node
} node_t;

// struct for the ready queue
typedef struct queue {
    node_t *head;       // pointer to the head node
    node_t *tail;       // pointer to the tail node
    pthread_mutex_t lock; // mutex lock to protect the queue
} queue_t;

queue_t* single_queue;
queue_t** queue_array;

// Function to create a new node
node_t* new_node(process_t *process) {
    node_t *node = (node_t*) malloc(sizeof(node_t));
    node->process = process;
    node->next = NULL;
    return node;
}

int  load_balance_queue_find( queue_t** queues ) {
    int index;
    long min = 1000000;
    for ( int i = 0; i < num_processors; i++ ) {
        node_t* traverser = queues[i]->head;
        long totalBurstLength = 0;
        while ( traverser != NULL ) {
            totalBurstLength += traverser->process->burst_length;
            traverser = traverser->next;
        }
        if ( traverser == NULL ){
            if ( totalBurstLength < min ) {
                min = totalBurstLength;
                index = i;
            }
        }
    }
    return index;
}

// Function to add a process to the ready queue
void add_to_queue(queue_t *queue, process_t *process) {
    node_t *node = new_node(process);
    pthread_mutex_lock(&(queue->lock));
    if (queue->head == NULL) {
        queue->head = node;
        queue->tail = node;
    } else {
        if (queue->tail->process->pid == -1) {
            queue->tail->process = node->process;
            process_t* dummyBurst = (process_t *) malloc(sizeof(process_t));
            dummyBurst->pid = -1;
            queue->tail->next = new_node(dummyBurst);
            queue->tail = queue->tail->next;
        } else {
            queue->tail->next = node;
            queue->tail = node;
        }
    }
    pthread_mutex_unlock(&(queue->lock));
}

// Function to remove a process from the ready queue
process_t* remove_from_queue(queue_t *queue) {
    pthread_mutex_lock(&(queue->lock));
    if (queue->head == NULL) {
        pthread_mutex_unlock(&(queue->lock));
        return NULL;
    }
    node_t *node = queue->head;
    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    pthread_mutex_unlock(&(queue->lock));
    process_t *process = node->process;
    free(node);
    return process;
}

typedef struct {
    int threadNo;
} ThreadArgs;

void execute_process(queue_t* queue, int threadNo) {
    printf(" thread %d try to execute process\n", threadNo);

    process_t* curr_proccess = remove_from_queue(queue);

            
    // Wait 1 sec if there is no process
    if (curr_proccess == NULL) {

        usleep(1);
    } else {
        
        // If marked node
        if(curr_proccess->pid == -1){
            printf("Exit thread %d\n", threadNo);
            add_to_queue(queue, curr_proccess);

            pthread_exit(NULL);
        } else {
            // burst the process
            if (scheduling_algorithm != "RR") { // SJF, FCFS //TODO: burda bir problem vardi check ederken, character by character check edilmeli
                usleep(curr_proccess->burst_length); 

                gettimeofday(&endtime, NULL);
                // Calculate elapsed time
                long int finish_time = (endtime.tv_sec - starttime.tv_sec) * 1000000L + (endtime.tv_usec - starttime.tv_usec);
                curr_proccess->finish_time = finish_time;
                curr_proccess->turnaround_time = curr_proccess->finish_time - curr_proccess->arrival_time;
                curr_proccess->waiting_time = curr_proccess->turnaround_time - curr_proccess->burst_length;
                
                printf("%ld - processing pid:%d burst: %ld threadNo: %d\n", finish_time, curr_proccess->pid, curr_proccess->burst_length, threadNo);
            } else { // RR
                if (curr_proccess->remaining_time > time_quantum) {
                    usleep(time_quantum);
                    // update remaining time
                    curr_proccess->remaining_time = curr_proccess->remaining_time - time_quantum;
                    // put back the end of the queue
                    add_to_queue(queue, curr_proccess);
                } else {
                    usleep(curr_proccess->remaining_time);

                    gettimeofday(&endtime, NULL);
                    // Calculate elapsed time
                    long int finish_time = (endtime.tv_sec - starttime.tv_sec) * 1000000L + (endtime.tv_usec - starttime.tv_usec);
                    curr_proccess->finish_time = finish_time;
                    curr_proccess->turnaround_time = curr_proccess->finish_time - curr_proccess->arrival_time;
                    curr_proccess->waiting_time = curr_proccess->turnaround_time - curr_proccess->burst_length;
                    printf("%ld - processing pid:%d burst: %ld threadNo: %d\n", finish_time, curr_proccess->pid, curr_proccess->burst_length, threadNo);
                }
            }
            
        }
    }
}

void* thread_function(void* args) {
    ThreadArgs* thread_args = (ThreadArgs*)args;

    // the function that will be run by each thread
    printf("Hello from thread %d\n", thread_args->threadNo);

    while(1) {
        // queuedan process al varsa yoksa bekle
        if (scheduling_approach == 'M') {   // Multi queue
            execute_process(queue_array[thread_args->threadNo], thread_args->threadNo);
        } else {    // Single queue
            execute_process(single_queue, thread_args->threadNo);
        }
    }
}

int main(int argc, char* argv[]) {

    // Get arguments
    char* input_file = "in.txt";
    int output_mode = 1;
    char* output_file = "out.txt";
    int random_mode = 0;
    int T = 0, T1 = 0, T2 = 0, L = 0, L1 = 0, L2 = 0;

    int opt;
    while ((opt = getopt(argc, argv, "n:a:s:i:m:o:r:")) != -1) {
        switch (opt) {
            case 'n':
                num_processors = atoi(optarg);
                break;
            case 'a':
                scheduling_approach = optarg[0];
                if (scheduling_approach == 'M') {
                    if (num_processors == 1) {
                        printf("Error: Number of processors must be greater than 1 for multi-queue approach.\n");
                        exit(1);
                    }
                    if (optarg[1] == 'L') {
                        queue_selection_method[0] = "L";
                    }
                } else if (scheduling_approach != 'S') {
                    printf("Error: Invalid scheduling approach.\n");
                    exit(1);
                }
                break;
            case 's':
                // TODO: scheduling_algorithm = optarg[0];
                if (scheduling_algorithm == 'R') {
                    time_quantum = atoi(optarg+1);
                } else if (scheduling_algorithm != 'F' && scheduling_algorithm != 'S') {
                    printf("Error: Invalid scheduling algorithm.\n");
                    exit(1);
                }
                break;
            case 'i':
                input_file = optarg;
                break;
            case 'm':
                output_mode = atoi(optarg);
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'r':
                random_mode = 1;
                sscanf(optarg, "%d %d %d %d %d %d", &T, &T1, &T2, &L, &L1, &L2);
                break;
            default:
                printf("Usage: mps [-n N] [-a SAP QS] [-s ALG Q] [-i INFILE] [-m OUTMODE] [-o OUTFILE] [-r T T1 T2 L L1 L2]\n");
                exit(1);
        }
    }

    // start the timer
    time_t t;
    srand((unsigned) time(&t));

    gettimeofday(&starttime, NULL);

    // Create processor threads
    pthread_t threads[num_processors];
    int rc;

    for (int i = 0; i < num_processors; i++) {
        ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
        args->threadNo = i;
        printf("Creating thread %d\n", i);
        rc = pthread_create(&threads[i], NULL, thread_function, args);

        if (rc) {
            printf("Error creating thread %d\n", i);
            return -1;
        }
    }

    // Create ready queues acc. to scheduling approach: multiqueue or singlequeue
    if (scheduling_approach == 'S') {
        // Create ready queue
        single_queue =  (queue_t*) malloc(sizeof(queue_t));
    } else {
        queue_array = (queue_t**) malloc(num_processors*sizeof(queue_t*));

        for (int i = 0; i < num_processors; i++)
        {
            queue_array[i] = (queue_t*) malloc(sizeof(queue_t));
        }
    }

    FILE *fp;
    char line[100];

    fp = fopen(input_file, "r");
    if (fp == NULL) {
        printf("Failed to open file\n");
    }

    int pid = 1; //first process id is 1.
    while (fgets(line, 100, fp)) {
        printf("line: %s", line);

        if ( line[0] == 'P' && line[1] == 'L') {
            int burst_length;
            if(sscanf(line,"PL %d", &burst_length) != 1) {
                printf("Error parsing burst length in line: %s", line);
                return -1;
            }
            process_t* newBurst = (process_t *) malloc(sizeof(process_t));
            newBurst->pid = pid;
            pid++;
            newBurst->burst_length = burst_length;

            gettimeofday(&currenttime, NULL);
            long elapsed_sec = starttime.tv_sec - currenttime.tv_sec;
            long elapsed_msec = starttime.tv_usec - currenttime.tv_usec;
            long arrival_time = elapsed_sec * 1000000L + elapsed_msec;
            newBurst->arrival_time = arrival_time;


            newBurst->remaining_time = burst_length;
            newBurst->finish_time = 0;  // will be updated later
            newBurst->turnaround_time = 0;  // will be updated later
            newBurst->waiting_time = 0;  // will be updated later
            newBurst->processor_id = -1;  // not assigned to any processor yet
            //printf("process pid: %d added to queue\n", newBurst->pid);

            if( scheduling_approach == 'S' ) {
                printf("process pid: %d added to queue\n", newBurst->pid);
                add_to_queue(single_queue, newBurst);
            }
            else if ( scheduling_approach == 'M') {
                //ToDo: LM OR RM implementation
                int queue_index = 0;
                if (queue_selection_method == 'R') {//Round Robin
                    add_to_queue(queue_array[queue_index], newBurst);
                    queue_index = ( queue_index + 1 ) % num_processors;
                }
                else if (queue_selection_method == 'L') {//Load Balancing
                    int q_index = load_balance_queue_find(queue_array);
                    add_to_queue(queue_array[q_index], newBurst);
                }
            }
        }
        else if ( line[0] == 'I' && line[1] == 'A' && line[2] == 'T' ) {
            int interarrival_time;
            if (sscanf(line, "IAT %d", &interarrival_time) != 1) {
                printf("Error parsing interarrival time in line: %s", line);
                return -1;
            }
            usleep(interarrival_time );
        } else {
            // Invalid line
            printf("Error: Invalid line in input file: %s", line);
            return -1;
        }
    }

    if (scheduling_approach == 'S') {
        process_t* dummyBurst = (process_t *) malloc(sizeof(process_t));
        dummyBurst->pid = -1;
        add_to_queue(single_queue, dummyBurst);
        // TODO: queue sonuna bi marker koy o geldiğinde diğer threadleri bekle
    }
    else if (scheduling_approach == 'M') {
        // TODO: queue sonuna bi marker koy o geldiğinde diğer threadleri bekle
    }



    // Read file

    fclose(fp);

    // wait for all threads to finish
    for (int i = 0; i < num_processors; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Threads finished\n");

    

    return 0;
}