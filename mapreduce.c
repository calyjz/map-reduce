#include "mapreduce.h"
#include "threadpool.h"
#include <sys/stat.h>

typedef struct {
    char* key;
    char* value;
    struct KeyValue_t *next;
} KeyValue_t;

typedef struct {
    KeyValue_t* head;
    int size;
    pthread_mutex_t lock;
} Partition_t;

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
    partitions = malloc(sizeof(Partition_t) * num_parts);
    num_partitions = num_parts;

    for (int i = 0; i < num_parts; i++) {
        partitions[i].head = NULL;
        partitions[i].size = 0;
        pthread_mutex_init(&partitions[i].lock, NULL);
    }

    // submit map jobs to queue
    for (int i = 0; i < file_count; i++) {
        // get file size
        struct stat sb;
        stat(file_names[i], &sb);

        // submit job
        ThreadPool_add_job(threadpool, mapper, file_names[i], sb.st_size);
    }

    //wait for all mapping jobs to finish

    //add all reduce jobs to the queue

    //wait for all reduce jobs to finish

    //cleanup and exit
    for (int i = 0; i < num_parts; i++) {
        KeyValue_t *temp;
        KeyValue_t *curr = partitions[i].head;

        while (curr != NULL) {
            temp = curr->next;
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
    int index = MR_Partitioner(key, num_partitions);
    Partition_t* partition = &partitions[index];
    pthread_mutex_lock(&partition->lock);

    //insert Key-Value pair into linked list in alphabetical order
    KeyValue_t *new_pair = malloc(sizeof(KeyValue_t));
    new_pair->key = key;
    new_pair->value = value;
    new_pair->next = NULL;

    if (partition->head == NULL) {
        //empty list
        partition->head = new_pair;
    } else if (strcmp(partition->head->key, new_pair->key) > 0 ) {
        //insert at head
        new_pair->next = partition->head;
        partition->head = new_pair;
    } else {
        //insert in middle of list
        KeyValue_t* prev = NULL;
        KeyValue_t* curr = partition->head;

        while (curr != NULL && strcmp(curr->key, new_pair->key) < 0) {
            prev = curr;
            curr = curr->next;
        }
        prev->next = new_pair;
        new_pair->next = curr;
    }
    partition->size++;
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
void MR_Reduce(void* threadarg);

/**
* Get the next value of the given key in the partition
* Parameters:
*     key           - Key of the values being reduced
*     partition_idx - Index of the partition containing this key
* Return:
*     char *        - Value of the next <key, value> pair if its key is the current key
*     NULL          - Otherwise
*/
char* MR_GetNext(char* key, unsigned int partition_idx);

