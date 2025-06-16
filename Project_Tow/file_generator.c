// file_generator.c
#include "shared_memory.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>

// Function to generate random float in a given range
float generate_random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

// Function to create directories if they do not already exist
void create_directory_if_needed(const char *dir_path) {
    struct stat st = {0};
    if (stat(dir_path, &st) == -1) {
        if (mkdir(dir_path, 0755) != 0) {
            perror("Error creating directory");
            exit(1);
        } else {
            printf("Created directory %s\n", dir_path);
        }
    }
}

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("File Generator received signal %d. Cleaning up and exiting.\n", sig);
    // Perform any necessary cleanup here
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <generator_id>\n", argv[0]);
        exit(1);
    }

    int generator_id = atoi(argv[1]);

    // Register signal handlers
    if (signal(SIGTERM, handle_signal) == SIG_ERR) {
        perror("Error registering SIGTERM handler");
        exit(1);
    }
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        perror("Error registering SIGINT handler");
        exit(1);
    }

    // Initialize shared memory and semaphore
    SharedMemory *shared_data = init_shared_memory();
    sem_t *sem = init_semaphore();

    // Read configuration
    Config config;
    parse_config("config.txt", &config);

    // Seed the random number generator
    srand(time(NULL) ^ (getpid() << 16));

    // Ensure the home directory exists
    create_directory_if_needed("./home");

    while (1) {
        // Generate a random sleep interval
        int sleep_time = config.gen_interval_min + rand() % (config.gen_interval_max - config.gen_interval_min + 1);
        sleep(sleep_time);

        // Generate a file name based on the current file count
        sem_wait(sem);
        int file_index = shared_data->files_generated;
        shared_data->files_generated++;
        sem_post(sem);

        char filename[MAX_FILENAME];
        snprintf(filename, sizeof(filename), "./home/%d.csv", file_index);

        // Determine random number of rows and columns
        int num_rows = config.rows_min + rand() % (config.rows_max - config.rows_min + 1);
        int num_columns = config.columns_min + rand() % (config.columns_max - config.columns_min + 1);

        FILE *file = fopen(filename, "w");
        if (file == NULL) {
            perror("Error creating CSV file");
            continue;
        }

        // Write CSV header
        for (int c = 0; c < num_columns; c++) {
            fprintf(file, "Col%d", c);
            if (c < num_columns - 1)
                fprintf(file, ",");
        }
        fprintf(file, "\n");

        // Write random data to CSV
        for (int r = 0; r < num_rows; r++) {
            for (int c = 0; c < num_columns; c++) {
                if (((float)rand() / RAND_MAX) * 100 < config.missing_percentage) {
                    fprintf(file, ","); // Missing data
                } else {
                    fprintf(file, "%.2f,", generate_random_float(config.value_min, config.value_max));
                }
            }
            fprintf(file, "\n");
        }

        fclose(file);
        printf("Generator %d: Generated file %s with %d rows and %d columns\n", generator_id, filename, num_rows, num_columns);
    }

    // Cleanup (unreachable in current design)
    cleanup_shared_memory(shared_data);
    cleanup_semaphore(sem);

    return 0;
}
