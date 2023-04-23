#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <getopt.h>

// Global variables
int num_processors = 2;
char scheduling_approach = 'M';
char queue_selection_method = 'R';
char scheduling_algorithm = 'R';
int time_quantum = 20;

queue_t* single_queue;
queue_t** queue_array;

struct timeval starttime, endtime;

// struct for a process
typedef struct process {
    int pid;                // process ID
    int burst_length;       // length of the CPU burst
    int arrival_time;       // arrival time of the process
    int remaining_time;     // remaining time of the process (used for RR algorithm)
    int finish_time;        // finish time of the process
    int turnaround_time;    // turnaround time of the process
    int waiting_time;       // waiting time of the process
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

// Function to create a new node
node_t* new_node(process_t *process) {
    node_t *node = (node_t*) malloc(sizeof(node_t));
    node->process = process;
    node->next = NULL;
    return node;
}

// Function to add a process to the ready queue
void add_to_queue(queue_t *queue, process_t *process) {
    node_t *node = new_node(process);
    pthread_mutex_lock(&(queue->lock));
    if (queue->head == NULL) {
        queue->head = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
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

void execute_process(queue_t* queue) {
    process_t* curr_proccess = remove_from_queue(queue);
            
    // Wait 1 sec if there is no process
    if (curr_proccess == NULL) {
        usleep(1);
    } else {
        // If marked node
        if(0){ // TODO: change condition as isMarked
            pthread_exit(NULL);
        } else {
            // burst the process
            if (scheduling_algorithm[0] != 'R') { // SJF, FCFS //TODO: burda bir problem vardi check ederken, character by character check edilmeli
                wait(curr_proccess->burst_length); //TODO: Bunlar usleep() mi olmalıydı?

                gettimeofday(&endtime, NULL);
                // Calculate elapsed time
                long int finish_time = (endtime.tv_sec - starttime.tv_sec) * 1000000L + (endtime.tv_usec - starttime.tv_usec);
                curr_proccess->finish_time = finish_time;
                curr_proccess->turnaround_time = curr_proccess->finish_time - curr_proccess->arrival_time;
                curr_proccess->waiting_time = curr_proccess->turnaround_time - curr_proccess->burst_length;
                
            } else { // RR
                if (curr_proccess->remaining_time > time_quantum) {
                    wait(time_quantum);
                    // update remaining time
                    curr_proccess->remaining_time = curr_proccess->remaining_time - time_quantum;
                    // put back the end of the queue
                    add_to_queue(queue, curr_proccess);
                } else {
                    wait(curr_proccess->remaining_time);

                    gettimeofday(&endtime, NULL);
                    // Calculate elapsed time
                    long int finish_time = (endtime.tv_sec - starttime.tv_sec) * 1000000L + (endtime.tv_usec - starttime.tv_usec);
                    curr_proccess->finish_time = finish_time;
                    curr_proccess->turnaround_time = curr_proccess->finish_time - curr_proccess->arrival_time;
                    curr_proccess->waiting_time = curr_proccess->turnaround_time - curr_proccess->burst_length;
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
            execute_process(queue_array[thread_args->threadNo]);
        } else {    // Single queue
            execute_process(single_queue);
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
                        queue_selection_method = 'L';
                    }
                } else if (scheduling_approach != 'S') {
                    printf("Error: Invalid scheduling approach.\n");
                    exit(1);
                }
                break;
            case 's':
                scheduling_algorithm = optarg[0];
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
        queue_t* ready_queue =  (queue_t*) malloc(sizeof(queue_t));
    } else {
        queue_t** ready_queues = (queue_t**) malloc(num_processors*sizeof(queue_t*))

        for (int i = 0; i < num_processors; i++)
        {
            ready_queues[i] = (queue_t*) malloc(sizeof(queue_t));
        }
    }

    FILE *fp;
    char line[100];

    fp = fopen(input_file, "r");
    if (fp == NULL) {
        printf("Failed to open file\n");
    }

    int pid = 1; //first process id is 1.
    int arrival_time = 0; //first process arrives at 0.
    while (fgets(line, 100, fp)) {
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
            newBurst->arrival_time = arrival_time;
            newBurst->remaining_time = burst_length;
            newBurst->finish_time = 0;  // will be updated later
            newBurst->turnaround_time = 0;  // will be updated later
            newBurst->waiting_time = 0;  // will be updated later
            newBurst->processor_id = -1;  // not assigned to any processor yet
            if( scheduling_approach == 'S' ) {
                add_to_queue(ready_queue, newBurst)
            }
            else if ( scheduling_approach == 'M') {
                //ToDo: LM OR RM implementation
            }
        }
        else if ( line[0] == 'I' && line[1] == 'A' && line[2] == 'T' ) {
            int interarrival_time;
            if (sscanf(line, "IAT %d", &interarrival_time) != 1) {
                printf("Error parsing interarrival time in line: %s", line);
                return -1;
            }
            usleep(interarrival_time * 1000);  //todo: emin degilim? convert to microseconds
            arrival_time += interarrival_time; //todo: emin degilim?
        } else {
            // Invalid line
            printf("Error: Invalid line in input file: %s", line);
            return -1;
        }
    }

    if (scheduling_approach == 'S') {
        process_t* dummyBurst = (process_t *) malloc(sizeof(process_t));
        dummyBurst->pid = -1;
        add_to_queue(ready_queue, dummyBurst);
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