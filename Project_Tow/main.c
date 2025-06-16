// main.c
#include "shared_memory.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>      
#include <string.h>
#include <sys/wait.h>
#include <time.h>

// Global array to hold child PIDs
pid_t *children_pids = NULL;
int total_children = 0;
Config config;

// Directories
const char *directories[] = {
    "./home",
    "./home/UnProcessed",
    "./home/Processed",
    "./home/Backup",
    "./home/Deleted",
    "./home/Processing" 
};

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

// Function to handle termination signals for graceful shutdown
void handle_signal(int sig) {
    printf("Received signal %d. Terminating all child processes.\n", sig);
    // Terminate all child processes
    for (int i = 0; i < total_children; i++) {
        if (kill(children_pids[i], SIGTERM) == -1) {
            perror("Error terminating child process");
        }
    }

    // Wait for all child processes to terminate
    for (int i = 0; i < total_children; i++) {
        waitpid(children_pids[i], NULL, 0);
    }

    // Cleanup shared memory and semaphore
    cleanup_shared_memory(NULL); // Assuming cleanup_shared_memory checks if NULL
    cleanup_semaphore(NULL);     // Assuming cleanup_semaphore checks if NULL

    // Exit the program
    exit(0);
}

int main(int argc, char *argv[]) {
    // Parse configuration file
    parse_config("config.txt", &config);

    // Initialize shared memory and semaphore
    SharedMemory *shared_data = init_shared_memory();
    sem_t *sem = init_semaphore();

    // Create necessary directories
    for (int i = 0; i < sizeof(directories)/sizeof(directories[0]); i++) {
        create_directory_if_needed(directories[i]);
    }

    // Allocate memory for child PIDs
    total_children = config.num_generators + config.num_calculators +
                     config.inspectors_type1 + config.inspectors_type2 + config.inspectors_type3;
    children_pids = malloc(sizeof(pid_t) * total_children);
    if (children_pids == NULL) {
        perror("Error allocating memory for child PIDs");
        cleanup_shared_memory(shared_data);
        cleanup_semaphore(sem);
        exit(1);
    }
    int child_index = 0;

    // Start File Generators
    for (int i = 0; i < config.num_generators; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char generator_id_str[12]; // Increased buffer size
            snprintf(generator_id_str, sizeof(generator_id_str), "%d", i);
            execl("./file_generator", "./file_generator", generator_id_str, NULL);
            perror("Error executing file_generator");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            children_pids[child_index++] = pid;
        } else {
            perror("Error forking generator");
        }
    }

    // Start Calculators
    for (int i = 0; i < config.num_calculators; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char calculator_id_str[12]; // Increased buffer size
            snprintf(calculator_id_str, sizeof(calculator_id_str), "%d", i);
            execl("./calculator", "./calculator", calculator_id_str, NULL);
            perror("Error executing calculator");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            children_pids[child_index++] = pid;
        } else {
            perror("Error forking calculator");
        }
    }

    // Start Inspectors Type1
    for (int i = 0; i < config.inspectors_type1; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char inspector_id_str[12]; // Increased buffer size
            snprintf(inspector_id_str, sizeof(inspector_id_str), "%d", i);
            execl("./inspector_type1", "./inspector_type1", inspector_id_str, NULL);
            perror("Error executing inspector_type1");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            children_pids[child_index++] = pid;
        } else {
            perror("Error forking inspector_type1");
        }
    }

    // Start Inspectors Type2
    for (int i = 0; i < config.inspectors_type2; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char inspector_id_str[12]; // Increased buffer size
            snprintf(inspector_id_str, sizeof(inspector_id_str), "%d", i);
            execl("./inspector_type2", "./inspector_type2", inspector_id_str, NULL);
            perror("Error executing inspector_type2");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            children_pids[child_index++] = pid;
        } else {
            perror("Error forking inspector_type2");
        }
    }

    // Start Inspectors Type3
    for (int i = 0; i < config.inspectors_type3; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char inspector_id_str[12]; // Increased buffer size
            snprintf(inspector_id_str, sizeof(inspector_id_str), "%d", i);
            execl("./inspector_type3", "./inspector_type3", inspector_id_str, NULL);
            perror("Error executing inspector_type3");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            children_pids[child_index++] = pid;
        } else {
            perror("Error forking inspector_type3");
        }
    }

    // Register signal handlers for graceful termination
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        perror("Error registering SIGINT handler");
        exit(1);
    }
    if (signal(SIGTERM, handle_signal) == SIG_ERR) {
        perror("Error registering SIGTERM handler");
        exit(1);
    }

    // Launch Visualization
    pid_t viz_pid = fork();
    if (viz_pid == 0) {
        // Child process
        execl("./visualization", "./visualization", NULL);
        perror("Error executing visualization");
        exit(1);
    } else if (viz_pid > 0) {
        // Parent process
        children_pids[child_index++] = viz_pid;
    } else {
        perror("Error forking visualization");
    }

    // Monitor termination conditions
    time_t start_time = time(NULL);
    while (1) {
        sleep(1); // Check every second

        // Calculate elapsed time in minutes
        time_t current_time = time(NULL);
        double elapsed_minutes = difftime(current_time, start_time) / 60.0;

        // Fetch shared counters
        sem_wait(sem);
        int processed = shared_data->files_processed;
        int not_processed = shared_data->files_generated - shared_data->files_processed;
        int backup = shared_data->files_moved_to_backup;
        int deleted = shared_data->files_deleted;
        sem_post(sem);

        // Check termination conditions
        if (processed > config.threshold_files_processed ||
            not_processed > config.threshold_files_not_processed ||
            backup > config.threshold_files_backup ||
            deleted > config.threshold_files_deleted ||
            elapsed_minutes > config.runtime_limit_minutes) {

            printf("\n*** Termination Condition Met ***\n");
            printf("Files Processed: %d (Threshold: %d)\n", processed, config.threshold_files_processed);
            printf("Files Not Processed: %d (Threshold: %d)\n", not_processed, config.threshold_files_not_processed);
            printf("Files Moved to Backup: %d (Threshold: %d)\n", backup, config.threshold_files_backup);
            printf("Files Deleted: %d (Threshold: %d)\n", deleted, config.threshold_files_deleted);
            printf("Elapsed Time: %.2f minutes (Limit: %d minutes)\n", elapsed_minutes, config.runtime_limit_minutes);
            printf("Terminating all child processes...\n\n");

            handle_signal(SIGTERM);
        }
    }

    // Cleanup (unreachable in current design)
    cleanup_shared_memory(shared_data);
    cleanup_semaphore(sem);
    free(children_pids);

    return 0;
}
