// inspector_type2.c
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
    printf("Inspector Type2 received signal %d. Cleaning up and exiting.\n", sig);
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

    // Ensure the Backup directory exists
    struct stat st_dir = {0};
    if (stat("./home/Backup", &st_dir) == -1) {
        if (mkdir("./home/Backup", 0755) != 0) {
            perror("Error creating Backup directory");
            exit(1);
        }
    }

    while (1) {
        DIR *dir = opendir("./home/Processed");
        if (dir == NULL) {
            perror("Error opening Processed directory");
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
            snprintf(filepath, sizeof(filepath), "./home/Processed/%s", entry->d_name);

            struct stat st;
            if (stat(filepath, &st) == -1) {
                perror("Error getting file status");
                continue;
            }

            time_t current_time = time(NULL);
            double age = difftime(current_time, st.st_mtime);

            // If file is older than Type2 threshold
            if (age > config.type1_threshold_age) { // You may define separate thresholds for different inspector types
                char backup_path[MAX_FILENAME];
                // Ensure that the file name does not exceed the buffer size for Backup
                snprintf(backup_path, sizeof(backup_path), "./home/Backup/%s", entry->d_name);

                if (rename(filepath, backup_path) == 0) {
                    sem_wait(sem);
                    shared_data->files_moved_to_backup++;
                    sem_post(sem);
                    printf("Inspector Type2 %d: Moved %s to Backup\n", inspector_id, entry->d_name);
                }
            }
        }

        closedir(dir);
        sleep(15); // Inspect every 15 seconds
    }

    // Cleanup (unreachable in current design)
    cleanup_shared_memory(shared_data);
    cleanup_semaphore(sem);

    return 0;
}
