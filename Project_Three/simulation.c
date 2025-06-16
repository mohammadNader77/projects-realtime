// simulation.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "config.h"
#include "shared.h"
#include "visualization.h"

// Global variable to control simulation running state
volatile int simulation_running = 1;

// Signal handler to gracefully terminate simulation
void handle_sigint(int sig) {
    (void)sig; // Marking 'sig' as unused to prevent compiler warnings
    simulation_running = 0;
}

// Structure to pass arguments to resistance member threads
typedef struct {
    int member_id;
    int is_spy;
    SharedData *shared;
    Config *config;
    int group_id; // Associate member with a group
} ResistanceMemberArgs;

// Structure to pass arguments to agency member threads
typedef struct {
    int agency_id;
    SharedData *shared;
    Config *config;
} AgencyMemberArgs;

// Structure to pass arguments to resistance group manager
typedef struct {
    Config config;
    SharedData *shared;
} ResManagerArgs;

// Structure to pass arguments to agency monitor thread
typedef struct {
    SharedData *shared;
    Config *config;
} MonitorArgs;

// Function declarations
void *resistance_group_manager(void *args);
void *resistance_member_thread(void *args);
void *agency_member_thread(void *args);
void *agency_monitor_thread(void *args);

// Function to simulate civilian spying
void *civilian_process(void *args) {
    SharedData *shared = (SharedData *)args;
    srand(time(NULL) ^ (getpid() << 16)); // Seed randomness

    while (simulation_running) {
        printf("[Civilian] Spying on resistance groups...\n");
        sleep(5); // Time between spying activities

        // Share data with agency
        sem_wait(&shared->semaphore);
        shared->data_shared += 1; // Increment shared data counter
        sem_post(&shared->semaphore);
    }
    return NULL;
}

