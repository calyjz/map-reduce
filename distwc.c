#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapreduce.h"
#include <sys/types.h>
#include <unistd.h>
#include <time.h>


void Map(char* file_name) {
    FILE* fp = fopen(file_name, "r");
    assert(fp != NULL);

    char* line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1) {
        char *token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
            MR_Emit(token, "1");
        }
    }
    free(line);
    fclose(fp);
}

void Reduce(char* key, unsigned int partition_idx) {
    int count = 0;
    char *value, name[100];
    while ((value = MR_GetNext(key, partition_idx)) != NULL) {
        count++;
        free(value);
    }
    sprintf(name, "result-%d.txt", partition_idx);
    FILE* fp = fopen(name, "a");
    fprintf(fp, "%s: %d\n", key, count);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    struct timespec start,  end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    MR_Run(argc - 1, &(argv[1]), Map, Reduce, 10, 1);
    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000; 
    }
    long milliseconds = nanoseconds / 1000000;
    printf("Total Time: %ld.%02ld\n", seconds, milliseconds / 10);
}
