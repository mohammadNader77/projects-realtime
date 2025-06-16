// shared_memory.h
#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <float.h>
#include <semaphore.h>

// Maximum constants
#define MAX_COLUMNS 50
#define MAX_ROWS 100000
#define MAX_FILENAME 512

// Shared memory structure for storing averages and file counters
typedef struct {
    float averages[MAX_COLUMNS];
    float min_averages[MAX_COLUMNS];
    float max_averages[MAX_COLUMNS];
    int files_generated;
    int files_processed;
    int files_moved_to_processed;
    int files_moved_to_unprocessed;
    int files_moved_to_backup;
    int files_deleted;
    // Additional fields can be added as needed
} SharedMemory;

// Function prototypes
SharedMemory* init_shared_memory();
void cleanup_shared_memory(SharedMemory *shared_data);
sem_t* init_semaphore();
void cleanup_semaphore(sem_t *sem);

#endif // SHARED_MEMORY_H