// Function to manage resistance groups
void *resistance_group_manager(void *args) {
    ResManagerArgs *res_args = (ResManagerArgs *)args;
    if (res_args == NULL) {
        fprintf(stderr, "[Resistance Group Manager] Invalid arguments.\n");
        pthread_exit(NULL);
    }

    Config config = res_args->config;
    SharedData *shared = res_args->shared;

    int group_id = 1;

    while (simulation_running) {
        // Create a new resistance group
        int group_size = config.group_size_min + rand() % (config.group_size_max - config.group_size_min + 1);
        int is_military = (rand() % 100) < config.military_group_percentage ? 1 : 0;
        printf("[Resistance] Creating a %s group (ID: %d) with %d members.\n", 
               is_military ? "Military" : "Social", group_id, group_size);

        // Determine if group has a spy
        int has_spy = (rand() % 100) < config.spy_infiltration_probability ? 1 : 0; // Based on config

        // Assign random position to the group
        float pos_x = (float)(rand() % 200 - 100); // X between -100 to +99
        float pos_y = (float)(rand() % 200 - 100); // Y between -100 to +99

        // Initialize group data
        sem_wait(&shared->group_semaphore);
        if (shared->resistance_groups_count < MAX_GROUPS) {
            shared->resistance_groups[shared->resistance_groups_count].group_id = group_id;
            shared->resistance_groups[shared->resistance_groups_count].has_spy = has_spy;
            shared->resistance_groups[shared->resistance_groups_count].is_military = is_military;
            shared->resistance_groups[shared->resistance_groups_count].spy_time = 0;
            shared->resistance_groups[shared->resistance_groups_count].current_member_count = group_size;
            shared->resistance_groups[shared->resistance_groups_count].x = pos_x;
            shared->resistance_groups[shared->resistance_groups_count].y = pos_y;
            shared->resistance_groups_count++;
        } else {
            fprintf(stderr, "[Resistance Group Manager] Maximum number of groups reached.\n");
        }
        sem_post(&shared->group_semaphore);

        // Create threads for each resistance group member
        pthread_t *member_threads = malloc(sizeof(pthread_t) * group_size);
        if (member_threads == NULL) {
            perror("[Resistance] Failed to allocate memory for member threads");
            pthread_exit(NULL);
        }

        ResistanceMemberArgs *member_args = malloc(sizeof(ResistanceMemberArgs) * group_size);
        if (member_args == NULL) {
            perror("[Resistance] Failed to allocate memory for member args");
            free(member_threads);
            pthread_exit(NULL);
        }

        for (int i = 0; i < group_size; i++) {
            member_args[i].member_id = group_id * 1000 + (i + 1); // Unique ID
            member_args[i].is_spy = (has_spy && i == 0) ? 1 : 0;
            member_args[i].shared = shared;
            member_args[i].config = &config;
            member_args[i].group_id = group_id; // Assign group ID

            if (member_args[i].is_spy) {
                printf("[Resistance Member %d (Spy)] Spy infiltrated Group %d.\n", 
                       member_args[i].member_id, group_id);
                // Add to spy_member_ids
                sem_wait(&shared->semaphore);
                if (shared->spy_member_ids_count < MAX_SPYS) {
                    shared->spy_member_ids[shared->spy_member_ids_count++] = member_args[i].member_id;
                }
                sem_post(&shared->semaphore);
            }

            // Allocate memory for each member's arguments
            ResistanceMemberArgs *arg_copy = malloc(sizeof(ResistanceMemberArgs));
            if (arg_copy == NULL) {
                perror("[Resistance] Failed to allocate memory for member arg copy");
                continue; // Skip creating this member
            }
            memcpy(arg_copy, &member_args[i], sizeof(ResistanceMemberArgs));

            pthread_t member_thread;
            if (pthread_create(&member_thread, NULL, resistance_member_thread, arg_copy) != 0) {
                perror("[Resistance] Failed to create Resistance Member thread");
                free(arg_copy);
                continue; // Skip creating this member
            }

            // Detach the thread since we won't join it
            pthread_detach(member_thread);
        }

        // Increment total resistance groups
        sem_wait(&shared->semaphore);
        shared->total_resistance_groups += 1;
        sem_post(&shared->semaphore);

        // Sleep for group_creation_interval before creating the next group
        sleep(config.group_creation_interval);

        free(member_threads);
        free(member_args);

        group_id++;
    }

    // Monitoring loop to replace killed or caught members
    while (simulation_running) {
        sleep(2); // Check every 2 seconds

        sem_wait(&shared->group_semaphore);
        for (int i = 0; i < shared->resistance_groups_count; i++) {
            ResistanceGroup *group = &shared->resistance_groups[i];
            if (group->current_member_count < config.group_size_max) {
                int members_to_add = config.group_size_max - group->current_member_count;
                for (int j = 0; j < members_to_add; j++) {
                    int new_member_id = group->group_id * 1000 + (group->current_member_count + 1);
                    ResistanceMemberArgs *new_member_args = malloc(sizeof(ResistanceMemberArgs));
                    if (new_member_args == NULL) {
                        perror("[Resistance Group Manager] Failed to allocate memory for new member args");
                        continue;
                    }
                    new_member_args->member_id = new_member_id;
                    new_member_args->is_spy = 0; // New members are not spies
                    new_member_args->shared = shared;
                    new_member_args->config = &config;
                    new_member_args->group_id = group->group_id;

                    pthread_t new_member_thread;
                    if (pthread_create(&new_member_thread, NULL, resistance_member_thread, new_member_args) != 0) {
                        perror("[Resistance Group Manager] Failed to create new Resistance Member thread");
                        free(new_member_args);
                        continue;
                    }

                    pthread_detach(new_member_thread);

                    group->current_member_count += 1;
                    printf("[Resistance Group Manager] Replaced missing member in Group %d with Member %d.\n",
                           group->group_id, new_member_id);
                }
            }
        }
        sem_post(&shared->group_semaphore);
    }

    pthread_exit(NULL);
}

