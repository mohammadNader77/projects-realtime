// config.c
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Initialize default configuration
void initialize_default_config(Config *config) {
    config->num_generators = 5;
    config->num_calculators = 2;
    config->inspectors_type1 = 2;
    config->inspectors_type2 = 2;
    config->inspectors_type3 = 2;
    config->gen_interval_min = 1;
    config->gen_interval_max = 5;
    config->rows_min = 1000;
    config->rows_max = 10000;
    config->columns_min = 5;
    config->columns_max = 15;
    config->value_min = 1.0;
    config->value_max = 100.0;
    config->missing_percentage = 5.0;
    config->threshold_files_processed = 100;
    config->threshold_files_not_processed = 50;
    config->threshold_files_backup = 200;
    config->threshold_files_deleted = 100;
    config->runtime_limit_minutes = 60;
    config->type1_threshold_age = 10;
}

// Function to parse the configuration file
void parse_config(const char *filename, Config *config) {
    // Initialize with default values
    initialize_default_config(config);

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Config file not found. Using default values.\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Ignore comments and empty lines
        if (line[0] == '#' || strlen(line) < 3)
            continue;

        char key[50], value[50];
        if (sscanf(line, "%49[^=]=%49s", key, value) != 2)
            continue;

        if (strcmp(key, "file_generators") == 0)
            config->num_generators = atoi(value);
        else if (strcmp(key, "calculators") == 0)
            config->num_calculators = atoi(value);
        else if (strcmp(key, "inspectors_type1") == 0)
            config->inspectors_type1 = atoi(value);
        else if (strcmp(key, "inspectors_type2") == 0)
            config->inspectors_type2 = atoi(value);
        else if (strcmp(key, "inspectors_type3") == 0)
            config->inspectors_type3 = atoi(value);
        else if (strcmp(key, "gen_interval") == 0)
            sscanf(value, "%d,%d", &config->gen_interval_min, &config->gen_interval_max);
        else if (strcmp(key, "rows") == 0)
            sscanf(value, "%d,%d", &config->rows_min, &config->rows_max);
        else if (strcmp(key, "columns") == 0)
            sscanf(value, "%d,%d", &config->columns_min, &config->columns_max);
        else if (strcmp(key, "value_range") == 0)
            sscanf(value, "%f,%f", &config->value_min, &config->value_max);
        else if (strcmp(key, "missing_percentage") == 0)
            config->missing_percentage = atof(value);
        else if (strcmp(key, "threshold_files_processed") == 0)
            config->threshold_files_processed = atoi(value);
        else if (strcmp(key, "threshold_files_not_processed") == 0)
            config->threshold_files_not_processed = atoi(value);
        else if (strcmp(key, "threshold_files_backup") == 0)
            config->threshold_files_backup = atoi(value);
        else if (strcmp(key, "threshold_files_deleted") == 0)
            config->threshold_files_deleted = atoi(value);
        else if (strcmp(key, "runtime_limit_minutes") == 0)
            config->runtime_limit_minutes = atoi(value);
        else if (strcmp(key, "type1_threshold_age") == 0)
            config->type1_threshold_age = atoi(value);
    }

    fclose(file);
}
