// inspector_type1.c
#include "shared_memory.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("Inspector Type1 received signal %d. Cleaning up and exiting.\n", sig);
    // Perform any necessary cleanup here
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <inspector_id>\n", argv[0]);
        exit(1);
    }

    int inspector_id = atoi(argv[1]);

    // Register signal handlers
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    // Initialize shared memory and semaphore
    SharedMemory *shared_data = init_shared_memory();
    sem_t *sem = init_semaphore();

    // Read configuration
    Config config;
    parse_config("config.txt", &config);

    // Ensure the UnProcessed directory exists
    struct stat st_dir = {0};
    if (stat("./home/UnProcessed", &st_dir) == -1) {
        if (mkdir("./home/UnProcessed", 0755) != 0) {
            perror("Error creating UnProcessed directory");
            exit(1);
        }
    }

    while (1) {
        DIR *dir = opendir("./home");
        if (dir == NULL) {
            perror("Error opening home directory");
            sleep(5);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Skip directories and non-CSV files
            if (entry->d_type == DT_DIR || !strstr(entry->d_name, ".csv"))
                continue;

            char filepath[MAX_FILENAME];
            // Ensure that the file name does not exceed the buffer size
            snprintf(filepath, sizeof(filepath), "./home/%s", entry->d_name);

            struct stat st;
            if (stat(filepath, &st) == -1) {
                perror("Error getting file status");
                continue;
            }

            time_t current_time = time(NULL);
            double age = difftime(current_time, st.st_mtime);

            // If file is older than Type1 threshold
            if (age > config.type1_threshold_age) {
                char unprocessed_path[MAX_FILENAME];
                // Ensure that the file name does not exceed the buffer size for UnProcessed
                snprintf(unprocessed_path, sizeof(unprocessed_path), "./home/UnProcessed/%s", entry->d_name);

                if (rename(filepath, unprocessed_path) == 0) {
                    sem_wait(sem);
                    shared_data->files_moved_to_unprocessed++;
                    sem_post(sem);
                    printf("Inspector Type1 %d: Moved %s to UnProcessed\n", inspector_id, entry->d_name);
                } else {
                    perror("Error moving file to UnProcessed");
                }
            }
        }

        closedir(dir);
        sleep(10); // Inspect every 10 seconds
    }

    // Cleanup (unreachable in current design)
    cleanup_shared_memory(shared_data);
    cleanup_semaphore(sem);

    return 0;
}