// Function to simulate resistance group member behavior
void *resistance_member_thread(void *args) {
    ResistanceMemberArgs *member = (ResistanceMemberArgs *)args;
    if (member == NULL) {
        fprintf(stderr, "[Resistance Member] Invalid arguments.\n");
        pthread_exit(NULL);
    }

    SharedData *shared = member->shared;
    Config *config = member->config;

    while (simulation_running) {
        // Check if this spy has been suspected and arrested
        if (member->is_spy && is_member_suspected(shared, member->member_id)) {
            printf("[Resistance Member %d (Spy)] Arrested by the agency.\n", member->member_id);
            sem_wait(&shared->semaphore);
            shared->caught_resistance += 1;
            // Remove from spy_member_ids
            for (int i = 0; i < shared->spy_member_ids_count; i++) {
                if (shared->spy_member_ids[i] == member->member_id) {
                    for (int j = i; j < shared->spy_member_ids_count - 1; j++) {
                        shared->spy_member_ids[j] = shared->spy_member_ids[j + 1];
                    }
                    shared->spy_member_ids_count--;
                    break;
                }
            }
            // Remove from suspected_spies
            for (int i = 0; i < shared->suspected_spies_count; i++) {
                if (shared->suspected_spies[i] == member->member_id) {
                    for (int j = i; j < shared->suspected_spies_count - 1; j++) {
                        shared->suspected_spies[j] = shared->suspected_spies[j + 1];
                    }
                    shared->suspected_spies_count--;
                    break;
                }
            }
            sem_post(&shared->semaphore);

            // Decrement group's member count and record position
            sem_wait(&shared->group_semaphore);
            for (int i = 0; i < shared->resistance_groups_count; i++) {
                if (shared->resistance_groups[i].group_id == member->group_id) {
                    shared->resistance_groups[i].current_member_count -= 1;
                    // Record position
                    if (shared->caught_resistance_positions_count < MAX_CAUGHT_RESISTANCE) {
                        shared->caught_resistance_positions[shared->caught_resistance_positions_count][0] = shared->resistance_groups[i].x;
                        shared->caught_resistance_positions[shared->caught_resistance_positions_count][1] = shared->resistance_groups[i].y;
                        shared->caught_resistance_positions_count++;
                    }
                    break;
                }
            }
            sem_post(&shared->group_semaphore);
            break; // Exit thread
        }

        if (member->is_spy) {
            // Spy behavior: gathers intelligence, affects targeting
            printf("[Resistance Member %d (Spy)] Gathering intelligence in Group %d...\n", 
                   member->member_id, member->group_id);
            sleep(3); // Time spent gathering intelligence

            // Increment spy_time for the group
            sem_wait(&shared->group_semaphore);
            for (int i = 0; i < shared->resistance_groups_count; i++) {
                if (shared->resistance_groups[i].group_id == member->group_id) {
                    shared->resistance_groups[i].spy_time += 3; // Increment by sleep time
                    break;
                }
            }
            sem_post(&shared->group_semaphore);

            // The spy contributes more data_shared, affecting targeting
            sem_wait(&shared->semaphore);
            shared->data_shared += 2; // Spies contribute more data
            sem_post(&shared->semaphore);
        } else {
            // Regular member behavior
            printf("[Resistance Member %d] Engaging in activities in Group %d...\n", 
                   member->member_id, member->group_id);
            sleep(5); // Time spent in activities

            // Possibly share data
            sem_wait(&shared->semaphore);
            shared->data_shared += 1; // Regular members contribute some data
            sem_post(&shared->semaphore);
        }

        // Simulate possible targeting by the enemy
        double targeting_chance = 0.05; // Base chance

        // Adjust targeting chance based on group type
        sem_wait(&shared->group_semaphore);
        int is_military = 0;
        int spy_time = 0;
        for (int i = 0; i < shared->resistance_groups_count; i++) {
            if (shared->resistance_groups[i].group_id == member->group_id) {
                is_military = shared->resistance_groups[i].is_military;
                spy_time = shared->resistance_groups[i].spy_time;
                break;
            }
        }
        sem_post(&shared->group_semaphore);

        if (is_military) {
            targeting_chance += 0.05; // Military groups have higher base targeting
        }

        // Increase targeting chance based on spy_time
        targeting_chance += (spy_time * 0.01); // Each unit of spy_time increases chance by 1%

        if (member->is_spy) {
            // Spies have lower chance or do not sustain injuries
            if (((double)rand() / RAND_MAX) < targeting_chance) {
                printf("[Resistance Member %d (Spy)] Targeted by the enemy but no serious injury.\n", 
                       member->member_id);
                // Spy is not injured or killed, just observed
            }
        } else {
            if (((double)rand() / RAND_MAX) < targeting_chance) {
                printf("[Resistance Member %d] Targeted by the enemy!\n", member->member_id);
                // Determine outcome
                int outcome = rand() % 3; // 0: killed, 1: injured, 2: caught
                if (outcome == 0) {
                    printf("[Resistance Member %d] Killed by the enemy.\n", member->member_id);
                    sem_wait(&shared->semaphore);
                    shared->killed_resistance += 1;
                    sem_post(&shared->semaphore);

                    // Decrement group's member count and record position
                    sem_wait(&shared->group_semaphore);
                    for (int i = 0; i < shared->resistance_groups_count; i++) {
                        if (shared->resistance_groups[i].group_id == member->group_id) {
                            shared->resistance_groups[i].current_member_count -= 1;
                            // Record position
                            if (shared->caught_resistance_positions_count < MAX_CAUGHT_RESISTANCE) {
                                shared->caught_resistance_positions[shared->caught_resistance_positions_count][0] = shared->resistance_groups[i].x;
                                shared->caught_resistance_positions[shared->caught_resistance_positions_count][1] = shared->resistance_groups[i].y;
                                shared->caught_resistance_positions_count++;
                            }
                            break;
                        }
                    }
                    sem_post(&shared->group_semaphore);
                    break; // Exit thread
                } else if (outcome == 1) {
                    // Light or severe injury
                    double injury_chance = 0.7; // 70% chance of light injury
                    if (((double)rand() / RAND_MAX) < injury_chance) {
                        printf("[Resistance Member %d] Lightly injured by the enemy.\n", member->member_id);
                        sem_wait(&shared->semaphore);
                        shared->injured_resistance += 1;
                        sem_post(&shared->semaphore);
                        // Recover after light injury period
                        sleep(config->recovery_light);
                        printf("[Resistance Member %d] Recovered from light injury and rejoining.\n", 
                               member->member_id);
                        sem_wait(&shared->semaphore);
                        shared->injured_resistance -= 1;
                        sem_post(&shared->semaphore);
                        // Continue the loop, rejoining the group
                    } else {
                        printf("[Resistance Member %d] Severely injured by the enemy and exiting.\n", 
                               member->member_id);
                        sem_wait(&shared->semaphore);
                        shared->injured_resistance += 1;
                        sem_post(&shared->semaphore);

                        // Decrement group's member count and record position
                        sem_wait(&shared->group_semaphore);
                        for (int i = 0; i < shared->resistance_groups_count; i++) {
                            if (shared->resistance_groups[i].group_id == member->group_id) {
                                shared->resistance_groups[i].current_member_count -= 1;
                                // Record position
                                if (shared->caught_resistance_positions_count < MAX_CAUGHT_RESISTANCE) {
                                    shared->caught_resistance_positions[shared->caught_resistance_positions_count][0] = shared->resistance_groups[i].x;
                                    shared->caught_resistance_positions_count++;
                                }
                                break;
                            }
                        }
                        sem_post(&shared->group_semaphore);
                        break; // Exit thread
                    }
                } else {
                    printf("[Resistance Member %d] Caught by the enemy.\n", member->member_id);
                    sem_wait(&shared->semaphore);
                    shared->caught_resistance += 1;
                    sem_post(&shared->semaphore);

                    // Decrement group's member count and record position
                    sem_wait(&shared->group_semaphore);
                    for (int i = 0; i < shared->resistance_groups_count; i++) {
                        if (shared->resistance_groups[i].group_id == member->group_id) {
                            shared->resistance_groups[i].current_member_count -= 1;
                            // Record position
                            if (shared->caught_resistance_positions_count < MAX_CAUGHT_RESISTANCE) {
                                shared->caught_resistance_positions[shared->caught_resistance_positions_count][0] = shared->resistance_groups[i].x;
                                shared->caught_resistance_positions_count++;
                            }
                            break;
                        }
                    }
                    sem_post(&shared->group_semaphore);
                    break; // Exit thread
                }
            }
        }
    }

    // If member exits normally, decrement group member count
    sem_wait(&shared->group_semaphore);
    for (int i = 0; i < shared->resistance_groups_count; i++) {
        if (shared->resistance_groups[i].group_id == member->group_id) {
            shared->resistance_groups[i].current_member_count -= 1;
            break;
        }
    }
    sem_post(&shared->group_semaphore);

    // Free the member arguments before exiting
    free(member);
    pthread_exit(NULL);
}

