#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <getopt.h>

// struct for a process
typedef struct process {
    int pid;                // process ID
    int burst_length;       // length of the CPU burst
    int arrival_time;       // arrival time of the process
    int remaining_time;     // remaining time of the process (used for RR algorithm)
    int finish_time;        // finish time of the process
    int turnaround_time;    // turnaround time of the process
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

void* thread_function(void* args) {

    // the function that will be run by each thread
    printf("Hello from thread %d\n");

    while(1) {
        // TODO: queuedan burst al varsa yoksa bekle
        // finish time ve turnaround timeı updatele 


        usleep(1);
    }

    pthread_exit(NULL);
}


int main(int argc, char* argv[]) {

    // Get arguments
    int num_processors = 2;
    char scheduling_approach = 'M';
    char queue_selection_method = 'R';
    char scheduling_algorithm = 'R';
    int time_quantum = 20;
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

    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    // Create processor threads
    pthread_t threads[num_processors];
    int rc;

    for (int i = 0; i < num_processors; i++) {
        // ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
        //args->threadNo = i;
        //args->size = 1000;
        printf("Creating thread %d\n", i);
        rc = pthread_create(&threads[i], NULL, thread_function, NULL);

        if (rc) {
            printf("Error creating thread %d\n", i);
            return -1;
        }
    }

    // Scheduling approach multiqueue or singlequeue
    if (1) {
        // Create ready queue
        queue_t* ready_queue =  (queue_t*) malloc(sizeof(queue_t));
    } else {
        for (int i = 0; i < num_processors; i++)
        {
            // TODO: queue array
        }
        
    }

    // TODO: queue sonuna bi marker koy o geldiğinde diğer threadleri bekle 
    // TODO: Algoritmalara göre queueya at
    // Scheduling approach multiqueue or singlequeue
    if (1) {
        // TODO: Add burst to single queue
    } else {
        // TODO: RM ya da LB olmasına göre quelara burstleri yerleştir
    }


    // Read file
    FILE *fp;
    char word[100];

    fp = fopen(input_file, "r");
    if (fp == NULL) {
        printf("Failed to open file\n");
    }

    while (fscanf(fp, "%s", word) != EOF) {
        printf("Word: %s\n", word);
    }
    fclose(fp);

    // wait for all threads to finish
    for (int i = 0; i < num_processors; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Threads finished\n");

    

    return 0;
}