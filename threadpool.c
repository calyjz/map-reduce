// You can modify this file however you like.
#include "threadpool.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


/**
* C style constructor for creating a new ThreadPool object
* Parameters:
*     num - Number of threads to create
* Return:
*     ThreadPool_t* - Pointer to the newly created ThreadPool object
*/
ThreadPool_t* ThreadPool_create(unsigned int num) {
    ThreadPool_t *pool = (ThreadPool_t*) malloc(sizeof(ThreadPool_t));

    *pool = (ThreadPool_t) {.num_threads = num, .active = 1, .idlecount = 0};
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->signal, NULL);
    pool->jobs = (ThreadPool_job_queue_t) {.size = 0, .head = NULL};
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num);

    for (unsigned int i = 0; i < num; i++) {
        pthread_create(&pool->threads[i], NULL, Thread_run, pool);
    }

    return pool;
}

/**
* C style destructor to destroy a ThreadPool object
* Parameters:
*     tp - Pointer to the ThreadPool object to be destroyed
*/
void ThreadPool_destroy(ThreadPool_t* tp) {
    tp->active = 0;

    //free threads
    pthread_cond_broadcast(&tp->signal);

    for (int i = 0; i < tp->num_threads; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    free(tp->threads);

    //free jobs in queue
    ThreadPool_job_t *temp = NULL;
    ThreadPool_job_t *curr = tp->jobs.head;
    while (curr != NULL) {
        temp = curr->next;
        free(curr);
        curr = temp;
    }

    //free locks
    pthread_mutex_destroy(&tp->lock);
    pthread_cond_destroy(&tp->signal);

    //free pool
    free(tp);
}

/**
* Add a job to the ThreadPool's job queue
* Parameters:
*     tp   - Pointer to the ThreadPool object
*     func - Pointer to the function that will be called by the serving thread
*     arg  - Arguments for that function
*     job_length - length of job
* Return:
*     true  - On success
*     false - Otherwise
*/
bool ThreadPool_add_job(ThreadPool_t* tp, thread_func_t func, void* arg, int job_length) {
    if (!tp->active) {
        return false;
    }

    //create new job
    ThreadPool_job_t* newjob = (ThreadPool_job_t*)malloc(sizeof(ThreadPool_job_t));
    newjob->func = func;
    newjob->arg = arg;
    newjob->next = NULL;
    newjob->length = job_length;

    pthread_mutex_lock(&tp->lock);

    //add to queue
    if (tp->jobs.head == NULL){
        tp->jobs.head = newjob;
    } 
    else if (newjob->length <= tp->jobs.head->length) {
        newjob->next = tp->jobs.head;
        tp->jobs.head = newjob;
    }
    else {
        ThreadPool_job_t *prev = NULL;
        ThreadPool_job_t *curr = tp->jobs.head;

        while (curr != NULL && curr->length < newjob->length) {
            prev = curr;
            curr = curr->next;
        }

        prev->next = newjob;
        newjob->next = curr;
    }

    tp->jobs.size++;
    assert(tp->jobs.size > 0);

    ThreadPool_job_t *curr = tp->jobs.head;
    printf("Job queue: ");
    while (curr != NULL) {
        printf("%d ", curr->length);
        curr = curr->next;
    }
    printf("\n");
    pthread_cond_signal(&tp->signal);
    pthread_mutex_unlock(&tp->lock);
    return true;
}

/**
* Get a job from the job queue of the ThreadPool object
* Parameters:
*     tp - Pointer to the ThreadPool object
* Return:
*     ThreadPool_job_t* - Next job to run
*/
ThreadPool_job_t* ThreadPool_get_job(ThreadPool_t* tp) {
    ThreadPool_job_t *job = tp->jobs.head;

    if (job) {
        tp->jobs.head = job->next;
        tp->jobs.size--;
    }
    return job;
}

/**
* Start routine of each thread in the ThreadPool Object
* In a loop, check the job queue, get a job (if any) and run it
* Parameters:
*     tp - Pointer to the ThreadPool object containing this thread
*/
void* Thread_run(void* arg) {
    ThreadPool_t *tp = (ThreadPool_t*) arg;

    while (tp->active == 1) {

        //retrieve job from job queue
        pthread_mutex_lock(&tp->lock);

        //If queue is empty, put thread to sleep until threadPool_add_job signals again
        if (tp->jobs.head == NULL) {
            tp->idlecount++;
            pthread_cond_wait(&tp->signal, &tp->lock);
            tp->idlecount--;
        }

        ThreadPool_job_t *job = ThreadPool_get_job(tp);
        pthread_mutex_unlock(&tp->lock);
        
        //execute job
        if (job != NULL) {
            printf("Executing job of length %d %s\n", job->length, (char*)job->arg);
            job->func(job->arg);
            free(job);
        }
    }
    return NULL;
}

/**
* Ensure that all threads are idle and the job queue is empty before returning
* Parameters:
*     tp - Pointer to the ThreadPool object
*/
void ThreadPool_check(ThreadPool_t *tp) {
    while(tp->jobs.size != 0 && tp->idlecount != tp->num_threads);
}