// Function to simulate agency member behavior
void *agency_member_thread(void *args) {
    AgencyMemberArgs *agency_member = (AgencyMemberArgs *)args;
    if (agency_member == NULL) {
        fprintf(stderr, "[Agency Member] Invalid arguments.\n");
        pthread_exit(NULL);
    }

    int agency_id = agency_member->agency_id;
    SharedData *shared = agency_member->shared;
    Config *config = agency_member->config;

    // Assign initial position
    sem_wait(&shared->semaphore);
    if (agency_id -1 < MAX_AGENCY_MEMBERS) {
        shared->agency_positions[agency_id -1][0] = (float)(rand() % 200 - 100); // X between -100 to +99
        shared->agency_positions[agency_id -1][1] = (float)(rand() % 200 - 100); // Y between -100 to +99
        shared->agency_positions_count++;
    }
    sem_post(&shared->semaphore);

    // Tracking the start time for the agency member
    time_t join_time = time(NULL);

    while (simulation_running) {
        // Simulate analyzing data
        printf("[Agency Member %d] Analyzing data for potential spies...\n", agency_id);
        sleep(7); // Time taken to analyze data

        // Simulate agency member being targeted based on time in agency
        time_t current_time = time(NULL);
        double time_in_agency = difftime(current_time, join_time);
        double target_chance = time_in_agency / config->agency_time_limit; // Linear increase

        if (((double)rand() / RAND_MAX) < target_chance * 0.1) { // Adjusted target chance
            // Agency member is targeted
            printf("[Agency Member %d] Targeted by the enemy!\n", agency_id);
            // Determine outcome
            int outcome = rand() % 3; // 0: killed, 1: injured, 2: caught
            if (outcome == 0) {
                printf("[Agency Member %d] Killed by the enemy.\n", agency_id);
                sem_wait(&shared->semaphore);
                shared->killed_agency += 1;
                shared->current_agency_members -= 1;
                sem_post(&shared->semaphore);

                // Record position
                sem_wait(&shared->semaphore);
                if (agency_id -1 < shared->agency_positions_count && shared->caught_agency_positions_count < MAX_CAUGHT_AGENCY) {
                    shared->caught_agency_positions[shared->caught_agency_positions_count][0] = shared->agency_positions[agency_id -1][0];
                    shared->caught_agency_positions[shared->caught_agency_positions_count][1] = shared->agency_positions[agency_id -1][1];
                    shared->caught_agency_positions_count++;
                }
                sem_post(&shared->semaphore);
                break; // Exit thread
            } else if (outcome == 1) {
                // Light or severe injury
                double injury_chance = 0.7; // 70% chance of light injury
                if (((double)rand() / RAND_MAX) < injury_chance) {
                    printf("[Agency Member %d] Lightly injured by the enemy.\n", agency_id);
                    sem_wait(&shared->semaphore);
                    shared->injured_agency += 1;
                    sem_post(&shared->semaphore);
                    // Recover after light injury period
                    sleep(config->recovery_light);
                    printf("[Agency Member %d] Recovered from light injury and resuming duties.\n", agency_id);
                    sem_wait(&shared->semaphore);
                    shared->injured_agency -= 1;
                    sem_post(&shared->semaphore);
                } else {
                    printf("[Agency Member %d] Severely injured by the enemy and exiting.\n", agency_id);
                    sem_wait(&shared->semaphore);
                    shared->injured_agency += 1;
                    shared->current_agency_members -= 1;
                    sem_post(&shared->semaphore);

                    // Record position
                    sem_wait(&shared->semaphore);
                    if (agency_id -1 < shared->agency_positions_count && shared->caught_agency_positions_count < MAX_CAUGHT_AGENCY) {
                        shared->caught_agency_positions[shared->caught_agency_positions_count][0] = shared->agency_positions[agency_id -1][0];
                        shared->caught_agency_positions[shared->caught_agency_positions_count][1] = shared->agency_positions[agency_id -1][1];
                        shared->caught_agency_positions_count++;
                    }
                    sem_post(&shared->semaphore);
                    break; // Exit thread
                }
            } else {
                printf("[Agency Member %d] Caught by the enemy.\n", agency_id);
                sem_wait(&shared->semaphore);
                shared->killed_agency += 1; // Treat caught as killed
                shared->current_agency_members -= 1;
                sem_post(&shared->semaphore);

                // Record position
                sem_wait(&shared->semaphore);
                if (agency_id -1 < shared->agency_positions_count && shared->caught_agency_positions_count < MAX_CAUGHT_AGENCY) {
                    shared->caught_agency_positions[shared->caught_agency_positions_count][0] = shared->agency_positions[agency_id -1][0];
                    shared->caught_agency_positions[shared->caught_agency_positions_count][1] = shared->agency_positions[agency_id -1][1];
                    shared->caught_agency_positions_count++;
                }
                sem_post(&shared->semaphore);
                break; // Exit thread
            }
        }

        // Analyze shared data to detect spies based on per-group data
        sem_wait(&shared->group_semaphore);
        int total_suspected = 0;
        for (int i = 0; i < shared->resistance_groups_count; i++) {
            ResistanceGroup group = shared->resistance_groups[i];
            double group_suspicion = (double)group.spy_time / config->agency_time_limit;

            if (group_suspicion > config->suspicion_threshold && group.has_spy) {
                // Identify the spy in the group
                int spy_id = group.group_id * 1000 + 1; // Assuming first member is spy
                sem_wait(&shared->semaphore);
                // Add to suspected_spies if not already
                int already_suspected = 0;
                for (int j = 0; j < shared->suspected_spies_count; j++) {
                    if (shared->suspected_spies[j] == spy_id) {
                        already_suspected = 1;
                        break;
                    }
                }
                if (!already_suspected && shared->suspected_spies_count < MAX_SUSPECTS) {
                    shared->suspected_spies[shared->suspected_spies_count++] = spy_id;
                    shared->total_arrests += 1;
                    printf("[Agency Member %d] Suspected Spy ID %d added to arrests.\n", 
                           agency_id, spy_id);
                }
                sem_post(&shared->semaphore);
                total_suspected++;
            }
        }
        sem_post(&shared->group_semaphore);

        // Decide to release or imprison suspected spies based on thresholds
        sem_wait(&shared->semaphore);
        for (int i = 0; i < shared->suspected_spies_count; ) {
            int suspect_id = shared->suspected_spies[i];
            // Calculate updated suspicion level for the suspect's group
            int group_id = suspect_id / 1000;
            double suspicion = 0.0;

            sem_wait(&shared->group_semaphore);
            for (int j = 0; j < shared->resistance_groups_count; j++) {
                if (shared->resistance_groups[j].group_id == group_id) {
                    suspicion = (double)shared->resistance_groups[j].spy_time / config->agency_time_limit;
                    break;
                }
            }
            sem_post(&shared->group_semaphore);

            if (suspicion < config->arrest_release_threshold) {
                printf("[Agency Member %d] Releasing Suspect ID %d.\n", agency_id, suspect_id);
                shared->total_released += 1;
                // Remove suspect from suspected_spies
                for (int j = i; j < shared->suspected_spies_count - 1; j++) {
                    shared->suspected_spies[j] = shared->suspected_spies[j + 1];
                }
                shared->suspected_spies_count--;
                // Do not increment i, as the current index now has a new suspect
            } else if (suspicion > config->arrest_imprison_threshold) {
                printf("[Agency Member %d] Imprisoning Suspect ID %d.\n", agency_id, suspect_id);
                shared->total_imprisoned += 1;
                // Remove from suspected_spies
                for (int j = i; j < shared->suspected_spies_count - 1; j++) {
                    shared->suspected_spies[j] = shared->suspected_spies[j + 1];
                }
                shared->suspected_spies_count--;
                // Remove from spy_member_ids
                for (int j = 0; j < shared->spy_member_ids_count; j++) {
                    if (shared->spy_member_ids[j] == suspect_id) {
                        for (int k = j; k < shared->spy_member_ids_count - 1; k++) {
                            shared->spy_member_ids[k] = shared->spy_member_ids[k + 1];
                        }
                        shared->spy_member_ids_count--;
                        break;
                    }
                }
                // Do not increment i, as the current index now has a new suspect
            } else {
                // Middle suspicion, decide based on additional logic or default action
                printf("[Agency Member %d] Maintaining Suspect ID %d status.\n", agency_id, suspect_id);
                i++; // Move to next suspect
            }
        }
        sem_post(&shared->semaphore);

        // Reset spy_time after processing
        sem_wait(&shared->group_semaphore);
        for (int i = 0; i < shared->resistance_groups_count; i++) {
            shared->resistance_groups[i].spy_time = 0;
        }
        sem_post(&shared->group_semaphore);

        // Simulate movement
        float dx = ((float)(rand() % 21) - 10) / 10.0f; // -1.0 to +1.0
        float dy = ((float)(rand() % 21) - 10) / 10.0f; // -1.0 to +1.0

        sem_wait(&shared->semaphore);
        if (agency_id -1 < MAX_AGENCY_MEMBERS) {
            shared->agency_positions[agency_id -1][0] += dx;
            shared->agency_positions[agency_id -1][1] += dy;

            // Ensure positions stay within -100 to +100
            if (shared->agency_positions[agency_id -1][0] < -100.0f) shared->agency_positions[agency_id -1][0] = -100.0f;
            if (shared->agency_positions[agency_id -1][0] > 100.0f) shared->agency_positions[agency_id -1][0] = 100.0f;
            if (shared->agency_positions[agency_id -1][1] < -100.0f) shared->agency_positions[agency_id -1][1] = -100.0f;
            if (shared->agency_positions[agency_id -1][1] > 100.0f) shared->agency_positions[agency_id -1][1] = 100.0f;
        }
        sem_post(&shared->semaphore);
    }

    // Free the agency member arguments before exiting
    free(agency_member);
    pthread_exit(NULL);
}

