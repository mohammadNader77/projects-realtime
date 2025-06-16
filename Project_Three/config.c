// config.c
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Function to parse the configuration file
int parse_config(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open configuration file");
        return -1;
    }

    // Initialize default values
    memset(config, 0, sizeof(Config));

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n')
            continue;

        // Parse key=value pairs
        char key[128];
        char value[128];
        if (sscanf(line, "%[^=]=%s", key, value) != 2)
            continue;

        // Assign values to the config structure
        if (strcmp(key, "agency_members") == 0)
            config->agency_members = atoi(value);
        else if (strcmp(key, "group_creation_interval") == 0)
            config->group_creation_interval = atoi(value);
        else if (strcmp(key, "group_size_min") == 0)
            config->group_size_min = atoi(value);
        else if (strcmp(key, "group_size_max") == 0)
            config->group_size_max = atoi(value);
        else if (strcmp(key, "military_group_percentage") == 0)
            config->military_group_percentage = atoi(value);
        else if (strcmp(key, "spy_infiltration_probability") == 0)
            config->spy_infiltration_probability = atof(value);
        else if (strcmp(key, "target_probability") == 0)
            config->target_probability = atof(value);
        else if (strcmp(key, "recovery_light") == 0)
            config->recovery_light = atoi(value);
        else if (strcmp(key, "recovery_severe") == 0)
            config->recovery_severe = atoi(value);
        else if (strcmp(key, "suspicion_threshold") == 0)
            config->suspicion_threshold = atof(value);
        else if (strcmp(key, "arrest_release_threshold") == 0)
            config->arrest_release_threshold = atof(value);
        else if (strcmp(key, "arrest_imprison_threshold") == 0)
            config->arrest_imprison_threshold = atof(value);
        else if (strcmp(key, "max_killed") == 0)
            config->max_killed = atoi(value);
        else if (strcmp(key, "max_injured") == 0)
            config->max_injured = atoi(value);
        else if (strcmp(key, "agency_time_limit") == 0)
            config->agency_time_limit = atoi(value);
    }

    fclose(file);
    return 0;
}
