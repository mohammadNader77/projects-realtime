// shared.c
#include "shared.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>           
#include <sys/stat.h>        
#include <sys/mman.h>
#include <unistd.h>

// Function to initialize shared data
SharedData* init_shared_data() {
    const char *shm_name = "/simulation_shared_memory";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return NULL;
    }

    // Size of shared memory
    size_t size = sizeof(SharedData);
    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate failed");
        return NULL;
    }

    // Map shared memory
    SharedData *shared = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }

    // Initialize shared data if first time
    // To keep it simple, we'll assume it's always the first time
    memset(shared, 0, sizeof(SharedData));

    // Initialize semaphores
    if (sem_init(&shared->semaphore, 1, 1) == -1) {
        perror("sem_init semaphore failed");
        return NULL;
    }

    if (sem_init(&shared->group_semaphore, 1, 1) == -1) {
        perror("sem_init group_semaphore failed");
        return NULL;
    }

    return shared;
}

// Function to destroy shared data
int destroy_shared_data(SharedData *data) {
    const char *shm_name = "/simulation_shared_memory";
    if (sem_destroy(&data->semaphore) == -1) {
        perror("sem_destroy semaphore failed");
        return -1;
    }

    if (sem_destroy(&data->group_semaphore) == -1) {
        perror("sem_destroy group_semaphore failed");
        return -1;
    }

    // Unmap shared memory
    if (munmap(data, sizeof(SharedData)) == -1) {
        perror("munmap failed");
        return -1;
    }

    // Unlink shared memory
    if (shm_unlink(shm_name) == -1) {
        perror("shm_unlink failed");
        return -1;
    }

    return 0;
}

// Function to check if a member is suspected
int is_member_suspected(SharedData *shared, int member_id) {
    for (int i = 0; i < shared->suspected_spies_count; i++) {
        if (shared->suspected_spies[i] == member_id)
            return 1;
    }
    return 0;
}