// Function to monitor and replace agency members
void *agency_monitor_thread(void *args) {
    MonitorArgs *mon_args = (MonitorArgs *)args;
    if (mon_args == NULL) {
        fprintf(stderr, "[Agency Monitor] Invalid arguments.\n");
        pthread_exit(NULL);
    }

    SharedData *shared = mon_args->shared;
    Config *config = mon_args->config;

    int agency_id_counter = config->agency_members + 1;

    while (simulation_running) {
        sleep(5); // Check every 5 seconds

        sem_wait(&shared->semaphore);
        int current_members = shared->current_agency_members;
        sem_post(&shared->semaphore);

        if (current_members < config->agency_members) {
            // Spawn new agency member
            printf("[Agency Monitor] Agency member missing. Spawning new member %d.\n", agency_id_counter);

            // Allocate memory for new member args
            AgencyMemberArgs *new_member_args = malloc(sizeof(AgencyMemberArgs));
            if (new_member_args == NULL) {
                perror("[Agency Monitor] Failed to allocate memory for new agency member args");
                continue;
            }
            new_member_args->agency_id = agency_id_counter;
            new_member_args->shared = shared;
            new_member_args->config = config;

            pthread_t new_member_thread;
            if (pthread_create(&new_member_thread, NULL, agency_member_thread, new_member_args) != 0) {
                perror("[Agency Monitor] Failed to create new Agency Member thread");
                free(new_member_args);
                continue;
            }

            // Detach the new thread as it will run independently
            pthread_detach(new_member_thread);

            // Update agency member count
            sem_wait(&shared->semaphore);
            shared->current_agency_members += 1;
            sem_post(&shared->semaphore);

            agency_id_counter++;
        }
    }

    // Free monitor_args before exiting
    free(mon_args);
    pthread_exit(NULL);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s config.txt\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Register signal handler for graceful termination
    signal(SIGINT, handle_sigint);

    // Parse configuration
    Config config;
    if (parse_config(argv[1], &config) != 0) {
        fprintf(stderr, "Failed to parse configuration file.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize shared data
    SharedData *shared = init_shared_data();
    if (shared == NULL) {
        fprintf(stderr, "Failed to initialize shared data.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize agency members count
    sem_wait(&shared->semaphore);
    shared->current_agency_members = config.agency_members;
    sem_post(&shared->semaphore);

    // Fork processes for Civilians, Resistance Groups, Agency, and Visualization
    pid_t pid_civilian, pid_resistance, pid_agency, pid_visualization;

    pid_civilian = fork();
    if (pid_civilian < 0) {
        perror("Failed to fork Civilians process");
        destroy_shared_data(shared);
        exit(EXIT_FAILURE);
    }
    if (pid_civilian == 0) {
        // Child process: Civilians
        civilian_process(shared);
        exit(EXIT_SUCCESS);
    }

    pid_resistance = fork();
    if (pid_resistance < 0) {
        perror("Failed to fork Resistance Groups process");
        kill(pid_civilian, SIGTERM);
        destroy_shared_data(shared);
        exit(EXIT_FAILURE);
    }
    if (pid_resistance == 0) {
        // Child process: Resistance Groups
        ResManagerArgs res_args = { config, shared };
        resistance_group_manager(&res_args);
        exit(EXIT_SUCCESS);
    }

    pid_agency = fork();
    if (pid_agency < 0) {
        perror("Failed to fork Agency process");
        kill(pid_civilian, SIGTERM);
        kill(pid_resistance, SIGTERM);
        destroy_shared_data(shared);
        exit(EXIT_FAILURE);
    }
    if (pid_agency == 0) {
        // Child process: Counter Espionage Agency
        // Create agency member threads
        for (int i = 0; i < config.agency_members; i++) {
            AgencyMemberArgs *member_args = malloc(sizeof(AgencyMemberArgs));
            if (member_args == NULL) {
                perror("[Agency] Failed to allocate memory for agency member args");
                continue; // Skip creating this member
            }
            member_args->agency_id = i + 1;
            member_args->shared = shared;
            member_args->config = &config;

            pthread_t member_thread;
            if (pthread_create(&member_thread, NULL, agency_member_thread, member_args) != 0) {
                perror("[Agency] Failed to create Agency Member thread");
                free(member_args);
                continue; // Skip creating this member
            }

            // Detach the thread since we won't join it
            pthread_detach(member_thread);
        }

        // Allocate memory for monitor thread arguments
        MonitorArgs *monitor_args = malloc(sizeof(MonitorArgs));
        if (monitor_args == NULL) {
            perror("[Agency Monitor] Failed to allocate memory for monitor arguments");
            exit(EXIT_FAILURE);
        }
        monitor_args->shared = shared;
        monitor_args->config = &config;

        // Create an agency monitor thread to replace agency members if needed
        pthread_t monitor_thread;
        if (pthread_create(&monitor_thread, NULL, agency_monitor_thread, monitor_args) != 0) {
            perror("Failed to create Agency Monitor thread");
            free(monitor_args);
            exit(EXIT_FAILURE);
        }

        // Wait for simulation to end
        while (simulation_running) {
            sleep(1);
        }

        // Terminate monitor thread
        pthread_cancel(monitor_thread);
        pthread_join(monitor_thread, NULL);

        exit(EXIT_SUCCESS);
    }

    pid_visualization = fork();
    if (pid_visualization < 0) {
        perror("Failed to fork Visualization process");
        kill(pid_civilian, SIGTERM);
        kill(pid_resistance, SIGTERM);
        kill(pid_agency, SIGTERM);
        destroy_shared_data(shared);
        exit(EXIT_FAILURE);
    }
    if (pid_visualization == 0) {
        // Child process: Visualization
        // Initialize OpenGL and start rendering
        init_opengl(argc, argv); // Pass argc and argv here
        render(shared);
        exit(EXIT_SUCCESS);
    }

    // Parent process monitors termination conditions
    time_t start_time = time(NULL);
    while (simulation_running) {
        sleep(1);

        // Check termination conditions
        sem_wait(&shared->semaphore);
        int terminate = 0;
        if (shared->killed_resistance >= config.max_killed) {
            printf("[Main] Maximum killed resistance members reached (%d). Terminating simulation.\n", shared->killed_resistance);
            terminate = 1;
        }
        if (shared->injured_resistance >= config.max_injured) {
            printf("[Main] Maximum injured resistance members reached (%d). Terminating simulation.\n", shared->injured_resistance);
            terminate = 1;
        }

        // Check if agency time limit is reached
        time_t current_time = time(NULL);
        if (difftime(current_time, start_time) >= config.agency_time_limit) {
            printf("[Main] Agency time limit reached (%d seconds). Checking agency members' status.\n", config.agency_time_limit);
            if ((shared->killed_agency + shared->caught_agency) >= config.agency_members) {
                printf("[Main] All agency members have been killed or caught. Terminating simulation.\n");
                terminate = 1;
            }
        }

        if (terminate) {
            simulation_running = 0;
        }
        sem_post(&shared->semaphore);
    }

    // Terminate child processes
    kill(pid_civilian, SIGTERM);
    kill(pid_resistance, SIGTERM);
    kill(pid_agency, SIGTERM);
    kill(pid_visualization, SIGTERM);

    // Wait for child processes to terminate
    waitpid(pid_civilian, NULL, 0);
    waitpid(pid_resistance, NULL, 0);
    waitpid(pid_agency, NULL, 0);
    waitpid(pid_visualization, NULL, 0);

    // Destroy shared data
    destroy_shared_data(shared);

    printf("[Main] Simulation terminated.\n");
    return 0;
}
