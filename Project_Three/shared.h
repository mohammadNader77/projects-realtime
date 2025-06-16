// shared.h
#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>

#define MAX_SUSPECTS 100
#define MAX_SPYS 100
#define MAX_GROUPS 1000
#define MAX_AGENCY_MEMBERS 100
#define MAX_CAUGHT_RESISTANCE 1000
#define MAX_CAUGHT_AGENCY 100

typedef struct {
    int group_id;
    int has_spy;
    int is_military;
    int spy_time; // Time a spy has spent in the group (seconds)
    int current_member_count;
    
    // Positional data for visualization
    float x; // X-coordinate
    float y; // Y-coordinate
} ResistanceGroup;

// Structure to hold shared counters and flags
typedef struct {
    // Counters for resistance members
    int killed_resistance;
    int injured_resistance;
    int caught_resistance;

    // Counters for agency members
    int killed_agency;
    int injured_agency;
    int caught_agency;

    // Counters for arrests
    int total_arrests;
    int total_imprisoned;
    int total_released;

    // Counters for current agency members
    int current_agency_members;

    // Counters for total resistance groups
    int total_resistance_groups;

    // Data shared from resistance groups to agency
    int data_shared;

    // Lists for managing spies
    int spy_member_ids[MAX_SPYS];
    int spy_member_ids_count;

    int suspected_spies[MAX_SUSPECTS];
    int suspected_spies_count;

    // Resistance groups
    ResistanceGroup resistance_groups[MAX_GROUPS];
    int resistance_groups_count;

    // Agency members positional data for visualization
    float agency_positions[MAX_AGENCY_MEMBERS][2]; // [x, y]
    int agency_positions_count;

    // Caught resistance members' positions
    float caught_resistance_positions[MAX_CAUGHT_RESISTANCE][2];
    int caught_resistance_positions_count;

    // Caught agency members' positions
    float caught_agency_positions[MAX_CAUGHT_AGENCY][2];
    int caught_agency_positions_count;

    // Semaphores for synchronization
    sem_t semaphore; // For general shared data
    sem_t group_semaphore; // For group-specific data
} SharedData;

// Function to initialize shared data (returns a pointer to shared memory)
SharedData* init_shared_data();

// Function to destroy shared data
int destroy_shared_data(SharedData *data);

// Function to check if a member is suspected
int is_member_suspected(SharedData *shared, int member_id);

#endif
