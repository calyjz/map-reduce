#define _GNU_SOURCE

#include "mapreduce.h"
#include "threadpool.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>


typedef struct KeyValue_t {
    char* key;
    char* value;
    struct KeyValue_t *next;
} KeyValue_t;

typedef struct Partition_t {
    KeyValue_t* head;
    int size;
    pthread_mutex_t lock;
    KeyValue_t *current;
} Partition_t;

typedef struct ThreadArg{
    int value;
    Reducer reducer; 
} ThreadArg_t;

Partition_t *partitions;
unsigned int num_partitions;
/**
 * Run the MapReduce framework
 * Parameters:
 *     file_count   - Number of files (i.e. input splits)
 *     file_names   - Array of filenames
 *     mapper       - Function pointer to the map function
 *     reducer      - Function pointer to the reduce function
 *     num_workers  - Number of threads in the thread pool
 *     num_parts    - Number of partitions to be created
 */
void MR_Run(unsigned int file_count, char *file_names[], Mapper mapper, Reducer reducer, unsigned int num_workers, unsigned int num_parts) {
    //create thread pool
    ThreadPool_t* threadpool = ThreadPool_create(num_workers);

    //initialize partitions
    partitions = (Partition_t*) malloc(sizeof(Partition_t) * num_parts);
    num_partitions = num_parts;

    for (unsigned int i = 0; i < num_parts; i++) {
        partitions[i].head = NULL;
        partitions[i].size = 0;
        pthread_mutex_init(&partitions[i].lock, NULL);
    }

    // submit map jobs to queue
    for (unsigned int i = 0; i < file_count; i++) {
        // get file size
        struct stat sb;
        stat(file_names[i], &sb);

        // submit job
        printf("Submiting job %s of size %ld to queue\n", file_names[i], sb.st_size);
        ThreadPool_add_job(threadpool, (thread_func_t)mapper, file_names[i], sb.st_size);
    }

    //print partitions
    sleep(3);
    fprintf(stderr, "SLEEP: %d\n", __LINE__);
    for (unsigned int i = 0; i < num_parts; i++) {
        KeyValue_t *curr = partitions[i].head;
        printf("Partition %d: {", i);
        while (curr != NULL) {
            printf("%s:%s, ", curr->key, curr->value);
            curr = curr->next;
        }
        printf("}\n");
    }

    // wait for all mapping jobs to finish

    // add all reduce jobs to the queue
    for (int i = 0; i < num_partitions; i++) {
        ThreadArg_t *args = (ThreadArg_t*)malloc(sizeof(ThreadArg_t));
        args->reducer = reducer;
        args->value = i;
        printf("Submiting reduce job of partition %d with size %d to queue\n", i, partitions[i].size);
        ThreadPool_add_job(threadpool, MR_Reduce, args, partitions[i].size);
    }
    // wait for all reduce jobs to finish
    sleep(3);

    // cleanup and exit
    for (unsigned int i = 0; i < num_parts; i++)
    {
        KeyValue_t *temp;
        KeyValue_t *curr = partitions[i].head;

        // free keyvalues
        while (curr != NULL)
        {
            temp = curr->next;
            free(curr->key);
            free(curr->value);
            free(curr);
            curr = temp;
        }
        pthread_mutex_destroy(&partitions[i].lock);
    }
    free(partitions);
    ThreadPool_destroy(threadpool);
}

/**
* Write a specific map output, a <key, value> pair, to a partition
* Parameters:
*     key           - Key of the output
*     value         - Value of the output
*/
void MR_Emit(char* key, char* value) {
    pid_t tid = gettid();
    int index = MR_Partitioner(key, num_partitions);

    Partition_t *partition = &partitions[index];
    pthread_mutex_lock(&partition->lock);
    printf("%d: LOCKED: mapping %s to partition %d\n", tid, key, index);


    //insert Key-Value pair into linked list in alphabetical order
    KeyValue_t *new_pair = (KeyValue_t*) malloc(sizeof(KeyValue_t));
    new_pair->key = strdup(key);
    new_pair->value = strdup(value);
    new_pair->next = NULL;

    if (partition->head == NULL) {
        fprintf(stderr, "Queue is empty: %d\n", __LINE__);
        //empty list
        partition->head = new_pair;

    } else if (strcmp(partition->head->key, new_pair->key) >= 0 ) {
        fprintf(stderr, "Insert at Head: %d\n", __LINE__);
        //insert at head
        new_pair->next = partition->head;
        partition->head = new_pair;

    } else {
        fprintf(stderr, "insert at middle: %d\n", __LINE__);
        //insert in middle of list
        KeyValue_t* prev = NULL;
        KeyValue_t* curr = partition->head;
        assert(curr != NULL);
        while (curr != NULL && strcmp(curr->key, new_pair->key) < 0) {
            prev = curr;
            curr = curr->next;
        }
        prev->next = new_pair;
        new_pair->next = curr;
    }

    partition->size++;
    // KeyValue_t *curr = partition->head;
    assert(partition->size >= 1);
    // for (int i = 0; i < partition->size; i++)
    // {
    //     printf("%s -> ", curr->key);
    //     curr = curr->next;
    // }
    printf("\n%d UNLOCKED\n", tid);

    pthread_mutex_unlock(&partition->lock);
}

/**
* Hash a mapper's output to determine the partition that will hold it
* Parameters:
*     key           - Key of a specific map output
*     num_partitions- Total number of partitions
* Return:
*     unsigned int  - Index of the partition
*/
unsigned int MR_Partitioner(char* key, unsigned int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0') {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % num_partitions;
}


/**
* Run the reducer callback function for each <key, (list of values)>
* retrieved from a partition
* Parameters:
*     threadarg     - Pointer to a hidden args object
*/
void MR_Reduce(void* threadarg) {
    printf("running MR_Reduce\n");
    ThreadArg_t *args = (ThreadArg_t *)threadarg;
    int index = args->value;
    Reducer reducer = args->reducer;

    Partition_t* partition = &partitions[index];
    partition->current = partition->head;

    while (partition->current != NULL) {
        printf("Reducing key %s\n", partition->current->key);
        reducer(partition->current->key, index);
    }

    free(args);
}

/**
* Get the next value of the given key in the partition
* Parameters:
*     key           - Key of the values being reduced
*     partition_idx - Index of the partition containing this key
* Return:
*     char *        - Value of the next <key, value> pair if its key is the current key
*     NULL          - Otherwise
*/
char* MR_GetNext(char* key, unsigned int partition_idx) {
    Partition_t *partition = &partitions[partition_idx];

    // printf("Partition from current: ");
    // KeyValue_t *curr = partition->current;
    // while (curr != NULL){
    //     printf("%s -> ", curr->key);
    //     curr = curr->next;
    // }
    // printf("\n");

    if (partition->current == NULL) {
        return NULL;
    }

    if (strcmp(partition->current->key, key) == 0) {
        char *value = strdup(partition->current->value);
        partition->current = partition->current->next;
        return value;
    }

    return NULL;
}
