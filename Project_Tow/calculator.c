// calculator.c
#include "shared_memory.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>

// Function prototypes
void handle_signal(int sig);

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("Calculator received signal %d. Cleaning up and exiting.\n", sig);
    // Perform any necessary cleanup here
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <calculator_id>\n", argv[0]);
        exit(1);
    }

    int calculator_id = atoi(argv[1]);

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

    while (1) {
        // Search for an unprocessed file in the home directory
        char filepath[MAX_FILENAME];
        int file_found = 0;
        int file_index = -1; // To store the index of the found file

        sem_wait(sem);
        for (int i = 0; i < shared_data->files_generated; i++) {
            snprintf(filepath, sizeof(filepath), "./home/%d.csv", i);
            struct stat st;
            if (stat(filepath, &st) == 0) {
                // File exists and is unprocessed
                file_found = 1;
                file_index = i; // Store the index
                break;
            }
        }
        sem_post(sem);

        if (file_found && file_index != -1) {
            // Mark file as being processed by moving it to Processing directory
            char temp_path[MAX_FILENAME];
            snprintf(temp_path, sizeof(temp_path), "./home/Processing/%d.csv", file_index);
            if (rename(filepath, temp_path) == 0) {
                sem_wait(sem);
                shared_data->files_processed++;
                sem_post(sem);
                printf("Calculator %d: Processing file %s\n", calculator_id, temp_path);
            } else {
                perror("Error moving file to Processing");
                continue;
            }

            // Simulate processing time
            sleep(2); // Simulate processing delay

            // Calculate averages
            FILE *file = fopen(temp_path, "r");
            if (file == NULL) {
                perror("Error opening file for reading");
                continue;
            }

            char line[1024];
            int num_columns = 0;
            float sum[MAX_COLUMNS] = {0.0};
            int count[MAX_COLUMNS] = {0};

            // Read header to determine number of columns
            if (fgets(line, sizeof(line), file)) {
                char *token = strtok(line, ",");
                while (token != NULL && num_columns < MAX_COLUMNS) {
                    num_columns++;
                    token = strtok(NULL, ",");
                }
            }

            // Read data lines
            while (fgets(line, sizeof(line), file)) {
                char *token = strtok(line, ",");
                int col = 0;
                while (token != NULL && col < num_columns) {
                    if (token[0] != '\0') {
                        sum[col] += atof(token);
                        count[col]++;
                    }
                    token = strtok(NULL, ",");
                    col++;
                }
            }

            fclose(file);

            // Update shared memory with averages
            sem_wait(sem);
            for (int c = 0; c < num_columns; c++) {
                if (count[c] > 0) {
                    float avg = sum[c] / count[c];
                    shared_data->averages[c] = avg;

                    if (avg < shared_data->min_averages[c]) {
                        shared_data->min_averages[c] = avg;
                    }
                    if (avg > shared_data->max_averages[c]) {
                        shared_data->max_averages[c] = avg;
                    }

                    printf("Calculator %d: File %s - Column %d Average: %.2f (Min: %.2f, Max: %.2f)\n",
                           calculator_id, temp_path, c, avg, shared_data->min_averages[c], shared_data->max_averages[c]);
                }
            }
            sem_post(sem);

            // Move file to Processed directory
            char processed_path[MAX_FILENAME];
            snprintf(processed_path, sizeof(processed_path), "./home/Processed/%d.csv", file_index);
            if (rename(temp_path, processed_path) == 0) {
                sem_wait(sem);
                shared_data->files_moved_to_processed++;
                sem_post(sem);
                printf("Calculator %d: Moved file %s to Processed\n", calculator_id, processed_path);
            } else {
                perror("Error moving file to Processed");
            }
        } else {
            // No file found, sleep for a while
            sleep(2);
        }
    }

    // Cleanup (unreachable in current design)
    cleanup_shared_memory(shared_data);
    cleanup_semaphore(sem);

    return 0;
}
