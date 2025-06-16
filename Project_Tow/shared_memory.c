// shared_memory.c
#include "shared_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           
#include <unistd.h>
#include <string.h>

// Initialize shared memory
SharedMemory* init_shared_memory() {
    int shm_fd = shm_open("/file_simulation_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error creating shared memory");
        exit(1);
    }

    // Check if this is the first time initializing
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("Error getting shared memory status");
        exit(1);
    }

    if (shm_stat.st_size == 0) {
        if (ftruncate(shm_fd, sizeof(SharedMemory)) == -1) {
            perror("Error truncating shared memory");
            exit(1);
        }
    }

    SharedMemory *shared_data = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(1);
    }

    // Initialize shared memory values only if first time
    if (shm_stat.st_size == 0) {
        for (int i = 0; i < MAX_COLUMNS; i++) {
            shared_data->min_averages[i] = FLT_MAX;
            shared_data->max_averages[i] = -FLT_MAX;
            shared_data->averages[i] = 0.0;
        }

        // Initialize shared counters
        shared_data->files_generated = 0;
        shared_data->files_processed = 0;
        shared_data->files_moved_to_processed = 0;
        shared_data->files_moved_to_unprocessed = 0;
        shared_data->files_moved_to_backup = 0;
        shared_data->files_deleted = 0;
    }

    return shared_data;
}

// Cleanup shared memory
void cleanup_shared_memory(SharedMemory *shared_data) {
    if (shared_data != NULL) {
        munmap(shared_data, sizeof(SharedMemory));
    }
    shm_unlink("/file_simulation_shm");
}

// Initialize semaphore
sem_t* init_semaphore() {
    sem_t *sem = sem_open("/files_semaphore", O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Error opening semaphore");
        exit(1);
    }
    return sem;
}

// Cleanup semaphore
void cleanup_semaphore(sem_t *sem) {
    if (sem != NULL) {
        sem_close(sem);
    }
    sem_unlink("/files_semaphore");
}
